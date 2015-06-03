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
    using System.Collections;
    using System.Collections.Generic;

    /// <summary>
    /// Collects information relevant to a warning or error returned by HPDb.
    /// </summary>
    public sealed class HPDbError
    {
        private int _errorCode;
        private string _message;
        private int _rowId;
        private string _state;

        internal HPDbError()
        {
        }

        /// <summary>
        /// Gets a number that identifies the type of error.
        /// </summary>
        public int ErrorCode 
        {
            get
            {
                return this._errorCode;
            }

            internal set
            {
                this._errorCode = value;
            }
        }

        /// <summary>
        /// Gets the text describing the error.
        /// </summary>
        public string Message
        {
            get
            {
                return this._message;
            }

            internal set
            {
                this._message = value;
            }
        }

        /// <summary>
        /// Gets the row id associated with this error.
        /// </summary>
        public int RowId
        {
            get
            {
                return this._rowId;
            }

            internal set
            {
                this._rowId = value;
            }
        }

        /// <summary>
        /// Gets the SQL State associated with this error.
        /// </summary>
        public string State
        {
            get
            {
                return this._state;
            }

            internal set
            {
                this._state = value;
            }
        }

        /// <summary>
        /// Gets the text of the error message. 
        /// </summary>
        /// <returns>The message text.</returns>
        public override string ToString()
        {
            return this._message;
        }
    }

    /// <summary>
    /// A collection of HPDbErrors.
    /// </summary>
    public sealed class HPDbErrorCollection : ICollection, IEnumerable
    {
        private List<HPDbError> _errors;
        private object _syncObject;

        internal HPDbErrorCollection()
        {
            this._syncObject = new object();
            this._errors = new List<HPDbError>();
        }

        /// <summary>
        /// Gets the number of errors in the collection.
        /// </summary>
        public int Count
        {
            get { return this._errors.Count; }
        }

        /// <summary>
        /// Gets a value indicating whether access to the HPDbErrorCollection is synchronized (thread safe).
        /// </summary>
        public bool IsSynchronized
        {
            get { return true; }
        }

        /// <summary>
        /// Gets an object that can be used to synchronize access to the HPDbErrorCollection.
        /// </summary>
        public object SyncRoot
        {
            get { return this._syncObject; }
        }

        /// <summary>
        /// Gets the HPDbError at the given ordinal.
        /// </summary>
        /// <param name="ordinal">The zero-based ordinal.</param>
        /// <returns>The HPDbError at the given ordinal.</returns>
        public HPDbError this[int ordinal]
        {
            get { return this._errors[ordinal]; }
        }

        /// <summary>
        /// Copies the elements of the HPDbErrorCollection collection into an Array, starting at the specified index.
        /// </summary>
        /// <param name="array">The Array to copy elements into. </param>
        /// <param name="index">The index from which to start copying into the array parameter. </param>
        public void CopyTo(Array array, int index)
        {
            this._errors.CopyTo((HPDbError[])array, index);
        }

        /// <summary>
        /// Returns an enumerator that iterates through the HPDbErrorCollection.
        /// </summary>
        /// <returns>An enumerator that iterates through the HPDbErrorCollection.</returns>
        public IEnumerator GetEnumerator()
        {
            return this._errors.GetEnumerator();
        }

        internal void Add(HPDbError error)
        {
            this._errors.Add(error);
        }
    }
}