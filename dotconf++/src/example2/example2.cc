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

const char * requiredOptions[] = {"AddModule", "LogOptions", "LogFile", NULL};

void printNode(const DOTCONFDocumentNode * node)
{
    static int spCount = 0;

    for(int i = 0; i<spCount; i++){
        printf("    ");
    }
    spCount++;

    printf("nodeName=%s,", node->getName());

    const char * value = NULL;
    int i = 0;
    while( (value = node->getValue(i++)) != NULL){
        printf(" value='%s'", value);
    }
    printf("\n");

    const DOTCONFDocumentNode * child = node->getChildNode();
    
    if(child){
        do {
            printNode(child);
        } while ( (child = child->getNextNode()) != NULL );
    }
    spCount--;
}

int main(int argc, char * argv[])
{    
    DOTCONFDocument conf(DOTCONFDocument::CASEINSENSITIVE);

    conf.setRequiredOptionNames(requiredOptions);

    if(conf.setContent("example2.conf") == 0){
        
        const DOTCONFDocumentNode * node = conf.getFirstNode();

        do {
            printNode(node);
        } while ((node = node->getNextNode()) != NULL);

    }
}
