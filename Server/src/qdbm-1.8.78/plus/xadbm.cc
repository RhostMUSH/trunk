/*************************************************************************************************
 * Abstraction for database managers compatible with DBM
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


#include "xadbm.h"
#include <new>
#include <cstdlib>
#include <cstring>

using namespace qdbm;



/*************************************************************************************************
 * private objects
 *************************************************************************************************/


namespace {
  char *xmalloc(size_t size) throw(std::bad_alloc);
  char *xrealloc(void *ptr, size_t size) throw(std::bad_alloc);
  char *xmalloc(size_t size) throw(std::bad_alloc){
    char *p;
    if(!(p = (char*)std::malloc(size))) throw std::bad_alloc();
    return p;
  }
  char *xrealloc(void *ptr, size_t size) throw(std::bad_alloc){
    char *p;
    if(!(p = (char*)std::realloc(ptr, size))) throw std::bad_alloc();
    return p;
  }
}



/*************************************************************************************************
 * DBM_error
 *************************************************************************************************/


DBM_error::DBM_error() throw(){
  errmsg = 0;
}


DBM_error::DBM_error(const char* message) throw(){
  int len = std::strlen(message);
  this->errmsg = xmalloc(len + 1);
  std::memcpy(this->errmsg, message, len);
  this->errmsg[len] = '\0';
}


DBM_error::~DBM_error() throw(){
  if(errmsg) std::free(errmsg);
}


DBM_error::operator const char*() const throw(){
  if(!errmsg) return "database error";
  return errmsg;
}


const char* DBM_error::message() const throw(){
  if(!errmsg) return "database error";
  return errmsg;
}



/*************************************************************************************************
 * Datum
 *************************************************************************************************/


Datum::Datum(const char* dptr, int dsize){
  if(dsize < 0) dsize = std::strlen(dptr);
  this->dptr = xmalloc(dsize + 1);
  std::memcpy(this->dptr, dptr, dsize);
  this->dptr[dsize] = '\0';
  this->dsize = dsize;
}


Datum::Datum(int num){
  this->dptr = xmalloc(sizeof(num));
  std::memcpy(this->dptr, &num, sizeof(num));
  this->dsize = sizeof(num);
}


Datum::Datum(const Datum& datum){
  this->dptr = xmalloc(datum.dsize + 1);
  std::memcpy(this->dptr, datum.dptr, datum.dsize);
  this->dptr[datum.dsize] = '\0';
  this->dsize = datum.dsize;
}


Datum::~Datum() throw(){
  std::free(dptr);
}


Datum& Datum::operator =(const Datum& datum){
  if(this == &datum) return *this;
  std::free(this->dptr);
  this->dptr = xmalloc(datum.dsize + 1);
  std::memcpy(this->dptr, datum.dptr, datum.dsize);
  this->dptr[datum.dsize] = '\0';
  this->dsize = datum.dsize;
  return *this;
}


Datum& Datum::operator =(const char* str){
  std::free(this->dptr);
  dsize = std::strlen(str);
  dptr = xmalloc(dsize + 1);
  std::memcpy(dptr, str, dsize);
  dptr[dsize] = '\0';
  return *this;
}


Datum& Datum::operator <<(const Datum& datum){
  dptr = xrealloc(dptr, dsize + datum.dsize + 1);
  std::memcpy(dptr + dsize, datum.dptr, datum.dsize);
  dsize += datum.dsize;
  dptr[dsize] = '\0';
  return *this;
}


Datum& Datum::operator <<(const char* str){
  int len = strlen(str);
  dptr = xrealloc(dptr, dsize + len + 1);
  std::memcpy(dptr + dsize, str, len);
  dsize += len;
  dptr[dsize] = '\0';
  return *this;
}


bool Datum::operator ==(const Datum& datum) const {
  if(dsize != datum.dsize) return false;
  return std::memcmp(dptr, datum.dptr, dsize) == 0 ? true : false;
}


bool Datum::operator !=(const Datum& datum) const {
  if(dsize != datum.dsize) return true;
  return std::memcmp(dptr, datum.dptr, dsize) == 0 ? false : true;
}


bool Datum::operator ==(const char* str) const {
  int len = strlen(str);
  if(dsize != len) return false;
  return std::memcmp(dptr, str, len) == 0 ? true : false;
}


bool Datum::operator !=(const char* str) const {
  int len = strlen(str);
  if(dsize != len) return true;
  return std::memcmp(dptr, str, len) == 0 ? false : true;
}


char& Datum::operator [](int idx) const {
  return dptr[idx];
}


Datum::operator const char*() const {
  return dptr;
}


const char* Datum::ptr() const {
  return dptr;
}


int Datum::size() const {
  return dsize;
}


Datum::Datum(char* dptr, int dsize, bool dummy){
  this->dptr = xrealloc(dptr, dsize + 1);
  this->dptr[dsize] = '\0';
  this->dsize = dsize;
}


Datum qdbm::operator +(const Datum& former, const Datum& latter){
  int len = former.dsize + latter.dsize;
  char *buf = xmalloc(former.dsize + latter.dsize + 1);
  std::memcpy(buf, former.dptr, former.dsize);
  std::memcpy(buf + former.dsize, latter.dptr, latter.dsize);
  buf[len] = '\0';
  return Datum(buf, len, true);
}


Datum qdbm::operator +(const Datum& datum, const char* str){
  int len = strlen(str);
  char* buf = xmalloc(datum.dsize + len + 1);
  std::memcpy(buf, datum.dptr, datum.dsize);
  std::memcpy(buf + datum.dsize, str, len);
  len += datum.dsize;
  buf[len] = '\0';
  return Datum(buf, len, true);
}


Datum qdbm::operator +(const char* str, const Datum& datum){
  int len = strlen(str);
  char* buf = xmalloc(len + datum.dsize + 1);
  std::memcpy(buf, str, len);
  std::memcpy(buf + len, datum.dptr, datum.dsize);
  len += datum.dsize;
  buf[len] = '\0';
  return Datum(buf, len, true);
}



/* END OF FILE */
