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
// File       : DoLogDb.hpp
// Description: DB interface configuration parameters.
// Author(s)  : Norbert Bondarczuk
// Created    : 2015-01-08
// Abstract   : DB interface configuration parameters.
//
///////////////////////////////////////////////////////////////////////////////
/*
 * static char *SCCS_VERSION = "%I%";
 */

#ifndef DoLogDb_hpp
#define DoLogDb_hpp

// hardcoded value of the application id of the producer: BCH
#define BCH_APP_PROGRAM_ID 113

// Oracle error code for 'table or view does not exist'.
#define NON_EXISTENT       -942
#define NOT_FOUND          1403

// Max in memory LONG VARCHAR variable buffer size
#define MAX_XML_SIZE       10000000

// field sizes
#define MAX_ROWID_LEN      32
#define MAX_ERRMSG_LEN     256
#define MAX_DIGEST_LEN     256
#define MAX_USERNAME_LEN   16

#endif
