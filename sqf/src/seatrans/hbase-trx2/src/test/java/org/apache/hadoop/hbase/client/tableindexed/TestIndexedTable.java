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
package org.apache.hadoop.hbase.client.tableindexed;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import junit.framework.Assert;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.HBaseTestingUtility;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.PerformanceEvaluation;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.RowLock;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.test.HBaseTrxTestUtil;
import org.apache.hadoop.hbase.util.Bytes;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

public class TestIndexedTable {

    private static final Log LOG = LogFactory.getLog(TestIndexedTable.class);
    private final static HBaseTestingUtility TEST_UTIL = new HBaseTestingUtility();

    private static final String TABLE_NAME = "table1";

    private static final byte[] FAMILY = Bytes.toBytes("family");
    private static final byte[] QUAL_A = Bytes.toBytes("a");
    private static final byte[] COL_A = Bytes.toBytes("family:a");
    private static final String INDEX_COL_A = "A";

    private static final int NUM_ROWS = 10;
    private static final int MAX_VAL = 10000;

    private static IndexedTableAdmin admin;
    private static IndexedTable table;
    private Random random = new Random();
    private static HTableDescriptor desc;

    /**
     * @throws java.lang.Exception
     */
    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        HBaseTrxTestUtil.configureForIndexingAndTransactions(TEST_UTIL.getConfiguration());

        TEST_UTIL.startMiniCluster(3);
        setupTables();
    }

    /**
     * @throws java.lang.Exception
     */
    @AfterClass
    public static void tearDownAfterClass() throws Exception {
        TEST_UTIL.shutdownMiniCluster();
    }

    /**
     * @throws java.lang.Exception
     */

    public static void setupTables() throws Exception {

        desc = new HTableDescriptor(TABLE_NAME);
        desc.addFamily(new HColumnDescriptor(FAMILY));

        IndexedTableDescriptor indexDesc = new IndexedTableDescriptor(desc);
        // Create a new index that does lexicographic ordering on COL_A
        IndexSpecification colAIndex = new IndexSpecification(INDEX_COL_A, COL_A);
        indexDesc.addIndex(colAIndex);

        admin = new IndexedTableAdmin(TEST_UTIL.getConfiguration());
        admin.setBatchSize(NUM_ROWS / 3); // To fully test re-indexing
        admin.createIndexedTable(indexDesc);
        table = new IndexedTable(TEST_UTIL.getConfiguration(), desc.getName());
    }

    @Test
    public void testInitialWrites() throws IOException {
        writeInitalRows();
        assertRowsInOrder(NUM_ROWS);
    }

    @Test
    public void testMultipleWrites() throws IOException {
        writeInitalRows();
        writeInitalRows(); // Update the rows.
        writeRowsMultiPut(); // Update the rows.
        assertRowsInOrder(NUM_ROWS);
    }

    @Test
    public void testRemoveAddReIndex() throws IOException {
        IndexSpecification index = table.getIndexedTableDescriptor().getIndexes().iterator().next();
        admin.removeIndex(table.getTableName(), index.getIndexId());

        Assert.assertFalse(admin.tableExists(index.getIndexedTableName(table.getTableName())));

        admin.addIndex(table.getTableName(), index);

        assertRowsInOrder(NUM_ROWS);
    }

    @Test
    public void testDelete() throws IOException {
        writeInitalRows();
        // Delete the first row;
        table.delete(new Delete(PerformanceEvaluation.format(0)));

        assertRowsInOrder(NUM_ROWS - 1);
    }

    @Test
    public void testRowUpdate() throws IOException {
        writeInitalRows();
        int row = NUM_ROWS - 2;
        int value = MAX_VAL + 111;
        updateRow(row, value);
        assertRowUpdated(row, value);
    }

    @Test
    public void testLockedRowUpdate() throws IOException {
        writeInitalRows();
        int row = NUM_ROWS - 2;
        int value = MAX_VAL + 111;
        updateLockedRow(row, value);
        assertRowUpdated(row, value);
    }

    @Test
    public void testLockedRowUpdateNoAutoFlush() throws IOException {
        writeInitalRows();
        int row = NUM_ROWS - 4;
        int value = MAX_VAL + 2222;
        updateLockedRowNoAutoFlush(row, value);
        assertRowUpdated(row, value);
    }

    @Test
    public void testLockedRowDelete() throws IOException {
        writeInitalRows();
        // Delete the first row;
        byte[] row = PerformanceEvaluation.format(0);
        RowLock lock = table.lockRow(row);
        table.delete(new Delete(row, HConstants.LATEST_TIMESTAMP, lock));
        table.unlockRow(lock);

        assertRowDeleted(NUM_ROWS - 1);
    }

    private void writeInitalRows() throws IOException {
        for (int i = 0; i < NUM_ROWS; i++) {
            Put update = new Put(PerformanceEvaluation.format(i));
            byte[] valueA = PerformanceEvaluation.format(random.nextInt(MAX_VAL));
            update.add(FAMILY, QUAL_A, valueA);
            table.put(update);
            LOG.info("Inserted row [" + Bytes.toString(update.getRow()) + "] val: [" + Bytes.toString(valueA) + "]");
        }
    }

    private void writeRowsMultiPut() throws IOException {
        List<Put> puts = new ArrayList<Put>(NUM_ROWS);
        for (int i = 0; i < NUM_ROWS; i++) {
            Put update = new Put(PerformanceEvaluation.format(i));
            byte[] valueA = PerformanceEvaluation.format(random.nextInt(MAX_VAL));
            update.add(FAMILY, QUAL_A, valueA);
            puts.add(update);
            LOG.info("Inserted row [" + Bytes.toString(update.getRow()) + "] val: [" + Bytes.toString(valueA) + "]");
        }

        table.put(puts);
    }

    private void assertRowsInOrder(final int numRowsExpected) throws IndexNotFoundException, IOException {
        ResultScanner scanner = table.getIndexedScanner(INDEX_COL_A, null, null, null, null, null);
        int numRows = 0;
        byte[] lastColA = null;
        for (Result rowResult : scanner) {
            byte[] colA = rowResult.getValue(FAMILY, QUAL_A);
            LOG.info("index scan : row [" + Bytes.toString(rowResult.getRow()) + "] value [" + Bytes.toString(colA)
                    + "]");
            if (lastColA != null) {
                Assert.assertTrue(Bytes.compareTo(lastColA, colA) <= 0);
            }
            lastColA = colA;
            numRows++;
        }
        scanner.close();
        Assert.assertEquals(numRowsExpected, numRows);
    }

    private void assertRowUpdated(final int updatedRow, final int expectedRowValue) throws IndexNotFoundException,
            IOException {
        ResultScanner scanner = table.getIndexedScanner(INDEX_COL_A, null, null, null, null, null);
        byte[] persistedRowValue = null;
        for (Result rowResult : scanner) {
            byte[] row = rowResult.getRow();
            byte[] value = rowResult.getValue(FAMILY, QUAL_A);
            if (Bytes.toString(row).equals(Bytes.toString(PerformanceEvaluation.format(updatedRow)))) {
                persistedRowValue = value;
                LOG.info("update found: row [" + Bytes.toString(row) + "] value [" + Bytes.toString(value) + "]");
            } else {
                LOG.info("updated index scan : row [" + Bytes.toString(row) + "] value [" + Bytes.toString(value) + "]");
            }
        }
        scanner.close();

        Assert.assertEquals(Bytes.toString(PerformanceEvaluation.format(expectedRowValue)),
            Bytes.toString(persistedRowValue));
    }

    private void assertRowDeleted(final int numRowsExpected) throws IndexNotFoundException, IOException {
        // Check the size of the primary table
        ResultScanner scanner = table.getScanner(new Scan());
        int numRows = 0;
        for (Result rowResult : scanner) {
            byte[] colA = rowResult.getValue(FAMILY, QUAL_A);
            LOG.info("primary scan : row [" + Bytes.toString(rowResult.getRow()) + "] value [" + Bytes.toString(colA)
                    + "]");
            numRows++;
        }
        scanner.close();
        Assert.assertEquals(numRowsExpected, numRows);

        // Check the size of the index tables
        assertRowsInOrder(numRowsExpected);
    }

    private void updateRow(final int row, final int newValue) throws IOException {
        Put update = new Put(PerformanceEvaluation.format(row));
        byte[] valueA = PerformanceEvaluation.format(newValue);
        update.add(FAMILY, QUAL_A, valueA);
        table.put(update);
        LOG.info("Updated row [" + Bytes.toString(update.getRow()) + "] val: [" + Bytes.toString(valueA) + "]");
    }

    private void updateLockedRow(final int row, final int newValue) throws IOException {
        RowLock lock = table.lockRow(PerformanceEvaluation.format(row));
        Put update = new Put(PerformanceEvaluation.format(row), lock);
        byte[] valueA = PerformanceEvaluation.format(newValue);
        update.add(FAMILY, QUAL_A, valueA);
        LOG.info("Updating row [" + Bytes.toString(update.getRow()) + "] val: [" + Bytes.toString(valueA) + "]");
        table.put(update);
        LOG.info("Updated row [" + Bytes.toString(update.getRow()) + "] val: [" + Bytes.toString(valueA) + "]");
        table.unlockRow(lock);
    }

    private void updateLockedRowNoAutoFlush(final int row, final int newValue) throws IOException {
        table.flushCommits();
        table.setAutoFlush(false);
        RowLock lock = table.lockRow(PerformanceEvaluation.format(row));
        Put update = new Put(PerformanceEvaluation.format(row), lock);
        byte[] valueA = PerformanceEvaluation.format(newValue);
        update.add(FAMILY, QUAL_A, valueA);
        LOG.info("Updating row [" + Bytes.toString(update.getRow()) + "] val: [" + Bytes.toString(valueA) + "]");
        table.put(update);
        LOG.info("Updated row [" + Bytes.toString(update.getRow()) + "] val: [" + Bytes.toString(valueA) + "]");
        table.flushCommits();
        table.close();
        table = new IndexedTable(TEST_UTIL.getConfiguration(), desc.getName());
    }
}
