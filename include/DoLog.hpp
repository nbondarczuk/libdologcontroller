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
// File       : DoLog.hpp
// Description: Provides declaration of the class DoLog for UNDO of SQL operations.
//              The dependent classes are as well declared here.
// Author(s)  : Norbert Bondarczuk
// Created    : 2015-01-08
// Abstract   : Provides declaration of the class DoLog for UNDO of SQL operations.
//
///////////////////////////////////////////////////////////////////////////////
/*
 * static char *SCCS_VERSION = "%I%";
 */

#ifndef DoLog_hpp
#define DoLog_hpp

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <stdexcept>
#include <sstream>

#include <stdarg.h>

namespace dolog
{

///////////////////////////////////////////////////////////////////////////////
// SQL mapping
///////////////////////////////////////////////////////////////////////////////

//
// Usage type for specific variable type or section separator.
// The usage type code determines the usage of the next argument of logUndo function.
// Two separate sections in the type enum values, one for usage codes and one
// for host variable type is done one purpose so that no two separate types
// are to be introduced.
//
// Example:
//       logUndo(INSERT,
//               "ENTITY_NAME",
//               KEY,
//               VAR_REF_INTEGER, "CUSTOMER_ID",      &ival,
//               VALUE,
//               VAR_REF_DATE,    "BCH_RUN_DATE",     cval,
//               VAR_REF_INTEGER, "CONTRACT_NUM",     &ival,
//               VAR_REF_DOUBLE,  "BILLING_DURATION", &dval,
//               VAR_REF_CHAR,    "BILLING_ECHO",     sval,
//               VAR_REF_LONG,    "LONGVALUE",        &lval,
//               END);
//
typedef enum HostVariableUse
{
    // Position markers in argument section: determine the interpretation of
    // next sequence of arguments which always has to have an END marker as the
    // number of parameters is not used a la moe of unix main(argn, argv) function.
    INIT    = 0,
    KEY     = 1,
    VALUE   = 2,
    END     = 3,
    NOP     = 4,
    // Allowed variable uses after KEY or VALUE position marker. The following
    // 2 arguments must be: entity label, pointer to host variable,which is
    VAR_REF_CHAR            = 5, // pointer to 0 terminated char sequence
    VAR_REF_INTEGER         = 6, // pointer to int
    VAR_REF_SMALLINT        = 7, // pointr to short
    VAR_REF_FLOAT           = 8, // pointer to float
    VAR_REF_DOUBLE          = 9, // pointer to double
    VAR_REF_DATE            = 10,// pointer to 0 terminated char sequence
    VAR_REF_VARCHAR         = 11,// pointer to Oracle type varchar struct (len, arr)
    VAR_REF_LONG            = 12 // pointer to long

} HostVariableUse;

//
// OperationType - DB operations handled by the DoLog class
//
typedef enum OperationType
{
    INSERT = 0,
    UPDATE = 1,
    DELETE = 2,
    SELECT = 3

} OperationType;

//
// ValueState - status of the value for an operation: post or pre op value.
// It determines the contents of the value containers for each of the sub-classes
// of class Operation. The exception is SELECT where the value Before and After
// does not get changed so no need to implement it with two separate container.
//
//        | Has Pre Op Value | Has Post Op Value
// ----------------------------------------------
// INSERT |       No         |        Yes
// UPDATE |       Yes        |        Yes
// DELETE |       Yes        |        No
// SELECT |       Yes        |        Yes
//
typedef enum OperationValueState
{
    PREOPVAL  = 0,
    POSTOPVAL = 1

} OperationValueState;

//
// The type presentation functions
//
std::string convertHostVariableUse2string(const HostVariableUse t);
std::string convertOperationType2string(const OperationType t);
std::string convertOperationValueState2string(const OperationValueState s);

typedef std::vector<std::string> StringVector;

// forward declaration
class Batch;

///////////////////////////////////////////////////////////////////////////////
// ColumnValueSet - set of typed values with columns naming them. The values
// are polymorfic. Their presentation in XML depends on their type. Internally
// all value is however already reprentat as a string prepared to be serialized
// in XML stream.
///////////////////////////////////////////////////////////////////////////////

typedef std::vector<SqlValue *>::iterator ColumnValueContainerIt;
typedef std::vector<SqlValue *>::const_iterator ColumnValueContainerConstIt;

class ColumnValueSet
{
public:
    ColumnValueSet();
    ~ColumnValueSet();
    void              clear();
    std::string       getXml();
    void              addValue(SqlValue* pValue);
    std::string       getDigest();
    std::string       sqlColumnClause(std::string pSeparator);
    std::string       sqlColumnValueClause(std::string pSeparator);
    std::string       sqlColumnValueAssignClause(std::string pSeparator);
    void              getColumnLabelSet(StringVector& pLabelContainer);
    void              reassignColumnValue(ColumnValueSet* pValueSet);
    std::string       findValueByLabel(const std::string& pLabel);
    bool              isEmpty();
    SqlValue*         findSqlValueByLabel(const std::string& pLabel);
    SqlValue*         sqlValue(std::string pTypeId,      // object factory
                               std::string pLabel,
                               std::string pValue);
    SqlValue*         sqlValue(HostVariableUse pUSeCase, // object factory
                               char*           pLabel,
                               void*           pValueAny);
    ColumnValueSet&   operator+=(SqlValue* rhs);
    ColumnValueSet&   operator=(ColumnValueSet& rhs);
    ColumnValueSet&   operator+=(ColumnValueSet& rhs);
private:
    std::vector<SqlValue *> mValueContainer;
};

///////////////////////////////////////////////////////////////////////////////
// Operation - trace of single operation on an entity, superclass with a key
// implementation of the interface of adding the value is left to sub-classes,
// the class knows only how to handle the keys. It has no knowledge of what
// values are stored in the super-class.
///////////////////////////////////////////////////////////////////////////////

class Operation // purely virtual class
{
public:
    Operation(std::string pEntity);
    virtual ~Operation();
    virtual std::string          getXmlRedo() = 0;
    virtual std::string          getXmlUndo() = 0;
    void                         addKey(SqlValue* pValue);
    void                         addKeySet(ColumnValueSet* pValueSet);
    virtual void                 addValue(SqlValue* pValueSet,
                                          OperationValueState pState) = 0;
    virtual void                 addValueSet(ColumnValueSet*  pValueBefore,
                                             ColumnValueSet*  pValueAfter) = 0;
    virtual std::string          sqlStatementText() = 0;
    ColumnValueSet*              getKeySet();
    virtual ColumnValueSet*      getValueSet() = 0;
    virtual bool                 isTypeEntityMatch(OperationType pType,
                                                   std::string   pEntity) = 0;
    void                         setBatch(Batch* pBatch);
protected:
    // key values must be avalable in sub-classes
    std::string                  mEntity;  // on what entity
    Batch*                       mMyBatch; // it knows its batch (UPDATE - SELECT match)
    ColumnValueSet               mKey;     // by which key
};

///////////////////////////////////////////////////////////////////////////////
// specific fields for subclass of Operation: Insert (subclass with a value)
///////////////////////////////////////////////////////////////////////////////

class OperationInsert: public Operation
{
public:
    OperationInsert(std::string pEntity);
    virtual ~OperationInsert();
    std::string          getXmlRedo();
    std::string          getXmlUndo();
    void                 addValue(SqlValue* pValue,
                                  OperationValueState pState);
    void                 addValueSet(ColumnValueSet*  pValueBefore,
                                     ColumnValueSet*  pValueAfter);
    ColumnValueSet*      getValueSet();
    std::string          sqlStatementText();
    bool                 isTypeEntityMatch(OperationType pType,
                                           std::string   pEntity);
protected:
    ColumnValueSet       mValueAfter;
};

///////////////////////////////////////////////////////////////////////////////
// specific fields for subclass of Operation: Delete (subclass with a value)
///////////////////////////////////////////////////////////////////////////////

class OperationDelete : public Operation
{
public:
    OperationDelete(std::string pEntity);
    virtual ~OperationDelete();
    std::string          getXmlRedo();
    std::string          getXmlUndo();
    void                 addValue(SqlValue* pValue,
                                  OperationValueState pState);
    void                 addValueSet(ColumnValueSet*  pValueBefore,
                                     ColumnValueSet*  pValueAfter);
    ColumnValueSet*      getValueSet();
    std::string          sqlStatementText();
    bool                 isTypeEntityMatch(OperationType pType,
                                           std::string   pEntity);
protected:
    ColumnValueSet       mValueBefore;
};

///////////////////////////////////////////////////////////////////////////////
// specific fields for subclass of Operation: Update (subclass with a value)
///////////////////////////////////////////////////////////////////////////////

class OperationUpdate : public Operation
{
public:
    OperationUpdate(std::string pEntity);
    virtual ~OperationUpdate();
    std::string          getXmlRedo();
    std::string          getXmlUndo();
    void                 addValue(SqlValue* pValue,
                                  OperationValueState pState);
    void                 addValueSet(ColumnValueSet*  pValueBefore,
                                     ColumnValueSet*  pValueAfter);
    ColumnValueSet*      getValueSet();
    std::string          sqlStatementText();
    bool                 isTypeEntityMatch(OperationType pType,
                                           std::string   pEntity);
protected:
    ColumnValueSet       mValueBefore;
    ColumnValueSet       mValueAfter;
};

///////////////////////////////////////////////////////////////////////////////
// specific fields for subclass of Operation: Select (subclass with a value)
///////////////////////////////////////////////////////////////////////////////

class OperationSelect : public Operation
{
public:
    OperationSelect(std::string pEntity);
    virtual ~OperationSelect();
    std::string          getXmlRedo();
    std::string          getXmlUndo();
    void                 addValue(SqlValue* pValue,
                                  OperationValueState pState);
    void                 addValueSet(ColumnValueSet*  pValueBefore,
                                     ColumnValueSet*  pValueAfter);
    ColumnValueSet*      getValueSet();
    std::string          sqlStatementText();
    bool                 isTypeEntityMatch(OperationType pType,
                                           std::string   pEntity);
protected:
    ColumnValueSet       mValueBefore; // and After but no need to declare separately
};

///////////////////////////////////////////////////////////////////////////////
// batch of db operations done in a sequential order, factory has access to data
///////////////////////////////////////////////////////////////////////////////

typedef std::list<Operation*>::iterator OperationListIt;           // REDO order
typedef std::list<Operation*>::reverse_iterator OperationListRevIt;// UNDO order

class Batch
{
    friend class DoLog;
public:
    Batch(std::string pDigest,
          ColumnValueSet* pBatchKey);
    ~Batch();
    std::string           getXmlRedo();
    std::string           getXmlUndo();
    void                  sqlStatementTextAll(StringVector& pSqlTextContainer);
    ColumnValueSet*       findFirstBatchOperation(OperationType pType,
                                                  std::string   pEntity);
    ColumnValueSet*       getBatchKey();
private:
    std::string           mDigest;
    ColumnValueSet*       mBatchKey;
    std::list<Operation*> mOperation;// the list keeps order of adding the operation
};

///////////////////////////////////////////////////////////////////////////////
// XML logger: set of db operations regiested on digest key stored in batches.
// It allows registration of DML like operations and then it allows to serialize them
// using file or Oracle DB table. Deserialization is: loading from DB a set of operations
// and recreating in original form in memory. The operations may have 2 types
// of presentation:
// 1. as XML (to be serialized)
// 2. as SQL statements (after deserialization, to be applied)
// The UNDO transformation is done while serializing the operations. It is done as:
// 1. INSERT -> DELETE
// 2. DELETE -> INSERT
// 3. UPDATE -> UPDATE
// 4. SELECT, UPDATE -> UPDATE
///////////////////////////////////////////////////////////////////////////////

typedef std::map<std::string, Batch*> BatchContainer;
typedef std::map<std::string, Batch*>::iterator BatchContainerIt;

class DoLog // Singleton class
{
    friend void logUndoInit(const char* pDbName,
                            const char* pDbUser,
                            const char* pDbPass,
                            const char* pDbConnectionId,
                            const int   pLogLevel);
    friend void logUndoBatch(int pCustomerId,
                             int pBillSeqNo);
    friend void logUndoFlush();
public:
    ~DoLog();
    static DoLog*        getInstance();
    void                 clean();
    std::string          getXmlRedo();
    std::string          getXmlUndo();
    bool                 save(const char* pFileName);
    bool                 load(const char* pFileName);
    bool                 save();                                // using DB
    bool                 load(const int pBillSeqNo = 0,         // using DB
                              const int pCustomerId = 0);
    Operation*           sqlOperation(ColumnValueSet* pValueSet,// object factory
                                      OperationType   pType,
                                      std::string     pEntity);
    void                 sqlStatementTextAll(std::vector<std::string>& pSqlStatementContainer);
    bool                 sqlStatementApply(const std::string &pSqlStatement);
    bool                 sqlStatementApplyAll(std::vector<std::string>& pSqlStatementContainer);
protected:
    bool                 dbLongVarcharInsert(std::string& pImage,
                                             std::string& pDigest,
                                             std::string& pCustomerId,
                                             std::string& pBillSeqNo);
    bool                 dbLongVarcharSelect(int pSeqNo,
                                             int pImageLength);
    void                 xmlParse(const unsigned char* pXmlString,// using XALAN engine
                                  const size_t         pXmlStringLength);
    ColumnValueSet*      findBatchKey(std::string& pSearchDigest);
    Batch*               addBatch(std::string&    pSearchDigest,
                                  ColumnValueSet* pKey);
private:
    static DoLog*        sInstance;
    bool                 mHandleDbConnect;
    char*                mDbHandle;
    std::string          mDbUserName;
    BatchContainer       mBatchContainer;
    DoLog();
    DoLog(const DoLog&);
};

///////////////////////////////////////////////////////////////////////////////
// Interface functions to be used in BCache objects
///////////////////////////////////////////////////////////////////////////////

//
// Init library for specific connection type opening Oracle connection
// if pDbName is not NULL and for specifc connection id (if sepecified)
//
void logUndoInit(const char* pDbName,
                 const char* pDbUser,
                 const char* pDbPass,
                 const char* pDbConnectionId,
                 const int   pLogLevel = 0);

//
// Init for next cache record setting the cursor for all subsequent operations
// to a specific pair of <CUSTOMER_ID, BILLSEQNO>
//
void logUndoBatch(int pCustomerId,
                  int pBillSeqNo);

//
// Log UNDO operation for entity of specified type assigning it to the previously
// initialized batch
//
void logUndo(const OperationType pOperationType,
             const char*         pEntity,
             ...);

//
// Flush all batches for a current cache doing commit if initialize with specific
// DB connection
//
void logUndoFlush();

}

#endif
