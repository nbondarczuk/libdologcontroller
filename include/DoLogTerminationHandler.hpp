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
// File       : DoLogTermiantionHanler.cpp
// Description: Provides DOLOGTerminationHandler standard BSCS termination functions
// Author(s)  : Norbert Bondarczuk
// Created    : 2015-02-02
// Abstract   : Provides DOLOGTerminationHandler
//
///////////////////////////////////////////////////////////////////////////////
/*
 * static char *SCCS_VERSION = "%I%";
 */

#ifndef DoLogTerminationHandler_hpp
#define DoLogTerminationHandler_hpp

#include "TerminationHandler.hpp"

class DOLOGTerminationHandler : public TerminationHandler
{
public:
    // constructor
    DOLOGTerminationHandler( void );

    virtual ~DOLOGTerminationHandler( void );

protected:

    // This virtual method has to be overloaded for each derivated
    // termination handler an should prepare the controlled component for
    // termination.
    //     - closing open files
    //     - closing database connects
    //     - free memory
    //     - etc.
    virtual void prepareTermination( void );

    // This virtual function has to be overloaded for each derivated
    // termination handler and should attach the Termination handlers of all
    // used libraries.
    // example for DOLOGTerminationHandler
    //
    // void DOLOGTerminationHandler::initDependencySelf( void )
    // {
    // }
    //
    virtual void initDependencySelf( void );

private:

};

#endif


