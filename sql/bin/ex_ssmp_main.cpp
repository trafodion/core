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
* File:         ex_ssmp_main.cpp
* Description:  This is the main program for SSMP. SQL stats merge process.
*
* Created:      05/08/2006
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
#if defined(NA_YOS)
extern "C"
{
_priv _resident void DS_PRIVSTATE_SAVE_ (unsigned short pin,
                                        unsigned short flag);
}
#endif
#endif
#include "ex_ex.h"
#include "Ipc.h"
#include "Globals.h"
#include "SqlStats.h"
#include "memorymonitor.h"
#include "ssmpipc.h"
#include "rts_msg.h"
#include "PortProcessCalls.h"
#if defined(NA_LINUX)
#include "seabed/ms.h"
#include "seabed/fs.h"
extern void my_mpi_fclose();
#include "SCMVersHelp.h"
DEFINE_DOVERS(mxssmp)
#endif

#ifdef NA_NSK
_callable 
#endif
void  runServer(Int32 argc, char **argv);

void processAccumulatedStatsReq(SsmpNewIncomingConnectionStream *ssmpMsgStream, SsmpGlobals *ssmpGlobals);

#if !defined(NA_NSK) || ! defined(NA_NO_C_RUNTIME)
Int32 main(Int32 argc, char **argv)
{
#ifdef NA_LINUX
  dovers(argc, argv);
  try {
    file_init(&argc, &argv);
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
      fprintf(stderr, "[Redirecting MXUDR stdout to %s]\n", stdOutFile);
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

  const Int32 maxSsmpArgs = 10;
  Int32 argc = 0;
  char *argv[maxSsmpArgs];
  Int32 pos = 0;

  // the pTAL main program just hands us a null-terminated parameter
  // string that is not divided into arguments (do as little as possible
  // in pTAL). The primitive method of splitting up the arguments should
  // work for now in the ESP, since no end-users can run an ESP.

  // add the executable first
  argv[argc++] = "mxssmp";

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
  jmp_buf ssmpJmpBuf;
  Int32 shmId;
  StatsGlobals *statsGlobals = (StatsGlobals *)shareStatsSegment(shmId);
  Int32 r = 0;
  while (statsGlobals == NULL && ++r < 10)
    { // try 9 more times if the shared segement is not available
      DELAY(100);  // delay for 1 sec.
      statsGlobals = (StatsGlobals *)shareStatsSegment(shmId);
    }
  if (statsGlobals == NULL)
  {
     char tmbuf[64];
     time_t now;
     struct tm *nowtm;
     now = time(NULL);
     nowtm = localtime(&now);
     strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S ", nowtm);

     cout << tmbuf << "SSCP didn't create/initialize the RMS shared segment"
             << ", SSMP exited\n";
     NAExit(0);
  }

  CliGlobals *cliGlobals = CliGlobals::createCliGlobals(FALSE);
  statsGlobals->setSsmpPid(cliGlobals->myPin());
  statsGlobals->setSsmpTimestamp(cliGlobals->myStartTime());
  short error = statsGlobals->openStatsSemaphore(
                                    statsGlobals->ssmpProcSemId_);
  ex_assert(error == 0, "Error in opening the semaphore");

  cliGlobals->setStatsGlobals(statsGlobals);
  cliGlobals->setSharedMemId(shmId);

  // Handle possibility that the previous instance of MXSSMP died 
  // while holding the stats semaphore.  This code has been covered in
  // a manual unit test, but it is not possible to cover this easily in
  // an automated test.
  // LCOV_EXCL_START
  if (statsGlobals->semPid_ != -1)
  {
    NAProcessHandle prevSsmpPhandle((SB_Phandle_Type *)
                            &statsGlobals->ssmpProcHandle_);
    prevSsmpPhandle.decompose();
    if (statsGlobals->semPid_ == prevSsmpPhandle.getPin())
    {
      NAProcessHandle myPhandle;
      myPhandle.getmine();
      myPhandle.decompose();
      short savedPriority, savedStopMode;
      StatsGlobals::CleanupStatus cstatus = 
           statsGlobals->releaseAndGetStatsSemaphore(
                     statsGlobals->ssmpProcSemId_,
                     (pid_t) myPhandle.getPin(), 
                     (pid_t) prevSsmpPhandle.getPin(),
                     savedPriority, savedStopMode,
                     FALSE /*shouldTimeout*/);
      if (cstatus != StatsGlobals::READY_TO_CLEANUP)
      {
        statsGlobals->deferredCleanupComplete(prevSsmpPhandle.getPin());
        ex_assert(0, "abandoned semaphore freed outside mxssmp");
      }
      statsGlobals->releaseStatsSemaphore(
                     statsGlobals->ssmpProcSemId_,
                     (pid_t) myPhandle.getPin(),
                     savedPriority, savedStopMode);
    }
    else 
    {
      // Handle possibility that the process which last got the semaphore
      // is no longer executing.  Normally, the cleanup would have
      // happened when MXSSMP processes a ZSYS_VAL_SMSG_PROCDEATH message,
      // but it could be that this was sent between instances of MXSSMP.
      NAProcessHandle myPhandle;
      myPhandle.getmine();
      myPhandle.decompose();
      char processName[50];
      Int32 loopCnt = 0;
      while (statsGlobals->semPid_ != -1)
      {
        pid_t tempPid = statsGlobals->semPid_;
        Int32 ln_error = msg_mon_get_process_name(myPhandle.getCpu(),
                                          statsGlobals->semPid_, processName);
        ex_assert(ln_error != XZFIL_ERR_INVALIDSTATE,
          "msg_mon_get_process_name shouldn't be called after "
          "msg_mon_process_shutdown");
        if (ln_error == XZFIL_ERR_NOSUCHDEV)
          {
            statsGlobals->seabedError_ = ln_error;
            statsGlobals->cleanup_SQL(tempPid, myPhandle.getPin());
          }
        else if (ln_error == XZFIL_ERR_OK)
          break;
        else 
          {
            // Let this process stop - maybe the next MXSSMP will
            // have better luck....
            DELAY(300);
            if (++loopCnt >= 3)
              ex_assert(ln_error == XZFIL_ERR_OK, 
                        "Too many errors from msg_mon_get_process_name")
          }
      }
    }
  }
  // LCOV_EXCL_STOP

#ifdef SQ_NEW_PHANDLE
  XPROCESSHANDLE_GETMINE_(&statsGlobals->ssmpProcHandle_);
#else
  XPROCESSHANDLE_GETMINE_(statsGlobals->ssmpProcHandle_);
#endif
#ifdef NA_NSK
  // Set number of calls to 1 so that IsExecutor() returns TRUE
  Lng32 numCliCalls = cliGlobals->incrNumOfCliCalls();
#endif
  NAHeap *ssmpHeap = cliGlobals->getExecutorMemory();
  cliGlobals->setJmpBufPtr(&ssmpJmpBuf);
  if (setjmp(ssmpJmpBuf))
    NAExit(1); // Abend
#else
_callable void runServer(Int32 argc, char **argv)
{
  jmp_buf ssmpJmpBuf;
  StatsGlobals *statsGlobals = NULL;
  
  // Check if SSMP is running and share the segment if it is already
  // running
#ifdef NA_NSK
  short error = -1;
  short errorDetail;
  short procHandle[10];
  short cpu;
  short pin;
  char  sqlControlProcName[50];
  short length;
  short options     = 34;   // use KMSF, let NSK assign base addr
  short noOfAttempts = 0;
  Int64 timeout = 1000; // 10 seconds
  short fsError = 0;

  char defineName[24+1];
  char className[16+1];
  char attrName[16+1];
  char attrValue[36];
  short defineValueMaxLen = 35, 
	defineValueLen = 0;

  // ProcessHandle wrapper in porting layer library
  NAProcessHandle phandle;

  // Get the pin of the SQLControLProc
  fsError = phandle.getmine((short *)&procHandle);
  fsError = phandle.decompose();
  // Extract cpu from phandle
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
  getServerProcessName(IPC_SQLSSCP_SERVER, NULL, 0, cpu, sqlControlProcName); 
  while (error != 0 && noOfAttempts < 20)
  {
    PROCESSSTRING_SCAN_(sqlControlProcName,
				(short)str_len(sqlControlProcName),
				&length,
				procHandle);

    // ProcessHandle wrapper in porting layer library
    NAProcessHandle serverPhandle((short *)&procHandle);
    error  = serverPhandle.decompose();
    if (error == 0)
    { // Extract pin from serverPhandle
      pin = serverPhandle.getPin();
      error = SEGMENT_ALLOCATE_(getStatsSegmentId(),
			    ,,,&errorDetail,pin,,(Lng32 *)&statsGlobals,,options);
      if (error != 0)
      {
         DELAY(timeout); 
      }
    }
    else
    {
      DELAY(timeout);
    }
    noOfAttempts++;
  }
  if (error != 0)
  {
#ifdef _DEBUG
    cerr << "SSCP has to be started first. Error code : "  << error << "\n";
#endif
    NAExit(0);
  }
  // Wait for SSCP to be initiaized
  noOfAttempts = 0;
  while ((!statsGlobals->IsSscpInitialized()) && noOfAttempts < 20)
  {
     DELAY(timeout);
     noOfAttempts++;
  }
  if (!statsGlobals->IsSscpInitialized())
  {
#ifdef _DEBUG
    cerr << "SSCP has not been initalized \n";
#endif
    NAExit(0);
  }
  CliGlobals *cliGlobals;
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
  cliGlobals = GetCliGlobals(FALSE);
   // Set number of calls to 1 so that IsExecutor() returns TRUE
  Lng32 numCliCalls = cliGlobals->incrNumOfCliCalls();
  NAHeap *ssmpHeap = cliGlobals->getExecutorMemory();
  cliGlobals->setJmpBufPtr(&ssmpJmpBuf);
  if (setjmp(ssmpJmpBuf))
    NAExit(1); // Abend
#else
  NAHeap *ssmpHeap = new NAHeap("SSCP Global Heap",
                               NAMemory::DERIVED_FROM_SYS_HEAP,
                               256 * 1024 // 256K block size
                               );
#endif
#endif
#ifndef NA_LINUX
  IpcEnvironment       *ssmpIpcEnv = new (ssmpHeap) IpcEnvironment(ssmpHeap,
            cliGlobals->getEventConsumed(), FALSE, IPC_SQLSSMP_SERVER);
#else
  IpcEnvironment       *ssmpIpcEnv = new (ssmpHeap) IpcEnvironment(ssmpHeap,
            cliGlobals->getEventConsumed(), FALSE, IPC_SQLSSMP_SERVER,
             FALSE, TRUE);
#endif
  
  SsmpGlobals *ssmpGlobals = new (ssmpHeap) SsmpGlobals(ssmpHeap, ssmpIpcEnv, 
                                                        statsGlobals);

  // Currently open $RECEIVE with 2048
  SsmpGuaReceiveControlConnection *cc =
	 new (ssmpHeap) SsmpGuaReceiveControlConnection(ssmpIpcEnv,
					  ssmpGlobals,
					  2048);
  
  ssmpIpcEnv->setControlConnection(cc);

  while (TRUE) 
  {

    // wait for the first open message to come in
    while (cc->getConnection() == NULL)
    {
      cc->wait(300);
      ssmpGlobals->getStatsGlobals()->cleanupDeferredSql();
    }
    // start the first receive operation
#ifdef _DEBUG_RTS
    cerr << "No. of Requesters-1 "  << cc->getNumRequestors() << " \n";
#endif
    while (cc->getNumRequestors() > 0)
    {
      ssmpGlobals->work();
    }
  } 
}

