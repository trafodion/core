/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
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
**********************************************************************/

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         CmpSeabaseDDLtable.cpp
 * Description:  Implements ddl operations for Seabase tables.
 *
 *
 * Created:     6/30/2013
 * Language:     C++
 *
 *
 *****************************************************************************
 */


#include "CmpSeabaseDDLincludes.h"
#include "ElemDDLColDefault.h"
#include "NumericType.h"
#include "ComUser.h"
#include "keycolumns.h"
#include "ElemDDLColRef.h"
#include "ElemDDLColName.h"

#include "CmpDDLCatErrorCodes.h"
#include "Globals.h"
#include "CmpMain.h"
#include "Context.h"
#include "PrivMgrCommands.h"

// defined in CmpDescribe.cpp
extern short CmpDescribeSeabaseTable ( 
                             const CorrName  &dtName,
                             short type, // 1, invoke. 2, showddl. 3, createLike
                             char* &outbuf,
                             ULng32 &outbuflen,
                             CollHeap *heap,
                             const char * pkeyStr = NULL,
                             NABoolean withPartns = FALSE,
                             NABoolean withoutSalt = FALSE,
                             NABoolean withoutDivisioning = FALSE,
                             NABoolean noTrailingSemi = FALSE);

void CmpSeabaseDDL::convertVirtTableColumnInfoToDescStruct( 
     const ComTdbVirtTableColumnInfo * colInfo,
     const ComObjectName * objectName,
     desc_struct * column_desc)
{
  column_desc->body.columns_desc.tablename = 
    convertNAString(objectName->getExternalName(), STMTHEAP);
  
  char * col_name = new(STMTHEAP) char[strlen(colInfo->colName) + 1];
  strcpy(col_name, colInfo->colName);
  column_desc->body.columns_desc.colname = col_name;
  column_desc->body.columns_desc.colnumber = colInfo->colNumber;
  column_desc->body.columns_desc.datatype  = colInfo->datatype;
  column_desc->body.columns_desc.length    = colInfo->length;
  if (!(DFS2REC::isInterval(colInfo->datatype) || DFS2REC::isDateTime(colInfo->datatype)))
    column_desc->body.columns_desc.scale     = colInfo->scale;
  else
    column_desc->body.columns_desc.scale = 0;
  column_desc->body.columns_desc.precision = colInfo->precision;
  column_desc->body.columns_desc.datetimestart = (rec_datetime_field) colInfo->dtStart;
  column_desc->body.columns_desc.datetimeend = (rec_datetime_field) colInfo->dtEnd;
  if (DFS2REC::isDateTime(colInfo->datatype))
    column_desc->body.columns_desc.datetimefractprec = colInfo->scale;
  else
    column_desc->body.columns_desc.datetimefractprec = 0;
  if (DFS2REC::isInterval(colInfo->datatype))
    column_desc->body.columns_desc.intervalleadingprec = colInfo->precision;
  else
    column_desc->body.columns_desc.intervalleadingprec = 0 ;
  column_desc->body.columns_desc.null_flag = colInfo->nullable;
  column_desc->body.columns_desc.upshift   = colInfo->upshifted;
  column_desc->body.columns_desc.character_set = (CharInfo::CharSet) colInfo->charset;
  switch (colInfo->columnClass)
    {
    case COM_USER_COLUMN:
      column_desc->body.columns_desc.colclass = 'U';
      break;
    case COM_SYSTEM_COLUMN:
      column_desc->body.columns_desc.colclass = 'S';
      break;
    default:
      CMPASSERT(0);
    }
  column_desc->body.columns_desc.defaultClass = colInfo->defaultClass;
  column_desc->body.columns_desc.colFlags = colInfo->colFlags;
  
  
  column_desc->body.columns_desc.pictureText =
    (char *)STMTHEAP->allocateMemory(340);
  NAType::convertTypeToText(column_desc->body.columns_desc.pictureText,  //OUT
                            column_desc->body.columns_desc.datatype,
                            column_desc->body.columns_desc.length,
                            column_desc->body.columns_desc.precision,
                            column_desc->body.columns_desc.scale,
                            column_desc->body.columns_desc.datetimestart,
                            column_desc->body.columns_desc.datetimeend,
                            column_desc->body.columns_desc.datetimefractprec,
                            column_desc->body.columns_desc.intervalleadingprec,
                            column_desc->body.columns_desc.upshift,
                            column_desc->body.columns_desc.caseinsensitive,
                            (CharInfo::CharSet)column_desc->body.columns_desc.character_set,
                            (CharInfo::Collation) 1, // default collation
                            NULL, // displayDataType
                            0); // displayCaseSpecific
  

  column_desc->body.columns_desc.offset    = -1; // not present in colInfo
  column_desc->body.columns_desc.caseinsensitive = (short)FALSE; // not present in colInfo
  column_desc->body.columns_desc.encoding_charset = (CharInfo::CharSet) column_desc->body.columns_desc.character_set ; // not present in colInfo so we go with the column's charset here. 
  column_desc->body.columns_desc.collation_sequence = (CharInfo::Collation)1; // not present in colInfo, so we go with default collation here (used in buildEncodeTree for some error handling)
  column_desc->body.columns_desc.uec     = (Cardinality)0; // not present in colInfo
  column_desc->body.columns_desc.highval = 0; // not present in colInfo
  column_desc->body.columns_desc.lowval  = 0; // not present in colInfo
  column_desc->body.columns_desc.defaultvalue = NULL ; // not present in colInfo
  column_desc->body.columns_desc.stored_on_disk = 0 ; // not present in colInfo
  column_desc->body.columns_desc.computed_column_text = NULL; // not present in colInfo
}

desc_struct * CmpSeabaseDDL::convertVirtTableColumnInfoArrayToDescStructs(
     const ComObjectName * objectName,
     const ComTdbVirtTableColumnInfo * colInfoArray,
     Lng32 numCols)
{
  desc_struct * prev_column_desc  = NULL;
  desc_struct * first_column_desc = NULL;
  for (Int32 i = 0; i < numCols; i++)
  {
    const ComTdbVirtTableColumnInfo* colInfo = &(colInfoArray[i]);

    // readtabledef_allocate_desc() requires that HEAP (STMTHEAP) 
    // be used for operator new herein
    desc_struct * column_desc = readtabledef_allocate_desc(DESC_COLUMNS_TYPE);
    if (prev_column_desc != NULL)
      prev_column_desc->header.next = column_desc;
    else
      first_column_desc = column_desc;      
    
    prev_column_desc = column_desc;
    convertVirtTableColumnInfoToDescStruct(colInfo, objectName, column_desc);
  }

  return first_column_desc;
}

desc_struct * CmpSeabaseDDL::convertVirtTableKeyInfoArrayToDescStructs(
     const ComTdbVirtTableKeyInfo *keyInfoArray,
     const ComTdbVirtTableColumnInfo *colInfoArray,
     Lng32 numKeys)
{
  desc_struct * prev_key_desc  = NULL;
  desc_struct * first_key_desc = NULL;
  for (Int32 i = 0; i < numKeys; i++)
    {
      const ComTdbVirtTableColumnInfo * colInfo = &(colInfoArray[keyInfoArray[i].tableColNum]);
      desc_struct * key_desc = readtabledef_allocate_desc(DESC_KEYS_TYPE);
      if (prev_key_desc != NULL)
        prev_key_desc->header.next = key_desc;
      else
       first_key_desc = key_desc;      
      
      prev_key_desc = key_desc;
      
      key_desc->body.keys_desc.tablecolnumber = keyInfoArray[i].tableColNum;
      key_desc->body.keys_desc.keyseqnumber = i;
      key_desc->body.keys_desc.ordering = keyInfoArray[i].ordering;
    }

  return first_key_desc;
}



void CmpSeabaseDDL::createSeabaseTableLike(
                                           StmtDDLCreateTable * createTableNode,
                                           NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;

  ComObjectName tgtTableName(createTableNode->getTableName(), COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tgtTableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  const NAString extTgtTableName = tgtTableName.getExternalName(TRUE);
  
  ComObjectName srcTableName(createTableNode->getLikeSourceTableName(), COM_TABLE_NAME);
  srcTableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  
  CorrName cn(srcTableName.getObjectNamePart().getInternalName(),
              STMTHEAP,
              srcTableName.getSchemaNamePart().getInternalName(),
              srcTableName.getCatalogNamePart().getInternalName());

  ElemDDLColRefArray &keyArray = 
    (createTableNode->getIsConstraintPKSpecified() ?
     createTableNode->getPrimaryKeyColRefArray() :
     (createTableNode->getStoreOption() == COM_KEY_COLUMN_LIST_STORE_OPTION ?
      createTableNode->getKeyColumnArray() :
      createTableNode->getPrimaryKeyColRefArray()));

  NAString keyClause;
  if ((keyArray.entries() > 0) &&
      ((createTableNode->getIsConstraintPKSpecified()) ||
       (createTableNode->getStoreOption() == COM_KEY_COLUMN_LIST_STORE_OPTION)))
    {
      if (createTableNode->getIsConstraintPKSpecified())
        keyClause = " primary key ( ";
      else if (createTableNode->getStoreOption() == COM_KEY_COLUMN_LIST_STORE_OPTION)
        keyClause = " store by ( ";
      
      for (CollIndex i = 0; i < keyArray.entries(); i++) 
        {
          NAString colName = keyArray[i]->getColumnName();
          
          // Generate a delimited identifier if source colName is delimited
          // Launchpad bug: 1383531
          colName/*InExternalFormat*/ = ToAnsiIdentifier (colName/*InInternalFormat*/);
         
          keyClause += colName;

          if (i < (keyArray.entries() - 1))
            keyClause += ", ";
        }

      keyClause += ")";
    }

  ParDDLLikeOptsCreateTable &likeOptions = createTableNode->getLikeOptions();
  
  char * buf = NULL;
  ULng32 buflen = 0;
  retcode = CmpDescribeSeabaseTable(cn, 3/*createlike*/, buf, buflen, STMTHEAP,
                                    NULL,
                                    likeOptions.getIsWithHorizontalPartitions(),
                                    likeOptions.getIsWithoutSalt(),
                                    likeOptions.getIsWithoutDivision(),
                                    TRUE);
  if (retcode)
    return;

  NAString query = "create table ";
  query += extTgtTableName;
  query += " ";

  NABoolean done = FALSE;
  Lng32 curPos = 0;
  while (NOT done)
    {
      short len = *(short*)&buf[curPos];
      NAString frag(&buf[curPos+sizeof(short)],
                    len - ((buf[curPos+len-1]== '\n') ? 1 : 0));

      query += frag;
      curPos += ((((len+sizeof(short))-1)/8)+1)*8;

      if (curPos >= buflen)
        done = TRUE;
    }

  if (NOT keyClause.isNull())
    {
      // add the keyClause
      query += keyClause;
    }

  ExeCliInterface cliInterface(STMTHEAP);

  Lng32 cliRC = 0;

  cliRC = cliInterface.executeImmediate((char*)query.data());
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }
  
  return;
}

short CmpSeabaseDDL::genPKeyName(StmtDDLAddConstraintPK *addPKNode,
                                 const char * catName,
                                 const char * schName,
                                 const char * objName,
                                 NAString &pkeyName)
{
  
  ComObjectName tableName( (addPKNode ? addPKNode->getTableName() : " "), COM_TABLE_NAME);

  ElemDDLConstraintPK *constraintNode = 
    (addPKNode ? (addPKNode->getConstraint())->castToElemDDLConstraintPK() : NULL);

  ComString specifiedConstraint;
  ComString constraintName;
  if( !constraintNode || (constraintNode->getConstraintName().isNull()))
    {
      specifiedConstraint.append( catName); 
      specifiedConstraint.append(".");
      specifiedConstraint.append("\"");
      specifiedConstraint.append( schName); 
      specifiedConstraint.append("\"");
      specifiedConstraint.append(".");

      ComString oName = "\"";
      oName += objName; 
      oName += "\"";
      Lng32 status = ToInternalIdentifier ( oName // in/out - from external- to internal-format
                                            , TRUE  // in - NABoolean upCaseInternalNameIfRegularIdent
                                            , TRUE  // in - NABoolean acceptCircumflex
                                            );
      ComDeriveRandomInternalName ( ComGetNameInterfaceCharSet()
                                    , /*internalFormat*/oName          // in  - const ComString &
                                    , /*internalFormat*/constraintName // out - ComString &
                                    , STMTHEAP
                                    );

      // Generate a delimited identifier if objectName was delimited
      constraintName/*InExternalFormat*/ = ToAnsiIdentifier (constraintName/*InInternalFormat*/);

      specifiedConstraint.append(constraintName);
    }
  else
    {
      specifiedConstraint = constraintNode->
        getConstraintNameAsQualifiedName().getQualifiedNameAsAnsiString();
    }

  pkeyName = specifiedConstraint;

  return 0;
}

short CmpSeabaseDDL::updatePKeyInfo(
                                    StmtDDLAddConstraintPK *addPKNode,
                                    const char * catName,
                                    const char * schName,
                                    const char * objName,
                                    const Int32 ownerID,
                                    const Int32 schemaOwnerID,
                                    Lng32 numKeys,
                                    Int64 * outPkeyUID,
                                    Int64 * outTableUID,
                                    const ComTdbVirtTableKeyInfo * keyInfoArray,
                                    ExeCliInterface *cliInterface)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];

  // update primary key constraint info
  NAString pkeyStr;
  if (genPKeyName(addPKNode, catName, schName, objName, pkeyStr))
    {
      return -1;
    }

  Int64 createTime = NA_JulianTimestamp();

  ComUID comUID;
  comUID.make_UID();
  Int64 pkeyUID = comUID.get_value();
  if (outPkeyUID)
    *outPkeyUID = pkeyUID;

  ComObjectName pkeyName(pkeyStr);
  const NAString catalogNamePart = pkeyName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = pkeyName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = pkeyName.getObjectNamePartAsAnsiString(TRUE);
  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schemaNamePart), FALSE);
  NAString quotedObjName;
  ToQuotedString(quotedObjName, NAString(objectNamePart), FALSE);

  str_sprintf(buf, "insert into %s.\"%s\".%s values ('%s', '%s', '%s', '%s', %Ld, %Ld, %Ld, '%s', '%s', %d, %d, 0 )",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              catalogNamePart.data(), quotedSchName.data(), quotedObjName.data(),
              COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT,
              pkeyUID,
              createTime, 
              createTime,
              " ",
              COM_NO_LIT,
              ownerID,
              schemaOwnerID);
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      *CmpCommon::diags() << DgSqlCode(-1423)
                          << DgString0(SEABASE_OBJECTS);

      return -1;
    }

  Int64 tableUID = 
    getObjectUID(cliInterface,
                 catName, schName, objName,
                 COM_BASE_TABLE_OBJECT_LIT);

  if (outTableUID)
    *outTableUID = tableUID;

  Int64 validatedTime = NA_JulianTimestamp();

  Int64 indexUID = 0;
  str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, %Ld, '%s', '%s', '%s', '%s', '%s', '%s', %Ld, %d, %Ld, 0 )",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
              tableUID, pkeyUID,
              COM_PRIMARY_KEY_CONSTRAINT_LIT,
              COM_NO_LIT,
              COM_NO_LIT,
              COM_NO_LIT,
              COM_YES_LIT,
              COM_YES_LIT,
              validatedTime,
              numKeys,
              indexUID);
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      *CmpCommon::diags() << DgSqlCode(-1423)
                          << DgString0(SEABASE_TABLE_CONSTRAINTS);

      return -1;
    }

  if (keyInfoArray)
    {
      for (Lng32 i = 0; i < numKeys; i++)
        {
          str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, '%s', %d, %d, %d, %d, 0)",
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
                      pkeyUID,
                      keyInfoArray[i].colName,
                      i+1,
                      keyInfoArray[i].tableColNum,
                      0,
                      0);
          
          cliRC = cliInterface->executeImmediate(buf);
          if (cliRC < 0)
            {
              cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
              
              *CmpCommon::diags() << DgSqlCode(-1423)
                                  << DgString0(SEABASE_KEYS);

              return -1;
            }
        }
    }

  return 0;
}

short CmpSeabaseDDL::constraintErrorChecks(
                                           StmtDDLAddConstraint *addConstrNode,
                                           NATable * naTable,
                                           ComConstraintType ct,
                                           NAList<NAString> &keyColList)
{

  const NAString &addConstrName = addConstrNode->
    getConstraintNameAsQualifiedName().getQualifiedNameAsAnsiString();

  // make sure that there is no other constraint on this table with this name.
  NABoolean foundConstr = FALSE;
  const CheckConstraintList &checkList = naTable->getCheckConstraints();
  for (Int32 i = 0; i < checkList.entries(); i++)
    {
      CheckConstraint *checkConstr = (CheckConstraint*)checkList[i];
      
      const NAString &tableConstrName = 
        checkConstr->getConstraintName().getQualifiedNameAsAnsiString();
      
      if (addConstrName == tableConstrName)
        {
          foundConstr = TRUE;
        }
    } // for
  
  if (NOT foundConstr)
    {
      const AbstractRIConstraintList &ariList = naTable->getUniqueConstraints();
      for (Int32 i = 0; i < ariList.entries(); i++)
        {
          AbstractRIConstraint *ariConstr = (AbstractRIConstraint*)ariList[i];
          
          const NAString &tableConstrName = 
            ariConstr->getConstraintName().getQualifiedNameAsAnsiString();
          
          if (addConstrName == tableConstrName)
            {
              foundConstr = TRUE;
            }
        } // for
    }

  if (NOT foundConstr)
    {
      const AbstractRIConstraintList &ariList = naTable->getRefConstraints();
      for (Int32 i = 0; i < ariList.entries(); i++)
        {
          AbstractRIConstraint *ariConstr = (AbstractRIConstraint*)ariList[i];
          
          const NAString &tableConstrName = 
            ariConstr->getConstraintName().getQualifiedNameAsAnsiString();
          
          if (addConstrName == tableConstrName)
            {
              foundConstr = TRUE;
            }
        } // for
    }
  
  if (foundConstr)
    {
      *CmpCommon::diags()
        << DgSqlCode(-1043)
        << DgConstraintName(addConstrName);
      
      processReturn();
      
      return -1;
    }
  
  if ((ct == COM_UNIQUE_CONSTRAINT) || 
      (ct == COM_FOREIGN_KEY_CONSTRAINT) ||
      (ct == COM_PRIMARY_KEY_CONSTRAINT))
    {
      for (CollIndex ndx = 0; ndx < keyColList.entries(); ndx++)
        {
          const ComString intColName = keyColList[ndx];
          if (intColName EQU "SYSKEY")
            {
              const NAString &tabName = addConstrNode->getTableName();
              
              *CmpCommon::diags() << 
                DgSqlCode(((ct == COM_UNIQUE_CONSTRAINT) ? -CAT_SYSKEY_COL_NOT_ALLOWED_IN_UNIQUE_CNSTRNT :
                           -CAT_SYSKEY_COL_NOT_ALLOWED_IN_RI_CNSTRNT))
                                  << DgColumnName( ToAnsiIdentifier( intColName ))
                                  << DgTableName(tabName);
              
              processReturn();
              
              return -1;
            }
        }
      
      const NAColumnArray & naColArray = naTable->getNAColumnArray();
      
      // Now process each column defined in the parseColList to see if
      // it exists in the table column list and it isn't a duplicate.
      NABitVector seenIt; 
      NAString keyColNameStr;
      for (CollIndex i = 0; i < keyColList.entries(); i++)
        {
          NAColumn * nac = naColArray.getColumn(keyColList[i]);
          if (! nac)
            {
              *CmpCommon::diags() << DgSqlCode(-1009)
                                  << DgColumnName( ToAnsiIdentifier(keyColList[i]));
              return -1;
            }
          
          Lng32 colNumber = nac->getPosition();
          
          // If the column has already been found, return error
          if( seenIt.contains(colNumber)) 
            {
              *CmpCommon::diags() << DgSqlCode(-CAT_REDUNDANT_COLUMN_REF_PK)
                                  << DgColumnName( ToAnsiIdentifier(keyColList[i]));
              return -1;
            }
          
          seenIt.setBit(colNumber);
        }

      if (ct == COM_UNIQUE_CONSTRAINT)      
        {
          // Compare the column list from parse tree to the unique and primary
          // key constraints already defined for the table.  The new unique 
          // constraint list must be distinct.  The order of the existing constraint
          // does not have to be in the same order as the new constraint.
          //
          NABoolean matchFound = FALSE;
          
          if (naTable->getCorrespondingConstraint(keyColList,
                                                  TRUE, // unique constraint
                                                  NULL))
            matchFound = TRUE;
          
          if (matchFound)
            {
              *CmpCommon::diags() << DgSqlCode(-CAT_DUPLICATE_UNIQUE_CONSTRAINT_ON_SAME_COL);
              
              return -1;
            }
        }
    }

  return 0;
}
                        
short CmpSeabaseDDL::genUniqueName(StmtDDLAddConstraint *addUniqueNode,
                                 NAString &uniqueName)
{
  
  ComObjectName tableName( addUniqueNode->getTableName(), COM_TABLE_NAME);

  ElemDDLConstraint *constraintNode = 
    (addUniqueNode->getConstraint())->castToElemDDLConstraint();

  ComString specifiedConstraint;
  ComString constraintName;
  if( constraintNode->getConstraintName().isNull() )
    {
      specifiedConstraint.append( tableName.getCatalogNamePartAsAnsiString() );
      specifiedConstraint.append(".");
      specifiedConstraint.append( tableName.getSchemaNamePartAsAnsiString() );
      specifiedConstraint.append(".");

      ComString oName = tableName.getObjectNamePartAsAnsiString() ; 
      Lng32 status = ToInternalIdentifier ( oName // in/out - from external- to internal-format
                                            , TRUE  // in - NABoolean upCaseInternalNameIfRegularIdent
                                            , TRUE  // in - NABoolean acceptCircumflex
                                            );
      ComDeriveRandomInternalName ( ComGetNameInterfaceCharSet()
                                    , /*internalFormat*/oName          // in  - const ComString &
                                    , /*internalFormat*/constraintName // out - ComString &
                                    , STMTHEAP
                                    );

      // Generate a delimited identifier if objectName was delimited
      constraintName/*InExternalFormat*/ = ToAnsiIdentifier (constraintName/*InInternalFormat*/);

      specifiedConstraint.append(constraintName);
    }
  else
    {
      specifiedConstraint = constraintNode->
        getConstraintNameAsQualifiedName().getQualifiedNameAsAnsiString();
    }

  uniqueName = specifiedConstraint;

  return 0;
}

short CmpSeabaseDDL::updateConstraintMD(
                                        NAList<NAString> &keyColList,
                                        NAList<NAString> &keyColOrderList,
                                        NAString &uniqueStr,
                                        Int64 tableUID,
                                        Int64 constrUID,
                                        NATable * naTable,
                                        ComConstraintType ct,
                                        NABoolean enforced,
                                        ExeCliInterface *cliInterface)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];

  const NAColumnArray & naColArray = naTable->getNAColumnArray();

  Int64 createTime = NA_JulianTimestamp();

  ComObjectName uniqueName(uniqueStr);
  const NAString catalogNamePart = uniqueName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = uniqueName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = uniqueName.getObjectNamePartAsAnsiString(TRUE);
  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schemaNamePart), FALSE);
  NAString quotedObjName;
  ToQuotedString(quotedObjName, NAString(objectNamePart), FALSE);

  str_sprintf(buf, "insert into %s.\"%s\".%s values ('%s', '%s', '%s', '%s', %Ld, %Ld, %Ld, '%s', '%s', %d, %d, 0 )",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              catalogNamePart.data(), quotedSchName.data(), quotedObjName.data(),
              ((ct == COM_UNIQUE_CONSTRAINT) ? COM_UNIQUE_CONSTRAINT_OBJECT_LIT :
               ((ct == COM_FOREIGN_KEY_CONSTRAINT) ? COM_REFERENTIAL_CONSTRAINT_OBJECT_LIT : COM_CHECK_CONSTRAINT_OBJECT_LIT)),
              constrUID,
              createTime, 
              createTime,
              " ",
              COM_NO_LIT,
              naTable->getOwner(),
              naTable->getSchemaOwner());
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  Int64 indexUID = 0;
  str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, %Ld, '%s', '%s', '%s', '%s', '%s', '%s', %Ld, %d, %Ld, 0 )",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
              tableUID, constrUID,
              ((ct == COM_UNIQUE_CONSTRAINT) ? COM_UNIQUE_CONSTRAINT_LIT :
               ((ct == COM_FOREIGN_KEY_CONSTRAINT) ? COM_FOREIGN_KEY_CONSTRAINT_LIT : COM_CHECK_CONSTRAINT_LIT)),
              COM_NO_LIT,
              COM_NO_LIT,
              COM_NO_LIT,
              (enforced ? COM_YES_LIT : COM_NO_LIT),
              COM_YES_LIT,
              createTime,
              keyColList.entries(),
              indexUID);
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  for (Lng32 i = 0; i < keyColList.entries(); i++)
    {
      NAColumn * nac = naColArray.getColumn(keyColList[i]);
      Lng32 colNumber = nac->getPosition();

      str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, '%s', %d, %d, %d, %d, 0)",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
                  constrUID,
                  keyColList[i].data(),
                  i+1,
                  colNumber,
                  (keyColOrderList[i] == "DESC" ? 1 : 0),
                  0);
      
      cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

          return -1;
        }
    }

  return 0;
}

short CmpSeabaseDDL::updateRIConstraintMD(
                                          Int64 ringConstrUID,
                                          Int64 refdConstrUID,
                                          ExeCliInterface *cliInterface)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];

  str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, %Ld, '%s', '%s', '%s', 0 )",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_REF_CONSTRAINTS,
              ringConstrUID, refdConstrUID,
              COM_FULL_MATCH_OPTION_LIT,
              COM_RESTRICT_UPDATE_RULE_LIT,
              COM_RESTRICT_DELETE_RULE_LIT);
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, %Ld, 0)",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_UNIQUE_REF_CONSTR_USAGE,
              refdConstrUID, ringConstrUID);
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  return 0;
}

short CmpSeabaseDDL::updateIndexInfo(
                                     NAList<NAString> &ringKeyColList,
                                     NAList<NAString> &ringKeyColOrderList,
                                     NAList<NAString> &refdKeyColList,
                                     NAString &uniqueStr,
                                     Int64 constrUID,
                                     const char * catName,
                                     const char * schName,
                                     const char * objName,
                                     NATable * naTable,
                                     NABoolean isUnique, // TRUE: uniq constr. FALSE: ref constr.
                                     NABoolean noPopulate,
                                     ExeCliInterface *cliInterface)
{
  // Now we need to determine if an index has to be created for
  // the unique or ref constraint.  
  NABoolean createIndex = TRUE;

  NAString existingIndexName;
  if (naTable->getCorrespondingIndex(ringKeyColList, 
                                     TRUE, // explicit index only
                                     isUnique, //TRUE, look for unique index.
                                     TRUE, //isUnique, //TRUE, look for primary key.
                                     (NOT isUnique), // TRUE, look for any index or pkey
                                     &existingIndexName))
    createIndex = FALSE;
  
  ComObjectName indexName(createIndex ? uniqueStr : existingIndexName);
  const NAString catalogNamePart = indexName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = indexName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = indexName.getObjectNamePartAsAnsiString(TRUE);
  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schemaNamePart), FALSE);
  NAString quotedObjName;
  ToQuotedString(quotedObjName, NAString(objectNamePart), FALSE);

  char buf[5000];
  Lng32 cliRC;

  Int64 tableUID = naTable->objectUid().castToInt64();
  
  if (createIndex)
    {
      NAString keyColNameStr;
      for (CollIndex i = 0; i < ringKeyColList.entries(); i++)
        {
          keyColNameStr += "\"";
          keyColNameStr += ringKeyColList[i];
          keyColNameStr += "\" ";
          keyColNameStr += ringKeyColOrderList[i];
          if (i+1 < ringKeyColList.entries())
            keyColNameStr += ", ";
        }

      char noPopStr[100];
      if (noPopulate)
        strcpy(noPopStr, " no populate ");
      else
        strcpy(noPopStr, " ");

      if (isUnique)
        str_sprintf(buf, "create unique index \"%s\" on \"%s\".\"%s\".\"%s\" ( %s ) %s",
                    quotedObjName.data(),
                    catName, schName, objName,
                    keyColNameStr.data(),
                    noPopStr);
      else
        str_sprintf(buf, "create index \"%s\" on \"%s\".\"%s\".\"%s\" ( %s ) %s",
                    quotedObjName.data(),
                    catName, schName, objName,
                    keyColNameStr.data(),
                    noPopStr);
        
      cliRC = cliInterface->executeImmediate(buf);
      
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }

      // update indexes table and mark this index as an implicit index.
      str_sprintf(buf, "update %s.\"%s\".%s set is_explicit = 0 where base_table_uid = %Ld and index_uid = (select object_uid from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' and object_name = '%s' and object_type = 'IX') ",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_INDEXES,
                  tableUID,
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                  catName, schemaNamePart.data(), objectNamePart.data());
     cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

          return -1;
        }

      if (noPopulate)
        {
          if (updateObjectValidDef(cliInterface, 
                                   catalogNamePart, schemaNamePart, objectNamePart,
                                   COM_INDEX_OBJECT_LIT,
                                   COM_YES_LIT))
            {
              return -1;
            }
        }
    }

  // update table_constraints table with the uid of this index.
  Int64 indexUID = 
    getObjectUID(cliInterface,
                 catName, schemaNamePart, objectNamePart,
                 COM_INDEX_OBJECT_LIT);
  if (indexUID < 0)
    {
      // primary key. Clear diags area since getObjectUID sets up diags entry.
      CmpCommon::diags()->clear();
    }

  str_sprintf(buf, "update %s.\"%s\".%s set index_uid = %Ld where table_uid = %Ld and constraint_uid = %Ld and constraint_type = '%s'",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
              indexUID,
              tableUID, constrUID,
              (isUnique ? COM_UNIQUE_CONSTRAINT_LIT : COM_FOREIGN_KEY_CONSTRAINT_LIT));
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  return 0;
}

void CmpSeabaseDDL::createSeabaseTable(
                                       StmtDDLCreateTable * createTableNode,
                                       NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ComObjectName tableName(createTableNode->getTableName());
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;
  
  
  ElemDDLColDefArray &colArray = createTableNode->getColDefArray();
  ElemDDLColRefArray &keyArray = 
    (createTableNode->getIsConstraintPKSpecified() ?
     createTableNode->getPrimaryKeyColRefArray() :
     (createTableNode->getStoreOption() == COM_KEY_COLUMN_LIST_STORE_OPTION ?
      createTableNode->getKeyColumnArray() :
      createTableNode->getPrimaryKeyColRefArray()));

  ExeCliInterface cliInterface(STMTHEAP);
  Int32 objectOwnerID = SUPER_USER;
  Int32 schemaOwnerID = SUPER_USER;
  ComSchemaClass schemaClass;

  retcode = verifyDDLCreateOperationAuthorized(&cliInterface,
                                               SQLOperation::CREATE_TABLE,
                                               catalogNamePart, 
                                               schemaNamePart,
                                               schemaClass,
                                               objectOwnerID,
                                               schemaOwnerID);
  if (retcode != 0)
  {
     handleDDLCreateAuthorizationError(retcode,catalogNamePart,schemaNamePart);
     processReturn();
     return;
  }

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();

      return;
    }

  if ((isSeabaseReservedSchema(tableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CREATE_TABLE_NOT_ALLOWED_IN_SMD)
                          << DgTableName(extTableName);
      deallocEHI(ehi); 

      processReturn();

      return;
    }


  retcode = existsInSeabaseMDTable(&cliInterface, 
                                   catalogNamePart, schemaNamePart, objectNamePart);
  if (retcode < 0)
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  if (retcode == 1) // already exists
    {
      if (NOT createTableNode->createIfNotExists())
        {
          if (createTableNode->isVolatile())
            *CmpCommon::diags() << DgSqlCode(-1390)
                                << DgString0(objectNamePart);
          else
            *CmpCommon::diags() << DgSqlCode(-1390)
                                << DgString0(extTableName);
        }

      deallocEHI(ehi); 

      processReturn();

      return;
    }

  if (createTableNode->getIsLikeOptionSpecified())
    {
      createSeabaseTableLike(createTableNode, currCatName, currSchName);

      processReturn();

      return;
    }

  // check if SYSKEY is specified as a column name.
  NABoolean explicitSyskeySpecified = FALSE;
  for (Lng32 i = 0; i < colArray.entries(); i++)
    {
      if (colArray[i]->getColumnName() == "SYSKEY")
        explicitSyskeySpecified = TRUE;
    }

  NABoolean implicitPK = FALSE;

  NAString syskeyColName("SYSKEY");
  SQLLargeInt * syskeyType = new(STMTHEAP) SQLLargeInt(TRUE, FALSE, STMTHEAP);
  ElemDDLColDef syskeyColDef(syskeyColName, syskeyType, NULL, NULL,
                             STMTHEAP);
  ElemDDLColRef edcr("SYSKEY", COM_ASCENDING_ORDER);
  CollIndex numSysCols = 0;
  CollIndex numSaltCols = 0;
  CollIndex numDivCols = 0;

  syskeyColDef.setColumnClass(COM_SYSTEM_COLUMN);

  if (((createTableNode->getStoreOption() == COM_KEY_COLUMN_LIST_STORE_OPTION) &&
       (NOT createTableNode->getIsConstraintPKSpecified())) ||
      (keyArray.entries() == 0))
    {
      colArray.insertAt(0, &syskeyColDef);

      keyArray.insert(&edcr);

      implicitPK = TRUE;
      numSysCols++;
    }

  if ((implicitPK) && (explicitSyskeySpecified))
    {
      *CmpCommon::diags() << DgSqlCode(-1080)
                                << DgColumnName("SYSKEY");

      deallocEHI(ehi); 

      processReturn();

      return;
    }

  int numSplits = 0;

  Lng32 numSaltPartnsFromCQD = 
    CmpCommon::getDefaultNumeric(TRAF_NUM_OF_SALT_PARTNS);
  
  if ((createTableNode->getSaltOptions()) ||
      ((numSaltPartnsFromCQD > 0) &&
       (NOT (implicitPK || explicitSyskeySpecified))))
    {
      // add a system column SALT INTEGER NOT NULL with a computed
      // default value HASH2PARTFUNC(<salting cols> FOR <num salt partitions>)
      ElemDDLSaltOptionsClause * saltOptions = createTableNode->getSaltOptions();
      ElemDDLColRefArray *saltArray = createTableNode->getSaltColRefArray();
      NAString saltExprText("HASH2PARTFUNC(");
      NABoolean firstSaltCol = TRUE;
      char numSaltPartnsStr[20];
      
      if (saltArray == NULL || saltArray->entries() == 0)
        {
          // if no salting columns are specified, use all key columns
          saltArray = &keyArray;
        }
      else
        {
          // Validate that salting columns refer to real key columns
          for (CollIndex s=0; s<saltArray->entries(); s++)
            if (keyArray.getColumnIndex((*saltArray)[s]->getColumnName()) < 0)
              {
                *CmpCommon::diags() << DgSqlCode(-1195) 
                                    << DgColumnName((*saltArray)[s]->getColumnName());
                deallocEHI(ehi); 
                processReturn();
                return;
              }
        }

      for (CollIndex i=0; i<saltArray->entries(); i++)
        {
          const NAString &colName = (*saltArray)[i]->getColumnName();
          ComAnsiNamePart cnp(colName, ComAnsiNamePart::INTERNAL_FORMAT);
          CollIndex      colIx    = colArray.getColumnIndex(colName);
          NAType         *colType = colArray[colIx]->getColumnDataType();
          NAString       typeText;
          short          rc       = colType->getMyTypeAsText(&typeText, FALSE);

          // don't include SYSKEY in the list of salt columns
          if (colName != "SYSKEY")
            {
              if (firstSaltCol)
                firstSaltCol = FALSE;
              else
                saltExprText += ",";
              saltExprText += "CAST(";
              if (NOT cnp.isDelimitedIdentifier())
                saltExprText += "\"";
              saltExprText += cnp.getExternalName();
              if (NOT cnp.isDelimitedIdentifier())
                saltExprText += "\"";
              saltExprText += " AS ";
              saltExprText += typeText;
              if (!colType->supportsSQLnull())
                saltExprText += " NOT NULL";
              saltExprText += ")";
              if (colType->getTypeQualifier() == NA_NUMERIC_TYPE &&
                  !(((NumericType *) colType)->isExact()))
                {
                  *CmpCommon::diags() << DgSqlCode(-1120);
                  deallocEHI(ehi); 
                  processReturn();
                  return;
                }
            }
          else if (saltArray != &keyArray || saltArray->entries() == 1)
            {
              // SYSKEY was explicitly specified in salt column or is the only column,
              // this is an error
              *CmpCommon::diags() << DgSqlCode(-1195) 
                                  << DgColumnName((*saltArray)[i]->getColumnName());
              deallocEHI(ehi); 
              processReturn();
              return;
            }
        }

      Lng32 numSaltPartns =
        (saltOptions ? saltOptions->getNumPartitions() : numSaltPartnsFromCQD);
      saltExprText += " FOR ";
      sprintf(numSaltPartnsStr,"%d", numSaltPartns);
      saltExprText += numSaltPartnsStr;
      saltExprText += ")";
      if (numSaltPartns <= 1 || numSaltPartns > 1024)
        {
          // number of salt partitions is out of bounds
          *CmpCommon::diags() << DgSqlCode(-1196) 
                              << DgInt0(2)
                              << DgInt1(1024);
          deallocEHI(ehi); 
          processReturn();
          return;
        }

      NAString saltColName(ElemDDLSaltOptionsClause::getSaltSysColName());
      SQLInt * saltType = new(STMTHEAP) SQLInt(FALSE, FALSE, STMTHEAP);
      ElemDDLColDefault *saltDef = 
        new(STMTHEAP) ElemDDLColDefault(
             ElemDDLColDefault::COL_COMPUTED_DEFAULT);
      saltDef->setComputedDefaultExpr(saltExprText);
      ElemDDLColDef * saltColDef =
        new(STMTHEAP) ElemDDLColDef(saltColName, saltType, saltDef, NULL,
                                    STMTHEAP);

      ElemDDLColRef * edcrs = 
        new(STMTHEAP) ElemDDLColRef(saltColName, COM_ASCENDING_ORDER);

      saltColDef->setColumnClass(COM_SYSTEM_COLUMN);

      // add this new salt column at the end
      // and also as key column 0
      colArray.insert(saltColDef);
      keyArray.insertAt(0, edcrs);
      numSysCols++;
      numSaltCols++;
      numSplits = numSaltPartns - 1;
    }

  // create table in seabase
  ParDDLFileAttrsCreateTable &fileAttribs =
    createTableNode->getFileAttributes();

  NABoolean alignedFormat = FALSE;
  if (fileAttribs.isRowFormatSpecified() == TRUE)
    {
      if (fileAttribs.getRowFormat() == ElemDDLFileAttrRowFormat::eALIGNED)
        {
          alignedFormat = TRUE;
        }
    }
  else if(CmpCommon::getDefault(TRAF_DEFAULT_ALIGNED_FORMAT) == DF_ON)
    {
      alignedFormat = TRUE;
    }

  // allow nullable clustering key or unique constraints based on the
  // CQD settings. If a volatile table is being created and cqd
  // VOLATILE_TABLE_FIND_SUITABLE_KEY is ON, then allow it.
  // If ALLOW_NULLABLE_UNIQUE_KEY_CONSTRAINT is set, then allow it.
  NABoolean allowNullableUniqueConstr = FALSE;
  if (((CmpCommon::getDefault(VOLATILE_TABLE_FIND_SUITABLE_KEY) != DF_OFF) &&
       (createTableNode->isVolatile())) ||
      (CmpCommon::getDefault(ALLOW_NULLABLE_UNIQUE_KEY_CONSTRAINT) == DF_ON))
    allowNullableUniqueConstr = TRUE;

  int numIterationsToCompleteColumnList = 1;
  Lng32 numCols = 0;
  Lng32 numKeys = 0;
  ComTdbVirtTableColumnInfo * colInfoArray = NULL;
  ComTdbVirtTableKeyInfo * keyInfoArray = NULL;
  Lng32 identityColPos = -1;

  // build colInfoArray and keyInfoArray, this may take two
  // iterations if we need to add a divisioning column
  for (int iter=0; iter < numIterationsToCompleteColumnList; iter++)
    {
      numCols = colArray.entries();
      numKeys = keyArray.entries();

      colInfoArray = new(STMTHEAP) ComTdbVirtTableColumnInfo[numCols];
      keyInfoArray = new(STMTHEAP) ComTdbVirtTableKeyInfo[numKeys];

      if (buildColInfoArray(&colArray, colInfoArray, implicitPK,
                            alignedFormat, &identityColPos))
        {
          processReturn();

          return;
        }

      if (buildKeyInfoArray(&colArray, &keyArray, colInfoArray, keyInfoArray, allowNullableUniqueConstr))
        {
          processReturn();

          return;
        }

      if (iter == 0 && createTableNode->isDivisionClauseSpecified())
        {
          // We need the colArray to be able to bind the divisioning
          // expression, check it and compute its type. Once we have the
          // type, we will add a divisioning column of that type and
          // also add that column to the key. Then we will need to go
          // through this loop once more and create the updated colArray.
          numIterationsToCompleteColumnList = 2;
          NAColumnArray *naColArrayForBindingDivExpr = new(STMTHEAP) NAColumnArray(STMTHEAP);
          NAColumnArray *keyColArrayForBindingDivExpr = new(STMTHEAP) NAColumnArray(STMTHEAP);
          ItemExprList * divExpr = createTableNode->getDivisionExprList();
          ElemDDLColRefArray *divColNamesFromDDL = createTableNode->getDivisionColRefArray();

          CmpSeabaseDDL::convertColAndKeyInfoArrays(
               numCols,
               colInfoArray,
               numKeys,
               keyInfoArray,
               naColArrayForBindingDivExpr,
               keyColArrayForBindingDivExpr);

          for (CollIndex d=0; d<divExpr->entries(); d++)
            {
              NABoolean exceptionOccurred = FALSE;
              ComColumnOrdering divKeyOrdering = COM_ASCENDING_ORDER;
              ItemExpr *boundDivExpr =
                bindDivisionExprAtDDLTime((*divExpr)[d],
                                          keyColArrayForBindingDivExpr,
                                          STMTHEAP);
              if (!boundDivExpr)
                {
                  processReturn();

                  return;
                }

              if (boundDivExpr->getOperatorType() == ITM_INVERSE)
                {
                  divKeyOrdering = COM_DESCENDING_ORDER;
                  boundDivExpr = boundDivExpr->child(0);
                  if (boundDivExpr->getOperatorType() == ITM_INVERSE)
                    {
                      // in rare cases we could have two inverse operators
                      // stacked on top of each other, indicating ascending
                      divKeyOrdering = COM_ASCENDING_ORDER;
                      boundDivExpr = boundDivExpr->child(0);
                    }
                }

              try 
                {
                  // put this into a try/catch block because it could throw
                  // an exception when type synthesis fails and that would leave
                  // the transaction begun by the DDL operation in limbo
                  boundDivExpr->synthTypeAndValueId();
                }
              catch (...)
                {
                  // diags area should be set
                  CMPASSERT(CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) > 0);
                  exceptionOccurred = TRUE;
                }

              if (exceptionOccurred ||
                  boundDivExpr->getValueId() == NULL_VALUE_ID)
                {
                  processReturn();

                  return;
                }

              if (validateDivisionByExprForDDL(boundDivExpr))
                {
                  processReturn();

                  return;
                }

              // Add a divisioning column to the list of columns and the key
              char buf[16];
              snprintf(buf, sizeof(buf), "_DIVISION_%d_", d+1);
              NAString divColName(buf);
              // if the division column name was specified in the DDL, use that instead
              if (divColNamesFromDDL && divColNamesFromDDL->entries() > d)
                divColName = (*divColNamesFromDDL)[d]->getColumnName();
              NAType * divColType =
                boundDivExpr->getValueId().getType().newCopy(STMTHEAP);
              ElemDDLColDefault *divColDefault = 
                new(STMTHEAP) ElemDDLColDefault(
                     ElemDDLColDefault::COL_COMPUTED_DEFAULT);
              NAString divExprText;
              boundDivExpr->unparse(divExprText, PARSER_PHASE, COMPUTED_COLUMN_FORMAT);
              divColDefault->setComputedDefaultExpr(divExprText);
              ElemDDLColDef * divColDef =
                new(STMTHEAP) ElemDDLColDef(divColName, divColType, divColDefault, NULL,
                                            STMTHEAP);

              ElemDDLColRef * edcrs = 
                new(STMTHEAP) ElemDDLColRef(divColName, divKeyOrdering);

              divColDef->setColumnClass(COM_SYSTEM_COLUMN);
              divColDef->setDivisionColumnFlag(TRUE);
              divColDef->setDivisionColumnSequenceNumber(d);

              // add this new divisioning column to the end of the row
              // and also to the key, right after any existing salt and divisioning columns
              colArray.insert(divColDef);
              keyArray.insertAt(numSaltCols+numDivCols, edcrs);
              numSysCols++;
              numDivCols++;
            }
        }

     } // iterate 1 or 2 times to get all columns, including divisioning columns

  Int32 keyLength = 0;

  for(CollIndex i = 0; i < keyArray.entries(); i++) 
    {
      const NAString &colName = keyArray[i]->getColumnName();
      Lng32      colIx    = colArray.getColumnIndex(colName);
      if (colIx < 0)
        {
          *CmpCommon::diags() << DgSqlCode(-1009)
                              << DgColumnName(colName);

          deallocEHI(ehi); 

          processReturn();

          return;
        }

      NAType *colType = colArray[colIx]->getColumnDataType();
      keyLength += colType->getEncodedKeyLength();
    }

  char ** encodedKeysBuffer = NULL;
  if (numSplits > 0) {

    desc_struct * colDescs = 
      convertVirtTableColumnInfoArrayToDescStructs(&tableName,
                                                   colInfoArray,
                                                   numCols) ;
    desc_struct * keyDescs = 
      convertVirtTableKeyInfoArrayToDescStructs(keyInfoArray,
                                                colInfoArray,
                                                numKeys) ;

    if (createEncodedKeysBuffer(encodedKeysBuffer,
                                colDescs, keyDescs, numSplits, numKeys, 
                                keyLength, FALSE))
      {
        processReturn();
        
        return;
      }
  }

  ComTdbVirtTableTableInfo * tableInfo = new(STMTHEAP) ComTdbVirtTableTableInfo[1];
  tableInfo->tableName = NULL;
  tableInfo->createTime = 0;
  tableInfo->redefTime = 0;
  tableInfo->objUID = 0;
  tableInfo->isAudited = (fileAttribs.getIsAudit() ? 1 : 0);
  tableInfo->validDef = 1;
  tableInfo->hbaseCreateOptions = NULL;
  
  if (fileAttribs.isOwnerSpecified())
    {
      // Fixed bug:  if BY CLAUSE specified an unregistered user, then the object
      // owner is set to 0 in metadata.  Once 0, the table could not be dropped.
      NAString owner = fileAttribs.getOwner();
      Int16 retcode =  (ComUser::getUserIDFromUserName(owner.data(),objectOwnerID));
      if (retcode == FENOTFOUND)
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_AUTHID_DOES_NOT_EXIST_ERROR)
                              << DgString0(owner.data());
          processReturn();
          return;
        }
       else if (retcode != FEOK)
         {
           *CmpCommon::diags() << DgSqlCode (-CAT_INTERNAL_EXCEPTION_ERROR)
                               << DgString0(__FILE__)
                               << DgInt0(__LINE__)
                               << DgString1("verifying grantee");
           processReturn();
           return;
         }
     if (schemaClass == COM_SCHEMA_CLASS_PRIVATE && 
         objectOwnerID != schemaOwnerID)
     {
        *CmpCommon::diags() << DgSqlCode(-CAT_BY_CLAUSE_IN_PRIVATE_SCHEMA);
     
        deallocEHI(ehi);
        processReturn();
        return;
     }
  }
  tableInfo->objOwnerID = objectOwnerID;
  tableInfo->schemaOwnerID = schemaOwnerID;

  tableInfo->numSaltPartns = (numSplits > 0 ? numSplits+1 : 0);
  tableInfo->rowFormat = (alignedFormat ? 1 : 0);

  NAList<HbaseCreateOption*> hbaseCreateOptions;
  NAString hco;

  short retVal = setupHbaseOptions(createTableNode->getHbaseOptionsClause(), 
                                   numSplits, extTableName, 
                                   hbaseCreateOptions, hco);
  if (retVal)
  {
    deallocEHI(ehi);
    processReturn();
    return;
  }

  if (alignedFormat)
    {
      hco += "ROW_FORMAT=>ALIGNED ";
    }

  tableInfo->hbaseCreateOptions = (hco.isNull() ? NULL : hco.data());
     
  Int64 objUID = -1;
  if (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
    {
      const char* v = ActiveSchemaDB()->getDefaults().
        getValue(TRAF_CREATE_TABLE_WITH_UID);
      if ((v) and (strlen(v) > 0))
        {
          objUID = str_atoi(v, strlen(v));
        }
    }

  if (updateSeabaseMDTable(&cliInterface, 
                           catalogNamePart, schemaNamePart, objectNamePart,
                           COM_BASE_TABLE_OBJECT,
                           COM_YES_LIT,
                           tableInfo,
                           numCols,
                           colInfoArray,
                           numKeys,
                           keyInfoArray,
                           0, NULL,
                           objectOwnerID,
                           schemaOwnerID,
                           objUID))
    {
      *CmpCommon::diags()
        << DgSqlCode(-CAT_UNABLE_TO_CREATE_OBJECT)
        << DgTableName(extTableName);

      deallocEHI(ehi); 
      processReturn();
      return;
    }

  if (createTableNode->getAddConstraintPK())
    {
      if (updatePKeyInfo(createTableNode->getAddConstraintPK(), 
                         catalogNamePart, schemaNamePart, objectNamePart,
                         objectOwnerID,
                         schemaOwnerID,
                         keyArray.entries(),
                         NULL,
                         NULL,
                         keyInfoArray,
                         &cliInterface))
        {
          return;
        }
    }

  if (identityColPos >= 0)
    {
      ElemDDLColDef *colDef = colArray[identityColPos];

      NAString seqName;
      SequenceGeneratorAttributes::genSequenceName
        (catalogNamePart, schemaNamePart, objectNamePart, colDef->getColumnName(),
         seqName);

      if (colDef->getSGOptions())
        {
          colDef->getSGOptions()->setFSDataType((ComFSDataType)colDef->getColumnDataType()->getFSDatatype());
          
          if (colDef->getSGOptions()->validate(2/*identity*/))
            {
              deallocEHI(ehi); 
              
              processReturn();
              
              return;
            }
        }

      SequenceGeneratorAttributes sga;
      colDef->getSGOptions()->genSGA(sga);

      NAString idOptions;
      sga.display(NULL, &idOptions, TRUE);

      char buf[4000];
      str_sprintf(buf, "create internal sequence %s.\"%s\".\"%s\" %s",
                  catalogNamePart.data(), schemaNamePart.data(), seqName.data(),
                  idOptions.data());
      
      cliRC = cliInterface.executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          
          deallocEHI(ehi); 

          processReturn();
          
          return;
        }

      CorrName cn(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
      ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

      // update datatype for this sequence
      str_sprintf(buf, "update %s.\"%s\".%s set fs_data_type = %d where seq_type = '%s' and seq_uid = (select object_uid from %s.\"%s\".\"%s\" where catalog_name = '%s' and schema_name = '%s' and object_name = '%s' and object_type = '%s') ",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_SEQ_GEN,
                  colDef->getColumnDataType()->getFSDatatype(),
                  COM_INTERNAL_SG_LIT,
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                  catalogNamePart.data(), schemaNamePart.data(), seqName.data(),
                  COM_SEQUENCE_GENERATOR_OBJECT_LIT);
      
      Int64 rowsAffected = 0;
      cliRC = cliInterface.executeImmediate(buf, NULL, NULL, FALSE, &rowsAffected);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          
          deallocEHI(ehi); 

          processReturn();
          
          return;
        }
    }

  HbaseStr hbaseTable;
  hbaseTable.val = (char*)extNameForHbase.data();
  hbaseTable.len = extNameForHbase.length();
  if (createHbaseTable(ehi, &hbaseTable, SEABASE_DEFAULT_COL_FAMILY, NULL, NULL,
                       &hbaseCreateOptions, numSplits, keyLength, 
                       encodedKeysBuffer) == -1)
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }


  // if this table has lob columns, create the lob files
  short *lobNumList = new (STMTHEAP) short[numCols];
  short *lobTypList = new (STMTHEAP) short[numCols];
  char  **lobLocList = new (STMTHEAP) char*[numCols];
  Lng32 j = 0;
  for (Int32 i = 0; i < colArray.entries(); i++)
    {
      ElemDDLColDef *column = colArray[i];
      
      Lng32 datatype = column->getColumnDataType()->getFSDatatype();
      if ((datatype == REC_BLOB) ||
	  (datatype == REC_CLOB))
	{
	  lobNumList[j] = i; //column->getColumnNumber();
	  lobTypList[j] = 
	    (short)(column->getLobStorage() == Lob_Invalid_Storage
		    ? Lob_HDFS_File : column->getLobStorage());
	  
	  //	   lobTypList[j] = (short)
	  //	     CmpCommon::getDefaultNumeric(LOB_STORAGE_TYPE); 
	  char * loc = new (STMTHEAP) char[1024];
	  
	  const char* f = ActiveSchemaDB()->getDefaults().
	    getValue(LOB_STORAGE_FILE_DIR);
	  
	  strcpy(loc, f);
	  
	  lobLocList[j] = loc;
	  j++;
	}
    }
  
  if (j > 0)
     {
       Int64 objUID = getObjectUID(&cliInterface,
				   catalogNamePart.data(), schemaNamePart.data(), 
				   objectNamePart.data(),
				   COM_BASE_TABLE_OBJECT_LIT);
       
       ComString newSchName = "\"";
       newSchName += catalogNamePart;
       newSchName.append("\".\"");
       newSchName.append(schemaNamePart);
       newSchName += "\"";
       Lng32 rc = SQL_EXEC_LOBddlInterface((char*)newSchName.data(),
					   newSchName.length(),
					   objUID,
					   j,
					   LOB_CLI_CREATE,
					   lobNumList,
					   lobTypList,
					   lobLocList);
       if (rc < 0)
	 {
	   //sss TBD need to retrive the cli diags here.
	   *CmpCommon::diags() << DgSqlCode(-CAT_CREATE_OBJECT_ERROR)
			       << DgTableName(extTableName);
	   deallocEHI(ehi); 
	   
	   processReturn();
	   
	   return;
	 }
     }
  // sss #endif



  CorrName cn(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn, FALSE);
  
  processReturn();

  return;
}

void CmpSeabaseDDL::addConstraints(
                                   ComObjectName &tableName,
                                   ComAnsiNamePart &currCatAnsiName,
                                   ComAnsiNamePart &currSchAnsiName,
                                   StmtDDLAddConstraintPK * pkConstr,
                                   StmtDDLAddConstraintUniqueArray &uniqueConstrArr,
                                   StmtDDLAddConstraintRIArray &riConstrArr,
                                   StmtDDLAddConstraintCheckArray &checkConstrArr)
{
  Lng32 cliRC = 0;

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP);

  char buf[5000];

  if (pkConstr)
    {
      StmtDDLAddConstraintUnique *uniqConstr = pkConstr;
      
      NAString uniqueName;
      genUniqueName(uniqConstr, uniqueName);
      
      ComObjectName constrName(uniqueName);
      constrName.applyDefaults(currCatAnsiName, currSchAnsiName);
      const NAString constrCatalogNamePart = constrName.getCatalogNamePartAsAnsiString();
      const NAString constrSchemaNamePart = constrName.getSchemaNamePartAsAnsiString(TRUE);
      const NAString constrObjectNamePart = constrName.getObjectNamePartAsAnsiString(TRUE);
      
      ElemDDLConstraintUnique *constraintNode = 
        ( uniqConstr->getConstraint() )->castToElemDDLConstraintUnique();
      ElemDDLColRefArray &keyColumnArray = constraintNode->getKeyColumnArray();
      
      NAString keyColNameStr;
      for (CollIndex i = 0; i < keyColumnArray.entries(); i++)
        {
          keyColNameStr += "\"";
          keyColNameStr += keyColumnArray[i]->getColumnName();
          keyColNameStr += "\"";

          if (keyColumnArray[i]->getColumnOrdering() == COM_DESCENDING_ORDER)
            keyColNameStr += "DESC";
          else
            keyColNameStr += "ASC";
 
          if (i+1 < keyColumnArray.entries())
            keyColNameStr += ", ";
        }
      
      str_sprintf(buf, "alter table \"%s\".\"%s\".\"%s\" add constraint \"%s\".\"%s\".\"%s\" unique (%s)",
                  catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
                  constrCatalogNamePart.data(), constrSchemaNamePart.data(), constrObjectNamePart.data(),
                  keyColNameStr.data());
      
      cliRC = cliInterface.executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          
          processReturn();
          
          goto label_return;
        }
    }
  
  if (uniqueConstrArr.entries() > 0)
    {
      for (Lng32 i = 0; i < uniqueConstrArr.entries(); i++)
        {
          StmtDDLAddConstraintUnique *uniqConstr = 
            uniqueConstrArr[i];

          NAString uniqueName;
          genUniqueName(uniqConstr, uniqueName);

          ComObjectName constrName(uniqueName);
          constrName.applyDefaults(currCatAnsiName, currSchAnsiName);
          const NAString constrCatalogNamePart = constrName.getCatalogNamePartAsAnsiString();
          const NAString constrSchemaNamePart = constrName.getSchemaNamePartAsAnsiString(TRUE);
          const NAString constrObjectNamePart = constrName.getObjectNamePartAsAnsiString(TRUE);

          ElemDDLConstraintUnique *constraintNode = 
            ( uniqConstr->getConstraint() )->castToElemDDLConstraintUnique();
          ElemDDLColRefArray &keyColumnArray = constraintNode->getKeyColumnArray();
 
          NAString keyColNameStr;
          for (CollIndex i = 0; i < keyColumnArray.entries(); i++)
            {
              keyColNameStr += "\"";
              keyColNameStr += keyColumnArray[i]->getColumnName();
              keyColNameStr += "\"";

              if (keyColumnArray[i]->getColumnOrdering() == COM_DESCENDING_ORDER)
                keyColNameStr += "DESC";
              else
                keyColNameStr += "ASC";

              if (i+1 < keyColumnArray.entries())
                keyColNameStr += ", ";
            }

          str_sprintf(buf, "alter table \"%s\".\"%s\".\"%s\" add constraint \"%s\".\"%s\".\"%s\" unique (%s)",
                      catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
                      constrCatalogNamePart.data(), constrSchemaNamePart.data(), constrObjectNamePart.data(),
                      keyColNameStr.data());
                      
          cliRC = cliInterface.executeImmediate(buf);
          if (cliRC < 0)
            {
              cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
              
              processReturn();
              
              goto label_return;
            }

        } // for
    } // if

  if (riConstrArr.entries() > 0)
    {
      for (Lng32 i = 0; i < riConstrArr.entries(); i++)
        {
          StmtDDLAddConstraintRI *refConstr = riConstrArr[i];

          ComObjectName refdTableName(refConstr->getReferencedTableName(), COM_TABLE_NAME);
          refdTableName.applyDefaults(currCatAnsiName, currSchAnsiName);
          const NAString refdCatNamePart = refdTableName.getCatalogNamePartAsAnsiString();
          const NAString refdSchNamePart = refdTableName.getSchemaNamePartAsAnsiString(TRUE);
          const NAString refdObjNamePart = refdTableName.getObjectNamePartAsAnsiString(TRUE);

          NAString uniqueName;
          genUniqueName(refConstr, uniqueName);

          ComObjectName constrName(uniqueName);
          constrName.applyDefaults(currCatAnsiName, currSchAnsiName);
          const NAString constrCatalogNamePart = constrName.getCatalogNamePartAsAnsiString();
          const NAString constrSchemaNamePart = constrName.getSchemaNamePartAsAnsiString(TRUE);
          const NAString constrObjectNamePart = constrName.getObjectNamePartAsAnsiString(TRUE);
          const NAString &addConstrName = constrName.getExternalName();

          ElemDDLConstraintRI *constraintNode = 
            ( refConstr->getConstraint() )->castToElemDDLConstraintRI();
          ElemDDLColNameArray &ringColumnArray = constraintNode->getReferencingColumns();
 
          NAString ringColNameStr;
          for (CollIndex i = 0; i < ringColumnArray.entries(); i++)
            {
              ringColNameStr += "\"";
              ringColNameStr += ringColumnArray[i]->getColumnName();
              ringColNameStr += "\"";
              if (i+1 < ringColumnArray.entries())
                ringColNameStr += ", ";
            }

          ElemDDLColNameArray &refdColumnArray = constraintNode->getReferencedColumns();
 
          NAString refdColNameStr;
          if (refdColumnArray.entries() > 0)
            refdColNameStr = "(";
          for (CollIndex i = 0; i < refdColumnArray.entries(); i++)
            {
              refdColNameStr += "\"";
              refdColNameStr += refdColumnArray[i]->getColumnName();
              refdColNameStr += "\"";         
              if (i+1 < refdColumnArray.entries())
                refdColNameStr += ", ";
            }
          if (refdColumnArray.entries() > 0)
            refdColNameStr += ")";

          str_sprintf(buf, "alter table \"%s\".\"%s\".\"%s\" add constraint \"%s\".\"%s\".\"%s\" foreign key (%s) references \"%s\".\"%s\".\"%s\" %s %s",
                      catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
                      constrCatalogNamePart.data(), constrSchemaNamePart.data(), constrObjectNamePart.data(),
                      ringColNameStr.data(),
                      refdCatNamePart.data(), refdSchNamePart.data(), refdObjNamePart.data(),
                      (refdColumnArray.entries() > 0 ? refdColNameStr.data() : " "),
                      (NOT constraintNode->isEnforced() ? " not enforced " : ""));
                      
          cliRC = cliInterface.executeImmediate(buf);
          if (cliRC < 0)
            {
              cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
              
              processReturn();
              
            }

          CorrName cn2(refdObjNamePart.data(),
                       STMTHEAP,
                       refdSchNamePart.data(),
                       refdCatNamePart.data());

          // remove natable for the table being referenced
          ActiveSchemaDB()->getNATableDB()->removeNATable(cn2);

          if (cliRC < 0)
            goto label_return;

          if (NOT constraintNode->isEnforced())
            {
              *CmpCommon::diags()
                << DgSqlCode(1313)
                << DgString0(addConstrName);
            }
          
        } // for
    } // if

  if (checkConstrArr.entries() > 0)
    {
      for (Lng32 i = 0; i < checkConstrArr.entries(); i++)
        {
          StmtDDLAddConstraintCheck *checkConstr = checkConstrArr[i];

          NAString uniqueName;
          genUniqueName(checkConstr, uniqueName);

          ComObjectName constrName(uniqueName);
          constrName.applyDefaults(currCatAnsiName, currSchAnsiName);
          const NAString constrCatalogNamePart = constrName.getCatalogNamePartAsAnsiString();
          const NAString constrSchemaNamePart = constrName.getSchemaNamePartAsAnsiString(TRUE);
          const NAString constrObjectNamePart = constrName.getObjectNamePartAsAnsiString(TRUE);

          NAString constrText;
          getCheckConstraintText(checkConstr, constrText);
          str_sprintf(buf, "alter table \"%s\".\"%s\".\"%s\" add constraint \"%s\".\"%s\".\"%s\" check %s",
                      catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
                      constrCatalogNamePart.data(), constrSchemaNamePart.data(), constrObjectNamePart.data(),
                      constrText.data()
                      );
                      
          cliRC = cliInterface.executeImmediate(buf);
          if (cliRC < 0)
            {
              cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
              
              processReturn();
              
              goto label_return;
            }
        }
    }

 label_return:
  // remove NATable cache entries for this table
  CorrName cn(objectNamePart.data(),
              STMTHEAP,
              schemaNamePart.data(),
              catalogNamePart.data());
  
  // remove NATable for this table
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  return;
}

void CmpSeabaseDDL::createSeabaseTableCompound(
                                       StmtDDLCreateTable * createTableNode,
                                       NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;
  ExeCliInterface cliInterface(STMTHEAP);

  ComObjectName tableName(createTableNode->getTableName());
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);

  if ((createTableNode->isVolatile()) &&
      ((createTableNode->getAddConstraintUniqueArray().entries() > 0) ||
       (createTableNode->getAddConstraintRIArray().entries() > 0) ||
       (createTableNode->getAddConstraintCheckArray().entries() > 0)))
    {
      *CmpCommon::diags() << DgSqlCode(-1283);
      
      processReturn();
      
      goto label_error;
    }

  cliRC = beginXn(&cliInterface);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }

  createSeabaseTable(createTableNode, currCatName, currSchName);
  if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_))
    {
      rollbackXn(&cliInterface);

      return;
    }

  cliRC = commitXn(&cliInterface);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      
      goto label_error;
    }

  cliRC = cliInterface.holdAndSetCQD("TRAF_NO_CONSTR_VALIDATION", "ON");
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      
      processReturn();
      
      goto label_error;
    }

  addConstraints(tableName, currCatAnsiName, currSchAnsiName,
                 NULL,
                 createTableNode->getAddConstraintUniqueArray(),
                 createTableNode->getAddConstraintRIArray(),
                 createTableNode->getAddConstraintCheckArray());

  if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_))
    {
      if (cliInterface.statusXn() == 0) // xn in progress
        {
          rollbackXn(&cliInterface);
        }

      *CmpCommon::diags() << DgSqlCode(-1029)
                          << DgTableName(extTableName);
      
      processReturn();
      
      goto label_error;
    }

  cliRC = cliInterface.restoreCQD("traf_no_constr_validation");

  return;

 label_error:
  cliRC = cliInterface.restoreCQD("traf_no_constr_validation");

  if (NOT createTableNode->isVolatile())
    {
      char buf [1000];

      cliRC = cliInterface.holdAndSetCQD("TRAF_RELOAD_NATABLE_CACHE", "ON");
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          
          processReturn();
          
          return;
        }

      str_sprintf(buf, "drop table if exists \"%s\".\"%s\".\"%s\" cascade",
                  catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data());
      
      cliRC = cliInterface.executeImmediate(buf);

      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

          cliRC = cliInterface.restoreCQD("TRAF_RELOAD_NATABLE_CACHE");
          
          processReturn();
          
          return;
        }

      cliRC = cliInterface.restoreCQD("TRAF_RELOAD_NATABLE_CACHE");
    }
}

void CmpSeabaseDDL::dropSeabaseTable(
                                     StmtDDLDropTable                  * dropTableNode,
                                     NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  NAString tabName = (NAString&)dropTableNode->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComObjectName volTabName; 
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
 

  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  if ((isSeabaseReservedSchema(tableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_CANNOT_DROP_SMD_TABLE)
                          << DgTableName(extTableName);
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  NABoolean isVolatile = FALSE;

  if ((dropTableNode->isVolatile()) &&
      (CmpCommon::context()->sqlSession()->volatileSchemaInUse()))
    {
      volTabName = tableName;
      isVolatile = TRUE;
    }
  
  if ((NOT dropTableNode->isVolatile()) &&
      (CmpCommon::context()->sqlSession()->volatileSchemaInUse()))
    {

      // updateVolatileQualifiedName qualifies the object name with a 
      // volatile catalog and schema name (if a volatile schema exists)
      QualifiedName *qn =
        CmpCommon::context()->sqlSession()->
        updateVolatileQualifiedName
        (dropTableNode->getOrigTableNameAsQualifiedName().getObjectName());
      
      // don't believe it is possible to get a null pointer returned
      if (qn == NULL)
        {
          *CmpCommon::diags()
            << DgSqlCode(-CAT_UNABLE_TO_DROP_OBJECT)
            << DgTableName(dropTableNode->getOrigTableNameAsQualifiedName().
                           getQualifiedNameAsAnsiString(TRUE));

          processReturn();

          return;
        }
      
        volTabName = qn->getQualifiedNameAsAnsiString();
        volTabName.applyDefaults(currCatAnsiName, currSchAnsiName);

        NAString vtCatNamePart = volTabName.getCatalogNamePartAsAnsiString();
        NAString vtSchNamePart = volTabName.getSchemaNamePartAsAnsiString(TRUE);
        NAString vtObjNamePart = volTabName.getObjectNamePartAsAnsiString(TRUE);

        retcode = existsInSeabaseMDTable(&cliInterface, 
                                         vtCatNamePart, vtSchNamePart, vtObjNamePart,
                                         COM_BASE_TABLE_OBJECT);

        if (retcode < 0)
          {
            processReturn();
            
            return;
          }

        if (retcode == 1)
          {
            // table found in volatile schema
            // Validate volatile table name.
            if (CmpCommon::context()->sqlSession()->
                validateVolatileQualifiedName
                (dropTableNode->getOrigTableNameAsQualifiedName()))
              {
                // Valid volatile table. Drop it.
                tabName = volTabName.getExternalName(TRUE);

                catalogNamePart = vtCatNamePart;
                schemaNamePart = vtSchNamePart;
                objectNamePart = vtObjNamePart;

                isVolatile = TRUE;
              }
            else
              {
                // volatile table found but the name is not a valid
                // volatile name. Look for the input name in the regular
                // schema.
                // But first clear the diags area.
                CmpCommon::diags()->clear();
              }
          }
        else
          {
            CmpCommon::diags()->clear();
          }
      }

  retcode = existsInSeabaseMDTable(&cliInterface, 
                                   catalogNamePart, schemaNamePart, objectNamePart,
                                   COM_BASE_TABLE_OBJECT);
  if (retcode < 0)
    {
      processReturn();

      return;
    }

  if (retcode == 0) // does not exist
    {
      if (NOT dropTableNode->dropIfExists())
        {
          CmpCommon::diags()->clear();

          if (isVolatile)
            *CmpCommon::diags() << DgSqlCode(-1389)
                                << DgString0(objectNamePart);
          else
            *CmpCommon::diags() << DgSqlCode(-1389)
                                << DgString0(extTableName);
        }

      processReturn();

      return;
    }

  // Check to see if the user has the authority to drop the table
  ComObjectName verifyName;
  if (isVolatile)
     verifyName = volTabName;
  else
     verifyName = tableName;

  if (CmpCommon::getDefault(TRAF_RELOAD_NATABLE_CACHE) == DF_OFF)
    ActiveSchemaDB()->getNATableDB()->useCache();

 // save the current parserflags setting
  ULng32 savedParserFlags = Get_SqlParser_Flags (0xFFFFFFFF);
  Set_SqlParser_Flags(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);

  

  CorrName cn(objectNamePart,
              STMTHEAP,
              schemaNamePart,
              catalogNamePart);

  NATable *naTable = bindWA.getNATable(cn); 
 
  const NAColumnArray &nacolArr =  naTable->getNAColumnArray();
  // Restore parser flags settings to what they originally were
  Set_SqlParser_Flags (savedParserFlags);

  if (naTable == NULL || bindWA.errStatus())
    {
      CmpCommon::diags()->clear();

      if (NOT dropTableNode->dropIfExists())
        {
          CmpCommon::diags()->clear();
          
          if (isVolatile)
            *CmpCommon::diags() << DgSqlCode(-1389)
                                << DgString0(objectNamePart);
          else
            *CmpCommon::diags() << DgSqlCode(-1389)
                                << DgString0(extTableName);
        }
      
      processReturn();

      return;
    }

  if ((dropTableNode->isVolatile()) && 
      (NOT CmpCommon::context()->sqlSession()->isValidVolatileSchemaName(schemaNamePart)))
    {
      *CmpCommon::diags() << DgSqlCode(-1279);

      processReturn();

      return;
    }

  Int64 objUID = getObjectUID(&cliInterface,
                              catalogNamePart.data(), schemaNamePart.data(), 
                              objectNamePart.data(),
                              COM_BASE_TABLE_OBJECT_LIT);
  if (objUID < 0)
    {

      processReturn();

      return;
    }

  // Make sure user has necessary privileges to perform drop
  if (!isDDLOperationAuthorized(SQLOperation::DROP_TABLE,
                                naTable->getOwner(),
                                naTable->getSchemaOwner()))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);

     processReturn ();

     return;
  }

  Queue * usingViewsQueue = NULL;
  if (dropTableNode->getDropBehavior() == COM_RESTRICT_DROP_BEHAVIOR)
    {
      NAString usingObjName;
      cliRC = getUsingObject(&cliInterface, objUID, usingObjName);
      if (cliRC < 0)
        {
          processReturn();
          
          return;
        }

      if (cliRC != 100) // found an object
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_DEPENDENT_VIEW_EXISTS)
                              << DgTableName(usingObjName);

          processReturn();

          return;
        }
    }
  else if (dropTableNode->getDropBehavior() == COM_CASCADE_DROP_BEHAVIOR)
    {
      cliRC = getUsingViews(&cliInterface, extTableName, TRUE, usingViewsQueue);
      if (cliRC < 0)
        {
          processReturn();
          
          return;
        }
    }

  // return error if cascade is not specified and a referential constraint exists on
  // any of the unique constraints.
  const AbstractRIConstraintList &uniqueList = naTable->getUniqueConstraints();
  
  if (dropTableNode->getDropBehavior() == COM_RESTRICT_DROP_BEHAVIOR)
    {
      for (Int32 i = 0; i < uniqueList.entries(); i++)
        {
          AbstractRIConstraint *ariConstr = uniqueList[i];
          
          if (ariConstr->getOperatorType() != ITM_UNIQUE_CONSTRAINT)
            continue;
          
          UniqueConstraint * uniqConstr = (UniqueConstraint*)ariConstr;

          if (uniqConstr->hasRefConstraintsReferencingMe())
            {
              const ComplementaryRIConstraint * rc = uniqConstr->getRefConstraintReferencingMe(0);
              
              const NAString &constrName = 
                (rc ? rc->getConstraintName().getObjectName() : " ");
              *CmpCommon::diags() << DgSqlCode(-1059)
                                  << DgConstraintName(constrName);
              
              processReturn();
              
              return;
            }
        }
    }

  // Drop referencing objects
  char query[4000];

  if (usingViewsQueue)
    {
      usingViewsQueue->position();
      for (int idx = 0; idx < usingViewsQueue->numEntries(); idx++)
        {
          OutputInfo * vi = (OutputInfo*)usingViewsQueue->getNext(); 
          
          char * viewName = vi->get(0);
          
          if (dropSeabaseObject(ehi, viewName,
                                currCatName, currSchName, COM_VIEW_OBJECT))
            {
              processReturn();
              
              return;
            }
        }
    }

  // drop all referential constraints referencing me.
  for (Int32 i = 0; i < uniqueList.entries(); i++)
    {
      AbstractRIConstraint *ariConstr = uniqueList[i];
      
      if (ariConstr->getOperatorType() != ITM_UNIQUE_CONSTRAINT)
        continue;

      UniqueConstraint * uniqConstr = (UniqueConstraint*)ariConstr;

     // We will only reach here is cascade option is specified.
      // drop all constraints referencing me.
      if (uniqConstr->hasRefConstraintsReferencingMe())
        {
          cliRC = cliInterface.holdAndSetCQD("TRAF_RELOAD_NATABLE_CACHE", "ON");
          if (cliRC < 0)
            {
              cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
              
              processReturn();
              
              return;
            }

          for (Lng32 j = 0; j < uniqConstr->getNumRefConstraintsReferencingMe(); j++)
            {
              const ComplementaryRIConstraint * rc = 
                uniqConstr->getRefConstraintReferencingMe(j);

              str_sprintf(query, "alter table \"%s\".\"%s\".\"%s\" drop constraint \"%s\".\"%s\".\"%s\"",
                          rc->getTableName().getCatalogName().data(),
                          rc->getTableName().getSchemaName().data(),
                          rc->getTableName().getObjectName().data(),
                          rc->getConstraintName().getCatalogName().data(),
                          rc->getConstraintName().getSchemaName().data(),
                          rc->getConstraintName().getObjectName().data());

              cliRC = cliInterface.executeImmediate(query);

              if (cliRC < 0)
                {
                  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

                  cliRC = cliInterface.restoreCQD("TRAF_RELOAD_NATABLE_CACHE");
                  
                  processReturn();
                  
                  return;
                }
              
            } // for
          
          cliRC = cliInterface.restoreCQD("TRAF_RELOAD_NATABLE_CACHE");
          
        } // if
    } // for

  // drop all unique constraints from metadata
  for (Int32 i = 0; i < uniqueList.entries(); i++)
    {
      AbstractRIConstraint *ariConstr = uniqueList[i];
      
      if (ariConstr->getOperatorType() != ITM_UNIQUE_CONSTRAINT)
        continue;

      UniqueConstraint * uniqConstr = (UniqueConstraint*)ariConstr;

      const NAString& constrCatName = 
        uniqConstr->getConstraintName().getCatalogName();

      const NAString& constrSchName = 
        uniqConstr->getConstraintName().getSchemaName();
      
      const NAString& constrObjName = 
        uniqConstr->getConstraintName().getObjectName();
      
      Int64 constrUID = getObjectUID(&cliInterface,
                                     constrCatName.data(), constrSchName.data(), constrObjName.data(),
                                     (uniqConstr->isPrimaryKeyConstraint() ?
                                      COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT :
                                      COM_UNIQUE_CONSTRAINT_OBJECT_LIT));                
      if (constrUID < 0)
        {
          processReturn();
          
          return;
        }

      if (deleteConstraintInfoFromSeabaseMDTables(&cliInterface,
                                                  naTable->objectUid().castToInt64(),
                                                  0,
                                                  constrUID,
                                                  0,
                                                  constrCatName,
                                                  constrSchName,
                                                  constrObjName,
                                                  (uniqConstr->isPrimaryKeyConstraint() ?
                                                   COM_PRIMARY_KEY_CONSTRAINT_OBJECT :
                                                   COM_UNIQUE_CONSTRAINT_OBJECT)))
        {
          processReturn();
          
          return;
        }
     }

  // drop all referential constraints from metadata
  const AbstractRIConstraintList &refList = naTable->getRefConstraints();
  
  for (Int32 i = 0; i < refList.entries(); i++)
    {
      AbstractRIConstraint *ariConstr = refList[i];
      
      if (ariConstr->getOperatorType() != ITM_REF_CONSTRAINT)
        continue;

      RefConstraint * refConstr = (RefConstraint*)ariConstr;

      const NAString& constrCatName = 
        refConstr->getConstraintName().getCatalogName();

      const NAString& constrSchName = 
        refConstr->getConstraintName().getSchemaName();
      
      const NAString& constrObjName = 
        refConstr->getConstraintName().getObjectName();
      
      Int64 constrUID = getObjectUID(&cliInterface,
                                     constrCatName.data(), constrSchName.data(), constrObjName.data(),
                                     COM_REFERENTIAL_CONSTRAINT_OBJECT_LIT);             
      if (constrUID < 0)
        {
          processReturn();
          
          return;
        }

      NATable *otherNaTable = NULL;
      
      //      CorrName otherCN(refConstr->getOtherTableName());
      CorrName otherCN(refConstr->getUniqueConstraintReferencedByMe().getTableName());

      otherNaTable = bindWA.getNATable(otherCN);
      if (otherNaTable == NULL || bindWA.errStatus())
        {
          deallocEHI(ehi); 
          
          processReturn();
          
          return;
        }

      AbstractRIConstraint * otherConstr = 
        refConstr->findConstraint(&bindWA, refConstr->getUniqueConstraintReferencedByMe());

      const NAString& otherSchName = 
        otherConstr->getConstraintName().getSchemaName();
      
      const NAString& otherConstrName = 
        otherConstr->getConstraintName().getObjectName();
      
      Int64 otherConstrUID = getObjectUID(&cliInterface,
                                          constrCatName.data(), otherSchName.data(), otherConstrName.data(),
                                          COM_UNIQUE_CONSTRAINT_OBJECT_LIT );
      if (otherConstrUID < 0)
        {
          CmpCommon::diags()->clear();
          otherConstrUID = getObjectUID(&cliInterface,
                                        constrCatName.data(), otherSchName.data(), otherConstrName.data(),
                                        COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT );
          if (otherConstrUID < 0)
            {
              processReturn();
              
              return;
            }
        }
      
      if (deleteConstraintInfoFromSeabaseMDTables(&cliInterface,
                                                  naTable->objectUid().castToInt64(),
                                                  otherNaTable->objectUid().castToInt64(),
                                                  constrUID,
                                                  otherConstrUID,
                                                  constrCatName,
                                                  constrSchName,
                                                  constrObjName,
                                                  COM_REFERENTIAL_CONSTRAINT_OBJECT))
        {
          processReturn();
          
          return;
        }
      
    }

  // drop all check constraints from metadata if 'no check' is not specified.
  if (NOT (dropTableNode->getDropBehavior() == COM_NO_CHECK_DROP_BEHAVIOR))
    {
      const CheckConstraintList &checkList = naTable->getCheckConstraints();
      for (Int32 i = 0; i < checkList.entries(); i++)
        {
          CheckConstraint *checkConstr = checkList[i];
          
          const NAString& constrCatName = 
            checkConstr->getConstraintName().getCatalogName();
          
          const NAString& constrSchName = 
            checkConstr->getConstraintName().getSchemaName();
          
          const NAString& constrObjName = 
            checkConstr->getConstraintName().getObjectName();
          
          Int64 constrUID = getObjectUID(&cliInterface,
                                         constrCatName.data(), constrSchName.data(), constrObjName.data(),
                                         COM_CHECK_CONSTRAINT_OBJECT_LIT);
          if (constrUID < 0)
            {
              processReturn();
              
              return;
            }
          
          if (deleteConstraintInfoFromSeabaseMDTables(&cliInterface,
                                                      naTable->objectUid().castToInt64(),
                                                      0,
                                                      constrUID,
                                                      0,
                                                      constrCatName,
                                                      constrSchName,
                                                      constrObjName,
                                                      COM_CHECK_CONSTRAINT_OBJECT))
            {
              processReturn();
              
              return;
            }
        }
    }

  const NAFileSetList &indexList = naTable->getIndexList();

  // first drop all index objects from metadata.
  Queue * indexInfoQueue = NULL;
  if (getAllIndexes(&cliInterface, objUID, TRUE, indexInfoQueue))
    {
      processReturn();
      return;
    }

  SQL_QIKEY *qiKeys = new (STMTHEAP) SQL_QIKEY[indexInfoQueue->numEntries()];
  indexInfoQueue->position();
  for (int idx = 0; idx < indexInfoQueue->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)indexInfoQueue->getNext(); 
      
      NAString idxCatName = (char*)vi->get(0);
      NAString idxSchName = (char*)vi->get(1);
      NAString idxObjName = (char*)vi->get(2);

      // set up a qiKey for this index, later we will removed the
      // index cache entry from concurrent processes
      Int64 objUID = *(Int64*)vi->get(3);
      qiKeys[idx].ddlObjectUID = objUID;
      qiKeys[idx].operation[0] = 'O';
      qiKeys[idx].operation[1] = 'R';
         
      NAString qCatName = "\"" + idxCatName + "\"";
      NAString qSchName = "\"" + idxSchName + "\"";
      NAString qObjName = "\"" + idxObjName + "\"";

      ComObjectName coName(qCatName, qSchName, qObjName);
      NAString ansiName = coName.getExternalName(TRUE);

      if (dropSeabaseObject(ehi, ansiName,
                            idxCatName, idxSchName, COM_INDEX_OBJECT, TRUE, FALSE))
        {
          NADELETEBASIC (qiKeys, STMTHEAP);

          processReturn();
          
          return;
        }

    } // for

  // Remove index entries from other processes cache
  // Fix for bug 1396774 & bug 1396746
  if (indexInfoQueue->numEntries() > 0)
    SQL_EXEC_SetSecInvalidKeys(indexInfoQueue->numEntries(), qiKeys);
  NADELETEBASIC (qiKeys, STMTHEAP);

  // if there is an identity column, drop sequence corresponding to it.
  NABoolean found = FALSE;
  Lng32 idPos = 0;
  NAColumn *col = NULL;
  while ((NOT found) && (idPos < naTable->getColumnCount()))
    {

      col = naTable->getNAColumnArray()[idPos];
      if (col->isIdentityColumn())
        {
          found = TRUE;
          continue;
        }

      idPos++;
    }

  if (found)
    {
      NAString seqName;
      SequenceGeneratorAttributes::genSequenceName
        (catalogNamePart, schemaNamePart, objectNamePart, col->getColName(),
         seqName);
      
    char buf[4000];
      str_sprintf(buf, "drop sequence %s.\"%s\".\"%s\"",
                  catalogNamePart.data(), schemaNamePart.data(), seqName.data());
      
      cliRC = cliInterface.executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          
          deallocEHI(ehi); 
          
          processReturn();
          
          return;
        }

    }

  // drop SB_HISTOGRAMS and SB_HISTOGRAM_INTERVALS entries, if any
  // if the table that we are dropping itself is not a SB_HISTOGRAMS or SB_HISTOGRAM_INTERVALS table
  if (objectNamePart != "SB_HISTOGRAMS" && 
      objectNamePart != "SB_HISTOGRAM_INTERVALS")
  {
    if (dropSeabaseStats(&cliInterface,
                         catalogNamePart.data(),
                         schemaNamePart.data(),
                         objUID))
    {
      processReturn();
      return;
    }
  }

  // if metadata drop succeeds, drop indexes from hbase.
  indexInfoQueue->position();
  for (int idx = 0; idx < indexInfoQueue->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)indexInfoQueue->getNext(); 
      
      NAString idxCatName = (char*)vi->get(0);
      NAString idxSchName = (char*)vi->get(1);
      NAString idxObjName = (char*)vi->get(2);

      NAString qCatName = "\"" + idxCatName + "\"";
      NAString qSchName = "\"" + idxSchName + "\"";
      NAString qObjName = "\"" + idxObjName + "\"";

      ComObjectName coName(qCatName, qSchName, qObjName);
      NAString ansiName = coName.getExternalName(TRUE);

      if (dropSeabaseObject(ehi, ansiName,
                            idxCatName, idxSchName, COM_INDEX_OBJECT, FALSE, TRUE))
        {
          processReturn();
          
          return;
        }


      CorrName cni(qObjName, STMTHEAP, qSchName, qCatName);
      ActiveSchemaDB()->getNATableDB()->removeNATable(cni);
      cni.setSpecialType(ExtendedQualName::INDEX_TABLE);

      ActiveSchemaDB()->getNATableDB()->removeNATable(cni);

    } // for

  // If blob/clob columns are present, drop all the dependent files.

  Lng32 numCols = nacolArr.entries();
  
  // if this table has lob columns, drop the lob files
  short *lobNumList = new (STMTHEAP) short[numCols];
  short *lobTypList = new (STMTHEAP) short[numCols];
  char  **lobLocList = new (STMTHEAP) char*[numCols];
  Lng32 j = 0;
  for (Int32 i = 0; i < nacolArr.entries(); i++)
    {
      NAColumn *naColumn = nacolArr[i];
      
      Lng32 datatype = naColumn->getType()->getFSDatatype();
      if ((datatype == REC_BLOB) ||
	  (datatype == REC_CLOB))
	{
	  lobNumList[j] = i; //column->getColumnNumber();
	  lobTypList[j] = 
	    (short)(naColumn->lobStorageType() == Lob_Invalid_Storage
		    ? Lob_HDFS_File : naColumn->lobStorageType());
	  
	  //	   lobTypList[j] = (short)
	  //	     CmpCommon::getDefaultNumeric(LOB_STORAGE_TYPE); 
	  char * loc = new (STMTHEAP) char[1024];
	  
	  const char* f = ActiveSchemaDB()->getDefaults().
	    getValue(LOB_STORAGE_FILE_DIR);
	  
	  strcpy(loc, f);
	  
	  lobLocList[j] = loc;
	  j++;
	}
    }
  if (j > 0)
    {
      Int64 objUID = getObjectUID(&cliInterface,
				  catalogNamePart.data(), schemaNamePart.data(), 
				  objectNamePart.data(),
				  COM_BASE_TABLE_OBJECT_LIT);
       
      ComString newSchName = "\"";
      newSchName += catalogNamePart;
      newSchName.append("\".\"");
      newSchName.append(schemaNamePart);
      newSchName += "\"";
      Lng32 rc = SQL_EXEC_LOBddlInterface((char*)newSchName.data(),
					  newSchName.length(),
					  objUID,
					  j,
					  LOB_CLI_DROP,
					  lobNumList,
					  lobTypList,
					  lobLocList);
      if (rc < 0)
	{
	  *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_DROP_OBJECT)
			      << DgTableName(extTableName);
	  deallocEHI(ehi); 
	   
	  processReturn();
	   
	  return;
	}
    }
  
   
  //Finally drop the table

  if (dropSeabaseObject(ehi, tabName,
                        currCatName, currSchName, COM_BASE_TABLE_OBJECT))
    {
      processReturn();
      
      return;
    }
 
  processReturn();

  CorrName cn2(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn2);
  
  for (Int32 i = 0; i < refList.entries(); i++)
    {
      AbstractRIConstraint *ariConstr = refList[i];
      
      if (ariConstr->getOperatorType() != ITM_REF_CONSTRAINT)
        continue;
      
      RefConstraint * refConstr = (RefConstraint*)ariConstr;
      CorrName otherCN(refConstr->getUniqueConstraintReferencedByMe().getTableName());
      
      ActiveSchemaDB()->getNATableDB()->removeNATable(otherCN);
    }

  for (Int32 i = 0; i < uniqueList.entries(); i++)
    {
      UniqueConstraint * uniqConstr = (UniqueConstraint*)uniqueList[i];

      // We will only reach here is cascade option is specified.
      // drop all constraints referencing me.
      if (uniqConstr->hasRefConstraintsReferencingMe())
        {
          for (Lng32 j = 0; j < uniqConstr->getNumRefConstraintsReferencingMe(); j++)
            {
              const ComplementaryRIConstraint * rc = 
                uniqConstr->getRefConstraintReferencingMe(j);

              // remove this ref constr entry from natable cache
              CorrName cnr(rc->getTableName().getObjectName().data(), STMTHEAP, 
                           rc->getTableName().getSchemaName().data(),
                           rc->getTableName().getCatalogName().data());
              ActiveSchemaDB()->getNATableDB()->removeNATable(cnr);
            } // for
          
        } // if
    } // for
}

void CmpSeabaseDDL::renameSeabaseTable(
                                       StmtDDLAlterTableRename * renameTableNode,
                                       NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ComObjectName tableName(renameTableNode->getTableName());
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ComObjectName newTableName(renameTableNode->getNewNameAsAnsiString());
  //  newTableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  newTableName.applyDefaults(catalogNamePart, schemaNamePart);
  const NAString newObjectNamePart = newTableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString newExtTableName = newTableName.getExternalName(TRUE);
  const NAString newExtNameForHbase = catalogNamePart + "." + schemaNamePart + "." + newObjectNamePart;

  ExeCliInterface cliInterface(STMTHEAP);
  
  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();

      return;
    }

  if (isSeabaseReservedSchema(tableName))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CREATE_TABLE_NOT_ALLOWED_IN_SMD)
                          << DgTableName(extTableName);
      deallocEHI(ehi); 

      processReturn();

      return;
    }
  
  if (CmpCommon::context()->sqlSession()->volatileSchemaInUse())
    {
      QualifiedName *qn =
        CmpCommon::context()->sqlSession()->
        updateVolatileQualifiedName
        (renameTableNode->getTableNameAsQualifiedName().getObjectName());
      
      if (qn == NULL)
        {
          *CmpCommon::diags()
            << DgSqlCode(-1427);
          
          processReturn();
          
          return;
        }
      
      ComObjectName volTabName (qn->getQualifiedNameAsAnsiString());
      volTabName.applyDefaults(currCatAnsiName, currSchAnsiName);
      
      NAString vtCatNamePart = volTabName.getCatalogNamePartAsAnsiString();
      NAString vtSchNamePart = volTabName.getSchemaNamePartAsAnsiString(TRUE);
      NAString vtObjNamePart = volTabName.getObjectNamePartAsAnsiString(TRUE);
      
      retcode = existsInSeabaseMDTable(&cliInterface, 
                                       vtCatNamePart, vtSchNamePart, vtObjNamePart,
                                       COM_BASE_TABLE_OBJECT);
      
      if (retcode < 0)
        {
          processReturn();
          
          return;
        }
      
      if (retcode == 1)
        {
          // table found in volatile schema. cannot rename it.
          *CmpCommon::diags()
            << DgSqlCode(-1427)
            << DgString0("Reason: Operation not allowed on volatile tables.");
          
          processReturn();
          return;
        }
    }
  
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  
  CorrName cn(objectNamePart,
              STMTHEAP,
              schemaNamePart,
              catalogNamePart);
  
  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      CmpCommon::diags()->clear();
      
      *CmpCommon::diags() << DgSqlCode(-1389)
                          << DgString0(extTableName);
  
      processReturn();
      
      return;
    }
 
  // Make sure user has the privilege to perform the rename
  if (!isDDLOperationAuthorized(SQLOperation::ALTER_TABLE,
                                naTable->getOwner(),naTable->getSchemaOwner()))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);

     processReturn ();

     return;
  }

 CorrName newcn(newObjectNamePart,
              STMTHEAP,
              schemaNamePart,
              catalogNamePart);
  
  NATable *newNaTable = bindWA.getNATable(newcn); 
  if (naTable != NULL && (NOT bindWA.errStatus()))
    {
      *CmpCommon::diags() << DgSqlCode(-1390)
                          << DgString0(newExtTableName);
      
      processReturn();
      
      return;
    }

  CmpCommon::diags()->clear();
  
  // cannot rename a view
  if (naTable->getViewText())
    {
      *CmpCommon::diags()
        << DgSqlCode(-1427)
        << DgString0("Reason: Operation not allowed on a view.");
      
      processReturn();
      
      return;
    }

  // cannot rename if views are using this table
  Queue * usingViewsQueue = NULL;
  cliRC = getUsingViews(&cliInterface, extTableName, TRUE, usingViewsQueue);
  if (cliRC < 0)
    {
      processReturn();
      
      return;
    }
  
  if (usingViewsQueue->numEntries() > 0)
    {
      *CmpCommon::diags() << DgSqlCode(-1427)
                          << DgString0("Reason: Dependent views exist.");
      
      processReturn();
      return;
    }

  Int64 objUID = getObjectUID(&cliInterface,
                              catalogNamePart.data(), schemaNamePart.data(), 
                              objectNamePart.data(),
                              COM_BASE_TABLE_OBJECT_LIT);
  if (objUID < 0)
    {

      processReturn();

      return;
    }

  cliRC = updateObjectName(&cliInterface,
                           objUID,
                           catalogNamePart.data(), schemaNamePart.data(),
                           newObjectNamePart.data());
  if (cliRC < 0)
    {
      processReturn();
      
      return;
    }

  // rename the underlying hbase object
  HbaseStr hbaseTable;
  hbaseTable.val = (char*)extNameForHbase.data();
  hbaseTable.len = extNameForHbase.length();
  
  HbaseStr newHbaseTable;
  newHbaseTable.val = (char*)newExtNameForHbase.data();
  newHbaseTable.len = newExtNameForHbase.length();

  retcode = ehi->copy(hbaseTable, newHbaseTable);
  if (retcode < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-8448)
                          << DgString0((char*)"ExpHbaseInterface::copy()")
                          << DgString1(getHbaseErrStr(-retcode))
                          << DgInt0(-retcode)
                          << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
      
      deallocEHI(ehi); 
      
      processReturn();
      
      return;
    }

  retcode = dropHbaseTable(ehi, &hbaseTable);
  if (retcode < 0)
    {
      return;
    }

  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);
  ActiveSchemaDB()->getNATableDB()->removeNATable(newcn);

  return;
}

void CmpSeabaseDDL::alterSeabaseTableAddColumn(
                                               StmtDDLAlterTableAddColumn * alterAddColNode,
                                               NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterAddColNode->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP);

  if ((isSeabaseReservedSchema(tableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CANNOT_ALTER_DEFINITION_METADATA_SCHEMA);
      processReturn();
      return;
    }

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
              STMTHEAP,
              tableName.getSchemaNamePart().getInternalName(),
              tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      CmpCommon::diags()->clear();

      *CmpCommon::diags()
        << DgSqlCode(-4082)
        << DgTableName(cn.getExposedNameAsAnsiString());

      processReturn();

      return;
    }

  // Make sure user has the privilege to perform the add column
  if (!isDDLOperationAuthorized(SQLOperation::ALTER_TABLE,
                                naTable->getOwner(),naTable->getSchemaOwner()))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);

     processReturn ();

     return;
  }

  const NAColumnArray &nacolArr = naTable->getNAColumnArray();

  ElemDDLColDefArray ColDefArray = alterAddColNode->getColDefArray();
  ElemDDLColDef *pColDef = ColDefArray[0];

  // Do not allow to using a NOT NULL constraint without a default
  // clause.  Do not allow DEFAULT NULL together with NOT NULL.
  if (pColDef->getIsConstraintNotNullSpecified())
    {
      if (pColDef->getDefaultClauseStatus() != ElemDDLColDef::DEFAULT_CLAUSE_SPEC)
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_DEFAULT_REQUIRED);

          processReturn();

          return;
        }
      ConstValue *pDefVal = (ConstValue *)pColDef->getDefaultValueExpr();
      
      if ((pDefVal->origOpType() != ITM_CURRENT_USER) &&
          (pDefVal->origOpType() != ITM_CURRENT_TIMESTAMP) &&
          (pDefVal->origOpType() != ITM_CAST)) 
        {
          if (pDefVal->isNull()) 
            {
              *CmpCommon::diags() << DgSqlCode(-CAT_CANNOT_BE_DEFAULT_NULL_AND_NOT_NULL);

              processReturn();

              return;
            }
        }
    }
  
  //Do not allow NO DEFAULT
  if (pColDef->getDefaultClauseStatus() == ElemDDLColDef::NO_DEFAULT_CLAUSE_SPEC)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_DEFAULT_REQUIRED);

      processReturn();

      return;
    }

  char query[4000];

  NAString colName;
  Lng32 datatype, length, precision, scale, dt_start, dt_end, nullable, upshifted;
  ComColumnClass colClass;
  ComColumnDefaultClass defaultClass;
  NAString charset, defVal;
  NAString heading;
  ULng32 hbaseColFlags;
  Int64 colFlags;
  LobsStorage lobStorage;
  if (getColInfo(pColDef,
		 colName, 
                 naTable->isSQLMXAlignedTable(),
		 datatype, length, precision, scale, dt_start, dt_end, upshifted, nullable,
		 charset, colClass, defaultClass, defVal, heading, lobStorage, hbaseColFlags, colFlags))
    {
      processReturn();
      
      return;
    }

  const NAColumn * nacol = nacolArr.getColumn(colName);
  if (nacol)
    {
      // column exists. Error or return, depending on 'if not exists' option.
      if (NOT alterAddColNode->addIfNotExists())
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_DUPLICATE_COLUMNS)
                              << DgColumnName(colName);
        }
      
      processReturn();

      return;
    }

  char * col_name = new(STMTHEAP) char[colName.length() + 1];
  strcpy(col_name, (char*)colName.data());

  NAString quotedHeading;
  if (NOT heading.isNull())
    {
      ToQuotedString(quotedHeading, heading, FALSE);
    }
  
  NAString quotedDefVal;
  if (NOT defVal.isNull())
    {
      ToQuotedString(quotedDefVal, defVal, FALSE);
    }

  str_sprintf(query, "insert into %s.\"%s\".%s values (%Ld, '%s', %d, '%s', %d, '%s', %d, %d, %d, %d, %d, '%s', %d, %d, '%s', %d, '%s', '%s', '%s', '%d', '%s', '%s', %Ld )",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
              naTable->objectUid().castToInt64(), 
              col_name,
              naTable->getColumnCount(), 
              COM_ADDED_USER_COLUMN_LIT,
              datatype,
              getAnsiTypeStrFromFSType(datatype),
              length,
              precision,
              scale,
              dt_start,
              dt_end,
              (upshifted ? "Y" : "N"),
              hbaseColFlags, 
              nullable,
              (char*)charset.data(),
              (Lng32)defaultClass,
              (quotedDefVal.isNull() ? "" : quotedDefVal.data()),
              (quotedHeading.isNull() ? "" : quotedHeading.data()),
              SEABASE_DEFAULT_COL_FAMILY,
              naTable->getColumnCount()+1,
              COM_UNKNOWN_PARAM_DIRECTION_LIT,
              "N",
              colFlags);
  
  cliRC = cliInterface.executeImmediate(query);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return;
    }

  //  CorrName cn(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  if ((alterAddColNode->getAddConstraintPK()) OR
      (alterAddColNode->getAddConstraintCheckArray().entries() NEQ 0) OR
      (alterAddColNode->getAddConstraintUniqueArray().entries() NEQ 0) OR
      (alterAddColNode->getAddConstraintRIArray().entries() NEQ 0))
    {
      cliRC = cliInterface.holdAndSetCQD("TRAF_RELOAD_NATABLE_CACHE", "ON");
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          
          processReturn();
          
          goto label_return;
        }

      addConstraints(tableName, currCatAnsiName, currSchAnsiName,
                     alterAddColNode->getAddConstraintPK(),
                     alterAddColNode->getAddConstraintUniqueArray(),
                     alterAddColNode->getAddConstraintRIArray(),
                     alterAddColNode->getAddConstraintCheckArray());                 

      cliRC = cliInterface.restoreCQD("TRAF_RELOAD_NATABLE_CACHE");

      if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_))
        return;
    }

  if (updateObjectRedefTime(&cliInterface, 
                            catalogNamePart, schemaNamePart, objectNamePart,
                            COM_BASE_TABLE_OBJECT_LIT))
    {
      processReturn();

      return;
    }

 label_return:
  processReturn();

  return;
}

void CmpSeabaseDDL::alterSeabaseTableDropColumn(
                                                StmtDDLAlterTableDropColumn * alterDropColNode,
                                                NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterDropColNode->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ExeCliInterface cliInterface(STMTHEAP);

  if ((isSeabaseReservedSchema(tableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CANNOT_ALTER_DEFINITION_METADATA_SCHEMA);
      processReturn();
      return;
    }

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
              STMTHEAP,
              tableName.getSchemaNamePart().getInternalName(),
              tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
        << DgSqlCode(-4082)
        << DgTableName(cn.getExposedNameAsAnsiString());
    
      processReturn();

      return;
    }

  // Make sure user has the privilege to perform the drop column
  if (!isDDLOperationAuthorized(SQLOperation::ALTER_TABLE,
                                naTable->getOwner(),naTable->getSchemaOwner()))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);

     processReturn ();

     return;
  }

  if (naTable->isSQLMXAlignedTable())
    {
     *CmpCommon::diags()
	<< DgSqlCode(-4222)
	<< DgString0("\"DROP COLUMN on ALIGNED format tables\"");
    
      processReturn();

      return;
     }

  const NAColumnArray &nacolArr = naTable->getNAColumnArray();
  const NAString &colName = alterDropColNode->getColName();

  const NAColumn * nacol = nacolArr.getColumn(colName);
  if (! nacol)
    {
      // column doesnt exist. Error or return, depending on 'if exists' option.
      if (NOT alterDropColNode->dropIfExists())
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_COLUMN_DOES_NOT_EXIST_ERROR)
                              << DgColumnName(colName);
        }

      processReturn();

      return;
    }

  const NAFileSet * naFS = naTable->getClusteringIndex();
  const NAColumnArray &naKeyColArr = naFS->getIndexKeyColumns();
  if (naKeyColArr.getColumn(colName))
    {
      // key column cannot be dropped
      *CmpCommon::diags() << DgSqlCode(-1420)
                          << DgColumnName(colName);

      processReturn();

      return;
    }

  if (naTable->hasSecondaryIndexes())
    {
      const NAFileSetList &naFsList = naTable->getIndexList();

      for (Lng32 i = 0; i < naFsList.entries(); i++)
        {
          naFS = naFsList[i];

          const NAColumnArray &naIndexColArr = naFS->getAllColumns();
          if (naIndexColArr.getColumn(colName))
            {
              // secondary index column cannot be dropped
              *CmpCommon::diags() << DgSqlCode(-1421)
                                  << DgColumnName(colName)
                                  << DgTableName(naFS->getExtFileSetName());

              processReturn();

              return;
            }
        } // for
    } // secondary indexes present

  if ((naTable->getClusteringIndex()->hasSyskey()) &&
      (nacolArr.entries() == 2))
    {
      // this table has one SYSKEY column and one other column.
      // Dropping that column will leave the table with no user column.
      // Return an error.
      *CmpCommon::diags() << DgSqlCode(-1424)
                          << DgColumnName(colName);
    }

  Int64 objUID = naTable->objectUid().castToInt64();

  if (isSeabaseMD(tableName))
    {
      // objectUID for a metadata table is not available in NATable struct
      // since the definition for a MD table is hardcoded.
      // get objectUID for metadata tables from the OBJECTS table.
      objUID =
        getObjectUID(&cliInterface,
                     catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
                     COM_BASE_TABLE_OBJECT_LIT);
    }

  Lng32 colNumber = nacol->getPosition();

  char buf[4000];
  str_sprintf(buf, "delete from %s.\"%s\".%s where object_uid = %Ld and column_number = %d",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
              objUID,
              colNumber);

  cliRC = cliInterface.executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return;
    }

  str_sprintf(buf, "update %s.\"%s\".%s set column_number = column_number - 1 where object_uid = %Ld and column_number >= %d",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
              objUID,
              colNumber);

  cliRC = cliInterface.executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return;
    }

  str_sprintf(buf, "update %s.\"%s\".%s set column_number = column_number - 1 where object_uid = %Ld and column_number >= %d",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
              objUID,
              colNumber);

  cliRC = cliInterface.executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return;
    }

  // remove column from all rows of the base table
  HbaseStr hbaseTable;
  hbaseTable.val = (char*)extNameForHbase.data();
  hbaseTable.len = extNameForHbase.length();

  //  Text column("cf1:");

  //  Text column(SEABASE_DEFAULT_COL_FAMILY);
  //  column.append(":");
  //  column.append(colName);

  Text column(nacol->getHbaseColFam());
  column.append(":");
  //  column.append(nacol->getHbaseColQual());

  char * colQualPtr = (char*)nacol->getHbaseColQual().data();
  Lng32 colQualLen = nacol->getHbaseColQual().length();
  Int64 colQval = str_atoi(colQualPtr, colQualLen);
  if (colQval <= UCHAR_MAX)
    {
      unsigned char c = (unsigned char)colQval;
      column.append((char*)&c, 1);
    }
  else if (colQval <= USHRT_MAX)
    {
      unsigned short s = (unsigned short)colQval;
      column.append((char*)&s, 2);
    }
  else if (colQval <= ULONG_MAX)
    {
      Lng32 l = (Lng32)colQval;
      column.append((char*)&l, 4);
    }
  else
    column.append((char*)&colQval, 8);
  
  retcode = ehi->deleteColumns(hbaseTable, column);
  if (retcode < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-8448)
                          << DgString0((char*)"ExpHbaseInterface::deleteColumns()")
                          << DgString1(getHbaseErrStr(-retcode))
                          << DgInt0(-retcode)
                          << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
      
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  if (updateObjectRedefTime(&cliInterface,
                            catalogNamePart, schemaNamePart, objectNamePart,
                            COM_BASE_TABLE_OBJECT_LIT))
    {
      processReturn();

      deallocEHI(ehi);

      return;
    }

  deallocEHI(ehi); 

  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  processReturn();

  return;
}

void CmpSeabaseDDL::alterSeabaseTableAlterIdentityColumn(
                                                         StmtDDLAlterTableAlterColumnSetSGOption * alterIdentityColNode,
                                                         NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterIdentityColNode->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP);

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
              STMTHEAP,
              tableName.getSchemaNamePart().getInternalName(),
              tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
        << DgSqlCode(-4082)
        << DgTableName(cn.getExposedNameAsAnsiString());
    
      processReturn();

      return;
    }

  const NAColumnArray &nacolArr = naTable->getNAColumnArray();
  const NAString &colName = alterIdentityColNode->getColumnName();

  const NAColumn * nacol = nacolArr.getColumn(colName);
  if (! nacol)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_COLUMN_DOES_NOT_EXIST_ERROR)
                          << DgColumnName(colName);

      processReturn();

      return;
    }

  if (! nacol->isIdentityColumn())
    {
      *CmpCommon::diags() << DgSqlCode(-1590)
                          << DgColumnName(colName);

      processReturn();

      return;
    }

  NAString seqName;
  SequenceGeneratorAttributes::genSequenceName
    (catalogNamePart, schemaNamePart, objectNamePart, 
     alterIdentityColNode->getColumnName(),
     seqName);
  
  ElemDDLSGOptions * sgo = alterIdentityColNode->getSGOptions();
  NAString options;
  if (sgo)
    {
      char tmpBuf[1000];
      if (sgo->isIncrementSpecified())
        {
          str_sprintf(tmpBuf, " increment by %Ld", sgo->getIncrement());
          options += tmpBuf;
        }
      
      if (sgo->isMaxValueSpecified())
        {
          if (sgo->isNoMaxValue())
            str_sprintf(tmpBuf, " no maxvalue ", sgo->getMaxValue());
          else
            str_sprintf(tmpBuf, " maxvalue %Ld", sgo->getMaxValue());
          options += tmpBuf;
        }
      
      if (sgo->isMinValueSpecified())
        {
          if (sgo->isNoMinValue())
            str_sprintf(tmpBuf, " no maxvalue ", sgo->getMinValue());
          else
            str_sprintf(tmpBuf, " minvalue %Ld", sgo->getMinValue());
          options += tmpBuf;
        }
      
      if (sgo->isStartValueSpecified())
        {
          str_sprintf(tmpBuf, " start with %Ld", sgo->getStartValue());
          options += tmpBuf;
        }
      
      if (sgo->isCacheSpecified())
        {
          if (sgo->isNoCache())
            str_sprintf(tmpBuf, " no cache ");
          else
            str_sprintf(tmpBuf, " cache %Ld ", sgo->getCache());
          options += tmpBuf;
        }
      
      if (sgo->isCycleSpecified())
        {
          if (sgo->isNoCycle())
            str_sprintf(tmpBuf, " no cycle ");
          else
            str_sprintf(tmpBuf, " cycle ");
          options += tmpBuf;
        }

      char buf[4000];
      str_sprintf(buf, "alter internal sequence %s.\"%s\".\"%s\" %s",
                  catalogNamePart.data(), schemaNamePart.data(), seqName.data(),
                  options.data());
      
      cliRC = cliInterface.executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          
          processReturn();
          
          return;
        }
    }

  //  CorrName cn(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  return;
}

void CmpSeabaseDDL::alterSeabaseTableAddPKeyConstraint(
                                                       StmtDDLAddConstraint * alterAddConstraint,
                                                       NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterAddConstraint->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  if ((isSeabaseReservedSchema(tableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CANNOT_ALTER_DEFINITION_METADATA_SCHEMA);
      processReturn();
      return;
    }

  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  if (CmpCommon::getDefault(TRAF_RELOAD_NATABLE_CACHE) == DF_OFF)
    ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
              STMTHEAP,
              tableName.getSchemaNamePart().getInternalName(),
              tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
        << DgSqlCode(-4082)
        << DgTableName(cn.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  // Make sure user has the privilege to perform the add pk constraint
  if (!isDDLOperationAuthorized(SQLOperation::ALTER_TABLE,
                                naTable->getOwner(),naTable->getSchemaOwner()))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);

     deallocEHI(ehi); 

     processReturn ();

     return;
  }

  ElemDDLColRefArray &keyColumnArray = alterAddConstraint->getConstraint()->castToElemDDLConstraintPK()->getKeyColumnArray();

  NAList<NAString> keyColList(HEAP, keyColumnArray.entries());
  NAString pkeyStr("(");
  for (Int32 j = 0; j < keyColumnArray.entries(); j++)
    {
      const NAString &colName = keyColumnArray[j]->getColumnName();
      keyColList.insert(colName);

      pkeyStr += colName;
      if (j < (keyColumnArray.entries() - 1))
        pkeyStr += ", ";
    }
  pkeyStr += ")";

  if (constraintErrorChecks(alterAddConstraint->castToStmtDDLAddConstraintUnique(),
                            naTable,
                            COM_UNIQUE_CONSTRAINT, //TRUE, 
                            keyColList))
    {
      return;
    }

 // update unique key constraint info
  NAString uniqueStr;
  if (genUniqueName(alterAddConstraint, uniqueStr))
    {
      return;
    }

  // if table doesnt have a user defined primary key, is empty and doesn't have any 
  // dependent objects (index, views, triggers, RI, etc), then drop it and recreate it with 
  // this new primary key.
  // Do this optimization in mode_special_4 only.
  Lng32 len = 0;
  Lng32 rowCount = 0;
  NABoolean ms4 = FALSE;
  if (CmpCommon::getDefault(MODE_SPECIAL_4) == DF_ON)
    {
      ms4 = TRUE;

      char query[2000];
      str_sprintf(query, "select [any 1] cast(1 as int not null) from \"%s\".\"%s\".\"%s\" for read committed access",
                  catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data());
      cliRC = cliInterface.executeImmediate(query, (char*)&rowCount, &len, NULL);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          return;
        }
    }

  // if the table is not empty, or there are dependent objects/constraints,
  //  or the table already has  a pkey/store by, then create a unique constraint.
  NABoolean isStoreBy = FALSE;
  Int32 nonSystemKeyCols = 0;
  if (naTable->getClusteringIndex())
    {
      NAFileSet * naf = naTable->getClusteringIndex();
      for (Lng32 i = 0; i < naf->getIndexKeyColumns().entries(); i++)
        {
          NAColumn * nac = naf->getIndexKeyColumns()[i];
   
          if (NOT nac->isSystemColumn())
            nonSystemKeyCols++;
          else if (nac->isSyskeyColumn())
            isStoreBy = TRUE;
        } // for

      if (nonSystemKeyCols == 0)
        isStoreBy = FALSE;
    } // if
  
  if ((rowCount > 0) || // not empty
      (NOT ms4) || // not mode_special_4
      (naTable->hasSecondaryIndexes()) || // user indexes
      (NOT naTable->getClusteringIndex()->hasSyskey()) || // user defined pkey
      (isStoreBy) ||     // user defined store by
      (naTable->getUniqueConstraints().entries() > 0) || // unique constraints
      (naTable->getRefConstraints().entries() > 0) || // ref constraints
      (naTable->getCheckConstraints().entries() > 0))
    {
      // cannot create clustered primary key constraint.
      // create a unique constraint instead.
      NAString cliQuery;
      cliQuery = "alter table " + extTableName + " add constraint " + uniqueStr
        + " unique " + pkeyStr + ";";
      cliRC = cliInterface.executeImmediate((char*)cliQuery.data());
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
        }

      return;
    }

  Int64 tableUID = 
    getObjectUID(&cliInterface,
                 catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
                 COM_BASE_TABLE_OBJECT_LIT);
   
  // empty table. Drop and recreate it with the new primary key.
  char * buf = NULL;
  ULng32 buflen = 0;
  retcode = CmpDescribeSeabaseTable(cn, 3/*createlike*/, buf, buflen, STMTHEAP,
                                    pkeyStr.data(), TRUE);
  if (retcode)
    return;
  
  NAString cliQuery;
  // drop this table.
  cliQuery = "drop table ";
  cliQuery += extTableName;
  cliQuery += " no check;";
  cliRC = cliInterface.executeImmediate((char*)cliQuery.data());
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }

  char cqdbuf[200];
  str_sprintf(cqdbuf, "cqd traf_create_table_with_uid '%Ld';",
              tableUID);
  cliRC = cliInterface.executeImmediate(cqdbuf);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }

  // and recreate it with the new primary key.
  cliQuery = "create table ";
  cliQuery += extTableName;
  cliQuery += " ";

  NABoolean done = FALSE;
  Lng32 curPos = 0;
  while (NOT done)
    {
      short len = *(short*)&buf[curPos];
      NAString frag(&buf[curPos+sizeof(short)],
                    len - ((buf[curPos+len-1]== '\n') ? 1 : 0));

      cliQuery += frag;
      curPos += ((((len+sizeof(short))-1)/8)+1)*8;

      if (curPos >= buflen)
        done = TRUE;
    }

  cliRC = cliInterface.executeImmediate((char*)cliQuery.data());
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    }

  str_sprintf(cqdbuf, "cqd traf_create_table_with_uid '' ;");
  cliInterface.executeImmediate(cqdbuf);

  if (cliRC < 0)
    {
      return;
    }

  if (updateObjectRedefTime(&cliInterface,
                            catalogNamePart, schemaNamePart, objectNamePart,
                            COM_BASE_TABLE_OBJECT_LIT))
    {
      processReturn();

      deallocEHI(ehi);

      return;
    }

  // remove NATable for this table
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  return;
}

void CmpSeabaseDDL::alterSeabaseTableAddUniqueConstraint(
                                                StmtDDLAddConstraint * alterAddConstraint,
                                                NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterAddConstraint->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  if ((isSeabaseReservedSchema(tableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CANNOT_ALTER_DEFINITION_METADATA_SCHEMA);
      processReturn();
      return;
    }

  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  if (CmpCommon::getDefault(TRAF_RELOAD_NATABLE_CACHE) == DF_OFF)
    ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
              STMTHEAP,
              tableName.getSchemaNamePart().getInternalName(),
              tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
        << DgSqlCode(-4082)
        << DgTableName(cn.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  // Make sure user has the privilege to perform the create unique constraint
  if (!isDDLOperationAuthorized(SQLOperation::ALTER_TABLE,
                                naTable->getOwner(),naTable->getSchemaOwner()))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);

     deallocEHI(ehi); 

     processReturn ();

     return;
  }

  ElemDDLColRefArray &keyColumnArray = alterAddConstraint->getConstraint()->castToElemDDLConstraintUnique()->getKeyColumnArray();

  NAList<NAString> keyColList(HEAP, keyColumnArray.entries());
  NAList<NAString> keyColOrderList(HEAP, keyColumnArray.entries());
  for (Int32 j = 0; j < keyColumnArray.entries(); j++)
    {
      const NAString &colName = keyColumnArray[j]->getColumnName();
      keyColList.insert(colName);

      if (keyColumnArray[j]->getColumnOrdering() == COM_DESCENDING_ORDER)
        keyColOrderList.insert("DESC");
      else
        keyColOrderList.insert("ASC");
    }

  if (constraintErrorChecks(alterAddConstraint->castToStmtDDLAddConstraintUnique(),
                            naTable,
                            COM_UNIQUE_CONSTRAINT, 
                            keyColList))
    {
      return;
    }

 // update unique key constraint info
  NAString uniqueStr;
  if (genUniqueName(alterAddConstraint, uniqueStr))
    {
      return;
    }
  
  Int64 tableUID = 
    getObjectUID(&cliInterface,
                 catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
                 COM_BASE_TABLE_OBJECT_LIT);

  ComUID comUID;
  comUID.make_UID();
  Int64 uniqueUID = comUID.get_value();

  if (updateConstraintMD(keyColList, keyColOrderList, uniqueStr, tableUID, uniqueUID, 
                         naTable, COM_UNIQUE_CONSTRAINT, TRUE, &cliInterface))
    {
      *CmpCommon::diags()
        << DgSqlCode(-1029)
        << DgTableName(uniqueStr);

      return;
    }

  NAList<NAString> emptyKeyColList;
  if (updateIndexInfo(keyColList,
                      keyColOrderList,
                      emptyKeyColList,
                      uniqueStr,
                      uniqueUID,
                      catalogNamePart, schemaNamePart, objectNamePart,
                      naTable,
                      TRUE,
                      (CmpCommon::getDefault(TRAF_NO_CONSTR_VALIDATION) == DF_ON),
                      &cliInterface))
    {
      *CmpCommon::diags()
        << DgSqlCode(-1029)
        << DgTableName(uniqueStr);

      return;
    }

  if (updateObjectRedefTime(&cliInterface,
                            catalogNamePart, schemaNamePart, objectNamePart,
                            COM_BASE_TABLE_OBJECT_LIT))
    {
      processReturn();

      deallocEHI(ehi);

      return;
    }

  // remove NATable for this table
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  return;
}

// returns 1 if referenced table refdTable has a dependency on the 
// original referencing table origRingTable.
// return 0, if it does not.
// return -1, if error.
short CmpSeabaseDDL::isCircularDependent(CorrName &ringTable,
                                         CorrName &refdTable,
                                         CorrName &origRingTable,
                                         BindWA *bindWA)
{
  // get natable for the referenced table.
  NATable *naTable = bindWA->getNATable(refdTable); 
  if (naTable == NULL || bindWA->errStatus())
    {
      *CmpCommon::diags() << DgSqlCode(-1389)
                          << DgString0(naTable->getTableName().getQualifiedNameAsString());
      
      processReturn();
      
      return -1;
    }
  
  // find all the tables the refdTable depends on.
  const AbstractRIConstraintList &refList = naTable->getRefConstraints();
  
  for (Int32 i = 0; i < refList.entries(); i++)
    {
      AbstractRIConstraint *ariConstr = refList[i];
      
      if (ariConstr->getOperatorType() != ITM_REF_CONSTRAINT)
        continue;

      RefConstraint * refConstr = (RefConstraint*)ariConstr;

      //      CorrName cn(refConstr->getDefiningTableName());
      CorrName cn(refConstr->getUniqueConstraintReferencedByMe().getTableName());
      if (cn == origRingTable)
        {
          return 1; // dependency exists
        }
      short rc = isCircularDependent(cn, cn, 
                                     origRingTable, bindWA);
      if (rc)
        return rc;

    } // for

  return 0;
}

void CmpSeabaseDDL::alterSeabaseTableAddRIConstraint(
                                                StmtDDLAddConstraint * alterAddConstraint,
                                                NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterAddConstraint->getTableName();

  ComObjectName referencingTableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  referencingTableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = referencingTableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = referencingTableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = referencingTableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = referencingTableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  if ((isSeabaseReservedSchema(referencingTableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CANNOT_ALTER_DEFINITION_METADATA_SCHEMA);
      processReturn();
      return;
    }

  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  if (CmpCommon::getDefault(TRAF_RELOAD_NATABLE_CACHE) == DF_OFF)
    ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(referencingTableName.getObjectNamePart().getInternalName(),
              STMTHEAP,
              referencingTableName.getSchemaNamePart().getInternalName(),
              referencingTableName.getCatalogNamePart().getInternalName());

  NATable *ringNaTable = bindWA.getNATable(cn); 
  if (ringNaTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
        << DgSqlCode(-4082)
        << DgTableName(cn.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  // Make sure user has the privilege to perform the add RI constraint
  if (!isDDLOperationAuthorized(SQLOperation::ALTER_TABLE,
                                ringNaTable->getOwner(),ringNaTable->getSchemaOwner()))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);

     deallocEHI(ehi); 

     processReturn ();

     return;
  }

  const ElemDDLConstraintRI *constraintNode = 
    alterAddConstraint->getConstraint()->castToElemDDLConstraintRI();
  ComObjectName referencedTableName( constraintNode->getReferencedTableName()
                                     , COM_TABLE_NAME);
  referencedTableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  if ((isSeabaseReservedSchema(referencedTableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CANNOT_ALTER_DEFINITION_METADATA_SCHEMA);
      processReturn();
      return;
    }

  CorrName cn2(referencedTableName.getObjectNamePart().getInternalName(),
              STMTHEAP,
              referencedTableName.getSchemaNamePart().getInternalName(),
              referencedTableName.getCatalogNamePart().getInternalName());

  NATable *refdNaTable = bindWA.getNATable(cn2); 
  if (refdNaTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
        << DgSqlCode(-4082)
        << DgTableName(cn2.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  if (refdNaTable->getViewText())
    {
     *CmpCommon::diags()
	<< DgSqlCode(-1127)
	<< DgTableName(cn2.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  // If the referenced and referencing tables are the same, 
  // reject the request.  At this time, we do not allow self
  // referencing constraints.
  if (referencingTableName == referencedTableName)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_SELF_REFERENCING_CONSTRAINT);

      processReturn();
      
      return;
    }

  // User must have REFERENCES privilege on the referenced table 
  if (isAuthorizationEnabled())
    {
      PrivMgrUserPrivs* privs = refdNaTable->getPrivInfo();
      if (privs == NULL)
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);

          deallocEHI(ehi);

          processReturn();

          return;
        }

      if (!ComUser::isRootUserID() && !privs->hasReferencePriv())
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);

          deallocEHI(ehi);

          processReturn();

          return;
        }
    }

  ElemDDLColNameArray &ringCols = alterAddConstraint->getConstraint()->castToElemDDLConstraintRI()->getReferencingColumns();

  NAList<NAString> ringKeyColList(HEAP, ringCols.entries());
  NAList<NAString> ringKeyColOrderList(HEAP, ringCols.entries());
  NAString ringColListForValidation;
  NAString ringNullList;
  for (Int32 j = 0; j < ringCols.entries(); j++)
    {
      const NAString &colName = ringCols[j]->getColumnName();
      ringKeyColList.insert(colName);
      ringKeyColOrderList.insert("ASC");

      ringColListForValidation += "\"";
      ringColListForValidation += colName;
      ringColListForValidation += "\"";
      if (j < (ringCols.entries() - 1))
        ringColListForValidation += ", ";

      ringNullList += "or ";
      ringNullList += "\"";
      ringNullList += colName;
      ringNullList += "\"";
      ringNullList += " is null ";
    }

  if (constraintErrorChecks(alterAddConstraint->castToStmtDDLAddConstraintRI(),
                            ringNaTable,
                            COM_FOREIGN_KEY_CONSTRAINT, //FALSE, // referencing constr
                            ringKeyColList))
    {
      return;
    }

 const NAString &addConstrName = alterAddConstraint->
    getConstraintNameAsQualifiedName().getQualifiedNameAsAnsiString();

  // Compare the referenced column list against the primary and unique
  // constraint defined for the referenced table.  The referenced 
  // column list must match one of these constraints.  Note that if there
  // was no referenced column list specified, the primary key is used
  // and a match has automatically been found.

  NABoolean matchFound = FALSE;

  const ElemDDLColNameArray &referencedColNode = 
    constraintNode->getReferencedColumns();

  NAList<NAString> refdKeyColList(HEAP, referencedColNode.entries());
  NAString refdColListForValidation;
  for (Int32 j = 0; j < referencedColNode.entries(); j++)
    {
      const NAString &colName = referencedColNode[j]->getColumnName();
      refdKeyColList.insert(colName);

      refdColListForValidation += "\"";
      refdColListForValidation += colName;
      refdColListForValidation += "\"";
      if (j < (referencedColNode.entries() - 1))
        refdColListForValidation += ", ";
    }

  if (referencedColNode.entries() == 0)
    {
      NAFileSet * naf = refdNaTable->getClusteringIndex();
      for (Lng32 i = 0; i < naf->getIndexKeyColumns().entries(); i++)
        {
          NAColumn * nac = naf->getIndexKeyColumns()[i];

          const NAString &colName = nac->getColName();
          refdKeyColList.insert(colName);

          refdColListForValidation += "\"";
          refdColListForValidation += nac->getColName();
          refdColListForValidation += "\"";
          if (i < (naf->getIndexKeyColumns().entries() - 1))
            refdColListForValidation += ", ";
        }
    }

  if (ringKeyColList.entries() != refdKeyColList.entries())
    {
      *CmpCommon::diags()
        << DgSqlCode(-1046)
        << DgConstraintName(addConstrName);
      
      processReturn();
      
      return;
    }

  const NAColumnArray &ringNACarr = ringNaTable->getNAColumnArray();
  const NAColumnArray &refdNACarr = refdNaTable->getNAColumnArray();
  for (Int32 i = 0; i < ringKeyColList.entries(); i++)
    {
      const NAString &ringColName = ringKeyColList[i];
      const NAString &refdColName = refdKeyColList[i];

      const NAColumn * ringNAC = ringNACarr.getColumn(ringColName);
      const NAColumn * refdNAC = refdNACarr.getColumn(refdColName);

      //      if (NOT(*ringNAC->getType() == *refdNAC->getType()))
      if (NOT (ringNAC->getType()->equalIgnoreNull(*refdNAC->getType())))
        {
          *CmpCommon::diags()
            << DgSqlCode(-1046)
            << DgConstraintName(addConstrName);
      
          processReturn();
          
          return;
        }
    }

  // method getCorrespondingConstraint expects an empty input list if there are no
  // user specified columns. Clear the refdKeyColList before calling it.
  if (referencedColNode.entries() == 0)
    {
      refdKeyColList.clear();
    }

  NAString constrName;
  NABoolean isPkey = FALSE;
  if (refdNaTable->getCorrespondingConstraint(refdKeyColList,
                                              TRUE, // unique constraint
                                              &constrName,
                                              &isPkey))
    matchFound = TRUE;
  
  if (NOT matchFound)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_REFERENCED_CONSTRAINT_DOES_NOT_EXIST)
                          << DgConstraintName(addConstrName);
      
      return;
    }

  // check for circular RI dependencies.
  // check if referenced table cn2 refers back to the referencing table cn.
  retcode = isCircularDependent(cn, cn2, cn, &bindWA);
  if (retcode == 1) // dependency exists
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_RI_CIRCULAR_DEPENDENCY)
                          << DgConstraintName(addConstrName)
                          << DgTableName(cn.getExposedNameAsAnsiString());
      
      return;
    }
  else if (retcode < 0)
    {
      // error. Diags area has been populated
      return; 
    }

  if (CmpCommon::getDefault(TRAF_NO_CONSTR_VALIDATION) == DF_OFF)
    {
      // validate data for RI constraint.
      // generate a "select" statement to validate the constraint.  For example:
      // SELECT count(*) FROM T1 
      //   WHERE NOT ((T1C1,T1C2) IN (SELECT T2C1,T2C2 FROM T2))
      //   OR T1C1 IS NULL OR T1C2 IS NULL;
      // This statement returns > 0 if there exist data violating the constraint.
      
      char * validQry =
        new(STMTHEAP) char[ringColListForValidation.length() +
                           refdColListForValidation.length() +
                           ringNullList.length() +
                           2000];
      str_sprintf(validQry, "select count(*) from \"%s\".\"%s\".\"%s\" where not ((%s) in (select %s from \"%s\".\"%s\".\"%s\")) %s;",
                  referencingTableName.getCatalogNamePart().getInternalName().data(),
                  referencingTableName.getSchemaNamePart().getInternalName().data(),
                  referencingTableName.getObjectNamePart().getInternalName().data(),
                  ringColListForValidation.data(),
                  refdColListForValidation.data(),
                  referencedTableName.getCatalogNamePart().getInternalName().data(),
                  referencedTableName.getSchemaNamePart().getInternalName().data(),
                  referencedTableName.getObjectNamePart().getInternalName().data(),
                  ringNullList.data());

      Lng32 len = 0;
      Int64 rowCount = 0;
      cliRC = cliInterface.executeImmediate(validQry, (char*)&rowCount, &len, NULL);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          return;
        }
      
      if (rowCount > 0)
        {
          *CmpCommon::diags() << DgSqlCode(-1143)
                              << DgConstraintName(addConstrName)
                              << DgTableName(referencingTableName.getObjectNamePart().getInternalName().data())
                              << DgString0(referencedTableName.getObjectNamePart().getInternalName().data()) 
                              << DgString1("TBD");
          
          return;
        }
    }

  ComObjectName refdConstrName(constrName);
  refdConstrName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString refdCatName = refdConstrName.getCatalogNamePartAsAnsiString();
  const NAString refdSchName = refdConstrName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString refdObjName = refdConstrName.getObjectNamePartAsAnsiString(TRUE);

 Int64 refdConstrUID = 
    getObjectUID(&cliInterface,
                 refdCatName.data(), refdSchName.data(), refdObjName.data(),
                 (isPkey ?  COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT :
                  COM_UNIQUE_CONSTRAINT_OBJECT_LIT));

  NAString uniqueStr;
  if (genUniqueName(alterAddConstraint, uniqueStr))
    {
      return;
    }
  
  Int64 tableUID = 
    getObjectUID(&cliInterface,
                 catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
                 COM_BASE_TABLE_OBJECT_LIT);

  ComUID comUID;
  comUID.make_UID();
  Int64 ringConstrUID = comUID.get_value();

  if (updateConstraintMD(ringKeyColList, ringKeyColOrderList, uniqueStr, tableUID, ringConstrUID, 
                         ringNaTable, COM_FOREIGN_KEY_CONSTRAINT, 
                         constraintNode->isEnforced(), &cliInterface))
    {
      *CmpCommon::diags()
        << DgSqlCode(-1029)
        << DgTableName(uniqueStr);

      return;
    }

  if (updateRIConstraintMD(ringConstrUID, refdConstrUID,
                           &cliInterface))
    {
      *CmpCommon::diags()
        << DgSqlCode(-1029)
        << DgTableName(uniqueStr);

      return;
    }

  if (updateIndexInfo(ringKeyColList,
                      ringKeyColOrderList,
                      refdKeyColList,
                      uniqueStr,
                      ringConstrUID,
                      catalogNamePart, schemaNamePart, objectNamePart,
                      ringNaTable,
                      FALSE,
                      (CmpCommon::getDefault(TRAF_NO_CONSTR_VALIDATION) == DF_ON),
                      &cliInterface))
    {
      *CmpCommon::diags()
        << DgSqlCode(-1029)
        << DgTableName(uniqueStr);

      return;
    }

  if (NOT constraintNode->isEnforced())
    {
      *CmpCommon::diags()
        << DgSqlCode(1313)
        << DgString0(addConstrName);
    }

  if (updateObjectRedefTime(&cliInterface,
                            catalogNamePart, schemaNamePart, objectNamePart,
                            COM_BASE_TABLE_OBJECT_LIT))
    {
      processReturn();

      deallocEHI(ehi);

      return;
    }

  // remove NATable for this table
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  // remove natable for the table being referenced
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn2);

  return;
}

short CmpSeabaseDDL::getCheckConstraintText(StmtDDLAddConstraintCheck *addCheckNode,
                                            NAString &qualifiedText)
{
  //  ComString             qualifiedText;
  const ParNameLocList &nameLocList = addCheckNode->getNameLocList();
  const ParNameLoc     *pNameLoc    = NULL;
  const char           *pInputStr   = nameLocList.getInputStringPtr();

  StringPos inputStrPos = addCheckNode->getStartPosition();
  //  CharInfo::CharSet mapCS = (CharInfo::CharSet) SqlParser_ISO_MAPPING;

  for (size_t x = 0; x < nameLocList.entries(); x++)
  {
    pNameLoc = &nameLocList[x];
    const NAString &nameExpanded = pNameLoc->getExpandedName(FALSE/*no assert*/);
    size_t nameAsIs = 0;
    size_t nameLenInBytes = 0;

    //
    // When the character set of the input string is a variable-length/width
    // multi-byte characters set, the value returned by getNameLength()
    // may not be numerically equal to the number of bytes in the original
    // input string that we need to skip.  So, we get the character
    // conversion routines to tell us how many bytes we need to skip.
    //
    enum cnv_charset eCnvCS = convertCharsetEnum(nameLocList.getInputStringCharSet());

    const char *str_to_test = (const char *) &pInputStr[pNameLoc->getNamePosition()];
    const int max_bytes2cnv = addCheckNode->getEndPosition()
      - pNameLoc->getNamePosition() + 1;
    const char *tmp_out_bufr = new (STMTHEAP) char[max_bytes2cnv * 4 + 10 /* Ensure big enough! */ ];
    char * p1stUnstranslatedChar = NULL;
    
    int cnvErrStatus = LocaleToUTF16(
                                     cnv_version1          // in  - const enum cnv_version version
                                     , str_to_test           // in  - const char *in_bufr
                                     , max_bytes2cnv         // in  - const int in_len
                                     , tmp_out_bufr          // out - const char *out_bufr
                                     , max_bytes2cnv * 4     // in  - const int out_len
                                     , eCnvCS                // in  - enum cnv_charset charset
                                     , p1stUnstranslatedChar // out - char * & first_untranslated_char
                                     , NULL                  // out - unsigned int *output_data_len_p
                                     , 0                     // in  - const int cnv_flags
                                     , (int)FALSE            // in  - const int addNullAtEnd_flag
                                     , NULL                  // out - unsigned int * translated_char_cnt_p
                                     , pNameLoc->getNameLength() // in - unsigned int max_chars_to_convert
                                     );
    // NOTE: No errors should be possible -- string has been converted before.
    
    NADELETEBASIC (tmp_out_bufr, STMTHEAP);
    nameLenInBytes = p1stUnstranslatedChar - str_to_test;
    
    // If name not expanded, then use the original name as is
    if (nameExpanded.isNull())
      nameAsIs = nameLenInBytes;
    
    // Copy from (last position in) input string up to current name
    qualifiedText += ComString(&pInputStr[inputStrPos],
                               pNameLoc->getNamePosition() - inputStrPos +
                               nameAsIs);
    
    if (NOT nameAsIs) // original name to be replaced with expanded
      {
        size_t namePos = pNameLoc->getNamePosition();
      size_t nameLen = pNameLoc->getNameLength();
      // Solution 10-080506-3000
      // For description and explanation of the fix, please read the
      // comments in method CatExecCreateView::buildViewText() in
      // module CatExecCreateView.cpp
      // Example: CREATE TABLE T ("c1" INT NOT NULL PRIMARY KEY,
      //          C2 INT CHECK (C2 BETWEEN 0 AND"c1")) NO PARTITION;
      if ( pInputStr[namePos] EQU '"'
           AND nameExpanded.data()[0] NEQ '"'
           AND namePos > 1
           AND ( pInputStr[namePos - 1] EQU '_' OR
                 isAlNumIsoMapCS((unsigned char)pInputStr[namePos - 1]) )
         )
      {
        // insert a blank separator to avoid syntax error
        // WITHOUT FIX - Example:
        // ... ALTER TABLE CAT.SCH.T ADD CONSTRAINT CAT.SCH.T_788388997_8627
        // CHECK (CAT.SCH.T.C2 BETWEEN 0 ANDCAT.SCH.T."c1") DROPPABLE ;
        // ...                           ^^^^^^
        qualifiedText += " "; // the FIX
        // WITH FIX - Example:
        // ... ALTER TABLE CAT.SCH.T ADD CONSTRAINT CAT.SCH.T_788388997_8627
        // CHECK (CAT.SCH.T.C2 BETWEEN 0 AND CAT.SCH.T."c1") DROPPABLE ;
        // ...                             ^^^
      }

      qualifiedText += nameExpanded;

      // Problem reported in solution 10-080506-3000
      // Example: CREATE TABLE T (C1 INT NOT NULL PRIMARY KEY,
      //          C2 INT CHECK ("C2"IN(1,2,3))) NO PARTITION;
      if ( pInputStr[namePos + nameLen - 1] EQU '"'
           AND nameExpanded.data()[nameExpanded.length() - 1] NEQ '"'
           AND pInputStr[namePos + nameLen] NEQ '\0'
           AND ( pInputStr[namePos + nameLen] EQU '_' OR
                 isAlNumIsoMapCS((unsigned char)pInputStr[namePos + nameLen]) )
         )
      {
        // insert a blank separator to avoid syntax error
        // WITHOUT FIX - Example:
        // ... ALTER TABLE CAT.SCH.T ADD CONSTRAINT CAT.SCH.T_654532688_9627
        // CHECK (CAT.SCH.T.C2IN (1, 2, 3)) DROPPABLE ;
        // ...              ^^^^
        qualifiedText += " "; // the FIX
        // WITH FIX - Example:
        // ... ALTER TABLE CAT.SCH.T ADD CONSTRAINT CAT.SCH.T_654532688_9627
        // CHECK (CAT.SCH.T.C2 IN (1, 2, 3)) DROPPABLE ;
        // ...               ^^^
      }
    } // if (NOT nameAsIs)

    inputStrPos = pNameLoc->getNamePosition() + nameLenInBytes;

  } // for

  //  CAT_ASSERT(addCheckNode->getEndPosition() >= inputStrPos);
  qualifiedText += ComString(&pInputStr[inputStrPos],
                             addCheckNode->getEndPosition() - inputStrPos + 1);
  
  PrettifySqlText(qualifiedText, NULL);

  //                  CharType::getCharSetAsPrefix(SqlParser_NATIONAL_CHARSET));
  
  return 0;
}

short CmpSeabaseDDL::getTextFromMD(
                                   ExeCliInterface * cliInterface,
                                   Int64 textUID,
                                   Lng32 textType,
                                   Lng32 textSubID,
                                   NAString &outText)
{
  Lng32 cliRC;

  char query[1000];

  str_sprintf(query, "select text from %s.\"%s\".%s where text_uid = %Ld and text_type = %d and sub_id = %d for read committed access order by seq_num",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TEXT,
              textUID, textType, textSubID);
  
  Queue * textQueue = NULL;
  cliRC = cliInterface->fetchAllRows(textQueue, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return -1;
    }
  
  // glue text together
  for (Lng32 idx = 0; idx < textQueue->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)textQueue->getNext(); 
    
      char * text = (char*)vi->get(0);
   
      outText += text;
    }

  return 0;
}

void CmpSeabaseDDL::alterSeabaseTableAddCheckConstraint(
                                                StmtDDLAddConstraint * alterAddConstraint,
                                                NAString &currCatName, NAString &currSchName)
{
  StmtDDLAddConstraintCheck *alterAddCheckNode = alterAddConstraint
    ->castToStmtDDLAddConstraintCheck();

  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterAddConstraint->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  if ((isSeabaseReservedSchema(tableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CANNOT_ALTER_DEFINITION_METADATA_SCHEMA);
      processReturn();
      return;
    }

  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  if (CmpCommon::getDefault(TRAF_RELOAD_NATABLE_CACHE) == DF_OFF)
    ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
              STMTHEAP,
              tableName.getSchemaNamePart().getInternalName(),
              tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
        << DgSqlCode(-4082)
        << DgTableName(cn.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  // Make sure user has the privilege to perform the add check constraint
  if (!isDDLOperationAuthorized(SQLOperation::ALTER_TABLE,
                                naTable->getOwner(),naTable->getSchemaOwner()))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);

     deallocEHI(ehi); 

     processReturn ();

     return;
  }

  const ParCheckConstraintColUsageList &colList = 
    alterAddCheckNode->getColumnUsageList();
  for (CollIndex cols = 0; cols < colList.entries(); cols++)
    {
      const ParCheckConstraintColUsage &ckColUsg = colList[cols];
      const ComString &colName = ckColUsg.getColumnName();
      if ((colName EQU "SYSKEY") &&
          (naTable->getClusteringIndex()->hasSyskey()))
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_SYSKEY_COL_NOT_ALLOWED_IN_CK_CNSTRNT)
                              << DgColumnName( "SYSKEY")
                              << DgTableName(extTableName);
          
          processReturn();
          
          deallocEHI(ehi); 

          return;
        }
    }

  NAList<NAString> keyColList;
  if (constraintErrorChecks(alterAddConstraint->castToStmtDDLAddConstraintCheck(),
                            naTable,
                            COM_CHECK_CONSTRAINT, 
                            keyColList))
    {
      return;
    }

  // update check constraint info
  NAString uniqueStr;
  if (genUniqueName(alterAddConstraint, uniqueStr))
    {
      return;
    }
  
  // get check text
  NAString origCheckConstrText;
  if (getCheckConstraintText(alterAddCheckNode, origCheckConstrText))
    {
      return;
    }

  NAString checkConstrText;
  ToQuotedString(checkConstrText, origCheckConstrText, FALSE);

  if (CmpCommon::getDefault(TRAF_NO_CONSTR_VALIDATION) == DF_OFF)
    {
      // validate data for check constraint.
      // generate a "select" statement to validate the constraint.  For example:
      // SELECT count(*) FROM T1 where not checkConstrText;
      // This statement returns > 0 if there exist data violating the constraint.
      char * validQry = new(STMTHEAP) char[checkConstrText.length() + 2000];
      
      str_sprintf(validQry, "select count(*) from \"%s\".\"%s\".\"%s\" where not %s",
                  tableName.getCatalogNamePart().getInternalName().data(),
                  tableName.getSchemaNamePart().getInternalName().data(),
                  tableName.getObjectNamePart().getInternalName().data(),
                  origCheckConstrText.data());
      
      Lng32 len = 0;
      Int64 rowCount = 0;
      cliRC = cliInterface.executeImmediate(validQry, (char*)&rowCount, &len, NULL);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          return;
        }
      
      if (rowCount > 0)
        {
          *CmpCommon::diags() << DgSqlCode(-1083)
                              << DgConstraintName(uniqueStr);
          return;
        }
    }
      
  Int64 tableUID = 
    getObjectUID(&cliInterface,
                 catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
                 COM_BASE_TABLE_OBJECT_LIT);

  ComUID comUID;
  comUID.make_UID();
  Int64 checkUID = comUID.get_value();

  NAList<NAString> emptyList;
  if (updateConstraintMD(keyColList, emptyList, uniqueStr, tableUID, checkUID, 
                         naTable, COM_CHECK_CONSTRAINT, TRUE, &cliInterface))
    {
      *CmpCommon::diags()
        << DgSqlCode(-1029)
        << DgTableName(uniqueStr);
      
      return;
    }

  if (updateTextTable(&cliInterface, checkUID, COM_CHECK_CONSTR_TEXT, 0,
                      checkConstrText))
    {
      processReturn();
      return;
    }

  if (updateObjectRedefTime(&cliInterface,
                            catalogNamePart, schemaNamePart, objectNamePart,
                            COM_BASE_TABLE_OBJECT_LIT))
    {
      processReturn();

      deallocEHI(ehi);

      return;
    }

  // remove NATable for this table
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  return;
}

void CmpSeabaseDDL::alterSeabaseTableDropConstraint(
                                                StmtDDLDropConstraint * alterDropConstraint,
                                                NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterDropConstraint->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  if ((isSeabaseReservedSchema(tableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CANNOT_ALTER_DEFINITION_METADATA_SCHEMA);
      processReturn();
      return;
    }

  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
              STMTHEAP,
              tableName.getSchemaNamePart().getInternalName(),
              tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
        << DgSqlCode(-4082)
        << DgTableName(cn.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      return;
    }

  // Make sure user has the privilege to perform the drop constraint
  if (!isDDLOperationAuthorized(SQLOperation::ALTER_TABLE,
                                naTable->getOwner(),naTable->getSchemaOwner()))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);

     deallocEHI(ehi); 

     processReturn ();

     return;
  }

  const NAString &dropConstrName = alterDropConstraint->
    getConstraintNameAsQualifiedName().getQualifiedNameAsAnsiString();
  const NAString &constrCatName = alterDropConstraint->
    getConstraintNameAsQualifiedName().getCatalogName();
  const NAString &constrSchName = alterDropConstraint->
    getConstraintNameAsQualifiedName().getSchemaName();
  const NAString &constrObjName = alterDropConstraint->
    getConstraintNameAsQualifiedName().getObjectName();

  char outObjType[10];
  Int64 constrUID = getObjectUID(&cliInterface,
                                 constrCatName.data(), constrSchName.data(), constrObjName.data(),
                                 NULL,
                                 "object_type = '"COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT"' or object_type = '"COM_UNIQUE_CONSTRAINT_OBJECT_LIT"' or object_type = '"COM_REFERENTIAL_CONSTRAINT_OBJECT_LIT"' or object_type = '"COM_CHECK_CONSTRAINT_OBJECT_LIT"' ",
                                 outObjType);
  if (constrUID < 0)
    {
      *CmpCommon::diags()
        << DgSqlCode(-1005)
        << DgConstraintName(dropConstrName);

      processReturn();

      return;
    }

  NABoolean isUniqConstr = 
    ((strcmp(outObjType, COM_UNIQUE_CONSTRAINT_OBJECT_LIT) == 0) ||
     (strcmp(outObjType, COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT) == 0));
  NABoolean isRefConstr = 
    (strcmp(outObjType, COM_REFERENTIAL_CONSTRAINT_OBJECT_LIT) == 0);
  NABoolean isPkeyConstr = 
    (strcmp(outObjType, COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT) == 0);
  NABoolean isCheckConstr = 
    (strcmp(outObjType, COM_CHECK_CONSTRAINT_OBJECT_LIT) == 0);

  if (isUniqConstr)
    {
      const AbstractRIConstraintList &ariList = naTable->getUniqueConstraints();
      for (Int32 i = 0; i < ariList.entries(); i++)
        {
          AbstractRIConstraint *ariConstr = ariList[i];
          UniqueConstraint * uniqueConstr = (UniqueConstraint*)ariList[i];
          
          const NAString &tableConstrName = 
            uniqueConstr->getConstraintName().getQualifiedNameAsAnsiString();
          
          if (dropConstrName == tableConstrName)
            {
              if (uniqueConstr->hasRefConstraintsReferencingMe())
                {
                  *CmpCommon::diags()
                    << DgSqlCode(-1050);
                  
                  deallocEHI(ehi);
                  processReturn();
                  return;
                }
            }
        } // for
    }
  
  NATable *otherNaTable = NULL;
  Int64 otherConstrUID = 0;
  if (isRefConstr)
    {
      RefConstraint * refConstr = NULL;
      
      const AbstractRIConstraintList &ariList = naTable->getRefConstraints();
      for (Int32 i = 0; i < ariList.entries(); i++)
        {
          AbstractRIConstraint *ariConstr = ariList[i];
          
          const NAString &tableConstrName = 
            ariConstr->getConstraintName().getQualifiedNameAsAnsiString();
          
          if (dropConstrName == tableConstrName)
            {
              refConstr = (RefConstraint*)ariConstr;
            }
        } // for
 
      CorrName otherCN(refConstr->getUniqueConstraintReferencedByMe().getTableName());
      otherNaTable = bindWA.getNATable(otherCN);
      if (otherNaTable == NULL || bindWA.errStatus())
        {
          deallocEHI(ehi); 
          
          processReturn();
          
          return;
        }

      AbstractRIConstraint * otherConstr = 
        refConstr->findConstraint(&bindWA, refConstr->getUniqueConstraintReferencedByMe());

      const NAString& otherCatName = 
        otherConstr->getConstraintName().getCatalogName();
      
      const NAString& otherSchName = 
        otherConstr->getConstraintName().getSchemaName();
      
      const NAString& otherConstrName = 
        otherConstr->getConstraintName().getObjectName();
      
      otherConstrUID = getObjectUID(&cliInterface,
                                    otherCatName.data(), otherSchName.data(), otherConstrName.data(),
                                    COM_UNIQUE_CONSTRAINT_OBJECT_LIT );
      if (otherConstrUID < 0)
        {
          CmpCommon::diags()->clear();
          otherConstrUID = getObjectUID(&cliInterface,
                                        otherCatName.data(), otherSchName.data(), otherConstrName.data(),
                                        COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT );
          if (otherConstrUID < 0)
            {
              processReturn();
              
              return;
            }
        }
    }

  NABoolean indexFound = FALSE;
  Lng32 isExplicit = 0;
  Lng32 keytag = 0;
  if ((isUniqConstr || isRefConstr) && (NOT isPkeyConstr))
    {
      // find the index that corresponds to this constraint
      char query[1000];
      
      str_sprintf(query, "select I.keytag, I.is_explicit from %s.\"%s\".%s T, %s.\"%s\".%s I where T.table_uid = %Ld and T.constraint_uid = %Ld and T.table_uid = I.base_table_uid and T.index_uid = I.index_uid ",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_INDEXES,
                  naTable->objectUid().castToInt64(),
                  constrUID);
      
      Queue * indexQueue = NULL;
      ExeCliInterface cliInterface(STMTHEAP);

      cliRC = cliInterface.fetchAllRows(indexQueue, query, 0, FALSE, FALSE, TRUE);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          
          processReturn();
          
          return;
        }

      if (indexQueue->numEntries() > 1)
        {
          *CmpCommon::diags()
            << DgSqlCode(-1005)
            << DgConstraintName(dropConstrName);
          
          processReturn();
          
          return;
        }

      if (indexQueue->numEntries() ==1)
        {
          indexFound = TRUE;
          indexQueue->position();
      
          OutputInfo * oi = (OutputInfo*)indexQueue->getCurr(); 
          keytag = *(Lng32*)oi->get(0);
          isExplicit = *(Lng32*)oi->get(1);
        }
    }

  if (deleteConstraintInfoFromSeabaseMDTables(&cliInterface,
                                              naTable->objectUid().castToInt64(),
                                              (otherNaTable ? otherNaTable->objectUid().castToInt64() : 0),
                                              constrUID,
                                              otherConstrUID, 
                                              constrCatName,
                                              constrSchName,
                                              constrObjName,
                                              (isPkeyConstr ? COM_PRIMARY_KEY_CONSTRAINT_OBJECT :
                                               (isUniqConstr ? COM_UNIQUE_CONSTRAINT_OBJECT :
                                                (isRefConstr ? COM_REFERENTIAL_CONSTRAINT_OBJECT :
                                                 COM_CHECK_CONSTRAINT_OBJECT)))))
    {
      processReturn();
      
      return;
    }
                                              
  // if the index corresponding to this constraint is an implicit index and 'no check'
  // option is not specified, drop it.
  if (((indexFound) && (NOT isExplicit) && (keytag != 0)) &&
      (alterDropConstraint->getDropBehavior() != COM_NO_CHECK_DROP_BEHAVIOR))
    {
      char buf[4000];

      str_sprintf(buf, "drop index \"%s\".\"%s\".\"%s\" no check",
                  constrCatName.data(), constrSchName.data(), constrObjName.data());

      cliRC = cliInterface.executeImmediate(buf);
      
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          return;
        }
    }

  if (updateObjectRedefTime(&cliInterface,
                            catalogNamePart, schemaNamePart, objectNamePart,
                            COM_BASE_TABLE_OBJECT_LIT))
    {
      processReturn();

      deallocEHI(ehi);

      return;
    }

  // remove NATable for this table
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  if (isRefConstr && otherNaTable)
  {
    CorrName otherCn(
      otherNaTable->getExtendedQualName().getQualifiedNameObj(), STMTHEAP);
    ActiveSchemaDB()->getNATableDB()->removeNATable(otherCn); 
  }
  return;
}


void CmpSeabaseDDL::seabaseGrantRevoke(
                                      StmtDDLNode                  * stmtDDLNode,
                                      NABoolean isGrant,
                                      NAString &currCatName, NAString &currSchName,
                                      NABoolean useHBase)
{
  StmtDDLGrant * grantNode = NULL;
  StmtDDLRevoke * revokeNode = NULL;
  NAString tabName;
  ComAnsiNameSpace nameSpace;  
  
  if (isGrant)
    {
      grantNode = stmtDDLNode->castToStmtDDLGrant();
      tabName = grantNode->getTableName();
      nameSpace = grantNode->getGrantNameAsQualifiedName().getObjectNameSpace();
    }
  else
    {
      revokeNode = stmtDDLNode->castToStmtDDLRevoke();
      tabName = revokeNode->getTableName();
      nameSpace = revokeNode->getRevokeNameAsQualifiedName().getObjectNameSpace();
    }

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  // If using HBase to perform authorization, call it now.
  if (useHBase || isHbase(tableName))
  {
    seabaseGrantRevokeHBase(stmtDDLNode, isGrant, currCatName, currSchName);
    return;
  }

  if (!isAuthorizationEnabled())
  {
    *CmpCommon::diags() << DgSqlCode(-CAT_AUTHORIZATION_NOT_ENABLED);
    return;
  }

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
              STMTHEAP,
              tableName.getSchemaNamePart().getInternalName(),
              tableName.getCatalogNamePart().getInternalName());

  ElemDDLGranteeArray & pGranteeArray = 
    (isGrant ? grantNode->getGranteeArray() : revokeNode->getGranteeArray());

  ElemDDLPrivActArray & pPrivActsArray =
    (isGrant ? grantNode->getPrivilegeActionArray() :
     revokeNode->getPrivilegeActionArray());

  NABoolean   allPrivs =
    (isGrant ? grantNode->isAllPrivilegesSpecified() : 
     revokeNode->isAllPrivilegesSpecified());

  NABoolean   isWGOSpecified =
    (isGrant ? grantNode->isWithGrantOptionSpecified() : 
     revokeNode->isGrantOptionForSpecified());

  NABoolean   isGrantedBySpecified =
    (isGrant ? grantNode->isByGrantorOptionSpecified() : 
     revokeNode->isByGrantorOptionSpecified());
     
  if (isGrantedBySpecified)
  {
    *CmpCommon::diags() << DgSqlCode(-CAT_OPTION_NOT_SUPPORTED)
                        << DgString0("GRANTED BY");
    processReturn();
    return;
  }

  vector<std::string> userPermissions;
  if (allPrivs)
    {
     userPermissions.push_back("ALL");
    }
  else
    {
      for (Lng32 i = 0; i < pPrivActsArray.entries(); i++)
        {
          switch (pPrivActsArray[i]->getOperatorType() )
            {
            case ELM_PRIV_ACT_SELECT_ELEM:
              {
                userPermissions.push_back("SELECT");
                break;
              }
              
            case ELM_PRIV_ACT_INSERT_ELEM:
              {
                userPermissions.push_back("INSERT");
                break;
              }
              
            case ELM_PRIV_ACT_DELETE_ELEM:
              {
                userPermissions.push_back("DELETE");
                break;
              }
              
            case ELM_PRIV_ACT_UPDATE_ELEM:   
              {
                userPermissions.push_back("UPDATE");
                break;
              }
              
            case ELM_PRIV_ACT_USAGE_ELEM:
              {
                userPermissions.push_back("USAGE");
                break;
              }
              
            case ELM_PRIV_ACT_REFERENCES_ELEM:
              {
                userPermissions.push_back("REFERENCES");
                break;
              }
              
            case ELM_PRIV_ACT_EXECUTE_ELEM:
              {
                userPermissions.push_back("EXECUTE");
                break;
              }
              
            case ELM_PRIV_ACT_ALL_DML_ELEM:
              {
                userPermissions.push_back("ALL_DML");
                break;
              }
              
            default:
              {
                NAString privType = "UNKNOWN";
                
                *CmpCommon::diags() << DgSqlCode(-CAT_INVALID_PRIV_FOR_OBJECT)
                                    <<  DgString0(privType)
                                    << DgString1(extTableName);
                
                
                processReturn();

                return;
              }
            }  // end switch
        } // for
    }

  // Prepare to call privilege manager
  NAString MDLoc;
  CONCAT_CATSCH(MDLoc, getSystemCatalog(), SEABASE_MD_SCHEMA);
  NAString privMgrMDLoc;
  CONCAT_CATSCH(privMgrMDLoc, getSystemCatalog(), SEABASE_PRIVMGR_SCHEMA);
  
  PrivMgrCommands command(std::string(MDLoc.data()), 
                          std::string(privMgrMDLoc.data()), 
                          CmpCommon::diags());

  // If the object is a metadata table or a privilege manager table, don't 
  // allow the privilege to be grantable.
  bool isPrivMgrTable = command.isPrivMgrTable(std::string(extTableName.data()));
  NABoolean isMDTable = (isSeabaseMD(tableName) || isPrivMgrTable) ? TRUE : FALSE;

  if (isMDTable && isWGOSpecified)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_WGO_NOT_ALLOWED);

      processReturn();
      return;
    }

  // Grants/revokes of the select privilege on metadata tables are allowed
  // Grants/revokes of other relevant privileges are allowed if parser flag
  //   INTERNAL_QUERY_FROM_EXEUTIL is set
  // Revoke:  allow ALL and ALL_DML to be specified
  if (isMDTable && (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      if (userPermissions.size() == 1 &&
          (userPermissions[0] == "SELECT" ||
           (!isGrant && userPermissions[0] == "ALL") ||
           (!isGrant && userPermissions[0] == "ALL_DML")))
        {} // operation allowed
      else
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_SMD_PRIVS_CANNOT_BE_CHANGED);

          processReturn();
          return;
        }
    }

  // set up common information for all grantees
  Int32 grantor = ComUser::getCurrentUser();
  const char *grantorNameChar  = GetCliGlobals()->currContext()->getDatabaseUserName(); 
  std::string grantorName (grantorNameChar);
  ComObjectType objectType = COM_BASE_TABLE_OBJECT;
  switch (nameSpace)
  {
    case COM_LIBRARY_NAME:
      objectType = COM_LIBRARY_OBJECT;
      break;
    case COM_UDF_NAME:
    case COM_UDR_NAME:
      objectType = COM_USER_DEFINED_ROUTINE_OBJECT;
      break;
    case COM_SEQUENCE_GENERATOR_NAME:
      objectType = COM_SEQUENCE_GENERATOR_OBJECT;
      break;
    default:
      objectType = COM_BASE_TABLE_OBJECT;
  }      

  // get the objectUID and objectOwner
  Int64 objectUID = 0;
  Int32 objectOwnerID = 0;
  if (objectType == COM_BASE_TABLE_OBJECT)
    {
      NATable *naTable = bindWA.getNATable(cn);
      if (naTable == NULL || bindWA.errStatus())
        {
          *CmpCommon::diags()
            << DgSqlCode(-4082)
            << DgTableName(cn.getExposedNameAsAnsiString());

          processReturn();
          return;
        }
      objectUID = (int64_t)naTable->objectUid().get_value();
      objectOwnerID = (int32_t)naTable->getOwner();
    }

  // for metadata tables, the objectUID is not initialized in the NATable
  // structure
  if (objectUID == 0)
    {
      ExeCliInterface cliInterface(STMTHEAP);
      Int32 objectOwnerID = 0;
      Int32 schemaOwnerID = 0;
      objectUID = getObjectUIDandOwners(&cliInterface,
                                       catalogNamePart.data(), schemaNamePart.data(),
                                       objectNamePart.data(), objectType,
                                       objectOwnerID,schemaOwnerID);

      if (objectUID == -1 || objectOwnerID == 0)
        {
          if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
            SEABASEDDL_INTERNAL_ERROR("getting object UID and object owner for grant/revoke request");
            processReturn();
            return;
          }
    }

  // TBD:  allow WGO once grant/restrict processing is completed
  if (isWGOSpecified )
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_WGO_NOT_ALLOWED);
      processReturn();
      return;
   }

  std::string objectName (extTableName.data());

  // For now, only support one grantee per request
  // TBD:  support multiple grantees - a testing effort?
  if (pGranteeArray.entries() > 1)
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_ONLY_ONE_GRANTEE_ALLOWED);
      processReturn();
      return;
    }

  for (CollIndex j = 0; j < pGranteeArray.entries(); j++)
    {
      NAString authName(pGranteeArray[j]->getAuthorizationIdentifier());
      Int32 grantee;
      if (pGranteeArray[j]->isPublic())
        {
          grantee = PUBLIC_USER;
          authName = "PUBLIC";
        }
      else
        {
          Int16 retcode = ComUser::getAuthIDFromAuthName(authName.data(), grantee);
          if (retcode == FENOTFOUND)
            {
              *CmpCommon::diags() << DgSqlCode(-CAT_AUTHID_DOES_NOT_EXIST_ERROR)
                                  << DgString0(authName.data());
              processReturn();
     
              return;
          }
          else if (retcode != FEOK)
            {
              *CmpCommon::diags() << DgSqlCode (-CAT_INTERNAL_EXCEPTION_ERROR)
                                  << DgString0(__FILE__)
                                  << DgInt0(__LINE__)
                                  << DgString1("verifying grantee");
               processReturn();
               return;
            }
        }

      std::string granteeName (authName.data());
      if (isGrant)
      {
        PrivStatus result = command.grantObjectPrivilege(objectUID, 
                                                         objectName, 
                                                         objectType, 
                                                         grantee, 
                                                         granteeName, 
                                                         grantor, 
                                                         grantorName, 
                                                         userPermissions,
                                                         allPrivs,
                                                         isWGOSpecified); 
      }
      else
      {
        PrivStatus result = command.revokeObjectPrivilege(objectUID, 
                                                          objectName, 
                                                          objectType, 
                                                          grantee, 
                                                          grantor, 
                                                          userPermissions, 
                                                          allPrivs, 
                                                          isWGOSpecified);
 
      }
  }
  return;
}

void CmpSeabaseDDL::seabaseGrantRevokeHBase(
                                             StmtDDLNode                  * stmtDDLNode,
                                             NABoolean isGrant,
                                             NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  StmtDDLGrant * grantNode = NULL;
  StmtDDLRevoke * revokeNode = NULL;
  NAString tabName;

  if (isGrant)
    {
      grantNode = stmtDDLNode->castToStmtDDLGrant();
      tabName = grantNode->getTableName();
    }
  else
    {
      revokeNode = stmtDDLNode->castToStmtDDLRevoke();
      tabName = revokeNode->getTableName();
    }

  ComObjectName tableName(tabName);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ExeCliInterface cliInterface(STMTHEAP);

  if (isSeabaseReservedSchema(tableName))
    {
      *CmpCommon::diags() << DgSqlCode(-1118)
                          << DgTableName(extTableName);
      //      deallocEHI(ehi); 
      processReturn();
      return;
    }

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();

      return;
    }

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
              STMTHEAP,
              tableName.getSchemaNamePart().getInternalName(),
              tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn);
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
        << DgSqlCode(-4082)
        << DgTableName(cn.getExposedNameAsAnsiString());

      deallocEHI(ehi);

      processReturn();

      return;
    }

  ElemDDLGranteeArray & pGranteeArray =
    (isGrant ? grantNode->getGranteeArray() : revokeNode->getGranteeArray());

  ElemDDLPrivActArray & pPrivActsArray =
    (isGrant ? grantNode->getPrivilegeActionArray() :
     revokeNode->getPrivilegeActionArray());

  NABoolean   allPrivs =
    (isGrant ? grantNode->isAllPrivilegesSpecified() :
     revokeNode->isAllPrivilegesSpecified());


  TextVec userPermissions;
  if (allPrivs)
    {
      userPermissions.push_back("READ");
      userPermissions.push_back("WRITE");
      userPermissions.push_back("CREATE");
    }
  else
    {
      for (Lng32 i = 0; i < pPrivActsArray.entries(); i++)
        {
          switch (pPrivActsArray[i]->getOperatorType() )
            {
            case ELM_PRIV_ACT_SELECT_ELEM:
              {
                userPermissions.push_back("READ");
                break;
              }

            case ELM_PRIV_ACT_INSERT_ELEM:
            case ELM_PRIV_ACT_DELETE_ELEM:
            case ELM_PRIV_ACT_UPDATE_ELEM:
              {
                userPermissions.push_back("WRITE");
                break;
              }

            case ELM_PRIV_ACT_CREATE_ELEM:
              {
                userPermissions.push_back("CREATE");
                break;
              }

            default:
              {
                NAString privType = "UNKNOWN";

                *CmpCommon::diags() << DgSqlCode(-CAT_INVALID_PRIV_FOR_OBJECT)
                                    <<  DgString0(privType)
                                    << DgString1(extTableName);

                deallocEHI(ehi);

                processReturn();

                return;
              }
            }  // end switch
        } // for
    }
  for (CollIndex j = 0; j < pGranteeArray.entries(); j++)
    {
      NAString authName(pGranteeArray[j]->getAuthorizationIdentifier());

      if (isGrant)
        retcode = ehi->grant(authName.data(), extNameForHbase.data(), userPermissions);
      else
        retcode = ehi->revoke(authName.data(), extNameForHbase.data(), userPermissions);

      if (retcode < 0)
        {
          *CmpCommon::diags() << DgSqlCode(-8448)
                              << (isGrant ? DgString0((char*)"ExpHbaseInterface::grant()") :
                                  DgString0((char*)"ExpHbaseInterface::revoke()"))
                              << DgString1(getHbaseErrStr(-retcode))
                              << DgInt0(-retcode)
                              << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());

          deallocEHI(ehi);

          processReturn();

          return;
        }
    }

  retcode = ehi->close();
  if (retcode < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-8448)
                          << DgString0((char*)"ExpHbaseInterface::close()")
                          << DgString1(getHbaseErrStr(-retcode))
                          << DgInt0(-retcode)
                          << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());

      deallocEHI(ehi);

      processReturn();

      return;
    }

  deallocEHI(ehi);

  processReturn();

  return;
}

void CmpSeabaseDDL::createNativeHbaseTable(
                                       StmtDDLCreateHbaseTable * createTableNode,
                                       NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ComObjectName tableName(createTableNode->getTableName());
  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  
  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();

      return;
    }

  const char * cf1 = NULL;
  const char * cf2 = NULL;
  const char * cf3 = NULL;
  for (Lng32 i = 0; i < createTableNode->csl()->entries(); i++)
    {
      const NAString * nas = (NAString*)(*createTableNode->csl())[i];

      switch (i)
        {
        case 0: cf1 = nas->data(); break;
        case 1: cf2 = nas->data(); break;
        case 2: cf3 = nas->data(); break;
        default: break;
        }

    }

  NAText hbaseOptionsStr;
  NAList<HbaseCreateOption*> * hbaseCreateOptions = NULL;
  if (createTableNode->getHbaseOptionsClause())
    {
      hbaseCreateOptions = 
        &createTableNode->getHbaseOptionsClause()->getHbaseOptions();
      
      for (CollIndex i = 0; i < hbaseCreateOptions->entries(); i++)
        {
          HbaseCreateOption * hbaseOption = (*hbaseCreateOptions)[i];

          hbaseOptionsStr += hbaseOption->key();
          hbaseOptionsStr += " = ''";
          hbaseOptionsStr += hbaseOption->val();
          hbaseOptionsStr += "'' ";
        }
    }

  HbaseStr hbaseTable;
  hbaseTable.val = (char*)objectNamePart.data();
  hbaseTable.len = objectNamePart.length();
  if (createHbaseTable(ehi, &hbaseTable, cf1, cf2, cf3,
                       hbaseCreateOptions, 0, 0, NULL, FALSE) == -1)
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }

}

void CmpSeabaseDDL::dropNativeHbaseTable(
                                       StmtDDLDropHbaseTable * dropTableNode,
                                       NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ComObjectName tableName(dropTableNode->getTableName());
  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  
  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();

      return;
    }

  HbaseStr hbaseTable;
  hbaseTable.val = (char*)objectNamePart.data();
  hbaseTable.len = objectNamePart.length();
  retcode = dropHbaseTable(ehi, &hbaseTable);
  if (retcode < 0)
    {
      deallocEHI(ehi); 
      
      processReturn();
      
      return;
    }
  
}

/////////////////////////////////////////////////////////////////////////
// This method generates and returns tableInfo struct for internal special
// tables (like metadata, histograms). These tables have hardcoded definitions
// but need objectUID to be returned. ObjectUID is stored in metadata
// and is read from there.
// This is done only if we are not in bootstrap mode, for example, when initializing
// metadata. At that time, there is no metadata available so it cannot be read
// to return objectUID.
// A NULL tableInfo is returned if in bootstrap mode.
//
// RETURN: -1, if error. 0, if all ok.
//////////////////////////////////////////////////////////////////////////
short CmpSeabaseDDL::getSpecialTableInfo
(
 const NAString &catName, 
 const NAString &schName, 
 const NAString &objName,
 const NAString &extTableName,
 const ComObjectType  objType,
 ComTdbVirtTableTableInfo* &tableInfo)
{
  Lng32 cliRC = 0;
  tableInfo = NULL;
  if (CmpCommon::getDefault(TRAF_BOOTSTRAP_MD_MODE) == DF_OFF)
    {
      ExeCliInterface cliInterface(STMTHEAP);
      
      cliRC = cliInterface.holdAndSetCQD("traf_bootstrap_md_mode", "ON");
      if (cliRC < 0)
        {
          return -1;
        }

      Int32 objectOwner = NA_UserIdDefault;
      Int32 schemaOwner = NA_UserIdDefault;
      Int64 objUID = getObjectUIDandOwners(&cliInterface, 
                                           catName.data(), schName.data(), objName.data(), 
                                           objType, objectOwner, schemaOwner);
      cliRC = cliInterface.restoreCQD("traf_bootstrap_md_mode");
      if (objUID <= 0)
        return -1;

      tableInfo = new(STMTHEAP) ComTdbVirtTableTableInfo[1];
      tableInfo->tableName = extTableName.data();
      tableInfo->createTime = 0;
      tableInfo->redefTime = 0;
      tableInfo->objUID = objUID;
      tableInfo->isAudited = 1;
      tableInfo->validDef = 1;
      tableInfo->objOwnerID = objectOwner;
      tableInfo->schemaOwnerID = schemaOwner;
      tableInfo->hbaseCreateOptions = NULL;
    }

  return 0;
}

desc_struct * CmpSeabaseDDL::getSeabaseMDTableDesc(
                                                   const NAString &catName, 
                                                   const NAString &schName, 
                                                   const NAString &objName,
                                                   const ComObjectType objType)
{
  Lng32 cliRC = 0;

  desc_struct * tableDesc = NULL;
  NAString schNameL = "\"";
  schNameL += schName;
  schNameL += "\"";

  ComObjectName coName(catName, schNameL, objName);
  NAString extTableName = coName.getExternalName(TRUE);

  Lng32 colInfoSize;
  const ComTdbVirtTableColumnInfo * colInfo;
  Lng32 keyInfoSize;
  const ComTdbVirtTableKeyInfo * keyInfo;

  Lng32 indexInfoSize = 0;
  const ComTdbVirtTableIndexInfo * indexInfo = NULL;
  if (NOT CmpSeabaseMDupgrade::getMDtableInfo(objName,
                                              colInfoSize, colInfo,
                                              keyInfoSize, keyInfo,
                                              indexInfoSize, indexInfo,
                                              objType))
    return NULL;

  ComTdbVirtTableTableInfo * tableInfo = NULL;
  if (getSpecialTableInfo(catName, schName, objName,
                          extTableName, objType, tableInfo))
    return NULL;

  tableDesc =
    Generator::createVirtualTableDesc
    ((char*)extTableName.data(),
     colInfoSize,
     (ComTdbVirtTableColumnInfo*)colInfo,
     keyInfoSize,
     (ComTdbVirtTableKeyInfo*)keyInfo,
     0, NULL,
     indexInfoSize, 
     (ComTdbVirtTableIndexInfo *)indexInfo,
     0, NULL,
     tableInfo);

  return tableDesc;

}

desc_struct * CmpSeabaseDDL::getSeabaseHistTableDesc(const NAString &catName, 
                                                     const NAString &schName, 
                                                     const NAString &objName)
{
  Lng32 cliRC = 0;

  desc_struct * tableDesc = NULL;
  NAString schNameL = "\"";
  schNameL += schName;
  schNameL += "\"";

  ComObjectName coName(catName, schNameL, objName);
  NAString extTableName = coName.getExternalName(TRUE);

  Lng32 numCols = 0;
  ComTdbVirtTableColumnInfo * colInfo = NULL;
  Lng32 numKeys;
  ComTdbVirtTableKeyInfo * keyInfo;
  ComTdbVirtTableIndexInfo * indexInfo;

  Parser parser(CmpCommon::context());

  ComTdbVirtTableConstraintInfo * constrInfo =
    new(STMTHEAP) ComTdbVirtTableConstraintInfo[1];

  NAString constrName;
  if (objName == HBASE_HIST_NAME)
    {
      if (processDDLandCreateDescs(parser,
                                   seabaseHistogramsDDL, sizeof(seabaseHistogramsDDL),
                                   FALSE,
                                   0, NULL, 0, NULL,
                                   numCols, colInfo,
                                   numKeys, keyInfo,
                                   indexInfo))
        return NULL;

      constrName = HBASE_HIST_PK;
    }
  else if (objName == HBASE_HISTINT_NAME)
    {
      if (processDDLandCreateDescs(parser,
                                   seabaseHistogramIntervalsDDL, sizeof(seabaseHistogramIntervalsDDL),
                                   FALSE,
                                   0, NULL, 0, NULL,
                                   numCols, colInfo,
                                   numKeys, keyInfo,
                                   indexInfo))
        return NULL;
      
      constrName = HBASE_HISTINT_PK;
    }
  else
    return NULL;
  
  ComObjectName coConstrName(catName, schName, constrName);
  NAString * extConstrName = 
    new(STMTHEAP) NAString(coConstrName.getExternalName(TRUE));
  
  constrInfo->baseTableName = (char*)extTableName.data();
  constrInfo->constrName = (char*)extConstrName->data();
  constrInfo->constrType = 3; // pkey_constr

  constrInfo->colCount = numKeys;
  constrInfo->keyInfoArray = keyInfo;

  constrInfo->numRingConstr = 0;
  constrInfo->ringConstrArray = NULL;
  constrInfo->numRefdConstr = 0;
  constrInfo->refdConstrArray = NULL;
  
  constrInfo->checkConstrLen = 0;
  constrInfo->checkConstrText = NULL;

  ComTdbVirtTableTableInfo * tableInfo = NULL;
  if (getSpecialTableInfo(catName, schName, objName,
                          extTableName, COM_BASE_TABLE_OBJECT, tableInfo))
    return NULL;

  tableDesc =
    Generator::createVirtualTableDesc
    ((char*)extTableName.data(),
     numCols,
     colInfo,
     numKeys,
     keyInfo,
     1, constrInfo,
     0, NULL,
     0, NULL,
     tableInfo);

  return tableDesc;
}

Lng32 CmpSeabaseDDL::getSeabaseColumnInfo(ExeCliInterface *cliInterface,
                                          Int64 objUID,
                                          const NAString &catName,
                                          const NAString &schName,
                                          const NAString &objName,
                                          char *direction,
                                          NABoolean *isTableSalted,
                                          Lng32 *identityColPos,
                                          Lng32 *numCols,
                                          ComTdbVirtTableColumnInfo **outColInfoArray)
{
  char query[3000];
  Lng32 cliRC;

  if (identityColPos)
    *identityColPos = -1;

  Queue * tableColInfo = NULL;
  str_sprintf(query, "select column_name, column_number, column_class, "
    "fs_data_type, column_size, column_precision, column_scale, "
    "datetime_start_field, datetime_end_field, trim(is_upshifted), column_flags, "
    "nullable, trim(character_set), default_class, default_value, "
    "trim(column_heading), hbase_col_family, hbase_col_qualifier, direction, "
    "is_optional, flags  from %s.\"%s\".%s "
    "where object_uid = %Ld and direction in (%s)"
    "order by 2 for read committed access",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
              objUID,
              direction);
  cliRC = cliInterface->fetchAllRows(tableColInfo, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
  {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
  }

  *numCols = tableColInfo->numEntries();
  ComTdbVirtTableColumnInfo *colInfoArray = 
    new(STMTHEAP) ComTdbVirtTableColumnInfo[*numCols];
  NABoolean tableIsSalted = FALSE;
  tableColInfo->position();
  for (Lng32 idx = 0; idx < *numCols; idx++)
  {
      OutputInfo * oi = (OutputInfo*)tableColInfo->getNext(); 
      ComTdbVirtTableColumnInfo &colInfo = colInfoArray[idx];

      char * data = NULL;
      Lng32 len = 0;

      // get the column name
      oi->get(0, data, len);
      colInfo.colName = new(STMTHEAP) char[len + 1];
      strcpy((char*)colInfo.colName, data);
      
      colInfo.colNumber = *(Lng32*)oi->get(1);

      char *colClass = (char*)oi->get(2);
      if (strcmp(colClass,COM_USER_COLUMN_LIT) == 0)
        colInfo.columnClass = COM_USER_COLUMN;
      else if (strcmp(colClass,COM_SYSTEM_COLUMN_LIT) == 0)
        colInfo.columnClass = COM_SYSTEM_COLUMN;
      else if (strcmp(colClass,COM_ADDED_USER_COLUMN_LIT) == 0)
        colInfo.columnClass = COM_ADDED_USER_COLUMN;
      else if (strcmp(colClass,COM_MV_SYSTEM_ADDED_COLUMN_LIT) == 0)
        colInfo.columnClass = COM_MV_SYSTEM_ADDED_COLUMN;
      else
        CMPASSERT(0);

      colInfo.datatype = *(Lng32*)oi->get(3);
      
      colInfo.length = *(Lng32*)oi->get(4);

      colInfo.precision = *(Lng32*)oi->get(5);

      colInfo.scale = *(Lng32*)oi->get(6);

      colInfo.dtStart = *(Lng32 *)oi->get(7);

      colInfo.dtEnd = *(Lng32 *)oi->get(8);

      if (strcmp((char*)oi->get(9), "Y") == 0)
        colInfo.upshifted = -1;
      else
        colInfo.upshifted = 0;

      colInfo.hbaseColFlags = *(ULng32 *)oi->get(10);

      colInfo.nullable = *(Lng32 *)oi->get(11);

      colInfo.charset = 
        (SQLCHARSET_CODE)CharInfo::getCharSetEnum((char*)oi->get(12));

      colInfo.defaultClass = (ComColumnDefaultClass)*(Lng32 *)oi->get(13);

      NAString tempDefVal;
      data = NULL;
      if (colInfo.defaultClass == COM_USER_DEFINED_DEFAULT ||
          colInfo.defaultClass == COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT ||
          colInfo.defaultClass == COM_ALWAYS_DEFAULT_COMPUTED_COLUMN_DEFAULT)
        {
          oi->get(14, data, len);
          if (colInfo.defaultClass != COM_USER_DEFINED_DEFAULT)
            {
              // get computed column definition from text table, but note
              // that for older tables the definition may be stored in
              // COLUMNS.DEFAULT_VALUE instead (that's returned in "data")
              cliRC = getTextFromMD(cliInterface,
                                    objUID,
                                    COM_COMPUTED_COL_TEXT,
                                    colInfo.colNumber,
                                    tempDefVal);
              if (cliRC < 0)
                {
                  cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
                  return -1;
                }
              if (strcmp(colInfo.colName,
                         ElemDDLSaltOptionsClause::getSaltSysColName()) == 0)
                tableIsSalted = TRUE;
            }
        }
      else if (colInfo.defaultClass == COM_NULL_DEFAULT)
        {
          tempDefVal = "NULL";
        }
      else if (colInfo.defaultClass == COM_USER_FUNCTION_DEFAULT)
        {
          tempDefVal = "USER";
        }
      else if (colInfo.defaultClass == COM_CURRENT_DEFAULT)
        {
          tempDefVal = "CURRENT_TIMESTAMP";
        }
      else if ((colInfo.defaultClass == COM_IDENTITY_GENERATED_BY_DEFAULT) ||
               (colInfo.defaultClass == COM_IDENTITY_GENERATED_ALWAYS))
        {
          NAString  userFunc("SEQNUM(");

          NAString seqName;
          SequenceGeneratorAttributes::genSequenceName
            (catName, schName, objName, colInfo.colName,
             seqName);

          NAString fullyQSeq = catName + "." + schName + "." + "\"" + seqName + "\"";

          tempDefVal = userFunc + fullyQSeq + ")";

          if (identityColPos)
            *identityColPos = idx;
        }

      if (! tempDefVal.isNull())
        {
          data = (char*)tempDefVal.data();
          len = tempDefVal.length();
        }

      if (colInfo.defaultClass != COM_NO_DEFAULT)
        {
          colInfo.defVal = new(STMTHEAP) char[len + 2];
          str_cpy_all((char*)colInfo.defVal, data, len);
          char * c = (char*)colInfo.defVal;
          c[len] = 0;
          c[len+1] = 0;
        }
      else
        colInfo.defVal = NULL;

      oi->get(15, data, len);
      if (len > 0)
        {
          colInfo.colHeading = new(STMTHEAP) char[len + 1];
          strcpy((char*)colInfo.colHeading, data);
        }
      else
        colInfo.colHeading = NULL;

      oi->get(16, data, len);
      colInfo.hbaseColFam = new(STMTHEAP) char[len + 1];
      strcpy((char*)colInfo.hbaseColFam, data);

      oi->get(17, data, len);
      colInfo.hbaseColQual = new(STMTHEAP) char[len + 1];
      strcpy((char*)colInfo.hbaseColQual, data);

      strcpy(colInfo.paramDirection, (char*)oi->get(18));
      if (*((char*)oi->get(19)) == 'Y')
         colInfo.isOptional = 1;
      else
         colInfo.isOptional = 0;
      colInfo.colFlags = *(Int64 *)oi->get(20);
      // temporary code, until we have updated flags to have the salt
      // flag set for all tables, even those created before end of November
      // 2014, when the flag was added during Trafodion R1.0 development
      if (colInfo.defaultClass == COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT &&
          strcmp(colInfo.colName,
                 ElemDDLSaltOptionsClause::getSaltSysColName()) == 0)
        colInfo.colFlags |=  SEABASE_COLUMN_IS_SALT;
   }
   if (isTableSalted != NULL)
      *isTableSalted = tableIsSalted;
   *outColInfoArray = colInfoArray;
   return *numCols;
}

ComTdbVirtTableSequenceInfo * CmpSeabaseDDL::getSeabaseSequenceInfo(
 const NAString &catName, 
 const NAString &schName, 
 const NAString &seqName,
 NAString &extSeqName,
 Int32 & objectOwner,
 Int32 & schemaOwner,
 Int64 & seqUID)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  NAString schNameL = "\"";
  schNameL += schName;
  schNameL += "\"";

  NAString seqNameL = "\"";
  seqNameL += seqName;
  seqNameL += "\"";
  ComObjectName coName(catName, schNameL, seqNameL);
  extSeqName = coName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP);

  objectOwner = NA_UserIdDefault;
  seqUID = -1;
  schemaOwner = NA_UserIdDefault;
  seqUID = getObjectUIDandOwners(&cliInterface,
                                 catName.data(), schName.data(), seqName.data(),
                                 COM_SEQUENCE_GENERATOR_OBJECT,  
                                 objectOwner,schemaOwner,TRUE/*report error*/);
  if (seqUID == -1 || objectOwner == 0)
  {
    // There may not be an error in the diags area, if not, add an error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      SEABASEDDL_INTERNAL_ERROR("getting object UID and owners for get sequence command");
    return NULL;
  }

 char buf[4000];

 str_sprintf(buf, "select fs_data_type, start_value, increment, max_value, min_value, cycle_option, cache_size, next_value, seq_type, redef_ts from %s.\"%s\".%s  where seq_uid = %Ld",
             getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_SEQ_GEN,
             seqUID);

  Queue * seqQueue = NULL;
  cliRC = cliInterface.fetchAllRows(seqQueue, buf, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return NULL;
    }
 
  if ((seqQueue->numEntries() == 0) ||
      (seqQueue->numEntries() > 1))
    {
      *CmpCommon::diags() << DgSqlCode(-4082)
                          << DgTableName(extSeqName);
      
      return NULL;
    }

  ComTdbVirtTableSequenceInfo *seqInfo = 
    new (STMTHEAP) ComTdbVirtTableSequenceInfo();

  seqQueue->position();
  OutputInfo * vi = (OutputInfo*)seqQueue->getNext(); 
 
  seqInfo->datatype = *(Lng32*)vi->get(0);
  seqInfo->startValue = *(Int64*)vi->get(1);
  seqInfo->increment = *(Int64*)vi->get(2);
  seqInfo->maxValue = *(Int64*)vi->get(3);
  seqInfo->minValue = *(Int64*)vi->get(4);
  seqInfo->cycleOption = (memcmp(vi->get(5), COM_YES_LIT, 1) == 0 ? 1 : 0);
  seqInfo->cache  = *(Int64*)vi->get(6);
  seqInfo->nextValue  = *(Int64*)vi->get(7);
  seqInfo->seqType = (memcmp(vi->get(8), "E", 1) == 0 ? COM_EXTERNAL_SG : COM_INTERNAL_SG);
  seqInfo->seqUID = seqUID;
  seqInfo->redefTime = *(Int64*)vi->get(9);

  return seqInfo;
}

desc_struct * CmpSeabaseDDL::getSeabaseSequenceDesc(const NAString &catName, 
                                                    const NAString &schName, 
                                                    const NAString &seqName)
{
  desc_struct * tableDesc = NULL;

  NAString extSeqName;
  Int32 objectOwner = 0;
  Int32 schemaOwner = 0;
  Int64 seqUID = -1;
  ComTdbVirtTableSequenceInfo * seqInfo =
    getSeabaseSequenceInfo(catName, schName, seqName, extSeqName, 
                           objectOwner, schemaOwner, seqUID);

  if (! seqInfo)
    {
      return NULL;
    }

  ComTdbVirtTableTableInfo * tableInfo =
    new(STMTHEAP) ComTdbVirtTableTableInfo[1];
  tableInfo->tableName = extSeqName.data();
  tableInfo->createTime = 0;
  tableInfo->redefTime = 0;
  tableInfo->objUID = seqUID;
  tableInfo->isAudited = 0;
  tableInfo->validDef = 1;
  tableInfo->objOwnerID = objectOwner;
  tableInfo->schemaOwnerID = schemaOwner;
  tableInfo->hbaseCreateOptions = NULL;

  tableDesc =
    Generator::createVirtualTableDesc
    ((char*)extSeqName.data(),
     0, NULL, // colInfo
     0, NULL, // keyInfo
     0, NULL,
     0, NULL, //indexInfo
     0, NULL, // viewInfo
     tableInfo,
     seqInfo);
  
  return tableDesc;
}

desc_struct * CmpSeabaseDDL::getSeabaseUserTableDesc(const NAString &catName, 
                                                     const NAString &schName, 
                                                     const NAString &objName,
                                                     const ComObjectType objType,
                                                     NABoolean includeInvalidDefs)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char query[4000];
  ExeCliInterface cliInterface(STMTHEAP);

  desc_struct * tableDesc = NULL;

  Int32 objectOwner =  0 ;
  Int32 schemaOwner =  0 ;
  Int64 objUID      = -1 ;

  //
  // For performance reasons, whenever possible, we want to issue only one
  // "select" to the OBJECTS metadata table to determine both the existence
  // of the specified table and the objUID for the table.  Since it is more
  // likely that a user query refers to tables (directly or indirectly) that
  // are already in existence, this optimization can save the cost of the
  // existence check for all such user objects.  In the less likely case that
  // an object does not exist we must drop back and re-issue the metadata
  // query for the existence check in order to ensure we get the proper error
  // reported.
  //
  if ( objType ) // Must have objType
  {
    objUID = getObjectUIDandOwners(&cliInterface,
                              catName.data(), schName.data(), objName.data(),
                                      objType, objectOwner, schemaOwner, FALSE /*no error now */);
  }
  // If we didn't call getObjectUIDandOwner() above OR if it gave an error, then:
  if ( objUID < 0 )
  {
    cliRC = existsInSeabaseMDTable(&cliInterface, 
                                 catName.data(), schName.data(), objName.data(),
                                 COM_UNKNOWN_OBJECT,
                                 (objType != COM_INDEX_OBJECT ? TRUE : FALSE));
    if (cliRC < 0)
      {
        processReturn();

        return NULL;
      }

    if (cliRC == 0) // doesn't exist
      {
        processReturn();

        return NULL;
      }
  }

  if (objUID < 0)
  {
     if (objType != COM_BASE_TABLE_OBJECT)
     {
        processReturn();
        return NULL;
     }
     else
     {
       // object type passed in was for a table. Could not find it but.
       // this could be a view. Look for that.
       CmpCommon::diags()->clear();
       objUID = getObjectUIDandOwners(&cliInterface,
                                      catName.data(), schName.data(), objName.data(), COM_VIEW_OBJECT,  
                                      objectOwner,schemaOwner);
       if (objUID < 0)
         {
          processReturn();
          return NULL;
       }
    }
  }

  str_sprintf(query, "select is_audited, num_salt_partns, row_format from %s.\"%s\".%s where table_uid = %Ld for read committed access",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLES,
              objUID);

  Queue * tableAttrQueue = NULL;
  cliRC = cliInterface.fetchAllRows(tableAttrQueue, query, 0, FALSE, FALSE, TRUE);

  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return NULL;
    }

  NABoolean isAudited = TRUE;
  Lng32 numSaltPartns = 0;
  NABoolean alignedFormat = FALSE;
  NAString *  hbaseCreateOptions = new(STMTHEAP) NAString();
  if (cliRC == 0) // read some rows
    {
      if (tableAttrQueue->entries() != 1) // only one row should be returned
        {
          processReturn();
          return NULL;                     
        }
      
      tableAttrQueue->position();
      OutputInfo * vi = (OutputInfo*)tableAttrQueue->getNext();
      
      char * audit = vi->get(0);
      isAudited =  (memcmp(audit, COM_YES_LIT, 1) == 0);
      
      numSaltPartns = *(Lng32*)vi->get(1);

      char * format = vi->get(1);
      alignedFormat = (memcmp(format, COM_ALIGNED_FORMAT_LIT, 2) == 0);

      if (getTextFromMD(&cliInterface, objUID, COM_HBASE_OPTIONS_TEXT, 0,
                        *hbaseCreateOptions))
        {
          processReturn();
          return NULL;
        }
    }

  Lng32 numCols;
  ComTdbVirtTableColumnInfo * colInfoArray;

  NABoolean tableIsSalted = FALSE;
  char direction[20];
  str_sprintf(direction, "'%s'", COM_UNKNOWN_PARAM_DIRECTION_LIT);

  Lng32 identityColPos = -1;
  if (getSeabaseColumnInfo(&cliInterface,
                           objUID,
                           catName, schName, objName,
                           (char *)direction,
                           &tableIsSalted,
                           &identityColPos,
                           &numCols,
                           &colInfoArray) <= 0)
    {
     processReturn();
     return NULL;                     
  } 

  if (objType == COM_INDEX_OBJECT)
    {
      str_sprintf(query, "select k.column_name, c.column_number, k.keyseq_number, ordering, cast(0 as int not null)  from %s.\"%s\".%s k, %s.\"%s\".%s c where k.column_name = c.column_name and k.object_uid = c.object_uid and k.object_uid = %Ld and k.nonkeycol = 0 for read committed access order by keyseq_number",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
                  objUID);
    }
  else
    {
      str_sprintf(query, "select column_name, column_number, keyseq_number, ordering, cast(0 as int not null)  from %s.\"%s\".%s where object_uid = %Ld and nonkeycol = 0 for read committed access order by keyseq_number",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
                  objUID);
    }

  Queue * tableKeyInfo = NULL;
  cliRC = cliInterface.fetchAllRows(tableKeyInfo, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return NULL;
    }

  ComTdbVirtTableKeyInfo * keyInfoArray = NULL;
  
  if (tableKeyInfo->numEntries() > 0)
    {
      keyInfoArray = 
        new(STMTHEAP) ComTdbVirtTableKeyInfo[tableKeyInfo->numEntries()];
    }

  tableKeyInfo->position();
  for (int idx = 0; idx < tableKeyInfo->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)tableKeyInfo->getNext(); 

      populateKeyInfo(keyInfoArray[idx], vi);
    }

  str_sprintf(query, "select O.catalog_name, O.schema_name, O.object_name, I.keytag, I.is_unique, I.is_explicit, I.key_colcount, I.nonkey_colcount, T.num_salt_partns from %s.\"%s\".%s I, %s.\"%s\".%s O ,  %s.\"%s\".%s T where I.base_table_uid = %Ld and I.index_uid = O.object_uid %s and I.index_uid = T.table_uid for read committed access order by 1,2,3",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_INDEXES,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLES,
              objUID,
              (includeInvalidDefs ? " " : " and O.valid_def = 'Y' "));

  Queue * indexInfoQueue = NULL;
  cliRC = cliInterface.fetchAllRows(indexInfoQueue, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return NULL;
    }

  ComTdbVirtTableIndexInfo * indexInfoArray = NULL;
  if (indexInfoQueue->numEntries() > 0)
    {
      indexInfoArray = 
        new(STMTHEAP) ComTdbVirtTableIndexInfo[indexInfoQueue->numEntries()];
    }

  NAString qCatName = "\"";
  qCatName += catName;
  qCatName += "\"";
  NAString qSchName = "\"";
  qSchName += schName;
  qSchName += "\"";
  NAString qObjName = "\"";
  qObjName += objName;
  qObjName += "\"";

  ComObjectName coName(qCatName, qSchName, qObjName);
  NAString * extTableName =
    new(STMTHEAP) NAString(coName.getExternalName(TRUE));

  indexInfoQueue->position();
  for (int idx = 0; idx < indexInfoQueue->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)indexInfoQueue->getNext(); 
      
      char * idxCatName = (char*)vi->get(0);
      char * idxSchName = (char*)vi->get(1);
      char * idxObjName = (char*)vi->get(2);
      Lng32 keyTag = *(Lng32*)vi->get(3);
      Lng32 isUnique = *(Lng32*)vi->get(4);
      Lng32 isExplicit = *(Lng32*)vi->get(5);
      Lng32 keyColCount = *(Lng32*)vi->get(6);
      Lng32 nonKeyColCount = *(Lng32*)vi->get(7);
      Lng32 idxNumSaltPartns = *(Lng32*)vi->get(8);

      Int64 idxUID = getObjectUID(&cliInterface,
                                  idxCatName, idxSchName, idxObjName,
                                  COM_INDEX_OBJECT_LIT);
      if (idxUID < 0)
        {

          processReturn();

          return NULL;
        }

      NAString * idxHbaseCreateOptions = new(STMTHEAP) NAString();
      if (getTextFromMD(&cliInterface, idxUID, COM_HBASE_OPTIONS_TEXT, 0,
                        *idxHbaseCreateOptions))
        {
          processReturn();
          
          return NULL;
        }

      indexInfoArray[idx].baseTableName = (char*)extTableName->data();

      NAString qIdxCatName = "\"";
      qIdxCatName += idxCatName;
      qIdxCatName += "\"";
      NAString qIdxSchName = "\"";
      qIdxSchName += idxSchName;
      qIdxSchName += "\"";
      NAString qIdxObjName = "\"";
      qIdxObjName += idxObjName;
      qIdxObjName += "\"";

      ComObjectName coIdxName(qIdxCatName, qIdxSchName, qIdxObjName);
      
      NAString * extIndexName = 
        new(STMTHEAP) NAString(coIdxName.getExternalName(TRUE));

      indexInfoArray[idx].indexName = (char*)extIndexName->data();
      indexInfoArray[idx].keytag = keyTag;
      indexInfoArray[idx].isUnique = isUnique;
      indexInfoArray[idx].isExplicit = isExplicit;
      indexInfoArray[idx].keyColCount = keyColCount;
      indexInfoArray[idx].nonKeyColCount = nonKeyColCount;
      indexInfoArray[idx].hbaseCreateOptions = 
        (idxHbaseCreateOptions->isNull() ? NULL : idxHbaseCreateOptions->data());
      indexInfoArray[idx].numSaltPartns = idxNumSaltPartns;

      Queue * keyInfoQueue = NULL;
      str_sprintf(query, "select column_name, column_number, keyseq_number, ordering, nonkeycol  from %s.\"%s\".%s where object_uid = %Ld for read committed access order by keyseq_number",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
                  idxUID);
      cliRC = cliInterface.initializeInfoList(keyInfoQueue, TRUE);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

          processReturn();

          return NULL;
        }
      
      cliRC = cliInterface.fetchAllRows(keyInfoQueue, query);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

          processReturn();

          return NULL;
        }

      if (keyInfoQueue->numEntries() == 0)
        {
          *CmpCommon::diags() << DgSqlCode(-4400);

          processReturn();

          return NULL;
        }

      ComTdbVirtTableKeyInfo * keyInfoArray = 
        new(STMTHEAP) ComTdbVirtTableKeyInfo[keyColCount];

      ComTdbVirtTableKeyInfo * nonKeyInfoArray = NULL;
      if (nonKeyColCount > 0)
        {
          nonKeyInfoArray = 
            new(STMTHEAP) ComTdbVirtTableKeyInfo[nonKeyColCount];
        }

      keyInfoQueue->position();
      Lng32 jk = 0;
      Lng32 jnk = 0;
      for (Lng32 j = 0; j < keyInfoQueue->numEntries(); j++)
        {
          OutputInfo * vi = (OutputInfo*)keyInfoQueue->getNext(); 
          
          Lng32 nonKeyCol = *(Lng32*)vi->get(4);
          if (nonKeyCol == 0)
            {
              populateKeyInfo(keyInfoArray[jk], vi, TRUE);
              jk++;
            }
          else
            {
              if (nonKeyInfoArray)
                {
                  populateKeyInfo(nonKeyInfoArray[jnk], vi, TRUE);
                  jnk++;
                }
            }
        }

      indexInfoArray[idx].keyInfoArray = keyInfoArray;

      indexInfoArray[idx].nonKeyInfoArray = nonKeyInfoArray;
    } // for

  // get constraint info
  str_sprintf(query, "select O.object_name, C.constraint_type, C.col_count, C.constraint_uid, C.enforced from %s.\"%s\".%s O, %s.\"%s\".%s C where O.catalog_name = '%s' and O.schema_name = '%s' and C.table_uid = %Ld and O.object_uid = C.constraint_uid order by 1",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
              catName.data(), schName.data(), 
              objUID);
              
  Queue * constrInfoQueue = NULL;
  cliRC = cliInterface.fetchAllRows(constrInfoQueue, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return NULL;
    }

  ComTdbVirtTableConstraintInfo * constrInfoArray = NULL;
  if (constrInfoQueue->numEntries() > 0)
    {
      constrInfoArray =
        new(STMTHEAP) ComTdbVirtTableConstraintInfo[constrInfoQueue->numEntries()];
    }

  NAString tableCatName = "\"";
  tableCatName += catName;
  tableCatName += "\"";
  NAString tableSchName = "\"";
  tableSchName += schName;
  tableSchName += "\"";
  NAString tableObjName = "\"";
  tableObjName += objName;
  tableObjName += "\"";

  ComObjectName coTableName(tableCatName, tableSchName, tableObjName);
  extTableName =
    new(STMTHEAP) NAString(coTableName.getExternalName(TRUE));

  constrInfoQueue->position();
  for (int idx = 0; idx < constrInfoQueue->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)constrInfoQueue->getNext(); 
      
      char * constrName = (char*)vi->get(0);
      char * constrType = (char*)vi->get(1);
      Lng32 colCount = *(Lng32*)vi->get(2);
      Int64 constrUID = *(Int64*)vi->get(3);
      char * enforced = (char*)vi->get(4);

      constrInfoArray[idx].baseTableName = (char*)extTableName->data();

      NAString cnNas = "\"";
      cnNas += constrName;
      cnNas += "\"";
      ComObjectName coConstrName(tableCatName, tableSchName, cnNas);
      NAString * extConstrName = 
        new(STMTHEAP) NAString(coConstrName.getExternalName(TRUE));

      constrInfoArray[idx].constrName = (char*)extConstrName->data();
      constrInfoArray[idx].colCount = colCount;

      if (strcmp(constrType, COM_UNIQUE_CONSTRAINT_LIT) == 0)
        constrInfoArray[idx].constrType = 0; // unique_constr
      else if (strcmp(constrType, COM_FOREIGN_KEY_CONSTRAINT_LIT) == 0)
        constrInfoArray[idx].constrType = 1; // ref_constr
     else if (strcmp(constrType, COM_CHECK_CONSTRAINT_LIT) == 0)
        constrInfoArray[idx].constrType = 2; // check_constr
     else if (strcmp(constrType, COM_PRIMARY_KEY_CONSTRAINT_LIT) == 0)
        constrInfoArray[idx].constrType = 3; // pkey_constr

      if (strcmp(enforced, COM_YES_LIT) == 0)
        constrInfoArray[idx].isEnforced = 1;
      else
        constrInfoArray[idx].isEnforced = 0;

      Queue * keyInfoQueue = NULL;
      str_sprintf(query, "select column_name, column_number, keyseq_number, ordering , cast(0 as int not null) from %s.\"%s\".%s where object_uid = %Ld for read committed access order by keyseq_number",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
                  constrUID);
      cliRC = cliInterface.initializeInfoList(keyInfoQueue, TRUE);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

          processReturn();

          return NULL;
        }
      
      cliRC = cliInterface.fetchAllRows(keyInfoQueue, query);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

          processReturn();

          return NULL;
        }
      
      ComTdbVirtTableKeyInfo * keyInfoArray = NULL;
      if (colCount > 0)
        {
          keyInfoArray = 
            new(STMTHEAP) ComTdbVirtTableKeyInfo[colCount];
          
          keyInfoQueue->position();
          Lng32 jk = 0;
          for (Lng32 j = 0; j < keyInfoQueue->numEntries(); j++)
            {
              OutputInfo * vi = (OutputInfo*)keyInfoQueue->getNext(); 
              
              populateKeyInfo(keyInfoArray[jk], vi, TRUE);
              jk++;
            }
        }

      constrInfoArray[idx].keyInfoArray = keyInfoArray;
      constrInfoArray[idx].numRingConstr = 0;
      constrInfoArray[idx].ringConstrArray = NULL;
      constrInfoArray[idx].numRefdConstr = 0;
      constrInfoArray[idx].refdConstrArray = NULL;

      constrInfoArray[idx].checkConstrLen = 0;
      constrInfoArray[idx].checkConstrText = NULL;

      // attach all the referencing constraints
      if ((strcmp(constrType, COM_UNIQUE_CONSTRAINT_LIT) == 0) ||
          (strcmp(constrType, COM_PRIMARY_KEY_CONSTRAINT_LIT) == 0))
        {
          str_sprintf(query, "select trim(O.catalog_name || '.' || '\"' || O.schema_name || '\"' || '.' || '\"' || O.object_name || '\"' ) constr_name, trim(O2.catalog_name || '.' || '\"' || O2.schema_name || '\"' || '.' || '\"' || O2.object_name || '\"' ) table_name from %s.\"%s\".%s U, %s.\"%s\".%s O, %s.\"%s\".%s O2, %s.\"%s\".%s T where  O.object_uid = U.foreign_constraint_uid and O2.object_uid = T.table_uid and T.constraint_uid = U.foreign_constraint_uid and U.unique_constraint_uid = %Ld order by 2, 1",
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_UNIQUE_REF_CONSTR_USAGE,
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
                      constrUID
                      );

          Queue * ringInfoQueue = NULL;
          cliRC = cliInterface.fetchAllRows(ringInfoQueue, query, 0, FALSE, FALSE, TRUE);
          if (cliRC < 0)
            {
              cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
              
              processReturn();
              
              return NULL;
            }

          ComTdbVirtTableRefConstraints * ringInfoArray = NULL;
          if (ringInfoQueue->numEntries() > 0)
            {
              ringInfoArray = 
                new(STMTHEAP) ComTdbVirtTableRefConstraints[ringInfoQueue->numEntries()];
            }
          
          ringInfoQueue->position();
          for (Lng32 i = 0; i < ringInfoQueue->numEntries(); i++)
            {
              OutputInfo * vi = (OutputInfo*)ringInfoQueue->getNext(); 
              
              ringInfoArray[i].constrName = (char*)vi->get(0);
              ringInfoArray[i].baseTableName = (char*)vi->get(1);
            }

          constrInfoArray[idx].numRingConstr = ringInfoQueue->numEntries();
          constrInfoArray[idx].ringConstrArray = ringInfoArray;
        }

      // attach all the referencing constraints
      if (strcmp(constrType, COM_FOREIGN_KEY_CONSTRAINT_LIT) == 0)
        {
          str_sprintf(query, "select trim(O.catalog_name || '.' || '\"' || O.schema_name || '\"' || '.' || '\"' || O.object_name || '\"' ) constr_name, trim(O2.catalog_name || '.' || '\"' || O2.schema_name || '\"' || '.' || '\"' || O2.object_name || '\"' ) table_name from %s.\"%s\".%s R, %s.\"%s\".%s O, %s.\"%s\".%s O2, %s.\"%s\".%s T where  O.object_uid = R.unique_constraint_uid and O2.object_uid = T.table_uid and T.constraint_uid = R.unique_constraint_uid and R.ref_constraint_uid = %Ld order by 2,1",
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_REF_CONSTRAINTS,
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
                      constrUID
                      );

          Queue * refdInfoQueue = NULL;
          cliRC = cliInterface.fetchAllRows(refdInfoQueue, query, 0, FALSE, FALSE, TRUE);
          if (cliRC < 0)
            {
              cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
              
              processReturn();
              
              return NULL;
            }

          ComTdbVirtTableRefConstraints * refdInfoArray = NULL;
          if (refdInfoQueue->numEntries() > 0)
            {
              refdInfoArray = 
                new(STMTHEAP) ComTdbVirtTableRefConstraints[refdInfoQueue->numEntries()];
            }
          
          refdInfoQueue->position();
          for (Lng32 i = 0; i < refdInfoQueue->numEntries(); i++)
            {
              OutputInfo * vi = (OutputInfo*)refdInfoQueue->getNext(); 
              
              refdInfoArray[i].constrName = (char*)vi->get(0);
              refdInfoArray[i].baseTableName = (char*)vi->get(1);
            }

          constrInfoArray[idx].numRefdConstr = refdInfoQueue->numEntries();
          constrInfoArray[idx].refdConstrArray = refdInfoArray;

        }

     if (strcmp(constrType, COM_CHECK_CONSTRAINT_LIT) == 0)
       {
         NAString constrText;
         if (getTextFromMD(&cliInterface, constrUID, COM_CHECK_CONSTR_TEXT, 0,
                           constrText))
           {
              processReturn();
              
              return NULL;
           }

         char * ct = new(STMTHEAP) char[constrText.length()+1];
         memcpy(ct, constrText.data(), constrText.length());
         ct[constrText.length()] = 0;

         constrInfoArray[idx].checkConstrLen = constrText.length();
         constrInfoArray[idx].checkConstrText = ct;
       }
    } // for

  str_sprintf(query, "select check_option, is_updatable, is_insertable from %s.\"%s\".%s where view_uid = %Ld for read committed access ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS,
              objUID);
  
  Queue * viewInfoQueue = NULL;
  cliRC = cliInterface.fetchAllRows(viewInfoQueue, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return NULL;
    }

  ComTdbVirtTableViewInfo * viewInfoArray = NULL;
  if (viewInfoQueue->numEntries() > 0)
    {
      // must have only one entry
      if (viewInfoQueue->numEntries() > 1)
        {
          processReturn();
          
          return NULL;
        }

      viewInfoArray = new(STMTHEAP) ComTdbVirtTableViewInfo[1];

      viewInfoQueue->position();
      
      OutputInfo * vi = (OutputInfo*)viewInfoQueue->getNext(); 
      
      char * checkOption = (char*)vi->get(0);
      Lng32 isUpdatable = *(Lng32*)vi->get(1);
      Lng32 isInsertable = *(Lng32*)vi->get(2);
      
      viewInfoArray[0].viewName = (char*)extTableName->data();
      
       if (NAString(checkOption) != COM_NONE_CHECK_OPTION_LIT)
        {
          viewInfoArray[0].viewCheckText = new(STMTHEAP) char[strlen(checkOption) + 1];
          strcpy(viewInfoArray[0].viewCheckText, checkOption);
        }
      else
        viewInfoArray[0].viewCheckText = NULL;
      viewInfoArray[0].isUpdatable = isUpdatable;
      viewInfoArray[0].isInsertable = isInsertable;

      // get view text from TEXT table
      NAString viewText;
      if (getTextFromMD(&cliInterface, objUID, COM_VIEW_TEXT, 0, viewText))
        {
          processReturn();
          
          return NULL;
        }

      viewInfoArray[0].viewText = new(STMTHEAP) char[viewText.length() + 1];
      strcpy(viewInfoArray[0].viewText, viewText.data());
    }

  ComTdbVirtTableSequenceInfo * seqInfo = NULL;
  if (identityColPos >= 0)
    {
      NAString seqName;
      SequenceGeneratorAttributes::genSequenceName
        (catName, schName, objName, colInfoArray[identityColPos].colName,
         seqName);
      
      NAString extSeqName;
      Int32 objectOwner;
      Int64 seqUID;
      seqInfo = getSeabaseSequenceInfo(catName, schName, seqName,
                                       extSeqName, objectOwner, schemaOwner, seqUID);
    }

  ComTdbVirtTableTableInfo * tableInfo = new(STMTHEAP) ComTdbVirtTableTableInfo[1];
  tableInfo->tableName = extTableName->data();
  tableInfo->createTime = 0;
  tableInfo->redefTime = 0;
  tableInfo->objUID = objUID;
  tableInfo->isAudited = (isAudited ? -1 : 0);
  tableInfo->validDef = 1;
  tableInfo->objOwnerID = objectOwner;
  tableInfo->schemaOwnerID = schemaOwner;
  tableInfo->numSaltPartns = numSaltPartns;
  tableInfo->hbaseCreateOptions = 
    (hbaseCreateOptions->isNull() ? NULL : hbaseCreateOptions->data());
  tableInfo->rowFormat = (alignedFormat ? 1 : 0);

  tableDesc =
    Generator::createVirtualTableDesc
    (
     extTableName->data(), //objName,
     numCols,
     colInfoArray,
     tableKeyInfo->numEntries(), //keyIndex,
     keyInfoArray,
     constrInfoQueue->numEntries(),
     constrInfoArray,
     indexInfoQueue->numEntries(),
     indexInfoArray,
     viewInfoQueue->numEntries(),
     viewInfoArray,
     tableInfo,
     seqInfo);

 // reset the SMD table flag
  tableDesc->body.table_desc.issystemtablecode = 0;

  if ( tableDesc ) {

       // request the default
      ExpHbaseInterface* ehi =CmpSeabaseDDL::allocEHI();
      ByteArrayList* bal = ehi->getRegionInfo(extTableName->data());

      // Set the header.nodetype to either HASH2 or RANGE based on whether
      // the table is salted or not.  
      if (tableIsSalted && CmpCommon::getDefault(HBASE_HASH2_PARTITIONING) == DF_ON) 
        ((table_desc_struct*)tableDesc)->hbase_regionkey_desc = 
          assembleRegionDescs(bal, DESC_HBASE_HASH2_REGION_TYPE);
      else
       if ( CmpCommon::getDefault(HBASE_RANGE_PARTITIONING) == DF_ON ) 
         ((table_desc_struct*)tableDesc)->hbase_regionkey_desc = 
            assembleRegionDescs(bal, DESC_HBASE_RANGE_REGION_TYPE);


      delete bal;
      CmpSeabaseDDL::deallocEHI(ehi);
  }


  if (! tableDesc)
    processReturn();
  
  return tableDesc;
}

desc_struct * CmpSeabaseDDL::getSeabaseTableDesc(const NAString &catName, 
                                                     const NAString &schName, 
                                                     const NAString &objName,
                                                     const ComObjectType objType,
                                                     NABoolean includeInvalidDefs)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  desc_struct *tDesc = NULL;
  if (isSeabaseMD(catName, schName, objName))
    {
      if (! CmpCommon::context()->getTrafMDDescsInfo())
        {
          *CmpCommon::diags() << DgSqlCode(-1428);

          return NULL;
        }

      tDesc = getSeabaseMDTableDesc(catName, schName, objName, objType);
      
      // Could not find this metadata object in the static predefined structs.
      // It could be a metadata view or other objects created in MD schema.
      // Look for it as a regular object.
    }
  else if ((objName == HBASE_HIST_NAME) ||
           (objName == HBASE_HISTINT_NAME))
    {
      NAString tabName = catName;
      tabName += ".";
      tabName += schName;
      tabName += ".";
      tabName += objName;
      if (existsInHbase(tabName))
        {
          tDesc = getSeabaseHistTableDesc(catName, schName, objName);
        }
      return tDesc;
    }
  
  if (! tDesc)
    {
      if (CmpCommon::context()->isUninitializedSeabase())
        {
          if (CmpCommon::context()->uninitializedSeabaseErrNum() == -1398)
            *CmpCommon::diags() << DgSqlCode(CmpCommon::context()->uninitializedSeabaseErrNum())
                                << DgInt0(CmpCommon::context()->hbaseErrNum())
                                << DgString0(CmpCommon::context()->hbaseErrStr());
          else
            *CmpCommon::diags() << DgSqlCode(CmpCommon::context()->uninitializedSeabaseErrNum());
        }
      else
        {
          // use metadata compiler
          NABoolean switched = FALSE;
          CmpContext* currContext = CmpCommon::context();
          if (IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER)
            if (SQL_EXEC_SWITCH_TO_COMPILER_TYPE(
                                     CmpContextInfo::CMPCONTEXT_TYPE_META))
              {
                // failed to switch/create metadata CmpContext
                // continue using current compiler
              }
             else
              switched = TRUE;

          if (sendAllControlsAndFlags(currContext))
            return NULL;

          if (objType == COM_SEQUENCE_GENERATOR_OBJECT)
            tDesc = getSeabaseSequenceDesc(catName, schName, objName);
          else
            tDesc = getSeabaseUserTableDesc(catName, schName, objName, 
                                            objType, includeInvalidDefs);

          // save existing diags info
          ComDiagsArea * tempDiags = ComDiagsArea::allocate(heap_);

          tempDiags->mergeAfter(*CmpCommon::diags());

          restoreAllControlsAndFlags();

          // ignore new (?) and restore old diags
          CmpCommon::diags()->clear();
          CmpCommon::diags()->mergeAfter(*tempDiags);
          tempDiags->clear();
          tempDiags->deAllocate();

          // switch back the internal commpiler, ignore error for now
          if (switched == TRUE)
            SQL_EXEC_SWITCH_BACK_COMPILER();

        }
    }

  return tDesc;
}

//
// Produce a list of desc_struct objects. In each object, the body_struct
// field points at hbase_region_desc. The order of the keyinfo, obtained from
// org.apache.hadoop.hbase.client.HTable.getEndKey(), is preserved.
//
// Allocate space from STMTHEAP, per the call of this function
// in CmpSeabaseDDL::getSeabaseTableDesc() and the
// Generator::createVirtualTableDesc() call make before this one that
// uses STMTPHEAP througout.
//
desc_struct* CmpSeabaseDDL::assembleRegionDescs(ByteArrayList* bal, desc_nodetype format)
{
   if ( !bal )
     return NULL;

   desc_struct *result = NULL;

   Int32 entries = bal->getSize();

   Int32 len = 0;
   char* buf = NULL;

   for (Int32 i=entries-1; i>=0; i-- ) {

      // call JNI interface
      len = bal->getEntrySize(i);
   
   
      if ( len > 0 ) {
   
         buf = new (STMTHEAP) char[len];
         Int32 datalen;
  
         if ( !bal->getEntry(i, buf, len, datalen) || datalen != len ) {
            return NULL;
         }
      }

      desc_struct* wrapper = NULL;
      wrapper = new (STMTHEAP) desc_struct();
      wrapper->header.OSV = 0; // TBD
      wrapper->header.OFV = 0; // TBD

      wrapper->header.nodetype = format;

      wrapper->body.hbase_region_desc.beginKey = NULL;
      wrapper->body.hbase_region_desc.beginKeyLen = 0;
      wrapper->body.hbase_region_desc.endKey = buf;
      wrapper->body.hbase_region_desc.endKeyLen = len;

      wrapper->header.next = result;
      result = wrapper;
   }

   return result;
}

// a wrapper method to getSeabaseRoutineDescInternal so 
// CmpContext context switching can take place. 
// getSeabaseRoutineDescInternal prepares and executes
// several queries on metadata tables
desc_struct *CmpSeabaseDDL::getSeabaseRoutineDesc(const NAString &catName,
                                      const NAString &schName,
                                      const NAString &objName)
{
   desc_struct *result = NULL;
   NABoolean switched = FALSE;
   CmpContext* currContext = CmpCommon::context();

   // use metadata compiler
   if (IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER)
     if (SQL_EXEC_SWITCH_TO_COMPILER_TYPE(
                              CmpContextInfo::CMPCONTEXT_TYPE_META))
       {
          // failed to switch/create metadata CmpContext
          // continue using current compiler 
       }
      else
       switched = TRUE;

   if (sendAllControlsAndFlags(currContext))
     return NULL;

   result = getSeabaseRoutineDescInternal(catName, schName, objName);

   // save existing diags info
   ComDiagsArea * tempDiags = ComDiagsArea::allocate(heap_);
   tempDiags->mergeAfter(*CmpCommon::diags());

   restoreAllControlsAndFlags();

   // ignore new (?) and restore old diags
   CmpCommon::diags()->clear();
   CmpCommon::diags()->mergeAfter(*tempDiags);
   tempDiags->clear();
   tempDiags->deAllocate();

   // switch back the internal compiler, ignore error for now
   if (switched == TRUE)
     SQL_EXEC_SWITCH_BACK_COMPILER();

   return result;
}


desc_struct *CmpSeabaseDDL::getSeabaseRoutineDescInternal(const NAString &catName,
                                      const NAString &schName,
                                      const NAString &objName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  desc_struct *result;
  char query[4000];
  char buf[4000];

  ExeCliInterface cliInterface(STMTHEAP);

  Int64 objectUid = getObjectUID(&cliInterface, 
                                 catName.data(), schName.data(),
                                 objName.data(),
                                 (char *)COM_UDR_NAME_LIT);
  if (objectUid == -1)
     return NULL;

  str_sprintf(buf, "select udr_type, language_type, deterministic_bool,"
  " sql_access, call_on_null, isolate_bool, param_style,"
  " transaction_attributes, max_results, state_area_size, external_name,"
  " parallelism, user_version, external_security, execution_mode,"
  " library_filename, version, signature from %s.\"%s\".%s a, %s.\"%s\".%s b"
  " where a.udr_uid = %Ld and a.library_uid = b.library_uid "
  " for read committed access",
       getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_ROUTINES,
       getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_LIBRARIES, objectUid);


  cliRC = cliInterface.fetchRowsPrologue(buf, TRUE/*no exec*/);
  if (cliRC < 0)
  {
     cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
     return NULL;
  }

  cliRC = cliInterface.clearExecFetchClose(NULL, 0);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
     return NULL;
  }
  if (cliRC == 100) // did not find the row
  {
     *CmpCommon::diags() << DgSqlCode(-1389)
                        << DgString0(objName);
     return NULL;
  }

  char * ptr = NULL;
  Lng32 len = 0;

  ComTdbVirtTableRoutineInfo *routineInfo = new (STMTHEAP) ComTdbVirtTableRoutineInfo();

  routineInfo->routine_name = objName.data();
  cliInterface.getPtrAndLen(1, ptr, len);
  str_cpy_all(routineInfo->UDR_type, ptr, len);
  routineInfo->UDR_type[len] =  '\0';
  cliInterface.getPtrAndLen(2, ptr, len);
  str_cpy_all(routineInfo->language_type, ptr, len);
  routineInfo->language_type[len] = '\0';
  cliInterface.getPtrAndLen(3, ptr, len);
  if (*ptr == 'Y')
     routineInfo->deterministic = 1;
  else
     routineInfo->deterministic = 0;
  cliInterface.getPtrAndLen(4, ptr, len);
  str_cpy_all(routineInfo->sql_access, ptr, len);
  routineInfo->sql_access[len] = '\0';
  cliInterface.getPtrAndLen(5, ptr, len);
  if (*ptr == 'Y')
     routineInfo->call_on_null = 1;
  else
     routineInfo->call_on_null = 0;
  cliInterface.getPtrAndLen(6, ptr, len);
  if (*ptr == 'Y')
     routineInfo->isolate = 1;
  else
     routineInfo->isolate = 0;
  cliInterface.getPtrAndLen(7, ptr, len);
  str_cpy_all(routineInfo->param_style, ptr, len);
  routineInfo->param_style[len] = '\0';
  cliInterface.getPtrAndLen(8, ptr, len);
  str_cpy_all(routineInfo->transaction_attributes, ptr, len);
  routineInfo->transaction_attributes[len] = '\0';
  cliInterface.getPtrAndLen(9, ptr, len);
  routineInfo->max_results = *(Int32 *)ptr;
  cliInterface.getPtrAndLen(10, ptr, len);
  routineInfo->state_area_size = *(Int32 *)ptr;
  cliInterface.getPtrAndLen(11, ptr, len);
  routineInfo->external_name = new (STMTHEAP) char[len+1];    
  str_cpy_and_null((char *)routineInfo->external_name, ptr, len, '\0', ' ', TRUE);
  cliInterface.getPtrAndLen(12, ptr, len);
  str_cpy_all(routineInfo->parallelism, ptr, len);
  routineInfo->parallelism[len] = '\0';
  cliInterface.getPtrAndLen(13, ptr, len);
  str_cpy_all(routineInfo->user_version, ptr, len);
  routineInfo->user_version[len] = '\0';
  cliInterface.getPtrAndLen(14, ptr, len);
  str_cpy_all(routineInfo->external_security, ptr, len);
  routineInfo->external_security[len] = '\0';
  cliInterface.getPtrAndLen(15, ptr, len);
  str_cpy_all(routineInfo->execution_mode, ptr, len);
  routineInfo->execution_mode[len] = '\0';
  cliInterface.getPtrAndLen(16, ptr, len);
  routineInfo->library_filename = new (STMTHEAP) char[len+1];    
  str_cpy_and_null((char *)routineInfo->library_filename, ptr, len, '\0', ' ', TRUE);
  cliInterface.getPtrAndLen(17, ptr, len);
  routineInfo->library_version = *(Int32 *)ptr;
  cliInterface.getPtrAndLen(18, ptr, len);
  routineInfo->signature = new (STMTHEAP) char[len+1];    
  str_cpy_and_null((char *)routineInfo->signature, ptr, len, '\0', ' ', TRUE);
  ComTdbVirtTableColumnInfo *paramsArray;
  Lng32 numParams;
  char direction[50];
  str_sprintf(direction, "'%s', '%s', '%s'", 
                      COM_INPUT_PARAM_LIT, COM_OUTPUT_PARAM_LIT,
                      COM_INOUT_PARAM_LIT);
  // Params
  if (getSeabaseColumnInfo(&cliInterface,
                           objectUid,
                           catName, schName, objName,
                           (char *)direction,
                           NULL,
                           NULL,
                           &numParams,
                           &paramsArray) < 0)
    {
      processReturn();
      return NULL;
    } 
  
  desc_struct *routine_desc = NULL;
  routine_desc = Generator::createVirtualRoutineDesc(
            objName.data(),
            routineInfo,
            numParams,
            paramsArray);

  if (routine_desc == NULL)
     processReturn();
  return routine_desc;
}
