// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1996-2014 Hewlett-Packard Development Company, L.P.
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

// -*-java-*-
// Filename :	SQLMXResultSetMetaData.java
//

package org.trafodion.jdbc.t2;

import java.sql.*;

public class SQLMXResultSetMetaData implements java.sql.ResultSetMetaData
{

	public String getCatalogName(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getCatalogName].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].catalogName_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getCatalogName].methodExit();
		}
	}
	
	public String getColumnClassName(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnClassName].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].getColumnClassName();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnClassName].methodExit();
		}
	}
	
	public int getColumnCount() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnCount].methodEntry();
		try
		{
			return outputDesc_.length;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnCount].methodExit();
		}
	}
 	
	public int getColumnDisplaySize(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnDisplaySize].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].displaySize_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnDisplaySize].methodExit();
		}
	}
 	
	public String getColumnLabel(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnLabel].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			
			// If the column heading/label is not set, return the column name.
			if (outputDesc_[column-1].columnLabel_.equals("")) 
				return outputDesc_[column-1].name_;
			else
				return outputDesc_[column-1].columnLabel_; 
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnLabel].methodExit();
		}
	}
	
	public String getColumnName(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnName].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].name_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnName].methodExit();
		}
	}
	
	public int getColumnType(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnType].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].dataType_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnType].methodExit();
		}
	}
 	
	public String getColumnTypeName(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getColumnTypeName].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].getColumnTypeName(connection_.locale_);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getColumnTypeName].methodExit();
		}
	}
	
	public int getPrecision(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getPrecision].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].precision_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getPrecision].methodExit();
		}
	}
 	
	public int getScale(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getScale].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].scale_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getScale].methodExit();
		}
	}
 	
	/* cpqGetCharacterName (extended) method added to allow SQLJ to */
	/* pull character data types from SQL/MX encoding info in the   */
	/* COLS table. Valid types are UCS2, ISO88591, or null.         */ 
	public String cpqGetCharacterSet(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_cpqGetCharacterSet].methodEntry();
		try
		{
			if ((column > outputDesc_.length) || (column <= 0))
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].getCharacterSetName();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_cpqGetCharacterSet].methodExit();
		}
	}

	public String getSchemaName(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getSchemaName].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].schemaName_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getSchemaName].methodExit();
		}
	}
	
	public String getTableName(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getTableName].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].tableName_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getTableName].methodExit();
		}
	}
	
	public boolean isAutoIncrement(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isAutoIncrement].methodEntry();
		try
		{
			if (column > outputDesc_.length)
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].isAutoIncrement_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isAutoIncrement].methodExit();
		}
	}
 	
	public boolean isCaseSensitive(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isCaseSensitive].methodEntry();
		try
		{
			if (column > outputDesc_.length) 
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].isCaseSensitive_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isCaseSensitive].methodExit();
		}
	}
 	
	public boolean isCurrency(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isCurrency].methodEntry();
		try
		{
			if (column > outputDesc_.length) 
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].isCurrency_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isCurrency].methodExit();
		}
	}
 	
	public boolean isDefinitelyWritable(int column) throws SQLException 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isDefinitelyWritable].methodEntry();
		try
		{
			return true;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isDefinitelyWritable].methodExit();
		}
	}
 	
	public int isNullable(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isNullable].methodEntry();
		try
		{
			if (column > outputDesc_.length) 
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].isNullable_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isNullable].methodExit();
		}
	}
 	
	public boolean isReadOnly(int column)  throws SQLException 
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isReadOnly].methodEntry();
		try
		{
			return false;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isReadOnly].methodExit();
		}
	}
 	
	public boolean isSearchable(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isSearchable].methodEntry();
		try
		{
			if (column > outputDesc_.length) 
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].isSearchable_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isSearchable].methodExit();
		}
	}
 	
	public boolean isSigned(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isSigned].methodEntry();
		try
		{
			if (column > outputDesc_.length) 
				throw Messages.createSQLException(connection_.locale_,"invalid_desc_index", null);
			return outputDesc_[column-1].isSigned_;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isSigned].methodExit();
		}
	}
 	
	public boolean isWritable(int column) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isWritable].methodEntry();
		try
		{
			return true;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isWritable].methodExit();
		}
	}

	// Constructors
	SQLMXResultSetMetaData(SQLMXStatement stmt, SQLMXDesc[] outputDesc)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXResultSetMetaData_LL_stmt].methodEntry();
		try
		{
			connection_ = stmt.connection_;
			outputDesc_ = outputDesc;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXResultSetMetaData_LL_stmt].methodExit();
		}
	}
  
	SQLMXResultSetMetaData(SQLMXResultSet resultSet, SQLMXDesc[] outputDesc)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXResultSetMetaData_LL_rs].methodEntry();
		try
		{
			resultSet_ = resultSet;
			connection_ = resultSet_.connection_;
			outputDesc_ = outputDesc;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_SQLMXResultSetMetaData_LL_rs].methodExit();
		}
	}

	// Fields
	SQLMXResultSet	resultSet_;
	SQLMXConnection	connection_;
	SQLMXDesc[]		outputDesc_;
	
	private static int methodId_getCatalogName					=  0;
	private static int methodId_getColumnClassName				=  1;
	private static int methodId_getColumnCount					=  2;
	private static int methodId_getColumnDisplaySize			=  3;
	private static int methodId_getColumnLabel					=  4;
	private static int methodId_getColumnName					=  5;
	private static int methodId_getColumnType					=  6;
	private static int methodId_getColumnTypeName				=  7;
	private static int methodId_getPrecision					=  8;
	private static int methodId_getScale						=  9;
	private static int methodId_cpqGetCharacterSet				= 10;
	private static int methodId_getSchemaName					= 11;
	private static int methodId_getTableName					= 12;
	private static int methodId_isAutoIncrement					= 13;
	private static int methodId_isCaseSensitive					= 14;
	private static int methodId_isCurrency						= 15;
	private static int methodId_isDefinitelyWritable			= 16;
	private static int methodId_isNullable						= 17;
	private static int methodId_isReadOnly						= 18;
	private static int methodId_isSearchable					= 19;
	private static int methodId_isSigned						= 20;
	private static int methodId_isWritable						= 21;
	private static int methodId_SQLMXResultSetMetaData_LL_stmt	= 22;
	private static int methodId_SQLMXResultSetMetaData_LL_rs	= 23;
	private static int totalMethodIds							= 24;
	private static JdbcDebug[] debug;
	
	static
	{
		String className = "SQLMXResultSetMetaData";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_getCatalogName] = new JdbcDebug(className,"getCatalogName"); 
			debug[methodId_getColumnClassName] = new JdbcDebug(className,"getColumnClassName"); 
			debug[methodId_getColumnCount] = new JdbcDebug(className,"getColumnCount"); 
			debug[methodId_getColumnDisplaySize] = new JdbcDebug(className,"getColumnDisplaySize"); 
			debug[methodId_getColumnLabel] = new JdbcDebug(className,"getColumnLabel"); 
			debug[methodId_getColumnName] = new JdbcDebug(className,"getColumnName"); 
			debug[methodId_getColumnType] = new JdbcDebug(className,"getColumnType"); 
			debug[methodId_getColumnTypeName] = new JdbcDebug(className,"getColumnTypeName"); 
			debug[methodId_getPrecision] = new JdbcDebug(className,"getPrecision"); 
			debug[methodId_getScale] = new JdbcDebug(className,"getScale"); 
			debug[methodId_cpqGetCharacterSet] = new JdbcDebug(className,"cpqGetCharacterSet"); 
			debug[methodId_getSchemaName] = new JdbcDebug(className,"getSchemaName"); 
			debug[methodId_getTableName] = new JdbcDebug(className,"getTableName"); 
			debug[methodId_isAutoIncrement] = new JdbcDebug(className,"isAutoIncrement"); 
			debug[methodId_isCaseSensitive] = new JdbcDebug(className,"isCaseSensitive"); 
			debug[methodId_isCurrency] = new JdbcDebug(className,"isCurrency"); 
			debug[methodId_isDefinitelyWritable] = new JdbcDebug(className,"isDefinitelyWritable"); 
			debug[methodId_isNullable] = new JdbcDebug(className,"isNullable"); 
			debug[methodId_isReadOnly] = new JdbcDebug(className,"isReadOnly"); 
			debug[methodId_isSearchable] = new JdbcDebug(className,"isSearchable"); 
			debug[methodId_isSigned] = new JdbcDebug(className,"isSigned"); 
			debug[methodId_isWritable] = new JdbcDebug(className,"isWritable"); 
			debug[methodId_SQLMXResultSetMetaData_LL_stmt] = new JdbcDebug(className,"SQLMXResultSetMetaData[LL_stmt]"); 
			debug[methodId_SQLMXResultSetMetaData_LL_rs] = new JdbcDebug(className,"SQLMXResultSetMetaData[LL_rs]"); 
		}
	}
	public Object unwrap(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isWrapperFor(Class iface) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}
    public int getSqlCharset(int column){
        System.out.println("sqlCharset_ :" + outputDesc_[column - 1].sqlCharset_);
        return outputDesc_[column - 1].sqlCharset_;
    }
    public int getOdbcCharset(int column){
        System.out.println("odbcCharset_ :" + outputDesc_[column - 1].odbcCharset_);
        return outputDesc_[column - 1].odbcCharset_;
    }
    public int getSqlDataType(int column){
        System.out.println("sqlDataType_ :" + outputDesc_[column - 1].sqlDataType_);
        return outputDesc_[column - 1].sqlDataType_;
    }
    public int getDataType(int column){
        System.out.println("dataType_ :" + outputDesc_[column - 1].dataType_);
        return outputDesc_[column - 1].dataType_;
    }
    public short getSqlPrecision(int column){
        System.out.println("sqlPrecision_ :" + outputDesc_[column - 1].sqlPrecision_);
        return outputDesc_[column - 1].sqlPrecision_;       
    }
    public int getOdbcPrecision(int column){
        System.out.println("odbcPrecision_ :" + outputDesc_[column - 1].odbcPrecision_);
        return outputDesc_[column - 1].odbcPrecision_;
    }
    public short getSqlDatetimeCode(int column){
        System.out.println("sqlDatetimeCode_ :" + outputDesc_[column - 1].sqlDatetimeCode_);
        return outputDesc_[column - 1].sqlDatetimeCode_;        
    }
    public int getSqlOctetLength(int column){
        System.out.println("sqlOctetLength_ :" + outputDesc_[column - 1].sqlOctetLength_);
        return outputDesc_[column - 1].sqlOctetLength_;     
    }
    public int getMaxLen(int column){
        System.out.println("maxLen_ :" + outputDesc_[column - 1].maxLen_);
        return outputDesc_[column - 1].maxLen_;     
    }
    public int getIsNullable(int column){
        System.out.println("isNullable_ :" + outputDesc_[column - 1].isNullable_);
        return outputDesc_[column - 1].isNullable_;     
    }
    public boolean getIsSigned(int column){
        System.out.println("isSigned_ :" + outputDesc_[column - 1].isSigned_);
        return outputDesc_[column - 1].isSigned_;       
    }
    public boolean getIsCurrency(int column){
        System.out.println("isCurrency_ :" + outputDesc_[column - 1].isCurrency_);
        return outputDesc_[column - 1].isCurrency_;     
    }
    public boolean getIsCaseSensitive(int column){
        System.out.println("isCaseSensitive_ :" + outputDesc_[column - 1].isCaseSensitive_);
        return outputDesc_[column - 1].isCaseSensitive_;        
    }
    public int getFsDataType(int column){
        System.out.println("fsDataType_ :" + outputDesc_[column - 1].fsDataType_);
        return outputDesc_[column - 1].fsDataType_;     
    }
    public int getIntLeadPrec(int column){
        System.out.println("intLeadPrec_ :" + outputDesc_[column - 1].intLeadPrec_);
        return outputDesc_[column - 1].intLeadPrec_;        
    }
    public int getMode(int column){
        System.out.println("paramMode_ :" + outputDesc_[column - 1].paramMode_);
        return outputDesc_[column - 1].paramMode_;      
    }
    public int getIndex(int column){
        System.out.println("paramIndex_ :" + outputDesc_[column - 1].paramIndex_);
        return outputDesc_[column - 1].paramIndex_;     
    }
    public int getPos(int column){
        System.out.println("paramPos_ :" + outputDesc_[column - 1].paramPos_);
        return outputDesc_[column - 1].paramPos_;       
    }
    public int getDisplaySize(int column){
        System.out.println("displaySize_ :" + outputDesc_[column - 1].displaySize_);
        return outputDesc_[column - 1].displaySize_;        
    }
/*
    public String getCatalogName(int column){
        System.out.println("catalogName_ :" + outputDesc_[column - 1].catalogName_);
        return inputDesc_[column - 1].catalogName_;
    }
    public String getSchemaName(int column){
        System.out.println("schemaName_ :" + outputDesc_[column - 1].schemaName_);
        return outputDesc_[column - 1].schemaName_;
    }
    public String getTableName(int param){
        System.out.println("tableName_ :" + outputDesc_[column - 1].tableName_);
        return outputDesc_[column - 1].tableName_;
    }
*/
    public String getName(int column){
        System.out.println("name_ :" + outputDesc_[column - 1].name_);
        return outputDesc_[column - 1].name_;
    }
    public String getLabel(int column){
        System.out.println("columnLabel_ :" + outputDesc_[column - 1].columnLabel_);
        return outputDesc_[column - 1].columnLabel_;
    }
    public String getClassName(int column){
        System.out.println("columnClassName_ :" + outputDesc_[column - 1].columnClassName_);
        return outputDesc_[column - 1].columnClassName_;
    }
//---------------------------------------------------------
}
