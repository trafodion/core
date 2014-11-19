/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef PCODEEXPRCACHE_H
#define PCODEEXPRCACHE_H

#include "BaseTypes.h"
#include "NABasicObject.h"
#include "CmpCommon.h"
//#include "ComDefs.h"                // to get ROUND8
//#include "QCache.h"
#include "Collections.h"
#include "NAString.h"
#include "ItemColRef.h"
#include "BindWA.h"
//#include "DefaultConstants.h"
//#include "CmpMessage.h"
//#include "ComSysUtils.h"
//#include "Generator.h"
//#include "CmpMain.h"
#include "ExpPCodeOptimizations.h"
#include "ExpPCodeInstruction.h"

typedef NAHeap NABoundedHeap;
class NAHeap ;

#define OPT_PCC_DEBUG 1 /* Set to 1 if PCEC Logging needed, 0 otherwise */

class PCECacheEntry {        // Doubly-linked PCode Expr Cache Entry
public:
  UInt64 incrPCEHits()                  { return ++hits_     ; }
  UInt64 getPCEHits()             const { return hits_       ; }
  UInt32 getOptPClen()            const { return pcLenO_     ; }
  UInt32 getUnOptPClen()          const { return pcLenU_     ; }
  UInt32 getUnOptConstsLen()      const { return constsLenU_ ; }
  UInt32 getOptConstsLen()        const { return constsLenO_ ; }
  UInt32 getNEConstsLen()         const { return constsLenN_ ; }
  UInt32 getTempsAreaLen()        const { return tempsLenO_  ; }

#if defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1
  Int64  getNEgenTime()             const { return NEgenTime_  ; }
  Int64  getOptTime()               const { return optTime_    ; }
  void   addToOptTime( Int64 totAddTime ) { optTime_ += totAddTime ; }
#endif /* defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1 */

  char          * getConstsArea() const { return constsPtr_  ; }
  PCodeBinary   * getOptPCptr()   const { return pCodeO_ ; }
  PCodeBinary   * getUnOptPCptr() const { return pCodeU_ ; }
  PCECacheEntry * getPCENextInCrOrder()    const { return pnextInCreateOrder_  ; }
  PCECacheEntry * getPCEPrevInCrOrder()    const { return pprevInCreateOrder_  ; }
  PCECacheEntry * getPCENextInMRUOrder()    const { return pnextMRU_  ; }
  PCECacheEntry * getPCEPrevInMRUOrder()    const { return pprevMRU_  ; }
#if defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1
  UInt64          getUniqCtr()    const { return uniqueCtr_  ; }
#endif /* defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1 */

  void setPCEPrevInCrOrder(PCECacheEntry* prevp) { pprevInCreateOrder_ = prevp ; }
  void setPCENextInCrOrder(PCECacheEntry* nextp) { pnextInCreateOrder_ = nextp ; }
  void setPCEPrevInMRUOrder(PCECacheEntry* prevp) { pprevMRU_ = prevp ; }
  void setPCENextInMRUOrder(PCECacheEntry* nextp) { pnextMRU_ = nextp ; }

  // constructor
  PCECacheEntry( PCodeBinary   * unOptPCptr
               , PCodeBinary   * optPCptr
               , char          * constsPtr
               , UInt32          unOptPClen
               , UInt32          optPClen
               , UInt32          constsLenU
               , UInt32          constsLenO
               , UInt32          constsLenN
               , UInt32          tempsLenO
#if defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1
               , Int64           optTime
               , Int64           NEgenTime
               , UInt64          uniqueCtr
#endif /* defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1 */
               )
  : pnextInCreateOrder_  (NULL) ,
    pprevInCreateOrder_  (NULL) ,
    pnextMRU_            (NULL) ,
    pprevMRU_            (NULL) ,
    pCodeU_ (unOptPCptr)   ,
    pCodeO_ (optPCptr)     ,
    constsPtr_ (constsPtr) ,
    pcLenU_ (unOptPClen)   ,
    pcLenO_ (optPClen)     ,
    constsLenU_ (constsLenU) ,
    constsLenO_ (constsLenO) ,
    constsLenN_ (constsLenN) ,
    tempsLenO_  (tempsLenO)  ,
#if defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1
    optTime_   (optTime)   ,
    NEgenTime_ (NEgenTime) ,
    uniqueCtr_ (uniqueCtr) ,
#endif /* defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1 */
    hits_   ( 0 )
    {};

   
  // destructor
  ~PCECacheEntry()
   {};

private:
  PCECacheEntry * pnextInCreateOrder_      ;
  PCECacheEntry * pprevInCreateOrder_      ;
  PCECacheEntry * pnextMRU_   ;
  PCECacheEntry * pprevMRU_   ;
  PCodeBinary   * pCodeU_     ;  // Ptr    to unoptimized PCode byte stream
  PCodeBinary   * pCodeO_     ;  // Ptr    to optimized   PCode byte stream
  char          * constsPtr_  ;  // Ptr to cached constants area
  UInt64          hits_       ;  // number of hits for this cache entry
  UInt32          pcLenU_     ;  // Length of unoptimized PCode byte stream
  UInt32          pcLenO_     ;  // Length of optimized   PCode byte stream
  UInt32          constsLenU_ ;  // Length of ConstantsArea for unoptimized PC
  UInt32          constsLenO_ ;  // Length of ConstantsArea for   optimized PC
  UInt32          constsLenN_ ;  // Length of ConstantsArea with Native Expr.
  UInt32          tempsLenO_  ;  // Length of TempsArea needed
#if defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1
  UInt64          uniqueCtr_  ;
  Int64           optTime_    ; // Will incl time to Add to the cache
  Int64           NEgenTime_  ;
#endif /* defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1 */
};

class OptPCodeCache : public NABasicObject { // Anchor for PCode Expr Cache
 public:
  void printPCodeExprCacheStats();

  void genUniqFileNamePart();

  UInt64 getUniqFileNameTime() { return fileNameTime_ ; } ;
  UInt32 getUniqFileNamePid()  { return fileNamePid_ ;  } ;

  void      setPCECLoggingEnabled( Lng32 enabVal ) { PCECLoggingEnabled_ = enabVal ; } ;
  NABoolean getPCECLoggingEnabled() { return (PCECLoggingEnabled_ > 0) ; } ;

  void addPCodeExpr( PCodeBinary * uncachedPCodePtr
                   , PCodeBinary * cachedPCodePtr
                   , char        * oldConstsArea
                   , UInt32        uncachedPCodeLen
                   , UInt32        cachedPCodeLen
                   , UInt32        unOptConstsAreaLen
                   , UInt32        optConstsAreaLen
                   , UInt32        NEConstsAreaLen
                   , UInt32        tempsAreaLen
#if defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1
                   , Int64         optTime
                   , Int64         NEgenTime
                   , timeval       begAddTime
                   , char        * sqlStmt
#endif /* defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1 */
                   ) ;

  PCodeBinary * findPCodeExprInCache( PCodeBinary * unOptPCodePtr
                                    , char    * unOptConstantsArea
                                    , UInt32    optFlags
                                    , UInt32    unOptPCodeLen
                                    , UInt32    unOptConstsLen
                                    , UInt32  * optPCodeLen
                                    , UInt32  * optConstsLen
                                    , UInt32  * NEConstsLen
                                    , UInt32  * tempsAreaLen
                                    , char   ** optConstantsArea
                                    , char    * sqlStmt
                                    ) ;

  UInt64 getNumLookups() const { return numLookups_ ; }
  UInt64 getNumHits()    const { return numHits_    ; }
  UInt64 getMaxHits()    const { return maxHits_    ; }
  UInt64 getNumNEHits()  const { return numNEHits_  ; }
  UInt64 getMaxHitsDel() const { return maxHitsDel_ ; }
  UInt32 getNumEntries() const { return numEntries_ ; }
  UInt32 getCurrSize()   const { return currSize_   ; }
  UInt32 getMaxSize()    const { return maxSize_    ; }

  void  clearStats() ;
  void  resizeCache( Lng32 newsiz )  ;
  void  throwOutExcessCacheEntries() ;

#if defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1
  const PCECacheEntry* getMRUHead() const { return MRUHead_ ; }  // Strictly for debug
  const OptPCodeCache * getThisPtr() const { return this  ; }  // Strictly for debug
  void   setPCDlogDirPath( NAString * logDirPth) ;
  char * getPCDlogDirPath() { return logDirPath_ ; } ;
  void   logPCCEvent( Int32 eventType, PCECacheEntry * PCEptr, char * sqlStmt );
#endif /* defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1 */

  //constructor
  OptPCodeCache( ULng32 maxSize )
    : heap_( new CTXTHEAP NABoundedHeap
            ("optPCode cache heap", (NAHeap *)CTXTHEAP, 0, 0) )
    , createOrderHead_  (NULL)
    , createOrderTail_  (NULL)
    , MRUHead_          (NULL)
    , MRUTail_          (NULL)
    , lastMatchedEntry_ (NULL)
    , numLookups_ ( 0 )
    , numSrchd_   ( 0 )
    , totSrchd_   ( 0 )
    , numHits_    ( 0 )
    , numNEHits_  ( 0 )
    , maxHits_    ( 0 )
    , maxHitsDel_ ( 0 )
    , totByCfC_   ( 0 )
    , numEntries_ ( 0 )
    , currSize_   ( 0 )
    , maxSize_( maxSize )
    , maxOptPCodeSize_( 0 )
    , PCECLoggingEnabled_( 0 )
    , PCECHeaderWritten_ ( 0 )
#if defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1
    , totalSavedTime_ ( 0 )
    , totalSearchTime_( 0 )
    , totalOptTime_   ( 0 )
    , totalNEgenTime_ ( 0 )
    , uniqueCtr_      ( 0 )
    , fileNameTime_   ( -1 )
    , fileNamePid_   ( -1 )
    , logDirPath_   ( NULL )
    { genUniqFileNamePart(); };
#else
    { };
#endif /* defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1 */

  // destructor
  ~OptPCodeCache()
   {};

#if defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1
void addToTotalSavedTime( Int64 totSavedTime ) { totalSavedTime_ += totSavedTime ; }
void addToTotalSearchTime( Int64 totSearchTime ) { totalSearchTime_ += totSearchTime ; }
#endif /* defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1 */

 private:
  NAHeap        * heap_ ;    // heap to use for memory allocations
  PCECacheEntry * createOrderHead_  ;
  PCECacheEntry * createOrderTail_  ;
  PCECacheEntry * MRUHead_          ;
  PCECacheEntry * MRUTail_          ;
  PCECacheEntry * lastMatchedEntry_ ;
  UInt64          numLookups_ ; // Number of searches done
  UInt64          totSrchd_   ; // Number of Entries examined in all searches
  UInt64          numSrchd_   ; // Number of Entries examined in successful searches
  UInt64          numHits_    ; // Number of cache hits (total)
  UInt64          numNEHits_  ; // Number of cache hits where we use Native Expr.
  UInt64          maxHits_    ; // Max hits of any cache entry
  UInt64          maxHitsDel_ ; // Max hits of any kicked out entry
  UInt64          totByCfC_   ; // Total bytes copied from Cache
  UInt32          numEntries_ ; // Number of cache entries
  UInt32          currSize_   ; // Current total size of cached byte streams
  UInt32          maxSize_    ; // Maximum total size allowed (see CQD)
  UInt32     maxOptPCodeSize_ ; // Maximum optimized PCode byte stream length
  Int32   PCECLoggingEnabled_ ;
  Int32   PCECHeaderWritten_  ;
#if defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1
  Int64       totalSavedTime_ ;
  Int64       totalSearchTime_;
  Int64       totalOptTime_   ;
  Int64       totalNEgenTime_ ;
  UInt64      uniqueCtr_      ;
  UInt64      fileNameTime_   ;
  UInt32      fileNamePid_    ;
  char      * logDirPath_     ;
#endif /* defined(_DEBUG) && !defined(NA_NO_C_RUNTIME) && OPT_PCC_DEBUG==1 */
};

#endif /* PCODEEXPRCACHE_H */
