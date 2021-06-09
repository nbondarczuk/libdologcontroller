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
// Description: Provides SQL_VALUE class with specialisations for each specific
//              host variable type representation. The main class stores the value
//              as string ready to be encoded into XML element. The attributes
//              of the element (like TypeId, FormatMask for date, ...) are also
//              stored in prepared format on the level of main class. Presentation
//              layer is XML and SQL statement format.
// Author(s)  : Norbert Bondarczuk
// Created    : 2015-01-08
// Abstract   : Provides SQL_VALUE class with specialisations
//
///////////////////////////////////////////////////////////////////////////////
/*
 * static char *SCCS_VERSION = "%I%";
 */

#ifndef DoLogSqlValue_hpp
#define DoLogSqlValue_hpp

#include <string>
#include <iostream>
#include <map>
#include <list>
#include <stdexcept>
#include <sstream>

namespace dolog
{

template <class T>
int any2int(const T& t)
{
    int val;
    if (!(istringstream ( t ) >> val))
    {
        return -1;
    }
    return val;
}

template <class T>
std::string any2string (const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

void translate(string &pString, string pFrom, string pInto);

//
// TYPEID - values coherent with Perl Oracle DBI
//
typedef enum SqlValueType
{
    SQL_CHAR_TYPEID            = 1,
    SQL_NUMERIC_TYPEID         = 2,
    SQL_DECIMAL_TYPEID         = 3,
    SQL_INTEGER_TYPEID         = 4,
    SQL_SMALLINT_TYPEID        = 5,
    SQL_FLOAT_TYPEID           = 6,
    SQL_REAL_TYPEID            = 7,
    SQL_DOUBLE_TYPEID          = 8,
    SQL_DATE_TYPEID            = 9,
    SQL_TIME_TYPEID            =10,
    SQL_TIMESTAMP_TYPEID       =11,
    SQL_VARCHAR_TYPEID         =12,
    SQL_LONG_TYPEID            =13

} SqlValueType;

std::string xmlEscCharDecode(SqlValueType pType, std::string pString);
std::string xmlEscCharEncode(SqlValueType pType, std::string pString);

////////////////////////////////////////////////////////////////////////////////
// SqlValue: representation of sql value, ready to show in XML
////////////////////////////////////////////////////////////////////////////////

typedef std::map <std::string, std::string> String2StringMap;

class SqlValue
{
public:
    SqlValue(SqlValueType pType,
             std::string  pLabel,
             std::string  pValueString);
    SqlValue(SqlValueType pType,
             std::string  pLabel);
    virtual ~SqlValue();
    virtual SqlValue*    clone() = 0;
    virtual std::string  getValue() = 0;
    std::string          getString();
    std::string          getLabel();
    void                 setAttribute(std::string pAttribute,
                                      std::string pAttributeValue);
    std::string          getXmlAttributes();
    std::string          getXml();
protected:
    SqlValueType         mTypeId;
    String2StringMap     mAttribute;
    std::string          mLabel;
    std::string          mValueString;
};

////////////////////////////////////////////////////////////////////////////////
// SqlChar
////////////////////////////////////////////////////////////////////////////////

class SqlChar : public SqlValue
{
public:
    SqlChar(std::string pLabel,
            std::string pValueString);
    SqlChar(std::string pLabel,
            char*       pValueChar);
    SqlChar(std::string pLabel,
            void*       pValueAny);
    SqlChar*    clone();
    std::string getValue();
};

////////////////////////////////////////////////////////////////////////////////
// SqlInteger
////////////////////////////////////////////////////////////////////////////////

class SqlInteger : public SqlValue
{
public:
    SqlInteger(std::string pLabel,
               std::string pValueString);
    SqlInteger(std::string pLabel,
               int         pValueInt);
    SqlInteger(std::string pLabel,
               void*       pValueAny);
    SqlInteger* clone();
    std::string getValue();
};

////////////////////////////////////////////////////////////////////////////////
// SqlSmallint
////////////////////////////////////////////////////////////////////////////////

class SqlSmallint : public SqlValue
{
public:
    SqlSmallint(std::string pLabel,
                std::string pValueString);
    SqlSmallint(std::string pLabel,
                short       pValueShort);
    SqlSmallint(std::string pLabel,
                void*       pValueAny);
    SqlSmallint* clone();
    std::string  getValue();
};

////////////////////////////////////////////////////////////////////////////////
// SqlFloat
////////////////////////////////////////////////////////////////////////////////

class SqlFloat : public SqlValue
{
public:
    SqlFloat(std::string pLabel,
             std::string pValueString);
    SqlFloat(std::string pLabel,
             float       pValueFloat);
    SqlFloat(std::string pLabel,
             void*       pValueAny);
    SqlFloat*   clone();
    std::string getValue();
};

////////////////////////////////////////////////////////////////////////////////
// SqlDouble
////////////////////////////////////////////////////////////////////////////////

class SqlDouble : public SqlValue
{
public:
    SqlDouble(std::string pLabel,
              std::string pValueString);
    SqlDouble(std::string pLabel,
              double      pValueDouble);
    SqlDouble(std::string pLabel,
              void*       pValueAny);
    SqlDouble*  clone();
    std::string getValue();
};

////////////////////////////////////////////////////////////////////////////////
// SqlDate
////////////////////////////////////////////////////////////////////////////////

class SqlDate : public SqlValue
{
public:
    SqlDate(std::string pLabel,
            std::string pValueString);
    SqlDate(std::string pLabel,
            std::string pValueString,
            std::string pFormatMask);
    SqlDate(std::string pLabel,
            void*       pValueAny);
    SqlDate*    clone();
    std::string getValue();
private:
    std::string mFormatMask;
};

////////////////////////////////////////////////////////////////////////////////
// SqlVarchar
////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    unsigned short  len;
    unsigned char   arr[1];

} Varchar;

class SqlVarchar : public SqlValue
{
public:
    SqlVarchar(std::string pLabel,
               std::string pValueString);
    SqlVarchar(std::string pLabel,
               void*       pValueAny);
    SqlVarchar* clone();
    std::string getValue();
};

////////////////////////////////////////////////////////////////////////////////
// SqlLong
////////////////////////////////////////////////////////////////////////////////

class SqlLong : public SqlValue
{
public:
    SqlLong(std::string pLabel,
            std::string pValueString);
    SqlLong(std::string pLabel,
            int         pValueInt);
    SqlLong(std::string pLabel,
            void*       pValueAny);
    SqlLong* clone();
    std::string getValue();
};


}

#endif
