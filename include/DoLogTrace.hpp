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
// Program    : BAT++ DOLOG TRACE
// File       : DoLogTrace.hpp
// Description: Declaration of tracing class. Used in the context of
//              invocation of the class constructor with parameters upon function
//              entry point. The logging is complemented by info from destructor
//              called upon exit from the function. Several logging levels
//              were defined: 1..4.
// Author(s)  : Norbert Bondarczuk
// Created    : 2015-01-08
// Abstract   : Declaration of tracing class.
//
///////////////////////////////////////////////////////////////////////////////
/*
 * static char *SCCS_VERSION = "%I%";
 */

#ifndef DoLogTrace_hpp
#define DoLogTrace_hpp

#include <string>
#include <iostream>
#include <sstream>

///////////////////////////////////////////////////////////////////////////////
// Function tracing facility:
// 1 - general process level
// 2 - logical (functional) record level
// 3 - DB level and record details
// 4 - parser level
///////////////////////////////////////////////////////////////////////////////

namespace dolog
{

class Trace
{
public:
    // store the level and name, all calls to Trace methods will use it
    Trace(int pLevel, const char* pFunctionName);
    // print info upon destruction while exiting from current context (also function)
    ~Trace();
    // print general info about step of execution
    void info(const std::string& pMessage);
    // use debug message
    void debug(const std::string& pMessage);
    // trace the usage of current function on initially declared level
    void trace(const std::string& pMessage);
    // trace but with a specified trace level
    void trace(int pLevel,
               const std::string& pMessage);
    // DOLOG_CRITICAL_ERROR returning false to the caller
    bool error(const std::string& pMessage);
    // DOLOG_CRITICAL_ERROR returning specific error code to the caller
    int  errorCode(const int pCode,
                   const std::string& pMessage);
    // used upon startup to determine the treshold for logging
    static void setLevel(int traceLevel);

private:
    std::string mCurrentFunctionName;
    int         mCurrentFunctionTraceLevel;
    static int  sCallLevel;

protected:
    // trace message format: [CALL_LEVEL_N]<N Spaces><FunctionName> message
    std::string prefix();
};

}

#define TRACE(level, fname) Trace __f__(level, fname)
#define TRACE_MSG(msg) __f__.trace(msg)
#define INFO(msg) DOLOG_LOG_STREAM << msg << std::endl;
#define ERROR(msg) __f__.error(msg)
#define ERROR_CODE(code, msg) __f__.errorCode(code, msg)

#endif
