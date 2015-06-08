// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

// Generated from: common.health_header.proto
// Note: requires additional types generated from: common.info_header.proto
namespace common
{
  [global::System.Serializable, global::ProtoBuf.ProtoContract(Name=@"health_header")]
  public partial class health_header : global::ProtoBuf.IExtensible
  {
    public health_header() {}
    
    private common.info_header _header;
    [global::ProtoBuf.ProtoMember(1, IsRequired = true, Name=@"header", DataFormat = global::ProtoBuf.DataFormat.Default)]
    public common.info_header header
    {
      get { return _header; }
      set { _header = value; }
    }
    private int _publication_type;
    [global::ProtoBuf.ProtoMember(2, IsRequired = true, Name=@"publication_type", DataFormat = global::ProtoBuf.DataFormat.TwosComplement)]
    public int publication_type
    {
      get { return _publication_type; }
      set { _publication_type = value; }
    }

    private int _check_interval_sec = default(int);
    [global::ProtoBuf.ProtoMember(3, IsRequired = false, Name=@"check_interval_sec", DataFormat = global::ProtoBuf.DataFormat.TwosComplement)]
    [global::System.ComponentModel.DefaultValue(default(int))]
    public int check_interval_sec
    {
      get { return _check_interval_sec; }
      set { _check_interval_sec = value; }
    }

    private int _error = default(int);
    [global::ProtoBuf.ProtoMember(4, IsRequired = false, Name=@"error", DataFormat = global::ProtoBuf.DataFormat.TwosComplement)]
    [global::System.ComponentModel.DefaultValue(default(int))]
    public int error
    {
      get { return _error; }
      set { _error = value; }
    }

    private string _error_text = "";
    [global::ProtoBuf.ProtoMember(5, IsRequired = false, Name=@"error_text", DataFormat = global::ProtoBuf.DataFormat.Default)]
    [global::System.ComponentModel.DefaultValue("")]
    public string error_text
    {
      get { return _error_text; }
      set { _error_text = value; }
    }
    private global::ProtoBuf.IExtension extensionObject;
    global::ProtoBuf.IExtension global::ProtoBuf.IExtensible.GetExtensionObject(bool createIfMissing)
      { return global::ProtoBuf.Extensible.GetExtensionObject(ref extensionObject, createIfMissing); }
  }
  
}
