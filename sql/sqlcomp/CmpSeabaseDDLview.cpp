/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2015 Hewlett-Packard Development Company, L.P.
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
 * File:         CmpSeabaseDDLview.cpp
 * Description:  Implements ddl views for SQL/seabase tables.
 *
 *
 * Created:     6/30/2013
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS	// must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "ComObjectName.h"
#include "ComUser.h"

#include "StmtDDLCreateView.h"
#include "StmtDDLDropView.h"
#include "ElemDDLColDefArray.h"
#include "ElemDDLColRefArray.h"

#include "SchemaDB.h"
#include "CmpSeabaseDDLincludes.h"

#include "ExpHbaseInterface.h"

#include "ExExeUtilCli.h"
#include "Generator.h"
#include "desc.h"

// for privilege checking
#include "PrivMgrCommands.h"
#include "PrivMgrDefs.h"
#include "PrivMgrPrivileges.h"
#include <bitset>

#include "ComCextdecs.h"

short CmpSeabaseDDL::buildViewText(StmtDDLCreateView * createViewParseNode,
				   NAString &viewText) 
{
  const ParNameLocList &nameLocList = createViewParseNode->getNameLocList();
  const char *pInputStr = nameLocList.getInputStringPtr();
  
  StringPos inputStrPos = createViewParseNode->getStartPosition();
  
  for (CollIndex i = 0; i < nameLocList.entries(); i++)
    {
      const ParNameLoc &nameLoc = nameLocList[i];
      const NAString &nameExpanded = nameLoc.getExpandedName(FALSE/*no assert*/);
      size_t nameAsIs = 0;
      size_t nameLenInBytes = 0;
      size_t nameLenInNAWchars = 0;
      
      //
      // When the character set of the input string is a variable-length/width
      // multi-byte characters set, the value returned by getNameLength()
      // may not be numerically equal to the number of bytes in the original
      // input string that we need to skip.  So, we get the character
      // conversion routines to tell us how many bytes we need to skip.
      //
      CMPASSERT(nameLocList.getInputStringCharSet() EQU CharInfo::UTF8);
      enum cnv_charset eCnvCS = convertCharsetEnum(nameLocList.getInputStringCharSet());
      
      const char *str_to_test = (const char *) &pInputStr[nameLoc.getNamePosition()];
      const Int32 max_bytes2cnv = createViewParseNode->getEndPosition()
	- nameLoc.getNamePosition() + 1;
      const char *tmp_out_bufr = new (STMTHEAP) char[max_bytes2cnv * 4 + 10 /* Ensure big enough! */ ];
      char * p1stUnstranslatedChar = NULL;
      UInt32 iTransCharCountInChars = 0;
      Int32 cnvErrStatus = LocaleToUTF16(
					 cnv_version1          // in  - const enum cnv_version version
					 , str_to_test           // in  - const char *in_bufr
					 , max_bytes2cnv         // in  - const int in_len
					 , tmp_out_bufr          // out - const char *out_bufr
					 , max_bytes2cnv * 4 + 1 // in  - const int out_len
					 , eCnvCS                // in  - enum cnv_charset charset
					 , p1stUnstranslatedChar // out - char * & first_untranslated_char
					 , NULL                  // out - unsigned int *output_data_len_p
					 , 0                     // in  - const int cnv_flags
					 , (Int32)TRUE           // in  - const int addNullAtEnd_flag
					 , &iTransCharCountInChars  // out - unsigned int * translated_char_cnt_p
					 , nameLoc.getNameLength()     // in  - unsigned int max_NAWchars_to_convert
					 );
      // NOTE: No errors should be possible -- string has been converted before.
      
      NADELETEBASIC (tmp_out_bufr, STMTHEAP);
      nameLenInBytes = p1stUnstranslatedChar - str_to_test;
      
      // If name not expanded, then use the original name as is
      if (nameExpanded.isNull())
	nameAsIs = nameLenInBytes;
      
      // Copy from (last position in) input string up to current name
      viewText += NAString(&pInputStr[inputStrPos],
			   nameLoc.getNamePosition() - inputStrPos +
			   nameAsIs);
      
      if (NOT nameAsIs) // original name to be replaced with expanded
	{
	  size_t namePos = nameLoc.getNamePosition();
	  size_t nameLen = nameLoc.getNameLength();
	  
	  if ( ( /* case #1 */ pInputStr[namePos] EQU '*' OR
		 /* case #2 */ pInputStr[namePos] EQU '"' )
	       AND nameExpanded.data()[0] NEQ '"'
	       AND namePos > 1
	       AND ( pInputStr[namePos - 1] EQU '_' OR
		     isAlNumIsoMapCS((unsigned char)pInputStr[namePos - 1]) )
	       )
	    {
	      // insert a blank separator to avoid syntax error
	      // WITHOUT FIX
	      // ex#1: CREATE VIEW C.S.V AS SELECTC.S.T.COL FROM C.S.T
	      // ex#2: CREATE VIEW C.S.V AS SELECTC.S.T.COL FROM C.S.T
	      viewText += " "; // the FIX
	      // WITH FIX
	      // ex#1: CREATE VIEW C.S.V AS SELECT C.S.T.COL FROM C.S.T
	      // ex#2: CREATE VIEW C.S.V AS SELECT C.S.T.COL FROM C.S.T
	    }
	  
	  // Add the expanded (fully qualified) name (if exists)
	  viewText += nameExpanded;
	  
	  if ( ( /* case #3 */ ( pInputStr[namePos] EQU '*' AND nameLen EQU 1 ) OR
		 /* case #4 */ pInputStr[namePos + nameLen - 1] EQU '"' )
	       AND nameExpanded.data()[nameExpanded.length() - 1] NEQ '"'
	       AND pInputStr[namePos + nameLen] NEQ '\0'
	       AND ( pInputStr[namePos + nameLen] EQU '_' OR
		     isAlNumIsoMapCS((unsigned char)pInputStr[namePos + nameLen]) )
	       )
	    {
	      // insert a blank separator to avoid syntax error
	      // WITHOUT FIX
	      // ex: CREATE VIEW C.S.V AS SELECT C.S.T.COLFROM C.S.T
	      viewText += " "; // the FIX
	      // WITH FIX
	      // ex: CREATE VIEW C.S.V AS SELECT C.S.T.COL FROM C.S.T
	    }
	} // if (NOT nameAsIs)
      
      // Advance input pointer beyond original name in input string
      inputStrPos = nameLoc.getNamePosition() + nameLenInBytes /* same as nameLenInNAWchars */;
      
    } // for
  
  if (createViewParseNode->getEndPosition() >= inputStrPos)
    {
      viewText += NAString(&pInputStr[inputStrPos],
			   createViewParseNode->getEndPosition()
			   + 1 - inputStrPos);
    }
  else
    CMPASSERT(createViewParseNode->getEndPosition() == inputStrPos-1);
  
  PrettifySqlText(viewText,
		  CharType::getCharSetAsPrefix(SqlParser_NATIONAL_CHARSET));

  return 0;
} // CmpSeabaseDDL::buildViewText()

short CmpSeabaseDDL::buildViewColInfo(StmtDDLCreateView * createViewParseNode,
				       ElemDDLColDefArray *colDefArray)
{
  // Builds the list of ElemDDLColDef parse nodes from the list of
  // NAType parse nodes derived from the query expression parse sub-tree
  // and the list of ElemDDLColViewDef parse nodes from the parse tree.
  // This extra step is needed to invoke (reuse) global func CatBuildColumnList.

  CMPASSERT(createViewParseNode->getQueryExpression()->
             getOperatorType() EQU REL_ROOT);

  RelRoot * pQueryExpr = (RelRoot *)createViewParseNode->getQueryExpression();

  const ValueIdList &valIdList = pQueryExpr->compExpr();        // select-list
  CMPASSERT(valIdList.entries() > 0);

  CollIndex numOfCols(createViewParseNode->getViewColDefArray().entries());
  if (numOfCols NEQ valIdList.entries())
    {
      *CmpCommon::diags() << DgSqlCode(-1108) //CAT_NUM_OF_VIEW_COLS_NOT_MATCHED
			  << DgInt0(numOfCols)
			  << DgInt1(valIdList.entries());
      return -1;
  }

  const ElemDDLColViewDefArray &viewColDefArray = createViewParseNode->
    getViewColDefArray();
  for (CollIndex i = 0; i < numOfCols; i++)
    {
      // ANSI 11.19 SR8
      if (viewColDefArray[i]->getColumnName().isNull())
	{
	  *CmpCommon::diags() << DgSqlCode(-1099) //CAT_VIEW_COLUMN_UNNAMED
			      << DgInt0(i+1);
	  return -1;
	}
      
      colDefArray->insert(new (STMTHEAP) ElemDDLColDef
			  ( viewColDefArray[i]->getColumnName()
			    , (NAType *)&valIdList[i].getType()
			    , NULL    // default value (n/a for view def)
			    , NULL    // col attr list (not needed)
			    , STMTHEAP));
      
      if (viewColDefArray[i]->isHeadingSpecified())
	{
	  (*colDefArray)[i]->setIsHeadingSpecified(TRUE);
	  (*colDefArray)[i]->setHeading(viewColDefArray[i]->getHeading());
	}
    }
  
  return 0;
}

const char *
CmpSeabaseDDL::computeCheckOption(StmtDDLCreateView * createViewParseNode) 
{
  if (createViewParseNode->isWithCheckOptionSpecified())
    {
      switch (createViewParseNode->getCheckOptionLevel())
	{
	case COM_CASCADED_LEVEL :
	  return COM_CASCADE_CHECK_OPTION_LIT;
	  break;
	case COM_LOCAL_LEVEL:
	  return COM_LOCAL_CHECK_OPTION_LIT;
	  break;
	case COM_UNKNOWN_LEVEL :
	  return COM_UNKNOWN_CHECK_OPTION_LIT;
	  break;
	default:
	  return COM_NONE_CHECK_OPTION_LIT;
	  break;
	} // switch
    }
  else
    {
      return COM_NONE_CHECK_OPTION_LIT;
    }
  
  return NULL;

} // CmpSeabaseDDL::computeCheckOption()

short CmpSeabaseDDL::updateViewUsage(StmtDDLCreateView * createViewParseNode,
				   Int64 viewUID,
				   ExeCliInterface * cliInterface)
{
  const ParViewUsages &vu = createViewParseNode->getViewUsages();
  const ParTableUsageList &vtul = vu.getViewTableUsageList();
  
  for (CollIndex i = 0; i < vtul.entries(); i++)
    {
      ComObjectName usedObjName(vtul[i].getQualifiedNameObj()
				.getQualifiedNameAsAnsiString(),
				vtul[i].getAnsiNameSpace());
      
      const NAString catalogNamePart = usedObjName.getCatalogNamePartAsAnsiString();
      const NAString schemaNamePart = usedObjName.getSchemaNamePartAsAnsiString(TRUE);
      const NAString objectNamePart = usedObjName.getObjectNamePartAsAnsiString(TRUE);
      const NAString extUsedObjName = usedObjName.getExternalName(TRUE);
      
      char objType[10];
      Int64 usedObjUID = getObjectUID(cliInterface,
				      catalogNamePart.data(), schemaNamePart.data(), 
				      objectNamePart.data(),
				      NULL,
                                      NULL,
				      objType);
      if (usedObjUID < 0)
	{
	  return -1;
	}

      char query[1000];
      str_sprintf(query, "upsert into %s.\"%s\".%s values (%Ld, %Ld, '%s', 0 )",
		  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS_USAGE,
		  viewUID,
		  usedObjUID,
		  objType);
      Lng32 cliRC = cliInterface->executeImmediate(query);
      

      if (cliRC < 0)
	{
	  cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

	  return -1;
	}
      
    } // for

  // Views can also reference functions.  Add the list of functions
  // referenced to the VIEWS_USAGE table.
  const LIST(OptUDFInfo *) & uul = createViewParseNode->getUDFList();
  for (CollIndex u = 0; u < uul.entries(); u++)
    {

      char query[1000];
      str_sprintf(query, "upsert into %s.\"%s\".%s values (%Ld, %Ld, '%s', 0 )",
		  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS_USAGE,
		  viewUID,
		  uul[u]->getUDFUID(),
		  COM_USER_DEFINED_ROUTINE_OBJECT_LIT);
      Lng32 cliRC = cliInterface->executeImmediate(query);
      

      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

          return -1;
        }
      
    } // for

  return 0;
} 

// ****************************************************************************
// method: gatherViewPrivileges
//
// For each referenced object (table or view) directly associated with the new
// view, combine privilege and grantable bitmaps together.  The list of 
// privileges gathered will be assigned as default privileges as the privilege
// owner values.
//
// TBD:  when column privileges are added, need to check only the affected
//       columns
//
// Parameters:
//    createViewNode - for list of objects and isUpdatable/isInsertable flags
//    cliInterface - used to get UID of referenced object
//    privilegeBitmap - returns privileges this user has on the view
//    grantableBitmap - returns privileges this user can grant
//
// returns:
//    0 - successful
//   -1 - user does not have the privilege
// ****************************************************************************
short CmpSeabaseDDL::gatherViewPrivileges (const StmtDDLCreateView * createViewNode,
				           ExeCliInterface * cliInterface,
                                           PrivMgrBitmap &privilegesBitmap,
                                           PrivMgrBitmap &grantableBitmap)
{
  // set all bits to true initially, we will be ANDing with privileges
  // from all referenced objects 
  // default table and view privileges are the same, set up default values
  PrivMgr::setTablePrivs(privilegesBitmap);
  PrivMgr::setTablePrivs(grantableBitmap);

  if (!isAuthorizationEnabled())
    return 0;

  const ParViewUsages &vu = createViewNode->getViewUsages();
  const ParTableUsageList &vtul = vu.getViewTableUsageList();

  // If DB__ROOT, no need to gather privileges
  if (!ComUser::isRootUserID())
    {
      // generate the lists of privileges and grantable privileges
      // a side effect is to return an error if basic privileges are not granted
      for (CollIndex i = 0; i < vtul.entries(); i++)
        {
          ComObjectName usedObjName(vtul[i].getQualifiedNameObj()
                                    .getQualifiedNameAsAnsiString(),
                                    vtul[i].getAnsiNameSpace());

          const NAString catalogNamePart = usedObjName.getCatalogNamePartAsAnsiString();
          const NAString schemaNamePart = usedObjName.getSchemaNamePartAsAnsiString(TRUE);
          const NAString objectNamePart = usedObjName.getObjectNamePartAsAnsiString(TRUE);
          const NAString extUsedObjName = usedObjName.getExternalName(TRUE);
          CorrName cn(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);

          // Grab privileges from the NATable structure
          BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
          NATable *naTable = bindWA.getNATable(cn);
          if (naTable == NULL)
            {
              SEABASEDDL_INTERNAL_ERROR("Bad NATable pointer in gather view privileges");
              return -1; 
            }

          PrivMgrUserPrivs *privs = naTable->getPrivInfo();
          if (privs == NULL) 
            {         
              *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
               return -1;
            }

          // Requester must have at least select privilege
          if ( !privs->hasSelectPriv() )
            {
               *CmpCommon::diags() << DgSqlCode( -4481 )
                                   << DgString0( "SELECT" )
                                   << DgString1( extUsedObjName.data());
 
                return -1;
            }

          // Summarize privileges
          privilegesBitmap &= privs->getObjectBitmap();
          grantableBitmap &= privs->getGrantableBitmap();
        }
    }

  // If view is not updatable or insertable, turn off privs in bitmaps
  if (!createViewNode->getIsUpdatable())
    {
      privilegesBitmap.set(UPDATE_PRIV,false);
      grantableBitmap.set(UPDATE_PRIV, false);
      privilegesBitmap.set(DELETE_PRIV,false);
      grantableBitmap.set(DELETE_PRIV, false);
    }

  if (!createViewNode->getIsInsertable())
    {
      privilegesBitmap.set(INSERT_PRIV,false);
      grantableBitmap.set(INSERT_PRIV, false);
    }

  return 0;
}

// ****************************************************************************
// method: getListOfReferencedTables
//
// Returns a list of all tables that are being referenced by the passed in 
// view UID
//
// Parameters:
//    cliInterface - used to get the list of object usages
//    objectUID - the UID being processed
//    tableList - a list of objectRefdByMe structures describing each usage
//
// returns:
//    0 - successful
//   -1 - unexpected error occurred
// ****************************************************************************
short CmpSeabaseDDL::getListOfReferencedTables( 
   ExeCliInterface * cliInterface,
   const Int64 objectUID,
   NAList<objectRefdByMe> &tablesList )
{
  Lng32 retcode = 0;

  NAList <objectRefdByMe> tempRefdList;
  retcode = getListOfDirectlyReferencedObjects (cliInterface, objectUID, tempRefdList);
  
  // If unexpected error - return
  if (retcode < 0)
    {
      if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
        SEABASEDDL_INTERNAL_ERROR("getting list of referenced tables");
      return -1;
    }
   

  // For each view in the list, call getReferencedTables recursively
  for (CollIndex i = 0; i < tempRefdList.entries(); i++)
    {
      objectRefdByMe objectRefd = tempRefdList[i];

      // views should only be referencing tables, other views, or functions
      CMPASSERT(objectRefd.objectType == COM_BASE_TABLE_OBJECT_LIT ||
                objectRefd.objectType == COM_USER_DEFINED_ROUTINE_OBJECT_LIT ||
                objectRefd.objectType == COM_SEQUENCE_GENERATOR_OBJECT_LIT ||
                objectRefd.objectType == COM_VIEW_OBJECT_LIT);

      // found a table, add to list
      if (objectRefd.objectType == COM_BASE_TABLE_OBJECT_LIT)
        {  
          // First make sure it has not already been added to the list
          NABoolean foundEntry = FALSE;
          for (CollIndex j = 0; j < tablesList.entries(); j++)
            {
               if (tablesList[j].objectUID == objectRefd.objectUID)
                 foundEntry = TRUE;
            }
        if (!foundEntry)             
          tablesList.insert(objectRefd);  
      } 

      // found a view, get objects associated with the view
      if (objectRefd.objectType == COM_VIEW_OBJECT_LIT)
        getListOfReferencedTables(cliInterface, objectRefd.objectUID, tablesList);
    }

  return 0;
}
  
// ****************************************************************************
// method: getListOfDirectlyReferencedObjects
//
// Returns a list of objects that are being directly referenced by the passed 
// in objectUID
//
// Parameters:
//    cliInterface - used to get the list of object usages
//    objectUID - the UID being processed
//    objectList - a list of objectRefdByMe structures describing each usage
//
// returns:
//    0 - successful
//   -1 - unexpected error occurred
// ****************************************************************************
short CmpSeabaseDDL::getListOfDirectlyReferencedObjects (
  ExeCliInterface *cliInterface,
  const Int64 objectUID,
  NAList<objectRefdByMe> &objectsList)
{
  // Select all the rows from views_usage associated with the passed in
  // objectUID
  Lng32 cliRC = 0;
  char buf[4000];
  str_sprintf(buf, "select object_type, object_uid, catalog_name," 
                   "schema_name, object_name from %s.\"%s\".%s T, %s.\"%s\".%s VU " 
                   "where VU.using_view_uid = %Ld "
                   "and T.object_uid = VU.used_object_uid",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS_USAGE,
              objectUID);

  Queue * usingObjectsQueue = NULL;
  cliRC = cliInterface->fetchAllRows(usingObjectsQueue, buf, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  // set up an objectRefdByMe struct for each returned row
  usingObjectsQueue->position();
  for (int idx = 0; idx < usingObjectsQueue->numEntries(); idx++)
    {
      OutputInfo * oi = (OutputInfo*)usingObjectsQueue->getNext();
      objectRefdByMe objectInfo;
      objectInfo.objectType = NAString(oi->get(0));
      objectInfo.objectUID = *(Int64*)oi->get(1);
      objectInfo.catalogName = NAString(oi->get(2));
      objectInfo.schemaName = NAString(oi->get(3));
      objectInfo.objectName = NAString(oi->get(4));
      objectsList.insert(objectInfo);
    }

  return 0;
}

void CmpSeabaseDDL::createSeabaseView(
				      StmtDDLCreateView * createViewNode,
				      NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ComObjectName viewName(createViewNode->getViewName());
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  viewName.applyDefaults(currCatAnsiName, currSchAnsiName);
  const NAString catalogNamePart = viewName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = viewName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = viewName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extViewName = viewName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;
  
  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  Int32 objectOwnerID = SUPER_USER;
  Int32 schemaOwnerID = SUPER_USER;
  ComSchemaClass schemaClass;

  retcode = verifyDDLCreateOperationAuthorized(&cliInterface,
                                               SQLOperation::CREATE_VIEW,
                                               catalogNamePart,
                                               schemaNamePart,
                                               schemaClass,
                                               objectOwnerID,
                                               schemaOwnerID);
  if (retcode != 0)
  {
     handleDDLCreateAuthorizationError(retcode,catalogNamePart,schemaNamePart);
     return;
  }
  
  ExpHbaseInterface * ehi = NULL;

  ehi = allocEHI();
  if (ehi == NULL)
    {
     processReturn();

     return;
    }

  if ((isSeabaseReservedSchema(viewName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
     {
      *CmpCommon::diags() << DgSqlCode(-1118)
			  << DgTableName(extViewName);
      deallocEHI(ehi); 
      return;
    }

  //if metadata views are being created and seabase is uninitialized, then this
  //indicates that these views are being created during 'initialize trafodion'
  //and this compiler contains stale version.
  //Reload version info.
  //
  if ((isSeabaseMD(viewName)) &&
      (CmpCommon::context()->isUninitializedSeabase()))
    {
      CmpCommon::context()->setIsUninitializedSeabase(FALSE);
      CmpCommon::context()->uninitializedSeabaseErrNum() = 0;
    }

  retcode = existsInSeabaseMDTable(&cliInterface, 
				   catalogNamePart, schemaNamePart, objectNamePart, 
				   COM_UNKNOWN_OBJECT, FALSE, FALSE);
  if (retcode < 0)
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  if (retcode == 1) // already exists
    {
      if (NOT ((createViewNode->isCreateOrReplaceViewCascade())|| 
	       (createViewNode->isCreateOrReplaceView())))
	{
	  *CmpCommon::diags() << DgSqlCode(-1390)
			      << DgString0(extViewName);
	  deallocEHI(ehi); 
	  
	  processReturn();
	  
	  return;
	}
    }

  char * query = NULL;
  int64_t objectUID = -1;
  std::vector<ObjectPrivsRow> viewPrivsRows;
  bool replacingView = false;
  
  if ((retcode == 1) && // exists
      ((createViewNode->isCreateOrReplaceViewCascade())|| 
       (createViewNode->isCreateOrReplaceView())))
    {
      // Replace view. Drop this view and recreate it.
      
      Int32 objectOwnerID = 0;
      Int32 schemaOwnerID = 0;
      Int64 objUID = getObjectUIDandOwners(&cliInterface,
    			                   catalogNamePart.data(), schemaNamePart.data(), 
    			                   objectNamePart.data(),
    			                   COM_VIEW_OBJECT,
                                           objectOwnerID,schemaOwnerID);

      if (objUID < 0 || objectOwnerID == 0)
        {
          if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
            SEABASEDDL_INTERNAL_ERROR("getting object UID and owner for create or replace view");

          deallocEHI(ehi); 

          processReturn();

          return;
        }

      if (isAuthorizationEnabled())
      {
         // Verify user can perform operation
         if (!isDDLOperationAuthorized(SQLOperation::ALTER_VIEW,objectOwnerID,schemaOwnerID))
         {
            *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
            deallocEHI(ehi);
            processReturn ();
            return;
         }
      
         // Initiate the privilege manager interface class
         NAString privMgrMDLoc;
         CONCAT_CATSCH(privMgrMDLoc,getSystemCatalog(),SEABASE_PRIVMGR_SCHEMA);
         PrivMgrCommands privInterface(std::string(privMgrMDLoc.data()), 
                                       CmpCommon::diags());
      
         PrivStatus privStatus = privInterface.getPrivRowsForObject(objUID,viewPrivsRows);
         if (privStatus != PrivStatus::STATUS_GOOD)
         {
            SEABASEDDL_INTERNAL_ERROR("Unable to retrieve privileges for replaced view");
            deallocEHI(ehi); 
            processReturn();
            
            return;
         }
         
      }
      
      if (dropOneTableorView(cliInterface,extViewName.data(),COM_VIEW_OBJECT,false))
      
        {
          deallocEHI(ehi); 
          processReturn();
          
          return;
        }
      replacingView = true;
    }

  // Gather the object and grantable privileges that the view creator has.
  // This code also verifies that the current user has the necessary
  // privileges to create the view.
  PrivMgrBitmap privilegesBitmap;
  PrivMgrBitmap grantableBitmap;
  privilegesBitmap.set();
  grantableBitmap.set();
  if (gatherViewPrivileges(createViewNode, 
                           &cliInterface, 
                           privilegesBitmap, 
                           grantableBitmap))
    {
      processReturn();

      deallocEHI(ehi); 
	  
      return;
    }

  NAString viewText(STMTHEAP);
  buildViewText(createViewNode, viewText);

  NAString newViewText(STMTHEAP);
  for (Lng32 i = 0; i < viewText.length(); i++)
    {
      if (viewText.data()[i] == '\'')
	newViewText += "''";
      else
	newViewText += viewText.data()[i];
    }

  ElemDDLColDefArray colDefArray(STMTHEAP);
  if (buildViewColInfo(createViewNode, &colDefArray))
    {
      deallocEHI(ehi); 
      processReturn();

      return;
    }

  Lng32 numCols = colDefArray.entries();
  ComTdbVirtTableColumnInfo * colInfoArray = 
    new(STMTHEAP) ComTdbVirtTableColumnInfo[numCols];

  if (buildColInfoArray(COM_VIEW_OBJECT, &colDefArray, colInfoArray, FALSE, 0, FALSE))
    {
      deallocEHI(ehi); 
      processReturn();
      
      return;
    }

  Int64 objUID = -1;
  if (updateSeabaseMDTable(&cliInterface, 
			   catalogNamePart, schemaNamePart, objectNamePart,
			   COM_VIEW_OBJECT,
			   "N",
			   NULL,
			   numCols,
			   colInfoArray,	       
			   0, NULL,
			   0, NULL,
                           objectOwnerID,
                           schemaOwnerID,
                           objUID))
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }

    if (objUID < 0)
      {
        deallocEHI(ehi);
        processReturn();
        return;
      }

  // grant privileges for view
  if (isAuthorizationEnabled())
    {
      char authName[MAX_AUTHNAME_LEN+1];
      Int32 lActualLen = 0;
      Int16 status = ComUser::getAuthNameFromAuthID( (Int32) objectOwnerID
                                                   , (char *)&authName
                                                   , MAX_AUTHNAME_LEN
                                                   , lActualLen );
      if (status != FEOK)
        {
          *CmpCommon::diags() << DgSqlCode(-20235)
                              << DgInt0(status)
                              << DgInt1(objectOwnerID);

          deallocEHI(ehi);

          processReturn();

          return;
       }

      // Initiate the privilege manager interface class
      NAString privMgrMDLoc;
      CONCAT_CATSCH(privMgrMDLoc, getSystemCatalog(), SEABASE_PRIVMGR_SCHEMA);
      PrivMgrCommands privInterface(std::string(privMgrMDLoc.data()), 
                                    CmpCommon::diags());

      retcode = privInterface.grantObjectPrivilege 
       (objUID, std::string(extViewName.data()), COM_VIEW_OBJECT, 
        objectOwnerID, std::string(authName), 
        privilegesBitmap, grantableBitmap);
      if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
        {
          deallocEHI(ehi);

          processReturn();

          return;
        }
      if (replacingView)
      {
         PrivStatus privStatus = privInterface.insertPrivRowsForObject(objUID,viewPrivsRows);
         
         if (privStatus != PrivStatus::STATUS_GOOD)
         {
            SEABASEDDL_INTERNAL_ERROR("Unable to restore privileges for replaced view");
            deallocEHI(ehi); 
            processReturn();
            
            return;
         }
      }  
    }


  query = new(STMTHEAP) char[newViewText.length() + 1000];
  str_sprintf(query, "upsert into %s.\"%s\".%s values (%Ld, '%s', %d, %d, 0)",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS,
	      objUID,
	      computeCheckOption(createViewNode),
	      (createViewNode->getIsUpdatable() ? 1 : 0),
	      (createViewNode->getIsInsertable() ? 1 : 0));
  
  cliRC = cliInterface.executeImmediate(query);

  NADELETEBASIC(query, STMTHEAP);
  if (cliRC < 0)
    {
      if (cliRC == -8402)
        // string overflow, view text does not fit into metadata table
        *CmpCommon::diags() << DgSqlCode(-1198);
      else
        cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      deallocEHI(ehi); 
      processReturn();

      return;
    }

  if (updateTextTable(&cliInterface, objUID, COM_VIEW_TEXT, 0, newViewText))
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  if (updateViewUsage(createViewNode, objUID, &cliInterface))
    {
      deallocEHI(ehi); 
      processReturn();
     
      return;
    }

  if (updateObjectValidDef(&cliInterface, 
			   catalogNamePart, schemaNamePart, objectNamePart,
			   COM_VIEW_OBJECT_LIT,
			   "Y"))
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  CorrName cn(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn,
    NATableDB::REMOVE_MINE_ONLY, COM_VIEW_OBJECT);

  deallocEHI(ehi); 
  processReturn();

  return;
}

void CmpSeabaseDDL::dropSeabaseView(
				    StmtDDLDropView * dropViewNode,
				    NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = dropViewNode->getViewName();

  ComObjectName viewName(tabName);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  viewName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = viewName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = viewName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = viewName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extViewName = viewName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    return;

  if ((isSeabaseReservedSchema(viewName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-1119)
			  << DgTableName(extViewName);
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  retcode = existsInSeabaseMDTable(&cliInterface, 
				   catalogNamePart, schemaNamePart, objectNamePart,
				   COM_VIEW_OBJECT, TRUE, FALSE);
  if (retcode < 0)
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  if (retcode == 0) // does not exist
    {
      *CmpCommon::diags() << DgSqlCode(-1389)
			  << DgString0(extViewName);

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  Int32 objectOwnerID = 0;
  Int32 schemaOwnerID = 0;
  Int64 objUID = getObjectUIDandOwners(&cliInterface,
			              catalogNamePart.data(), schemaNamePart.data(), 
			              objectNamePart.data(),
			              COM_VIEW_OBJECT,
                                      objectOwnerID,schemaOwnerID);

  if (objUID < 0 || objectOwnerID == 0)
    {
      if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
        SEABASEDDL_INTERNAL_ERROR("getting object UID and owner for drop view");

      deallocEHI(ehi); 

      processReturn();

      return;
    }

  // Verify user can perform operation
  if (!isDDLOperationAuthorized(SQLOperation::DROP_VIEW,objectOwnerID,schemaOwnerID))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
     deallocEHI(ehi);
     processReturn ();
     return;
  }
 
  Queue * usingViewsQueue = NULL;
  if (dropViewNode->getDropBehavior() == COM_RESTRICT_DROP_BEHAVIOR)
    {
      NAString usingObjName;
      cliRC = getUsingObject(&cliInterface, objUID, usingObjName);
      if (cliRC < 0)
	{
	  deallocEHI(ehi); 

	  processReturn();
	  
	  return;
	}

      if (cliRC != 100) // found an object
	{
	  *CmpCommon::diags() << DgSqlCode(-1047)
			      << DgTableName(usingObjName);

	  deallocEHI(ehi); 

	  processReturn();

	  return;
	}
    }
  else if (dropViewNode->getDropBehavior() == COM_CASCADE_DROP_BEHAVIOR)
    {
      cliRC = getUsingViews(&cliInterface, objUID, usingViewsQueue);
      if (cliRC < 0)
	{
	  deallocEHI(ehi); 

	  processReturn();
	  
	  return;
	}
    }

  // get the list of all tables referenced by the view.  Save this list so 
  // referenced tables can be removed from cache later
  NAList<objectRefdByMe> tablesRefdList;
  short status = getListOfReferencedTables(&cliInterface, objUID, tablesRefdList);

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
	      deallocEHI(ehi); 

	      processReturn();
	      
	      return;
	    }
	}
    }

  if (dropSeabaseObject(ehi, tabName,
			 currCatName, currSchName, COM_VIEW_OBJECT))
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  // clear view definition from my cache only. 
  CorrName cn(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn,
    NATableDB::REMOVE_MINE_ONLY, COM_VIEW_OBJECT);

  // clear view from all other caches here. This compensates for a 
  // scenario where the object UID is not available in removeNATable, 
  // and the look up failed too.  Solution is just to use the objectUID 
  // here.
  SQL_QIKEY qiKey;
  qiKey.operation[0] = 'O';
  qiKey.operation[1] = 'R';
  qiKey.ddlObjectUID = objUID;
  SQL_EXEC_SetSecInvalidKeys(1, &qiKey);

  // Now remove referenced tables from cache.
  // When a query that references a view is compiled, all views are converted
  // to the underlying base tables.  Query plans are generated to access the
  // tables, and the views are no longer relevant.
  // When dropping a view, query plans that reference the dropped view will
  // continue to work if the plans are cached.  This code removes the 
  // referenced tables from caches to force recompilations so dropped views
  // are noticed.
  for (CollIndex i = 0; i < tablesRefdList.entries(); i++)
    {
      CorrName cn(tablesRefdList[i].objectName,
                  STMTHEAP,
                  tablesRefdList[i].schemaName,
                  tablesRefdList[i].catalogName);
      ActiveSchemaDB()->getNATableDB()->removeNATable(cn,
        NATableDB::REMOVE_FROM_ALL_USERS, COM_BASE_TABLE_OBJECT);
    }

  deallocEHI(ehi); 
      
  processReturn();

  return;
}

void CmpSeabaseDDL::glueQueryFragments(Lng32 queryArraySize,
				       const QString * queryArray,
				       char * &gluedQuery,
				       Lng32 &gluedQuerySize)
{
  Int32 i = 0;
  gluedQuerySize = 0;
  gluedQuery = NULL;

  for (i = 0; i < queryArraySize; i++)
    {
      UInt32 j = 0;
      const char * partn_frag = queryArray[i].str;
      while ((j < strlen(queryArray[i].str)) &&
	     (partn_frag[j] == ' '))
	j++;
      if (j < strlen(queryArray[i].str))
	gluedQuerySize += strlen(&partn_frag[j]);
    }
  
  gluedQuery = 
    new(STMTHEAP) char[gluedQuerySize + 100];
  gluedQuery[0] = 0;
  for (i = 0; i < queryArraySize; i++)
    {
      UInt32 j = 0;
      const char * partn_frag = queryArray[i].str;
      while ((j < strlen(queryArray[i].str)) &&
	     (partn_frag[j] == ' '))
	j++;
      
      if (j < strlen(queryArray[i].str))
	strcat(gluedQuery, &partn_frag[j]);
    }
}

short CmpSeabaseDDL::createMetadataViews(ExeCliInterface * cliInterface)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  char queryBuf[5000];

  for (Int32 i = 0; i < sizeof(allMDviewsInfo)/sizeof(MDViewInfo); i++)
    {
      const MDViewInfo &mdi = allMDviewsInfo[i];
      
      if (! mdi.viewName)
	continue;

      for (Int32 j = 0; j < NUM_MAX_PARAMS; j++)
	{
	  param_[j] = NULL;
	}

      const QString * qs = NULL;
      Int32 sizeOfqs = 0;

      qs = mdi.viewDefnQuery;
      sizeOfqs = mdi.sizeOfDefnArr; 

      Int32 qryArraySize = sizeOfqs / sizeof(QString);
      char * gluedQuery;
      Lng32 gluedQuerySize;
      glueQueryFragments(qryArraySize,  qs,
			 gluedQuery, gluedQuerySize);

      if (strcmp(mdi.viewName, TRAF_TABLES_VIEW) == 0)
	{
	  param_[0] = getSystemCatalog();
	  param_[1] = SEABASE_MD_SCHEMA;
	  param_[2] = getSystemCatalog();
	  param_[3] = SEABASE_MD_SCHEMA;
	  param_[4] = SEABASE_OBJECTS;
	  param_[5] = getSystemCatalog();
	  param_[6] = SEABASE_MD_SCHEMA;
	  param_[7] = SEABASE_TABLES;
	  param_[8] = getSystemCatalog();
	  param_[9] = SEABASE_MD_SCHEMA;
	  param_[10] = COM_BASE_TABLE_OBJECT_LIT;
	}
      else if (strcmp(mdi.viewName, TRAF_COLUMNS_VIEW) == 0)
	{
	  param_[0] = getSystemCatalog();
	  param_[1] = SEABASE_MD_SCHEMA;
	  param_[2] = getSystemCatalog();
	  param_[3] = SEABASE_MD_SCHEMA;
	  param_[4] = SEABASE_OBJECTS;
	  param_[5] = getSystemCatalog();
	  param_[6] = SEABASE_MD_SCHEMA;
	  param_[7] = SEABASE_COLUMNS;
	  param_[8] = getSystemCatalog();
	  param_[9] = SEABASE_MD_SCHEMA;
	  param_[10] = COM_BASE_TABLE_OBJECT_LIT;
	}
      else if (strcmp(mdi.viewName, TRAF_INDEXES_VIEW) == 0)
	{
	  param_[0] = getSystemCatalog();
	  param_[1] = SEABASE_MD_SCHEMA;
	  param_[2] = getSystemCatalog();
	  param_[3] = SEABASE_MD_SCHEMA;
	  param_[4] = SEABASE_INDEXES;
	  param_[5] = getSystemCatalog();
	  param_[6] = SEABASE_MD_SCHEMA;
	  param_[7] = SEABASE_OBJECTS;
	  param_[8] = getSystemCatalog();
	  param_[9] = SEABASE_MD_SCHEMA;
	  param_[10] = SEABASE_OBJECTS;
	  param_[11] = getSystemCatalog();
	  param_[12] = SEABASE_MD_SCHEMA;
	}
      else if (strcmp(mdi.viewName, TRAF_KEYS_VIEW) == 0)
	{
	  param_[0] = getSystemCatalog();
	  param_[1] = SEABASE_MD_SCHEMA;
	  param_[2] = getSystemCatalog();
	  param_[3] = SEABASE_MD_SCHEMA;
	  param_[4] = SEABASE_TABLE_CONSTRAINTS;
	  param_[5] = getSystemCatalog();
	  param_[6] = SEABASE_MD_SCHEMA;
	  param_[7] = SEABASE_OBJECTS;
	  param_[8] = getSystemCatalog();
	  param_[9] = SEABASE_MD_SCHEMA;
	  param_[10] = SEABASE_OBJECTS;
	  param_[11] = getSystemCatalog();
	  param_[12] = SEABASE_MD_SCHEMA;
	  param_[13] = SEABASE_KEYS;
	  param_[14] = getSystemCatalog();
	  param_[15] = SEABASE_MD_SCHEMA;
	}
     else if (strcmp(mdi.viewName, TRAF_REF_CONSTRAINTS_VIEW) == 0)
	{
	  param_[0] = getSystemCatalog();
	  param_[1] = SEABASE_MD_SCHEMA;
	  param_[2] = getSystemCatalog();
	  param_[3] = SEABASE_MD_SCHEMA;
	  param_[4] = SEABASE_REF_CONSTRAINTS;
	  param_[5] = getSystemCatalog();
	  param_[6] = SEABASE_MD_SCHEMA;
	  param_[7] = SEABASE_OBJECTS;
	  param_[8] = getSystemCatalog();
	  param_[9] = SEABASE_MD_SCHEMA;
	  param_[10] = SEABASE_OBJECTS;
	  param_[11] = getSystemCatalog();
	  param_[12] = SEABASE_MD_SCHEMA;
	  param_[13] = SEABASE_OBJECTS;
	  param_[14] = getSystemCatalog();
	  param_[15] = SEABASE_MD_SCHEMA;
	  param_[16] = SEABASE_TABLE_CONSTRAINTS;
	  param_[17] = getSystemCatalog();
	  param_[18] = SEABASE_MD_SCHEMA;
	}
     else if (strcmp(mdi.viewName, TRAF_SEQUENCES_VIEW) == 0)
	{
	  param_[0] = getSystemCatalog();
	  param_[1] = SEABASE_MD_SCHEMA;
	  param_[2] = getSystemCatalog();
	  param_[3] = SEABASE_MD_SCHEMA;
	  param_[4] = SEABASE_OBJECTS;
	  param_[5] = getSystemCatalog();
	  param_[6] = SEABASE_MD_SCHEMA;
	  param_[7] = SEABASE_SEQ_GEN;
	  param_[8] = getSystemCatalog();
	  param_[9] = SEABASE_MD_SCHEMA;
	  param_[10] = COM_SEQUENCE_GENERATOR_OBJECT_LIT;
	}
      else if (strcmp(mdi.viewName, TRAF_VIEWS_VIEW) == 0)
	{
	  param_[0] = getSystemCatalog();
	  param_[1] = SEABASE_MD_SCHEMA;
	  param_[2] = getSystemCatalog();
	  param_[3] = SEABASE_MD_SCHEMA;
	  param_[4] = SEABASE_OBJECTS;
	  param_[5] = getSystemCatalog();
	  param_[6] = SEABASE_MD_SCHEMA;
	  param_[7] = SEABASE_VIEWS;
	  param_[8] = getSystemCatalog();
	  param_[9] = SEABASE_MD_SCHEMA;
	  param_[10] = COM_VIEW_OBJECT_LIT;
	}
      else
	{
          NADELETEBASIC(gluedQuery, STMTHEAP);
	  continue;
	}

      str_sprintf(queryBuf, gluedQuery,
		  param_[0], param_[1], param_[2], param_[3], param_[4],
		  param_[5], param_[6], param_[7], param_[8], param_[9],
		  param_[10], param_[11], param_[12], param_[13], param_[14],
		  param_[15], param_[16], param_[17], param_[18]);

      NADELETEBASIC(gluedQuery, STMTHEAP);

      NABoolean xnWasStartedHere = FALSE;
      if (beginXnIfNotInProgress(cliInterface, xnWasStartedHere))
        return -1;

      cliRC = cliInterface->executeImmediate(queryBuf);
      if (cliRC == -1390)  // view already exists
	{
	  // ignore the error.
          cliRC = 0;
	}
      else if (cliRC < 0)
	{
	  cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
	}
      
      if (endXnIfStartedHere(cliInterface, xnWasStartedHere, cliRC) < 0)
        return -1;
      
    } // for

  return 0;
}

short CmpSeabaseDDL::dropMetadataViews(ExeCliInterface * cliInterface)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  char queryBuf[5000];

  for (Int32 i = 0; i < sizeof(allMDviewsInfo)/sizeof(MDViewInfo); i++)
    {
      const MDViewInfo &mdi = allMDviewsInfo[i];
      
      if (! mdi.viewName)
	continue;

      str_sprintf(queryBuf, "drop view %s.\"%s\".%s",
		  getSystemCatalog(), SEABASE_MD_SCHEMA,
		  mdi.viewName);

      NABoolean xnWasStartedHere = FALSE;
      if (beginXnIfNotInProgress(cliInterface, xnWasStartedHere))
        return -1;

      cliRC = cliInterface->executeImmediate(queryBuf);
      if (cliRC < 0)
	{
	  cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
	}
      
      if (endXnIfStartedHere(cliInterface, xnWasStartedHere, cliRC) < 0)
        return -1;
      
    } // for

  return 0;
}


  
