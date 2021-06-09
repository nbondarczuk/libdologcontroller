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
// File       : DoLogComponentController.cpp
// Description: Provides DOLOGComponentController standard BSCS logging interface
//              together with macros managing the access to OuptputController
// Author(s)  : Norbert Bondarczuk
// Created    : 2015-02-02
// Abstract   : Provides DOLOGComponentController class and macros
//
///////////////////////////////////////////////////////////////////////////////
/*
 * static char *SCCS_VERSION = "%I%";
 */

#ifndef DoLogComponentController_hpp
#define DoLogComponentController_hpp

#include "ComponentController.hpp"

//
// Returns a reference to the ErrorHandler object
//
#define DOLOG_ERROR_HANDLER                          COMMON_ERROR_HANDLER(DOLOG)
//
// Returns a reference to the OutputHandler object
//
#define DOLOG_OUTPUT_HANDLER                         COMMON_OUTPUT_HANDLER(DOLOG)
//
// Returns a reference to the DOLOGTerminationHandler object
//
#define DOLOG_TERMINATION_HANDLER                    COMMON_TERMINATION_HANDLER(DOLOG)

//
// Returns stream objects of type fileOutStream for log, error and trace stream
//
//
// can be used with a printf-like calling convention:
//
// DOLOG_LOG_STREAM.printf( "This is my log message. Time = %s\n", ctime( NULL ) );
//
// or like a c++ stream object:
//
// DOLOG_LOG_STREAM << "This is my log message. Time = " << ctime( NULL ) << endl;
//
#define DOLOG_LOG_STREAM                            COMMON_LOG_STREAM(DOLOG)
#define DOLOG_ERROR_STREAM                          COMMON_ERROR_STREAM(DOLOG)
#define DOLOG_DEBUG_STREAM                          COMMON_DEBUG_STREAM(DOLOG)

#define DOLOG_TRACE_STREAM(n)                       COMMON_TRACE_STREAM(DOLOG,n)

#define DOLOG_TRACE_LEVEL                           COMMON_TRACE_LEVEL(DOLOG)
#define DOLOG_TRACE_LEVEL_SET(n)                    DOLOGComponentController::getInstance().getOutputHandler().setTraceLevel(n)

//
// Macro for log message, it has a printf-like prototype:
//
// DOLOG_LOG_MESSAGE( const char *fFormatString,
//                  ... );
//
#define DOLOG_LOG_MESSAGE                           COMMON_LOG_MESSAGE(DOLOG)

//
// Macro for trace message, it has a printf-like prototype:
//
// DOLOG_TRACE_MESSAGE_1( const char *fFormatString,
//                      ... );
//
#define DOLOG_TRACE_MESSAGE_1                       COMMON_TRACE_MESSAGE(DOLOG,1)
#define DOLOG_TRACE_MESSAGE_2                       COMMON_TRACE_MESSAGE(DOLOG,2)
#define DOLOG_TRACE_MESSAGE_3                       COMMON_TRACE_MESSAGE(DOLOG,3)
#define DOLOG_TRACE_MESSAGE_4                       COMMON_TRACE_MESSAGE(DOLOG,4)

//
// Macro for debug message, it has a printf-like prototype:
//
//
// DOLOG_DEBUG_MESSAGE( const char *fFormatString,
//                    ... );
//
// Debug messages will be printed when preprocessor define DEBUG_FILE during
// compilation is set
#define DOLOG_DEBUG_MESSAGE                         COMMON_DEBUG_MESSAGE(DOLOG)


//
// The following Macros has a printf-like prototype:
//
// DOLOG_CFG_ERROR( const char *fFormatString,
//                 ... );
//
// use like:
// DOLOG_CFG_ERROR( "Error reading refenrce data config data in file '%s'\n", lFileName );
//
#define DOLOG_CFG_ERROR                              COMMON_CFG_ERROR(DOLOG)
#define DOLOG_FILE_ERROR                             COMMON_FILE_ERROR(DOLOG)
#define DOLOG_MEMORY_ERROR                           COMMON_MEMORY_ERROR(DOLOG)
#define DOLOG_USER_BREAK                             COMMON_USER_BREAK(DOLOG)
#define DOLOG_UDRLIB_ERROR                           COMMON_UDRLIB_ERROR(DOLOG)

//
// SQL error macros for OCI are not automatically supported!
//
// *************************************************
// *************************************************
// *                                               *
// *                                               *
// *        ATTENTION ATTENTION ATTENTION          *
// *                                               *
// *                                               *
// *************************************************
// *************************************************
//
// There is no mcro definition like the following
//
// #define DOLOG_OCI_SQL_ERROR(Status,Lda,Cursor)   COMMON_OCI_SQL_ERROR(DOLOG,Status,Lda,Cursor)
//
// For correct usage an individual macro has to be defined in the C++ source file
// e.g.:
//
// #define SQL_ERR       COMMON_OCI_SQL_ERROR(DOLOG,doiSqlRc, NULL, cur_ptr )
// or something like that because in each  c files the names of variables to store
// the return code, pointer to cursor and LDA area have different names.
//
// With such a definition the new defined macro SQL_ERR can be used like
//
// SQL_ERR( const char *fFormat, ... );
//

//
// Macros for Embedded/SQL error has a print like prototype
//
// DOLOG_EMBEDDED_SQL_ERROR( const char *fFormatString,
//                         ... );
//
// use like:
// DOLOG_EMBEDDED_SQL_ERROR( "Error loading %ld tariff models from MPUTMTAB\n", lCount );
//
#define DOLOG_EMBEDDED_SQL_ERROR                     COMMON_EMBEDDED_SQL_ERROR(DOLOG)

//
// The following Macros has an extenden printf-like prototype:
//
// DOLOG_FATAL_ERROR( const char *sReason,
//                  const char *fFormatString,
//                  ... );
//
// use like:
// DOLOG_FATAL_ERROR( "Insufficient System Resources",
//                  "Can't continue will stop the program execution after %ld seconds\n",
//                  lSecondsUpTime );
//
#define DOLOG_FATAL_ERROR                            COMMON_FATAL_ERROR(DOLOG)
#define DOLOG_CRITICAL_ERROR                         COMMON_CRITICAL_ERROR(DOLOG)
#define DOLOG_PETTY_ERROR                            COMMON_PETTY_ERROR(DOLOG)
#define DOLOG_WARNING                                COMMON_WARNING(DOLOG)


// Some additional macros for db load messages and memory usage
//
// print log message about total used memory
//
// This method prints the relative memory usage between old memory usage given
// in parameter frSysInfo and the current memory usage
//
//    void DOLOG_MEM_USED_MESSAGE (
//        const char  * cfpLocation,  // Where?
//        const char  * cfpReason,    // Why does memory increase?
//        BSysInfo    & frSysInfo     // How much does memory increase?
//    );
//
// This method prints the absolte memory usage
//
//    void DOLOG_MEM_USED_MESSAGE (
//        const char  * cfpLocation,  // Where?
//        const char  * cfpReason     // Why does memory increase?
//    );
#define DOLOG_MEM_USED_MESSAGE                    COMMON_MEM_USED_MESSAGE(DOLOG)

// print message after loading database tables to log file
//
//    void DOLOG_DB_LOAD_MESSAGE(
//        const char *cfpEnrity,    // What has been loaded from database?
//        const char *cfpTableName, // From which table in the database?
//        long       fNumber,       // Number of entries loaded from DB.
//        time_t     fDuration,     // Duration of the loading in sec.
//        bool       fLoadComplete  // Loading is now complete.
//    );
#define DOLOG_DB_LOAD_MESSAGE                     COMMON_DB_LOAD_MESSAGE(DOLOG)

class DOLOGComponentController : public ComponentController
{
public:

    static DOLOGComponentController &getInstance( void );

    // destructor
    virtual ~DOLOGComponentController( void );

protected:

    // constructor
    DOLOGComponentController( void );

private:

    static DOLOGComponentController    *sdpMyInstance;
};

#endif


