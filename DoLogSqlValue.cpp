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
// File       : SqlValue.hpp
// Description: Provides SqlValue class with specialisations for each specific
//              host variable type representation. The main class stores the value
//              as string ready to be encoded into XML element. The attributes
//              of the element (like TypeId, FormatMask for date, ...) are also
//              stored in prepared format on the level of main class. Presentation
//              layer is XML and SQL statement format.
// Author(s)  : Norbert Bondarczuk
// Created    : 2015-01-20
// Abstract   : Provides SQL_VALUE class with specialisations
//
///////////////////////////////////////////////////////////////////////////////
/*
 * static char *SCCS_VERSION = "%I%";
 */

#include <string>
#include <iostream>
#include <map>
#include <sstream>

#include "DoLogTerminationHandler.hpp"
#include "DoLogComponentController.hpp"
#include "DoLogTrace.hpp"
#include "DoLogSqlValue.hpp"

using namespace std;

namespace dolog
{

// do decode of values from -> into on a string provided
void translate(string& pString,
               string pFrom,
               string pInto)
{
    size_t index = 0;
    while (true)
    {
        index = pString.find(pFrom, index);
        if (index == string::npos)
        {
            break;
        }
        else
        {
            pString.replace(index, pFrom.length(), pInto);
            index += pInto.length();
        }
    }
}

// decode XML special escape sequence but only for sensible types of values
string xmlEscCharDecode(SqlValueType pType, string pString)
{
    string outString = pString;

    if (pType == SQL_CHAR_TYPEID ||
        pType == SQL_VARCHAR_TYPEID)
    {
        translate(outString, "&lt", "<");
        translate(outString, "&gt", ">");
        translate(outString, "&quot", "\"");
        translate(outString, "&apos", "\'");
        translate(outString, "&amp", "&");
    }

    return string(outString);
}

// encode XML special characters: <>\'"& but only for sensible types of values
string xmlEscCharEncode(SqlValueType pType, string pString)
{
    string outString;
    stringstream ss;

    if (pType == SQL_CHAR_TYPEID ||
        pType == SQL_VARCHAR_TYPEID)
    {
        unsigned int i;
        for (i = 0; i < pString.length(); i++)
        {
            switch (pString[i])
            {
                case '<' : ss << "&lt"; break;
                case '>' : ss << "&gt"; break;
                case '\"': ss << "&quot"; break;
                case '\'': ss << "&apos"; break;
                case '&' : ss << "&amp"; break;
                default: ss << pString[i];
            }
        }

        outString = ss.str();
    }
    else
    {
        outString = pString;
    }

    return outString;
}

////////////////////////////////////////////////////////////////////////////////
// SqlValue: representation of sql value, ready to show in XML
////////////////////////////////////////////////////////////////////////////////

SqlValue::SqlValue(SqlValueType pType, string pLabel, string pValue)
    : mTypeId(pType),
      mLabel(pLabel),
      mValueString(pValue)
{
    TRACE(4, "SqlValue");
}

SqlValue::SqlValue(SqlValueType pType, string pLabel)
    : mTypeId(pType),
      mLabel(pLabel)
{
    TRACE(4, "SqlValue");
}

SqlValue::~SqlValue()
{}

string SqlValue::getString()
{
    return mValueString;
}

string SqlValue::getLabel()
{
    return mLabel;
}

void SqlValue::setAttribute(string pAttribute, string pAttributeValue)
{
    mAttribute[pAttribute] = pAttributeValue;
}

string SqlValue::getXmlAttributes()
{
    stringstream ss;
    ss << " TypeId=" << "\"" << mTypeId << "\""; // mandatory attribute
    // all optional attributes
    map<string, string>::iterator it = mAttribute.begin();
    while (it != mAttribute.end())
    {
        ss << " ";
        ss << it->first
           << "="
           << "\""
           << it->second
           << "\"";
        ++it;
    }
    return ss.str();
}

string SqlValue::getXml()
{
    stringstream ss;
    ss << "<"
       << mLabel
       << getXmlAttributes()
       << ">"
       << xmlEscCharEncode(mTypeId, mValueString)
       << "</"
       << mLabel
       << ">";
    return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
// SqlChar
////////////////////////////////////////////////////////////////////////////////

SqlChar::SqlChar(string pLabel, string pValueString)
    : SqlValue(SQL_CHAR_TYPEID, pLabel, pValueString)
{}

SqlChar::SqlChar(string pLabel, char* pValueChar)
    : SqlValue(SQL_CHAR_TYPEID, pLabel, string(pValueChar))
{}

SqlChar::SqlChar(string pLabel, void* pValueAny)
    : SqlValue(SQL_CHAR_TYPEID, pLabel)
{
    TRACE(4, "SqlChar");
    char* tmpValue = static_cast<char *>(pValueAny);
    string tmpString(tmpValue);
    TRACE_MSG("SqlChar: " + tmpString);
    mValueString = tmpString;
}

SqlChar* SqlChar::clone()
{
    return new SqlChar(*this);
}

string SqlChar::getValue()
{
    return "\'" + mValueString + "\'";
}

////////////////////////////////////////////////////////////////////////////////
// SqlInteger
////////////////////////////////////////////////////////////////////////////////

SqlInteger::SqlInteger(string pLabel, string pValueString)
    : SqlValue(SQL_INTEGER_TYPEID, pLabel, pValueString)
{}

SqlInteger::SqlInteger(string pLabel, int pValueInt)
    : SqlValue(SQL_INTEGER_TYPEID, pLabel, any2string(pValueInt))
{}

SqlInteger::SqlInteger(string pLabel, void *pValueAny)
    : SqlValue(SQL_INTEGER_TYPEID, pLabel)
{
    TRACE(4, "SqlInteger");
    int *tmpValue = static_cast<int *>(pValueAny);
    string tmpString = any2string(*tmpValue);
    TRACE_MSG("SqlInteger: " + tmpString);
    mValueString = tmpString;
}

SqlInteger* SqlInteger::clone()
{
    return new SqlInteger(*this);
}

string SqlInteger::getValue()
{
    return mValueString;
}

////////////////////////////////////////////////////////////////////////////////
// SqlSmallint
////////////////////////////////////////////////////////////////////////////////

SqlSmallint::SqlSmallint(string pLabel, string pValueString)
    : SqlValue(SQL_SMALLINT_TYPEID, pLabel, pValueString)
{}

SqlSmallint::SqlSmallint(string pLabel, short pValueShort)
    : SqlValue(SQL_SMALLINT_TYPEID, pLabel, any2string(pValueShort))
{}

SqlSmallint::SqlSmallint(string pLabel, void *pValueAny)
    : SqlValue(SQL_SMALLINT_TYPEID, pLabel)
{
    TRACE(4, "SqlSmallint");
    short *tmpValue = static_cast<short *>(pValueAny);
    string tmpString = any2string(*tmpValue);
    TRACE_MSG("SqlSmallint: " + tmpString);
    mValueString = tmpString;
}

SqlSmallint* SqlSmallint::clone()
{
    return new SqlSmallint(*this);
}

string SqlSmallint::getValue()
{
    return mValueString;
}

////////////////////////////////////////////////////////////////////////////////
// SqlFloat
////////////////////////////////////////////////////////////////////////////////

SqlFloat::SqlFloat(string pLabel, string pValueString)
    : SqlValue(SQL_FLOAT_TYPEID, pLabel, pValueString)
{}

SqlFloat::SqlFloat(string pLabel, float pValueFloat)
    : SqlValue(SQL_FLOAT_TYPEID, pLabel, any2string(pValueFloat))
{}

SqlFloat::SqlFloat(string pLabel, void *pValueAny)
    : SqlValue(SQL_FLOAT_TYPEID, pLabel)
{
    TRACE(4, "SqlFloat");
    float *tmpValue = static_cast<float *>(pValueAny);
    string tmpString = any2string(*tmpValue);
    TRACE_MSG("SqlFloat: " + tmpString);
    mValueString = tmpString;
}

SqlFloat* SqlFloat::clone()
{
    return new SqlFloat(*this);
}

string SqlFloat::getValue()
{
    return mValueString;
}

////////////////////////////////////////////////////////////////////////////////
// SqlDouble
////////////////////////////////////////////////////////////////////////////////

SqlDouble::SqlDouble(string pLabel, string pValueString)
    : SqlValue(SQL_DOUBLE_TYPEID, pLabel, pValueString)
{}

SqlDouble::SqlDouble(string pLabel, double pValueDouble)
    : SqlValue(SQL_DOUBLE_TYPEID, pLabel, any2string(pValueDouble))
{}

SqlDouble::SqlDouble(string pLabel, void *pValueAny)
    : SqlValue(SQL_FLOAT_TYPEID, pLabel)
{
    TRACE(4, "SqlDouble");
    double *tmpValue = static_cast<double *>(pValueAny);
    string tmpString = any2string(*tmpValue);
    TRACE_MSG("SqlDouble: " + tmpString);
    mValueString = tmpString;
}

SqlDouble* SqlDouble::clone()
{
    return new SqlDouble(*this);
}

string SqlDouble::getValue()
{
    return mValueString;
}

////////////////////////////////////////////////////////////////////////////////
// SqlDate
////////////////////////////////////////////////////////////////////////////////

SqlDate::SqlDate(string pLabel, string pValueString)
    : SqlValue(SQL_DATE_TYPEID, pLabel, pValueString),
      mFormatMask(string("YYYYMMDDHH24MISS"))
{
    SqlValue::setAttribute("FormatMask", mFormatMask);
}

SqlDate::SqlDate(string pLabel, string pValueString, string pFormatMask)
    : SqlValue(SQL_DATE_TYPEID, pLabel, pValueString),
      mFormatMask(pFormatMask)
{
    SqlValue::setAttribute("FormatMask", mFormatMask);
}

SqlDate::SqlDate(string pLabel, void *pValueAny)
    : SqlValue(SQL_DATE_TYPEID, pLabel),
      mFormatMask(string("YYYYMMDDHH24MISS"))
{
    TRACE(4, "SqlDate");
    char *tmpValue = static_cast<char *>(pValueAny);
    string tmpString(tmpValue);
    TRACE_MSG("Integer: " + tmpString);
    mValueString = tmpString;
}

SqlDate* SqlDate::clone()
{
    return new SqlDate(*this);
}

string SqlDate::getValue()
{
    stringstream ss;
    string format;
    if (mFormatMask.length() > 0)
    {
        format = ",\'" + mFormatMask + "\'";
    }
    ss << "TO_DATE(" << "\'" << mValueString << "\'" << format << ")";
    return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
// SqlVarchar
////////////////////////////////////////////////////////////////////////////////

SqlVarchar::SqlVarchar(string pLabel, string pValueString)
    : SqlValue(SQL_VARCHAR_TYPEID, pLabel, pValueString)
{}

SqlVarchar::SqlVarchar(string pLabel, void* pValueAny)
    : SqlValue(SQL_VARCHAR_TYPEID, pLabel)
{
    TRACE(4, __func__);
    Varchar *tmpValue = static_cast<Varchar*>(pValueAny);
    string tmpString((char *)tmpValue->arr, tmpValue->len);
    TRACE_MSG("Varchar: " + tmpString);
    mValueString = tmpString;
}

SqlVarchar* SqlVarchar::clone()
{
    return new SqlVarchar(*this);
}

string SqlVarchar::getValue()
{
    return "\'" + mValueString + "\'";
}

////////////////////////////////////////////////////////////////////////////////
// SqlLong
////////////////////////////////////////////////////////////////////////////////

SqlLong::SqlLong(string pLabel, string pValueString)
    : SqlValue(SQL_LONG_TYPEID, pLabel, pValueString)
{}

SqlLong::SqlLong(string pLabel, int pValueLong)
    : SqlValue(SQL_LONG_TYPEID, pLabel, any2string(pValueLong))
{}

SqlLong::SqlLong(string pLabel, void *pValueAny)
    : SqlValue(SQL_LONG_TYPEID, pLabel)
{
    TRACE(4, "SqlLong");
    long *tmpValue = static_cast<long *>(pValueAny);
    string tmpString = any2string(*tmpValue);
    TRACE_MSG("SqlLong: " + tmpString);
    mValueString = tmpString;
}

SqlLong* SqlLong::clone()
{
    return new SqlLong(*this);
}

string SqlLong::getValue()
{
    return mValueString;
}

}
