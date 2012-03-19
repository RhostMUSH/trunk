/*************************************************************************************************
 * Implementation of Depot for Java
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


#include "qdbm_Depot.h"
#include <depot.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAXOPEN 1024


DEPOT *dptable[MAXOPEN];

static int getnewindex(void);
static int checkdup(const char *name);
static int getdpomode(jint omode);
static int getdpdmode(jint dmode);



/*************************************************************************************************
 * public objects
 *************************************************************************************************/


JNIEXPORT void JNICALL
Java_qdbm_Depot_dpinit(JNIEnv *env, jclass myclass)
{
  int i;
  for(i = 0; i < MAXOPEN; i++){
    dptable[i] = NULL;
  }
}


JNIEXPORT jstring JNICALL
Java_qdbm_Depot_dpversion(JNIEnv *env, jclass myclass)
{
  return (*env)->NewStringUTF(env, dpversion);
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpecode(JNIEnv *env, jclass myclass){
  switch(dpecode){
  case DP_ENOERR: return qdbm_Depot_ENOERR;
  case DP_EFATAL: return qdbm_Depot_EFATAL;
  case DP_EMODE: return qdbm_Depot_EMODE;
  case DP_EBROKEN: return qdbm_Depot_EBROKEN;
  case DP_EKEEP: return qdbm_Depot_EKEEP;
  case DP_ENOITEM: return qdbm_Depot_ENOITEM;
  case DP_EALLOC: return qdbm_Depot_EALLOC;
  case DP_EMAP: return qdbm_Depot_EMAP;
  case DP_EOPEN: return qdbm_Depot_EOPEN;
  case DP_ECLOSE: return qdbm_Depot_ECLOSE;
  case DP_ETRUNC: return qdbm_Depot_ETRUNC;
  case DP_ESYNC: return qdbm_Depot_ESYNC;
  case DP_ESTAT: return qdbm_Depot_ESTAT;
  case DP_ESEEK: return qdbm_Depot_ESEEK;
  case DP_EREAD: return qdbm_Depot_EREAD;
  case DP_EWRITE: return qdbm_Depot_EWRITE;
  case DP_ELOCK: return qdbm_Depot_ELOCK;
  case DP_EUNLINK: return qdbm_Depot_EUNLINK;
  case DP_EMKDIR: return qdbm_Depot_EMKDIR;
  case DP_ERMDIR: return qdbm_Depot_ERMDIR;
  case DP_EMISC: return qdbm_Depot_EMISC;
  }
  return -1;
}


JNIEXPORT jstring JNICALL
Java_qdbm_Depot_dperrmsg(JNIEnv *env, jclass myclass, jint ecode)
{
  return (*env)->NewStringUTF(env, dperrmsg(ecode));
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpopen(JNIEnv *env, jclass myclass, jstring name, jint omode, jint bnum)
{
  DEPOT *depot;
  const char *tname;
  jboolean ic;
  int index;
  if((index = getnewindex()) == -1) return -1;
  tname = (*env)->GetStringUTFChars(env, name, &ic);
  if(checkdup(tname) == -1){
    if(ic == JNI_TRUE) (*env)->ReleaseStringUTFChars(env, name, tname);
    dpecode = DP_EMISC;
    return -1;
  }
  depot = dpopen(tname, getdpomode(omode), bnum);
  if(ic == JNI_TRUE) (*env)->ReleaseStringUTFChars(env, name, tname);
  if(!depot) return -1;
  dptable[index] = depot;
  return index;
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpclose(JNIEnv *env, jclass myclass, jint index)
{
  DEPOT *depot;
  depot = dptable[index];
  dptable[index] = NULL;
  return dpclose(depot);
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpput(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz,
                      jbyteArray val, jint vsiz, jint dmode)
{
  jbyte *kbuf, *vbuf;
  jboolean ick, icv;
  int rv;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  vbuf = (*env)->GetByteArrayElements(env, val, &icv);
  rv = dpput(dptable[index], (char *)kbuf, ksiz, (char *)vbuf, vsiz, getdpdmode(dmode));
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  if(icv == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, val, vbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpout(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz)
{
  jbyte *kbuf;
  jboolean ick;
  int rv;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  rv = dpout(dptable[index], (char *)kbuf, ksiz);
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jbyteArray JNICALL
Java_qdbm_Depot_dpget(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz,
                      jint start, jint max)
{
  jbyte *kbuf;
  jboolean ick;
  char *val;
  int vsiz;
  jbyteArray vbuf;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  val = dpget(dptable[index], (char *)kbuf, ksiz, start, max, &vsiz);
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  if(val){
    vbuf = (*env)->NewByteArray(env, vsiz);
    (*env)->SetByteArrayRegion(env, vbuf, 0, vsiz, (jbyte *)val);
    free(val);
  } else {
    vbuf = NULL;
  }
  return vbuf;
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpvsiz(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz)
{
  jbyte *kbuf;
  jboolean ick;
  int rv;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  rv = dpvsiz(dptable[index], (char *)kbuf, ksiz);
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpiterinit(JNIEnv *env, jclass myclass, jint index)
{
  return dpiterinit(dptable[index]);
}


JNIEXPORT jbyteArray JNICALL
Java_qdbm_Depot_dpiternext(JNIEnv *env, jclass myclass, jint index)
{
  char *val;
  int vsiz;
  jbyteArray vbuf;
  val = dpiternext(dptable[index], &vsiz);
  if(val){
    vbuf = (*env)->NewByteArray(env, vsiz);
    (*env)->SetByteArrayRegion(env, vbuf, 0, vsiz, (jbyte *)val);
    free(val);
  } else {
    vbuf = NULL;
  }
  return vbuf;
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpsetalign(JNIEnv *env, jclass myclass, jint index, jint align)
{
  return dpsetalign(dptable[index], align);
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpsetfbpsiz(JNIEnv *env, jclass myclass, jint index, jint size)
{
  return dpsetfbpsiz(dptable[index], size);
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpsync(JNIEnv *env, jclass myclass, jint index)
{
  return dpsync(dptable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpoptimize(JNIEnv *env, jclass myclass, jint index, jint bnum)
{
  return dpoptimize(dptable[index], bnum);
}


JNIEXPORT jstring JNICALL
Java_qdbm_Depot_dpname(JNIEnv *env, jclass myclass, jint index)
{
  char *name;
  jstring nbuf;
  name = dpname(dptable[index]);
  if(name){
    nbuf = (*env)->NewStringUTF(env, name);
    free(name);
  } else {
    nbuf = NULL;
  }
  return nbuf;
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpfsiz(JNIEnv *env, jclass myclass, jint index)
{
  return dpfsiz(dptable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpbnum(JNIEnv *env, jclass myclass, jint index)
{
  return dpbnum(dptable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpbusenum(JNIEnv *env, jclass myclass, jint index)
{
  return dpbusenum(dptable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dprnum(JNIEnv *env, jclass myclass, jint index)
{
  return dprnum(dptable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpwritable(JNIEnv *env, jclass myclass, jint index)
{
  return dpwritable(dptable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpfatalerror(JNIEnv *env, jclass myclass, jint index)
{
  return dpfatalerror(dptable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpinode(JNIEnv *env, jclass myclass, jint index){
  return dpinode(dptable[index]);
}


JNIEXPORT jlong JNICALL
Java_qdbm_Depot_dpmtime(JNIEnv *env, jclass myclass, jint index){
  return dpmtime(dptable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Depot_dpremove(JNIEnv *env, jclass myclass, jstring name)
{
  const char *tname;
  jboolean ic;
  int rv;
  tname = (*env)->GetStringUTFChars(env, name, &ic);
  rv = dpremove(tname);
  if(ic == JNI_TRUE) (*env)->ReleaseStringUTFChars(env, name, tname);
  return rv;
}


JNIEXPORT jbyteArray JNICALL
Java_qdbm_Depot_dpsnaffle(JNIEnv *env, jclass myclass, jstring name, jbyteArray key, jint ksiz)
{
  jbyte *kbuf;
  jboolean icn, ick;
  const char *tname;
  char *val;
  int vsiz;
  jbyteArray vbuf;
  tname = (*env)->GetStringUTFChars(env, name, &icn);
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  val = dpsnaffle(tname, (char *)kbuf, ksiz, &vsiz);
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  if(icn == JNI_TRUE) (*env)->ReleaseStringUTFChars(env, name, tname);
  if(val){
    vbuf = (*env)->NewByteArray(env, vsiz);
    (*env)->SetByteArrayRegion(env, vbuf, 0, vsiz, (jbyte *)val);
    free(val);
  } else {
    vbuf = NULL;
  }
  return vbuf;
}



/*************************************************************************************************
 * private objects
 *************************************************************************************************/


static int getnewindex(void){
  int i;
  for(i = 0; i < MAXOPEN; i++){
    if(dptable[i] == NULL) return i;
  }
  return -1;
}


static int checkdup(const char *name){
  struct stat sbuf;
  int i, inode;
  if(stat(name, &sbuf) == -1) return 0;
  inode = sbuf.st_ino;
  for(i = 0; i < MAXOPEN; i++){
    if(dptable[i] != NULL && dpinode(dptable[i]) == inode) return -1;
  }
  return 0;
}


static int getdpomode(jint omode){
  int dpomode;
  dpomode = DP_OREADER;
  if(omode & qdbm_Depot_OWRITER){
    dpomode = DP_OWRITER;
    if(omode & qdbm_Depot_OCREAT) dpomode |= DP_OCREAT;
    if(omode & qdbm_Depot_OTRUNC) dpomode |= DP_OTRUNC;
  }
  if(omode & qdbm_Depot_ONOLCK) dpomode |= DP_ONOLCK;
  if(omode & qdbm_Depot_OLCKNB) dpomode |= DP_OLCKNB;
  if(omode & qdbm_Depot_OSPARSE) dpomode |= DP_OSPARSE;
  return dpomode;
}


static int getdpdmode(jint dmode){
  switch(dmode){
  case qdbm_Depot_DOVER: return DP_DOVER;
  case qdbm_Depot_DKEEP: return DP_DKEEP;
  case qdbm_Depot_DCAT: return DP_DCAT;
  }
  return -1;
}



/* END OF FILE */
