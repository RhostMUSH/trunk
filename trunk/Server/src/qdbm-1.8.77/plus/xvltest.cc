/*************************************************************************************************
 * Test cases of Villa for C++
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


#include <xvilla.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <exception>
#include <iostream>
#include <iomanip>

extern "C" {
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
}

using namespace std;
using namespace qdbm;


/* global constants */
const int RECBUFSIZ = 32;
const int LRECMAX = 25;
const int NIDXMAX = 64;
const int LCNUM = 32;
const int NCNUM = 32;


/* struct for closure per thread */
struct Mission {
  Villa* villa;
  int rnum;
  int id;
  bool printer;
};


/* global variables */
const char* progname;


/* function prototypes */
int main(int argc, char** argv);
void myterminate();
void myunexpected();
void usage();
int runwrite(int argc, char** argv);
int runread(int argc, char** argv);
int runmulti(int argc, char** argv);
int runmisc(int argc, char** argv);
void pxdperror(const char* name, DBM_error &e);
int myrand(void);
int dowrite(const char* name, int rnum);
int doread(const char* name);
int domulti(const char* name, int rnum, int tnum);
void* mtwriter(void* arg);
int domisc(const char* name);


/* main routine */
int main(int argc, char** argv){
  int rv;
  set_terminate(myterminate);
  set_unexpected(myunexpected);
  progname = argv[0];
  if(argc < 2) usage();
  rv = 0;
  if(!strcmp(argv[1], "write")){
    rv = runwrite(argc, argv);
  } else if(!strcmp(argv[1], "read")){
    rv = runread(argc, argv);
  } else if(!strcmp(argv[1], "multi")){
    rv = runmulti(argc, argv);
  } else if(!strcmp(argv[1], "misc")){
    rv = runmisc(argc, argv);
  } else {
    usage();
  }
  return rv;
}


/* for debug */
void myterminate(){
  cout << progname << ": terminate" << endl;
  exit(1);
}


/* for debug */
void myunexpected(){
  cout << progname << ": unexpected" << endl;
  exit(1);
}


/* print the usage and exit */
void usage(){
  cerr << progname << ": test cases for Villa for C++" << endl;
  cerr << endl;
  cerr << "usage:" << endl;
  cerr << "  " << progname << " write name rnum" << endl;
  cerr << "  " << progname << " read name" << endl;
  cerr << "  " << progname << " multi name rnum tnum" << endl;
  cerr << "  " << progname << " misc name" << endl;
  cerr << endl;
  exit(1);
}


/* parse arguments of write command */
int runwrite(int argc, char** argv){
  char *name, *rstr;
  int i, rnum, rv;
  name = 0;
  rstr = 0;
  for(i = 2; i < argc; i++){
    if(argv[i][0] == '-'){
      usage();
    } else if(!name){
      name = argv[i];
    } else if(!rstr){
      rstr = argv[i];
    } else {
      usage();
    }
  }
  if(!name || !rstr) usage();
  rnum = atoi(rstr);
  if(rnum < 1) usage();
  rv = dowrite(name, rnum);
  return rv;
}


/* parse arguments of read command */
int runread(int argc, char** argv){
  char* name;
  int i, rv;
  name = 0;
  for(i = 2; i < argc; i++){
    if(argv[i][0] == '-'){
      usage();
    } else if(!name){
      name = argv[i];
    } else {
      usage();
    }
  }
  if(!name) usage();
  rv = doread(name);
  return rv;
}


/* parse arguments of multi command */
int runmulti(int argc, char** argv){
  char *name, *rstr, *tstr;
  int i, rnum, tnum, rv;
  name = 0;
  rstr = 0;
  tstr = 0;
  for(i = 2; i < argc; i++){
    if(argv[i][0] == '-'){
      usage();
    } else if(!name){
      name = argv[i];
    } else if(!rstr){
      rstr = argv[i];
    } else if(!tstr){
      tstr = argv[i];
    } else {
      usage();
    }
  }
  if(!name || !rstr || !tstr) usage();
  rnum = atoi(rstr);
  tnum = atoi(tstr);
  if(rnum < 1 || tnum < 1) usage();
  rv = domulti(name, rnum, tnum);
  return rv;
}


/* parse arguments of misc command */
int runmisc(int argc, char** argv){
  char* name;
  int i, rv;
  name = 0;
  for(i = 2; i < argc; i++){
    if(argv[i][0] == '-'){
      usage();
    } else if(!name){
      name = argv[i];
    } else {
      usage();
    }
  }
  if(!name) usage();
  rv = domisc(name);
  return rv;
}


/* print an error message */
void pxdperror(const char* name, DBM_error &e){
  cerr << progname << ": " << name << ": " << e << endl;
}


/* pseudo random number generator */
int myrand(void){
  static int cnt = 0;
  if(cnt == 0) std::srand(std::time(NULL));
  return (std::rand() * std::rand() + (std::rand() >> (sizeof(int) * 4)) + (cnt++)) & 0x7FFFFFFF;
}


/* do writing test */
int dowrite(const char* name, int rnum){
  int i, len;
  char buf[RECBUFSIZ];
  cout << "<Writing Test>" << endl;
  cout << "  name=" << name << "  rnum=" << rnum << endl;
  cout << endl;
  Villa* villa = 0;
  bool err = false;
  try {
    // open the database
    villa = new Villa(name, Villa::OWRITER | Villa::OCREAT | Villa::OTRUNC);
    // roop for storing
    for(i = 1; i <= rnum; i++){
      // set a buffer for a key and a value
      len = sprintf(buf, "%08d", i);
      // store a record into the database
      villa->put(buf, len, buf, len);
      // print progression
      if(rnum > 250 && i % (rnum / 250) == 0){
        cout << '.';
        cout.flush();
        if(i == rnum || i % (rnum / 10) == 0){
          cout << " (" << setw(8) << setfill('0') << i << ")" << endl;
          cout.flush();
        }
      }
    }
    // close the database
    villa->close();
    cout << "ok" << endl << endl;
  } catch(Villa_error& e){
    err = true;
    // report an error
    pxdperror(name, e);
  }
  // destroy instance
  if(villa) delete villa;
  return err ? 1 : 0;
}


/* do reading test */
int doread(const char* name){
  int i, rnum, len;
  char buf[RECBUFSIZ], *val;
  cout << "<Reading Test>" << endl;
  cout << "  name=" << name << endl;
  cout << endl;
  Villa* villa = 0;
  bool err = false;
  try {
    // open the database
    villa = new Villa(name);
    // get the number of records
    rnum = villa->rnum();
    // roop for retrieving
    for(i = 1; i <= rnum; i++){
      // set a buffer for a key
      len = sprintf(buf, "%08d", i);
      // retrieve a record from the database
      val = villa->get(buf, len);
      free(val);
      // print progression
      if(rnum > 250 && i % (rnum / 250) == 0){
        cout << '.';
        cout.flush();
        if(i == rnum || i % (rnum / 10) == 0){
          cout << " (" << setw(8) << setfill('0') << i << ")" << endl;
          cout.flush();
        }
      }
    }
    // close the database
    villa->close();
    cout << "ok" << endl << endl;
  } catch(Villa_error& e){
    err = true;
    // report an error
    pxdperror(name, e);
  }
  // destroy instance
  if(villa) delete villa;
  return err ? 1 : 0;
}


/* do multi thread test */
int domulti(const char* name, int rnum, int tnum){
  Villa* villa;
  pthread_t* tids;
  bool* tsts;
  Mission* mss;
  Villa_error* ep;
  int i, len, size, vnum;
  bool err;
  char buf[RECBUFSIZ], *val;
  cout << "<Multi-thread Test>" << endl;
  cout << "  name=" << name << "  rnum=" << rnum << "  tnum=" << tnum << endl;
  cout << endl;
  // open the database
  villa = 0;
  try {
    cout << "Creating a database ... ";
    villa = new Villa(name, Villa::OWRITER | Villa::OCREAT | Villa::OTRUNC);
    villa->settuning(LRECMAX, NIDXMAX, LCNUM, NCNUM);
    cout << "ok" << endl;
  } catch(Villa_error& e){
    if(villa) delete villa;
    pxdperror(name, e);
    return 1;
  }
  // prepare threads
  tids = new pthread_t[tnum];
  tsts = new bool[tnum];
  mss = new Mission[tnum];
  err = false;
  // roop for each threads
  cout << "Writing" << endl;
  for(i = 0; i < tnum; i++){
    mss[i].villa = villa;
    mss[i].rnum = rnum;
    mss[i].id = i;
    mss[i].printer = (i == tnum / 2);
    if(pthread_create(tids + i, 0, mtwriter, (void*)(mss + i)) == 0){
      tsts[i] = true;
    } else {
      tsts[i] = false;
      err = true;
      cerr << progname << ": " << name << ": " << "pthread error" << endl;
    }
  }
  // join every threads
  for(i = 0; i < tnum; i++){
    if(tsts[i]){
      if(pthread_join(tids[i], (void**)&ep) != 0){
        err = true;
        cerr << progname << ": " << name << ": " << "pthread error" << endl;
      }
    }
    if(ep){
      err = true;
      pxdperror(name, *ep);
      delete ep;
    }
  }
  delete[] mss;
  delete[] tsts;
  delete[] tids;
  if(!err) cout << "ok" << endl;
  cout.flush();
  // check every record
  cout << "Validation checking ... ";
  try {
    for(i = 1; i <= rnum; i++){
      len = sprintf(buf, "%08d", i);
      val = villa->get(buf, len, &size);
      free(val);
      if(size != 1 || villa->vsiz(buf, len) != 1){
        cerr << progname << ": " << name << ": " << "size error: " << size << endl;
        err = true;
      }
      vnum = villa->vnum(buf, len);
      if(vnum != tnum){
        cerr << progname << ": " << name << ": " << "vnum error: " << vnum << endl;
        err = true;
      }
    }
  } catch(Villa_error& e){
    pxdperror(name, e);
    err = true;
  }
  if(!err) cout << "ok" << endl;
  // close the database
  cout << "Closing the database ... ";
  try {
    villa->close();
  } catch(Villa_error& e){
    pxdperror(name, e);
    delete villa;
    return 1;
  }
  cout << "ok" << endl;
  if(!err) cout << "all ok" << endl << endl;
  delete villa;
  return err ? 1 : 0;
}


/* writer with each thread */
void* mtwriter(void* arg){
  Mission* mp = (Mission*)arg;
  Villa* villa = mp->villa;
  int rnum = mp->rnum;
  int id = mp->id;
  bool printer = mp->printer;
  char buf[RECBUFSIZ];
  void* mc;
  int i, len;
  // roop for storing
  for(i = 1; i <= rnum; i++){
    len = sprintf(buf, "%08d", i);
    // store a record
    bool tran = false;
    try {
      if(myrand() % (mp->rnum / 7 + 1) == 0){
        villa->tranbegin();
        tran = true;
      }
      villa->put(buf, len, id % 2 ? "*" : "=", 1, Villa::DDUP);
    } catch(Villa_error& e){
      if(tran) villa->tranabort();
      return new Villa_error(e);
    }
    if(tran) villa->trancommit();
    // print progression
    if(printer && rnum > 250 && i % (rnum / 250) == 0){
      cout << '.';
      cout.flush();
      if(i == rnum || i % (rnum / 10) == 0){
        cout << " (" << setw(8) << setfill('0') << i << ")" << endl;
        cout.flush();
      }
    }
    // malloc reentrant check
    if((mc = (char*)malloc(1)) != 0){
      free(mc);
    } else {
      return new Villa_error(Villa::EALLOC);
    }
  }
  return 0;
}


/* do combination test */
int domisc(const char* name){
  bool err;
  int i, len;
  char buf[RECBUFSIZ], *tmp;
  cout << "<Miscellaneous Test>" << endl;
  cout << "  name=" << name << endl;
  cout << endl;
  err = false;
  ADBM* adbm = 0;
  try {
    // open the database
    cout << "Creating a database ... ";
    adbm = new Villa(name, Villa::OWRITER | Villa::OCREAT | Villa::OTRUNC);
    cout << "ok" << endl;
    // write with datadase abstraction
    cout << "Writing with database abstraction ... ";
    for(i = 1; i <= 100; i++){
      len = sprintf(buf, "%08d", i);
      Datum key(buf, len);
      Datum val(buf, len);
      adbm->storerec(key, val, true);
    }
    cout << "ok" << endl;
    // read with datadase abstraction
    cout << "Reading with database abstraction ... ";
    for(i = 1; i <= 100; i++){
      len = sprintf(buf, "%08d", i);
      Datum key(buf, len);
      const Datum& val = adbm->fetchrec(key);
      len = val.size();
      if(len != 8) throw DBM_error("size error");
    }
    cout << "ok" << endl;
    // traversal access with database abstraction
    cout << "Traversal access with database abstraction ... ";
    i = 0;
    try {
      for(adbm->firstkey(); true; adbm->nextkey()){
        i++;
      }
    } catch(DBM_error& e){
      if(adbm->error()) throw;
    }
    if(i != 100) throw DBM_error("iteration error");
    cout << "ok" << endl;
    // close the database
    cout << "Closing the database ... ";
    adbm->close();
    cout << "ok" << endl;
  } catch(DBM_error& e){
    err = true;
    pxdperror(name, e);
  }
  if(adbm) delete adbm;
  if(err) return 1;
  Villa* villa = 0;
  try {
    // open the database
    cout << "Opening the database ... ";
    villa = new Villa(name);
    cout << "ok" << endl;
    // traversal access
    cout << "Traversal access ... ";
    try {
      villa->curfirst();
      for(i = 0; true; i++){
        villa->curnext();
      }
      if(i != 100) throw Villa_error();
    } catch(Villa_error& e){
      if(e != Villa::ENOITEM) throw;
    }
    cout << "ok" << endl;
    // checking information
    cout << "Checking information" << endl;
    cout << "  - fsiz ... " << villa->fsiz() << endl;
    cout << "  - lnum ... " << villa->lnum() << endl;
    cout << "  - nnum ... " << villa->nnum() << endl;
    cout << "  - rnum ... " << villa->rnum() << endl;
    cout << "  - writable ... " << villa->writable() << endl;
    cout << "  - fatalerror ... " << villa->fatalerror() << endl;
    cout << "  - inode ... " << villa->inode() << endl;
    cout << "  - mtime ... " << villa->mtime() << endl;
    cout << "ok" << endl;
    // close the database
    cout << "Closing the database ... ";
    villa->close();
    cout << "ok" << endl;
  } catch(Villa_error& e){
    err = true;
    pxdperror(name, e);
  }
  if(villa) delete villa;
  if(err) return 1;
  villa = 0;
  try {
    // open the database
    cout << "Opening the database ... ";
    villa = new Villa(name, Villa::OWRITER);
    villa->silent = true;
    cout << "ok" << endl;
    // sync the database
    cout << "Syncing the database ... ";
    villa->sync();
    cout << "ok" << endl;
    // optimize the database
    cout << "Optimizing the database ... ";
    villa->optimize();
    cout << "ok" << endl;
    // write in silent mode
    cout << "Writing in silent mode ... ";
    for(i = 1; i <= 100; i++){
      len = sprintf(buf, "%d", rand() % 100);
      villa->put(buf, len, buf, len, Villa::DKEEP);
    }
    cout << "ok" << endl;
    // read in silent mode
    cout << "Reading in silent mode ... ";
    for(i = 1; i <= 100; i++){
      len = sprintf(buf, "%d", rand() % 100);
      if((tmp = villa->get(buf, len)) != 0) free(tmp);
    }
    cout << "ok" << endl;
    // traversal access
    cout << "Traversal access in silent mode ... ";
    villa->curfirst();
    while((tmp = villa->curkey()) != 0){
      free(tmp);
      villa->curnext();
    }
    cout << "ok" << endl;
    // close the database
    cout << "Closing the database ... ";
    villa->close();
    cout << "ok" << endl;
  } catch(Villa_error& e){
    err = true;
    pxdperror(name, e);
  }
  if(villa) delete villa;
  if(err) return 1;
  villa = 0;
  try {
    // open the database
    cout << "Opening the database with leaves compressed ... ";
    Villa zvilla(name, Villa::OWRITER | Villa::OCREAT | Villa::OTRUNC | Villa::OZCOMP);
    cout << "ok" << endl;
    // write records
    cout << "Writing records ... ";
    for(i = 1; i <= 100; i++){
      len = sprintf(buf, "%08d", i);
      zvilla.put(buf, len, buf,len, Villa::DCAT);
      zvilla.put(buf, len, buf,len, Villa::DDUP);
      zvilla.put(buf, len, buf,len, Villa::DCAT);
    }
    cout << "ok" << endl;
    // read records
    cout << "Checking records ... ";
    for(i = 1; i <= 100; i++){
      len = sprintf(buf, "%08d", i);
      if(zvilla.vnum(buf, len) != 2) throw Villa_error();
    }
    cout << "ok" << endl;
    // cursor operations
    cout << "Cursor operations ... ";
    zvilla.curjump("00000010", -1, Villa::JFORWARD);
    zvilla.curput("CURRENT", -1, Villa::CPCURRENT);
    zvilla.curput("BEFORE", -1, Villa::CPBEFORE);
    zvilla.curput("AFTER", -1, Villa::CPAFTER);
    zvilla.curnext();
    zvilla.curout();
    zvilla.curput("OMIT", -1);
    cout << "ok" << endl;
    // close the database
    cout << "Closing the database ... ";
    zvilla.close();
    cout << "ok" << endl;
  } catch(Villa_error& e){
    err = true;
    pxdperror(name, e);
  }
  if(err) return 1;
  cout << "all ok" << endl << endl;
  return 0;
}



/* END OF FILE */
