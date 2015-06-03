﻿/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2015 Hewlett-Packard Development Company, L.P.
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
namespace HP.Data.ETL
{
    internal class GetParallelExtractInfoMessage: INetworkMessage
    {
        public int ServerStmtHandle;
        public int NumStreams;

        public void WriteToDataStream(DataStream ds)
        {
            ds.WriteInt32(this.ServerStmtHandle);
            ds.WriteInt32(this.NumStreams);
        }

        public int PrepareMessageParams(HPDbEncoder enc)
        {
            return 8;
        }
    }
}