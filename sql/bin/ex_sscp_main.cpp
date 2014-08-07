/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2014 Hewlett-Packard Development Company, L.P.
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
* File:         ex_sscp_main.cpp
* Description:  This is the main program for SSCP. SQL stats control process The process 
*				does the following:
*				- Creates the shared segment
*               . Handle messages from SSMP
*
* Created:      04/172006
* Language:     C++
*
*****************************************************************************
*/
#include "Platform.h"
#ifdef _DEBUG
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <errno.h>
#if !defined(NA_NSK) && !defined(NA_LINUX)
#include <windows.h>
#include <winsock2.h>
#include "cextdecs/cextdecs.h"
#else
#include "ExCextdecs.h"
extern "C"
{
#if defined(NA_YOS)
_priv _resident void DS_PRIVSTATE_SAVE_ (unsigned short pin,
                                        unsigned short  flag);
#endif
}
#endif
#include "ex_ex.h"
#include "Ipc.h"
#include "Globals.h"
#include "SqlStats.h"
#include "memorymonitor.h"
#include "sscpipc.h"
#include "rts_msg.h"
#include "ex_stdh.h"
#include "ExStats.h"
#include "PortProcessCalls.h"
#ifdef NA_LINUX
#include <sys/ipc.h>
#include <sys/shm.h>
#include "seabed/ms.h"
#include "seabed/fs.h"
extern void my_mpi_fclose();
#include "SCMVersHelp.h"
DEFINE_DOVERS(mxsscp)
#endif
#ifdef NA_NSK
_callable 
#endif 
void  runServer(Int32 argc, char **argv);


#if !defined(NA_NSK) || ! defined(NA_NO_C_RUNTIME)
Int32 main(Int32 argc, char **argv)
{
#ifdef NA_LINUX
  dovers(argc, argv);
  msg_debug_hook("mxsscp", "mxsscp.hook");
  try {
    file_init_attach(&argc, &argv, TRUE, (char *)"");
  }
  catch (SB_Fatal_Excep &e) {
    SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, e.what(), 0);
    exit(1);
  }

  try {
    file_mon_process_startup(true);
  }
  catch (SB_Fatal_Excep &e) {
    SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, e.what(), 0);
    exit(1);
  }

  atexit(my_mpi_fclose);
#endif

  // Synchronize C and C++ output streams
  ios::sync_with_stdio();

#ifdef _DEBUG
  // Redirect stdout and stderr to files named in environment
  // variables
  const char *stdOutFile = getenv("SQLMX_SSCP_STDOUT");
  const char *stdErrFile = getenv("SQLMX_SSCP_STDERR");
  Int32 fdOut = -1;
  Int32 fdErr = -1;

  if (stdOutFile && stdOutFile[0])
  {
    fdOut = open(stdOutFile,
                 O_WRONLY | O_APPEND | O_CREAT | O_SYNC,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fdOut >= 0)
    {
      fprintf(stderr, "[Redirecting MXSSCP stdout to %s]\n", stdOutFile);
      fflush(stderr);
      dup2(fdOut, fileno(stdout));
    }
    else
    {
      fprintf(stderr, "*** WARNING: could not open %s for redirection: %s.\n",
              stdOutFile, strerror(errno));
    }
  }

  if (stdErrFile && stdErrFile[0])
  {
    fdErr = open(stdErrFile,
                 O_WRONLY | O_APPEND | O_CREAT | O_SYNC,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fdErr >= 0)
    {
      fprintf(stderr, "[Redirecting MXUDR stderr to %s]\n", stdErrFile);
      fflush(stderr);
      dup2(fdErr, fileno(stderr));
    }
    else
    {
      fprintf(stderr, "*** WARNING: could not open %s for redirection: %s.\n",
              stdErrFile, strerror(errno));
    }
  }
  runServer(argc, argv);

  if (fdOut >= 0)
  {
    close(fdOut);
  }
  if (fdErr >= 0)
  {
    close(fdErr);
  }
#else
  runServer(argc, argv);
#endif
  return 0;
}
#else
extern "C" void RUN_ESP_CPP(char *args)
{

  const Int32 maxSscpArgs = 10;
  Int32 argc = 0;
  char *argv[maxSscpArgs];
  Int32 pos = 0;

  // the pTAL main program just hands us a null-terminated parameter
  // string that is not divided into arguments (do as little as possible
  // in pTAL). The primitive method of splitting up the arguments should
  // work for now in the ESP, since no end-users can run an ESP.

  // add the executable first
  argv[argc++] = "mxsscp";

  while (args[pos])
    {
      // skip whitespace and zero it out
      while (args[pos] == ' ')
	{
	  args[pos] = 0;
	  pos++;
	}

      // if this is not a NULL terminator, add it as a new argument
      if (args[pos])
	argv[argc++] = &args[pos];

      // skip non-whitespace
      while (args[pos] != ' ' && args[pos] != 0)
	pos++;
      // add a NULL terminator if it is not already there
      args[pos] = 0;
    }

  // last entry in argv is a NULL
  argv[argc] = 0;
  runServer(argc, argv);
  return;
}
#endif

#ifdef NA_LINUX
void runServer(Int32 argc, char **argv)
{
  Int32 shmid;
  jmp_buf sscpJmpBuf;
  StatsGlobals *statsGlobals = NULL;
  void *statsGlobalsAddr;
  NABoolean createStatsGlobals = FALSE;
  CliGlobals *cliGlobals = CliGlobals::createCliGlobals(FALSE);
  char tmbuf[64];
  time_t now;
  struct tm *nowtm;

  long maxSegSize = STATS_MAX_SEG_SIZE;
  char *envSegSize = getenv("MX_RTS_STATS_SEG_SIZE");
  if (envSegSize)
  {
    maxSegSize = (long) str_atoi(envSegSize, str_len(envSegSize));
    if (maxSegSize < 32)
      maxSegSize = 32;
    else if (maxSegSize > 256)
      maxSegSize = 256;
    maxSegSize *= 1024 * 1024;
  }
  long enableHugePages = 0;
  int shmFlag = RMS_SHMFLAGS;
  char *envShmHugePages = getenv("SQ_RMS_ENABLE_HUGEPAGES");
  if (envShmHugePages != NULL)
  {
     enableHugePages = (long) str_atoi(envShmHugePages, 
                         str_len(envShmHugePages));
     if (enableHugePages > 0)
       shmFlag =  shmFlag | SHM_HUGETLB;
  }

  now = time(NULL);
  nowtm = localtime(&now);
  strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S ", nowtm);

  if ((shmid = shmget((key_t)getStatsSegmentId(),
                         0,  // size doesn't matter unless we are creating.
                         shmFlag)) == -1)
  {
    if (errno == ENOENT)
    {
      if ((shmid = shmget((key_t)getStatsSegmentId(),
                                maxSegSize,
                                shmFlag | IPC_CREAT)) == -1)
      {
         cout << tmbuf << " Shmget failed, key=" << getStatsSegmentId() <<", Error code : "  << errno << "(" << strerror(errno) << ")\n";
         exit(errno);
      }
      else
      {
         if (enableHugePages > 0)
             cout << tmbuf << " RMS Shared segment id=" << shmid << ", key=" << (key_t)getStatsSegmentId() << ", created with huge pages support\n";
         else
             cout << tmbuf << " RMS Shared segment created id=" << shmid << ", key=" << (key_t)getStatsSegmentId() << "\n";

      }
      createStatsGlobals = TRUE;
    }
    else
    {
      cout << tmbuf << " Shmget failed key=" << (key_t)getStatsSegmentId()<< ", Error code : "  << errno << "(" << strerror(errno) << ")\n";
      exit(errno);
    } 
  }
  else
  {
     cout << tmbuf << " RMS Shared segment exists, attaching to it, shmid="<< shmid << ", key=" << (key_t)getStatsSegmentId() << "\n";
  }
  if ((statsGlobalsAddr = shmat(shmid, getRmsSharedMemoryAddr(), 0))
		== (void *)-1)
  {
    cout << tmbuf << "Shmat failed, shmid=" <<shmid << ", key=" << (key_t) getStatsSegmentId() << ", Error code : "  << errno << "(" << strerror(errno) << ")\n";
    exit(errno);
  }
  char *statsGlobalsStartAddr = (char *)statsGlobalsAddr;
  if (createStatsGlobals)
  {
     short envType = StatsGlobals::RTS_GLOBAL_ENV;
     statsGlobals = new (statsGlobalsStartAddr) 
             StatsGlobals((void *)statsGlobalsAddr, envType, maxSegSize);
     cliGlobals->setSharedMemId(shmid);
     // We really should not squirrel the statsGlobals pointer away like
     // this until the StatsGloblas is initialized, but 
     // statsGlobals->init() needs it ......
     cliGlobals->setStatsGlobals(statsGlobals);
     statsGlobals->init();
  }
  else
  {
    statsGlobals = (StatsGlobals *)statsGlobalsAddr;
    cliGlobals->setSharedMemId(shmid);
    cliGlobals->setStatsGlobals(statsGlobals);
  }
#ifdef SQ_NEW_PHANDLE
  XPROCESSHANDLE_GETMINE_(&statsGlobals->sscpProcHandle_);
#else
  XPROCESSHANDLE_GETMINE_(statsGlobals->sscpProcHandle_);
#endif
#ifdef NA_NSK
   // Set number of calls to 1 so that IsExecutor() returns TRUE
  Lng32 numCliCalls = cliGlobals->incrNumOfCliCalls();
#endif
  NAHeap *sscpHeap = cliGlobals->getExecutorMemory();
  cliGlobals->setJmpBufPtr(&sscpJmpBuf);
  if (setjmp(sscpJmpBuf))
    NAExit(1); // Abend 
#else
_callable 
void runServer(Int32 argc, char **argv)
{
  jmp_buf sscpJmpBuf;
  StatsGlobals *statsGlobals = NULL;
  
  // Check if SSMP is running and share the segment if it is already
  // running
#ifdef NA_NSK
  short error;
  short errorDetail;
  short procHandle[10];
  short cpu;
  short pin;
  char  sqlControlProcName[50];
  short length;
  short options     = 34;   // use KMSF, let NSK assign base addr
  char defineName[24+1];
  char className[16+1];
  char attrName[16+1];
  char attrValue[36];
  short defineValueMaxLen = 35, 
	defineValueLen = 0;

  // ProcessHandle wrapper in porting layer library 
  NAProcessHandle phandle;
 
  // Get the pin of the SQLControLProc
  error = phandle.getmine((short *)&procHandle);
  error = phandle.decompose();
  cpu = phandle.getCpu();
  pin = phandle.getPin();

 #if defined(NA_YOS)
  short privStateNoSave = DEFINEINFO("=_SQLMX_PRIVSTATE_NOSAVE",
			    className, 
			    attrName,
			    attrValue, 
			    defineValueMaxLen,
			    &defineValueLen);
  if (privStateNoSave != 0)
  {
    DS_PRIVSTATE_SAVE_(pin, 1);
  }
#endif
  short envType;
  getServerProcessName(IPC_SQLSSMP_SERVER, NULL, 0, cpu, sqlControlProcName, &envType); 
  PROCESSSTRING_SCAN_(sqlControlProcName,
				(short)str_len(sqlControlProcName),
				&length,
				procHandle);

  // ProcessHandle wrapper in porting layer library
  NAProcessHandle serverPhandle((short *)&procHandle);
  error = serverPhandle.decompose();
  if (error == 0)
  { // extract pin from serverPhandle
    pin = serverPhandle.getPin();
    error = SEGMENT_ALLOCATE_(getStatsSegmentId(),
			  ,,,&errorDetail,pin,,(Lng32 *)&statsGlobals,,options);
    // You would get error in SEGMENT_ALLOCATE here, because SSMP got started first
    // after a CPU failure or system startup time.
    // So Try allocate the shared segment by SSCP. If it fails then abend
  }
  if (error != 0)
  {

    short segSize;
    Lng32 maxSegSize = STATS_MAX_SEG_SIZE;
    short i;

    str_cpy_all (defineName, "=_MX_RTS_STATS_SEG_SIZE ", 24);

    error = DEFINEINFO(defineName,
                       className, attrName, attrValue, 
		       defineValueMaxLen, &defineValueLen);
    
    if (!error)
    {
      if (defineValueLen > 0)
      {
        attrValue[defineValueLen] = 0;
        //the value is of the form $A.Bnnnnn.
        //look only for the trailing numbers.
        for (i=defineValueLen;attrValue[i]<'A';i--);
        NUMIN (&attrValue[i+1],  // Character input
               &segSize, // Numeric output 
               10,             // Interpret as base 10 number
               &error);
      
        if (!error)
        {
	  if (segSize < 32)
            segSize = 32;
          else
          if (segSize > 256)
            segSize = 256;
          maxSegSize = segSize * 1024 *1024;
        }
      }
    }

    ULng32 baseAddr;
    error = SEGMENT_ALLOCATE_(getStatsSegmentId()
            ,STATS_INITAL_SEG_SIZE,,,&errorDetail,,,(Lng32 *)&baseAddr,maxSegSize, 34);
    if (error != 0)
    {
#ifdef _DEBUG
      cerr << "Stats SEGMENT ALLOCATE failed, Error code : "  << error << "\n";
#endif
      NAExit(1);      
    }
    char *statsGlobalsStartAddr = (char *)baseAddr;
    statsGlobals = new (statsGlobalsStartAddr) StatsGlobals((void *)baseAddr, 
            envType, maxSegSize);
    error = BINSEM_CREATE_(&statsGlobals->sscpProcSemId_, 0);
    if (error != 0)
    {
#ifdef _DEBUG
      cerr << "BINSEM_CREATE failed, Error code : "  << error << "\n";
#endif
      NAExit(1);      
    }
    // ProcessHandle wrapper in porting layer library
    NAProcessHandle sscpPhandle;

    error = sscpPhandle.getmine((short *)&(statsGlobals->sscpProcHandle_));
    error = BINSEM_UNLOCK_(statsGlobals->sscpProcSemId_);
  }

  Lng32 addr = 0;
  error = SEGMENT_ALLOCATE_(
       NA_CLI_FIRST_PRIV_SEG_ID,
       NA_CLI_FIRST_PRIV_SEG_SIZE,
       (Lng32) 0,     // swapfilename
       (short) 0,    // swapfilename length
       &errorDetail,
       (short) -1,   // PIN
       (short) 0,    // segment is not extensible (don't want to trap)
       &addr,
       NA_CLI_FIRST_PRIV_SEG_MAX_SIZE,
       options);
  if (error != 0)
  {
#ifdef _DEBUG
      cerr << "Executor SEGMENT_ALLOCATE failed. Error code : "  << error << "\n";
#endif
      NAExit(1);
  }
  CliGlobals *cliGlobals = GetCliGlobals(FALSE);
   // Set number of calls to 1 so that IsExecutor() returns TRUE
  Lng32 numCliCalls = cliGlobals->incrNumOfCliCalls();
  NAHeap *sscpHeap = cliGlobals->getExecutorMemory();
  cliGlobals->setJmpBufPtr(&sscpJmpBuf);
  if (setjmp(sscpJmpBuf))
    NAExit(1); // Abend
#else
  NAHeap *sscpHeap = new NAHeap("SSCP Global Heap",
                               NAMemory::DERIVED_FROM_SYS_HEAP,
                               256 * 1024 // 256K block size
                               );
#endif
#endif  // NA_LINUX else 
#ifndef NA_LINUX
  IpcEnvironment  *sscpIpcEnv = new (sscpHeap) IpcEnvironment(sscpHeap, cliGlobals->getEventConsumed(),
      FALSE, IPC_SQLSSCP_SERVER);
#else
  IpcEnvironment  *sscpIpcEnv = new (sscpHeap) IpcEnvironment(sscpHeap, cliGlobals->getEventConsumed(),
      FALSE, IPC_SQLSSCP_SERVER, FALSE, TRUE);
#endif

  SscpGlobals *sscpGlobals = NULL;

  sscpGlobals = new (sscpHeap) SscpGlobals(sscpHeap, statsGlobals);

  // Currently open $RECEIVE with 256
  SscpGuaReceiveControlConnection *cc =
	 new(sscpHeap) SscpGuaReceiveControlConnection(sscpIpcEnv,
					  sscpGlobals,
                                           256);
  sscpIpcEnv->setControlConnection(cc);
  while (TRUE) 
  {
     while (cc->getConnection() == NULL)
      cc->wait(IpcInfiniteTimeout);
    
#ifdef _DEBUG_RTS
    cerr << "No. of Requesters-1 "  << cc->getNumRequestors() << " \n";
#endif
    while (cc->getNumRequestors() > 0)
    {
      sscpIpcEnv->getAllConnections()->waitOnAll(IpcInfiniteTimeout);
    } // Inner while
  } 
}  // runServer


  

