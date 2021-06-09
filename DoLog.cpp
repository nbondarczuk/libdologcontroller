///////////////////////////////////////////////////////////////////////////////
//Copyright (c) 2014 Ericsson
//
//The copyright in this work is vested in Ericsson Telekommunikation GmbH
//(hereafter "Ericsson"). The information contained in this work (either in whole
//or in part) is confidential and must not be modified, reproduced, disclosed or
//disseminated to others or used for purposes other than that for which it is
//supplied, without the prior written permission of Ericsson. If this work or any
//part hereof is furnished to a third party by virtue of a contract with that
//party, use of this work by such party shall be governed by the express
//contractual terms between Ericsson, which is party to that contract and the
//said party.
//
//The information in this document is subject to change without notice and
//should not be construed as a commitment by Ericsson. Ericsson assumes no
//responsibility for any errors that may appear in this document. With the
//appearance of a new version of this document all older versions become
//invalid.
//
//All rights reserved.
//
// Program    : BAT++ UNDOLOG
// File       : DoLog.cpp
// Description: Implementation of the main classes for storing the SQL trace
//              as set of batches containing the SQL operations like INSERT,
//              UPDATE, DELETE, SELECT. The statements provide information about
//              changed entities. The initial state of the entities may be
//              restored by creation of the undo of the change operation.
// Author(s)  : Norbert Bondarczuk
// Created    : 2015-01-08
// Abstract   : Implementation of the main classes of UNDOLOG functionality
//
///////////////////////////////////////////////////////////////////////////////
/*
 * static char *SCCS_VERSION = "%I%";
 */

#include <string>
#include <iostream>
#include <map>
#include <list>
#include <stdexcept>
#include <sstream>
#include <fstream>

#include "DbConnect.hpp"

#include "DoLogTerminationHandler.hpp"
#include "DoLogComponentController.hpp"
#include "DoLogTrace.hpp"
#include "DoLogSqlValue.hpp"
#include "DoLog.hpp"

using namespace std;

namespace dolog
{

////////////////////////////////////////////////////////////////////////////////
// static objects section
////////////////////////////////////////////////////////////////////////////////

// instance of DoLog singleton, static class variable
DoLog *DoLog::sInstance = NULL;

// LRU batch key
static ColumnValueSet* sLastBatchKey = NULL;

// Oracle connection
static DbConnect sDbConnect;

////////////////////////////////////////////////////////////////////////////////
// data conversion functions
////////////////////////////////////////////////////////////////////////////////

string convertHostVariableUse2string (const HostVariableUse pUseCase)
{
    string s;
    switch (pUseCase)
    {
        case INIT:            s = string("INIT");            break;
        case KEY:             s = string("KEY");             break;
        case VALUE:           s = string("VALUE");           break;
        case END:             s = string("END");             break;
        case NOP:             s = string("NOP");             break;
        case VAR_REF_CHAR:    s = string("VAR_REF_CHAR");    break;
        case VAR_REF_INTEGER: s = string("VAR_REF_INTEGER"); break;
        case VAR_REF_SMALLINT:s = string("VAR_REF_SMALLINT");break;
        case VAR_REF_FLOAT:   s = string("VAR_REF_FLOAT");   break;
        case VAR_REF_DOUBLE:  s = string("VAR_REF_DOUBLE");  break;
        case VAR_REF_DATE:    s = string("VAR_REF_DATE");    break;
        case VAR_REF_VARCHAR: s = string("VAR_REF_VARCHAR"); break;
        case VAR_REF_LONG:    s = string("VAR_REF_LONG"); break;
    }

    return s;
}

string convertOperationType2string (const OperationType pType)
{
    string s;
    switch (pType)
    {
        case INSERT: s = string("INSERT"); break;
        case UPDATE: s = string("UPDATE"); break;
        case DELETE: s = string("DELETE"); break;
        case SELECT: s = string("SELECT"); break;
    }

    return s;
}

string convertOperationValueState2string (const OperationValueState pState)
{
    string s;
    switch (pState)
    {
        case PREOPVAL: s = string("PREOPVAL");  break;
        case POSTOPVAL:s = string("POSTOPVAL"); break;
    }

    return s;
}

////////////////////////////////////////////////////////////////////////////////
// ColumnValueSet
////////////////////////////////////////////////////////////////////////////////

ColumnValueSet::ColumnValueSet()
{
    TRACE(4, "ColumnValueSet::ColumnValueSet");
}

// deep destruction (pointers released) via virtual destructor
ColumnValueSet::~ColumnValueSet()
{
    TRACE(4, "ColumnValueSet::~ColumnValueSet");
    clear();
}

// deep delete as pointer can hold any type of SQLValue subclasses
void ColumnValueSet::clear()
{
    ColumnValueContainerIt it = mValueContainer.begin();

    while (it != mValueContainer.end())
    {
        delete (*it);
        ++it;
    }

    mValueContainer.clear();
}

// render xml format of each value (one line)
string ColumnValueSet::getXml()
{
    SqlValue *ptr;
    stringstream ss;
    ColumnValueContainerIt it = mValueContainer.begin();

    while (it != mValueContainer.end())
    {
        ptr = *it;
        ss << ptr->getXml();
        ss << "\n";
        ++it;
    }

    return ss.str();
}

// append a new value to the list
void ColumnValueSet::addValue(SqlValue *pValue)
{
    TRACE(4, "ColumnValueSet::add");
    mValueContainer.push_back(pValue);
}

// concise presentation of the whole list of labelled values
string ColumnValueSet::getDigest()
{
    static string sLabelValueSeparator = "/";
    static string sKeyValueSeparator   = ".";
    SqlValue *ptr;
    string keyDigest;
    ColumnValueContainerIt it = mValueContainer.begin();

    while (it != mValueContainer.end())
    {
        ptr = *it;
        keyDigest += ptr->getLabel();
        keyDigest += sLabelValueSeparator;
        keyDigest += ptr->getString();
        if (++it != mValueContainer.end())
        {
            keyDigest += sKeyValueSeparator;
        }
    }

    return keyDigest;
}

// deep copy assignement operator
ColumnValueSet& ColumnValueSet::operator=(ColumnValueSet& rhs)
{
    if (this != &rhs)
    {
        SqlValue *ptr;

        // destination object is cleaned before operation
        this->clear();

        // source object is not changed
        ColumnValueContainerConstIt it = rhs.mValueContainer.begin();
        while (it != rhs.mValueContainer.end())
        {
            ptr = *it;
            this->mValueContainer.push_back(ptr->clone()); // deep  copy
            ++it;
        }
    }

    return *this;
}

// deep append operator
ColumnValueSet& ColumnValueSet::operator+=(ColumnValueSet& rhs)
{
    if (this != &rhs)
    {
        SqlValue *ptr;
        // source object is not changed
        ColumnValueContainerConstIt it = rhs.mValueContainer.begin();
        while (it != rhs.mValueContainer.end())
        {
            ptr = *it;
            this->mValueContainer.push_back(ptr->clone()); // deep copy
            ++it;
        }
    }

    return *this;
}

// provide a list of columns
string ColumnValueSet::sqlColumnClause(string pSeparator)
{
    SqlValue *ptr;
    stringstream ss;
    ColumnValueContainerIt it = mValueContainer.begin();

    while (it != mValueContainer.end())
    {
        ptr = *it;
        ss << ptr->getLabel();
        if (++it != mValueContainer.end())
        {
            ss << pSeparator;
        }
    }

    return ss.str();
}

// provide a list of values of columns
string ColumnValueSet::sqlColumnValueClause(string pSeparator)
{
    SqlValue *ptr;
    stringstream ss;
    ColumnValueContainerIt it = mValueContainer.begin();

    while (it != mValueContainer.end())
    {
        ptr = *it;
        ss << ptr->getValue();
        if (++it != mValueContainer.end())
        {
            ss << pSeparator;
        }
    }

    return ss.str();
}

// provide list of columns with assignement of values
string ColumnValueSet::sqlColumnValueAssignClause(string pSeparator)
{
    SqlValue *ptr;
    stringstream ss;
    ColumnValueContainerIt it = mValueContainer.begin();

    while (it != mValueContainer.end())
    {
        ptr = *it;
        ss << ptr->getLabel();
        ss << " = ";
        ss << ptr->getValue();
        if (++it != mValueContainer.end())
        {
            ss << pSeparator;
        }
    }

    return ss.str();
}

// SqlValue factory: makes labeled values stored in labeled value sets (called from DOM decoder)
SqlValue* ColumnValueSet::sqlValue(std::string pTypeId,
                                   std::string pLabel,
                                   std::string pValue)
{
    TRACE(4, "ColumnValueSet::sqlValue");

    SqlValue *productValue = NULL;
    SqlValueType typeId = (SqlValueType)any2int(pTypeId);

    switch(typeId)
    {
        case SQL_CHAR_TYPEID:
            productValue = new SqlChar(pLabel, xmlEscCharDecode(typeId, pValue));
            break;

        case SQL_INTEGER_TYPEID:
            productValue = new SqlInteger(pLabel, pValue);
            break;

        case SQL_SMALLINT_TYPEID:
            productValue = new SqlSmallint(pLabel, pValue);
            break;

        case SQL_FLOAT_TYPEID:
            productValue = new SqlFloat(pLabel, pValue);
            break;

        case SQL_DOUBLE_TYPEID:
            productValue = new SqlDouble(pLabel, pValue);
            break;

        case SQL_DATE_TYPEID:
            productValue = new SqlDate(pLabel, pValue);
            break;

        case SQL_VARCHAR_TYPEID:
            productValue = new SqlVarchar(pLabel, xmlEscCharDecode(typeId, pValue));
            break;

        case SQL_LONG_TYPEID:
            productValue = new SqlLong(pLabel, xmlEscCharDecode(typeId, pValue));
            break;

        default: // invalid parameter value: out of bound
            throw(invalid_argument("Invalid TypeId value: " + pTypeId));
    }

    TRACE_MSG("SQL Value: " + productValue->getLabel() + "/" + productValue->getString());

    return productValue;
}

// same same but using more primitive input data (called from variadic log interface)
SqlValue* ColumnValueSet::sqlValue(HostVariableUse pUseCase,
                                   char*           pLabel,
                                   void*           pValueAny)
{
    TRACE(4, "ColumnValueSet::sqlValue");

    string label(pLabel);
    SqlValue *productValue = NULL;
    switch(pUseCase)
    {
        case VAR_REF_CHAR:
            productValue = new SqlChar(label, pValueAny);
            break;

        case VAR_REF_INTEGER:
            productValue = new SqlInteger(label, pValueAny);
            break;

        case VAR_REF_SMALLINT:
            productValue = new SqlSmallint(label, pValueAny);
            break;

        case VAR_REF_FLOAT:
            productValue = new SqlFloat(label, pValueAny);
            break;

        case VAR_REF_DOUBLE:
            productValue = new SqlDouble(label, pValueAny);
            break;

        case VAR_REF_DATE:
            productValue = new SqlDate(label, pValueAny);
            break;

        case VAR_REF_VARCHAR:
            productValue = new SqlVarchar(label, pValueAny);
            break;

        case VAR_REF_LONG:
            productValue = new SqlLong(label, pValueAny);
            break;

        default:
            throw(invalid_argument("Invalid HostVariableUse value: " + convertHostVariableUse2string(pUseCase)));
    }

    TRACE_MSG("SQL Value: " + productValue->getLabel() + "/" + productValue->getString());

    return productValue;
}

// label lookup returning the structure pointer
SqlValue* ColumnValueSet::findSqlValueByLabel(const string& pLabel)
{
    SqlValue *finding = NULL;
    SqlValue *ptr;
    string searchLabel(pLabel);
    ColumnValueContainerIt it = mValueContainer.begin();

    while (it != mValueContainer.end())
    {
        ptr = *it;
        if (searchLabel == ptr->getLabel())
        {
            finding = ptr;
            break;
        }
        ++it;
    }

    return finding;
}

// label lookup for value, in case of value not found return empty string
string ColumnValueSet::findValueByLabel(const string& pLabel)
{
    string finding;
    SqlValue *value = findSqlValueByLabel(pLabel);
    if (value != NULL)
    {
        finding = value->getValue();
    }

    return finding;
}

// return all labels of a labelled value set
void ColumnValueSet::getColumnLabelSet(StringVector& pLabelContainer)
{
    ColumnValueContainerIt it = mValueContainer.begin();

    while (it != mValueContainer.end())
    {
        SqlValue *ptr = *it;
        pLabelContainer.push_back(ptr->getLabel());
        ++it;
    }
}

// for value sets A and B: replace sql value of each member of A with sql value from B
void ColumnValueSet::reassignColumnValue(ColumnValueSet* pValueSet)
{
    TRACE(4, "Operation::reassignColumnValue");

    SqlValue *finding;
    ColumnValueContainerIt it = mValueContainer.begin();

    while (it != mValueContainer.end())
    {
        SqlValue *ptr = *it;
        string label = ptr->getLabel();
        finding = pValueSet->findSqlValueByLabel(label);
        if (NULL == finding)
        {
            string msg("Unable assign value to variable: " + label);
            TRACE_MSG(msg);
            throw(invalid_argument(msg));
        }
        else
        {
            delete ptr;
            *it = finding->clone();
        }
        ++it;
    }
}

bool ColumnValueSet::isEmpty()
{
    return mValueContainer.size() == 0;
}

////////////////////////////////////////////////////////////////////////////////
// Operation
// This is abstract class to be fully defined in derived sub-classes. They provide
// implementation of the specific operations as well as the value containers.
////////////////////////////////////////////////////////////////////////////////

Operation::Operation(string pEntity): mEntity(pEntity)
{
    TRACE(2, "Operation::Operation");
}

Operation::~Operation()
{
    TRACE(2, "Operation::~Operation");
    mKey.clear();
}

void Operation::addKey(SqlValue* pValue)
{
    TRACE(4, "Operation::addKey");
    mKey.addValue(pValue);
}

void Operation::addKeySet(ColumnValueSet* pValueSet)
{
    TRACE(4, "Operation::addKeySet");
    mKey = *pValueSet;
}

// the operation must know to which batch it belongs as in UPDATE case
// it has to look for matching SELECT
void Operation::setBatch(Batch* pBatch)
{
    mMyBatch = pBatch;
}

////////////////////////////////////////////////////////////////////////////////
// OperationInsert
////////////////////////////////////////////////////////////////////////////////

OperationInsert::OperationInsert(string pEntity): Operation(pEntity)
{
    TRACE(2, "OperationInsert::OperationInsert");
}

OperationInsert::~OperationInsert()
{
    TRACE(2, "OperationInsert::~OperationInsert");
    mValueAfter.clear();
}

// provide the XML REDO for this batch of operations
string OperationInsert::getXmlRedo()
{
    stringstream ss;
    ss << "<INSERT>\n";
    ss << "<ENTITY>" << mEntity << "</ENTITY>\n";
    ss << "<KEY>\n";
    ss << mKey.getXml();
    ss << "</KEY>\n";
    ss << "<VALUE>\n";
    ss << mValueAfter.getXml();
    ss << "</VALUE>\n";
    ss << "</INSERT>\n";
    return ss.str();
}

// provide the XML UNDO (transformation is done) for this batch of operations
string OperationInsert::getXmlUndo()
{
    TRACE(4, "OperationInsert::getXmlUndo");

    stringstream ss;
    ss << "<DELETE>\n";
    ss << "<ENTITY>" << mEntity << "</ENTITY>\n";
    ss << "<KEY>\n";
    ss << mKey.getXml();
    ss << "</KEY>\n";
    ss << "<VALUE>\n";
    ss << mValueAfter.getXml();
    ss << "</VALUE>\n";
    ss << "</DELETE>\n";
    return ss.str();
}

// for Insert the only sensible value is the one after the operation
void OperationInsert::addValue(SqlValue*           pValue,
                               OperationValueState pState)
{
    TRACE(4, "OperationInsert::addValue");

    switch(pState)
    {
        case POSTOPVAL:
            mValueAfter.addValue(pValue);
            break;

        default:
            throw invalid_argument("Incompatible state selected");
    }
}

// add the whole set of values deep assignement operator (overloaded)
void OperationInsert::addValueSet(ColumnValueSet* pValueBefore,
                                  ColumnValueSet* pValueAfter)
{
    TRACE(4, "OperationInsert::addValueSet");
    if (pValueAfter)
    {
        mValueAfter = *pValueAfter;
    }
}

// render SQL statement to be executed upon UNDO or REDO
string OperationInsert::sqlStatementText()
{
    stringstream ss;

    ss << "INSERT INTO " << mEntity;
    ss << " (" << mKey.sqlColumnClause(",") << "," << mValueAfter.sqlColumnClause(",") << ") ";
    ss << "VALUES";
    ss << " (" << mKey.sqlColumnValueClause(",") << "," << mValueAfter.sqlColumnValueClause(",") << ")";

    return ss.str();
}

// the only sensible value set is the one specific for Insert operation
ColumnValueSet* OperationInsert::getValueSet()
{
    return &mValueAfter;
}

// match the operation by type & entity
bool OperationInsert::isTypeEntityMatch(OperationType pType,
                                        string        pEntity)
{
    if (pType == INSERT && pEntity == mEntity)
    {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// OperationDelete
////////////////////////////////////////////////////////////////////////////////

OperationDelete::OperationDelete(string pEntity): Operation(pEntity)
{
    TRACE(2, "OperationDelete::OperationDelete");
}

OperationDelete::~OperationDelete()
{
    TRACE(2, "OperationDelete::~OperationDelete");
    mValueBefore.clear();
}

// provide the XML REDO for this batch of operations
string OperationDelete::getXmlRedo()
{
    stringstream ss;

    ss << "<DELETE>\n";
    ss << "<ENTITY>" << mEntity << "</ENTITY>\n";
    ss << "<KEY>\n";
    ss << mKey.getXml();
    ss << "</KEY>\n";
    ss << "<VALUE>\n";
    ss << mValueBefore.getXml();
    ss << "</VALUE>\n";
    ss << "</DELETE>\n";

    return ss.str();
}

// provide the XML UNDO (transformation is done) for this batch of operations
string OperationDelete::getXmlUndo()
{
    TRACE(4, "OperationDelete::getXmlUndo");

    stringstream ss;

    ss << "<INSERT>\n";
    ss << "<ENTITY>" << mEntity << "</ENTITY>\n";
    ss << "<KEY>\n";
    ss << mKey.getXml();
    ss << "</KEY>\n";
    ss << "<VALUE>\n";
    ss << mValueBefore.getXml();
    ss << "</VALUE>\n";
    ss << "</INSERT>\n";

    return ss.str();
}

// for Delete the only sensible value is the one before the operation
void OperationDelete::addValue(SqlValue*           pValue,
                               OperationValueState pState)
{
    TRACE(4, "OperationDelete::addValue");

    switch(pState)
    {
        case PREOPVAL:
            mValueBefore.addValue(pValue);
            break;

        default:
            throw invalid_argument("Incompatible state selected");
    }
}

// add the whole set of values deep assignement operator (overloaded)
void OperationDelete::addValueSet(ColumnValueSet* pValueBefore,
                                  ColumnValueSet* pValueAfter)
{
    TRACE(4, "OperationDelete::AddValueSet");
    if (pValueBefore)
    {
        mValueBefore = *pValueBefore;
    }
}

// render SQL statement to be executed upon UNDO or REDO
string OperationDelete::sqlStatementText()
{
    stringstream ss;

    ss << "DELETE FROM " << mEntity << " WHERE ";
    ss << mKey.sqlColumnValueAssignClause(" AND ");
    ss << " AND ";
    ss << mValueBefore.sqlColumnValueAssignClause(" AND ");

    return ss.str();
}

ColumnValueSet* OperationDelete::getValueSet()
{
    return &mValueBefore;
}

// match the operation by type & entity
bool OperationDelete::isTypeEntityMatch(OperationType pType,
                                        string pEntity)
{
    if (pType == DELETE && pEntity == mEntity)
    {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// OperationUpdate
////////////////////////////////////////////////////////////////////////////////

OperationUpdate::OperationUpdate(string pEntity): Operation(pEntity)
{
    TRACE(2, "OperationUpdate::OperationUpdate");
}

OperationUpdate::~OperationUpdate()
{
    TRACE(2, "OperationUpdate::~OperationUpdate");
    mValueBefore.clear();
    mValueAfter.clear();
}

// provide the XML REDO for this batch of operations
string OperationUpdate::getXmlRedo()
{
    stringstream ss;

    ss << "<UPDATE>\n";
    ss << "<ENTITY>" << mEntity << "</ENTITY>\n";
    ss << "<KEY>\n";
    ss << mKey.getXml();
    ss << "</KEY>\n";
    ss << "<VALUE>\n";
    ss << "<BEFORE>\n";
    ss << mValueBefore.getXml();
    ss << "</BEFORE>\n";
    ss << "<AFTER>\n";
    ss << mValueAfter.getXml();
    ss << "</AFTER>\n";
    ss << "</VALUE>\n";
    ss << "</UPDATE>\n";

    return ss.str();
}

// provide the XML UNDO (transformation is done) for this batch of operations
// the operation is specific as the values for it may be registered in a separate
// SELECT operation or they may be provided as a separate VALUE block.
string OperationUpdate::getXmlUndo()
{
    TRACE(4, "OperationUpdate::getXmlUndo");

    stringstream    ss;

    // try to find earliest SELECT registration
    ColumnValueSet* selectValue = mMyBatch->findFirstBatchOperation(SELECT, mEntity);
    if (selectValue == NULL && mValueBefore.isEmpty())
    {
        string msg("SELECT not done for UPDATE on entity " + mEntity + ", no previous state infor provided");
        TRACE_MSG(msg);
        throw(invalid_argument(msg));
    }
    else if (mValueBefore.isEmpty())
    {
        mValueBefore = mValueAfter;
        mValueBefore.reassignColumnValue(selectValue);
    }

    ss << "<UPDATE>\n";
    ss << "<ENTITY>" << mEntity << "</ENTITY>\n";
    ss << "<KEY>\n";
    ss << mKey.getXml();
    ss << "</KEY>\n";
    ss << "<VALUE>\n";
    ss << mValueBefore.getXml();
    ss << "</VALUE>\n";
    ss << "</UPDATE>\n";

    return ss.str();
}

// for Update the allowed values are both before and after operation
void OperationUpdate::addValue(SqlValue* pValue,
                               OperationValueState pState)
{
    TRACE(4, "OperationUpdate::addValue");

    switch(pState)
    {
        case PREOPVAL:
            mValueBefore.addValue(pValue);
            break;

        case POSTOPVAL:
            mValueAfter.addValue(pValue);
            break;

        default:
            throw invalid_argument("Invalid state selected");
    }
}

// add the whole set of values deep assignement operator (overloaded)
void OperationUpdate::addValueSet(ColumnValueSet* pValueBefore,
                                  ColumnValueSet* pValueAfter)
{
    TRACE(4, "OperationUpdate::addValueSet");

    if (pValueBefore)
    {
        mValueBefore = *pValueBefore;
    }

    if (pValueAfter)
    {
        mValueAfter = *pValueAfter;
    }
}

// render SQL statement to be executed upon UNDO or REDO
string OperationUpdate::sqlStatementText()
{
    stringstream ss;

    ss << "UPDATE " << mEntity;
    ss << " SET ";
    ss << mValueAfter.sqlColumnValueAssignClause(",");
    ss << " WHERE ";
    ss << mKey.sqlColumnValueAssignClause(" AND ");

    return ss.str();
}

ColumnValueSet* OperationUpdate::getValueSet()
{
    return &mValueBefore;
}

// match the operation by type & entity
bool OperationUpdate::isTypeEntityMatch(OperationType pType,
                                        string        pEntity)
{
    if (pType == UPDATE && pEntity == mEntity)
    {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// OperationSelect
////////////////////////////////////////////////////////////////////////////////

OperationSelect::OperationSelect(string pEntity): Operation(pEntity)
{
    TRACE(2, "OperationSelect::OperationSelect");
}

OperationSelect::~OperationSelect()
{
    TRACE(2, "OperationSelect::~OperationSelect");
    mValueBefore.clear();
}

// operation not eligible for UNDO or REDO
string OperationSelect::getXmlRedo()
{
    TRACE(4, "OperationSelect::getXmlRedo");

    return string("");
}

// operation not eligible for UNDO or REDO
string OperationSelect::getXmlUndo()
{
    TRACE(4, "OperationSelect::getXmlUndo");

    return string("");
}

// for Select the allowed values are only before operation
void OperationSelect::addValue(SqlValue*           pValue,
                               OperationValueState pState)
{
    TRACE(4, "OperationSelect::addValue");

    switch(pState)
    {
        case PREOPVAL:
            mValueBefore.addValue(pValue);
            break;

        default:
            throw invalid_argument("Incompatible state selected");
    }
}

// for Select the only sensible values are the values before operation
void OperationSelect::addValueSet(ColumnValueSet* pValueBefore,
                                  ColumnValueSet* pValueAfter)
{
    TRACE(4, "OperationSelect::addValueSet");

    if (pValueBefore)
    {
        mValueBefore = *pValueBefore;
    }
}

// render SQL statement to be executed upon UNDO or REDO
string OperationSelect::sqlStatementText()
{
    stringstream ss;

    ss << "SELECT " << mValueBefore.sqlColumnClause(",");
    ss << " FROM " << mEntity;
    ss << " WHERE ";
    ss << mKey.sqlColumnValueClause(" AND ") << " AND " << mValueBefore.sqlColumnValueClause(" AND ");

    return ss.str();
}

ColumnValueSet* OperationSelect::getValueSet()
{
    return &mValueBefore;
}

// match the operation by type & entity
bool OperationSelect::isTypeEntityMatch(OperationType pType,
                                        string pEntity)
{
    if (pType == SELECT && pEntity == mEntity)
    {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// Batch
////////////////////////////////////////////////////////////////////////////////

Batch::Batch(string pDigest,
             ColumnValueSet* pBatchKey) : mDigest(pDigest), mBatchKey(pBatchKey)
{
    TRACE(2, "Batch::Batch");
}

// deep release as batches are registered with a pointer to heap memory block
Batch::~Batch()
{
    TRACE(2, "Batch::~Batch");

    Operation* operation;
    for(OperationListIt it = mOperation.begin(); it != mOperation.end(); ++it)
    {
        operation = *it;
        delete operation;
    }
    mOperation.clear();
}

// provide the XML REDO for this batch of operations
string Batch::getXmlRedo()
{
    Operation* operation;
    stringstream ss;

    for(OperationListIt it = mOperation.begin(); it != mOperation.end(); ++it)
    {
        operation = *it;
        ss << operation->getXmlRedo();
    }

    return ss.str();
}

// provide the XML UNDO (transformation is done) for this batch of operations
string Batch::getXmlUndo()
{
    TRACE(4, "Batch::getXmlUndo");

    Operation* operation;
    stringstream ss;

    for(OperationListRevIt it = mOperation.rbegin(); it != mOperation.rend(); ++it)
    {
        operation = *it;
        ss << operation->getXmlUndo();
    }

    return ss.str();
}

// execute all SQL statements from the container provided
void Batch::sqlStatementTextAll(StringVector& pSqlTextContainer)
{
    for(OperationListIt it = mOperation.begin(); it != mOperation.end(); ++it)
    {
        stringstream ss;
        Operation* operation = *it;
        ss << operation->sqlStatementText();
        pSqlTextContainer.push_back(ss.str());
    }
}

// search the given batch for first operation on type & entity
// but only for the povided column names of the key
// if found return pointer
// if not found return NULL
ColumnValueSet* Batch::findFirstBatchOperation(OperationType pType,
                                               string        mEntity)
{
    ColumnValueSet* finding = NULL;

    for(OperationListIt it = mOperation.begin(); it != mOperation.end(); ++it)
    {
        Operation* operation = *it;
        if (operation->isTypeEntityMatch(pType, mEntity))
        {
            finding = operation->getValueSet();
            break;
        }
    }

    return finding;
}

ColumnValueSet* Batch::getBatchKey()
{
    return mBatchKey;
}


////////////////////////////////////////////////////////////////////////////////
// DoLog
////////////////////////////////////////////////////////////////////////////////

// default mode: file processing, no DB needed
DoLog::DoLog() : mHandleDbConnect(false), mDbHandle("")
{
    TRACE(1, "DoLog::DoLog");
}

// deep release whole memory for all batches regstered
DoLog::~DoLog()
{
    TRACE(1, "DoLog::~DoLog");
    clean();
}

// singleton idiom: only one instance of the object
DoLog* DoLog::getInstance()
{
    if (!sInstance)
    {
        sInstance = new DoLog;
    }

    return sInstance;
}

// deep release of memory of all batches from the container
void DoLog::clean()
{
    TRACE(1, "DoLog::Clean");
    for(BatchContainerIt it = mBatchContainer.begin(); it != mBatchContainer.end(); ++it)
    {
        delete it->second;
    }
    mBatchContainer.clear();
}

// find batch key by digest string
ColumnValueSet* DoLog::findBatchKey(string& pSearchDigest)
{
    ColumnValueSet* foundKey = NULL;

    BatchContainerIt it = mBatchContainer.find(pSearchDigest);
    if (it != mBatchContainer.end())
    {
        // operation indexed by the key was already found in the map
        Batch* batch = it->second;
        foundKey = batch->mBatchKey;
    }

    return foundKey;
}

// build new batch and init it with first value
Batch* DoLog::addBatch(string&         pSearchDigest,
                       ColumnValueSet* pKey)
{
    Batch* batch = new Batch(pSearchDigest, pKey);
    mBatchContainer.insert(pair<string, Batch*>(pSearchDigest, batch));
    return batch;
}

// operations factory: produces operations stored in indexed batches
Operation* DoLog::sqlOperation(ColumnValueSet* pBatchKey,
                               OperationType   pOperationType,
                               string          pEntity)
{
    TRACE(2, "DoLog::sqlOperation");

    // always new operation to be produced by the factory upon call

    TRACE_MSG(convertOperationType2string(pOperationType) + " -> " + pEntity);
    Operation* operation;
    switch(pOperationType)
    {
        case INSERT:
            operation = new OperationInsert(pEntity);
            break;

        case DELETE:
            operation = new OperationDelete(pEntity);
            break;

        case UPDATE:
            operation = new OperationUpdate(pEntity);
            break;

        case SELECT:
            operation = new OperationSelect(pEntity);
            break;

        default: throw invalid_argument("Wrong operation type, only INSERT, DELETE, UPDATE, SELECT allowed");
    }

    // create new or reuse existing batch
    Batch* batch;
    string batchKeyDigest = pBatchKey->getDigest();
    BatchContainerIt it = mBatchContainer.find(batchKeyDigest);
    if (it == mBatchContainer.end())
    { // new operation for a given key
        batch = addBatch(batchKeyDigest, pBatchKey);
    }
    else
    {   // operation indexed by the key was already found in the map
        batch = it->second;
    }

    // the operations must be kept sorted by priority of the entities
    batch->mOperation.push_back(operation);

    // linking the operation with it's batch (usefull in search for own operations)
    operation->setBatch(batch);

    return operation;
}

// get  the XML image of REDO operations for all batch containers registered
string DoLog::getXmlRedo()
{
    TRACE(1, "DoLog::getXmlRedo");

    stringstream ss;

    ss << "<REDOLOG>\n";
    for(BatchContainerIt it = mBatchContainer.begin(); it != mBatchContainer.end(); ++it)
    {
        Batch *batch = it->second;
        ss << "<BATCH>\n";
        ss << "<DIGEST>" << it->first << "</DIGEST>\n";
        ss << "<KEY>\n";
        ss << batch->mBatchKey->getXml();
        ss << "</KEY>\n";
        ss << batch->getXmlRedo();
        ss << "</BATCH>\n";
    }
    ss << "</REDOLOG>\n";

    return ss.str();
}

// get the XML image of UNDO operations (transformation is done) for all batch containers registered
string DoLog::getXmlUndo()
{
    TRACE(1, "DoLog::getXmlUndo");

    stringstream ss;

    ss << "<UNDOLOG>\n";
    for(BatchContainerIt it = mBatchContainer.begin(); it != mBatchContainer.end(); ++it)
    {
        Batch *batch = it->second;
        ss << "<BATCH>\n";
        ss << "<DIGEST>" << it->first << "</DIGEST>\n";
        ss << "<KEY>\n";
        ss << batch->mBatchKey->getXml();
        ss << "</KEY>\n";
        ss << batch->getXmlUndo();
        ss << "</BATCH>\n";
    }
    ss << "</UNDOLOG>\n";

    return ss.str();
}

// execute all SQL statements from the container
void DoLog::sqlStatementTextAll(StringVector& pSqlTextContainer)
{
    TRACE(1, "DoLog::sqlStatementTextAll");

    for(BatchContainerIt it = mBatchContainer.begin(); it != mBatchContainer.end(); ++it)
    {
        Batch* batch = it->second;
        batch->sqlStatementTextAll(pSqlTextContainer);
    }
}

// save all operations for all batches in one file
bool DoLog::save(const char* pFileName)
{
    TRACE(1, "DoLog::save");

    stringstream ss;

    try
    {
        ss << "<UNDOLOG>\n";
        for(BatchContainerIt it = mBatchContainer.begin(); it != mBatchContainer.end(); ++it)
        {
            Batch* batch = it->second;
            ss << "<BATCH>\n";
            ss << "<DIGEST>" << it->first << "</DIGEST>\n";
            ss << "<KEY>\n";
            ss << batch->mBatchKey->getXml();
            ss << "</KEY>\n";
            ss << batch->getXmlUndo();
            ss << "</BATCH>\n";
        }
        ss << "</UNDOLOG>\n";
    }
    catch (exception &e)
    {
        return ERROR("Exception caught while building XML: " + string(e.what()));
    }

    ofstream output;
    output.exceptions ( ofstream::failbit | ofstream::badbit );
    try
    {
        output.open (pFileName);
        output << ss.str();
        output.close();
    }
    catch (ofstream::failure& e)
    {
        return ERROR("Exception handling file: " + string(pFileName) + ", " + string(e.what()));
    }

    return true;
}

// save each batch to DB in a separate block
bool DoLog::save()
{
    TRACE(1, "DoLog::save");

    // DB operation status
    bool ok;
    // key values for the materialized XML record
    string customerId;
    string billSeqNo;
    string image;

    for(BatchContainerIt it = mBatchContainer.begin(); it != mBatchContainer.end(); ++it)
    {
        stringstream ss;
        Batch* batch = it->second;

        try
        {
            ss << "<UNDOLOG>\n";
            ss << "<BATCH>\n";
            ss << "<DIGEST>" << it->first << "</DIGEST>\n";
            ss << "<KEY>\n";
            ss << batch->mBatchKey->getXml();
            ss << "</KEY>\n";
            ss << batch->getXmlUndo();
            ss << "</BATCH>\n";
            ss << "</UNDOLOG>\n";

            image = ss.str();
        }
        catch (exception &e)
        {
            return ERROR("Exception caught while building XML, " + string(e.what()));
        }

        // optional values: may be not used in the key
        // in this case the empty string is returned
        customerId = batch->mBatchKey->findValueByLabel("CUSTOMER_ID");
        billSeqNo = batch->mBatchKey->findValueByLabel("BILLSEQNO");

        ok = dbLongVarcharInsert(image,
                                 batch->mDigest,
                                 customerId,
                                 billSeqNo);
        if (!ok)
        {
            return ERROR("Error inserting XML record: " + batch->mDigest);
        }
    }

    return true;
}

// load XML from file
bool DoLog::load(const char* pFileName)
{
    TRACE(1, "DoLog::load" );

    stringstream ss;
    ifstream input;
    string s;

    input.exceptions ( ifstream::failbit | ifstream::badbit );
    try
    {
        input.open (pFileName);
        ss << input.rdbuf();
        input.close();
    }
    catch (ofstream::failure &e)
    {
        return ERROR("Exception handling file: " + string(pFileName) + ", " + string(e.what()));
    }

    s = ss.str();
    try
    {
        xmlParse((unsigned char *)s.c_str(), (size_t)s.length());
    }
    catch (exception &e)
    {
        return ERROR("Exception parsing XML: " + string(e.what()));
    }
    catch (...)
    {
        return ERROR("Exception parsing XML");
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Interface functions
////////////////////////////////////////////////////////////////////////////////

//
// Init library for specific connection type, may be used in following modes:
// 1. Standalone: explicit DBNAME given as well as the full login info
// 2. External: DBNAME is NULL but it can give the specific connection used by environment
// Log level is an default value 0, may be not given in the invocation
//

void logUndoInit(const char* pDbName,
                 const char* pDbUser,
                 const char* pDbPass,
                 const char* pDbConnectionId,
                 const int   pLogLevel)
{
    TRACE(2, "logUndoInit");

    if (pDbConnectionId == NULL)
    {
        DoLog::getInstance()->mDbHandle = strdup("UNDO");
    }
    else // requested specific connection id but not necessarilly standalone log in
    {
        DoLog::getInstance()->mDbHandle = strdup(pDbConnectionId);
    }

    // requested standlone connection
    if (pDbName != NULL)
    {
        DoLog::getInstance()->mHandleDbConnect = true;
        DoLog::getInstance()->mDbUserName = string(pDbUser);
        sDbConnect.connect(pDbName,
                           pDbUser,
                           pDbPass,
                           DoLog::getInstance()->mDbHandle);
        TRACE_MSG("Connected to Oracle DB: " + string(pDbName));
    }
    else
    {
        DoLog::getInstance()->mHandleDbConnect = false;
    }

    // requested specific log level, default value 0
    Trace::setLevel(pLogLevel);
}

//
// Register new batch or use the existing one from the previously allocated batch
//

void logUndoBatch(int pCustomerId,
                  int pBillSeqNo)
{
    TRACE(2, "logUndoBatch");

    // build a key for parameters
    ColumnValueSet* batchKey = new ColumnValueSet;
    SqlValue* keyValueBillSeqNo = new SqlInteger("BILLSEQNO", pBillSeqNo);
    batchKey->addValue(keyValueBillSeqNo);
    SqlValue* keyValueCustomerId = new SqlInteger("CUSTOMER_ID", pCustomerId);
    batchKey->addValue(keyValueCustomerId);

    // try find batch by provided key (may be it will be previously used)
    string searchDigest = batchKey->getDigest();
    ColumnValueSet* finding = DoLog::getInstance()->findBatchKey(searchDigest);
    if (finding)
    {
        // already there -> use its key for all subsequent operations
        sLastBatchKey = finding;
        delete batchKey;
    }
    else // not found -> create empty batch storing pointer to its key
    {
        Batch* batch = DoLog::getInstance()->addBatch(searchDigest, batchKey);
        sLastBatchKey = batch->getBatchKey();
    }
}

//
// Variadic function interface: state of va_list entries in arg:
// INIT -> KEY -> {host variable ref}+ -> {VALUE {host variable ref}+}+ -> END
//

void logUndo(const OperationType pOperationType,
             const char*         pEntity,
             ...)
{
    TRACE(2, "logUndo");

    int                  argumentId = 0;    // id of currently analyzed argument
    va_list              list;              // list of arguments to be analyzed
    bool                 end = false;       // is end of argument read loop reached
    HostVariableUse      state = INIT;      // arg 1 - host variable type or code (no follow up)
    HostVariableUse      useCase;           // parameter id form argument list
    char*                hostVariableLabel; // arg 2 - host variable label
    void*                hostVariablePtr;   // arg 3 - host variable pointer
    ColumnValueSet*      lastUsedValueSet = NULL;// last used seq of labelled attributes, may be key or value
    ColumnValueSet       keySet;            // key to be filled up by current triplets
    ColumnValueSet       valueSetFirst;     // value pre op
    ColumnValueSet       valueSetSecond;
    bool                 isUseVariableRead   = false;
    bool                 isUseKeyFound       = false;
    bool                 isUseValueFound     = false;
    int                  valueSectionCounter = 0;

    //
    // process each argument tuple of format <USE, ...>
    // where ... is the list of parameters for the USE trigger
    //
    va_start(list, pEntity);
    while (!end)
    {
        hostVariableLabel = NULL;
        hostVariablePtr = NULL;
        useCase = (HostVariableUse)va_arg(list, int);
        TRACE_MSG("[" + any2string(argumentId) + "] " + "Arg Type: " + convertHostVariableUse2string(useCase));

        switch(useCase)
        {
            case KEY:
                isUseKeyFound = true;
                isUseVariableRead = false;
                state = KEY;
                lastUsedValueSet = &keySet;
                break;

            case VALUE:
                isUseValueFound = true;
                valueSectionCounter++;
                isUseVariableRead = false;
                if (&keySet == lastUsedValueSet)
                {
                    lastUsedValueSet = &valueSetFirst;
                }
                else if (&valueSetFirst == lastUsedValueSet)
                {
                    lastUsedValueSet = &valueSetSecond;
                }

                break;

            case END:
                isUseVariableRead = false;
                state = END;
                end = true;
                break;

            case NOP:
                isUseVariableRead = false;
                break;

            case VAR_REF_CHAR:     // FALLTHROUGH
            case VAR_REF_INTEGER:  // FALLTHROUGH
            case VAR_REF_SMALLINT: // FALLTHROUGH
            case VAR_REF_FLOAT:    // FALLTHROUGH
            case VAR_REF_DOUBLE:   // FALLTHROUGH
            case VAR_REF_DATE:     // FALLTHROUGH
            case VAR_REF_VARCHAR:  // FALLTHROUGH
            case VAR_REF_LONG:     // Finally
                isUseVariableRead = true;
                hostVariableLabel = va_arg(list, char *);
                TRACE_MSG("[" + any2string(argumentId) + "] " + "Label: " + hostVariableLabel);
                hostVariablePtr = va_arg(list, void *);
                TRACE_MSG("[" + any2string(argumentId) + "] " + "HostVariable: " + convertHostVariableUse2string(useCase));
                break;

            default:
                end = true;
                break;
        }

        if (isUseVariableRead)
        {
            if (hostVariableLabel &&
                hostVariablePtr) // got label and some (untyped yet) value?
            {
                // decode value using factory and register it as xml string
                SqlValue* value = lastUsedValueSet->sqlValue(useCase,
                                                             hostVariableLabel,
                                                             hostVariablePtr);
                if (value)
                {
                    lastUsedValueSet->addValue(value);
                }
                else // problem with decoding value
                {
                    throw(invalid_argument("Unable decode value"));
                }

                hostVariableLabel = NULL;
                hostVariablePtr = NULL;
            }
            else // wrong (missing) parameters of the SQL variable
            {
                throw(invalid_argument("Missing label or value"));
            }
        }

        argumentId++;
    }
    va_end(list);

    // mandatory final state
    if (END != state)
    {
        throw(invalid_argument("Invalid state reached, expecting END"));
    }
    else // END state reached in arg list parsing
    {
        // register operation with all required fields in the heap
        // returnning pointer to allocated area
        // REMARK: no need to release it here, it will happen in flush phase
        if (!sLastBatchKey)
        {
            throw(invalid_argument("Batch not initialized"));
        }

        if (!isUseKeyFound)
        {
            throw(invalid_argument("KEY section not defined"));
        }

        if (!isUseValueFound)
        {
            throw(invalid_argument("VALUE section not defined"));
        }

        Operation* operation = DoLog::getInstance()->sqlOperation(sLastBatchKey, pOperationType, pEntity);
        operation->addKeySet(&keySet);
        //
        // Each operation on an entity has pecific mandatory value set:
        // 1. INSERT: PreOperationValue = NULL, PostOperationValue
        // 2. DELETE: PreOperationValue,        PostOperationValue = NULL
        // 3. UPDATE: PreOperationValue = NULL, PostOperationValue (there must be matching SELECT)
        //            PreOperationValue,        PostOperationValue
        // 4. SELECT: PreOperationValue,        PostOperationValue = NULL
        switch (pOperationType)
        {
            case INSERT:
                if (valueSectionCounter != 1)
                {
                    throw(invalid_argument("VALUE section used too many times or incorrectly"));
                }
                else
                {
                    operation->addValueSet(NULL, &valueSetFirst);
                }
                break;

            case DELETE:
                if (valueSectionCounter != 1)
                {
                    throw(invalid_argument("VALUE section used too many times or not at all"));
                }
                else
                {
                    operation->addValueSet(&valueSetFirst, NULL);
                }
                break;

            case UPDATE:
                if (valueSectionCounter == 1) // only once VALUE used, SELECT mandatory
                {
                    operation->addValueSet(NULL, &valueSetFirst);
                }
                else if (valueSectionCounter == 2) // VALUE used twice, SELECT optional
                {
                    operation->addValueSet(&valueSetFirst, &valueSetSecond);
                }
                else
                {
                    throw(invalid_argument("VALUE section used too many times or not at all"));
                }
                break;

            case SELECT:
                if (valueSectionCounter != 1)
                {
                    throw(invalid_argument("VALUE section used too many times or not at all"));
                }
                else
                {
                    operation->addValueSet(&valueSetFirst, NULL);
                }
                break;

            default:
                throw(invalid_argument("Invalid type of operation"));
        }

        TRACE_MSG("[" + any2string(argumentId) + "] "
                  + convertOperationType2string(pOperationType)
                  + "/"
                  + pEntity);
    }

    // here happens automatic destruction of the key & value handlers
}

//
// Flush cache saving log in the DB: close to the commit point
// If the environment handles the connection then it has to take care of commit point.
//

void logUndoFlush()
{
    TRACE(2, "logUndoFlush");

    DoLog::getInstance()->save();
    DoLog::getInstance()->clean();

    // the user handles commit point
    if (DoLog::getInstance()->mHandleDbConnect)
    {
        sDbConnect.commit();
        TRACE_MSG("Commit done on DB connection " + string(DoLog::getInstance()->mDbHandle));
    }

    // no batch to process via variadic function
    sLastBatchKey = NULL;
}

}
