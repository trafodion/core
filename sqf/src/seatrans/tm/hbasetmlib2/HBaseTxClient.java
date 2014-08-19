// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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

package org.trafodion.dtm;

import java.util.Collections;
import java.util.Set;
import java.util.HashSet;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;

import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.CommitUnsuccessfulException;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.TransactionRegionLocation;
import org.apache.hadoop.hbase.client.transactional.TransState;
import org.apache.hadoop.hbase.client.transactional.TransReturnCode;
import org.apache.hadoop.hbase.client.transactional.TransactionMap;
import org.apache.hadoop.hbase.ipc.TransactionalRegionInterface;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.util.Bytes;
import org.trafodion.dtm.HBaseTmZK;
import org.trafodion.dtm.TmAuditTlog;

import java.util.Map;
import java.util.HashMap;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;

public class HBaseTxClient {

   static final Log LOG = LogFactory.getLog(HBaseTxClient.class);
   private static TmAuditTlog tLog;
   private static HBaseTmZK tmZK;
   private static RecoveryThread recovThread;
   private short dtmID;
   private static int stallWhere;

   boolean useTlog;
   boolean useForgotten;
   boolean forceForgotten;
   boolean useRecovThread;

   Configuration config;
   TransactionManager trxManager;
   //ConcurrentHashMap<Long, TransactionState> mapTransactionStates = TransactionMap.get();
   //ConcurrentHashMap<Long, TransactionState> mapTransactionStates = new ConcurrentHashMap<Long, TransactionState>();
   ConcurrentHashMap<Long, TransactionState> mapTransactionStates;
      
   void setupLog4j() {
     //System.out.println("In setupLog4J");
     String confFile = System.getenv("MY_SQROOT")
          + "/logs/log4j.dtm.config";
     PropertyConfigurator.configure(confFile);
   }
   
   public HBaseTxClient() {
     mapTransactionStates = TransactionMap.get();
   }
   
   public boolean init(String hBasePath, String zkServers, String zkPort) throws Exception {
      //System.out.println("In init - hbp");
      setupLog4j();
      LOG.debug("Enter init, hBasePath:" + hBasePath);
      config = HBaseConfiguration.create();
      //System.out.println("In init - HBaseConfiguration.create completed.");

      config.set("hbase.zookeeper.quorum", zkServers);
      config.set("hbase.zookeeper.property.clientPort",zkPort);
      config.set("hbase.rootdir", hBasePath);
      config.set("dtmid", "0");
      this.dtmID = 0;
      this.useRecovThread = false;
      this.stallWhere = 0;

      useForgotten = true;
      try {
         String useAuditRecords = System.getenv("TM_ENABLE_FORGOTTEN_RECORDS");
         if (useAuditRecords != null) {
            useForgotten = (Integer.parseInt(useAuditRecords) != 0);
         }
      }
      catch (Exception e) {
         LOG.debug("TM_ENABLE_FORGOTTEN_RECORDS is not in ms.env");
      }
      LOG.info("useForgotten is " + useForgotten);

      forceForgotten = false;
      try {
         String forgottenForce = System.getenv("TM_TLOG_FORCE_FORGOTTEN");
         if (forgottenForce != null){
            forceForgotten = (Integer.parseInt(forgottenForce) != 0);
            LOG.debug("forgottenForce != null");
         }
      }
      catch (Exception e) {
         LOG.debug("TM_TLOG_FORCE_FORGOTTEN is not in ms.env");
      }
      LOG.info("forceForgotten is " + forceForgotten);

      useTlog = false;
      useRecovThread = false;
      try {
         String useAudit = System.getenv("TM_ENABLE_TLOG_WRITES");
         if (useAudit != null) {
            useTlog = useRecovThread = (Integer.parseInt(useAudit) != 0);
         }
      }
      catch (Exception e) {
         LOG.debug("TM_ENABLE_TLOG_WRITES is not in ms.env");
      }
      //ystem.out.println("In init - useTlog");

      if (useTlog) {
         try {
            tLog = new TmAuditTlog(config);
         } catch (Exception e ){
            LOG.error("Unable to create TmAuditTlog, throwing exception");
            throw new RuntimeException(e);
         }
      }
      try {
           //System.out.println("In init - pre-newTransactionMgr");
           trxManager = new TransactionManager(config);
           //System.out.println("In init - post-newTransactionMgr");
      } catch (IOException e ){
            LOG.error("Unable to create TransactionManager, throwing exception");
            throw new RuntimeException(e);
      }

      if (useRecovThread) {
         LOG.debug("Entering recovThread Usage");
          try {                                                                          
              tmZK = new HBaseTmZK(config, dtmID);                              
          }catch (IOException e ){                                                       
              LOG.error("Unable to create HBaseTmZK TM-zookeeper class, throwing exception");
              throw new RuntimeException(e);                                             
          }                                                                              
          recovThread = new RecoveryThread(tLog, tmZK, trxManager);                      
          recovThread.start();                     
      }
      LOG.debug("Exit init(String, String, String)");
      return true;
   }

   public boolean init(short dtmid) throws Exception {
      //System.out.println("In init - dtmId" + dtmid);

      setupLog4j();
      LOG.debug("Enter init(" + dtmid + ")");
      config = HBaseConfiguration.create();
      config.set("hbase.regionserver.class", "org.apache.hadoop.hbase.ipc.TransactionalRegionInterface");
      config.set("hbase.regionserver.impl", "org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegionServer");
      config.set("hbase.hregion.impl", "org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegion");
      config.set("hbase.hlog.splitter.impl", "org.apache.hadoop.hbase.regionserver.transactional.THLogSplitter");
      config.set("dtmid", String.valueOf(dtmid));
      config.set("CONTROL_POINT_TABLE_NAME", "TRAFODION._DTM_.TLOG" + String.valueOf(dtmid) + "_CONTROL_POINT");
      config.set("TLOG_TABLE_NAME", "TRAFODION._DTM_.TLOG" + String.valueOf(dtmid));

      this.dtmID = dtmid;
      this.useRecovThread = false;
      this.stallWhere = 0;

      useForgotten = true;
      try {
         String useAuditRecords = System.getenv("TM_ENABLE_FORGOTTEN_RECORDS");
         if (useAuditRecords != null) {
            useForgotten = (Integer.parseInt(useAuditRecords) != 0);
         }
      }
      catch (Exception e) {
         LOG.debug("TM_ENABLE_FORGOTTEN_RECORDS is not in ms.env");
      }
      LOG.info("useForgotten is " + useForgotten);

      useTlog = false;
      useRecovThread = false;
      try {
         String useAudit = System.getenv("TM_ENABLE_TLOG_WRITES");
         if (useAudit != null){
            useTlog = useRecovThread = (Integer.parseInt(useAudit) != 0);
         }
      }
      catch (Exception e) {
         LOG.debug("TM_ENABLE_TLOG_WRITES is not in ms.env");
      }
      if (useTlog) {
         try {
            tLog = new TmAuditTlog(config);
         } catch (Exception e ){
            LOG.error("Unable to create TmAuditTlog, throwing exception");
            throw new RuntimeException(e);
         }
      }
      try {
            trxManager = new TransactionManager(config);
      } catch (IOException e ){
            LOG.error("Unable to create TransactionManager, Exception: " + e + "throwing new RuntimeException");
            throw new RuntimeException(e);
      }

      if (useRecovThread) {
         LOG.debug("Entering recovThread Usage");
          try {                                                                          
              tmZK = new HBaseTmZK(config, dtmID);                              
          }catch (IOException e ){                                                       
              LOG.error("Unable to create HBaseTmZK TM-zookeeper class, throwing exception");
              throw new RuntimeException(e);                                             
          }                                                                              
          recovThread = new RecoveryThread(tLog, tmZK, trxManager);                      
          recovThread.start();                     
      }
      LOG.trace("Exit init()");
      return true;
   }

   public short stall (int where) {
      LOG.debug("Entering stall with parameter " + where);
      this.stallWhere = where;
      return TransReturnCode.RET_OK.getShort();
   }

   public long beginTransaction(final long transactionId) throws Exception
    {
      
      LOG.debug("Enter beginTransaction, txid: " + transactionId);
      TransactionState tx = trxManager.beginTransaction(transactionId);
      if(tx == null) {
    	  LOG.error("null Transaction State returned by the Transaction Manager, txid: " + transactionId);
    	  throw new Exception("TransactionState is null");
      }

      mapTransactionStates.put(tx.getTransactionId(), tx);

      LOG.debug("Exit beginTransaction, Transaction State: " + tx + " mapsize: " + mapTransactionStates.size());
     return tx.getTransactionId();
   }

   public short abortTransaction(final long transactionID) throws Exception {
      LOG.debug("Enter abortTransaction, txid: " + transactionID);
      TransactionState ts = mapTransactionStates.get(transactionID);

      if(ts == null) {
          LOG.error("Returning from HBaseTxClient:abortTransaction, txid: " + transactionID + " retval: " + TransReturnCode.RET_NOTX.toString());
          return TransReturnCode.RET_NOTX.getShort();
      }

      try {
         ts.setStatus(TransState.STATE_ABORTED);
         if (useTlog) {
            tLog.putSingleRecord(transactionID, "ABORTED", ts.getParticipatingRegions(), false);
         }
      } catch(Exception e) {
         LOG.error("Returning from HBaseTxClient:abortTransaction, txid: " + transactionID + " tLog.putRecord: EXCEPTION");
         return TransReturnCode.RET_EXCEPTION.getShort();
      }

      if ((stallWhere == 1) || (stallWhere == 3)) {
         LOG.info("Stalling in phase 2 for abortTransaction");
         Thread.sleep(300000); // Initially set to run every 5 min                                 
      }

      try {
         trxManager.abort(ts);
      } catch(IOException e) {
          mapTransactionStates.remove(transactionID);
          LOG.error("Returning from HBaseTxClient:abortTransaction, txid: " + transactionID + " retval: EXCEPTION");
          return TransReturnCode.RET_EXCEPTION.getShort();
      }
      if (useTlog && useForgotten) {
         tLog.putSingleRecord(transactionID, "FORGOTTEN", null, false);
      }
 //     mapTransactionStates.remove(transactionID);

      LOG.trace("Exit abortTransaction, retval: OK txid: " + transactionID + " mapsize: " + mapTransactionStates.size());
      return TransReturnCode.RET_OK.getShort();
   }

   public short prepareCommit(long transactionId) throws Exception {
     LOG.debug("Enter prepareCommit, txid: " + transactionId);
     TransactionState ts = mapTransactionStates.get(transactionId);
     if(ts == null) {
       //System.out.println("prepare no TS for id " + transactionId + " so no work performed against txn.");
       LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + TransReturnCode.RET_NOTX.toString());
       return TransReturnCode.RET_NOTX.getShort(); 
     }
     else
       //System.out.println("prepare TS found for id " + transactionId);

     try {
        short result = (short) trxManager.prepareCommit(ts);
        LOG.debug("prepareCommit, [ " + ts + " ], result " + result + ((result == TransactionalRegionInterface.COMMIT_OK_READ_ONLY)?", Read-Only":""));

        switch (result) {
          case TransactionalRegionInterface.COMMIT_OK:
              //System.out.println("prepare reply COMMIT_OK id " + transactionId);
              LOG.trace("Exit OK prepareCommit, txid: " + transactionId);
              return TransReturnCode.RET_OK.getShort();
          case TransactionalRegionInterface.COMMIT_UNSUCCESSFUL:
              mapTransactionStates.remove(transactionId);
              LOG.trace("Exit NO_TX prepareCommit, txid: " + transactionId);
              return TransReturnCode.RET_NOTX.getShort();
          case TransactionalRegionInterface.COMMIT_OK_READ_ONLY:
             mapTransactionStates.remove(transactionId);
             LOG.trace("Exit OK_READ_ONLY prepareCommit, txid: " + transactionId);
             return TransReturnCode.RET_READONLY.getShort();
          case TransactionalRegionInterface.COMMIT_CONFLICT:
             LOG.info("Exit RET_HASCONFLICT prepareCommit, txid: " + transactionId);
             return TransReturnCode.RET_HASCONFLICT.getShort();
          default:
             LOG.trace("Exit TransReturnCode.RET_PARAMERR prepareCommit, txid: " + transactionId + ", error " + result);
             return TransReturnCode.RET_PARAMERR.getShort();
        }
     } catch (IOException e) {
  	   LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + TransReturnCode.RET_IOEXCEPTION.toString() + " IOException");
  	   return TransReturnCode.RET_IOEXCEPTION.getShort();
     } catch (CommitUnsuccessfulException e) {
  	   LOG.error("Returning from HBaseTxClient:prepareCommit, txid: " + transactionId + " retval: " + TransReturnCode.RET_NOCOMMITEX.toString() + " CommitUnsuccessfulException");
  	   return TransReturnCode.RET_NOCOMMITEX.getShort();
     }
   }

   public short doCommit(long transactionId) throws Exception {
       LOG.debug("Enter doCommit, txid: " + transactionId);
       TransactionState ts = mapTransactionStates.get(transactionId);

       if(ts == null) {
	  LOG.error("Returning from HBaseTxClient:doCommit, (null tx) retval: " + TransReturnCode.RET_NOTX.toString() + " txid: " + transactionId);
          return TransReturnCode.RET_NOTX.getShort();
       }

       try {
          ts.setStatus(TransState.STATE_COMMITTED);
          if (useTlog) {
             tLog.putSingleRecord(transactionId, "COMMITTED", ts.getParticipatingRegions(), true);
          }
       } catch(Exception e) {
          LOG.error("Returning from HBaseTxClient:doCommit, txid: " + transactionId + " tLog.putRecord: EXCEPTION " + e);
          return TransReturnCode.RET_EXCEPTION.getShort();
       }

       if ((stallWhere == 2) || (stallWhere == 3)) {
          LOG.info("Stalling in phase 2 for doCommit");
          Thread.sleep(300000); // Initially set to run every 5 min                                 
       }

       try {
          trxManager.doCommit(ts);
       } catch (CommitUnsuccessfulException e) {
          LOG.error("Returning from HBaseTxClient:doCommit, retval: " + TransReturnCode.RET_EXCEPTION.toString() + " IOException" + " txid: " + transactionId);
          return TransReturnCode.RET_EXCEPTION.getShort();
       }
       if (useTlog && useForgotten) {
          tLog.putSingleRecord(transactionId, "FORGOTTEN", null, false);
       }
//       mapTransactionStates.remove(transactionId);

       LOG.trace("Exit doCommit, retval(ok): " + TransReturnCode.RET_OK.toString() + " txid: " + transactionId + " mapsize: " + mapTransactionStates.size());

       return TransReturnCode.RET_OK.getShort();
   }

   public short completeRequest(long transactionId) throws Exception {
     LOG.debug("Enter completeRequest, txid: " + transactionId);
     TransactionState ts = mapTransactionStates.get(transactionId);

     if(ts == null) {
          LOG.error("Returning from HBaseTxClient:completeRequest, (null tx) retval: " + TransReturnCode.RET_NOTX.toString() + " txid: " + transactionId);
          return TransReturnCode.RET_NOTX.getShort();
       }

       try {
          ts.completeRequest();
       } catch(Exception e) {
          LOG.error("Returning from HBaseTxClient:completeRequest, ts.completeRequest: EXCEPTION" + " txid: " + transactionId);
       throw new Exception("Exception during completeRequest, unable to commit.");
       }

     mapTransactionStates.remove(transactionId);

     LOG.debug("Exit completeRequest txid: " + transactionId + " mapsize: " + mapTransactionStates.size());
     return TransReturnCode.RET_OK.getShort();
   }

   
   public short tryCommit(long transactionId) throws Exception {
     LOG.debug("Enter tryCommit, txid: " + transactionId);
     short err, commitErr, abortErr = TransReturnCode.RET_OK.getShort();
    
     try {
       err = prepareCommit(transactionId);
       if (err != TransReturnCode.RET_OK.getShort()) {
         LOG.debug("tryCommit prepare failed with error " + err);
         return err;
       }
       commitErr = doCommit(transactionId);
       if (commitErr != TransReturnCode.RET_OK.getShort()) {
         abortErr = abortTransaction(transactionId);
         LOG.debug("tryCommit commit failed and was aborted. Commit error " + 
                   commitErr + ", Abort error " + abortErr);
       }
       err = completeRequest(transactionId);
       if (err != TransReturnCode.RET_OK.getShort()) 
         LOG.debug("tryCommit completeRequest failed with error " + err);
       
     } catch(Exception e) {
       mapTransactionStates.remove(transactionId);
       LOG.error("Returning from HBaseTxClient:tryCommit, ts: EXCEPTION" + " txid: " + transactionId);
       throw new Exception("Exception " + e + "during tryCommit, unable to commit.");
    }

    mapTransactionStates.remove(transactionId);
  
    LOG.debug("Exit tryCommit txid: " + transactionId + " mapsize: " + mapTransactionStates.size());
    return TransReturnCode.RET_OK.getShort();
  }
   
   
    public short callRegisterRegion(long transactionId,
						 int  pv_port,
						 byte[] pv_hostname,
 						 byte[] pv_regionInfo) throws Exception {
 	String hostname    = new String(pv_hostname);
	LOG.debug("Enter callRegisterRegion, txid: [" + transactionId + "]");
	LOG.trace("callRegisterRegion, txid: [" + transactionId + "], port: " + pv_port + ", hostname: " + hostname + ", reg info len: " + pv_regionInfo.length + " " + new String(pv_regionInfo, "UTF-8"));

       ByteArrayInputStream lv_bis = new ByteArrayInputStream(pv_regionInfo);
       DataInputStream lv_dis = new DataInputStream(lv_bis);
       HRegionInfo lv_regionInfo = new HRegionInfo();
       try {
	   lv_regionInfo.readFields(lv_dis);
       }
       catch (IOException e) {
           LOG.error("HBaseTxClient:callRegisterRegion exception in lv_regionInfo.readFields, retval: " +
		     TransReturnCode.RET_EXCEPTION.toString() +
		     " txid: " + transactionId +
		     " IOException: " + e);
           throw new Exception("IOException in lv_regionInfo.readFields, unable to register region");
       }

       TransactionRegionLocation regionLocation = new TransactionRegionLocation(lv_regionInfo, hostname, pv_port);
       String regionTableName = regionLocation.getRegionInfo().getTableNameAsString();

       TransactionState ts = mapTransactionStates.get(transactionId);
       if(ts == null) {
          ts = trxManager.beginTransaction(transactionId);
       }

       try {
          trxManager.registerRegion(ts, regionLocation);
       } catch (IOException e) {
          LOG.error("HBaseTxClient:callRegisterRegion exception in registerRegion call, txid: " + transactionId +
		    " retval: " + TransReturnCode.RET_EXCEPTION.toString() + " IOException " + e);
          return TransReturnCode.RET_EXCEPTION.getShort();
       }

       LOG.debug("RegisterRegion adding table name " + regionTableName);
       ts.addTableName(regionTableName);

       mapTransactionStates.put(ts.getTransactionId(), ts); 

       LOG.trace("Exit callRegisterRegion, txid: [" + transactionId + "] with mapsize: "
                  + mapTransactionStates.size());
       return TransReturnCode.RET_OK.getShort();
   }

   public int participatingRegions(long transactionId) throws Exception {
       LOG.trace("Enter participatingRegions, txid: " + transactionId);
       TransactionState ts = mapTransactionStates.get(transactionId);
       if(ts == null) {
         LOG.trace("Returning from HBaseTxClient:participatingRegions, txid: " + transactionId + " not found returning: 0");
          return 0;
       }
       int participants = ts.getParticipantCount() - ts.getRegionsToIgnoreCount();
       LOG.trace("Exit participatingRegions , txid: [" + transactionId + "] " + participants + " participants");
       return (ts.getParticipantCount() - ts.getRegionsToIgnoreCount());
   }

   public boolean addControlPoint() throws Exception {
      LOG.trace("Enter addControlPoint");
      try {
         LOG.trace("HBaseTxClient calling tLog.addControlPoint with mapsize " + mapTransactionStates.size());
         tLog.addControlPoint(mapTransactionStates);
      }
      catch(IOException e){
          LOG.error("addControlPoint IOException " + e);
          throw e;
      }
      LOG.trace("Exit addControlPoint, returning: ");
      return true;
   }
   
     /**
      * Thread to gather recovery information for regions that need to be recovered 
      */
     private static class RecoveryThread extends Thread{
             final int SLEEP_DELAY = 180000; // Initially set to run every 3min
             private TmAuditTlog audit;
             private HBaseTmZK zookeeper;
             private TransactionManager txnManager;
             private short tmID;
             private Set<Long> inDoubtList;

             /**
              * 
              * @param audit
              * @param zookeeper
              * @param txnManager
              */
             public RecoveryThread(TmAuditTlog audit, HBaseTmZK zookeeper,
                             TransactionManager txnManager) {
                          this.audit = audit;
                          this.zookeeper = zookeeper;
                          this.txnManager = txnManager;
                          this.inDoubtList = new HashSet<Long> ();
                          this.tmID = zookeeper.getTMID();
             }
             
             private void addRegionToTS(String hostnamePort, byte[] regionInfo,
            		                    TransactionState ts) throws Exception{
            	 HRegionInfo regionInfoLoc = new HRegionInfo();
                 final byte [] delimiter = ",".getBytes();
                 String[] result = hostnamePort.split(new String(delimiter), 3);

                 if (result.length < 2)
                         throw new IllegalArgumentException("Region array format is incorrect");

                 String hostname = result[0];
                 int port = Integer.parseInt(result[1]);                 
                 ByteArrayInputStream lv_bis = new ByteArrayInputStream(regionInfo);
                 DataInputStream lv_dis = new DataInputStream(lv_bis);
                 try {
                         regionInfoLoc.readFields(lv_dis);
                 } catch (Exception e) {                        
                         throw new Exception();
                 }
                 TransactionRegionLocation loc = new TransactionRegionLocation(regionInfoLoc,
                		                                                       hostname,
                		                                                       port);
                 ts.addRegion(loc);
             }

            @Override
             public void run() {                     
            	     int sleepTimeInt = 0;
                     String sleepTime = System.getenv("TMRECOV_SLEEP");
                     if (sleepTime != null)
                             sleepTimeInt = Integer.parseInt(sleepTime);                                         
                     
                     LOG.debug("Starting recovery thread for TM" + tmID);
                     while(true) {
                             Map<String, byte []> regions = new HashMap<String, byte []>();
                             Map<Long, TransactionState> transactionStates = 
                            		 new HashMap<Long, TransactionState>();                             		 
                             try {                                    
                                     regions = zookeeper.checkForRecovery();
                                     if(regions != null) 
                                         LOG.trace("Processing " + regions.size() + " regions");
                             } catch (Exception e) {
                                     LOG.error("An ERROR occurred while checking for regions to recover. " + "TM: " + tmID);
                                     StringWriter sw = new StringWriter();
                                     PrintWriter pw = new PrintWriter(sw);
                                     e.printStackTrace(pw);
                                     LOG.error(sw.toString()); 
                             }
                             
                             
                             for(Map.Entry<String,byte[]> region : regions.entrySet()) {  
                            	     List<Long> TxRecoverList = new ArrayList<Long>();
                                     LOG.trace("Processing region: " + new String(region.getValue()));
                                     String hostnamePort = region.getKey();
                                     byte [] regionInfo =  region.getValue();                                    
				                     try {
                                         TxRecoverList = txnManager.recoveryRequest(hostnamePort, regionInfo, tmID);
                                     } catch (Exception e) {
                                         LOG.error("Error calling recoveryRequest " + new String(regionInfo) + " TM " + tmID);
                                         e.printStackTrace();
                                     }				                    
				                     for(Long txid : TxRecoverList) {
				                    	 TransactionState ts = transactionStates.get(txid);
				                    	 if(ts == null) {
				                    		 ts = new TransactionState(txid);
				                    	 }
				                    	 try {
				                    		 this.addRegionToTS(hostnamePort, regionInfo, ts);
				                    	 } catch (Exception e) {
				                    		 LOG.error("Unable to add region to TransactionState" +
				                    		 		"region info: " + new String(regionInfo));
				                    		 e.printStackTrace();
				                    	 }
				                    	 transactionStates.put(txid, ts);
				                     }				                     
                             }

                             for(Map.Entry<Long, TransactionState> tsEntry: transactionStates.entrySet()) {
                            	   TransactionState ts = tsEntry.getValue();
                            	   Long txID = tsEntry.getKey();
                                   // TransactionState ts = new TransactionState(txID);
                                   try {
                                           audit.getTransactionState(ts);
                                           if(ts.getStatus().equals("COMMITTED")) {
                                                   LOG.debug("Redriving commit for " + ts.getTransactionId());
                                                   txnManager.doCommit(ts);
                                           }
                                           else if(ts.getStatus().equals("ABORTED")) {
                                                   LOG.debug("Redriving abort for " + ts.getTransactionId());
                                                   txnManager.abort(ts);
                                           }
                                           else {
                                                   LOG.debug("Redriving abort for " + ts.getTransactionId());
                                                   LOG.warn("Recovering transaction " + txID + ", status is not set to COMMITTED or ABORTED. Aborting.");
                                                   txnManager.abort(ts);
                                           }

                                   }catch (Exception e) {
                                           LOG.error("Unable to get audit record for tx: " + txID + ", audit is throwing exception.");
                                           e.printStackTrace();
                                   }
                             }

                             try {
                            	     if(sleepTimeInt > 0) 
                            	    	 Thread.sleep(sleepTimeInt);
                            	     else
                            	    	 Thread.sleep(SLEEP_DELAY);                                     
                             } catch (Exception e) {
                                     e.printStackTrace();
                             }
                     }
             }
     }

     //================================================================================
     // DTMCI Calls
     //================================================================================
    
     //--------------------------------------------------------------------------------
     // callRequestRegionInfo
     // Purpose: Prepares HashMapArray class to get region information
     //--------------------------------------------------------------------------------
      public HashMapArray callRequestRegionInfo() throws Exception {

      String tablename, encoded_region_name, region_name, is_offline, region_id, hostname, port, thn;

      HashMap<String, String> inMap;
      long lv_ret = -1;
      Long key;
      TransactionState value;
      int tnum = 0; // Transaction number

      LOG.trace("HBaseTxClient::callRequestRegionInfo:: start\n");

      HashMapArray hm = new HashMapArray();

      try{
      for(ConcurrentHashMap.Entry<Long, TransactionState> entry : mapTransactionStates.entrySet()){
          key = entry.getKey();
          value = entry.getValue();
          long id = value.getTransactionId();

          TransactionState ts = mapTransactionStates.get(id);
          final Set<TransactionRegionLocation> regions = ts.getParticipatingRegions();

          // TableName
          Iterator<TransactionRegionLocation> it = regions.iterator();
          tablename = it.next().getRegionInfo().getTableNameAsString();
          while(it.hasNext()){
              tablename = tablename + ";" + it.next().getRegionInfo().getTableNameAsString();
          }
          hm.addElement(tnum, "TableName", tablename);

          // Encoded Region Name
          Iterator<TransactionRegionLocation> it2 = regions.iterator();
          encoded_region_name = it2.next().getRegionInfo().getEncodedName();
          while(it2.hasNext()){
              encoded_region_name = encoded_region_name + ";" + it2.next().getRegionInfo().getTableNameAsString();
          }
          hm.addElement(tnum, "EncodedRegionName", encoded_region_name);

          // Region Name
          Iterator<TransactionRegionLocation> it3 = regions.iterator();
          region_name = it3.next().getRegionInfo().getRegionNameAsString();
          while(it3.hasNext()){
              region_name = region_name + ";" + it3.next().getRegionInfo().getTableNameAsString();
          }
          hm.addElement(tnum, "RegionName", region_name);

          // Region Offline
          Iterator<TransactionRegionLocation> it4 = regions.iterator();
          boolean is_offline_bool = it4.next().getRegionInfo().isOffline();
          is_offline = String.valueOf(is_offline_bool);
          hm.addElement(tnum, "RegionOffline", is_offline);

          // Region ID
          Iterator<TransactionRegionLocation> it5 = regions.iterator();
          region_id = String.valueOf(it5.next().getRegionInfo().getRegionId());
          while(it5.hasNext()){
              region_id = region_id + ";" + it5.next().getRegionInfo().getRegionId();
          }
          hm.addElement(tnum, "RegionID", region_id);

          // Hostname
          Iterator<TransactionRegionLocation> it6 = regions.iterator();
          thn = String.valueOf(it6.next().getHostname());
          hostname = thn.substring(0, thn.length()-1);
          while(it6.hasNext()){
              thn = String.valueOf(it6.next().getHostname());
              hostname = hostname + ";" + thn.substring(0, thn.length()-1);
          }
          hm.addElement(tnum, "Hostname", hostname);

          // Port
          Iterator<TransactionRegionLocation> it7 = regions.iterator();
          port = String.valueOf(it7.next().getPort());
          while(it7.hasNext()){
              port = port + ";" + String.valueOf(it7.next().getPort());
          }
          hm.addElement(tnum, "Port", port);

          tnum = tnum + 1;
        }
      }catch(Exception e){
         LOG.trace("Error in getting region info. Map might be empty. Please ensure sqlci insert was done");
      }

      LOG.trace("HBaseTxClient::callRequestRegionInfo:: end size: " + hm.getSize());
      return hm;
   }
}

