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
// Author(s)  : Norbert Bondarczuk
// Created    : 2015-02-02
// Abstract   : Provides DOLOGComponentController
//
///////////////////////////////////////////////////////////////////////////////
/*
 * static char *SCCS_VERSION = "%I%";
 */

#include "DoLogComponentController.hpp"
#include "DoLogTerminationHandler.hpp"

// instance of singleton
DOLOGComponentController * DOLOGComponentController::sdpMyInstance = NULL;

// getInstance of singleton
DOLOGComponentController & DOLOGComponentController::getInstance( void )
{
    if ( NULL == sdpMyInstance)
    {
        sdpMyInstance = new DOLOGComponentController();
    }

    return * sdpMyInstance;
}

// constructor
DOLOGComponentController::DOLOGComponentController( void )
{
    DOLOGTerminationHandler * lpTermHandler = new DOLOGTerminationHandler();
    setTerminationHandler( *lpTermHandler );
}

// destructor
DOLOGComponentController::~DOLOGComponentController( void )
{}
