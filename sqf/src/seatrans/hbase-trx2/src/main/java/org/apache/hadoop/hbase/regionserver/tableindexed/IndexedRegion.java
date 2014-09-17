/**
 * Copyright 2009 The Apache Software Foundation Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements. See the NOTICE file distributed with this work for additional information regarding
 * copyright ownership. The ASF licenses this file to you under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and limitations under the
 * License.
 */
package org.apache.hadoop.hbase.regionserver.tableindexed;

import java.io.IOException;
import java.util.Collection;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.NavigableMap;
import java.util.NavigableSet;
import java.util.SortedMap;
import java.util.TreeMap;
import java.util.TreeSet;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HTableInterface;
import org.apache.hadoop.hbase.client.HTablePool;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.tableindexed.IndexSpecification;
import org.apache.hadoop.hbase.client.tableindexed.IndexedTableDescriptor;
import org.apache.hadoop.hbase.regionserver.OperationStatus;
import org.apache.hadoop.hbase.regionserver.RegionServerServices;
import org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegion;
import org.apache.hadoop.hbase.regionserver.wal.HLog;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.Pair;

public class IndexedRegion extends TransactionalRegion {

  private static final Log LOG = LogFactory.getLog(IndexedRegion.class);

  private final IndexedTableDescriptor indexTableDescriptor;
  private final HTablePool tablePool;

  private Configuration conf;
  public IndexedRegion(final Path basedir, final HLog log, final FileSystem fs,
	      final Configuration conf, final HRegionInfo regionInfo,
	      final HTableDescriptor htd, final RegionServerServices rsServices) throws IOException {
    super(basedir, log, fs, conf, regionInfo, htd, rsServices);
    this.indexTableDescriptor = new IndexedTableDescriptor(htd);
    this.conf = conf;
    this.tablePool = new HTablePool();
  }

  private HTableInterface getIndexTable(final IndexSpecification index)
      throws IOException {
    return tablePool.getTable(index.getIndexedTableName(super.getRegionInfo()
        .getTableDesc().getName()));
  }

  private void putTable(final HTableInterface t) {
    if (t == null) {
      return;
    }
    try {
		tablePool.putTable(t);
	} catch (IOException e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	}
  }

  private Collection<IndexSpecification> getIndexes() {
    return indexTableDescriptor.getIndexes();
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void put(final Put put, final Integer lockId, final boolean writeToWAL)
      throws IOException {
    updateIndexes(put, lockId); // Do this first because will want to see the
                                // old row
    super.put(put, lockId, writeToWAL);
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public OperationStatus[] put(Pair<Put, Integer>[] putsAndLocks)
      throws IOException {
    for (Pair<Put, Integer> pair : putsAndLocks) {
      updateIndexes(pair.getFirst(), pair.getSecond());
    }
    return super.put(putsAndLocks);
  }

  private void updateIndexes(final Put put, final Integer lockId)
      throws IOException {
    List<IndexSpecification> indexesToUpdate = new LinkedList<IndexSpecification>();

    // Find the indexes we need to update
    for (IndexSpecification index : getIndexes()) {
      if (possiblyAppliesToIndex(index, put)) {
        indexesToUpdate.add(index);
      }
    }

    if (indexesToUpdate.size() == 0) {
      return;
    }

    NavigableSet<byte[]> neededColumns = getColumnsForIndexes(indexesToUpdate);
    NavigableMap<byte[], byte[]> newColumnValues = getColumnsFromPut(put);

    Get oldGet = new Get(put.getRow());
    for (byte[] neededCol : neededColumns) {
      oldGet.addColumn(neededCol, Bytes.toBytes(""));
    }

    Result oldResult = super.get(oldGet, lockId);

    // Add the old values to the new if they are not there
    if (oldResult != null && oldResult.raw() != null) {
      for (KeyValue oldKV : oldResult.raw()) {
        byte[] column = KeyValue.makeColumn(oldKV.getFamily(),
            oldKV.getQualifier());
        if (!newColumnValues.containsKey(column)) {
          newColumnValues.put(column, oldKV.getValue());
        }
      }
    }

    Iterator<IndexSpecification> indexIterator = indexesToUpdate.iterator();
    while (indexIterator.hasNext()) {
      IndexSpecification indexSpec = indexIterator.next();
      if (!IndexMaintenanceUtils.doesApplyToIndex(indexSpec, newColumnValues)) {
        indexIterator.remove();
      }
    }

    SortedMap<byte[], byte[]> oldColumnValues = convertToValueMap(oldResult);

    for (IndexSpecification indexSpec : indexesToUpdate) {
      updateIndex(indexSpec, put, newColumnValues, oldColumnValues);
    }
  }

  // FIXME: This call takes place in an RPC, and requires an RPC. This makes for
  // a likely deadlock if the number of RPCs we are trying to serve is >= the
  // number of handler threads.
  private void updateIndex(final IndexSpecification indexSpec, final Put put,
      final NavigableMap<byte[], byte[]> newColumnValues,
      final SortedMap<byte[], byte[]> oldColumnValues) throws IOException {
    Delete indexDelete = makeDeleteToRemoveOldIndexEntry(indexSpec,
        put.getRow(), oldColumnValues);
    Put indexPut = makeIndexUpdate(indexSpec, put.getRow(), newColumnValues);

    HTableInterface indexTable = getIndexTable(indexSpec);
    try {
      if (indexDelete != null
          && !Bytes.equals(indexDelete.getRow(), indexPut.getRow())) {
        // Only do the delete if the row changed. This way we save the put after
        // delete issues in HBASE-2256
        LOG.debug("Deleting old index row ["
            + Bytes.toString(indexDelete.getRow()) + "]. New row is ["
            + Bytes.toString(indexPut.getRow()) + "].");
        indexTable.delete(indexDelete);
      } else if (indexDelete != null) {
        LOG.debug("Skipping deleting index row ["
            + Bytes.toString(indexDelete.getRow())
            + "] because it has not changed.");
      }
      indexTable.put(indexPut);
    } finally {
      putTable(indexTable);
    }
  }

  /** Return the columns needed for the update. */
  private NavigableSet<byte[]> getColumnsForIndexes(
      final Collection<IndexSpecification> indexes) {
    NavigableSet<byte[]> neededColumns = new TreeSet<byte[]>(
        Bytes.BYTES_COMPARATOR);
    for (IndexSpecification indexSpec : indexes) {
      for (byte[] col : indexSpec.getAllColumns()) {
        neededColumns.add(col);
      }
    }
    return neededColumns;
  }

  private Delete makeDeleteToRemoveOldIndexEntry(
      final IndexSpecification indexSpec, final byte[] row,
      final SortedMap<byte[], byte[]> oldColumnValues) throws IOException {
    for (byte[] indexedCol : indexSpec.getIndexedColumns()) {
      if (!oldColumnValues.containsKey(indexedCol)) {
        LOG.debug("Index [" + indexSpec.getIndexId()
            + "] not trying to remove old entry for row ["
            + Bytes.toString(row) + "] because col ["
            + Bytes.toString(indexedCol) + "] is missing");
        return null;
      }
    }

    byte[] oldIndexRow = indexSpec.getKeyGenerator().createIndexKey(row,
        oldColumnValues);
    LOG.debug("Index [" + indexSpec.getIndexId() + "] removing old entry ["
        + Bytes.toString(oldIndexRow) + "]");
    return new Delete(oldIndexRow);
  }

  private NavigableMap<byte[], byte[]> getColumnsFromPut(final Put put) {
    NavigableMap<byte[], byte[]> columnValues = new TreeMap<byte[], byte[]>(
        Bytes.BYTES_COMPARATOR);
    for (List<KeyValue> familyPuts : put.getFamilyMap().values()) {
      for (KeyValue kv : familyPuts) {
        columnValues.put(
            KeyValue.makeColumn(kv.getFamily(), kv.getQualifier()),
            kv.getValue());
      }
    }
    return columnValues;
  }

  /**
   * Ask if this put *could* apply to the index. It may actually apply if some
   * of the columns needed are missing.
   * 
   * @param indexSpec
   * @param put
   * @return true if possibly apply.
   */
  private boolean possiblyAppliesToIndex(final IndexSpecification indexSpec,
      final Put put) {
    for (List<KeyValue> familyPuts : put.getFamilyMap().values()) {
      for (KeyValue kv : familyPuts) {
        if (indexSpec.containsColumn(KeyValue.makeColumn(kv.getFamily(),
            kv.getQualifier()))) {
          return true;
        }
      }
    }
    return false;
  }

  private Put makeIndexUpdate(final IndexSpecification indexSpec,
      final byte[] row, final SortedMap<byte[], byte[]> columnValues)
      throws IOException {
    Put indexUpdate = IndexMaintenanceUtils.createIndexUpdate(indexSpec, row,
        columnValues);
    LOG.debug("Index [" + indexSpec.getIndexId() + "] adding new entry ["
        + Bytes.toString(indexUpdate.getRow()) + "] for row ["
        + Bytes.toString(row) + "]");

    return indexUpdate;

  }

  // FIXME we can be smarter about this and avoid the base gets and index
  // maintenance in many cases.
  @Override
  public void delete(final Delete delete, final Integer lockid,
      final boolean writeToWAL) throws IOException {
    // First look at the current (to be the old) state.
    SortedMap<byte[], byte[]> oldColumnValues = null;
    if (!getIndexes().isEmpty()) {
      // Need all columns
      NavigableSet<byte[]> neededColumns = getColumnsForIndexes(getIndexes());

      Get get = new Get(delete.getRow());
      for (byte[] col : neededColumns) {
        get.addColumn(col, Bytes.toBytes(""));
      }

      Result oldRow = super.get(get, lockid);
      oldColumnValues = convertToValueMap(oldRow);
    }

    super.delete(delete, lockid, writeToWAL);

    if (!getIndexes().isEmpty()) {
      Get get = new Get(delete.getRow());

      // Rebuild index if there is still a version visible.
      Result currentRow = super.get(get, lockid);
      SortedMap<byte[], byte[]> currentColumnValues = convertToValueMap(currentRow);

      for (IndexSpecification indexSpec : getIndexes()) {
        Delete indexDelete = null;
        if (IndexMaintenanceUtils.doesApplyToIndex(indexSpec, oldColumnValues)) {
          indexDelete = makeDeleteToRemoveOldIndexEntry(indexSpec,
              delete.getRow(), oldColumnValues);
        }
        Put indexPut = null;
        if (IndexMaintenanceUtils.doesApplyToIndex(indexSpec,
            currentColumnValues)) {
          indexPut = makeIndexUpdate(indexSpec, delete.getRow(),
              currentColumnValues);
        }
        if (indexPut == null && indexDelete == null) {
          continue;
        }

        HTableInterface indexTable = getIndexTable(indexSpec);
        try {
          if (indexDelete != null
              && (indexPut == null || !Bytes.equals(indexDelete.getRow(),
                  indexPut.getRow()))) {
            // Only do the delete if the row changed. This way we save the put
            // after delete issues in HBASE-2256
            LOG.debug("Deleting old index row ["
                + Bytes.toString(indexDelete.getRow()) + "].");
            indexTable.delete(indexDelete);
          } else if (indexDelete != null) {
            LOG.debug("Skipping deleting index row ["
                + Bytes.toString(indexDelete.getRow())
                + "] because it has not changed.");

            for (byte[] indexCol : indexSpec.getAdditionalColumns()) {
              byte[][] parsed = KeyValue.parseColumn(indexCol);
              List<KeyValue> famDeletes = delete.getFamilyMap().get(parsed[0]);
              if (famDeletes != null) {
                for (KeyValue kv : famDeletes) {
                  if (Bytes.equals(indexCol,
                      KeyValue.makeColumn(kv.getFamily(), kv.getQualifier()))) {
                    LOG.debug("Need to delete this specific column: "
                        + Bytes.toString(KeyValue.makeColumn(kv.getFamily(),
                            kv.getQualifier())));
                    Delete columnDelete = new Delete(indexDelete.getRow());
                    columnDelete.deleteColumns(kv.getFamily(),
                        kv.getQualifier());
                    indexTable.delete(columnDelete);
                  }
                }

              }
            }
          }

          if (indexPut != null) {
            indexTable.put(indexPut);
          }
        } finally {
          putTable(indexTable);
        }
      }
    }

  }

  private SortedMap<byte[], byte[]> convertToValueMap(final Result result) {
    SortedMap<byte[], byte[]> currentColumnValues = new TreeMap<byte[], byte[]>(
        Bytes.BYTES_COMPARATOR);

    if (result == null || result.raw() == null) {
      return currentColumnValues;
    }
    List<KeyValue> list = result.list();
    if (list != null) {
      for (KeyValue kv : result.list()) {
        currentColumnValues.put(
            KeyValue.makeColumn(kv.getFamily(), kv.getQualifier()),
            kv.getValue());
      }
    }
    return currentColumnValues;
  }
}
