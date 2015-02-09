#ifndef VPROC_H
#define VPROC_H
#include "SCMBuildStr.h"
#include "SCMVersHelp.h"
#define JDBC_VERS_BUILD1(name,branch) #name #branch
#define JDBC_VERS_BUILD2(name,branch) name ## branch
#define JDBC_VERS1(name,branch) JDBC_VERS_BUILD1(name,branch)
#define JDBC_VERS2(name,branch) JDBC_VERS_BUILD2(name,branch)
extern const char *driverVproc;
extern "C" void JDBC_VERS2(Traf_JDBC_Type2_Build_,VERS_BR3)(void);
#endif /* VPROC_H */
