package org.trafodion.dtm;

import java.io.IOException;
import java.util.Collection;
import java.util.HashMap;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.ServerName;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HConnection;
import org.apache.hadoop.hbase.client.HConnectionManager;
import org.apache.hadoop.hbase.ipc.HMasterInterface;
import org.apache.hadoop.hbase.ipc.TransactionalRegionInterface;
import org.apache.hadoop.hbase.ipc.HRegionInterface;


public class TrafInfo {
  
    private HBaseAdmin hbadmin;
    private HConnection connection;
    Configuration     config;
    HMasterInterface  hmaster; 

    public TrafInfo() throws IOException {
        init();
    }
           
    public void init() throws IOException {
        this.config = HBaseConfiguration.create();
        this.connection = HConnectionManager.getConnection(config);   
    
        try {
            hbadmin = new HBaseAdmin(config);
            hmaster = hbadmin.getMaster();
        } catch(Exception e) {
            System.out.println("ERROR: Unable to obtain HBase accessors, Exiting");
            e.printStackTrace();
            System.exit(1);
        } 
    }
   
    public static void printHelp() {
        System.out.println("Run: $JAVA_HOME/bin/java org.trafodion.dtm.TrafInfo <command>");
        System.out.println("Commands to gather Transactional Region information:");
        System.out.println("    active      ::  active transactions per region");
        System.out.println("    committed   ::  committed transactions per region by sequence number");
        System.out.println("    indoubt     ::  in-doubt transactions per region");
        System.out.println("    <command> -v::  shows metadata tables"); 
    }
   
    public void getActivePendingTrans(String transType, String showmd){
        System.out.println("\n====================================================================");
        System.out.println("\t\tActive Pending Transactions");
        getTransactions(transType, showmd);
    }

    public void getCommittedTransactions(String transType, String showmd){
        System.out.println("\n====================================================================");
        System.out.println("\t\tCommitted Transactions by Sequence Number");
        getTransactions(transType, showmd);
    }

    public void getInDoubtTransactions(String transType, String showmd){
        System.out.println("\n====================================================================");
        System.out.println("\t\tIn-Doubt Transactions");
        getTransactions(transType, showmd);
    }

    public void getTransactions(String transType, String showmd){
        String regionName, tableName;
        int idx;
        
        Collection<ServerName> sn = hmaster.getClusterStatus().getServers();
        for(ServerName sname : sn) {
            System.out.println("===================================================================="
                             + "\nServer Name: " + sname.toString() + "\n"
                             + "\nTransId    RegionId             TableName");

            try {

                HRegionInterface regionServer = connection.getHRegionConnection(sname.getHostname(), sname.getPort());
                List<HRegionInfo> regions = regionServer.getOnlineRegions();
                connection.close();

                TransactionalRegionInterface transactionalRegionServer = (TransactionalRegionInterface)this.connection
                                                            .getHRegionConnection(sname.getHostname(), sname.getPort());

                for (HRegionInfo rinfo: regions) {

                    regionName = rinfo.getRegionNameAsString();
                    idx = regionName.indexOf(',');
                    tableName = regionName.substring(0, idx);

                    if(!showmd.contains("-v")){
                        if((tableName.contains("TRAFODION._MD_.")) || (tableName.contains("-ROOT-")) ||
                           (tableName.equals(".META."))){
                            continue;
                        }
                    }

                    System.out.println("--------------------------------------------------------------------"
                                     + "\n\t   " + rinfo.getRegionId()
                                     + "\t"      + tableName);

                    if(transType.equals("active")){
                        List<Long> result = transactionalRegionServer.getPendingTrans(rinfo.getRegionName());
                        for(Long res : result)
                            System.out.println(res);
                    }
                    else if(transType.equals("committed")){
                        List<Long> result = transactionalRegionServer.getCommittedTrans(rinfo.getRegionName());
                        for(Long res : result)
                            System.out.println(res);
                    }
                    else if(transType.equals("indoubt")){
                        List<Long> result = transactionalRegionServer.getInDoubtTrans(rinfo.getRegionName());
                            for(Long res : result)
                            System.out.println(res);
                    }
                }

            } catch(Exception e) {
                System.out.println("ERROR: Unable to get region info, Exiting");
                e.printStackTrace();
                System.exit(1);
            }
        }
    }
    
    public static void main(String[] args) throws IOException {
        
        if(args.length == 0) {
            TrafInfo.printHelp();
            System.exit(0);
        }

        TrafInfo ti = new TrafInfo();

        if(args.length == 1){
            if(args[0].equals("help"))
                TrafInfo.printHelp();
            else if(args[0].equals("active")) 
                ti.getActivePendingTrans(args[0], "");
            else if(args[0].equals("committed"))
                ti.getCommittedTransactions(args[0], "");
            else if(args[0].equals("indoubt"))
                ti.getInDoubtTransactions(args[0], "");
        }
        // Verbose shows Metadata tables
        if(args.length == 2){
            if(args[0].equals("active"))
                ti.getActivePendingTrans(args[0], "-v");
            else if(args[0].equals("committed"))
                ti.getCommittedTransactions(args[0], "-v");
            else if(args[0].equals("indoubt"))
                ti.getInDoubtTransactions(args[0], "-v");
        }
    }
}   
