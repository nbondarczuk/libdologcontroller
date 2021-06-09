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
// Description: Implementation of tracing class. Used by
//              invocation of the class constructor with parameters upon function
//              entry point. The logging is complemented by info from destructor
//              called upon exit from the function. Several logging levels
//              were defined: 1..4.
// Author(s)  : Norbert Bondarczuk
// Created    : 2015-01-08
// Abstract   : Implementation of tracing class.
//
///////////////////////////////////////////////////////////////////////////////
/*
 * static char *SCCS_VERSION = "%I%";
 */

#include "DoLogComponentController.hpp"
#include "DoLogTerminationHandler.hpp"
#include "DoLogTrace.hpp"

namespace dolog
{

int Trace::sCallLevel = 0;

// trace message format: [CALL_LEVEL_N]<N Spaces><FunctionName> message
std::string Trace::prefix()
{
    std::stringstream ss;

    ss << "DOLOG ";
    ss << "[" << Trace::sCallLevel << "] ";
    for (int i = 0; i < sCallLevel; i++)
    {
        ss << " ";
    }
    ss << Trace::mCurrentFunctionName;

    return ss.str();
}

// store the level and name, all calls to Trace methods will use it
Trace::Trace(const int pLevel, const char *pFunctionName)
    : mCurrentFunctionName(pFunctionName), mCurrentFunctionTraceLevel(pLevel)
{
    ++sCallLevel;

    DOLOG_TRACE_STREAM(pLevel)
        << this->prefix()
        << " - Started"
        << std::endl;
}

// print info upon destruction while exiting from current context (also function)
Trace::~Trace()
{
    DOLOG_TRACE_STREAM(mCurrentFunctionTraceLevel)
        << this->prefix()
        << " - Finished"
        << std::endl;

    --sCallLevel;
}

// used upon startup to determine the treshold for logging
void Trace::setLevel(int pTraceLevel)
{
    DOLOG_TRACE_LEVEL_SET(pTraceLevel);
}

// print general info about step of execution in a LOG stream
void Trace::info(const std::string& pMessage)
{
    DOLOG_LOG_STREAM
        << pMessage
        << std::endl;
}

// use debug message in DEBUG stream
void Trace::debug(const std::string& pMessage)
{
    DOLOG_DEBUG_STREAM
        << pMessage
        << std::endl;
}

// trace the usage of current function on initially declared level
void Trace::trace(const std::string& pMessage)
{
    DOLOG_TRACE_STREAM(mCurrentFunctionTraceLevel)
        << this->prefix()
        << ": "
        << pMessage
        << std::endl;
}

// trace but with a specified trace level
void Trace::trace(const int pLevel,
                  const std::string& pMessage)
{
    DOLOG_TRACE_STREAM(pLevel)
        << prefix()
        << ": "
        << pMessage
        << std::endl;
}

// DOLOG_CRITICAL_ERROR returning false status to the caller
bool Trace::error(const std::string& pMessage)
{
    DOLOG_CRITICAL_ERROR("%s", pMessage.c_str());
    return false;
}

// DOLOG_CRITICAL_ERROR returning specific error code to the caller
int Trace::errorCode(const int pCode,
                     const std::string& pMessage)
{
    DOLOG_CRITICAL_ERROR("%s", pMessage.c_str());
    return pCode;
}

}
