#! /bin/sh
#######################################################################
# @@@ START COPYRIGHT @@@
#
#        HP CONFIDENTIAL: NEED TO KNOW ONLY
#
#        Copyright 2001
#        Hewlett-Packard Development Company, L.P.
#        Protected as an unpublished work.
#
#  The computer program listings, specifications and documentation 
#  herein are the property of Hewlett-Packard Development Company,
#  L.P., or a third party supplier and shall not be reproduced, 
#  copied, disclosed, or used in whole or in part for any reason 
#  without the prior express written permission of Hewlett-Packard 
#  Development Company, L.P.
#
# @@@ END COPYRIGHT @@@
# +++ Copyright added on 2003/12/3
# +++ Code modified on 2001/8/1
#######################################################################


# Determine if running on NT or NSK.  Different compilers/linkers are used
if [ `uname` = "Windows_NT" -o `uname` = "Linux" ]; then
  nsk=0
  mxci=`which sqlci`
else
  nsk=1
  mxci=${mxcidir}/mxci
fi

# Setting the mxci initialization string here is necessary to accommodate
# the sqlutils test suite.
schema=${TEST_SCHEMA:-'cat.sch'}
export SQL_MXCI_INITIALIZATION="set schema $schema"

echo $mxci
$mxci $*
