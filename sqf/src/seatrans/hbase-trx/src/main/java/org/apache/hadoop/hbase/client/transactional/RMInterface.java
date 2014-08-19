package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;


import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.client.HConnection;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.client.ServerCallable;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.CommitUnsuccessfulException;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.TransState;
import org.apache.hadoop.hbase.client.transactional.TransReturnCode;
import org.apache.hadoop.hbase.client.transactional.TransactionMap;
import org.apache.hadoop.hbase.ipc.TransactionalRegionInterface;

import java.util.Map;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.HashSet;
import java.util.concurrent.ConcurrentHashMap;

public class RMInterface extends TransactionalTable{
    static final Log LOG = LogFactory.getLog(RMInterface.class);
    TransactionManager trxManager;
    private static int stallWhere;
    boolean useTlog;
    boolean useForgotten;
    boolean useRecovThread;
    TransactionMap transactionMap;
    
    ConcurrentHashMap<Long, TransactionState> mapTransactionStates;
    
    static Map<Long, Set<RMInterface>> mapRMsPerTransaction = new HashMap<Long,  Set<RMInterface>>();

    static {
        System.loadLibrary("stmlib");
   }

    //private native void registerRegion(int port, byte[] hostname, byte[] regionInfo);
 
    public static void main(String[] args) {
      System.out.println("MAIN ENTRY");      
    }
    
    public RMInterface(final Configuration conf, final String tableName) throws IOException, RuntimeException {
        super(conf, Bytes.toBytes(tableName));
        //mapTransactionStates = new ConcurrentHashMap<Long, TransactionState>();
        mapTransactionStates = TransactionMap.get();
        
        // From HBaseTxClient
        stallWhere = 0;
        useTlog = false;
        useForgotten = false;
        useRecovThread = false;
        try {
          //System.out.println("In RMInterface ctor - pre-newTransactionMgr");
          trxManager = new TransactionManager(conf);
          //System.out.println("In RMInterface ctor - post-newTransactionMgr");
        } catch (IOException e ){
          LOG.error("Unable to create TransactionManager, throwing exception");
          throw new RuntimeException(e);
        }
    }
    
    public synchronized TransactionState registerTransaction(final long transactionID, final byte[] row) throws IOException {
        LOG.trace("Enter RMInterface:registerTransaction, transaction ID: " + transactionID);
        boolean register = false;
        short ret = 0;

        TransactionState ts = mapTransactionStates.get(transactionID);
        
        // if we don't have a TransactionState for this ID we need to register it with the TM
        if (ts == null) {
            ts = new TransactionState(transactionID);
            LOG.trace("RMInterface:registerTransaction, created TransactionState " + ts);
            mapTransactionStates.put(transactionID, ts);
            register = true;
        }
        else {
            LOG.trace("RMInterface:registerTransaction - Found TS in map for id " + transactionID);
        }
        HRegionLocation location = super.getRegionLocation(row, false /*reload*/);

        TransactionRegionLocation trLocation = new TransactionRegionLocation(location.getRegionInfo(),
                                                                             location.getHostname(), 
                                                                             location.getPort());
        LOG.trace("RMInterface:registerTransaction, created TransactionRegionLocation " + trLocation);
           
        // if this region hasn't been registered as participating in the transaction, we need to register it
        if (ts.addRegion(trLocation)) {
          register = true;
          LOG.trace("RMInterface:registerTransaction, added TransactionRegionLocation to ts");
        }
         
        // register region with TM.
        if (register) {
    	    ts.registerLocation(location);
    	    
    	    Set<RMInterface> lv_set_rm = mapRMsPerTransaction.get(transactionID);
    	    if (lv_set_rm == null) {
        		lv_set_rm = new HashSet<RMInterface>();
        		mapRMsPerTransaction.put(transactionID, lv_set_rm);
    	    }
    	    lv_set_rm.add(this);
    	    LOG.trace("txid: " + transactionID + " mapRMsPerTransaction.lv_set_rm length: " + lv_set_rm.size());
        }
        else {
          LOG.trace("RMInterface:registerTransaction did not send registerRegion.");
        }

        if ((ts == null) || (ret != 0)) {
            LOG.error("registerTransaction failed, TransactionState is NULL"); 
            throw new IOException("registerTransaction failed with error.");
        }

        LOG.trace("Exit registerTransaction, transaction ID: " + transactionID);
        return ts;
    }
   
    
    static public void clearTransactionStates(final long transactionID) {
	LOG.trace("cts1 Enter txid: " + transactionID);
	Set<RMInterface> lv_set_rm = mapRMsPerTransaction.get(transactionID);
	if (lv_set_rm == null) {
	    LOG.warn("No entry for txid: " + transactionID);
	    return;
	}
	LOG.trace("cts2 txid: " + transactionID + " mapRMsPerTransaction.lv_set_rm length: " + lv_set_rm.size());
	for (RMInterface lv_rm : lv_set_rm) {
	    lv_rm.unregisterTransaction(transactionID);
	}
	
	mapRMsPerTransaction.remove(transactionID);
	LOG.trace("cts3 txid: " + transactionID);
    }
    
    public synchronized void unregisterTransaction(final long transactionID) {
	LOG.trace("Enter txid: " + transactionID);
        mapTransactionStates.remove(transactionID);
    }

    public synchronized void unregisterTransaction(TransactionState ts) {
        mapTransactionStates.remove(ts.getTransactionId());
    }
    
    public synchronized Result get(final long transactionID, final Get get) throws IOException {
        LOG.trace("get txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, get.getRow());
        Result res = super.get(ts, get);
        LOG.trace("EXIT get -- result: " + res.toString());
        return res;	
    }
    
    public synchronized void delete(final long transactionID, final Delete delete) throws IOException {
        LOG.trace("delete txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, delete.getRow());
        super.delete(ts, delete);
    }

    public synchronized void delete(final long transactionID, final List<Delete> deletes) throws IOException {
        LOG.trace("delete txid: " + transactionID);
        TransactionState ts;
        for (Delete del : deletes) {
            ts = registerTransaction(transactionID, del.getRow());
        }
        ts = mapTransactionStates.get(transactionID);
        super.delete(ts, deletes);
    }

    
    public synchronized ResultScanner getScanner(final long transactionID, final Scan scan) throws IOException {
        LOG.trace("getScanner txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, scan.getStartRow());
        ResultScanner res = super.getScanner(ts, scan);
        LOG.trace("EXIT getScanner");
        return res;
    }
    
    public synchronized void put(final long transactionID, final Put put) throws IOException {
        LOG.trace("Enter Put txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, put.getRow());
        super.put(ts, put);
        LOG.trace("Exit Put txid: " + transactionID);
    }

    public synchronized void put(final long transactionID, final List<Put> puts) throws IOException {
        LOG.trace("Enter put (list of puts) txid: " + transactionID);
      	TransactionState ts;
      	for (Put put : puts) {
      	    ts = registerTransaction(transactionID, put.getRow());
      	}
      	ts = mapTransactionStates.get(transactionID);
        super.put(ts, puts);
        LOG.trace("Exit put (list of puts) txid: " + transactionID);
    }

    public synchronized boolean checkAndPut(final long transactionID, byte[] row, byte[] family, byte[] qualifier,
                       byte[] value, Put put) throws IOException {

        LOG.trace("Enter checkAndPut txid: " + transactionID);
      	TransactionState ts = registerTransaction(transactionID, row);
      	return super.checkAndPut(ts, row, family, qualifier, value, put);
    }

    public synchronized boolean checkAndDelete(final long transactionID, byte[] row, byte[] family, byte[] qualifier,
                       byte[] value, Delete delete) throws IOException {

        LOG.trace("Enter checkAndDelete txid: " + transactionID);
      	TransactionState ts = registerTransaction(transactionID, row);
      	return super.checkAndDelete(ts, row, family, qualifier, value, delete);
    }
}
