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
#ifndef ODBCCOMMON_H_
#define ODBCCOMMON_H_
/*
 * Translation unit: ODBCCOMMON
 * Generated by CNPGEN(TANTAU CNPGEN TANTAU_AG_PC8 20001120.103031) on Mon Jan 31 11:14:07 2011
 * C++ constructs used
 * Header file for use with the CEE
 * Client functionality included
 * Server functionality included
 */
#include <stdarg.h>
#include <cee.h>
#if CEE_H_VERSION != 19991123
#error Version mismatch CEE_H_VERSION != 19991123
#endif
#include <idltype.h>
#if IDL_TYPE_H_VERSION != 19971225
#error Version mismatch IDL_TYPE_H_VERSION != 19971225
#endif
typedef IDL_string UUID_def;
#define UUID_def_cin_ ((char *) "d0+")
#define UUID_def_csz_ ((IDL_unsigned_long) 3)
typedef Long DIALOGUE_ID_def;
#define DIALOGUE_ID_def_cin_ ((char *) "F")
#define DIALOGUE_ID_def_csz_ ((IDL_unsigned_long) 1)
typedef IDL_char SQL_IDENTIFIER_def[513];
#define SQL_IDENTIFIER_def_cin_ ((char *) "d512+")
#define SQL_IDENTIFIER_def_csz_ ((IDL_unsigned_long) 5)
typedef IDL_char STMT_NAME_def[513];
#define STMT_NAME_def_cin_ ((char *) "d512+")
#define STMT_NAME_def_csz_ ((IDL_unsigned_long) 5)
typedef IDL_long SQL_DATATYPE_def;
#define SQL_DATATYPE_def_cin_ ((char *) "F")
#define SQL_DATATYPE_def_csz_ ((IDL_unsigned_long) 1)
typedef IDL_char SQLSTATE_def[6];
#define SQLSTATE_def_cin_ ((char *) "d5+")
#define SQLSTATE_def_csz_ ((IDL_unsigned_long) 3)
typedef IDL_string ERROR_STR_def;
#define ERROR_STR_def_cin_ ((char *) "d0+")
#define ERROR_STR_def_csz_ ((IDL_unsigned_long) 3)
typedef struct SQL_DataValue_def_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    IDL_octet *_buffer;
    IDL_PTR_PAD(_buffer, 1)
} SQL_DataValue_def;
#define SQL_DataValue_def_cin_ ((char *) "c0+H")
#define SQL_DataValue_def_csz_ ((IDL_unsigned_long) 4)
typedef IDL_short SQL_INDICATOR_def;
#define SQL_INDICATOR_def_cin_ ((char *) "I")
#define SQL_INDICATOR_def_csz_ ((IDL_unsigned_long) 1)
typedef IDL_long_long INTERVAL_NUM_def;
#define INTERVAL_NUM_def_cin_ ((char *) "G")
#define INTERVAL_NUM_def_csz_ ((IDL_unsigned_long) 1)
typedef IDL_char TIMESTAMP_STR_def[31];
#define TIMESTAMP_STR_def_cin_ ((char *) "d30+")
#define TIMESTAMP_STR_def_csz_ ((IDL_unsigned_long) 4)
typedef struct USER_SID_def_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    IDL_octet *_buffer;
    IDL_PTR_PAD(_buffer, 1)
} USER_SID_def;
#define USER_SID_def_cin_ ((char *) "c0+H")
#define USER_SID_def_csz_ ((IDL_unsigned_long) 4)
typedef struct USER_PASSWORD_def_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    IDL_octet *_buffer;
    IDL_PTR_PAD(_buffer, 1)
} USER_PASSWORD_def;
#define USER_PASSWORD_def_cin_ ((char *) "c0+H")
#define USER_PASSWORD_def_csz_ ((IDL_unsigned_long) 4)
typedef struct USER_NAME_def_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    IDL_octet *_buffer;
    IDL_PTR_PAD(_buffer, 1)
} USER_NAME_def;
#define USER_NAME_def_cin_ ((char *) "c0+H")
#define USER_NAME_def_csz_ ((IDL_unsigned_long) 4)
typedef IDL_long TIME_def;
#define TIME_def_cin_ ((char *) "F")
#define TIME_def_csz_ ((IDL_unsigned_long) 1)
typedef IDL_short GEN_PARAM_TOKEN_def;
#define GEN_PARAM_TOKEN_def_cin_ ((char *) "I")
#define GEN_PARAM_TOKEN_def_csz_ ((IDL_unsigned_long) 1)
typedef IDL_short GEN_PARAM_OPERATION_def;
#define GEN_PARAM_OPERATION_def_cin_ ((char *) "I")
#define GEN_PARAM_OPERATION_def_csz_ ((IDL_unsigned_long) 1)
typedef struct GEN_PARAM_VALUE_def_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    IDL_octet *_buffer;
    IDL_PTR_PAD(_buffer, 1)
} GEN_PARAM_VALUE_def;
#define GEN_PARAM_VALUE_def_cin_ ((char *) "c0+H")
#define GEN_PARAM_VALUE_def_csz_ ((IDL_unsigned_long) 4)
typedef IDL_char VPROC_def[33];
#define VPROC_def_cin_ ((char *) "d32+")
#define VPROC_def_csz_ ((IDL_unsigned_long) 4)
typedef IDL_char APLICATION_def[130];
#define APLICATION_def_cin_ ((char *) "d129+")
#define APLICATION_def_csz_ ((IDL_unsigned_long) 5)
typedef IDL_char COMPUTER_def[130];
#define COMPUTER_def_cin_ ((char *) "d129+")
#define COMPUTER_def_csz_ ((IDL_unsigned_long) 5)
typedef IDL_char NAME_def[130];
#define NAME_def_cin_ ((char *) "d129+")
#define NAME_def_csz_ ((IDL_unsigned_long) 5)
struct ERROR_DESC_t {
    IDL_long rowId;
    IDL_long errorDiagnosticId;
    IDL_long sqlcode;
    SQLSTATE_def sqlstate;
    char pad_to_offset_24_[6];
    ERROR_STR_def errorText;
    IDL_PTR_PAD(errorText, 1)
        IDL_long operationAbortId;
    IDL_long errorCodeType;
    IDL_string Param1;
    IDL_PTR_PAD(Param1, 1)
        IDL_string Param2;
    IDL_PTR_PAD(Param2, 1)
        IDL_string Param3;
    IDL_PTR_PAD(Param3, 1)
        IDL_string Param4;
    IDL_PTR_PAD(Param4, 1)
        IDL_string Param5;
    IDL_PTR_PAD(Param5, 1)
        IDL_string Param6;
    IDL_PTR_PAD(Param6, 1)
        IDL_string Param7;
    IDL_PTR_PAD(Param7, 1)
};
typedef ERROR_DESC_t ERROR_DESC_def;
#define ERROR_DESC_def_cin_ ((char *) \
        "b14+FFFd5+d0+FFd0+d0+d0+d0+d0+d0+d0+")
#define ERROR_DESC_def_csz_ ((IDL_unsigned_long) 36)
typedef struct ERROR_DESC_LIST_def_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    ERROR_DESC_def *_buffer;
    IDL_PTR_PAD(_buffer, 1)
} ERROR_DESC_LIST_def;
#define ERROR_DESC_LIST_def_cin_ ((char *) \
        "c0+b14+FFFd5+d0+FFd0+d0+d0+d0+d0+d0+d0+")
#define ERROR_DESC_LIST_def_csz_ ((IDL_unsigned_long) 39)
struct SQLItemDesc_t {
    IDL_long varAlign;
    IDL_long indAlign;
    IDL_long version;
    SQL_DATATYPE_def dataType;
    IDL_long datetimeCode;
    IDL_long maxLen;
    IDL_short precision;
    IDL_short scale;
    long vc_ind_length;
    IDL_boolean nullInfo;
    IDL_char colHeadingNm[514];
    IDL_boolean signType;
    IDL_long ODBCDataType;
    IDL_short ODBCPrecision;
    char pad_to_offset_544_[2];
    IDL_long SQLCharset;
    IDL_long ODBCCharset;
    IDL_long fsDataType;
    IDL_char ColumnLabel[514];
    IDL_char ColumnName[514];
    IDL_char TableName[514];
    IDL_char CatalogName[514];
    IDL_char SchemaName[514];
    IDL_char Heading[514];
    IDL_long intLeadPrec;
    IDL_long paramMode;
};
typedef SQLItemDesc_t SQLItemDesc_def;
#define SQLItemDesc_def_cin_ ((char *) \
        "b19+FFFFIIBd513+BFIFFd513+d513+d513+d513+FF")
#define SQLItemDesc_def_csz_ ((IDL_unsigned_long) 43)
typedef struct SQLItemDescList_def_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    SQLItemDesc_def *_buffer;
    IDL_PTR_PAD(_buffer, 1)
} SQLItemDescList_def;
#define SQLItemDescList_def_cin_ ((char *) \
        "c0+b19+FFFFIIBd513+BFIFFd513+d513+d513+d513+FF")
#define SQLItemDescList_def_csz_ ((IDL_unsigned_long) 46)
struct SQLValue_t {
    SQL_DATATYPE_def dataType;
    SQL_INDICATOR_def dataInd;
    char pad_to_offset_8_[2];
    SQL_DataValue_def dataValue;
    IDL_long dataCharset;
    char pad_to_size_32_[4];
};
typedef SQLValue_t SQLValue_def;
#define SQLValue_def_cin_ ((char *) "b4+FIc0+HF")
#define SQLValue_def_csz_ ((IDL_unsigned_long) 10)
typedef struct SQLValueList_def_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    SQLValue_def *_buffer;
    IDL_PTR_PAD(_buffer, 1)
} SQLValueList_def;
#define SQLValueList_def_cin_ ((char *) "c0+b4+FIc0+HF")
#define SQLValueList_def_csz_ ((IDL_unsigned_long) 13)
typedef IDL_enum USER_DESC_TYPE_t;
#define SID_TYPE ((IDL_enum) 0)
#define AUTHENTICATED_USER_TYPE ((IDL_enum) 1)
#define UNAUTHENTICATED_USER_TYPE ((IDL_enum) 2)
#define PASSWORD_ENCRYPTED_USER_TYPE ((IDL_enum) 3)
#define SID_ENCRYPTED_USER_TYPE ((IDL_enum) 4)
#define WIN95_USER_TYPE ((IDL_enum) 5)
typedef USER_DESC_TYPE_t USER_DESC_TYPE_def;
#define USER_DESC_TYPE_def_cin_ ((char *) "h5+")
#define USER_DESC_TYPE_def_csz_ ((IDL_unsigned_long) 3)
struct USER_DESC_t {
    USER_DESC_TYPE_def userDescType;
    char pad_to_offset_8_[4];
    USER_SID_def userSid;
    IDL_string domainName;
    IDL_PTR_PAD(domainName, 1)
        IDL_string userName;
    IDL_PTR_PAD(userName, 1)
        USER_PASSWORD_def password;
};
typedef USER_DESC_t USER_DESC_def;
#define USER_DESC_def_cin_ ((char *) "b5+h5+c0+Hd0+d0+c0+H")
#define USER_DESC_def_csz_ ((IDL_unsigned_long) 20)
struct VERSION_t {
    IDL_short componentId;
    IDL_short majorVersion;
    IDL_short minorVersion;
    char pad_to_offset_8_[2];
    IDL_unsigned_long buildId;
};
typedef VERSION_t VERSION_def;
#define VERSION_def_cin_ ((char *) "b4+IIIK")
#define VERSION_def_csz_ ((IDL_unsigned_long) 7)
typedef struct VERSION_LIST_def_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    VERSION_def *_buffer;
    IDL_PTR_PAD(_buffer, 1)
} VERSION_LIST_def;
#define VERSION_LIST_def_cin_ ((char *) "c0+b4+IIIK")
#define VERSION_LIST_def_csz_ ((IDL_unsigned_long) 10)
struct CONNECTION_CONTEXT_t {
    SQL_IDENTIFIER_def datasource;
    SQL_IDENTIFIER_def catalog;
    SQL_IDENTIFIER_def schema;
    SQL_IDENTIFIER_def location;
    SQL_IDENTIFIER_def userRole;
    char pad_to_offset_2566_[1];
    IDL_short accessMode;
    IDL_short autoCommit;
    char pad_to_offset_2572_[2];
    IDL_unsigned_long queryTimeoutSec;
    IDL_unsigned_long idleTimeoutSec;
    IDL_unsigned_long loginTimeoutSec;
    IDL_short txnIsolationLevel;
    IDL_short rowSetSize;
    IDL_long diagnosticFlag;
    IDL_unsigned_long processId;
    IDL_char computerName[61];
    char pad_to_offset_2664_[7];
    IDL_string windowText;
    IDL_PTR_PAD(windowText, 1)
        IDL_unsigned_long ctxACP;
    IDL_unsigned_long ctxDataLang;
    IDL_unsigned_long ctxErrorLang;
    IDL_short ctxCtrlInferNCHAR;
    IDL_short cpuToUse;
    IDL_short cpuToUseEnd;
    IDL_char clientVproc[101];
    char pad_to_offset_2744_[3];
    IDL_string connectOptions;
    IDL_PTR_PAD(connectOptions, 1)
        VERSION_LIST_def clientVersionList;
    IDL_unsigned_long inContextOptions1;
    IDL_unsigned_long inContextOptions2;
    IDL_char sessionName[101];
    char pad_to_offset_2880_[3];
    IDL_string clientUserName;
    IDL_PTR_PAD(clientUserName, 1)
};
typedef CONNECTION_CONTEXT_t CONNECTION_CONTEXT_def;
#define CONNECTION_CONTEXT_def_cin_ ((char *) \
        "b29+d512+d512+d512+d512+d512+IIKKKIIFKa1+61+Cd0+KKKIIIa1+51+"\
        "Cd0+c0+b4+IIIKKKa1+101+Cd0+")
#define CONNECTION_CONTEXT_def_csz_ ((IDL_unsigned_long) 87)
struct OUT_CONNECTION_CONTEXT_t {
    VERSION_LIST_def versionList;
    IDL_short nodeId;
    char pad_to_offset_20_[2];
    IDL_unsigned_long processId;
    IDL_char computerName[61];
    SQL_IDENTIFIER_def catalog;
    SQL_IDENTIFIER_def schema;
    char pad_to_offset_1112_[1];
    IDL_unsigned_long outContextOptions1;
    IDL_unsigned_long outContextOptions2;
    IDL_unsigned_long outContextOptionStringLen;
    char pad_to_offset_1128_[4];
    IDL_string outContextOptionString;
    IDL_PTR_PAD(outContextOptionString, 1)
};
typedef OUT_CONNECTION_CONTEXT_t OUT_CONNECTION_CONTEXT_def;
#define OUT_CONNECTION_CONTEXT_def_cin_ ((char *) \
        "b10+c0+b4+IIIKIKa1+61+Cd512+d512+KKKd0+")
#define OUT_CONNECTION_CONTEXT_def_csz_ ((IDL_unsigned_long) 39)
typedef IDL_char IDL_OBJECT_def[128];
#define IDL_OBJECT_def_cin_ ((char *) "a1+128+C")
#define IDL_OBJECT_def_csz_ ((IDL_unsigned_long) 8)
struct GEN_Param_t {
    GEN_PARAM_TOKEN_def paramToken;
    GEN_PARAM_OPERATION_def paramOperation;
    char pad_to_offset_8_[4];
    GEN_PARAM_VALUE_def paramValue;
};
typedef GEN_Param_t GEN_Param_def;
#define GEN_Param_def_cin_ ((char *) "b3+IIc0+H")
#define GEN_Param_def_csz_ ((IDL_unsigned_long) 9)
typedef struct GEN_ParamList_def_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    GEN_Param_def *_buffer;
    IDL_PTR_PAD(_buffer, 1)
} GEN_ParamList_def;
#define GEN_ParamList_def_cin_ ((char *) "c0+b3+IIc0+H")
#define GEN_ParamList_def_csz_ ((IDL_unsigned_long) 12)
struct RES_DESC_t {
    SQL_IDENTIFIER_def AttrNm;
    char pad_to_offset_520_[7];
    IDL_long_long Limit;
    IDL_string Action;
    IDL_PTR_PAD(Action, 1)
        IDL_long Settable;
    char pad_to_size_544_[4];
};
typedef RES_DESC_t RES_DESC_def;
#define RES_DESC_def_cin_ ((char *) "b4+d512+Gd0+F")
#define RES_DESC_def_csz_ ((IDL_unsigned_long) 13)
typedef struct RES_DESC_LIST_def_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    RES_DESC_def *_buffer;
    IDL_PTR_PAD(_buffer, 1)
} RES_DESC_LIST_def;
#define RES_DESC_LIST_def_cin_ ((char *) "c0+b4+d512+Gd0+F")
#define RES_DESC_LIST_def_csz_ ((IDL_unsigned_long) 16)
struct ENV_DESC_t {
    IDL_long VarSeq;
    IDL_long VarType;
    IDL_string VarVal;
    IDL_PTR_PAD(VarVal, 1)
};
typedef ENV_DESC_t ENV_DESC_def;
#define ENV_DESC_def_cin_ ((char *) "b3+FFd0+")
#define ENV_DESC_def_csz_ ((IDL_unsigned_long) 8)
typedef struct ENV_DESC_LIST_def_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    ENV_DESC_def *_buffer;
    IDL_PTR_PAD(_buffer, 1)
} ENV_DESC_LIST_def;
#define ENV_DESC_LIST_def_cin_ ((char *) "c0+b3+FFd0+")
#define ENV_DESC_LIST_def_csz_ ((IDL_unsigned_long) 11)
struct SRVR_CONTEXT_t {
    INTERVAL_NUM_def srvrIdleTimeout;
    INTERVAL_NUM_def connIdleTimeout;
    RES_DESC_LIST_def resDescList;
    ENV_DESC_LIST_def envDescList;
};
typedef SRVR_CONTEXT_t SRVR_CONTEXT_def;
#define SRVR_CONTEXT_def_cin_ ((char *) \
        "b4+GGc0+b4+d512+Gd0+Fc0+b3+FFd0+")
#define SRVR_CONTEXT_def_csz_ ((IDL_unsigned_long) 32)
#ifdef USE_NEW_PHANDLE
typedef SB_Phandle_Type PROCESS_HANDLE_def;    
#else
typedef IDL_short PROCESS_HANDLE_def[10];
#endif
#define PROCESS_HANDLE_def_cin_ ((char *) "a1+10+I")
#define PROCESS_HANDLE_def_csz_ ((IDL_unsigned_long) 7)
typedef struct PROCESS_HANDLE_List_def_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    PROCESS_HANDLE_def *_buffer;
    IDL_PTR_PAD(_buffer, 1)
} PROCESS_HANDLE_List_def;
#define PROCESS_HANDLE_List_def_cin_ ((char *) "c0+a1+10+I")
#define PROCESS_HANDLE_List_def_csz_ ((IDL_unsigned_long) 10)
/*
 * End translation unit: ODBCCOMMON
 */
#endif /* ODBCCOMMON_H_ */
