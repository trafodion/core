#!/bin/bash
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@

DATE=`date +%y%m%d`
BLDID=`../../../../../sqf/build-scripts/build.id`

#create_version_file function to update version string
# $1 should be lib name version or version_drvr
create_version_file() {
if [ "$1" == "version" ]; then
	 filename="version.cpp"
	 ptname="TRAFODBC"
elif [ "$1" == "version_drvr" ]; then
	 filename="version_drvr.cpp"
	 ptname="TRAFODBC_DRVR"
fi
echo "//$ptname Version File generated on $DATE, bldId $BLDID" > $filename
echo "extern char* versionString=\"$ptname (Build Id [$BLDID])\";" >> $filename
echo "extern \"C\" void" $ptname"_Build_Id_"$BLDID" ()" >> $filename
echo "{ }" >> $filename
}

create_version_file "$1"

