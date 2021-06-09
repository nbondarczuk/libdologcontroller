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
// File       : DoLogXmlParse.cpp
// Description: Implementation of XML parsing method of class DoLog.
// Author(s)  : Norbert Bondarczuk
// Created    : 2015-01-08
// Abstract   : Implementation of XML parsing method of class DoLog.
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

#include <stdio.h>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <string>

#include "DoLogTerminationHandler.hpp"
#include "DoLogComponentController.hpp"
#include "DoLogTrace.hpp"
#include "DoLogSqlValue.hpp"
#include "DoLog.hpp"
#include "DoLogXmlParse.hpp"

using namespace xercesc;
using namespace std;

namespace dolog
{

////////////////////////////////////////////////////////////////////////////////
// mandatory Xerces objects: error handler and container
////////////////////////////////////////////////////////////////////////////////

//
// xercesc exception & error handling
//

class XmlDomErrorHandler : public HandlerBase
{
public:
    void fatalError(const SAXParseException &pException);
private:
    bool mIsParserError;
    char mParserErrmsg[MAX_PARSER_ERRMSG_LEN + 1];
};

void XmlDomErrorHandler:: fatalError(const SAXParseException &pException)
{
    sprintf(mParserErrmsg, "Fatal parsing error at line %d", (int)pException.getLineNumber());
    mIsParserError = true;
}

//
// xercesc XML Document container
//

class XmlDomDocument
{
public:
    XmlDomDocument(const unsigned char* pXmlString,
                   const size_t pXmlStringLength);
    ~XmlDomDocument();
    DOMDocument* getDocument();
private:
    XmlDomDocument();
    XmlDomDocument(const class XmlDOMDocument&);
    void tryCreateParser();
    static class XercesDOMParser* sParser;
    static class ErrorHandler*    sErrorHandler;
    DOMDocument*                  mXmlDocument;
};

DOMDocument* XmlDomDocument::getDocument()
{
    return mXmlDocument;
}

class XercesDOMParser* XmlDomDocument::sParser       = NULL;
class ErrorHandler*    XmlDomDocument::sErrorHandler = NULL;

// create XML parser upon first call
void XmlDomDocument::tryCreateParser()
{
    if (!sParser)
    {
        XMLPlatformUtils::Initialize();
        sParser = new XercesDOMParser();
        sErrorHandler = (ErrorHandler*) new XmlDomErrorHandler();
        sParser->setErrorHandler(sErrorHandler);
    }
}

// perform parsing of XML document
XmlDomDocument::XmlDomDocument(const unsigned char* pXmlImage,
                               const size_t pXmlImageLength) : mXmlDocument(NULL)
{
    tryCreateParser();
    MemBufInputSource xmlBuffer(pXmlImage, pXmlImageLength, "xml (in memory)");
    sParser->parse(xmlBuffer);
    mXmlDocument = sParser->getDocument();
}

// XML document instance was processed - memory may be relased
XmlDomDocument::~XmlDomDocument()
{
    if (mXmlDocument)
    {
        mXmlDocument->release();
    }
}

////////////////////////////////////////////////////////////////////////////////
// using Xerrces parser dom objects
////////////////////////////////////////////////////////////////////////////////

// last recently found batch key to be used across the whole call stack of parser
// functions
static ColumnValueSet* sBatchKey = NULL;

// the function converts XALAN string into a standard readable format
string xmlCh2string(const XMLCh* pXmlChValue)
{
    char* labelTrans = XMLString::transcode(pXmlChValue);
    string label(labelTrans);
    XMLString::release(&labelTrans);
    return label;
}

// the fucntion checks is the DOM element label is matching the string
bool elementTagNameEq(DOMElement* pElement,
                      std::string pLabelQuery)
{
    TRACE(4, "elementTagNameEq");

    TRACE_MSG("Tesing element label: " + pLabelQuery);

    if( !pElement )
    {
        throw(std::runtime_error( "Empty XML document element" ));
    }

    string label = xmlCh2string(pElement->getTagName());
    TRACE_MSG("Element label is: " + label);
    if (label != pLabelQuery)
    {
        return false;
    }

    return true;
}

// the functions gets the number of sub-elements of a DOM element
// and their number
DOMNodeList* childNodeListGet(const DOMElement* pElement,
                              XMLSize_t& pElementListLength)
{
    DOMNodeList* nodeList = pElement->getChildNodes();
    const XMLSize_t nodeListLength = nodeList->getLength();
    if ( nodeListLength  == 0 )
    {
        throw(std::runtime_error( "Empty XML node list" ));
    }

    pElementListLength = nodeListLength;
    return nodeList;
}

// the function gets the ENTITY tag for a given DOM element
void xmlParseBatchOperationEntity(DOMElement* pElement,
                                  string& pEntityLabel)
{
    TRACE(4, "xmlParseBatchOperationEntity");

    XMLSize_t nodeListLength;
    DOMNodeList* nodeList = childNodeListGet(pElement, nodeListLength);
    for( XMLSize_t i = 0; i < nodeListLength; i++ )
    {
        DOMNode* node = nodeList->item(i);
        if (node &&
            node->getNodeType() == DOMNode::TEXT_NODE )
        {
            pEntityLabel = xmlCh2string(node->getNodeValue());
            break;
        }
    }
}

// the function gets the value of a DOM element where KEY/VALUE
// pairs are stored for each SQL attribute stored in XML
string xmlParseBatchOperationKeyValue(DOMElement* pElement)
{
    TRACE(4, "DoLog::xmlParseBatchOperationKeyValue");

    string value;
    XMLSize_t nodeListLength;
    DOMNodeList* nodeList;

    nodeList = childNodeListGet(pElement, nodeListLength);
    for( XMLSize_t i = 0; i < nodeListLength; i++ )
    {
        DOMNode* node = nodeList->item(i);
        if (node &&
            node->getNodeType() == DOMNode::TEXT_NODE)
        {
            value = xmlCh2string(node->getNodeValue());
            break;
        }
    }

    return value;
}

// the function gets the attribute of a given DOM element by it's name
// the deoding from XALAN format of strings is done here
string xmlParseBatchOperationKeyValueAttribute(DOMElement* pElement,
                                               string pAttributeLabel)
{
    TRACE(4, "xmlParseBatchOperationKeyValueAttribute");

    string attributeValue;
    XMLCh* chAttributeLabel = XMLString::transcode(pAttributeLabel.c_str());
    const XMLCh* chAttributeValue = pElement->getAttribute(chAttributeLabel);
    if (chAttributeValue)
    {
        attributeValue = xmlCh2string(chAttributeValue);
    }
    XMLString::release(&chAttributeLabel);
    return attributeValue;
}

// the function parses a sequence of SQL values starting with a list
// of KEYs of VALUEs
ColumnValueSet* xmlParseBatchOperationArg(DOMElement* pElement)
{
    TRACE(4, "DoLog::xmlParseBatchOperationArg");

    ColumnValueSet* columnValueSet = new ColumnValueSet;
    SqlValue* sqlValue = NULL;

    // KEY or VALUE
    if (!elementTagNameEq(pElement, "KEY") &&
        !elementTagNameEq(pElement, "VALUE"))
    {
        throw(std::runtime_error( "XML element neither KEY nor VALUE" ));
    }

    XMLSize_t nodeListLength;
    DOMNodeList* nodeList = childNodeListGet(pElement, nodeListLength);
    TRACE_MSG("KEY/VALUE: Found sub-nodes: " + any2string(nodeListLength));

    // for each sub-node: list of column names with values add to a set
    for( XMLSize_t i = 0; i < nodeListLength; i++ )
    {
        DOMNode* node = nodeList->item(i);
        if (node &&
            node->getNodeType() == DOMNode::ELEMENT_NODE )
        {
            string label  = xmlCh2string(node->getNodeName());
            string valueString  = xmlParseBatchOperationKeyValue(dynamic_cast< xercesc::DOMElement* >(node));
            string typeId = xmlParseBatchOperationKeyValueAttribute(dynamic_cast< xercesc::DOMElement* >(node), "TypeId");
            TRACE_MSG(label + " = " + valueString + " - TypeId: " + typeId);
            sqlValue = columnValueSet->sqlValue(typeId, label, valueString);
            columnValueSet->addValue(sqlValue);
        }
    }

    // new set of labeled values
    return columnValueSet;
}

// the function parses the DOM element with type INSERT
void xmlParseBatchOperationInsert(DOMElement* pElement)
{
    TRACE(4, "DoLog::xmlParseBatchOperationInsert");

    string entityLabel;
    ColumnValueSet* operationKey = NULL;
    ColumnValueSet* operationValueAfter = NULL;
    XMLSize_t nodeListLength;
    DOMNodeList* nodeList = childNodeListGet(pElement, nodeListLength);
    TRACE_MSG("INSERT: Found sub-nodes: " + any2string(nodeListLength));

    // for each sub-node: ENTITY KEY VALUE
    for( XMLSize_t i = 0; i < nodeListLength; i++ )
    {
        DOMNode* node = nodeList->item(i);
        if(node &&
           node->getNodeType() == DOMNode::ELEMENT_NODE )
        {
            DOMElement* elementOfNode = dynamic_cast< xercesc::DOMElement* >(node);
            if (!elementOfNode)
            {
                throw(std::runtime_error( "Empty XML element OPERATION" ));
            }

            string label = xmlCh2string(elementOfNode->getTagName());
            if (label == "ENTITY")
            {
                xmlParseBatchOperationEntity(elementOfNode, entityLabel);
                TRACE_MSG("Entity: " + entityLabel);
            }
            else if (label == "KEY")
            {
                operationKey = xmlParseBatchOperationArg(elementOfNode);
                TRACE_MSG("Parsed KEY");
            }
            else if (label == "VALUE")
            {
                operationValueAfter = xmlParseBatchOperationArg(elementOfNode);
                TRACE_MSG("Parsed VALUE");
            }
            else
            {
                throw(std::runtime_error( "Invalid operation field tag found: " + label));
            }
        }
    }

    // all needed values collected
    TRACE_MSG("Registering INSERT on entity: " + entityLabel);
    Operation* operation = DoLog::getInstance()->sqlOperation(sBatchKey, INSERT, entityLabel);
    operation->addKeySet(operationKey);            // deep copy
    operation->addValueSet(NULL, operationValueAfter); // deep copy
    delete operationKey;
    delete operationValueAfter;
    TRACE_MSG("Registered INSERT on entity: " + entityLabel);
}

// the function parses the DOM element with type UPDATE
void xmlParseBatchOperationUpdate(DOMElement* pElement)
{
    TRACE(4, "DoLog::xmlParseBatchOperationUpdate");

    string entityLabel;
    ColumnValueSet* operationKey = NULL;
    ColumnValueSet* operationValueAfter = NULL;

    XMLSize_t nodeListLength;
    DOMNodeList* nodeList = childNodeListGet(pElement, nodeListLength);
    TRACE_MSG("UPDATE: Found sub-nodes: " + any2string(nodeListLength));

    // for each sub-node: ENTITY KEY VALUE
    for( XMLSize_t i = 0; i < nodeListLength; i++ )
    {
        DOMNode* node = nodeList->item(i);
        if(node &&
           node->getNodeType() == DOMNode::ELEMENT_NODE )
        {
            DOMElement* elementOfNode = dynamic_cast< xercesc::DOMElement* >(node);
            if (!elementOfNode)
            {
                throw(std::runtime_error( "Empty XML element OPERATION" ));
            }

            string label = xmlCh2string(elementOfNode->getTagName());
            if (label == "ENTITY")
            {
                xmlParseBatchOperationEntity(elementOfNode, entityLabel);
                TRACE_MSG("Parsed ENTITY: " + entityLabel);
            }
            else if (label == "KEY")
            {
                operationKey = xmlParseBatchOperationArg(elementOfNode);
                TRACE_MSG("Parsed KEY");
            }
            else if (label == "VALUE")
            {
                operationValueAfter = xmlParseBatchOperationArg(elementOfNode);
                TRACE_MSG("Parsed VALUE");
            }
            else
            {
                throw(std::runtime_error( "Invalid operation field tag found: " + label));
            }
        }
    }

    // all needed values collected
    TRACE_MSG("Registering UPDATE on entity: " + entityLabel);
    Operation* operation = DoLog::getInstance()->sqlOperation(sBatchKey, UPDATE, entityLabel);
    operation->addKeySet(operationKey);                // deep copy
    operation->addValueSet(NULL, operationValueAfter); // deep copy
    delete operationKey;
    delete operationValueAfter;
    TRACE_MSG("Registered UPDATE on entity: " + entityLabel);
}

// the function parses the DOM element with type DELETE
void xmlParseBatchOperationDelete(DOMElement* pElement)
{
    TRACE(4, "DoLog::xmlParseBatchOperationDelete");

    string entityLabel;
    ColumnValueSet* operationKey = NULL;
    ColumnValueSet* operationValueBefore = NULL;
    ColumnValueSet* operationValueAfter = NULL;

    XMLSize_t nodeListLength;
    DOMNodeList* nodeList = childNodeListGet(pElement, nodeListLength);
    TRACE_MSG("DELETE: Found sub-nodes: " + any2string(nodeListLength));

    // for each sub-node: ENTITY KEY VALUE
    for( XMLSize_t i = 0; i < nodeListLength; i++ )
    {
        DOMNode* node = nodeList->item(i);
        if (node &&
            node->getNodeType() == DOMNode::ELEMENT_NODE )
        {
            TRACE_MSG("Parsing next element");
            DOMElement* elementOfNode = dynamic_cast< xercesc::DOMElement* >(node);
            if (!elementOfNode)
            {
                throw(std::runtime_error( "Empty XML element OPERATION" ));
            }
            string label = xmlCh2string(elementOfNode->getTagName());
            if (label == "ENTITY")
            {
                xmlParseBatchOperationEntity(elementOfNode, entityLabel);
                TRACE_MSG("Parsed ENTITY: " + entityLabel);
            }
            else if (label == "KEY")
            {
                operationKey = xmlParseBatchOperationArg(elementOfNode);
                TRACE_MSG("Parsed KEY");
            }
            else if (label == "VALUE")
            {
                operationValueBefore = xmlParseBatchOperationArg(elementOfNode);
                TRACE_MSG("Parsed VALUE");
            }
            else
            {
                throw(std::runtime_error( "Invalid operation field tag found: " + label));
            }
        }
    }

    // all needed values collected
    TRACE_MSG("Registering DELETE on entity: " + entityLabel);
    Operation* operation = DoLog::getInstance()->sqlOperation(sBatchKey, DELETE, entityLabel);
    operation->addKeySet(operationKey);                  // deep copy
    operation->addValueSet(operationValueBefore, NULL);  // deep copy
    delete operationKey;
    delete operationValueBefore;
    TRACE_MSG("Registered DELETE on entity: " + entityLabel);
}

// the function parses the DOM element with any type of operation
// it starts proper handler foea each type of operation
void xmlParseBatchOperation(DOMElement* pElement)
{
    TRACE(4, "DoLog::xmlParseBatchOperation");

    // DIGEST KEY INSERT UPDATE DELETE
    if (!pElement)
    {
        throw(std::runtime_error( "Empty XML element BATCH" ));
    }
    string label = xmlCh2string(pElement->getTagName());
    TRACE_MSG("XML Parser: found element: " + label);

    if (label == "DIGEST")
    {
        // NOP - digest not needed
    }
    else if (label == "KEY")
    {
        sBatchKey = xmlParseBatchOperationArg(pElement);
    }
    else if (label == "INSERT")
    {
        xmlParseBatchOperationInsert(pElement);
    }
    else if (label == "UPDATE")
    {
        xmlParseBatchOperationUpdate(pElement);
    }
    else if (label == "DELETE")
    {
        xmlParseBatchOperationDelete(pElement);
    }
    else
    {
        throw(std::runtime_error( "Invalid element found: " + label));
    }
}

// the function parses a DOM element of type BATCH
// it also validates if it was callsed with the right element
void xmlParseBatch(DOMElement* pElement)
{
    TRACE(4, "DoLog::xmlParseBatch");

    // BATCH
    if (!elementTagNameEq(pElement, "BATCH"))
    {
        throw(std::runtime_error( "XML document level 2 element not BATCH" ));
    }

    // start building next batch

    sBatchKey = NULL;

    // for each sub-node DIGEST KEY INSERT UPDATE DELETE

    XMLSize_t nodeListLength;
    DOMNodeList* nodeList = childNodeListGet(pElement, nodeListLength);
    TRACE_MSG("BATCH: Found sub-nodes: " + any2string(nodeListLength));

    for( XMLSize_t i = 0; i < nodeListLength; i++ )
    {
        DOMNode* node = nodeList->item(i);
        if (node &&
            node->getNodeType() == DOMNode::ELEMENT_NODE )
        {
            xmlParseBatchOperation(dynamic_cast< xercesc::DOMElement* >(node));
        }
    }
}

// the function parses a DOM element of type UNDOLOG
// it also validates if it was callsed with the right element
void xmlParseUndolog(DOMDocument *pDocument)
{
    TRACE(4, "DoLog::xmlParseUndolog");

    if( !pDocument )
    {
        throw(std::runtime_error( "Empty XML document" ));
    }

    DOMElement* element = pDocument->getDocumentElement();

    // UNDOLOG
    if (!elementTagNameEq(element, "UNDOLOG"))
    {
        throw(std::runtime_error( "XML document level 1 root element not UNDOLOG" ));
    }

    // for each sub-node: BATCH
    XMLSize_t nodeListLength;
    DOMNodeList* nodeList = childNodeListGet(element, nodeListLength);
    TRACE_MSG("UNDOLOG: Found sub-nodes: " + any2string(nodeListLength));

    for( XMLSize_t i = 0; i < nodeListLength; i++ )
    {
        DOMNode* node = nodeList->item(i);
        if( node &&
            node->getNodeType() == DOMNode::ELEMENT_NODE )
        {
            xmlParseBatch(dynamic_cast< xercesc::DOMElement* >(node));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// DoLog::xmlParse
////////////////////////////////////////////////////////////////////////////////

void DoLog::xmlParse(const unsigned char *pBuffer,
                     const size_t pBufferLength)
{
    TRACE(4, "DoLog::xmlParse");

    XmlDomDocument* document = new XmlDomDocument(pBuffer, pBufferLength);
    xmlParseUndolog(document->getDocument());
    delete document;
}

}
