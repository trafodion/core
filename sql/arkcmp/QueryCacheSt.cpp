/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2002-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         QueryCacheSt.cpp
 * Description:
 *
 *
 * Created:      3/19/2002
 * Language:     C++
 *
 *****************************************************************************
 */

#include "QueryCacheSt.h"
#include "QCache.h"
#include "CmpMain.h"
#include "Globals.h"
#include "Context.h"

class cacheInfoList : public NAList<QueryCache*>
{
public:
    cacheInfoList()
    : counter_(0)
    , currCacheIndex_(-1)
    , currEntryIndex_(-1)
    , currEntriesPerKey_(-1)
    , currQCache_(NULL)
    , currHKeyPtr_(NULL)
    , currValueList_(NULL)
    , HQCIterator_(NULL)
    {}
    virtual ~cacheInfoList()
    {
        if(HQCIterator_)
            delete HQCIterator_;
    }

    NABoolean initializeList(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, SP_ERROR_STRUCT* error)
    {
	 //extract ISP input, find QueryCache belonging to specified context
	 //and use it for fetch later
      Lng32 maxSize = 16;
      char receivingField[maxSize+1];
      if (eFunc (0, inputData, (Lng32)maxSize, receivingField, FALSE) == SP_ERROR_EXTRACT_DATA)
      {
          error->error = arkcmpErrorISPFieldDef;
          return FALSE;
      }
       //choose context
      NAString qCntxt = receivingField;
      qCntxt.toLower();
      const NAArray<CmpContextInfo*> & ctxInfos = GetCliGlobals()->currContext()->getCmpContextInfo();
      //find the specified context
      if(ctxInfos.entries() == 0){
        //for remote compiler
        if( strncmp(qCntxt.data(), "all", 3)==0 ||strncmp(qCntxt.data(), "user", 4)==0 )
          insert(CURRENTQCACHE);
      }
      else
      {
        if(strncmp(qCntxt.data(), "all", 3)==0)
        {//add caches of all contexts to list
           for(Int32 i = 0; i < ctxInfos.entries(); i++ )
               insert(ctxInfos[i]->getCmpContext()->getQueryCache());
        }
        else if(strncmp(qCntxt.data(), "user", 4)==0)
        {//add user query cache to list
           for(Int32 i = 0; i < ctxInfos.entries(); i++ )
               if(ctxInfos[i]->isSameClass("NONE")){
                   insert(ctxInfos[i]->getCmpContext()->getQueryCache());
                   break;
               }
        }
        else if(strncmp(qCntxt.data(), "meta", 4)==0)
        {//add metadata query cache to list
           for(Int32 i = 0; i < ctxInfos.entries(); i++ )
               if(ctxInfos[i]->isSameClass("META")){
                   insert(ctxInfos[i]->getCmpContext()->getQueryCache());
                   break;
               }
        }
        else if(strncmp(qCntxt.data(), "ustats", 6)==0)
        {//add ustats query cache to list
           for(Int32 i = 0; i < ctxInfos.entries(); i++ )
               if(ctxInfos[i]->isSameClass("USTATS")){
                   insert(ctxInfos[i]->getCmpContext()->getQueryCache());
                   break;
               }
        }
      }           
      return TRUE;
    }
    
    Int32 counter_;
    
protected:
    Int32 currCacheIndex_;
    Int32 currEntryIndex_;
    Int32 currEntriesPerKey_;
    QueryCache* currQCache_;
    HQCCacheKey* currHKeyPtr_;
    HQCCacheData* currValueList_;
    HQCHashTblItor* HQCIterator_;
    LRUList::iterator SQCIterator_;
};

struct InfoStruct : public cacheInfoList
{
    NABoolean initQueryCacheStat(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, SP_ERROR_STRUCT* error)
    {
        NABoolean result = initializeList( inputData, eFunc, error);
       //iterator start with preparser cache entries of first query cache, if any
       if(entries() > 0) {
         currCacheIndex_ = 0;
       }
       return result;
    }
    
    NABoolean initQueryCacheEntries(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, SP_ERROR_STRUCT* error)
    {
        NABoolean result = initializeList( inputData, eFunc, error);
        //iterator start with preparser cache entries of first query cache, if any
        if(entries() > 0){
            currCacheIndex_ = 0;
            SQCIterator_ = at(currCacheIndex_)->beginPre();
        }
        return result;
    }

    NABoolean initHybridQueryCacheStat(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, SP_ERROR_STRUCT* error)
    {
        NABoolean result = initializeList( inputData, eFunc, error);
       if(entries() > 0){
          currCacheIndex_ = 0;
       }
       return result;
    }

    NABoolean initHybridQueryCacheEntries(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, SP_ERROR_STRUCT* error)
    {
        NABoolean result = initializeList( inputData, eFunc, error);
        if(entries() > 0)
        { 
            currCacheIndex_ = 0;
            currQCache_ = at(currCacheIndex_);
            HQCIterator_ = new HQCHashTblItor (currQCache_->getHQC()->begin());
         }
         return result;
    }
     //init() need to be called before getNext
     NABoolean getNext(QueryCacheStats & details);
     NABoolean getNext(QueryCacheDetails & details);
     NABoolean getNext(HybridQueryCacheStats & details);
     NABoolean getNext(HybridQueryCacheDetails & details);
};

NABoolean InfoStruct::getNext(QueryCacheStats & details)
{
  if(entries() == 0 || currCacheIndex_ >= entries())
    return FALSE;//all entries of all caches are fetched, we are done!

  at(currCacheIndex_++)->getCacheStats(details);
    return TRUE;
}

NABoolean InfoStruct::getNext(QueryCacheDetails & details)
{
  if(entries() == 0 || currCacheIndex_ >= entries())
    return FALSE;//all entries of all caches are fetched, we are done!

  if (SQCIterator_ == at(currCacheIndex_)->endPre()) {
    // end of preparser cache, continue with postparser entries
    SQCIterator_ = at(currCacheIndex_)->begin();
  }

  if (SQCIterator_ == at(currCacheIndex_)->end()) {
    // end of postparser cache entries, shift to next cache, if any
    currCacheIndex_++;
    if(entries() > currCacheIndex_)
      SQCIterator_ = at(currCacheIndex_)->beginPre();
    return getNext(details);
  }
  at(currCacheIndex_)->getEntryDetails(SQCIterator_++, details);
  return TRUE;
}

NABoolean InfoStruct::getNext(HybridQueryCacheStats & details)
{
    if(entries() == 0 || currCacheIndex_ >= entries())
      return FALSE;//all entries of all caches are fetched, we are done!

    at(currCacheIndex_++)->getHQCStats(details);
    return TRUE;
}

NABoolean InfoStruct::getNext(HybridQueryCacheDetails & details)
{
      //when come to end of query cache list, iterator_ is set to NULL
      if(HQCIterator_ == NULL)
        return FALSE;
        
      //get next entry valid curQCache, currHKeyPtr_, currValueList_
      if( currEntryIndex_ >= currEntriesPerKey_ )
      {
        //get next key-value pair
        HQCIterator_->getNext(currHKeyPtr_, currValueList_);
        if(currHKeyPtr_ && currValueList_ && currQCache_->isCachingOn())
        {
          currEntryIndex_ = 0;
          currEntriesPerKey_ = currValueList_->entries();
        }
        else//interator == end
        {// to next cache in cacheList, and get valid hkeyPtr, valueList
            currCacheIndex_++;
            if( entries() > currCacheIndex_)
            {  //point to begin of next query cache
               currQCache_ = at(currCacheIndex_);
               if(HQCIterator_)
                 delete HQCIterator_;
               HQCIterator_ = NULL;
               HQCIterator_ = new HQCHashTblItor (currQCache_->getHQC()->begin());
            }
            else
            {//end of list
               if(HQCIterator_)
                 delete HQCIterator_;
               HQCIterator_ = NULL;
            }
            return getNext(details);
        }
      }
      //just get next entry in previous valueList
      currQCache_->getHQCEntryDetails(currHKeyPtr_, (*currValueList_)[currEntryIndex_++], details);
      return TRUE;
}


SP_STATUS QueryCacheStatStoredProcedure::sp_InputFormat(SP_FIELDDESC_STRUCT *inputFieldFormat,
				  Lng32 numFields,
				  SP_COMPILE_HANDLE spCompileObj,
				  SP_HANDLE spObj,
	    			  SP_ERROR_STRUCT *error)
{
      if ( numFields != 2 )
      {
        //accepts 1 input columns
        error->error = arkcmpErrorISPWrongInputNum;
        strcpy(error->optionalString[0], "QueryCache");
        error->optionalInteger[0] = 2;
        return SP_FAIL;
      }
  
     //column as input parameter for ISP, specifiy query cache of metadata or user context
     strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "instance char(16)  not null");
     strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "location char(16)  not null"); 
     return SP_SUCCESS;
}

SP_STATUS QueryCacheStatStoredProcedure::sp_NumOutputFields(Lng32 *numFields,
							    SP_COMPILE_HANDLE spCompileObj,
							    SP_HANDLE spObj,
							    SP_ERROR_STRUCT *error)
{
  *numFields = 21;
  return SP_SUCCESS;
}


// Specifies the columns of the QueryCache table and their types
SP_STATUS QueryCacheStatStoredProcedure::sp_OutputFormat(SP_FIELDDESC_STRUCT *format,
							 SP_KEYDESC_STRUCT keyFields[],
							 Lng32 *numKeyFields,
							 SP_HANDLE spCompileObj,
							 SP_HANDLE spObj,
							 SP_ERROR_STRUCT *error)
{
  strcpy(&((format++)->COLUMN_DEF[0]), "Avg_template_size     	INT UNSIGNED"); //  0
  strcpy(&((format++)->COLUMN_DEF[0]), "Current_size       	INT UNSIGNED"); //  1
  strcpy(&((format++)->COLUMN_DEF[0]), "Max_cache_size        	INT UNSIGNED"); //  2
  strcpy(&((format++)->COLUMN_DEF[0]), "Max_num_victims       INT UNSIGNED"); //  3
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_entries    	    	INT UNSIGNED"); //  4
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_plans    	    	INT UNSIGNED"); //  5
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_compiles        	INT UNSIGNED"); //  6
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_recompiles        	INT UNSIGNED"); //  7
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_retries    	    	INT UNSIGNED"); //  8
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_cacheable_parsing  	INT UNSIGNED"); //  9
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_cacheable_binding  	INT UNSIGNED"); // 10
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_cache_hits_parsing 	INT UNSIGNED"); // 11
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_cache_hits_binding 	INT UNSIGNED"); // 12
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_cacheable_too_large	INT UNSIGNED"); // 13
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_displaced  		INT UNSIGNED"); // 14
  strcpy(&((format++)->COLUMN_DEF[0]), "Optimization_level    	CHAR(10 BYTES) character set utf8");	// 15
  strcpy(&((format++)->COLUMN_DEF[0]), "Text_cache_hits INT UNSIGNED"); // 16
  strcpy(&((format++)->COLUMN_DEF[0]), "Avg_text_size        	INT UNSIGNED"); //  17
  strcpy(&((format++)->COLUMN_DEF[0]), "Text_entries   	    	INT UNSIGNED"); //  18
  strcpy(&((format++)->COLUMN_DEF[0]), "Displaced_texts 		INT UNSIGNED"); //  19
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_lookups        	INT UNSIGNED"); //  20
  return SP_SUCCESS;
}

// Copies information on the query cache by the interface provided in
// QueryCache.h and processes it
SP_STATUS
  QueryCacheStatStoredProcedure::sp_Process(SP_PROCESS_ACTION action,
					    SP_ROW_DATA inputData,
					    SP_EXTRACT_FUNCPTR eFunc,
					    SP_ROW_DATA outputData,
					    SP_FORMAT_FUNCPTR fFunc,
					    SP_KEY_VALUE keys,
					    SP_KEYVALUE_FUNCPTR kFunc,
					    SP_PROCESS_HANDLE *spProcHandle,
					    SP_HANDLE spObj,
					    SP_ERROR_STRUCT *error)
{  
  if (action == SP_PROC_OPEN) {

    InfoStruct *is = new InfoStruct;
    
    if(!is->initQueryCacheStat(inputData, eFunc, error))
      return SP_FAIL;

    *spProcHandle = is;
    return SP_SUCCESS;
  }

  if (action == SP_PROC_FETCH) {
    InfoStruct* is = (InfoStruct *)(*spProcHandle);

    if (!is) {
      return SP_FAIL;
    }

    QueryCacheStats stats;
    if(!is->getNext(stats))
       return SP_SUCCESS;

    ULong kBytes;

    kBytes = stats.avgPlanSize;
    fFunc(0,  outputData, sizeof(kBytes), &(kBytes), 0);
    kBytes = stats.s.currentSize / 1024;
    fFunc(1,  outputData, sizeof(kBytes), &(kBytes), 0);
    kBytes = stats.maxSize / 1024;
    fFunc(2,  outputData, sizeof(kBytes), &(kBytes), 0);
    fFunc(3,  outputData, sizeof(stats.maxVictims),	&(stats.maxVictims),  0);
    fFunc(4,  outputData, sizeof(stats.nEntries),	&(stats.nEntries),    0);
    fFunc(5,  outputData, sizeof(stats.nPlans),		&(stats.nPlans),    0);
    fFunc(6,  outputData, sizeof(stats.nCompiles),	&(stats.nCompiles),   0);
    fFunc(7,  outputData, sizeof(stats.s.nRecompiles),	&(stats.s.nRecompiles), 0);
    fFunc(8,  outputData, sizeof(stats.nRetries),	&(stats.nRetries),    0);
    fFunc(9,  outputData, sizeof(stats.s.nCacheableP),	&(stats.s.nCacheableP), 0);
    fFunc(10, outputData, sizeof(stats.s.nCacheableB),	&(stats.s.nCacheableB), 0);
    fFunc(11, outputData, sizeof(stats.s.nCacheHitsP),	&(stats.s.nCacheHitsP), 0);
    fFunc(12, outputData, sizeof(stats.s.nCacheHitsB),	&(stats.s.nCacheHitsB), 0);
    fFunc(13, outputData, sizeof(stats.nTooLarge),	&(stats.nTooLarge),   0);
    fFunc(14, outputData, sizeof(stats.nDisplaced),	&(stats.nDisplaced),  0);
    char optimizationLevel[8];
    switch (stats.optimLvl) {
      case CompilerEnv::OPT_MINIMUM:
	strcpy(optimizationLevel, "0      ");	  break;
      case CompilerEnv::OPT_MEDIUM_LOW:
	strcpy(optimizationLevel, "2      ");	  break;
      case CompilerEnv::OPT_MEDIUM:
	strcpy(optimizationLevel, "3      ");	  break;
      case CompilerEnv::OPT_MAXIMUM:
	strcpy(optimizationLevel, "5      ");	  break;
	default: strcpy(optimizationLevel, "UNKNOWN"); break;
    }

    fFunc(15,outputData,(Lng32)strlen(optimizationLevel),optimizationLevel,1);
    fFunc(16,outputData,sizeof(stats.s.nCacheHitsPP),&(stats.s.nCacheHitsPP),0);
    fFunc(17,outputData,sizeof(stats.avgTEntSize),&(stats.avgTEntSize),0);
    fFunc(18,outputData,sizeof(stats.nTextEntries),&(stats.nTextEntries),0);
    fFunc(19,outputData,sizeof(stats.nDispTEnts),&(stats.nDispTEnts),0);
    fFunc(20,outputData,sizeof(stats.s.nLookups),&(stats.s.nLookups),0);

    return SP_MOREDATA;
  } // if (action == SP_PROC_FETCH)

  if (action == SP_PROC_CLOSE) {
    delete (InfoStruct *)(*spProcHandle);
    return SP_SUCCESS;
  }

  return SP_SUCCESS;
}

// Registers the QueryCache function
void QueryCacheStatStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc("QUERYCACHE",
           sp_Compile,
           sp_InputFormat,
	   0,
           sp_NumOutputFields,
           sp_OutputFormat,
	   sp_Process,
	   0,
	   CMPISPVERSION);
}

SP_STATUS QueryCacheEntriesStoredProcedure::sp_InputFormat(SP_FIELDDESC_STRUCT *inputFieldFormat,
				  Lng32 numFields,
				  SP_COMPILE_HANDLE spCompileObj,
				  SP_HANDLE spObj,
	    			  SP_ERROR_STRUCT *error)
{
      if ( numFields != 2 )
      {
        //accepts 1 input columns
        error->error = arkcmpErrorISPWrongInputNum;
        strcpy(error->optionalString[0], "QueryCacheEntries");
        error->optionalInteger[0] = 2;
        return SP_FAIL;
      }
  
     //column as input parameter for ISP, specifiy query cache of metadata or user context
     strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "instance    char(16) not null");
     strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "location    char(16) not null"); 
     return SP_SUCCESS;
}

SP_STATUS
  QueryCacheEntriesStoredProcedure::sp_NumOutputFields(Lng32 *numFields,
						       SP_COMPILE_HANDLE spCompileObj,
						       SP_HANDLE spObj,
						       SP_ERROR_STRUCT *error)
{
  *numFields = 20;
  return SP_SUCCESS;
}

// Specifies the columns of the QueryCacheEntries table and their types
SP_STATUS QueryCacheEntriesStoredProcedure::sp_OutputFormat(
						SP_FIELDDESC_STRUCT* format,
						SP_KEYDESC_STRUCT*  /*keyFields */,
						Lng32*  /*numKeyFields */,
						SP_COMPILE_HANDLE cmpHandle,
						SP_HANDLE /* spHandle */,
						SP_ERROR_STRUCT* /* error */  )
{
  strcpy(&((format++)->COLUMN_DEF[0]), "Row_id        		INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Plan_id     		LARGEINT");
  strcpy(&((format++)->COLUMN_DEF[0]), "Text  			VARCHAR(4096 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Entry_size  		INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Plan_length  		INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_hits        	INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Phase        		CHAR(10 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Optimization_level	CHAR(10 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Catalog_name 		CHAR(40 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Schema_name 		CHAR(40 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_params   		INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Param_types   		VARCHAR(1024 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Compilation_time	INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Average_hit_time	INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Shape    		VARCHAR(1024 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Isolation_level    	CHAR(20 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Isolation_level_For_Updates   	CHAR(20 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Access_mode     	CHAR(20 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Auto_commit     	CHAR(15 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Rollback_mode     	CHAR(15 BYTES) character set utf8");
  return SP_SUCCESS;
}

void
setIsolationLevelAsString(TransMode::IsolationLevel il,
                          char* buffer, Int32 bufLen)
{
   memset(buffer, ' ', bufLen);
   strcpy(buffer, getStrOfIsolationLevel(il, FALSE));
}

// Copies information on the query cache by the interface provided in
// QueryCache.h and processes it
SP_STATUS QueryCacheEntriesStoredProcedure::sp_Process(SP_PROCESS_ACTION action,
						       SP_ROW_DATA  inputData ,
						       SP_EXTRACT_FUNCPTR  eFunc ,
						       SP_ROW_DATA outputData ,
						       SP_FORMAT_FUNCPTR fFunc,
						       SP_KEY_VALUE,
						       SP_KEYVALUE_FUNCPTR,
						       SP_PROCESS_HANDLE* spProcHandle,
						       SP_HANDLE /* spHandle */,
						       SP_ERROR_STRUCT*  error )
{
  switch (action) {
  case SP_PROC_OPEN:
    {
      InfoStruct *is = new InfoStruct;
      
      if(!is->initQueryCacheEntries(inputData, eFunc, error))
        return SP_FAIL;
      
      *spProcHandle = is;
      return SP_SUCCESS;
    }
    break;

  case SP_PROC_FETCH:
    {
      InfoStruct * is = (InfoStruct *)(*spProcHandle);
      if (!is) {
        return SP_FAIL;
      }

      QueryCacheDetails details;
      if(!is->getNext(details))
         return SP_SUCCESS;

      fFunc(0,  outputData, sizeof(is->counter_),	&(is->counter_), 0);
      fFunc(1,  outputData, sizeof(details.planId),	&(details.planId), 0);
      fFunc(2,outputData,(Lng32)strlen(details.qryTxt),(void*)(details.qryTxt),1);
      fFunc(3,  outputData, sizeof(details.entrySize),	&(details.entrySize), 0);
      fFunc(4,  outputData, sizeof(details.planLength), &(details.planLength), 0);
      fFunc(5,  outputData, sizeof(details.nOfHits),	&(details.nOfHits), 0);

      char phase[11];

      switch (details.phase) {
      case CmpMain::PREPARSE:  strcpy(phase, "PREPARSING"); break;
      case CmpMain::PARSE:  strcpy(phase, "PARSING   "); break;
      case CmpMain::BIND:  strcpy(phase, "BINDING   "); break;
      default: strcpy(phase, "UNKNOWN   "); break;
      }
      fFunc(6,outputData,(Lng32)strlen(phase),phase,1);

      char optimizationLevel[8];

      switch (details.optLvl) {
      case CompilerEnv::OPT_MINIMUM:    strcpy(optimizationLevel, "0      ");	break;
      case CompilerEnv::OPT_MEDIUM_LOW: strcpy(optimizationLevel, "2      ");	break;
      case CompilerEnv::OPT_MEDIUM:     strcpy(optimizationLevel, "3      ");	break;
      case CompilerEnv::OPT_MAXIMUM:    strcpy(optimizationLevel, "5      ");	break;
      default: strcpy(optimizationLevel,"UNKNOWN");				break;
	  }
      fFunc(7,outputData,(Lng32)strlen(optimizationLevel),optimizationLevel,1);
      fFunc(8,outputData,(Lng32)strlen(details.catalog),(void*)(details.catalog),1);
      fFunc(9,outputData,(Lng32)strlen(details.schema),(void*)(details.schema),1);
      fFunc(10, outputData, sizeof(details.nParams), &(details.nParams), 0);
      fFunc(11,outputData,(Lng32)strlen(details.paramTypes),(void*)(details.paramTypes),1);

      ULong time = details.compTime / 1000;
      fFunc(12, outputData, sizeof(time), &(time), 0);
      time = details.avgHitTime / 1000;
      fFunc(13, outputData, sizeof(time), &(time), 0);
      fFunc(14,outputData,(Lng32)strlen(details.reqdShape),(void*)(details.reqdShape),1);

      char isolationLevel[18];

      setIsolationLevelAsString(details.isoLvl, isolationLevel, 18);
      fFunc(15,outputData,(Lng32)strlen(isolationLevel),isolationLevel,1);

      setIsolationLevelAsString(details.isoLvlForUpdates, isolationLevel, 18);
      fFunc(16,outputData,(Lng32)strlen(isolationLevel),isolationLevel,1);

      char accessMode[14];
      switch(details.accMode) {
      case TransMode::READ_ONLY_:
      case TransMode::READ_ONLY_SPECIFIED_BY_USER_:
        strcpy(accessMode, "READ ONLY    ");	break;
      case TransMode::READ_WRITE_:
        strcpy(accessMode, "READ/WRITE   ");	break;
      default:
        strcpy(accessMode, "NOT SPECIFIED");	break;
      }
      fFunc(17,outputData,(Lng32)strlen(accessMode),accessMode,1);

      char autoCommit[14];
      switch(details.autoCmt) {
      case TransMode::ON_:
        strcpy(autoCommit, "ON           ");	break;
      case TransMode::OFF_:
        strcpy(autoCommit, "OFF          ");	break;
      default:
        strcpy(autoCommit, "NOT SPECIFIED");	break;
      }
      fFunc(18,outputData,(Lng32)strlen(autoCommit),autoCommit,1);

      char rollBackMode[14];
      switch(details.rbackMode) {
      case TransMode::ROLLBACK_MODE_WAITED_:
        strcpy(rollBackMode, "WAITED       ");	break;
      case TransMode::ROLLBACK_MODE_NOWAITED_:
        strcpy(rollBackMode, "NO WAITED    ");	break;
      default:
        strcpy(rollBackMode, "NOT SPECIFIED");	break;
	  }
      fFunc(19,outputData,(Lng32)strlen(rollBackMode),rollBackMode,1);

      is->counter_++;

      return SP_MOREDATA;
    }
    break;

  case SP_PROC_CLOSE: {
    delete (InfoStruct *)(*spProcHandle);
    return SP_SUCCESS;
  }

  default: break;
  } // switch
  return SP_SUCCESS;
}

// Registers the QueryCache function
void QueryCacheEntriesStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc("QUERYCACHEENTRIES",
           sp_Compile,
           sp_InputFormat,
	   0,
           sp_NumOutputFields,
           sp_OutputFormat,
	   sp_Process,
	   0,
	   CMPISPVERSION);
}


//-----------------------------------------------------------------------
// QueryCacheDeleteStoredProcedure is a class that contains functions used
// to delete the contents of the  QueryCache virtual table. The delete 
// function is implemented as an internal stored procedure.
//-----------------------------------------------------------------------


SP_STATUS QueryCacheDeleteStoredProcedure::sp_Process(
  SP_PROCESS_ACTION action,
  SP_ROW_DATA inputData,
  SP_EXTRACT_FUNCPTR eFunc,
  SP_ROW_DATA outputData,
  SP_FORMAT_FUNCPTR fFunc,
  SP_KEY_VALUE keys,
  SP_KEYVALUE_FUNCPTR kFunc,
  SP_PROCESS_HANDLE *spProcHandle,
  SP_HANDLE spObj,
  SP_ERROR_STRUCT *error)
{
  struct InfoStruct
  {
    ULng32 counter;
  };

  SP_STATUS status = SP_SUCCESS;
  InfoStruct *is = NULL;

  switch (action)
  {
  case SP_PROC_OPEN:
    // No inputs to process
    is = new InfoStruct;
    is->counter = 0;
    *spProcHandle = is;
    break;

  case SP_PROC_FETCH:
    is = (InfoStruct*)(*spProcHandle);
    if (is == NULL )
    {
      status = SP_FAIL;
      break;
    }

    //clear out QueryCache
    CURRENTQCACHE->makeEmpty();
    break;

  case SP_PROC_CLOSE:
    delete (InfoStruct*) (*spProcHandle);
    break;
  }
  return status;
}

void QueryCacheDeleteStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc("QUERYCACHEDELETE",
          sp_Compile,
          sp_InputFormat,
          0,
          sp_NumOutputFields,
          sp_OutputFormat,
          sp_Process,
          0,
	  CMPISPVERSION);
}

//------------------------Hybrid Query Cache Stat--------------------------//
void HybridQueryCacheStatStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc("HYBRIDQUERYCACHE",
          sp_Compile,
          sp_InputFormat,
          0,
          sp_NumOutputFields,
          sp_OutputFormat,
          sp_Process,
          0,
	  CMPISPVERSION);
}

SP_STATUS HybridQueryCacheStatStoredProcedure::sp_InputFormat(SP_FIELDDESC_STRUCT *inputFieldFormat,
				  Lng32 numFields,
				  SP_COMPILE_HANDLE spCompileObj,
				  SP_HANDLE spObj,
	    			  SP_ERROR_STRUCT *error)
{
      if ( numFields != 2 )
      {
        //accepts 1 input columns
        error->error = arkcmpErrorISPWrongInputNum;
        strcpy(error->optionalString[0], "HybridQueryCache");
        error->optionalInteger[0] = 2;
        return SP_FAIL;
      }
  
     //column as input parameter for ISP, specifiy query cache of metadata or user context
     strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "instance char(16) not null");  
     strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "location char(16)  not null");  
     return SP_SUCCESS;
}

SP_STATUS
  HybridQueryCacheStatStoredProcedure::sp_NumOutputFields(Lng32 *numFields,
						       SP_COMPILE_HANDLE spCompileObj,
						       SP_HANDLE spObj,
						       SP_ERROR_STRUCT *error)
{
  *numFields = 4;
  return SP_SUCCESS;
}

SP_STATUS HybridQueryCacheStatStoredProcedure::sp_OutputFormat(
						SP_FIELDDESC_STRUCT* format,
						SP_KEYDESC_STRUCT*  /*keyFields */,
						Lng32*  /*numKeyFields */,
						SP_COMPILE_HANDLE cmpHandle,
						SP_HANDLE /* spHandle */,
						SP_ERROR_STRUCT* /* error */  )
{
  strcpy(&((format++)->COLUMN_DEF[0]), "num_hkeys                                    INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "num_skeys                                     INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "num_max_values_per_key            INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "num_hash_table_buckets              INT UNSIGNED");
  return SP_SUCCESS;
}

SP_STATUS HybridQueryCacheStatStoredProcedure::sp_Process(SP_PROCESS_ACTION action,
						       SP_ROW_DATA  inputData ,
						       SP_EXTRACT_FUNCPTR  eFunc ,
						       SP_ROW_DATA outputData ,
						       SP_FORMAT_FUNCPTR fFunc,
						       SP_KEY_VALUE keys,
						       SP_KEYVALUE_FUNCPTR kFunc,
						       SP_PROCESS_HANDLE* spProcHandle,
						       SP_HANDLE spObj/* spHandle */,
						       SP_ERROR_STRUCT*  error )		       
{
  switch (action) {
    case SP_PROC_OPEN:
    {
        InfoStruct *is = new InfoStruct;
        
        if(!is->initHybridQueryCacheStat(inputData, eFunc, error))
          return SP_FAIL;
        
        *spProcHandle = is;
        return SP_SUCCESS;
    }
    break;

    case SP_PROC_FETCH:
    {
      InfoStruct* is = (InfoStruct *)(*spProcHandle);
      if (!is) {
        return SP_FAIL;
      }

      HybridQueryCacheStats stats;
      if(!is->getNext(stats))
         return SP_SUCCESS;
         
      fFunc(0,  outputData, sizeof(stats.nHKeys),	&(stats.nHKeys), 0);
      fFunc(1,  outputData, sizeof(stats.nSKeys),	&(stats.nSKeys), 0);
      fFunc(2,  outputData, sizeof(stats.nMaxValuesPerKey),	&(stats.nMaxValuesPerKey), 0);
      fFunc(3,  outputData, sizeof(stats.nHashTableBuckets),	&(stats.nHashTableBuckets), 0);
      
      return SP_MOREDATA;
    }
    break;

    case SP_PROC_CLOSE: {
       delete (InfoStruct *)(*spProcHandle);
       return SP_SUCCESS;
    }
    break;
     
    default: break;
  } // switch
  return SP_SUCCESS;
}

//------------------------Hybrid Query Cache Entries--------------------------//
void HybridQueryCacheEntriesStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc("HYBRIDQUERYCACHEENTRIES",
          sp_Compile,
          sp_InputFormat,
          0,
          sp_NumOutputFields,
          sp_OutputFormat,
          sp_Process,
          0,
	  CMPISPVERSION);
}

SP_STATUS HybridQueryCacheEntriesStoredProcedure::sp_InputFormat(SP_FIELDDESC_STRUCT *inputFieldFormat,
				  Lng32 numFields,
				  SP_COMPILE_HANDLE spCompileObj,
				  SP_HANDLE spObj,
	    			  SP_ERROR_STRUCT *error)
{
      if ( numFields != 2 )
      {
        //accepts 1 input columns
        error->error = arkcmpErrorISPWrongInputNum;
        strcpy(error->optionalString[0], "HybridQueryCacheEntries");
        error->optionalInteger[0] = 2;
        return SP_FAIL;
      }
  
      // Describe input columns
     strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "instance char(16)  not null");
     strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "location char(16)   not null"); 
     return SP_SUCCESS;
}

SP_STATUS
  HybridQueryCacheEntriesStoredProcedure::sp_NumOutputFields(Lng32 *numFields,
						       SP_COMPILE_HANDLE spCompileObj,
						       SP_HANDLE spObj,
						       SP_ERROR_STRUCT *error)
{
  *numFields = 8;
  return SP_SUCCESS;
}

SP_STATUS HybridQueryCacheEntriesStoredProcedure::sp_OutputFormat(
						SP_FIELDDESC_STRUCT* format,
						SP_KEYDESC_STRUCT*  /*keyFields */,
						Lng32*  /*numKeyFields */,
						SP_COMPILE_HANDLE cmpHandle,
						SP_HANDLE /* spHandle */,
						SP_ERROR_STRUCT* /* error */  )
{
  strcpy(&((format++)->COLUMN_DEF[0]), "plan_id     		LARGEINT");
  strcpy(&((format++)->COLUMN_DEF[0]), "hkey  			VARCHAR(4096 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "skey        		VARCHAR(4096 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "num_hits  		INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "num_PLiterals  		INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "PLiterals	VARCHAR(4096 BYTES) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "num_NPLiterals        	INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "NPLiterals   VARCHAR(4096 BYTES) character set utf8");
  return SP_SUCCESS;
}


SP_STATUS HybridQueryCacheEntriesStoredProcedure::sp_Process(SP_PROCESS_ACTION action,
						       SP_ROW_DATA  inputData ,
						       SP_EXTRACT_FUNCPTR  eFunc ,
						       SP_ROW_DATA outputData ,
						       SP_FORMAT_FUNCPTR fFunc,
						       SP_KEY_VALUE keys,
						       SP_KEYVALUE_FUNCPTR kFunc,
						       SP_PROCESS_HANDLE* spProcHandle,
						       SP_HANDLE spObj/* spHandle */,
						       SP_ERROR_STRUCT*  error )		       
{
  switch (action) {
    case SP_PROC_OPEN:
    {
        InfoStruct *is = new InfoStruct;

        if(!is->initHybridQueryCacheEntries(inputData, eFunc, error))
          return SP_FAIL;
        
        *spProcHandle = is;
        return SP_SUCCESS;
    }
    break;

    case SP_PROC_FETCH:
    {
       InfoStruct* is = (InfoStruct *)(*spProcHandle);
       if (!is) 
          return SP_FAIL;
      
      //fill data field
      HybridQueryCacheDetails details;
      
      if(!is->getNext(details))
           return SP_SUCCESS;

      fFunc(0, outputData, sizeof(details.planId), &(details.planId), 0);
      
      Lng32 len = (Lng32)details.hkeyTxt.length() < 2048 ? (Lng32)details.hkeyTxt.length() : 2048;
      fFunc(1, outputData, len, (void*)(details.hkeyTxt.data()),1);

      len = (Lng32)details.skeyTxt.length() < 2048 ? (Lng32)details.skeyTxt.length() : 2048;
      fFunc(2, outputData,(Lng32)details.skeyTxt.length(), (void*)(details.skeyTxt.data()),1);

      fFunc(3, outputData, sizeof(details.nHits), &(details.nHits), 0);
      
      fFunc(4, outputData, sizeof(details.nOfPConst),	&(details.nOfPConst), 0);

      len = (Lng32)details.PConst.length() < 1024 ? (Lng32)details.PConst.length() : 1024;
      fFunc(5, outputData,(Lng32)details.PConst.length(), (void*)(details.PConst.data()),1);
      
      fFunc(6, outputData, sizeof(details.nOfNPConst),	&(details.nOfNPConst), 0);

      len = (Lng32)details.NPConst.length() < 1024 ? (Lng32)details.NPConst.length() : 1024;
      fFunc(7, outputData,(Lng32)details.NPConst.length(), (void*)(details.NPConst.data()),1);
      
      return SP_MOREDATA;
    }
    break;

    case SP_PROC_CLOSE: {
       delete (InfoStruct *)(*spProcHandle);
       return SP_SUCCESS;
    }
    break;
     
    default: break;
  } // switch
    return SP_SUCCESS;
}

