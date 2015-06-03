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
    using System.Data;
    using System.Data.Common;

    /// <summary>
    /// Represents a collection of parameters associated with a HPDbCommand and their respective mappings to columns in a DataSet. This class cannot be inherited.
    /// </summary>
    public sealed class HPDbParameterCollection : DbParameterCollection, IDataParameterCollection
    {
        private HPDbCommand _cmd;
        private List<HPDbParameter> _parameters;
        private object _syncObject;

        internal HPDbParameterCollection(HPDbCommand cmd)
        {
            this._parameters = new List<HPDbParameter>();
            this._syncObject = new object();

            this._cmd = cmd;
        }

        /// <summary>
        /// Returns an Integer that contains the number of elements in the HPDbParameterCollection. Read-only.
        /// </summary>
        public override int Count
        {
            get { return this._parameters.Count; }
        }

        /// <summary>
        /// Gets a value that indicates whether the HPDbParameterCollection has a fixed size.
        /// </summary>
        public override bool IsFixedSize
        {
            get { return false; }
        }

        /// <summary>
        /// Gets a value that indicates whether the HPDbParameterCollection is read-only.
        /// </summary>
        public override bool IsReadOnly
        {
            get { return false; }
        }

        /// <summary>
        /// Gets a value that indicates whether the HPDbParameterCollection is synchronized.
        /// </summary>
        public override bool IsSynchronized
        {
            get { return false; }
        }

        /// <summary>
        /// Gets an object that can be used to synchronize access to the HPDbParameterCollection.
        /// </summary>
        public override object SyncRoot
        {
            get { return this._syncObject; }
        }

        internal List<HPDbParameter> Parameters
        {
            get { return this._parameters; }
        }

        /// <summary>
        /// Gets the HPDbParameter with the specified name.
        /// </summary>
        /// <param name="parameterName">The name of the parameter to retrieve. </param>
        /// <returns>The HPDbParameter with the specified name.</returns>
        /// <exception cref="IndexOutOfRangeException">The specified parameterName is not valid.</exception>
        public new HPDbParameter this[string parameterName]
        {
            get
            {
                return this._parameters[this.IndexOf(parameterName)];
            }

            set
            {
                this._parameters[this.IndexOf(parameterName)] = value;
                this._cmd.IsPrepared = false;
            }
        }

        /// <summary>
        /// Gets the HPDbParameter at the specified index.
        /// </summary>
        /// <param name="index">The zero-based index of the parameter to retrieve.</param>
        /// <returns>The HPDbParameter at the specified index.</returns>
        /// <exception cref="IndexOutOfRangeException">The specified index does not exist.</exception>
        public new HPDbParameter this[int index]
        {
            get
            {
                return this._parameters[index];
            }

            set
            {
                this._parameters[index] = value;
                this._cmd.IsPrepared = false;
            }
        }

        /// <summary>
        /// Adds the specified HPDbParameter object to the HPDbParameterCollection.
        /// </summary>
        /// <param name="value">The HPDbParameter to add to the collection. </param>
        /// <returns>The index of the new HPDbParameter object.</returns>
        /// <exception cref="InvalidCastException">The parameter passed was not a HPDbParameter.</exception>
        /// <exception cref="ArgumentNullException">The value parameter is null.</exception>
        public override int Add(object value)
        {
            if (HPDbTrace.IsPublicEnabled)
            {
                HPDbTrace.Trace(this._cmd.Connection, TraceLevel.Public, value);
            }

            if (value == null)
            {
                HPDbException.ThrowException(null, new ArgumentNullException("value"));
            }

            HPDbParameter param = (HPDbParameter)value;

            this._parameters.Add(param);
            this._cmd.IsPrepared = false;

            return this._parameters.IndexOf(param);
        }

        /// <summary>
        /// Adds an Enumerable Collection of HPDbParameter objects to the end of the HPDbParameterCollection.
        /// </summary>
        /// <param name="values">The Enumerable Collection of HPDbParameter objects to add.</param>
        /// <exception cref="InvalidCastException">The parameter passed was not an Enumerable Collection of HPDbParameter objects.</exception>
        /// <exception cref="ArgumentNullException">The values parameter is null.</exception>
        public override void AddRange(Array values)
        {
            if (HPDbTrace.IsPublicEnabled)
            {
                HPDbTrace.Trace(this._cmd.Connection, TraceLevel.Public, values);
            }

            if (values == null)
            {
                HPDbException.ThrowException(null, new ArgumentNullException("values"));
            }

            foreach (HPDbParameter p in (IEnumerable<HPDbParameter>)values)
            {
                this.Add(p);
            }
        }

        /// <summary>
        /// Removes all the HPDbParameter objects from the HPDbParameterCollection.
        /// </summary>
        public override void Clear()
        {
            if (HPDbTrace.IsPublicEnabled)
            {
                HPDbTrace.Trace(this._cmd.Connection, TraceLevel.Public);
            }

            this._parameters.Clear();
            this._cmd.IsPrepared = false;
        }

        /// <summary>
        /// Gets a value indicating whether a HPDbParameter in this HPDbParameterCollection has the specified name.
        /// </summary>
        /// <param name="value">The name of the HPDbParameter. </param>
        /// <returns>true if the HPDbParameterCollections contains the HPDbParameter; otherwise false.</returns>
        public override bool Contains(string value)
        {
            return this.IndexOf(value) != -1;
        }

        /// <summary>
        /// Determines whether the specified HPDbParameter is in this HPDbParameterCollection.
        /// </summary>
        /// <param name="value">The HPDbParameter value.</param>
        /// <returns>true if the HPDbParameterCollections contains the HPDbParameter; otherwise false.</returns>
        /// <exception cref="InvalidCastException">The parameter passed was not a HPDbParameter.</exception>
        public override bool Contains(object value)
        {
            return this._parameters.Contains((HPDbParameter)value);
        }

        /// <summary>
        /// Copies all the elements of the current HPDbParameterCollection to the specified HPDbParameterCollection starting at the specified destination index.
        /// </summary>
        /// <param name="array">The HPDbParameterCollection that is the destination of the elements copied from the current HPDbParameterCollection.</param>
        /// <param name="index">A 32-bit integer that represents the index in the HPDbParameterCollection at which copying starts.</param>
        /// <exception cref="InvalidCastException">The parameter passed was not a HPDbParameterCollection.</exception>
        public override void CopyTo(Array array, int index)
        {
            this._parameters.CopyTo((HPDbParameter[])array, index);
        }

        /// <summary>
        /// Returns an enumerator that iterates through the HPDbParameterCollection.
        /// </summary>
        /// <returns>An IEnumerator for the HPDbParameterCollection.</returns>
        public override IEnumerator GetEnumerator()
        {
            return this._parameters.GetEnumerator();
        }

        /// <summary>
        /// Gets the location of the specified HPDbParameter with the specified name.
        /// </summary>
        /// <param name="parameterName">The case-sensitive name of the HPDbParameter to find.</param>
        /// <returns>The zero-based location of the specified HPDbParameter with the specified case-sensitive name. Returns -1 when the object does not exist in the HPDbParameterCollection.</returns>
        public override int IndexOf(string parameterName)
        {
            for (int i = 0; i < this._parameters.Count; i++)
            {
                if (this._parameters[i].ParameterName.Equals(parameterName))
                {
                    return i;
                }
            }

            return -1;
        }

        /// <summary>
        /// Gets the location of the specified HPDbParameter within the collection.
        /// </summary>
        /// <param name="value">The HPDbParameter to find.</param>
        /// <returns>The zero-based location of the specified HPDbParameter that is a HPDbParameter within the collection. Returns -1 when the object does not exist in the HPDbParameterCollection.</returns>
        /// <exception cref="InvalidCastException">The parameter passed was not a HPDbParameter.</exception>
        public override int IndexOf(object value)
        {
            return this.IndexOf(((HPDbParameter)value).ParameterName);
        }

        /// <summary>
        /// Inserts a HPDbParameter object into the HPDbParameterCollection at the specified index.
        /// </summary>
        /// <param name="index">The zero-based index at which value should be inserted.</param>
        /// <param name="value">A HPDbParameter object to be inserted in the HPDbParameterCollection.</param>
        /// <exception cref="InvalidCastException">The parameter passed was not a HPDbParameter.</exception>
        public override void Insert(int index, object value)
        {
            if (HPDbTrace.IsPublicEnabled)
            {
                HPDbTrace.Trace(this._cmd.Connection, TraceLevel.Public, index, value);
            }

            this._parameters.Insert(index, (HPDbParameter)value);
            this._cmd.IsPrepared = false;
        }

        /// <summary>
        /// Removes the specified HPDbParameter from the collection.
        /// </summary>
        /// <param name="value">A HPDbParameter object to remove from the collection.</param>
        /// <exception cref="InvalidCastException">The parameter passed was not a HPDbParameter.</exception>
        public override void Remove(object value)
        {
            if (HPDbTrace.IsPublicEnabled)
            {
                HPDbTrace.Trace(this._cmd.Connection, TraceLevel.Public, value);
            }

            HPDbParameter param = (HPDbParameter)value;
            if (this._parameters.Remove(param))
            {
                this._cmd.IsPrepared = false;
            }
        }

        /// <summary>
        /// Removes the HPDbParameter from the HPDbParameterCollection at the specified parameter name.
        /// </summary>
        /// <param name="parameterName">The name of the HPDbParameter to remove.</param>
        /// <exception cref="ArgumentOutOfRangeException">The specified index does not exist.</exception>
        public override void RemoveAt(string parameterName)
        {
            if (HPDbTrace.IsPublicEnabled)
            {
                HPDbTrace.Trace(this._cmd.Connection, TraceLevel.Public, parameterName);
            }

            this._parameters.RemoveAt(this.IndexOf(parameterName));
            this._cmd.IsPrepared = false;
        }

        /// <summary>
        /// Removes the HPDbParameter from the HPDbParameterCollection at the specified index.
        /// </summary>
        /// <param name="index">The zero-based index of the HPDbParameter object to remove.</param>
        /// <exception cref="ArgumentOutOfRangeException">The specified index does not exist.</exception>
        public override void RemoveAt(int index)
        {
            if (HPDbTrace.IsPublicEnabled)
            {
                HPDbTrace.Trace(this._cmd.Connection, TraceLevel.Public, index);
            }

            this._parameters.RemoveAt(index);
            this._cmd.IsPrepared = false;
        }

        internal void Prepare(Descriptor[] desc)
        {
            if (HPDbTrace.IsInternalEnabled)
            {
                HPDbTrace.Trace(this._cmd.Connection, TraceLevel.Internal);
            }

            if (this._parameters.Count != desc.Length && this._cmd.isRWRS == false)
            {
                string msg = HPDbResources.FormatMessage(HPDbMessage.ParameterCountMismatch);
                HPDbException.ThrowException(this._cmd.Connection, new InvalidOperationException(msg));
            }
            int idx = 0;
            for (int i = 0; i < desc.Length; i++)
            {
                if (this._cmd.isRWRS && i < 3)
                {
                    idx = 3;
                    continue;
                }
                this._parameters[i - idx].Descriptor = desc[i];
            }
        }

        /// <summary>
        /// Returns DbParameter the object with the specified name.
        /// </summary>
        /// <param name="parameterName">The name of the DbParameter in the collection.</param>
        /// <returns>The DbParameter the object with the specified name.</returns>
        protected override DbParameter GetParameter(string parameterName)
        {
            return this[parameterName];
        }

        /// <summary>
        /// Returns the DbParameter object at the specified index in the collection.
        /// </summary>
        /// <param name="index">The index of the DbParameter in the collection.</param>
        /// <returns>The DbParameter object at the specified index in the collection.</returns>
        protected override DbParameter GetParameter(int index)
        {
            return this[index];
        }

        /// <summary>
        /// Sets the DbParameter object with the specified name to a new value.
        /// </summary>
        /// <param name="parameterName">The name of the DbParameter object in the collection.</param>
        /// <param name="value">The new DbParameter value.</param>
        protected override void SetParameter(string parameterName, DbParameter value)
        {
            this[parameterName] = (HPDbParameter)value;
        }

        /// <summary>
        /// Sets the DbParameter object at the specified index to a new value.
        /// </summary>
        /// <param name="index">The index where the DbParameter object is located.</param>
        /// <param name="value">The new DbParameter value.</param>
        protected override void SetParameter(int index, DbParameter value)
        {
            this[index] = (HPDbParameter)value;
        }
    }
}