/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett-Packard Development Company, L.P.
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
********************************************************************/

namespace HP.Data
{
    using System;
    using System.Data.Common;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Represents a set of methods for creating instances of the <c>HP.Data</c> provider's implementation of the data source classes.
    /// </summary>
    /// //[Guid("18E26E64-50F9-4ba3-8530-DBBD117B1DCD")]
    [Guid("CD57E4B4-13EC-4FFF-87F9-34F743D8EE7D")]
    public sealed class HPDbFactory : DbProviderFactory
    {
        /// <summary>
        /// Gets an instance of the HPDbFactory. This can be used to retrieve strongly typed data objects.
        /// </summary>
        public static readonly HPDbFactory Instance = new HPDbFactory();

        /// <summary>
        /// Returns <c>false</c>.  <c>CreateDataSourceEnumerator()</c> is not supported
        /// </summary>
        public override bool CanCreateDataSourceEnumerator 
        { 
            get { return false; }
        }

        /// <summary>
        /// Creates a new <c>HPDbCommand</c> object.
        /// </summary>
        /// <returns>A new <c>HPDbCommand</c> object.</returns>
        public override DbCommand CreateCommand()
        {
            return new HPDbCommand();
        }

        /// <summary>
        /// Creates a new <c>HPDbCommandBuilder</c> object.
        /// </summary>
        /// <returns>A new <c>HPDbCommandBuilder</c> object.</returns>
        public override DbCommandBuilder CreateCommandBuilder()
        {
            return new HPDbCommandBuilder();
        }

        /// <summary>
        /// Creates a new <c>HPDbConnection</c> object.
        /// </summary>
        /// <returns>A new <c>HPDbConnection</c> object.</returns>
        public override DbConnection CreateConnection()
        {
            return new HPDbConnection();
        }

        /// <summary>
        /// Creates a new <c>HPDbConnectionStringBuilder</c> object.
        /// </summary>
        /// <returns>A new <c>HPDbConnectionStringBuilder</c> object.</returns>
        public override DbConnectionStringBuilder CreateConnectionStringBuilder()
        {
            return new HPDbConnectionStringBuilder();
        }

        /// <summary>
        /// Creates a new <c>HPDbDataAdapter</c> object.
        /// </summary>
        /// <returns>A new <c>HPDbDataAdapter</c> object.</returns>
        public override DbDataAdapter CreateDataAdapter()
        {
            return new HPDbDataAdapter();
        }

        /// <summary>
        /// Not supported.
        /// </summary>
        /// <returns>Not Applicable</returns>
        /// <exception cref="NotSupportedException">Always thrown.</exception>
        public override DbDataSourceEnumerator CreateDataSourceEnumerator()
        {
            throw new NotSupportedException();
        }

        /// <summary>
        /// Creates a new <c>HPDbParameter</c> object.
        /// </summary>
        /// <returns>A new <c>HPDbParameter</c> object.</returns>
        public override DbParameter CreateParameter()
        {
            return new HPDbParameter();
        }
    }
}