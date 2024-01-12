/*  Copyright (C) 2003 FOSS-On-Line <http://www.foss.kharkov.ua>,
*   Aleksey Krivoshey <krivoshey@users.sourceforge.net>
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

#include "module_manager.h"

using namespace Outpost;

ModuleManager::ModuleManager():
    log(Outpost::__server->getLog()),
    loadedModules(NULL), loadedModulesCount(0)
{
    errlog = log->getLogId("errlog");
};

ModuleManager::~ModuleManager()
{
    for(size_t i = 0; i<loadedModulesCount; i++){
        loadedModules[i]->module->finish();
        delete loadedModules[i]->module;

        if(dlclose(loadedModules[i]->dlHandle) != 0){
            log->error(errlog, 0, "dlclose() failed: %s", dlerror());
        }

        delete(loadedModules[i]);
    }
    free(loadedModules);
}

int ModuleManager::loadModules(const DOTCONFDocumentNode * addModuleParentNode, Module::Type moduleType)
{
    const DOTCONFDocument * conf = addModuleParentNode->getDocument();
    const DOTCONFDocumentNode * node = NULL;

    const char * moduleAlias = NULL;
    const char * modulePath = NULL;

    while( (node = conf->findNode("AddModule", addModuleParentNode, node)) != NULL){
        moduleAlias = node->getValue(0);
        modulePath = node->getValue(1);

        if(moduleAlias == NULL || modulePath == NULL){
            log->error(errlog, 0, "ModuleManager: file %s, line %d: AddModule directive requires two parameters: module_alias and module_path",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
        //check this alias for duplicate
        for(size_t i = 0; i<loadedModulesCount; i++){
            if(!strcmp(loadedModules[i]->module->getAlias(), moduleAlias)){
                log->error(errlog, 0, "ModuleManager: file %s, line %d: Duplicate module alias '%s'",
                    node->getConfigurationFileName(), node->getConfigurationLineNumber(), moduleAlias);
                return -1;
            }
        }

        //dlopen module
        log->info(errlog ,2, "loading module '%s'", modulePath);
        void * handle = dlopen(modulePath, RTLD_NOW);
        if(handle == NULL){
            log->error(errlog,0, "failed to load module : %s", dlerror());
            return -1;
        }

        //resolve getModule symbol
        const char * error = NULL;
        OUTPOST_GET_MODULE_FUNC_T getModule = (OUTPOST_GET_MODULE_FUNC_T)dlsym(handle, "getModule");
        if(( error = dlerror()) != NULL){
            log->error(errlog,0, "ModuleManager: %s: %s", modulePath, error);
            (void) dlclose(handle);
            return -1;
        }

        //get instance of the module class
        Module * module = getModule(moduleAlias);
        if(module->moduleType() != moduleType){
            log->error(errlog,0, "module %s cannot be loaded because its type is different from being requested", modulePath);
            delete module;
            (void) dlclose(handle);
            return -1;
        }

        if(module->initialize(node) == -1){
            delete module;
            (void) dlclose(handle);
            return -1;
        }

        ++loadedModulesCount;
        loadedModules = (ModuleInstance**)realloc(loadedModules, loadedModulesCount*sizeof(ModuleInstance*));
	if(loadedModules == NULL){
	    --loadedModulesCount;
	    return -1;
	}
	ModuleInstance * moduleInstance = new ModuleInstance(handle, module);
        loadedModules[loadedModulesCount-1] = moduleInstance;
    }

    return 0;
}

Module * ModuleManager::getModuleByAlias(const char * moduleAlias)const
{
    ModuleInstance * instance = NULL;

    for(size_t i = 0; i<loadedModulesCount; i++){
        instance = loadedModules[i];
        if(!strcmp(instance->module->getAlias(), moduleAlias)){
            return instance->module;
        }
    }
    return NULL;
}

Module * ModuleManager::getModuleByType(Module::Type moduleType, const Module * prevFoundModule)const
{
    ModuleInstance * instance = NULL;

    for(size_t i = 0; i<loadedModulesCount; i++){
        instance = loadedModules[i];
        if(prevFoundModule != NULL){
            if(instance->module != prevFoundModule){
                continue;
            } else {
                prevFoundModule = NULL;
                continue;
            }
        }
        if( instance->module->moduleType() == moduleType ){
            return instance->module;
        }
    }
    return NULL;
}


