/*************************************************************************************************
 * Implementation of Depot for C++
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


#include "xdepot.h"
#include <new>
#include <cstdlib>
#include <ctime>

extern "C" {
#include <depot.h>
#include <pthread.h>
}

using namespace qdbm;



/*************************************************************************************************
 * Depot_error
 *************************************************************************************************/


Depot_error::Depot_error() throw()
  : DBM_error(){
  ecode = DP_EMISC;
  return;
}


Depot_error::Depot_error(int ecode) throw()
  : DBM_error(){
  this->ecode = ecode;
  return;
}


Depot_error::Depot_error(const Depot_error& de) throw()
  : DBM_error(de){
  ecode = de.ecode;
  return;
}


Depot_error::~Depot_error() throw(){
  return;
}


Depot_error& Depot_error::operator =(const Depot_error& de) throw(){
  this->ecode = de.ecode;
  return *this;
}


Depot_error& Depot_error::operator =(int ecode) throw(){
  this->ecode = ecode;
  return *this;
}


bool Depot_error::operator ==(const Depot_error& de) const throw(){
  return ecode == de.ecode;
}


bool Depot_error::operator !=(const Depot_error& de) const throw(){
  return ecode != de.ecode;
}


bool Depot_error::operator ==(int ecode) const throw(){
  return this->ecode == ecode;
}


bool Depot_error::operator !=(int ecode) const throw(){
  return this->ecode != ecode;
}


Depot_error::operator const char*() const throw(){
  return dperrmsg(ecode);
}


int Depot_error::code() const throw(){
  return ecode;
}


const char* Depot_error::message() const throw(){
  return dperrmsg(ecode);
}



/*************************************************************************************************
 * Depot
 *************************************************************************************************/


const int Depot::ENOERR = DP_ENOERR;
const int Depot::EFATAL = DP_EFATAL;
const int Depot::EMODE = DP_EMODE;
const int Depot::EBROKEN = DP_EBROKEN;
const int Depot::EKEEP = DP_EKEEP;
const int Depot::ENOITEM = DP_ENOITEM;
const int Depot::EALLOC = DP_EALLOC;
const int Depot::EMAP = DP_EMAP;
const int Depot::EOPEN = DP_EOPEN;
const int Depot::ECLOSE = DP_ECLOSE;
const int Depot::ETRUNC = DP_ETRUNC;
const int Depot::ESYNC = DP_ESYNC;
const int Depot::ESTAT = DP_ESTAT;
const int Depot::ESEEK = DP_ESEEK;
const int Depot::EREAD = DP_EREAD;
const int Depot::EWRITE = DP_EWRITE;
const int Depot::ELOCK = DP_ELOCK;
const int Depot::EUNLINK = DP_EUNLINK;
const int Depot::EMKDIR = DP_EMKDIR;
const int Depot::ERMDIR = DP_ERMDIR;
const int Depot::EMISC = DP_EMISC;
const int Depot::OREADER = DP_OREADER;
const int Depot::OWRITER = DP_OWRITER;
const int Depot::OCREAT = DP_OCREAT;
const int Depot::OTRUNC = DP_OTRUNC;
const int Depot::ONOLCK = DP_ONOLCK;
const int Depot::OLCKNB = DP_OLCKNB;
const int Depot::OSPARSE = DP_OSPARSE;
const int Depot::DOVER = DP_DOVER;
const int Depot::DKEEP = DP_DKEEP;
const int Depot::DCAT = DP_DCAT;


const char* Depot::version() throw(){
  return dpversion;
}


void Depot::remove(const char* name) throw(Depot_error){
  if(pthread_mutex_lock(&ourmutex) != 0) throw Depot_error();
  if(!dpremove(name)){
    int ecode = dpecode;
    pthread_mutex_unlock(&ourmutex);
    throw Depot_error(ecode);
  }
  pthread_mutex_unlock(&ourmutex);
}


char* Depot::snaffle(const char* name, const char* kbuf, int ksiz, int* sp) throw(Depot_error){
  char* vbuf;
  if(pthread_mutex_lock(&ourmutex) != 0) throw Depot_error();
  if(!(vbuf = dpsnaffle(name, kbuf, ksiz, sp))){
    int ecode = dpecode;
    pthread_mutex_unlock(&ourmutex);
    throw Depot_error(ecode);
  }
  pthread_mutex_unlock(&ourmutex);
  return vbuf;
}


Depot::Depot(const char* name, int omode, int bnum) throw(Depot_error)
  : ADBM(){
  pthread_mutex_init(&mymutex, NULL);
  silent = false;
  if(!lock()) throw Depot_error();
  if(!(depot = dpopen(name, omode, bnum))){
    int ecode = dpecode;
    unlock();
    throw Depot_error(ecode);
  }
  unlock();
}


Depot::~Depot() throw(){
  if(!depot){
    pthread_mutex_destroy(&mymutex);
    return;
  }
  if(lock()){
    dpclose(depot);
    unlock();
  }
  depot = 0;
  pthread_mutex_destroy(&mymutex);
}


void Depot::close() throw(Depot_error){
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if(!dpclose(depot)){
    int ecode = dpecode;
    depot = 0;
    unlock();
    throw Depot_error(ecode);
  }
  depot = 0;
  unlock();
}


bool Depot::put(const char* kbuf, int ksiz, const char* vbuf, int vsiz, int dmode)
  throw(Depot_error){
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if(!dpput(depot, kbuf, ksiz, vbuf, vsiz, dmode)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == EKEEP) return false;
    throw Depot_error(ecode);
  }
  unlock();
  return true;
}


bool Depot::out(const char* kbuf, int ksiz) throw(Depot_error){
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if(!dpout(depot, kbuf, ksiz)){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return false;
    throw Depot_error(ecode);
  }
  unlock();
  return true;
}


char* Depot::get(const char* kbuf, int ksiz, int start, int max, int* sp) throw(Depot_error){
  char* vbuf;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if(!(vbuf = dpget(depot, kbuf, ksiz, start, max, sp))){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Depot_error(ecode);
  }
  unlock();
  return vbuf;
}


int Depot::getwb(const char *kbuf, int ksiz, int start, int max, char *vbuf) throw(Depot_error){
  int vsiz;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if((vsiz = dpgetwb(depot, kbuf, ksiz, start, max, vbuf)) == -1){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return -1;
    throw Depot_error(ecode);
  }
  unlock();
  return vsiz;
}


int Depot::vsiz(const char* kbuf, int ksiz) throw(Depot_error){
  int vsiz;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if((vsiz = dpvsiz(depot, kbuf, ksiz)) == -1){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return -1;
    throw Depot_error(ecode);
  }
  unlock();
  return vsiz;
}


void Depot::iterinit() throw(Depot_error){
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if(!dpiterinit(depot)){
    int ecode = dpecode;
    unlock();
    throw Depot_error(ecode);
  }
  unlock();
}


char* Depot::iternext(int* sp) throw(Depot_error){
  char* vbuf;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if(!(vbuf = dpiternext(depot, sp))){
    int ecode = dpecode;
    unlock();
    if(silent && ecode == ENOITEM) return 0;
    throw Depot_error(ecode);
  }
  unlock();
  return vbuf;
}


void Depot::setalign(int align) throw(Depot_error){
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if(!dpsetalign(depot, align)){
    int ecode = dpecode;
    unlock();
    throw Depot_error(ecode);
  }
  unlock();
}


void Depot::setfbpsiz(int size) throw(Depot_error){
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if(!dpsetfbpsiz(depot, size)){
    int ecode = dpecode;
    unlock();
    throw Depot_error(ecode);
  }
  unlock();
}


void Depot::sync() throw(Depot_error){
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if(!dpsync(depot)){
    int ecode = dpecode;
    unlock();
    throw Depot_error(ecode);
  }
  unlock();
}


void Depot::optimize(int bnum) throw(Depot_error){
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if(!dpoptimize(depot, bnum)){
    int ecode = dpecode;
    unlock();
    throw Depot_error(ecode);
  }
  unlock();
}


char* Depot::name() throw(Depot_error){
  char* buf;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if(!(buf = dpname(depot))){
    int ecode = dpecode;
    unlock();
    throw Depot_error(ecode);
  }
  unlock();
  return buf;
}


int Depot::fsiz() throw(Depot_error){
  int rv;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if((rv = dpfsiz(depot)) == -1){
    int ecode = dpecode;
    unlock();
    throw Depot_error(ecode);
  }
  unlock();
  return rv;
}


int Depot::bnum() throw(Depot_error){
  int rv;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if((rv = dpbnum(depot)) == -1){
    int ecode = dpecode;
    unlock();
    throw Depot_error(ecode);
  }
  unlock();
  return rv;
}


int Depot::busenum() throw(Depot_error){
  int rv;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if((rv = dpbusenum(depot)) == -1){
    int ecode = dpecode;
    unlock();
    throw Depot_error(ecode);
  }
  unlock();
  return rv;
}


int Depot::rnum() throw(Depot_error){
  int rv;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  if((rv = dprnum(depot)) == -1){
    int ecode = dpecode;
    unlock();
    throw Depot_error(ecode);
  }
  unlock();
  return rv;
}


bool Depot::writable() throw(Depot_error){
  int rv;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  rv = dpwritable(depot);
  unlock();
  return rv ? true : false;
}


bool Depot::fatalerror() throw(Depot_error){
  int rv;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  rv = dpfatalerror(depot);
  unlock();
  return rv ? true : false;
}


int Depot::inode() throw(Depot_error){
  int rv;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  rv = dpinode(depot);
  unlock();
  return rv;
}


time_t Depot::mtime() throw(Depot_error){
  time_t rv;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  rv = dpmtime(depot);
  unlock();
  return rv;
}


int Depot::fdesc() throw(Depot_error){
  int rv;
  if(!depot) throw Depot_error();
  if(!lock()) throw Depot_error();
  rv = dpfdesc(depot);
  unlock();
  return rv;
}


void Depot::storerec(const Datum& key, const Datum& val, bool replace) throw(Depot_error){
  if(!put(key.ptr(), key.size(), val.ptr(), val.size(), replace ? DP_DOVER : DP_DKEEP))
    throw Depot_error(EKEEP);
}


void Depot::deleterec(const Datum& key) throw(Depot_error){
  if(!out(key.ptr(), key.size())) throw Depot_error(ENOITEM);
}


Datum Depot::fetchrec(const Datum& key) throw(Depot_error){
  char* vbuf;
  int vsiz;
  vbuf = get(key.ptr(), key.size(), 0, -1, &vsiz);
  if(!vbuf) throw Depot_error(ENOITEM);
  return Datum(vbuf, vsiz, true);
}


Datum Depot::firstkey() throw(Depot_error){
  iterinit();
  return nextkey();
}


Datum Depot::nextkey() throw(Depot_error){
  char* kbuf;
  int ksiz;
  kbuf = iternext(&ksiz);
  if(!kbuf) throw Depot_error(ENOITEM);
  return Datum(kbuf, ksiz, true);
}


bool Depot::error() throw(Depot_error){
  return fatalerror();
}


Depot::Depot(const Depot& depot) throw(Depot_error){
  throw Depot_error();
}


Depot& Depot::operator =(const Depot& depot) throw(Depot_error){
  throw Depot_error();
}


bool Depot::lock(){
  if(dpisreentrant){
    if(pthread_mutex_lock(&mymutex) != 0) return false;
    return true;
  }
  if(pthread_mutex_lock(&ourmutex) != 0) return false;
  return true;
}


void Depot::unlock(){
  if(dpisreentrant){
    pthread_mutex_unlock(&mymutex);
  } else {
    pthread_mutex_unlock(&ourmutex);
  }
}



/* END OF FILE */
