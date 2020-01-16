/*************************************************************************************************
 * Implementation of Curia for C++
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


#include "xcuria.h"
#include <new>
#include <cstdlib>
#include <ctime>

extern "C" {
#include <depot.h>
#include <curia.h>
#include <pthread.h>
}

using namespace qdbm;



/*************************************************************************************************
 * Curia_error
 *************************************************************************************************/


Curia_error::Curia_error() throw()
  : DBM_error(){
  ecode = DP_EMISC;
  return;
}


Curia_error::Curia_error(int ecode) throw()
  : DBM_error(){
  this->ecode = ecode;
  return;
}


Curia_error::Curia_error(const Curia_error& ce) throw()
  : DBM_error(ce){
  ecode = ce.ecode;
  return;
}


Curia_error::~Curia_error() throw(){
  return;
}


Curia_error& Curia_error::operator =(const Curia_error& ce) throw(){
  this->ecode = ce.ecode;
  return *this;
}


Curia_error& Curia_error::operator =(int ecode) throw(){
  this->ecode = ecode;
  return *this;
}


bool Curia_error::operator ==(const Curia_error& ce) const throw(){
  return ecode == ce.ecode;
}


bool Curia_error::operator !=(const Curia_error& ce) const throw(){
  return ecode != ce.ecode;
}


bool Curia_error::operator ==(int ecode) const throw(){
  return this->ecode == ecode;
}


bool Curia_error::operator !=(int ecode) const throw(){
  return this->ecode != ecode;
}


Curia_error::operator const char*() const throw(){
  return dperrmsg(ecode);
}


int Curia_error::code() const throw(){
  return ecode;
}


const char* Curia_error::message() const throw(){
  return dperrmsg(ecode);
}



/*************************************************************************************************
 * Curia
 *************************************************************************************************/


const int Curia::ENOERR = DP_ENOERR;
const int Curia::EFATAL = DP_EFATAL;
const int Curia::EMODE = DP_EMODE;
const int Curia::EBROKEN = DP_EBROKEN;
const int Curia::EKEEP = DP_EKEEP;
const int Curia::ENOITEM = DP_ENOITEM;
const int Curia::EALLOC = DP_EALLOC;
const int Curia::EMAP = DP_EMAP;
const int Curia::EOPEN = DP_EOPEN;
const int Curia::ECLOSE = DP_ECLOSE;
const int Curia::ETRUNC = DP_ETRUNC;
const int Curia::ESYNC = DP_ESYNC;
const int Curia::ESTAT = DP_ESTAT;
const int Curia::ESEEK = DP_ESEEK;
const int Curia::EREAD = DP_EREAD;
const int Curia::EWRITE = DP_EWRITE;
const int Curia::ELOCK = DP_ELOCK;
const int Curia::EUNLINK = DP_EUNLINK;
const int Curia::EMKDIR = DP_EMKDIR;
const int Curia::ERMDIR = DP_ERMDIR;
const int Curia::EMISC = DP_EMISC;
const int Curia::OREADER = CR_OREADER;
const int Curia::OWRITER = CR_OWRITER;
const int Curia::OCREAT = CR_OCREAT;
const int Curia::OTRUNC = CR_OTRUNC;
const int Curia::ONOLCK = CR_ONOLCK;
const int Curia::OLCKNB = CR_OLCKNB;
const int Curia::OSPARSE = CR_OSPARSE;
const int Curia::DOVER = CR_DOVER;
const int Curia::DKEEP = CR_DKEEP;
const int Curia::DCAT = CR_DCAT;


const char* Curia::version() throw(){
  return dpversion;
}


void Curia::remove(const char* name) throw(Curia_error){
  if(pthread_mutex_lock(&ourmutex) != 0) throw Curia_error();
  if(!crremove(name)){
    int ecode = dpecode;
    pthread_mutex_unlock(&ourmutex);
    throw Curia_error(ecode);
  }
  pthread_mutex_unlock(&ourmutex);
}


char* Curia::snaffle(const char* name, const char* kbuf, int ksiz, int* sp) throw(Curia_error){
  char* vbuf;
  if(pthread_mutex_lock(&ourmutex) != 0) throw Curia_error();
  if(!(vbuf = crsnaffle(name, kbuf, ksiz, sp))){
    int ecode = dpecode;
    pthread_mutex_unlock(&ourmutex);
    throw Curia_error(ecode);
  }
  pthread_mutex_unlock(&ourmutex);
  return vbuf;
}


Curia::Curia(const char* name, int omode, int bnum, int dnum) throw(Curia_error)
  : ADBM(){
  pthread_mutex_init(&mymutex, NULL);
  silent = false;
  if(!lock()) throw Curia_error();
  if(!(curia = cropen(name, omode, bnum, dnum))){
    int ecode = dpecode;
    unlock();
    throw Curia_error(ecode);
  }
  unlock();
}


Curia::~Curia() throw(){
  if(!curia){
    pthread_mutex_destroy(&mymutex);
    return;
  }
  if(lock()){
    crclose(curia);
    unlock();
  }
  curia = 0;
  pthread_mutex_destroy(&mymutex);
}


void Curia::close() throw(Curia_error){
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!crclose(curia)){
    int ecode = dpecode;
    curia = 0;
    unlock();
    throw Curia_error(ecode);
  }
  curia = 0;
  unlock();
}


bool Curia::put(const char* kbuf, int ksiz, const char* vbuf, int vsiz, int dmode)
  throw(Curia_error){
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!crput(curia, kbuf, ksiz, vbuf, vsiz, dmode)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == EKEEP) return false;
    throw Curia_error(ecode);
  }
  unlock();
  return true;
}


bool Curia::out(const char* kbuf, int ksiz) throw(Curia_error){
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!crout(curia, kbuf, ksiz)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return false;
    throw Curia_error(ecode);
  }
  unlock();
  return true;
}


char* Curia::get(const char* kbuf, int ksiz, int start, int max, int* sp) throw(Curia_error){
  char* vbuf;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!(vbuf = crget(curia, kbuf, ksiz, start, max, sp))){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Curia_error(ecode);
  }
  unlock();
  return vbuf;
}


int Curia::getwb(const char *kbuf, int ksiz, int start, int max, char *vbuf) throw(Curia_error){
  int vsiz;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if((vsiz = crgetwb(curia, kbuf, ksiz, start, max, vbuf)) == -1){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return -1;
    throw Curia_error(ecode);
  }
  unlock();
  return vsiz;
}


int Curia::vsiz(const char* kbuf, int ksiz) throw(Curia_error){
  int vsiz;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if((vsiz = crvsiz(curia, kbuf, ksiz)) == -1){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return -1;
    throw Curia_error(ecode);
  }
  unlock();
  return vsiz;
}


void Curia::iterinit() throw(Curia_error){
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!criterinit(curia)){
    int ecode = dpecode;
    unlock();
    throw Curia_error(ecode);
  }
  unlock();
}


char* Curia::iternext(int* sp) throw(Curia_error){
  char* vbuf;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!(vbuf = criternext(curia, sp))){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Curia_error(ecode);
  }
  unlock();
  return vbuf;
}


void Curia::setalign(int align) throw(Curia_error){
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!crsetalign(curia, align)){
    int ecode = dpecode;
    unlock();
    throw Curia_error(ecode);
  }
  unlock();
}


void Curia::setfbpsiz(int size) throw(Curia_error){
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!crsetfbpsiz(curia, size)){
    int ecode = dpecode;
    unlock();
    throw Curia_error(ecode);
  }
  unlock();
}


void Curia::sync() throw(Curia_error){
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!crsync(curia)){
    int ecode = dpecode;
    unlock();
    throw Curia_error(ecode);
  }
  unlock();
}


void Curia::optimize(int bnum) throw(Curia_error){
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!croptimize(curia, bnum)){
    int ecode = dpecode;
    unlock();
    throw Curia_error(ecode);
  }
  unlock();
}


char* Curia::name() throw(Curia_error){
  char* buf;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!(buf = crname(curia))){
    int ecode = dpecode;
    unlock();
    throw Curia_error(ecode);
  }
  unlock();
  return buf;
}


int Curia::fsiz() throw(Curia_error){
  int rv;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if((rv = crfsiz(curia)) == -1){
    int ecode = dpecode;
    unlock();
    throw Curia_error(ecode);
  }
  unlock();
  return rv;
}


double Curia::fsizd() throw(Curia_error){
  double rv;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if((rv = crfsizd(curia)) == -1){
    int ecode = dpecode;
    unlock();
    throw Curia_error(ecode);
  }
  unlock();
  return rv;
}


int Curia::bnum() throw(Curia_error){
  int rv;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if((rv = crbnum(curia)) == -1){
    int ecode = dpecode;
    unlock();
    throw Curia_error(ecode);
  }
  unlock();
  return rv;
}


int Curia::busenum() throw(Curia_error){
  int rv;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if((rv = crbusenum(curia)) == -1){
    int ecode = dpecode;
    unlock();
    throw Curia_error(ecode);
  }
  unlock();
  return rv;
}


int Curia::rnum() throw(Curia_error){
  int rv;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if((rv = crrnum(curia)) == -1){
    int ecode = dpecode;
    unlock();
    throw Curia_error(ecode);
  }
  unlock();
  return rv;
}


bool Curia::writable() throw(Curia_error){
  int rv;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  rv = crwritable(curia);
  unlock();
  return rv ? true : false;
}


bool Curia::fatalerror() throw(Curia_error){
  int rv;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  rv = crfatalerror(curia);
  unlock();
  return rv ? true : false;
}


int Curia::inode() throw(Curia_error){
  int rv;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  rv = crinode(curia);
  unlock();
  return rv;
}


time_t Curia::mtime() throw(Curia_error){
  time_t rv;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  rv = crmtime(curia);
  unlock();
  return rv;
}


bool Curia::putlob(const char* kbuf, int ksiz, const char* vbuf, int vsiz, int dmode)
  throw(Curia_error){
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!crputlob(curia, kbuf, ksiz, vbuf, vsiz, dmode)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == EKEEP) return false;
    throw Curia_error(ecode);
  }
  unlock();
  return true;
}


bool Curia::outlob(const char* kbuf, int ksiz) throw(Curia_error){
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!croutlob(curia, kbuf, ksiz)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return false;
    throw Curia_error(ecode);
  }
  unlock();
  return true;
}


char* Curia::getlob(const char* kbuf, int ksiz, int start, int max, int* sp) throw(Curia_error){
  char* vbuf;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if(!(vbuf = crgetlob(curia, kbuf, ksiz, start, max, sp))){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Curia_error(ecode);
  }
  unlock();
  return vbuf;
}


int Curia::getlobfd(const char *kbuf, int ksiz){
  int fd;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if((fd = crgetlobfd(curia, kbuf, ksiz)) == -1){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return -1;
    throw Curia_error(ecode);
  }
  unlock();
  return fd;
}


int Curia::vsizlob(const char* kbuf, int ksiz) throw(Curia_error){
  int vsiz;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  if((vsiz = crvsizlob(curia, kbuf, ksiz)) == -1){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return -1;
    throw Curia_error(ecode);
  }
  unlock();
  return vsiz;
}


int Curia::rnumlob() throw(Curia_error){
  int rv;
  if(!curia) throw Curia_error();
  if(!lock()) throw Curia_error();
  rv = crrnumlob(curia);
  unlock();
  return rv;
}


void Curia::storerec(const Datum& key, const Datum& val, bool replace) throw(Curia_error){
  if(!put(key.ptr(), key.size(), val.ptr(), val.size(), replace ? CR_DOVER : CR_DKEEP))
    throw Curia_error(EKEEP);
}


void Curia::deleterec(const Datum& key) throw(Curia_error){
  if(!out(key.ptr(), key.size())) throw Curia_error(ENOITEM);
}


Datum Curia::fetchrec(const Datum& key) throw(Curia_error){
  char* vbuf;
  int vsiz;
  vbuf = get(key.ptr(), key.size(), 0, -1, &vsiz);
  if(!vbuf) throw Curia_error(ENOITEM);
  return Datum(vbuf, vsiz, true);
}


Datum Curia::firstkey() throw(Curia_error){
  iterinit();
  return nextkey();
}


Datum Curia::nextkey() throw(Curia_error){
  char* kbuf;
  int ksiz;
  kbuf = iternext(&ksiz);
  if(!kbuf) throw Curia_error(ENOITEM);
  return Datum(kbuf, ksiz, true);
}


bool Curia::error() throw(Curia_error){
  return fatalerror();
}


Curia::Curia(const Curia& curia) throw(Curia_error){
  throw Curia_error();
}


Curia& Curia::operator =(const Curia& curia) throw(Curia_error){
  throw Curia_error();
}


bool Curia::lock(){
  if(dpisreentrant){
    if(pthread_mutex_lock(&mymutex) != 0) return false;
    return true;
  }
  if(pthread_mutex_lock(&ourmutex) != 0) return false;
  return true;
}


void Curia::unlock(){
  if(dpisreentrant){
    pthread_mutex_unlock(&mymutex);
  } else {
    pthread_mutex_unlock(&ourmutex);
  }
}



/* END OF FILE */
