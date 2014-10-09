package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;
import java.util.LinkedList;
import java.util.Map;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.client.AbstractClientScanner;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.coprocessor.Batch;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CloseScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CloseScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.OpenScannerRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.OpenScannerResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PerformScanRequest;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PerformScanResponse;
import org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.TrxRegionService;
import org.apache.hadoop.hbase.ipc.BlockingRpcCallback;
import org.apache.hadoop.hbase.ipc.ServerRpcController;
import org.apache.hadoop.hbase.protobuf.ProtobufUtil;
import org.apache.hadoop.hbase.util.Bytes;

import com.google.protobuf.ByteString;


/*
 *   Simple Transaction Scanner
 */
public class TransactionalScanner extends AbstractClientScanner {
	private final Log LOG = LogFactory.getLog(this.getClass());
	public Scan scan;
	public Long scannerID;
	public TransactionState ts;
	public TransactionalTable ttable;
	protected boolean closed = false;
    protected boolean doNotCloseOnLast = true;
    protected int nbRows = 100;
    protected long nextCallSeq = 0;
	private boolean firstEntry = true;
	public HRegionInfo currentRegion;
	public byte[] currentBeginKey;
	public byte[] currentEndKey;
	protected final LinkedList<Result> cache = new LinkedList<Result>();
	
	
	public TransactionalScanner(final TransactionalTable ttable, final TransactionState ts, final Scan scan, final Long scannerID) {
		super();
		this.scan = scan;
		this.scannerID = scannerID;
		this.ts = ts;
		this.ttable = ttable;
        this.nbRows = scan.getCaching();
        if (nbRows <= 0)
            nbRows = 100;
		try {
			nextScanner(false);
		}catch (IOException e) {
			LOG.error("nextScanner error");
		}
	}

    protected boolean checkScanStopRow(final byte [] endKey) {
      if (this.scan.getStopRow().length > 0) {
        byte [] stopRow = scan.getStopRow();
        int cmp = Bytes.compareTo(stopRow, 0, stopRow.length,
          endKey, 0, endKey.length);
        if (cmp <= 0) {
          return true;
        }
      }
      return false;
    }
    
	@Override
	public void close() {
		if(LOG.isTraceEnabled()) LOG.trace("close() -- ENTRY txID: " + ts.getTransactionId());
		if(closed == true) {
			if(LOG.isTraceEnabled()) LOG.trace("close()  already closed -- EXIT txID: " + ts.getTransactionId());
			return;
		}
		 Batch.Call<TrxRegionService, CloseScannerResponse> callable =
			        new Batch.Call<TrxRegionService, CloseScannerResponse>() {
			      ServerRpcController controller = new ServerRpcController();
			      BlockingRpcCallback<CloseScannerResponse> rpcCallback =
			        new BlockingRpcCallback<CloseScannerResponse>();

			      @Override
			      public CloseScannerResponse call(TrxRegionService instance) throws IOException {
			        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.CloseScannerRequest.Builder builder = CloseScannerRequest.newBuilder();
			        builder.setTransactionId(ts.getTransactionId());
			        builder.setScannerId(scannerID);			        
			        builder.setRegionName(ByteString.copyFromUtf8(""));

			        instance.closeScanner(controller, builder.build(), rpcCallback);
			        return rpcCallback.get();
			      }
			    };

			    Map<byte[], CloseScannerResponse> result = null;

			    try {
			      result = ttable.coprocessorService(TrxRegionService.class, 
                                                     currentBeginKey, 
                                                     currentEndKey,
                                                     callable);
			    } catch (Throwable e) {
			      e.printStackTrace();
			    }

			      for (CloseScannerResponse cresponse : result.values())
			      {
			        boolean hasException = cresponse.getHasException();
			        String exception = cresponse.getException();
			        if (hasException)
			        	if(LOG.isTraceEnabled()) LOG.trace("  CloseScannerResponse exception " + exception );
			      }
	    this.closed = true;
	    if(LOG.isTraceEnabled()) LOG.trace("close() -- EXIT txID: " + ts.getTransactionId());
		
	}
	
	protected boolean nextScanner(final boolean done) throws IOException{
		if(LOG.isTraceEnabled()) LOG.trace("nextScanner() -- ENTRY txID: " + ts.getTransactionId());
		if(this.currentBeginKey != null) {
			if(LOG.isTraceEnabled()) LOG.trace("nextScanner() currentBeginKey += null txID: " + ts.getTransactionId());
                        if (doNotCloseOnLast)
			  close();
			byte [] endKey = this.currentRegion.getEndKey();

			if(endKey == null ||
			   Bytes.equals(endKey, HConstants.EMPTY_BYTE_ARRAY) ||
			   checkScanStopRow(endKey) ||
			   done) {
				
				if(LOG.isTraceEnabled()) LOG.trace("nextScanner() -- EXIT -- returning false txID: " + ts.getTransactionId());
				return false;				
			}
			this.currentBeginKey = TransactionManager.binaryIncrementPos(endKey,1);
			
		}
		else {
			this.currentBeginKey = this.scan.getStartRow();					
		}
		
		this.currentRegion = ttable.getRegionLocation(this.currentBeginKey).getRegionInfo();
		if(LOG.isTraceEnabled()) LOG.trace("Region Info: " + currentRegion.getRegionNameAsString());
		if(this.currentEndKey != HConstants.EMPTY_END_ROW)
			this.currentEndKey = TransactionManager.binaryIncrementPos(currentRegion.getEndKey(), -1);
		
		this.closed = false;
	        Batch.Call<TrxRegionService, OpenScannerResponse> callable =
		        new Batch.Call<TrxRegionService, OpenScannerResponse>() {
		      ServerRpcController controller = new ServerRpcController();
		      BlockingRpcCallback<OpenScannerResponse> rpcCallback =
		        new BlockingRpcCallback<OpenScannerResponse>();

		      @Override
		      public OpenScannerResponse call(TrxRegionService instance) throws IOException {
		        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.OpenScannerRequest.Builder builder = OpenScannerRequest.newBuilder();
		        builder.setTransactionId(ts.getTransactionId());
		        builder.setRegionName(ByteString.copyFromUtf8(currentRegion.getRegionNameAsString()));
		        builder.setScan(ProtobufUtil.toScan(scan));

		        instance.openScanner(controller, builder.build(), rpcCallback);
		        return rpcCallback.get();
		      }
		    };

		    Map<byte[], OpenScannerResponse> result = null;

		    try {
		      result = ttable.coprocessorService(TrxRegionService.class, 
		    		  							 this.currentBeginKey,
		    		  							 this.currentEndKey,
		    		  							 callable);
		      		      
		    } catch (Throwable e) {
		      e.printStackTrace();
		    }

		      for (OpenScannerResponse oresponse : result.values())
		      {
		        
		        String exception = oresponse.getException();
		        boolean hasException = oresponse.getHasException();
		        if (hasException) {
		        	if(LOG.isTraceEnabled()) LOG.trace("nextScanner() -- EXIT -- encountered EXCEPTION returning false txID: " + ts.getTransactionId() + 
		        		  exception);		          		          
		        }
		        else{
		          
		        	this.scannerID = oresponse.getScannerId();
		        	if(LOG.isTraceEnabled()) LOG.trace("  OpenScannerResponse scannerId is " + this.scannerID );
		        }
		      }

		this.nextCallSeq = 0;
		if(LOG.isTraceEnabled()) LOG.trace("nextScanner() -- EXIT -- returning true txID: " + ts.getTransactionId());
		return true;
	}

	@Override
	public Result next() throws IOException {
		if(cache.size() == 0 && firstEntry == true) { 
			firstEntry = false;
			do { 
				if(LOG.isTraceEnabled()) LOG.trace("next() before coprocessor PerformScan call txID: " + ts.getTransactionId());	
				final long nextCallSeqInput = this.nextCallSeq; 
				Batch.Call<TrxRegionService, PerformScanResponse> callable = 
				        new Batch.Call<TrxRegionService, PerformScanResponse>() {
				      ServerRpcController controller = new ServerRpcController();
				      BlockingRpcCallback<PerformScanResponse> rpcCallback = 
				        new BlockingRpcCallback<PerformScanResponse>();         

				      @Override
				      public PerformScanResponse call(TrxRegionService instance) throws IOException {        
				        org.apache.hadoop.hbase.coprocessor.transactional.generated.TrxRegionProtos.PerformScanRequest.Builder builder = PerformScanRequest.newBuilder();        
				        builder.setTransactionId(ts.getTransactionId());
				        builder.setRegionName(ByteString.copyFromUtf8(currentRegion.getRegionNameAsString()));
				        builder.setScannerId(scannerID);
				        builder.setNumberOfRows(nbRows);
                                        if (doNotCloseOnLast)
				          builder.setCloseScanner(false);
                                        else
				          builder.setCloseScanner(true);
				        builder.setNextCallSeq(nextCallSeqInput);

				        instance.performScan(controller, builder.build(), rpcCallback);
				        return rpcCallback.get();        
				      }
				    };
				 
				    Map<byte[], PerformScanResponse> presult = null;   
				    org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Result[]
				    results = null;


				    try {
				      presult = ttable.coprocessorService(TrxRegionService.class, currentBeginKey, currentEndKey, callable);
				    } catch (Throwable e) {
				    	LOG.error("ERROR when calling PerformScan coprocessor" + e.toString());
				    }

				      int count = 0;
				      boolean hasMore = false;

				      org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Result
				        result = null;
				            
				      for (PerformScanResponse presponse : presult.values())
				      {
				        if (presponse.getHasException())
				        {
				          String exception = presponse.getException();
				          if(LOG.isTraceEnabled()) LOG.trace("  PerformScanResponse exception " + exception );
				        }
				        else
				        {				        	
				          this.nextCallSeq = presponse.getNextCallSeq();
				          if(LOG.isTraceEnabled()) LOG.trace("next() nextCallSeq: " + this.nextCallSeq);	
				          count = presponse.getResultCount();
				          results = 
				            new org.apache.hadoop.hbase.protobuf.generated.ClientProtos.Result[count];

				          for (int i = 0; i < count; i++)
				          {
				            result = presponse.getResult(i);
				            if (result != null) {
				            	cache.add(ProtobufUtil.toResult(result));
				            }
				            hasMore = presponse.getHasMore();
				            results[i] = result;
				            result = null;
				            if(LOG.isTraceEnabled()) LOG.trace("  PerformScan response count " + count + ", hasMore is " + hasMore + ", result " + results[i] );
				          }
				        }
				      }
				      
				      if(!hasMore) {
				    	  if(LOG.isTraceEnabled()) LOG.trace("hasMore is false");
				    	  if(nextScanner(false) == true){
				    		  LOG.trace("nextScanner == true, continuing");
				    		  continue;
				    	  }
				    	  else {
				    		  if(LOG.isTraceEnabled()) LOG.trace("nextScanner == false, break");
				    		  break;
				    	  }
				    		  
				      }  
			}while (true); 

		}
		
		if (cache.size() > 0)  {
			return cache.poll();
		}
		return null;
	}


}
