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

import java.util.SortedMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.tableindexed.IndexSpecification;
import org.apache.hadoop.hbase.client.tableindexed.IndexedTable;
import org.apache.hadoop.hbase.util.Bytes;

/**
 * Singleton class for index maintence logic.
 */
public class IndexMaintenanceUtils {

    private static final Log LOG = LogFactory.getLog(IndexMaintenanceUtils.class);

    public static Put createIndexUpdate(final IndexSpecification indexSpec, final byte[] row,
            final SortedMap<byte[], byte[]> columnValues) {
        byte[] indexRow = indexSpec.getKeyGenerator().createIndexKey(row, columnValues);
        Put update = new Put(indexRow);

        update.add(IndexedTable.INDEX_COL_FAMILY, IndexedTable.INDEX_BASE_ROW, row);

        for (byte[] col : indexSpec.getIndexedColumns()) {
            byte[] val = columnValues.get(col);
            if (val == null) {
                throw new RuntimeException("Unexpected missing column value. [" + Bytes.toString(col) + "]");
            }
            byte[][] colSeperated = KeyValue.parseColumn(col);
            update.add(colSeperated[0], colSeperated[1], val);
        }

        for (byte[] col : indexSpec.getAdditionalColumns()) {
            byte[] val = columnValues.get(col);
            if (val != null) {
                byte[][] colSeperated = KeyValue.parseColumn(col);
                update.add(colSeperated[0], colSeperated[1], val);
            }
        }

        return update;
    }

    /**
     * Ask if this update does apply to the index.
     * 
     * @param indexSpec
     * @param columnValues
     * @return true if possibly apply.
     */
    public static boolean doesApplyToIndex(final IndexSpecification indexSpec,
            final SortedMap<byte[], byte[]> columnValues) {

        for (byte[] neededCol : indexSpec.getIndexedColumns()) {
            if (!columnValues.containsKey(neededCol)) {
                LOG.debug("Index [" + indexSpec.getIndexId() + "] can't be updated because ["
                        + Bytes.toString(neededCol) + "] is missing");
                return false;
            }
        }
        return true;
    }
}
