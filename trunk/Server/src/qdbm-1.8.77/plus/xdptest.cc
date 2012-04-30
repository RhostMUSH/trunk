/*************************************************************************************************
 * Test cases of Depot for C++
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


#include <xdepot.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
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
const int ALIGNSIZ = 8;
const int FBPSIZ = 8;


/* struct for closure per thread */
struct Mission {
  Depot* depot;
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
int dowrite(const char* name, int rnum, int bnum);
int doread(const char* name);
int domulti(const char* name, int rnum, int bnum, int tnum);
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
  cerr << progname << ": test cases for Depot for C++" << endl;
  cerr << endl;
  cerr << "usage:" << endl;
  cerr << "  " << progname << " write name rnum bnum" << endl;
  cerr << "  " << progname << " read name" << endl;
  cerr << "  " << progname << " multi name rnum bnum tnum" << endl;
  cerr << "  " << progname << " misc name" << endl;
  cerr << endl;
  exit(1);
}


/* parse arguments of write command */
int runwrite(int argc, char** argv){
  char *name, *rstr, *bstr;
  int i, rnum, bnum, rv;
  name = 0;
  rstr = 0;
  bstr = 0;
  for(i = 2; i < argc; i++){
    if(argv[i][0] == '-'){
      usage();
    } else if(!name){
      name = argv[i];
    } else if(!rstr){
      rstr = argv[i];
    } else if(!bstr){
      bstr = argv[i];
    } else {
      usage();
    }
  }
  if(!name || !rstr || !bstr) usage();
  rnum = atoi(rstr);
  bnum = atoi(bstr);
  if(rnum < 1 || bnum < 1) usage();
  rv = dowrite(name, rnum, bnum);
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
  char *name, *rstr, *bstr, *tstr;
  int i, rnum, bnum, tnum, rv;
  name = 0;
  rstr = 0;
  bstr = 0;
  tstr = 0;
  for(i = 2; i < argc; i++){
    if(argv[i][0] == '-'){
      usage();
    } else if(!name){
      name = argv[i];
    } else if(!rstr){
      rstr = argv[i];
    } else if(!bstr){
      bstr = argv[i];
    } else if(!tstr){
      tstr = argv[i];
    } else {
      usage();
    }
  }
  if(!name || !rstr || !bstr || !tstr) usage();
  rnum = atoi(rstr);
  bnum = atoi(bstr);
  tnum = atoi(tstr);
  if(rnum < 1 || bnum < 1 || tnum < 1) usage();
  rv = domulti(name, rnum, bnum, tnum);
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


/* do writing test */
int dowrite(const char* name, int rnum, int bnum){
  int i, len;
  char buf[RECBUFSIZ];
  cout << "<Writing Test>" << endl;
  cout << "  name=" << name << "  rnum=" << rnum << "  bnum=" << bnum << endl;
  cout << endl;
  Depot* depot = 0;
  bool err = false;
  try {
    // open the database
    depot = new Depot(name, Depot::OWRITER | Depot::OCREAT | Depot::OTRUNC, bnum);
    // roop for storing
    for(i = 1; i <= rnum; i++){
      // set a buffer for a key and a value
      len = sprintf(buf, "%08d", i);
      // store a record into the database
      depot->put(buf, len, buf, len);
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
    depot->close();
    cout << "ok" << endl << endl;
  } catch(Depot_error& e){
    err = true;
    // report an error
    pxdperror(name, e);
  }
  // destroy instance
  if(depot) delete depot;
  return err ? 1 : 0;
}


/* do reading test */
int doread(const char* name){
  int i, rnum, len;
  char buf[RECBUFSIZ], *val;
  cout << "<Reading Test>" << endl;
  cout << "  name=" << name << endl;
  cout << endl;
  Depot* depot = 0;
  bool err = false;
  try {
    // open the database
    depot = new Depot(name);
    // get the number of records
    rnum = depot->rnum();
    // roop for retrieving
    for(i = 1; i <= rnum; i++){
      // set a buffer for a key
      len = sprintf(buf, "%08d", i);
      // retrieve a record from the database
      val = depot->get(buf, len);
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
    depot->close();
    cout << "ok" << endl << endl;
  } catch(Depot_error& e){
    err = true;
    // report an error
    pxdperror(name, e);
  }
  // destroy instance
  if(depot) delete depot;
  return err ? 1 : 0;
}


/* do multi thread test */
int domulti(const char* name, int rnum, int bnum, int tnum){
  Depot* depot;
  pthread_t* tids;
  bool* tsts;
  Mission* mss;
  Depot_error* ep;
  int i, len, size;
  bool err;
  char buf[RECBUFSIZ], *val;
  cout << "<Multi-thread Test>" << endl;
  cout << "  name=" << name << "  rnum=" << rnum << "  bnum=" << bnum << "  tnum=" << tnum << endl;
  cout << endl;
  // open the database
  depot = 0;
  try {
    cout << "Creating a database ... ";
    depot = new Depot(name, Depot::OWRITER | Depot::OCREAT | Depot::OTRUNC, bnum);
    depot->setalign(ALIGNSIZ);
    depot->setfbpsiz(FBPSIZ);
    cout << "ok" << endl;
  } catch(Depot_error& e){
    if(depot) delete depot;
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
    mss[i].depot = depot;
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
      val = depot->get(buf, len, 0, -1, &size);
      free(val);
      if(size != tnum){
        cerr << progname << ": " << name << ": " << "size error: " << size << endl;
        err = true;
      }
    }
  } catch(Depot_error& e){
    pxdperror(name, e);
    err = true;
  }
  if(!err) cout << "ok" << endl;
  // close the database
  cout << "Closing the database ... ";
  try {
    depot->close();
  } catch(Depot_error& e){
    pxdperror(name, e);
    delete depot;
    return 1;
  }
  cout << "ok" << endl;
  if(!err) cout << "all ok" << endl << endl;
  delete depot;
  return err ? 1 : 0;
}


/* writer with each thread */
void* mtwriter(void* arg){
  Mission* mp = (Mission*)arg;
  Depot* depot = mp->depot;
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
    try {
      depot->put(buf, len, id % 2 ? "*" : "=", 1, Depot::DCAT);
    } catch(Depot_error& e){
      return new Depot_error(e);
    }
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
      return new Depot_error(Depot::EALLOC);
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
    adbm = new Depot(name, Depot::OWRITER | Depot::OCREAT | Depot::OTRUNC);
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
  Depot* depot = 0;
  try {
    // open the database
    cout << "Opening the database ... ";
    depot = new Depot(name);
    cout << "ok" << endl;
    // traversal access
    cout << "Traversal access ... ";
    try {
      depot->iterinit();
      for(i = 0; true; i++){
        free(depot->iternext(0));
      }
      if(i != 100) throw Depot_error();
    } catch(Depot_error& e){
      if(e != Depot::ENOITEM) throw;
    }
    cout << "ok" << endl;
    // checking information
    cout << "Checking information" << endl;
    cout << "  - fsiz ... " << depot->fsiz() << endl;
    cout << "  - bnum ... " << depot->bnum() << endl;
    cout << "  - busenum ... " << depot->busenum() << endl;
    cout << "  - rnum ... " << depot->rnum() << endl;
    cout << "  - writable ... " << depot->writable() << endl;
    cout << "  - fatalerror ... " << depot->fatalerror() << endl;
    cout << "  - inode ... " << depot->inode() << endl;
    cout << "  - mtime ... " << depot->mtime() << endl;
    cout << "ok" << endl;
    // close the database
    cout << "Closing the database ... ";
    depot->close();
    cout << "ok" << endl;
  } catch(Depot_error& e){
    err = true;
    pxdperror(name, e);
  }
  if(depot) delete depot;
  if(err) return 1;
  depot = 0;
  try {
    // open the database
    cout << "Opening the database ... ";
    depot = new Depot(name, Depot::OWRITER);
    depot->silent = true;
    cout << "ok" << endl;
    // sync the database
    cout << "Syncing the database ... ";
    depot->sync();
    cout << "ok" << endl;
    // optimize the database
    cout << "Optimizing the database ... ";
    depot->optimize();
    cout << "ok" << endl;
    // write in silent mode
    cout << "Writing in silent mode ... ";
    for(i = 1; i <= 100; i++){
      len = sprintf(buf, "%d", rand() % 100);
      depot->put(buf, len, buf, len, Depot::DKEEP);
    }
    cout << "ok" << endl;
    // read in silent mode
    cout << "Reading in silent mode ... ";
    for(i = 1; i <= 100; i++){
      len = sprintf(buf, "%d", rand() % 100);
      if((tmp = depot->get(buf, len)) != 0) free(tmp);
    }
    cout << "ok" << endl;
    // traversal access
    cout << "Traversal access in silent mode ... ";
    depot->iterinit();
    while((tmp = depot->iternext()) != 0){
      free(tmp);
    }
    cout << "ok" << endl;
    // close the database
    cout << "Closing the database ... ";
    depot->close();
    cout << "ok" << endl;
  } catch(Depot_error& e){
    err = true;
    pxdperror(name, e);
  }
  if(depot) delete depot;
  if(err) return 1;
  cout << "all ok" << endl << endl;
  return 0;
}



/* END OF FILE */
