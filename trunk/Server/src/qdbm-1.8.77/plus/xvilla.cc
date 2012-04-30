/*************************************************************************************************
 * Implementation of Villa for C++
 *                                                      Copyright (C) 2000-2005 Mikio Hirabayashi
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


#include "xvilla.h"
#include <new>
#include <cstdlib>
#include <ctime>

extern "C" {
#include <depot.h>
#include <cabin.h>
#include <villa.h>
#include <pthread.h>
}

using namespace qdbm;



/*************************************************************************************************
 * Villa_error
 *************************************************************************************************/


Villa_error::Villa_error() throw()
  : DBM_error(){
  ecode = DP_EMISC;
  return;
}


Villa_error::Villa_error(int ecode) throw()
  : DBM_error(){
  this->ecode = ecode;
  return;
}


Villa_error::Villa_error(const Villa_error& ve) throw()
  : DBM_error(ve){
  ecode = ve.ecode;
  return;
}


Villa_error::~Villa_error() throw(){
  return;
}


Villa_error& Villa_error::operator =(const Villa_error& ve) throw(){
  this->ecode = ve.ecode;
  return *this;
}


Villa_error& Villa_error::operator =(int ecode) throw(){
  this->ecode = ecode;
  return *this;
}


bool Villa_error::operator ==(const Villa_error& ve) const throw(){
  return ecode == ve.ecode;
}


bool Villa_error::operator !=(const Villa_error& ve) const throw(){
  return ecode != ve.ecode;
}


bool Villa_error::operator ==(int ecode) const throw(){
  return this->ecode == ecode;
}


bool Villa_error::operator !=(int ecode) const throw(){
  return this->ecode != ecode;
}


Villa_error::operator const char*() const throw(){
  return dperrmsg(ecode);
}


int Villa_error::code() const throw(){
  return ecode;
}


const char* Villa_error::message() const throw(){
  return dperrmsg(ecode);
}



/*************************************************************************************************
 * Villa
 *************************************************************************************************/


const int Villa::ENOERR = DP_ENOERR;
const int Villa::EFATAL = DP_EFATAL;
const int Villa::EMODE = DP_EMODE;
const int Villa::EBROKEN = DP_EBROKEN;
const int Villa::EKEEP = DP_EKEEP;
const int Villa::ENOITEM = DP_ENOITEM;
const int Villa::EALLOC = DP_EALLOC;
const int Villa::EMAP = DP_EMAP;
const int Villa::EOPEN = DP_EOPEN;
const int Villa::ECLOSE = DP_ECLOSE;
const int Villa::ETRUNC = DP_ETRUNC;
const int Villa::ESYNC = DP_ESYNC;
const int Villa::ESTAT = DP_ESTAT;
const int Villa::ESEEK = DP_ESEEK;
const int Villa::EREAD = DP_EREAD;
const int Villa::EWRITE = DP_EWRITE;
const int Villa::ELOCK = DP_ELOCK;
const int Villa::EUNLINK = DP_EUNLINK;
const int Villa::EMKDIR = DP_EMKDIR;
const int Villa::ERMDIR = DP_ERMDIR;
const int Villa::EMISC = DP_EMISC;
const int Villa::OREADER = VL_OREADER;
const int Villa::OWRITER = VL_OWRITER;
const int Villa::OCREAT = VL_OCREAT;
const int Villa::OTRUNC = VL_OTRUNC;
const int Villa::ONOLCK = VL_ONOLCK;
const int Villa::OLCKNB = VL_OLCKNB;
const int Villa::OZCOMP = VL_OZCOMP;
const int Villa::OYCOMP = VL_OYCOMP;
const int Villa::OXCOMP = VL_OXCOMP;
const int Villa::DOVER = VL_DOVER;
const int Villa::DKEEP = VL_DKEEP;
const int Villa::DCAT = VL_DCAT;
const int Villa::DDUP = VL_DDUP;
const int Villa::DDUPR = VL_DDUPR;
const int Villa::JFORWARD = VL_JFORWARD;
const int Villa::JBACKWARD = VL_JBACKWARD;
const int Villa::CPCURRENT = VL_CPCURRENT;
const int Villa::CPBEFORE = VL_CPBEFORE;
const int Villa::CPAFTER = VL_CPAFTER;


const char* Villa::version() throw(){
  return dpversion;
}


void Villa::remove(const char* name) throw(Villa_error){
  if(pthread_mutex_lock(&ourmutex) != 0) throw Villa_error();
  if(!vlremove(name)){
    int ecode = dpecode;
    pthread_mutex_unlock(&ourmutex);
    throw Villa_error(ecode);
  }
  pthread_mutex_unlock(&ourmutex);
}


int Villa::cmplex(const char *aptr, int asiz, const char *bptr, int bsiz) throw(){
  return VL_CMPLEX(aptr, asiz, bptr, bsiz);
}


int Villa::cmpint(const char *aptr, int asiz, const char *bptr, int bsiz) throw(){
  return VL_CMPINT(aptr, asiz, bptr, bsiz);
}


int Villa::cmpnum(const char *aptr, int asiz, const char *bptr, int bsiz) throw(){
  return VL_CMPNUM(aptr, asiz, bptr, bsiz);
}


int Villa::cmpdec(const char *aptr, int asiz, const char *bptr, int bsiz) throw(){
  return VL_CMPDEC(aptr, asiz, bptr, bsiz);
}


Villa::Villa(const char* name, int omode, VLCFUNC cmp) throw(Villa_error)
  : ADBM(){
  pthread_mutex_init(&mymutex, NULL);
  silent = false;
  if(!lock()) throw Villa_error();
  if(!(villa = vlopen(name, omode, cmp))){
    int ecode = dpecode;
    unlock();
    throw Villa_error(ecode);
  }
  pthread_mutex_init(&tranmutex, NULL);
  unlock();
}


Villa::~Villa() throw(){
  pthread_mutex_destroy(&tranmutex);
  if(!villa){
    pthread_mutex_destroy(&mymutex);
    return;
  }
  if(lock()){
    vlclose(villa);
    unlock();
  }
  villa = 0;
  pthread_mutex_destroy(&mymutex);
}


void Villa::close() throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vlclose(villa)){
    int ecode = dpecode;
    villa = 0;
    unlock();
    throw Villa_error(ecode);
  }
  villa = 0;
  unlock();
}


bool Villa::put(const char* kbuf, int ksiz, const char* vbuf, int vsiz, int dmode)
  throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vlput(villa, kbuf, ksiz, vbuf, vsiz, dmode)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == EKEEP) return false;
    throw Villa_error(ecode);
  }
  unlock();
  return true;
}


bool Villa::out(const char* kbuf, int ksiz) throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vlout(villa, kbuf, ksiz)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return false;
    throw Villa_error(ecode);
  }
  unlock();
  return true;
}


char* Villa::get(const char* kbuf, int ksiz, int* sp) throw(Villa_error){
  char* vbuf;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!(vbuf = vlget(villa, kbuf, ksiz, sp))){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Villa_error(ecode);
  }
  unlock();
  return vbuf;
}


int Villa::vsiz(const char *kbuf, int ksiz) throw(Villa_error){
  int vsiz;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if((vsiz = vlvsiz(villa, kbuf, ksiz)) == -1){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Villa_error(ecode);
  }
  unlock();
  return vsiz;
}


int Villa::vnum(const char *kbuf, int ksiz) throw(Villa_error){
  int vnum;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  vnum = vlvnum(villa, kbuf, ksiz);
  unlock();
  return vnum;
}


bool Villa::curfirst() throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vlcurfirst(villa)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Villa_error(ecode);
  }
  unlock();
  return true;
}


bool Villa::curlast() throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vlcurlast(villa)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Villa_error(ecode);
  }
  unlock();
  return true;
}


bool Villa::curprev() throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vlcurprev(villa)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Villa_error(ecode);
  }
  unlock();
  return true;
}


bool Villa::curnext() throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vlcurnext(villa)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Villa_error(ecode);
  }
  unlock();
  return true;
}


bool Villa::curjump(const char* kbuf, int ksiz, int jmode) throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vlcurjump(villa, kbuf, ksiz, jmode)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Villa_error(ecode);
  }
  unlock();
  return true;
}


char* Villa::curkey(int *sp) throw(Villa_error){
  char* kbuf;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!(kbuf = vlcurkey(villa, sp))){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Villa_error(ecode);
  }
  unlock();
  return kbuf;
}


char* Villa::curval(int *sp) throw(Villa_error){
  char* vbuf;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!(vbuf = vlcurval(villa, sp))){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Villa_error(ecode);
  }
  unlock();
  return vbuf;
}


bool Villa::curput(const char* vbuf, int vsiz, int cpmode) throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vlcurput(villa, vbuf, vsiz, cpmode)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return false;
    throw Villa_error(ecode);
  }
  unlock();
  return true;
}


bool Villa::curout() throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vlcurout(villa)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return false;
    throw Villa_error(ecode);
  }
  unlock();
  return true;
}


void Villa::settuning(int lrecmax, int nidxmax, int lcnum, int ncnum) throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  vlsettuning(villa, lrecmax, nidxmax, lcnum, ncnum);
  unlock();
}


void Villa::sync() throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vlsync(villa)){
    int ecode = dpecode;
    unlock();
    throw Villa_error(ecode);
  }
  unlock();
}


void Villa::optimize() throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vloptimize(villa)){
    int ecode = dpecode;
    unlock();
    throw Villa_error(ecode);
  }
  unlock();
}


char* Villa::name() throw(Villa_error){
  char* buf;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!(buf = vlname(villa))){
    int ecode = dpecode;
    unlock();
    throw Villa_error(ecode);
  }
  unlock();
  return buf;
}


int Villa::fsiz() throw(Villa_error){
  int rv;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if((rv = vlfsiz(villa)) == -1){
    int ecode = dpecode;
    unlock();
    throw Villa_error(ecode);
  }
  unlock();
  return rv;
}


int Villa::lnum() throw(Villa_error){
  int rv;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if((rv = vllnum(villa)) == -1){
    int ecode = dpecode;
    unlock();
    throw Villa_error(ecode);
  }
  unlock();
  return rv;
}


int Villa::nnum() throw(Villa_error){
  int rv;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if((rv = vlnnum(villa)) == -1){
    int ecode = dpecode;
    unlock();
    throw Villa_error(ecode);
  }
  unlock();
  return rv;
}


int Villa::rnum() throw(Villa_error){
  int rv;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if((rv = vlrnum(villa)) == -1){
    int ecode = dpecode;
    unlock();
    throw Villa_error(ecode);
  }
  unlock();
  return rv;
}


bool Villa::writable() throw(Villa_error){
  int rv;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  rv = vlwritable(villa);
  unlock();
  return rv ? true : false;
}


bool Villa::fatalerror() throw(Villa_error){
  int rv;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  rv = vlfatalerror(villa);
  unlock();
  return rv ? true : false;
}


int Villa::inode() throw(Villa_error){
  int rv;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  rv = vlinode(villa);
  unlock();
  return rv;
}


time_t Villa::mtime() throw(Villa_error){
  time_t rv;
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  rv = vlmtime(villa);
  unlock();
  return rv;
}


void Villa::tranbegin() throw(Villa_error){
  if(!villa) throw Villa_error();
  if(pthread_mutex_lock(&tranmutex) != 0) throw Villa_error();
  if(!lock()){
    pthread_mutex_unlock(&tranmutex);
    throw Villa_error();
  }
  if(!vltranbegin(villa)){
    int ecode = dpecode;
    unlock();
    pthread_mutex_unlock(&tranmutex);
    throw Villa_error(ecode);
  }
  unlock();
}


void Villa::trancommit() throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vltrancommit(villa)){
    int ecode = dpecode;
    unlock();
    pthread_mutex_unlock(&tranmutex);
    throw Villa_error(ecode);
  }
  unlock();
  if(pthread_mutex_unlock(&tranmutex) != 0) throw Villa_error();
}


void Villa::tranabort() throw(Villa_error){
  if(!villa) throw Villa_error();
  if(!lock()) throw Villa_error();
  if(!vltranabort(villa)){
    int ecode = dpecode;
    unlock();
    pthread_mutex_unlock(&tranmutex);
    throw Villa_error(ecode);
  }
  unlock();
  if(pthread_mutex_unlock(&tranmutex) != 0) throw Villa_error();
}


void Villa::storerec(const Datum& key, const Datum& val, bool replace) throw(Villa_error){
  if(!put(key.ptr(), key.size(), val.ptr(), val.size(), replace ? VL_DOVER : VL_DKEEP))
    throw Villa_error(EKEEP);
}


void Villa::deleterec(const Datum& key) throw(Villa_error){
  if(!out(key.ptr(), key.size())) throw Villa_error(ENOITEM);
}


Datum Villa::fetchrec(const Datum& key) throw(Villa_error){
  char* vbuf;
  int vsiz;
  vbuf = get(key.ptr(), key.size(), &vsiz);
  if(!vbuf) throw Villa_error(ENOITEM);
  return Datum(vbuf, vsiz, true);
}


Datum Villa::firstkey() throw(Villa_error){
  char* kbuf;
  int ksiz;
  curfirst();
  kbuf = curkey(&ksiz);
  if(!kbuf) throw Villa_error(ENOITEM);
  return Datum(kbuf, ksiz, true);
}


Datum Villa::nextkey() throw(Villa_error){
  char* kbuf;
  int ksiz;
  curnext();
  kbuf = curkey(&ksiz);
  if(!kbuf) throw Villa_error(ENOITEM);
  return Datum(kbuf, ksiz, true);
}


bool Villa::error() throw(Villa_error){
  return fatalerror();
}


Villa::Villa(const Villa& villa) throw(Villa_error){
  throw Villa_error();
}


Villa& Villa::operator =(const Villa& villa) throw(Villa_error){
  throw Villa_error();
}


bool Villa::lock(){
  if(dpisreentrant){
    if(pthread_mutex_lock(&mymutex) != 0) return false;
    return true;
  }
  if(pthread_mutex_lock(&ourmutex) != 0) return false;
  return true;
}


void Villa::unlock(){
  if(dpisreentrant){
    pthread_mutex_unlock(&mymutex);
  } else {
    pthread_mutex_unlock(&ourmutex);
  }
}



/* END OF FILE */
