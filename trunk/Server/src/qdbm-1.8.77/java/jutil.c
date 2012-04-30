/*************************************************************************************************
 * Implementation of Utilities of QDBM for Java
 *                                                      Copyright (C) 2000-2006 Mikio Hirabayashi
 * This file is part of QDBM, Quick Database Manager.
 * QDBM is free software; you can redistribute it and/or modify it under the terms of the GNU
 * Lesser General Public License as published by the Free Software Foundation; either version
 * 2.1 of the License or any later version.  QDBM is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * You should have received a copy of the GNU Lesser General Public License along with QDBM; if
 * not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA.
 *************************************************************************************************/


#include "qdbm_Util.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#define PATHBUFSIZ 2048



/*************************************************************************************************
 * public objects
 *************************************************************************************************/


JNIEXPORT jint JNICALL
Java_qdbm_Util_system(JNIEnv *env, jclass myclass, jstring cmd)
{
  const char *tmp;
  jboolean ic;
  int rv;
  tmp = (*env)->GetStringUTFChars(env, cmd, &ic);
  rv = system(tmp);
  if(ic == JNI_TRUE) (*env)->ReleaseStringUTFChars(env, cmd, tmp);
  return rv;
}


JNIEXPORT jint JNICALL
Java_qdbm_Util_chdir(JNIEnv *env, jclass myclass, jstring path)
{
  const char *tmp;
  jboolean ic;
  int rv;
  tmp = (*env)->GetStringUTFChars(env, path, &ic);
  rv = chdir(tmp);
  if(ic == JNI_TRUE) (*env)->ReleaseStringUTFChars(env, path, tmp);
  return rv;
}


JNIEXPORT jstring JNICALL
Java_qdbm_Util_getcwd(JNIEnv *env, jclass myclass)
{
  char path[PATHBUFSIZ];
  if(!getcwd(path, PATHBUFSIZ)) return NULL;
  return (*env)->NewStringUTF(env, path);
}


JNIEXPORT jint JNICALL
Java_qdbm_Util_getpid(JNIEnv *env, jclass myclass)
{
  return getpid();
}


JNIEXPORT jstring JNICALL
Java_qdbm_Util_getenv(JNIEnv *env, jclass myclass, jstring name)
{
  const char *tmp;
  char *val;
  jboolean ic;
  tmp = (*env)->GetStringUTFChars(env, name, &ic);
  val = getenv(tmp);
  if(ic == JNI_TRUE) (*env)->ReleaseStringUTFChars(env, name, tmp);
  if(!val) return 0;
  return (*env)->NewStringUTF(env, val);
}



/* END OF FILE */
