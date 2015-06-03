﻿/**********************************************************************
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

using System;

namespace HP.Data
{
    internal class CancelMessage: INetworkMessage
    {
        public int DialogueId;
        public int ServerType;
        public string ServerObjRef;
        public int StopType;

        private byte[] _serverObjRef;

        public void WriteToDataStream(DataStream ds)
        {
            ds.WriteInt32(DialogueId);
            ds.WriteInt32(ServerType);
            ds.WriteString(_serverObjRef);
            ds.WriteInt32(StopType);
        }

        public int PrepareMessageParams(HPDbEncoder enc)
        {
            int len = 16; //3*4 Int32, 1*4 string len

            _serverObjRef = enc.GetBytes(ServerObjRef, enc.Transport);
            if (_serverObjRef.Length > 0)
            {
                len += _serverObjRef.Length + 1;
            }

            return len;
        }
    }
}
