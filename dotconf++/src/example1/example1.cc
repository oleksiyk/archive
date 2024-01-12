/*  Copyright (C) 2003 Aleksey Krivoshey <voodoo@foss.kharkov.ua>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include <stdio.h>
#include "dotconfpp.h"

const char * requiredOptions[] = {"Users", NULL};

int parsePolicy(const DOTCONFDocument *conf)
{
    const DOTCONFDocumentNode *pnode = NULL;
    
    while((pnode = conf->findNode("Policy", NULL, pnode)) != NULL){
	const char * user = pnode->getValue();
	const char * v = NULL;
	if(!user){
	    printf("file %s, line %d: %s must have value\n", pnode->getConfigurationFileName(),
		pnode->getConfigurationLineNumber(), pnode->getName());
	    return -1;
	}
	
	// Read
	const DOTCONFDocumentNode * vnode = conf->findNode("Read", pnode);
	if(vnode){
	    if((v = vnode->getValue()) != NULL){
		printf("Read for user '%s' = '%s'\n", user, v);
	    }
	}
	// Write
	vnode = conf->findNode("Write", pnode);
	if(vnode){
	    if((v = vnode->getValue()) != NULL){
		printf("Write for user '%s' = '%s'\n", user, v);
	    }
	}
    }
    return 0;
}

int main()
{    
    DOTCONFDocument *conf = new DOTCONFDocument(DOTCONFDocument::CASEINSENSITIVE);
    const DOTCONFDocumentNode * node = NULL;
    const char * v = NULL;
    char * fqdn = NULL;

    conf->setRequiredOptionNames(requiredOptions);

    if(conf->setContent("example1.conf") == -1){
	return -1;
    }
    
    // ---
        
    // find "Users" and print them
    // do not need to free memory for values returned by node->getValue()
    node = conf->findNode("Users");
    int i = 0;    
    while(( v = node->getValue(i++)) != NULL){
	printf("user = '%s'\n", v);
    }
    
    // find "serverFQDN" and save it for future use ( after deleting conf)
    node = conf->findNode("serverFQDN");    
    if((v = node->getValue()) != NULL){
	fqdn = strdup(v);
    }
    
    // find global "read" and "write"
    node = conf->findNode("Read");
    if((v = node->getValue()) != NULL){
	printf("Global Read = '%s'\n", v);
    }
    node = conf->findNode("Write");
    if((v = node->getValue()) != NULL){
	printf("Global Write = '%s'\n", v);
    }
    
    // find "Read" and "Write" values inside "Policy" tag
    if(parsePolicy(conf) == -1){
	return -1;
    }
    
    // all memory used by conf is freed
    delete(conf);
    
    if(fqdn){
	printf("serverFQDN = '%s'\n", fqdn);
	free(fqdn);
    }
    
    return 0;        
}
