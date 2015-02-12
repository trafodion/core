# Top level Linux ODBC makefile

# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2010-2015 Hewlett-Packard Development Company, L.P.
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
include $(MY_SQROOT)/macros.gmk

.PHONY: all
all: install 

.PHONY: clean
clean:
	$(MAKE) -f makefile.lnx clean; 
	/bin/rm -f $(LIBEXPDIR)/libtrafodbc*
	/bin/rm -f $(MY_SQROOT)/../conn/unixodbc/odbc/odbcclient/unixcli/package/trafodbc_Linux64_pkg.tar.gz
	/bin/rm -f $(MY_SQROOT)/../conn/clients/TRAF_ODBC_Linux_Driver_64.tar.gz

# Linux ODBC Driver targets
.PHONY: bldlnx_drvr bldlnx_pkg linux64_$(SQ_BUILD_TYPE) install
bldlnx_drvr: linux64_$(SQ_BUILD_TYPE)
	$(MAKE) -f makefile.lnx $^
$(MY_SQROOT)/../conn/unixodbc/odbc/odbcclient/unixcli/package/trafodbc_Linux64_pkg.tar.gz: bldlnx_drvr
	cd $(MY_SQROOT)/../conn/unixodbc/odbc/odbcclient/unixcli/package && ./mklnxpkg.sh -f
install: $(MY_SQROOT)/../conn/unixodbc/odbc/odbcclient/unixcli/package/trafodbc_Linux64_pkg.tar.gz
	cd $(MY_SQROOT)/../conn/unixodbc/odbc/odbcclient/unixcli/package && cp -p trafodbc_Linux64_pkg.tar.gz $(MY_SQROOT)/../conn/clients/TRAF_ODBC_Linux_Driver_64.tar.gz

