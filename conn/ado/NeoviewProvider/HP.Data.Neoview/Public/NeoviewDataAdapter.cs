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
    using System.Collections.Generic;
    using System.Data;
    using System.Data.Common;

    /// <summary>
    /// Represents a set of data commands and a database connection that are used to fill the DataSet and update a HPDb database. This class cannot be inherited.
    /// </summary>
    public sealed class HPDbDataAdapter : DbDataAdapter, IDbDataAdapter
    {
        private int _updateBatchSize;
        private List<HPDbCommand> _commandBatch;
        private List<Object[]> _batchedParams;

        /// <summary>
        /// Initializes a new instance of the HPDbDataAdapter class.
        /// </summary>
        public HPDbDataAdapter()
        {
            if (HPDbTrace.IsPublicEnabled)
            {
                HPDbTrace.Trace(null, TraceLevel.Public);
            }
            this._batchedParams = new List<object[]>();
            this._updateBatchSize = 1;
        }

        /// <summary>
        /// Initializes a new instance of the HPDbDataAdapter class with the specified HPDbCommand as the SelectCommand property.
        /// </summary>
        /// <param name="selectCommand">The HPDbCommand to set as the SelectCommand property.</param>
        public HPDbDataAdapter(HPDbCommand selectCommand)
        {
            if (HPDbTrace.IsPublicEnabled)
            {
                HPDbTrace.Trace(null, TraceLevel.Public, selectCommand);
            }

            this._batchedParams = new List<object[]>();
            this._updateBatchSize = 1;
            this.SelectCommand = selectCommand;
        }

        /* 
        public HPDbDataAdapter(string selectCommandText, string selectConnectionString)
        {
            
        }

        public HPDbDataAdapter(string selectCommandText, HPDbConnection selectConnection)
        {

        }
        */

        /// <summary>
        /// Occurs during Update before a command is executed against the data source. The attempt to update is made, so the event fires.
        /// </summary>
        public event HPDbRowUpdatingEventHandler RowUpdating;

        /// <summary>
        /// Occurs during Update after a command is executed against the data source. The attempt to update is made, so the event fires.
        /// </summary>
        public event HPDbRowUpdatedEventHandler RowUpdated;

        //these provide strong typing
        protected override RowUpdatedEventArgs CreateRowUpdatedEvent(DataRow dataRow, IDbCommand command, System.Data.StatementType statementType, DataTableMapping tableMapping)
        {
            return new HPDbRowUpdatedEventArgs(dataRow, command, statementType, tableMapping);
        }
        protected override RowUpdatingEventArgs CreateRowUpdatingEvent(DataRow dataRow, IDbCommand command, System.Data.StatementType statementType, DataTableMapping tableMapping)
        {
            return new HPDbRowUpdatingEventArgs(dataRow, command, statementType, tableMapping);
        }

        /// <summary>
        /// Overridden. Raises the RowUpdating event.
        /// </summary>
        /// <param name="value">A HPDbRowUpdatingEventArgs that contains the event data.</param>
        protected override void OnRowUpdating(RowUpdatingEventArgs value)
        {
            if (RowUpdating != null)
            {
                RowUpdating(this, (value as HPDbRowUpdatingEventArgs));
            }
            if (value.StatementType == System.Data.StatementType.Insert)
            {
                this._batchedParams.Add(value.Row.ItemArray);
            }
        }

        /// <summary>
		/// Overridden. Raises the RowUpdated event.
		/// </summary>
		/// <param name="value">A HPDbUpdatedEventArgs that contains the event data. </param>
        override protected void OnRowUpdated(RowUpdatedEventArgs value)
        {
            if (RowUpdated != null)
            {
                RowUpdated(this, (value as HPDbRowUpdatedEventArgs));
            }
            this._batchedParams.Clear();
        }

        public new HPDbCommand SelectCommand { get; set; }
        public new HPDbCommand InsertCommand { get; set; }
        public new HPDbCommand DeleteCommand { get; set; }
        public new HPDbCommand UpdateCommand { get; set; }

        /// <summary>
        /// Gets the parameters set by the user when executing an SQL SELECT statement.
        /// </summary>
        public new HPDbParameter[] GetFillParameters()
        {
            HPDbParameter[] parameters = new HPDbParameter[SelectCommand.Parameters.Count];
            for( int index = 0; index < SelectCommand.Parameters.Count; index++ )
            {
                parameters[index] = SelectCommand.Parameters[index];
            }

            return parameters;
        }

        /// <summary>
        /// Returns a HPDbParameter from one of the commands in the current batch.
        /// </summary>
        public new HPDbParameter GetBatchedParameter(int commandIdentifier, int parameterIndex)
        {
            return (HPDbParameter)_commandBatch[commandIdentifier].Parameters[parameterIndex];
        }

        public override int UpdateBatchSize
        {
            get
            {
                return this._updateBatchSize;
            }
            set
            {
                this._updateBatchSize = value;
            }
        }

        protected override void InitializeBatching()
        {
            if (HPDbTrace.IsInternalEnabled)
            {
                HPDbTrace.Trace(null, TraceLevel.Internal);
            }

            _commandBatch = new List<HPDbCommand>();
        }

        protected int AddToBatch(HPDbCommand command)
        {
            if (HPDbTrace.IsInternalEnabled)
            {
                HPDbTrace.Trace(null, TraceLevel.Internal);
            }

            _commandBatch.Add((HPDbCommand)((ICloneable)command).Clone());

            return _commandBatch.Count - 1;
        }

        protected override int AddToBatch(IDbCommand command)
        {
            return this.AddToBatch((HPDbCommand)command);
        }

        protected override int ExecuteBatch()
        {
            if (HPDbTrace.IsInternalEnabled)
            {
                HPDbTrace.Trace(null, TraceLevel.Internal);
            }

            int recordsAffected = 0;
            int index = 0;
            if (this.InsertCommand != null)
            {
                HPDbCommand command = this._commandBatch[0];
                for (int i = 0; i < this._batchedParams.Count; i++)
                {
                    command.AddBatch(this._batchedParams[i]);
                }

                recordsAffected = command.ExecuteNonQuery();
            }
            else if(this.SelectCommand != null)
            {
                while (index < _commandBatch.Count)
                {
                    HPDbCommand command = _commandBatch[index++];
                    recordsAffected += command.ExecuteNonQuery();
                }
            }

            return recordsAffected;
        }

        protected override void ClearBatch()
        {
            if (HPDbTrace.IsInternalEnabled)
            {
                HPDbTrace.Trace(null, TraceLevel.Internal);
            }

            _commandBatch.Clear();
        }

        protected override void TerminateBatching()
        {
            if (HPDbTrace.IsInternalEnabled)
            {
                HPDbTrace.Trace(null, TraceLevel.Internal);
            }

            this.ClearBatch();
            this._commandBatch = null;
        }

        protected override int Update(DataRow[] dataRows, DataTableMapping tableMapping)
        {
            if (this.UpdateCommand != null)
            {
                this.UpdateCommand.Prepare();
            }
            if (this.InsertCommand != null)
            {
                this.InsertCommand.Prepare();
            }
            if (this.DeleteCommand != null)
            {
                this.DeleteCommand.Prepare();
            }

            return base.Update(dataRows, tableMapping);
        }

        public new void Dispose()
        {
            base.Dispose();
        }

        IDbCommand IDbDataAdapter.SelectCommand
        {
            get { return this.SelectCommand; }
            set { this.SelectCommand = (HPDbCommand)value; }
        }

        IDbCommand IDbDataAdapter.InsertCommand
        {
            get { return this.InsertCommand; }
            set { this.InsertCommand = (HPDbCommand)value; }
        }

        IDbCommand IDbDataAdapter.DeleteCommand
        {
            get { return this.DeleteCommand; }
            set { this.DeleteCommand = (HPDbCommand)value; }
        }

        IDbCommand IDbDataAdapter.UpdateCommand
        {
            get { return this.UpdateCommand; }
            set { this.UpdateCommand = (HPDbCommand)value; }
        }

        IDataParameter[] IDataAdapter.GetFillParameters()
        {
            return this.GetFillParameters();
        }
    }
}
