/*************************************************************************************************
 * Implementation of Curia for Java
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


#include "qdbm_Curia.h"
#include <depot.h>
#include <curia.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAXOPEN 1024


CURIA *crtable[MAXOPEN];

static int getnewindex(void);
static int checkdup(const char *name);
static int getcromode(jint omode);
static int getcrdmode(jint dmode);



/*************************************************************************************************
 * public objects
 *************************************************************************************************/


JNIEXPORT void JNICALL
Java_qdbm_Curia_crinit(JNIEnv *env, jclass myclass)
{
  int i;
  for(i = 0; i < MAXOPEN; i++){
    crtable[i] = NULL;
  }
}


JNIEXPORT jstring JNICALL
Java_qdbm_Curia_crversion(JNIEnv *env, jclass myclass)
{
  return (*env)->NewStringUTF(env, dpversion);
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crecode(JNIEnv *env, jclass myclass)
{
  switch(dpecode){
  case DP_ENOERR: return qdbm_Curia_ENOERR;
  case DP_EFATAL: return qdbm_Curia_EFATAL;
  case DP_EMODE: return qdbm_Curia_EMODE;
  case DP_EBROKEN: return qdbm_Curia_EBROKEN;
  case DP_EKEEP: return qdbm_Curia_EKEEP;
  case DP_ENOITEM: return qdbm_Curia_ENOITEM;
  case DP_EALLOC: return qdbm_Curia_EALLOC;
  case DP_EMAP: return qdbm_Curia_EMAP;
  case DP_EOPEN: return qdbm_Curia_EOPEN;
  case DP_ECLOSE: return qdbm_Curia_ECLOSE;
  case DP_ETRUNC: return qdbm_Curia_ETRUNC;
  case DP_ESYNC: return qdbm_Curia_ESYNC;
  case DP_ESTAT: return qdbm_Curia_ESTAT;
  case DP_ESEEK: return qdbm_Curia_ESEEK;
  case DP_EREAD: return qdbm_Curia_EREAD;
  case DP_EWRITE: return qdbm_Curia_EWRITE;
  case DP_ELOCK: return qdbm_Curia_ELOCK;
  case DP_EUNLINK: return qdbm_Curia_EUNLINK;
  case DP_EMKDIR: return qdbm_Curia_EMKDIR;
  case DP_ERMDIR: return qdbm_Curia_ERMDIR;
  case DP_EMISC: return qdbm_Curia_EMISC;
  }
  return -1;
}



JNIEXPORT jstring JNICALL
Java_qdbm_Curia_crerrmsg(JNIEnv *env, jclass myclass, jint ecode)
{
  return (*env)->NewStringUTF(env, dperrmsg(ecode));
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_cropen(JNIEnv *env, jclass myclass, jstring name, jint omode, jint bnum, jint dnum)
{
  CURIA *curia;
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
  curia = cropen(tname, getcromode(omode), bnum, dnum);
  if(ic == JNI_TRUE) (*env)->ReleaseStringUTFChars(env, name, tname);
  if(!curia) return -1;
  crtable[index] = curia;
  return index;
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crclose(JNIEnv *env, jclass myclass, jint index)
{
  CURIA *curia;
  curia = crtable[index];
  crtable[index] = NULL;
  return crclose(curia);
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crput(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz,
                      jbyteArray val, jint vsiz, jint dmode){
  jbyte *kbuf, *vbuf;
  jboolean ick, icv;
  int rv;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  vbuf = (*env)->GetByteArrayElements(env, val, &icv);
  rv = crput(crtable[index], (char *)kbuf, ksiz, (char *)vbuf, vsiz, getcrdmode(dmode));
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  if(icv == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, val, vbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crout(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz)
{
  jbyte *kbuf;
  jboolean ick;
  int rv;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  rv = crout(crtable[index], (char *)kbuf, ksiz);
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jbyteArray JNICALL
Java_qdbm_Curia_crget(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz,
                      jint start, jint max)
{
  jbyte *kbuf;
  jboolean ick;
  char *val;
  int vsiz;
  jbyteArray vbuf;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  val = crget(crtable[index], (char *)kbuf, ksiz, start, max, &vsiz);
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
Java_qdbm_Curia_crvsiz(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz)
{
  jbyte *kbuf;
  jboolean ick;
  int rv;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  rv = crvsiz(crtable[index], (char *)kbuf, ksiz);
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_criterinit(JNIEnv *env, jclass myclass, jint index)
{
  return criterinit(crtable[index]);
}


JNIEXPORT jbyteArray JNICALL
Java_qdbm_Curia_criternext(JNIEnv *env, jclass myclass, jint index)
{
  char *val;
  int vsiz;
  jbyteArray vbuf;
  val = criternext(crtable[index], &vsiz);
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
Java_qdbm_Curia_crsetalign(JNIEnv *env, jclass myclass, jint index, jint align)
{
  return crsetalign(crtable[index], align);
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crsetfbpsiz(JNIEnv *env, jclass myclass, jint index, jint size)
{
  return crsetfbpsiz(crtable[index], size);
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crsync(JNIEnv *env, jclass myclass, jint index)
{
  return crsync(crtable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_croptimize(JNIEnv *env, jclass myclass, jint index, jint bnum)
{
  return croptimize(crtable[index], bnum);
}


JNIEXPORT jstring JNICALL
Java_qdbm_Curia_crname(JNIEnv *env, jclass myclass, jint index)
{
  char *name;
  jstring nbuf;
  name = crname(crtable[index]);
  if(name){
    nbuf = (*env)->NewStringUTF(env, name);
    free(name);
  } else {
    nbuf = NULL;
  }
  return nbuf;
}


JNIEXPORT jdouble JNICALL
Java_qdbm_Curia_crfsizd(JNIEnv *env, jclass myclass, jint index)
{
  return crfsizd(crtable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crbnum(JNIEnv *env, jclass myclass, jint index)
{
  return crbnum(crtable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crbusenum(JNIEnv *env, jclass myclass, jint index)
{
  return crbusenum(crtable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crrnum(JNIEnv *env, jclass myclass, jint index)
{
  return crrnum(crtable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crwritable(JNIEnv *env, jclass myclass, jint index)
{
  return crwritable(crtable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crfatalerror(JNIEnv *env, jclass myclass, jint index)
{
  return crfatalerror(crtable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crinode(JNIEnv *env, jclass myclass, jint index)
{
  return crinode(crtable[index]);
}


JNIEXPORT jlong JNICALL
Java_qdbm_Curia_crmtime(JNIEnv *env, jclass myclass, jint index)
{
  return crmtime(crtable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crputlob(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz,
                         jbyteArray val, jint vsiz, jint dmode)
{
  jbyte *kbuf, *vbuf;
  jboolean ick, icv;
  int rv;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  vbuf = (*env)->GetByteArrayElements(env, val, &icv);
  rv = crputlob(crtable[index], (char *)kbuf, ksiz, (char *)vbuf, vsiz, getcrdmode(dmode));
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  if(icv == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, val, vbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_croutlob(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz)
{
  jbyte *kbuf;
  jboolean ick;
  int rv;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  rv = croutlob(crtable[index], (char *)kbuf, ksiz);
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jbyteArray JNICALL
Java_qdbm_Curia_crgetlob(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz,
                         jint start, jint max)
{
  jbyte *kbuf;
  jboolean ick;
  char *val;
  int vsiz;
  jbyteArray vbuf;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  val = crgetlob(crtable[index], (char *)kbuf, ksiz, start, max, &vsiz);
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
Java_qdbm_Curia_crvsizlob(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz)
{
  jbyte *kbuf;
  jboolean ick;
  int rv;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  rv = crvsizlob(crtable[index], (char *)kbuf, ksiz);
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crrnumlob(JNIEnv *env, jclass myclass, jint index)
{
  return crrnumlob(crtable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Curia_crremove(JNIEnv *env, jclass myclass, jstring name)
{
  const char *tname;
  jboolean ic;
  int rv;
  tname = (*env)->GetStringUTFChars(env, name, &ic);
  rv = crremove(tname);
  if(ic == JNI_TRUE) (*env)->ReleaseStringUTFChars(env, name, tname);
  return rv;
}


JNIEXPORT jbyteArray JNICALL
Java_qdbm_Curia_crsnaffle(JNIEnv *env, jclass myclass, jstring name, jbyteArray key, jint ksiz)
{
  jbyte *kbuf;
  jboolean icn, ick;
  const char *tname;
  char *val;
  int vsiz;
  jbyteArray vbuf;
  tname = (*env)->GetStringUTFChars(env, name, &icn);
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  val = crsnaffle(tname, (char *)kbuf, ksiz, &vsiz);
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
    if(crtable[i] == NULL) return i;
  }
  return -1;
}


static int checkdup(const char *name){
  struct stat sbuf;
  int i, inode;
  if(stat(name, &sbuf) == -1) return 0;
  inode = sbuf.st_ino;
  for(i = 0; i < MAXOPEN; i++){
    if(crtable[i] != NULL && crinode(crtable[i]) == inode) return -1;
  }
  return 0;
}


static int getcromode(jint omode){
  int cromode;
  cromode = CR_OREADER;
  if(omode & qdbm_Curia_OWRITER){
    cromode = CR_OWRITER;
    if(omode & qdbm_Curia_OCREAT) cromode |= CR_OCREAT;
    if(omode & qdbm_Curia_OTRUNC) cromode |= CR_OTRUNC;
  }
  if(omode & qdbm_Curia_ONOLCK) cromode |= CR_ONOLCK;
  if(omode & qdbm_Curia_OLCKNB) cromode |= CR_OLCKNB;
  if(omode & qdbm_Curia_OSPARSE) cromode |= CR_OSPARSE;
  return cromode;
}


static int getcrdmode(jint dmode){
  switch(dmode){
  case qdbm_Curia_DOVER: return CR_DOVER;
  case qdbm_Curia_DKEEP: return CR_DKEEP;
  case qdbm_Curia_DCAT: return CR_DCAT;
  }
  return -1;
}



/* END OF FILE */
