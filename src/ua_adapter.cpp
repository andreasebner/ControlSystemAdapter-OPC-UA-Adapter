/*
 * Copyright (c) 2016 Chris Iatrou <Chris_Paul.Iatrou@tu-dresden.de>
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

extern "C" {
#include "unistd.h"
#include "csa_namespaceinit_generated.h" // Output des pyUANamespacecompilers
#include <stdio.h>
#include <stdlib.h>
}

#include <thread>
#include <future>
#include <functional>     // std::ref

#include "csa_config.h"

#include "ua_adapter.h"
#include "ua_processvariable.h"
#include "ua_additionalvariable.h"
#include "xml_file_handler.h"
#include <ipc_manager.h>

#include "ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h"
#include "ChimeraTK/ControlSystemAdapter/ControlSystemSynchronizationUtility.h"
#include "ChimeraTK/ControlSystemAdapter/PVManager.h"
#include "ChimeraTK/ControlSystemAdapter/TimeStamp.h"

using namespace ChimeraTK;
using namespace std;

ua_uaadapter::ua_uaadapter(string configFile) : ua_mapped_class() {
	this->fileHandler = new xml_file_handler(configFile);
	
	this->readConfig();
	this->constructServer();
	
	this->mapSelfToNamespace();
	
	this->readAdditionalNodes();
	
}

ua_uaadapter::~ua_uaadapter() {
	
	if (this->isRunning()) {
		this->doStop();
	}	
	//UA_Server_delete(this->mappedServer);
	this->fileHandler->~xml_file_handler();
	for(auto ptr : variables) delete ptr;
	for(auto ptr : additionalVariables) delete ptr;
	for(auto ptr : mappedVariables) delete ptr;

}

void ua_uaadapter::constructServer() {

    this->server_config = UA_ServerConfig_standard;
    this->server_nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, this->serverConfig.opcuaPort);
    this->server_config.logger = UA_Log_Stdout;
    this->server_config.networkLayers = &this->server_nl;
    this->server_config.networkLayersSize = 1;
		
		this->server_config.enableUsernamePasswordLogin = this->serverConfig.UsernamePasswordLogin;
		this->server_config.enableAnonymousLogin = !this->serverConfig.UsernamePasswordLogin;
		
    UA_UsernamePasswordLogin* usernamePasswordLogins = new UA_UsernamePasswordLogin; //!< Brief description after the member
		usernamePasswordLogins->password = UA_STRING((char*)this->serverConfig.password.c_str());
		usernamePasswordLogins->username = UA_STRING((char*)this->serverConfig.username.c_str());
		this->server_config.usernamePasswordLogins = usernamePasswordLogins;
		this->server_config.usernamePasswordLoginsSize = (size_t)(usernamePasswordLogins->password.length + usernamePasswordLogins->username.length);
		this->server_config.applicationDescription.applicationName =  UA_LOCALIZEDTEXT((char*)"en_US", (char*)this->serverConfig.applicationName.c_str());
		this->server_config.applicationDescription.gatewayServerUri = UA_STRING((char*)"GatewayURI");
		this->server_config.applicationDescription.applicationUri = UA_STRING((char*)"opc.tcp://localhost");
		this->server_config.applicationDescription.applicationType = UA_APPLICATIONTYPE_SERVER;
		this->server_config.buildInfo.productName = UA_STRING((char*)"csa_opcua_adapter");
		this->server_config.buildInfo.productUri = UA_STRING((char*)"HZDR OPCUA Server");
		this->server_config.buildInfo.manufacturerName = UA_STRING((char*)"TU Dresden - Professur für Prozessleittechnik");
		
    this->mappedServer = UA_Server_new(this->server_config);
		this->baseNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
		
		csa_namespaceinit_generated(this->mappedServer);
}

void ua_uaadapter::readConfig() {
	
	string xpath = "//config";
	xmlXPathObjectPtr result = this->fileHandler->getNodeSet(xpath);	
	string placeHolder = "";
	if(result) {
		xmlNodeSetPtr nodeset = result->nodesetval;
		// There should be only one <config>-Tag in config file
		if(nodeset->nodeNr > 1) {
			//cout << "More than one <config>-Tag was found in config file. Please provide only one Tag." << endl;
			throw runtime_error ("To many <config>-Tags in config file");
		}		
		
		placeHolder = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[0], "rootFolder");
		if(!placeHolder.empty()) {
			this->serverConfig.rootFolder = placeHolder;
		}
		
		placeHolder = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[0], "description");
		if(!placeHolder.empty()) {
			this->serverConfig.descriptionFolder = placeHolder;
		}
	}	
	
	result = this->fileHandler->getNodeSet(xpath + "//login");	
	if(result) {
		xmlNodeSetPtr nodeset = result->nodesetval;
		this->serverConfig.UsernamePasswordLogin = UA_TRUE;
		placeHolder = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[0], "password");
		if(!placeHolder.empty()) {
			this->serverConfig.password = placeHolder;
		}
		
		placeHolder = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[0], "username");
		if(!placeHolder.empty()) {
			this->serverConfig.username = placeHolder;
		}
	}
	
	result = this->fileHandler->getNodeSet(xpath +"//serverConfig");	
	if(result) {
		xmlNodeSetPtr nodeset = result->nodesetval;
		string opcuaPort = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[0], "port");
		if(opcuaPort.compare("") != 0) {
			this->serverConfig.opcuaPort = std::stoi(opcuaPort);
		}
		else {
			throw runtime_error ("No 'port'-Attribute is set in config file");
		}
		
		placeHolder = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[0], "applicationName");
		if(placeHolder.compare("") != 0) {
			this->serverConfig.applicationName = placeHolder;
		}
		else {
			throw runtime_error ("No 'applicationName'-Attribute is set in config file");
		}
	}
	else {
		throw runtime_error ("No <serverConfig>-Tag in config file");
	}
}

void ua_uaadapter::readAdditionalNodes() {
	xmlXPathObjectPtr result = this->fileHandler->getNodeSet("//additionalNodes");	
	if(result) {
		xmlNodeSetPtr nodeset = result->nodesetval;
		for (int32_t i=0; i < nodeset->nodeNr; i++) {
			
			string folderName = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[i], "folderName"); 
			string folderDescription = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[i], "description");
			if(folderName.empty()) {
				cout << "There is no folder name specified, ignore <additionalNode>-Element. Please set a name" << endl;
			}
			else {
				UA_NodeId folderNodeId = this->createFolder(this->ownNodeId, folderName, folderDescription);
					
				vector<xmlNodePtr> variableNodeList = this->fileHandler->getNodesByName(nodeset->nodeTab[i]->children, "variable");
				
				for(auto variableNode: variableNodeList) {
					string name = this->fileHandler->getAttributeValueFromNode(variableNode, "name").c_str();
					string value = this->fileHandler->getAttributeValueFromNode(variableNode, "value");
					string description = this->fileHandler->getAttributeValueFromNode(variableNode, "description");
					this->additionalVariables.push_back(new ua_additionalvariable(this->mappedServer, folderNodeId, name, value, description));
				}
			}
		}
	}
}

void ua_uaadapter::workerThread() {
	if (this->mappedServer == nullptr) {
		return;
	}
    
	UA_Boolean runUAServer = UA_TRUE;
		
// 		std::packaged_task<UA_StatusCode(UA_Server*, UA_Boolean*)> task(UA_Server_run);
//    std::future<UA_StatusCode> result = task.get_future();
//    std::thread *serverThread = new std::thread (std::move(task), this->mappedServer, &runUAServer);
		
		//std::future<UA_StatusCode> serverThread = std::async(std::launch::async, UA_Server_run, this->mappedServer, &runUAServer);
		

 	thread *serverThread = new std::thread(UA_Server_run, this->mappedServer, &runUAServer);
//  		std::future<UA_StatusCode> serverThread = std::async(std::launch::async, UA_Server_run, this->mappedServer, &runUAServer);
//  		if(serverThread.get() != UA_STATUSCODE_GOOD) {
//  			cout << "Error during establishing the network interface." << endl;
//  			exit(0);
//   		}

	while (runUAServer == true) {
		 if (! this->isRunning()) {
			 runUAServer = false;
		}
		sleep(1);
	}
	
 	if (serverThread->joinable()) {
  	serverThread->join();
  }
 
 	delete serverThread;
	serverThread = NULL;
}

void ua_uaadapter::addVariable(std::string varName, boost::shared_ptr<ControlSystemPVManager> csManager) {
	
	this->variables.push_back(new ua_processvariable(this->mappedServer, this->variablesListId, varName, csManager));
	
	xmlXPathObjectPtr result = this->fileHandler->getNodeSet("//map");
	xmlNodeSetPtr nodeset;
	string srcVarName = "";
	string applicName = "";
	
	// TODO. What happen if application name are not unique?
	if(result) {
		nodeset = result->nodesetval;
		for (int32_t i=0; i < nodeset->nodeNr; i++) {
			srcVarName = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[i], "sourceVariableName");
			//keyword = xmlNodeListGetString(this->fileHandler->getDoc(), nodeset->nodeTab[i]->xmlChildrenNode, 1);
			if(varName.compare(srcVarName) == 0) {
				// get name attribute from <application>-tag
				applicName = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[i]->parent, "name");
				// Check if "rename" is not empty
				string renameVar = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[i], "rename");
				string engineeringUnit = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[i], "engineeringUnit");
				string description = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[i], "description");
			
				// Application Name have to be unique!!!
				UA_NodeId appliFolderNodeId = this->existFolder(this->ownNodeId, applicName);
				FolderInfo newFolder;
				if(UA_NodeId_isNull(&appliFolderNodeId)) {
					newFolder.folderName = applicName;
					newFolder.folderNodeId = this->createFolder(this->ownNodeId, applicName);
					this->folderVector.push_back(newFolder);
					appliFolderNodeId = newFolder.folderNodeId;
				}
				
					vector<string> varPathVector;
					vector<xmlNodePtr> nodeVectorUnrollPath = this->fileHandler->getNodesByName(nodeset->nodeTab[i]->children, "unrollPath");
					string seperator = "";
					bool unrollPathIs = false;
					for(auto nodeUnrollPath: nodeVectorUnrollPath) {
						string shouldUnrollPath = this->fileHandler->getContentFromNode(nodeUnrollPath);
						if(shouldUnrollPath.compare("True") == 0) {
							seperator = seperator + this->fileHandler->getAttributeValueFromNode(nodeUnrollPath, "pathSep");
							unrollPathIs = true;
						}	
					}
					
					if(!seperator.empty()) {
						vector<string> newPathVector = this->fileHandler->praseVariablePath(srcVarName, seperator);
						varPathVector.insert(varPathVector.end(), newPathVector.begin(), newPathVector.end());
					}
						
			
				// assumption last element is name of variable, hence no folder for name is needed
				if(renameVar.compare("") == 0 && !unrollPathIs) {
					renameVar = srcVarName;
				}
				else {
					if(unrollPathIs && renameVar.compare("") == 0) {
						renameVar = varPathVector.at(varPathVector.size()-1);
						varPathVector.pop_back();
					}	
					else {
						if(varPathVector.size() > 0) {
							varPathVector.pop_back();
						}
					}
				}
				std::cout << "Variable '" << srcVarName << "' wird unter '" << renameVar << "' in Application Name '" << applicName << "' eingetragen." << std::endl;
				
				vector<xmlNodePtr> nodeVectorFolderPath = this->fileHandler->getNodesByName(nodeset->nodeTab[i]->children, "folder");
				vector<string> folderPathVector;
				bool createdVar = false;
				for(auto nodeFolderPath: nodeVectorFolderPath) {
						
						string folderPath = this->fileHandler->getContentFromNode(nodeFolderPath);
						if(folderPath.empty() && unrollPathIs) {
							break;
						}
			
						folderPathVector = this->fileHandler->praseVariablePath(folderPath);
						// Create folders
						UA_NodeId newFolderNodeId = appliFolderNodeId;
						if(folderPathVector.size() > 0) {
							newFolderNodeId = this->createFolderPath(newFolderNodeId, folderPathVector);
						}
						
						if(varPathVector.size() > 0) {
							newFolderNodeId = this->createFolderPath(newFolderNodeId, varPathVector);
						}						
						this->mappedVariables.push_back(new ua_processvariable(this->mappedServer, newFolderNodeId, varName, renameVar, engineeringUnit, description, csManager));
						createdVar = true;
				}
				
				// in case no <folder> or <unrollpath> is set
				if(!createdVar) {
					UA_NodeId newFolderNodeId = appliFolderNodeId;
					if(varPathVector.size() > 0) {
						newFolderNodeId = this->createFolderPath(newFolderNodeId, varPathVector);
					}
					this->mappedVariables.push_back(new ua_processvariable(this->mappedServer, newFolderNodeId, varName, renameVar, engineeringUnit, description, csManager));
				}
 			}
		}
	}

	xmlXPathFreeObject (result);
}

vector<ua_processvariable *> ua_uaadapter::getVariables() {
	return this->variables;
}

UA_NodeId ua_uaadapter::createUAFolder(UA_NodeId basenodeid, std::string folderName, std::string description) {
	// FIXME: Check if folder name a possible name or should it be escaped (?!"§%-:, etc)
	UA_StatusCode retval = UA_STATUSCODE_GOOD;
	UA_NodeId createdNodeId = UA_NODEID_NULL;
  
	if(UA_NodeId_equal(&baseNodeId, &createdNodeId) == UA_TRUE) {
		return createdNodeId; // Something went UA_WRING (initializer should have set this!)
	}
    
	// Create our toplevel instance
	UA_ObjectAttributes oAttr; 
	UA_ObjectAttributes_init(&oAttr);
	// Classcast to prevent Warnings
	oAttr.displayName = UA_LOCALIZEDTEXT((char*)"en_US", (char*)folderName.c_str());
	oAttr.description = UA_LOCALIZEDTEXT((char*)"en_US", (char*)description.c_str());    
	
	UA_INSTATIATIONCALLBACK(icb);
	UA_Server_addObjectNode(this->mappedServer, UA_NODEID_NUMERIC(1,0),
                          basenodeid, UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                          UA_QUALIFIEDNAME(1, (char*)folderName.c_str()), UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), oAttr, &icb, &createdNodeId);
    
	return createdNodeId;
}

UA_StatusCode ua_uaadapter::mapSelfToNamespace() {
	UA_StatusCode retval = UA_STATUSCODE_GOOD;
	UA_NodeId createdNodeId = UA_NODEID_NULL;
	
	if (UA_NodeId_equal(&this->baseNodeId, &createdNodeId) == UA_TRUE) {
		return 0; // Something went UA_WRING (initializer should have set this!)
	}
	
	// Create our toplevel instance
	UA_ObjectAttributes oAttr; 
	UA_ObjectAttributes_init(&oAttr);
	// Classcast to prevent Warnings
	oAttr.displayName = UA_LOCALIZEDTEXT((char*)"en_US", (char*)this->serverConfig.rootFolder.c_str());
	oAttr.description = UA_LOCALIZEDTEXT((char*)"en_US", (char*)this->serverConfig.descriptionFolder.c_str());
    
	UA_INSTATIATIONCALLBACK(icb);
	UA_Server_addObjectNode(this->mappedServer, UA_NODEID_NUMERIC(1,0),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, (char*)this->serverConfig.rootFolder.c_str()), UA_NODEID_NUMERIC(CSA_NSID, UA_NS2ID_MTCAMODULE), oAttr, &icb, &createdNodeId);
    
	this->ownNodeId = createdNodeId;
	// Nodes "Variables" and "Constants" where created on object instantiation, we need these IDs to add new process variables to them...
	UA_NodeId_copy(nodePairList_getTargetIdBySourceId(this->ownedNodes, UA_NODEID_NUMERIC(CSA_NSID, CSA_NSID_VARIABLES)), &this->variablesListId);
		
    return UA_STATUSCODE_GOOD;
}

UA_NodeId ua_uaadapter::getOwnNodeId() {
	return this->ownNodeId;
}

UA_NodeId ua_uaadapter::existFolderPath(UA_NodeId basenodeid, std::vector<string> folderPath) {
	UA_NodeId lastNodeId = basenodeid;
	for(std::string t : folderPath) {
		lastNodeId = this->existFolder(lastNodeId, t);
		if(UA_NodeId_isNull(&lastNodeId)) {
			return UA_NODEID_NULL;
		}
	}
	return lastNodeId;
}

UA_NodeId ua_uaadapter::existFolder(UA_NodeId basenodeid, string folder) {
	UA_NodeId lastNodeId = UA_NODEID_NULL;
	for(uint32_t i=0; i < this->folderVector.size(); i++) {
		if((this->folderVector.at(i).folderName.compare(folder) == 0) && (UA_NodeId_equal(&this->folderVector.at(i).prevFolderNodeId, &basenodeid))) {
			return this->folderVector.at(i).folderNodeId;
		}
	}
	return UA_NODEID_NULL;
}

UA_NodeId ua_uaadapter::createFolderPath(UA_NodeId basenodeid, std::vector<string> folderPath) {
		
 	if(UA_NodeId_isNull(&basenodeid)){
 		return UA_NODEID_NULL;
 	}
	
	// Check if path exist
	UA_NodeId toCheckNodeId = existFolderPath(basenodeid, folderPath);
	int32_t starter4Folder = 0;
	UA_NodeId nextNodeId = basenodeid;
	UA_NodeId startNodeId = basenodeid;
 	if(UA_NodeId_isNull(&toCheckNodeId)) {
		bool setted = false;
		// Check if path exist partly 
		for(uint32_t m=0; m < folderPath.size(); m++) {
			for(uint32_t i=0; i < this->folderVector.size(); i++) {
				// get correct folder NodeId from first folderPath element
				if(!setted && (folderPath.at(m).compare(this->folderVector.at(i).folderName) == 0) && (UA_NodeId_equal(&this->folderVector.at(i).prevFolderNodeId, &nextNodeId)) && ((m+1) < folderPath.size())) {
					// remember on witch position the folder still exist
					setted = true;
					starter4Folder = m+1;
					nextNodeId = this->folderVector.at(i).folderNodeId;
				}
				if(setted) {
					break;
				}
			}
			setted = false;
		}
	}
	else {
		// Path exist nothing to do
		return toCheckNodeId;
	}

	UA_NodeId prevNodeId = nextNodeId;
	// use the remembered position to start the loop
	for(uint32_t m=starter4Folder; m < folderPath.size(); m++) {
		prevNodeId = this->createFolder(prevNodeId, folderPath.at(m));		
	}
	// return last created folder UA_NodeId
	return prevNodeId;
}

UA_NodeId ua_uaadapter::createFolder(UA_NodeId basenodeid, string folderName, string description) {
	
	if(UA_NodeId_isNull(&basenodeid)){
		return UA_NODEID_NULL;
	}
	
	// Check if path exist
	UA_NodeId toCheckNodeId = existFolder(basenodeid, folderName);
	FolderInfo newFolder;
 	if(UA_NodeId_isNull(&toCheckNodeId)) {
		newFolder.folderName = folderName;
		newFolder.folderNodeId = this->createUAFolder(basenodeid, folderName);	
		newFolder.prevFolderNodeId = basenodeid;
		this->folderVector.push_back(newFolder);
	}

	return newFolder.folderNodeId;
}

vector<string> ua_uaadapter::getAllNotMappableVariablesNames() {
	
	vector<string> notMappableVariablesNames;
	xmlXPathObjectPtr result = this->fileHandler->getNodeSet("//map");
	
	if(result) {
		xmlNodeSetPtr nodeset = result->nodesetval;
		for (int32_t i=0; i < nodeset->nodeNr; i++) {
			//for(auto var:this->variables) {
			bool mapped = false;
			string mappedVar = this->fileHandler->getAttributeValueFromNode(nodeset->nodeTab[i], "sourceVariableName");
			for(auto var:this->getVariables()) {
				if(var->getName().compare(mappedVar) == 0) {
					mapped = true;
				}
			}
 			if(!mapped) {
 				notMappableVariablesNames.push_back(mappedVar);
 			}
		}
	}
	
	return notMappableVariablesNames;
}

UA_DateTime ua_uaadapter::getSourceTimeStamp() {
	return UA_DateTime_now();
}