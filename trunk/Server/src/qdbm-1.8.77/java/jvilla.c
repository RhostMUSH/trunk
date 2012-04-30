/*************************************************************************************************
 * Implementation of Villa for Java
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


#include "qdbm_Villa.h"
#include "qdbm_VillaCursor.h"
#include <depot.h>
#include <cabin.h>
#include <villa.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#if SIZEOF_VOID_P == SIZEOF_INT
#define PTRNUM         jint
#else
#define PTRNUM         jlong
#endif

#define MAXOPEN 1024


JNIEnv *vljnienv;
jclass vlmyclass;
VILLA *vltable[MAXOPEN];

static int getnewindex(void);
static int checkdup(const char *name);
static int getvlomode(jint omode);
static VLCFUNC getvlcmp(jint cmode);
static int getvldmode(jint dmode);
static int getvljmode(jint jmode);
static int getvlcpmode(jint cpmode);
static int objcompare(const char *aptr, int asiz, const char *bptr, int bsiz);



/*************************************************************************************************
 * public objects
 *************************************************************************************************/


JNIEXPORT void JNICALL
Java_qdbm_Villa_vlinit(JNIEnv *env, jclass myclass)
{
  int i;
  for(i = 0; i < MAXOPEN; i++){
    vltable[i] = NULL;
  }
}


JNIEXPORT jstring JNICALL
Java_qdbm_Villa_vlversion(JNIEnv *env, jclass myclass)
{
  return (*env)->NewStringUTF(env, dpversion);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlecode(JNIEnv *env, jclass myclass)
{
  switch(dpecode){
  case DP_ENOERR: return qdbm_Villa_ENOERR;
  case DP_EFATAL: return qdbm_Villa_EFATAL;
  case DP_EMODE: return qdbm_Villa_EMODE;
  case DP_EBROKEN: return qdbm_Villa_EBROKEN;
  case DP_EKEEP: return qdbm_Villa_EKEEP;
  case DP_ENOITEM: return qdbm_Villa_ENOITEM;
  case DP_EALLOC: return qdbm_Villa_EALLOC;
  case DP_EMAP: return qdbm_Villa_EMAP;
  case DP_EOPEN: return qdbm_Villa_EOPEN;
  case DP_ECLOSE: return qdbm_Villa_ECLOSE;
  case DP_ETRUNC: return qdbm_Villa_ETRUNC;
  case DP_ESYNC: return qdbm_Villa_ESYNC;
  case DP_ESTAT: return qdbm_Villa_ESTAT;
  case DP_ESEEK: return qdbm_Villa_ESEEK;
  case DP_EREAD: return qdbm_Villa_EREAD;
  case DP_EWRITE: return qdbm_Villa_EWRITE;
  case DP_ELOCK: return qdbm_Villa_ELOCK;
  case DP_EUNLINK: return qdbm_Villa_EUNLINK;
  case DP_EMKDIR: return qdbm_Villa_EMKDIR;
  case DP_ERMDIR: return qdbm_Villa_ERMDIR;
  case DP_EMISC: return qdbm_Villa_EMISC;
  }
  return -1;
}


JNIEXPORT jstring JNICALL
Java_qdbm_Villa_vlerrmsg(JNIEnv *env, jclass myclass, jint ecode)
{
  return (*env)->NewStringUTF(env, dperrmsg(ecode));
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlopen(JNIEnv *env, jclass myclass, jstring name, jint omode, jint cmode)
{
  VILLA *villa;
  const char *tname;
  jboolean ic;
  int index;
  VLCFUNC cmp;
  vljnienv = env;
  vlmyclass = myclass;
  if((index = getnewindex()) == -1) return -1;
  tname = (*env)->GetStringUTFChars(env, name, &ic);
  cmp = NULL;
  if(checkdup(tname) == -1 || !(cmp = getvlcmp(cmode))){
    if(ic == JNI_TRUE) (*env)->ReleaseStringUTFChars(env, name, tname);
    dpecode = DP_EMISC;
    return -1;
  }
  villa = vlopen(tname, getvlomode(omode), cmp);
  if(ic == JNI_TRUE) (*env)->ReleaseStringUTFChars(env, name, tname);
  if(!villa) return -1;
  vltable[index] = villa;
  return index;
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlclose(JNIEnv *env, jclass myclass, jint index)
{
  VILLA *villa;
  vljnienv = env;
  vlmyclass = myclass;
  villa = vltable[index];
  vltable[index] = NULL;
  return vlclose(villa);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlput(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz,
                      jbyteArray val, jint vsiz, jint dmode)
{
  jbyte *kbuf, *vbuf;
  jboolean ick, icv;
  int rv;
  vljnienv = env;
  vlmyclass = myclass;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  vbuf = (*env)->GetByteArrayElements(env, val, &icv);
  rv = vlput(vltable[index], (char *)kbuf, ksiz, (char *)vbuf, vsiz, getvldmode(dmode));
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  if(icv == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, val, vbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlout(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz)
{
  jbyte *kbuf;
  jboolean ick;
  int rv;
  vljnienv = env;
  vlmyclass = myclass;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  rv = vlout(vltable[index], (char *)kbuf, ksiz);
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jbyteArray JNICALL
Java_qdbm_Villa_vlget(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz)
{
  jbyte *kbuf;
  jboolean ick;
  const char *val;
  int vsiz;
  jbyteArray vbuf;
  vljnienv = env;
  vlmyclass = myclass;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  val = vlgetcache(vltable[index], (char *)kbuf, ksiz, &vsiz);
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  if(val){
    vbuf = (*env)->NewByteArray(env, vsiz);
    (*env)->SetByteArrayRegion(env, vbuf, 0, vsiz, (jbyte *)val);
  } else {
    vbuf = NULL;
  }
  return vbuf;
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlvsiz(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz)
{
  jbyte *kbuf;
  jboolean ick;
  int rv;
  vljnienv = env;
  vlmyclass = myclass;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  rv = vlvsiz(vltable[index], (char *)kbuf, ksiz);
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlvnum(JNIEnv *env, jclass myclass, jint index, jbyteArray key, jint ksiz)
{
  jbyte *kbuf;
  jboolean ick;
  int rv;
  vljnienv = env;
  vlmyclass = myclass;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  rv = vlvnum(vltable[index], (char *)kbuf, ksiz);
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlcurfirst(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vlcurfirst(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlcurlast(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vlcurlast(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlcurprev(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vlcurprev(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlcurnext(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vlcurnext(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlcurjump(JNIEnv *env, jclass myclass, jint index,
                          jbyteArray key, jint ksiz, jint jmode)
{
  jbyte *kbuf;
  jboolean ick;
  int rv;
  vljnienv = env;
  vlmyclass = myclass;
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  rv = vlcurjump(vltable[index], (char *)kbuf, ksiz, getvljmode(jmode));
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jbyteArray JNICALL
Java_qdbm_Villa_vlcurkey(JNIEnv *env, jclass myclass, jint index)
{
  const char *key;
  int ksiz;
  jbyteArray kbuf;
  vljnienv = env;
  vlmyclass = myclass;
  key = vlcurkeycache(vltable[index], &ksiz);
  if(key){
    kbuf = (*env)->NewByteArray(env, ksiz);
    (*env)->SetByteArrayRegion(env, kbuf, 0, ksiz, (jbyte *)key);
  } else {
    kbuf = NULL;
  }
  return kbuf;
}


JNIEXPORT jbyteArray JNICALL
Java_qdbm_Villa_vlcurval(JNIEnv *env, jclass myclass, jint index)
{
  const char *val;
  int vsiz;
  jbyteArray vbuf;
  vljnienv = env;
  vlmyclass = myclass;
  val = vlcurvalcache(vltable[index], &vsiz);
  if(val){
    vbuf = (*env)->NewByteArray(env, vsiz);
    (*env)->SetByteArrayRegion(env, vbuf, 0, vsiz, (jbyte *)val);
  } else {
    vbuf = NULL;
  }
  return vbuf;
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlcurput(JNIEnv *env, jclass myclass, jint index,
                         jbyteArray val, jint vsiz, jint cpmode){
  jbyte *vbuf;
  jboolean icv;
  int rv;
  vljnienv = env;
  vlmyclass = myclass;
  vbuf = (*env)->GetByteArrayElements(env, val, &icv);
  rv = vlcurput(vltable[index], (char *)vbuf, vsiz, getvlcpmode(cpmode));
  if(icv == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, val, vbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlcurout(JNIEnv *env, jclass myclass, jint index){
  vljnienv = env;
  vlmyclass = myclass;
  return vlcurout(vltable[index]);
}


JNIEXPORT void JNICALL
Java_qdbm_Villa_vlsettuning(JNIEnv *env, jclass myclass, jint index,
                            jint lrecmax, jint nidxmax, jint lcnum, jint ncnum)
{
  vljnienv = env;
  vlmyclass = myclass;
  vlsettuning(vltable[index], lrecmax, nidxmax, lcnum, ncnum);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlsync(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vlsync(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vloptimize(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vloptimize(vltable[index]);
}


JNIEXPORT jstring JNICALL
Java_qdbm_Villa_vlname(JNIEnv *env, jclass myclass, jint index)
{
  char *name;
  jstring nbuf;
  vljnienv = env;
  vlmyclass = myclass;
  name = vlname(vltable[index]);
  if(name){
    nbuf = (*env)->NewStringUTF(env, name);
    free(name);
  } else {
    nbuf = NULL;
  }
  return nbuf;
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlfsiz(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vlfsiz(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vllnum(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vllnum(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlnnum(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vlnnum(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlrnum(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vlrnum(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlwritable(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vlwritable(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlfatalerror(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vlfatalerror(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlinode(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vlinode(vltable[index]);
}


JNIEXPORT jlong JNICALL
Java_qdbm_Villa_vlmtime(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vlmtime(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vltranbegin(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vltranbegin(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vltrancommit(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vltrancommit(vltable[index]);
}


JNIEXPORT jint JNICALL Java_qdbm_Villa_vltranabort(JNIEnv *env, jclass myclass, jint index)
{
  vljnienv = env;
  vlmyclass = myclass;
  return vltranabort(vltable[index]);
}


JNIEXPORT jint JNICALL
Java_qdbm_Villa_vlremove(JNIEnv *env, jclass myclass, jstring name)
{
  const char *tname;
  jboolean ic;
  int rv;
  tname = (*env)->GetStringUTFChars(env, name, &ic);
  rv = vlremove(tname);
  if(ic == JNI_TRUE) (*env)->ReleaseStringUTFChars(env, name, tname);
  return rv;
}


JNIEXPORT jint JNICALL
Java_qdbm_VillaCursor_vlmulcurecode(JNIEnv *env, jclass myclass)
{
  return Java_qdbm_Villa_vlecode(env, myclass);
}


JNIEXPORT jint
JNICALL Java_qdbm_VillaCursor_vlmulcurnew(JNIEnv *env, jobject obj, jint index)
{
  jclass cls;
  jfieldID fid;
  VLMULCUR *mulcur;
  if(!(mulcur = vlmulcuropen(vltable[index]))) return 0;
  cls = (*env)->GetObjectClass(env, obj);
  fid = (*env)->GetFieldID(env, cls, "coreptr", "J");
  (*env)->SetLongField(env, obj, fid, (PTRNUM)mulcur);
  return 1;
}


JNIEXPORT void JNICALL
Java_qdbm_VillaCursor_vlmulcurdelete(JNIEnv *env, jobject obj)
{
  jclass cls;
  jfieldID fid;
  jlong coreptr;
  cls = (*env)->GetObjectClass(env, obj);
  fid = (*env)->GetFieldID(env, cls, "coreptr", "J");
  coreptr = (*env)->GetLongField(env, obj, fid);


  printf("hoge\n");

  vlmulcurclose((VLMULCUR *)(PTRNUM)coreptr);
}


JNIEXPORT jint JNICALL
Java_qdbm_VillaCursor_vlmulcurfirst(JNIEnv *env, jobject obj, jint index)
{
  jclass cls;
  jfieldID fid;
  jlong coreptr;
  cls = (*env)->GetObjectClass(env, obj);
  fid = (*env)->GetFieldID(env, cls, "coreptr", "J");
  coreptr = (*env)->GetLongField(env, obj, fid);
  return vlmulcurfirst((VLMULCUR *)(PTRNUM)coreptr);
}


JNIEXPORT jint JNICALL
Java_qdbm_VillaCursor_vlmulcurlast(JNIEnv *env, jobject obj, jint index)
{
  jclass cls;
  jfieldID fid;
  jlong coreptr;
  cls = (*env)->GetObjectClass(env, obj);
  fid = (*env)->GetFieldID(env, cls, "coreptr", "J");
  coreptr = (*env)->GetLongField(env, obj, fid);
  return vlmulcurlast((VLMULCUR *)(PTRNUM)coreptr);
}


JNIEXPORT jint JNICALL
Java_qdbm_VillaCursor_vlmulcurprev(JNIEnv *env, jobject obj, jint index)
{
  jclass cls;
  jfieldID fid;
  jlong coreptr;
  cls = (*env)->GetObjectClass(env, obj);
  fid = (*env)->GetFieldID(env, cls, "coreptr", "J");
  coreptr = (*env)->GetLongField(env, obj, fid);
  return vlmulcurprev((VLMULCUR *)(PTRNUM)coreptr);
}


JNIEXPORT jint JNICALL
Java_qdbm_VillaCursor_vlmulcurnext(JNIEnv *env, jobject obj, jint index)
{
  jclass cls;
  jfieldID fid;
  jlong coreptr;
  cls = (*env)->GetObjectClass(env, obj);
  fid = (*env)->GetFieldID(env, cls, "coreptr", "J");
  coreptr = (*env)->GetLongField(env, obj, fid);
  return vlmulcurnext((VLMULCUR *)(PTRNUM)coreptr);
}


JNIEXPORT jint JNICALL
Java_qdbm_VillaCursor_vlmulcurjump(JNIEnv *env, jobject obj, jint index,
                                   jbyteArray key, jint ksiz, jint jmode)
{
  jclass cls;
  jfieldID fid;
  jlong coreptr;
  jbyte *kbuf;
  jboolean ick;
  int rv;
  cls = (*env)->GetObjectClass(env, obj);
  fid = (*env)->GetFieldID(env, cls, "coreptr", "J");
  coreptr = (*env)->GetLongField(env, obj, fid);
  kbuf = (*env)->GetByteArrayElements(env, key, &ick);
  rv = vlmulcurjump((VLMULCUR *)(PTRNUM)coreptr, (char *)kbuf, ksiz, getvljmode(jmode));
  if(ick == JNI_TRUE) (*env)->ReleaseByteArrayElements(env, key, kbuf, JNI_ABORT);
  return rv;
}


JNIEXPORT jbyteArray JNICALL
Java_qdbm_VillaCursor_vlmulcurkey(JNIEnv *env, jobject obj, jint index)
{
  jclass cls;
  jfieldID fid;
  jlong coreptr;
  const char *key;
  int ksiz;
  jbyteArray kbuf;
  cls = (*env)->GetObjectClass(env, obj);
  fid = (*env)->GetFieldID(env, cls, "coreptr", "J");
  coreptr = (*env)->GetLongField(env, obj, fid);
  key = vlmulcurkeycache((VLMULCUR *)(PTRNUM)coreptr, &ksiz);
  if(key){
    kbuf = (*env)->NewByteArray(env, ksiz);
    (*env)->SetByteArrayRegion(env, kbuf, 0, ksiz, (jbyte *)key);
  } else {
    kbuf = NULL;
  }
  return kbuf;
}


JNIEXPORT jbyteArray JNICALL
Java_qdbm_VillaCursor_vlmulcurval(JNIEnv *env, jobject obj, jint index)
{
  jclass cls;
  jfieldID fid;
  jlong coreptr;
  const char *key;
  int ksiz;
  jbyteArray kbuf;
  cls = (*env)->GetObjectClass(env, obj);
  fid = (*env)->GetFieldID(env, cls, "coreptr", "J");
  coreptr = (*env)->GetLongField(env, obj, fid);
  key = vlmulcurvalcache((VLMULCUR *)(PTRNUM)coreptr, &ksiz);
  if(key){
    kbuf = (*env)->NewByteArray(env, ksiz);
    (*env)->SetByteArrayRegion(env, kbuf, 0, ksiz, (jbyte *)key);
  } else {
    kbuf = NULL;
  }
  return kbuf;
}



/*************************************************************************************************
 * private objects
 *************************************************************************************************/


static int getnewindex(void){
  int i;
  for(i = 0; i < MAXOPEN; i++){
    if(vltable[i] == NULL) return i;
  }
  return -1;
}


static int checkdup(const char *name){
  struct stat sbuf;
  int i, inode;
  if(stat(name, &sbuf) == -1) return 0;
  inode = sbuf.st_ino;
  for(i = 0; i < MAXOPEN; i++){
    if(vltable[i] != NULL && vlinode(vltable[i]) == inode) return -1;
  }
  return 0;
}


static int getvlomode(jint omode){
  int vlomode;
  vlomode = VL_OREADER;
  if(omode & qdbm_Villa_OWRITER){
    vlomode = VL_OWRITER;
    if(omode & qdbm_Villa_OCREAT) vlomode |= VL_OCREAT;
    if(omode & qdbm_Villa_OTRUNC) vlomode |= VL_OTRUNC;
  }
  if(omode & qdbm_Villa_ONOLCK) vlomode |= VL_ONOLCK;
  if(omode & qdbm_Villa_OLCKNB) vlomode |= VL_OLCKNB;
  if(omode & qdbm_Villa_OZCOMP) vlomode |= VL_OZCOMP;
  if(omode & qdbm_Villa_OYCOMP) vlomode |= VL_OYCOMP;
  if(omode & qdbm_Villa_OXCOMP) vlomode |= VL_OXCOMP;
  return vlomode;
}


static VLCFUNC getvlcmp(jint cmode){
  switch(cmode){
  case qdbm_Villa_CMPLEX: return VL_CMPLEX;
  case qdbm_Villa_CMPNUM: return VL_CMPNUM;
  case qdbm_Villa_CMPDEC: return VL_CMPDEC;
  case qdbm_Villa_CMPOBJ: return objcompare;
  }
  return NULL;
}


static int getvldmode(jint dmode){
  switch(dmode){
  case qdbm_Villa_DOVER: return VL_DOVER;
  case qdbm_Villa_DKEEP: return VL_DKEEP;
  case qdbm_Villa_DCAT: return VL_DCAT;
  case qdbm_Villa_DDUP: return VL_DDUP;
  case qdbm_Villa_DDUPR: return VL_DDUPR;
  }
  return -1;
}


static int getvljmode(jint jmode){
  switch(jmode){
  case qdbm_Villa_JFORWARD: return VL_JFORWARD;
  case qdbm_Villa_JBACKWARD: return VL_JBACKWARD;
  }
  return -1;
}


static int getvlcpmode(jint cpmode){
  switch(cpmode){
  case qdbm_Villa_CPCURRENT: return VL_CPCURRENT;
  case qdbm_Villa_CPBEFORE: return VL_CPBEFORE;
  case qdbm_Villa_CPAFTER: return VL_CPAFTER;
  }
  return -1;
}


static int objcompare(const char *aptr, int asiz, const char *bptr, int bsiz){
  jmethodID mid;
  jbyteArray abuf, bbuf;
  mid = (*vljnienv)->GetStaticMethodID(vljnienv, vlmyclass, "objcompare", "([B[B)I");
  abuf = (*vljnienv)->NewByteArray(vljnienv, asiz);
  (*vljnienv)->SetByteArrayRegion(vljnienv, abuf, 0, asiz, (jbyte *)aptr);
  bbuf = (*vljnienv)->NewByteArray(vljnienv, bsiz);
  (*vljnienv)->SetByteArrayRegion(vljnienv, bbuf, 0, bsiz, (jbyte *)bptr);
  return (*vljnienv)->CallStaticIntMethod(vljnienv, vlmyclass, mid, abuf, bbuf);
}



/* END OF FILE */
