/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_apache_hadoop_hbase_client_transactional_RMInterface */

#ifndef _Included_org_apache_hadoop_hbase_client_transactional_RMInterface
#define _Included_org_apache_hadoop_hbase_client_transactional_RMInterface
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_apache_hadoop_hbase_client_transactional_RMInterface
 * Method:    registerRegion
 * Signature: (I[BJ[B)V
 */
JNIEXPORT void JNICALL Java_org_apache_hadoop_hbase_client_transactional_RMInterface_registerRegion
  (JNIEnv *, jobject, jint, jbyteArray, jlong, jbyteArray);

/*
 * Class:     org_apache_hadoop_hbase_client_transactional_RMInterface
 * Method:    createTableReq
 * Signature: ([B[[BIIJ[B)V
 */
JNIEXPORT void JNICALL Java_org_apache_hadoop_hbase_client_transactional_RMInterface_createTableReq
  (JNIEnv *, jobject, jbyteArray, jobjectArray, jint, jint, jlong, jbyteArray);

/*
 * Class:     org_apache_hadoop_hbase_client_transactional_RMInterface
 * Method:    dropTableReq
 * Signature: ([BJ)V
 */
JNIEXPORT void JNICALL Java_org_apache_hadoop_hbase_client_transactional_RMInterface_dropTableReq
  (JNIEnv *, jobject, jbyteArray, jlong);

#ifdef __cplusplus
}
#endif
#endif
