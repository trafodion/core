// TrxTableClient.java
  
/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/
/* TEST */
package org.apache.hadoop.hbase.coprocessor.transactional;

import java.io.IOException;
import java.util.List;
import java.util.Map;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.TableExistsException;
import org.apache.hadoop.hbase.TableNotFoundException;
import org.apache.hadoop.hbase.TableNotEnabledException;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto;
import org.apache.hadoop.hbase.protobuf.generated.ClientProtos.MutationProto.MutationType;
import org.apache.hadoop.hbase.protobuf.ProtobufUtil;
import com.google.protobuf.HBaseZeroCopyByteString;
import com.google.protobuf.ByteString;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.*;
import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
import org.apache.hadoop.hbase.ipc.ServerRpcController;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.Pair;

public class TestDDL {

  static String regionname = "RegionName";
  static HTable ht = null;
  static long id = 1L;
  static long scannerId = 0L;
  static boolean checkResult = false;
  static boolean hasMore = false;
  static long totalRows = 0L;
  static boolean continuePerform = true;
  static byte [][] startKeys = null;
  static int startPos = 0;
  static byte [] startRow = null;
  static byte [] lastRow = null;
  private static Result lastResult = null;
  static List<HRegionLocation> regionsList = null;
  static int regionCount = 0;
  static Scan scan = null;
  private HRegionInfo currentRegion = null;
  static Pair<byte[][], byte[][]> startEndKeys = null;

  private static final String TABLE_NAME = "table1";
  
  private static final byte[] FAMILY = Bytes.toBytes("family");
  private static final byte[] FAMILYBAD = Bytes.toBytes("familybad");
  private static final byte[] QUAL_A = Bytes.toBytes("a");
  private static final byte[] QUAL_B = Bytes.toBytes("b");

  private static final byte[] ROW1 = Bytes.toBytes("row1");
  private static final byte[] ROW2 = Bytes.toBytes("row2");
  private static final byte[] ROW3 = Bytes.toBytes("row3");
  private static final byte[] ROW4 = Bytes.toBytes("row4");
  private static final byte[] ROW5 = Bytes.toBytes("row5");
  private static final byte[] ROW6 = Bytes.toBytes("row6");
  private static final byte [] VALUE = Bytes.toBytes("testValue");
  private static final byte [] VALUE1 = Bytes.toBytes(1);
  private static final byte [] VALUE2 = Bytes.toBytes(2);

  private static HBaseAdmin admin;

 // Initialize and set up tables 
    public static void initialize() throws Exception {
 
     Configuration config = HBaseConfiguration.create();

     HTableDescriptor desc = new HTableDescriptor(TABLE_NAME);
     desc.addFamily(new HColumnDescriptor(FAMILY));
     admin = new HBaseAdmin(config);
  
     try {
       System.out.println ("  Cleaning up the table " + TABLE_NAME);
       admin.disableTable(TABLE_NAME);
       admin.deleteTable(TABLE_NAME);
     }
     catch (TableNotFoundException e) {
       System.out.println("  Table " + TABLE_NAME + " was not found");
     }
     catch (TableNotEnabledException n) {
       System.out.println("  Table " + TABLE_NAME + " is not enabled");
     }
  

     try {
       System.out.println ("  Creating the table " + TABLE_NAME);
       admin.createTable(desc);
     }
     catch (TableExistsException e) {
       System.out.println("  Table " + TABLE_NAME + " already exists");
     }

     ht = new HTable(config, desc.getName());
     try {
       startKeys = ht.getStartKeys();
       startRow = startKeys[startPos];
       System.out.println("  Table " + TABLE_NAME + " startRow is " + startRow);
     } catch (IOException e) {
       System.out.println("  Table " + TABLE_NAME + " unable to get start keys" + e);
     }
     for (int i = 0; i < startKeys.length; i++){
     String regionLocation = ht.getRegionLocation(startKeys[i]).
        getHostname();
       System.out.println("  Table " + TABLE_NAME + " region location" + regionLocation + ", startKey is " + startKeys[i]);
     }

 try {
        startEndKeys = ht.getStartEndKeys();
        for (int i = 0; i < startEndKeys.getFirst().length; i++) {
          System.out.println(" First key: " + startEndKeys.getFirst()[i] +  ", Second key: "  + startEndKeys.getSecond()[i]);
        }
     } catch (Exception e) {
       System.out.println("  Table " + TABLE_NAME + " unable to get start and endkeys" + e);
     }


     regionsList = ht.getRegionsInRange(HConstants.EMPTY_START_ROW, HConstants.EMPTY_START_ROW);

     int first = 0;
     for (HRegionLocation regionLocation : regionsList) {
        HRegionInfo region = regionLocation.getRegionInfo();
        if (first == 0) {
          regionname = region.getRegionNameAsString();
          first++;
        }
          
        System.out.println("\t\t" + region.getRegionNameAsString());
     }
  }


   public static void testDDLCreate(String tblname) throws IOException {

    System.out.println("Starting testDDLCreate");
    
    final HTableDescriptor ddldesc = new HTableDescriptor(tblname);
    ddldesc.addFamily(new HColumnDescriptor(FAMILY));
    
    Batch.Call<TrxRegionService, DDLRequestResponse> callable = 
        new Batch.Call<TrxRegionService, DDLRequestResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<DDLRequestResponse> rpcCallback = 
        new BlockingRpcCallback<DDLRequestResponse>();         

      @Override
      public DDLRequestResponse call(TrxRegionService instance) throws IOException {        
        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DDLRequestRequest.Builder builder = DDLRequestRequest.newBuilder();        
        builder.setTransactionId(id);
        builder.setTableSchema(ddldesc.convert());
        builder.setDdlType(org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DDLType.CREATE);
                
        instance.ddlRequest(controller, builder.build(), rpcCallback);
        return rpcCallback.get();        
      }
    };
 
     Map<byte[], DDLRequestResponse> result = null;   
      try {
        result = ht.coprocessorService(TrxRegionService.class, null, null, callable);
      } catch (Throwable e) {
        e.printStackTrace();     
      }

      for (DDLRequestResponse dresponse : result.values())
      {
        boolean hasException = dresponse.getHasException();
        String exception = dresponse.getException();
        if (hasException)
        {
          System.out.println("DDLRequestResponse exception " + exception );
          throw new IOException(exception);
        }
      }

    System.out.println("Finished testDDLCreate");
    return;
  } 
  
  public static void testDDLDrop(String tblname) throws IOException {

    System.out.println("Starting testDDLDrop");
    
    final HTableDescriptor ddldesc = new HTableDescriptor(tblname);
    ddldesc.addFamily(new HColumnDescriptor(FAMILY));
    
    Batch.Call<TrxRegionService, DDLRequestResponse> callable = 
        new Batch.Call<TrxRegionService, DDLRequestResponse>() {
      ServerRpcController controller = new ServerRpcController();
      BlockingRpcCallback<DDLRequestResponse> rpcCallback = 
        new BlockingRpcCallback<DDLRequestResponse>();         

      @Override
      public DDLRequestResponse call(TrxRegionService instance) throws IOException {        
        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DDLRequestRequest.Builder builder = DDLRequestRequest.newBuilder();        
        builder.setTransactionId(id);
        builder.setTableSchema(ddldesc.convert());
        builder.setDdlType(org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.DDLType.DROP);
                
        instance.ddlRequest(controller, builder.build(), rpcCallback);
        return rpcCallback.get();        
      }
    };
 
     Map<byte[], DDLRequestResponse> result = null;   
      try {
        result = ht.coprocessorService(TrxRegionService.class, null, null, callable);
      } catch (Throwable e) {
        e.printStackTrace();     
      }

      for (DDLRequestResponse dresponse : result.values())
      {
        boolean hasException = dresponse.getHasException();
        String exception = dresponse.getException();
        if (hasException)
        {
          System.out.println("DDLRequestResponse exception " + exception );
          throw new IOException(exception);
        }
      }

    System.out.println("Finished testDDLDrop");
    return;
  } 
  
 
   public static void main(String[] args) {
    
    System.out.println("Starting TrxTableClient");
    int option =0;
	String tblname =null;
	if (args.length > 0) {
    try {
        option = Integer.parseInt(args[0]);
		tblname = args[1];
		} catch (NumberFormatException e) {
			System.err.println("Argument" + args[0] + " must be an integer.");
			System.exit(1);
		}
	}
	
    try {
       initialize();
	   switch(option)
	   {
		   case 1: testDDLCreate(tblname);
		   break;
		   
		   case 2: testDDLDrop(tblname);
		   break;
		   
		   default:
	   }
	        
    } catch (IOException e) {
      System.out.println("TrxTableClient threw IOException");
      System.out.println(e.toString());
    } catch (Throwable t) {
      System.out.println("TrxTableClient threw throwable exception");
      System.out.println(t.toString());
    }

    System.out.println("Finished TrxTableClient");
  }

}
