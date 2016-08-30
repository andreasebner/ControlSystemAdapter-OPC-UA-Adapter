/*
 * Copyright (c) 2016 <copyright holder> <email>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 */

#include "XMLFileHandler.h"

#include <iostream>

#include <libxml2/libxml/xpath.h>
#include <libxml2/libxml/xpathInternals.h>
#include <libxml2/libxml/tree.h>

XMLFileHandler::XMLFileHandler(std::string filePath) {
	
	this->doc = this->createDoc(filePath);
}

xmlDocPtr XMLFileHandler::getDoc() {
	return this->doc;
}

xmlXPathObjectPtr XMLFileHandler::getNodeSet(std::string xPathString) {
	xmlChar *xpath = (xmlChar*) xPathString.c_str();
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	context = xmlXPathNewContext(doc);
	if (context == NULL) {
		std::printf("Error in xmlXPathNewContext\n");
		return NULL;
	}
	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) {
		std::printf("Error in xmlXPathEvalExpression\n");
		return NULL;
	}
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
        std::printf("No result\n");
		return NULL;
	}
	return result;

}

xmlDocPtr XMLFileHandler::createDoc(std::string filePath) {
	xmlDocPtr doc;
	doc = xmlParseFile(filePath.c_str());
	
	if (doc == NULL ) {
		std::fprintf(stderr,"Document not parsed successfully. \n");
		return NULL;
	}

	return doc;
}


XMLFileHandler::~XMLFileHandler() {
	xmlFreeDoc(this->doc);
	xmlCleanupParser();
}
