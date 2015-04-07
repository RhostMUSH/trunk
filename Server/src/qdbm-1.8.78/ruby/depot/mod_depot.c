/*************************************************************************************************
 * Implementation of Depot for Ruby
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


#include "ruby.h"
#include <depot.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAXOPEN 1024


VALUE cdepoterror;
VALUE cdepoterror_ENOERR;
VALUE cdepoterror_EFATAL;
VALUE cdepoterror_EMODE;
VALUE cdepoterror_EBROKEN;
VALUE cdepoterror_EKEEP;
VALUE cdepoterror_ENOITEM;
VALUE cdepoterror_EALLOC;
VALUE cdepoterror_EMAP;
VALUE cdepoterror_EOPEN;
VALUE cdepoterror_ECLOSE;
VALUE cdepoterror_ETRUNC;
VALUE cdepoterror_ESYNC;
VALUE cdepoterror_ESTAT;
VALUE cdepoterror_ESEEK;
VALUE cdepoterror_EREAD;
VALUE cdepoterror_EWRITE;
VALUE cdepoterror_ELOCK;
VALUE cdepoterror_EUNLINK;
VALUE cdepoterror_EMKDIR;
VALUE cdepoterror_ERMDIR;
VALUE cdepoterror_EMISC;
VALUE mdepot;
DEPOT *dptable[MAXOPEN];
char dpsltable[MAXOPEN];


static void dpinit(void);
static int getnewindex(void);
static int checkdup(const char *name);
static void myerror(int ecode);
static VALUE rbdpversion(VALUE vself);
static VALUE rbdperrmsg(VALUE vself, VALUE vecode);
static VALUE rbdpopen(VALUE vself, VALUE vname, VALUE vomode, VALUE vbnum);
static VALUE rbdpclose(VALUE vself, VALUE vindex);
static VALUE rbdpsetsilent(VALUE vself, VALUE vindex, VALUE vvalue);
static VALUE rbdpput(VALUE vself, VALUE vindex, VALUE vkey, VALUE vval, VALUE vdmode);
static VALUE rbdpout(VALUE vself, VALUE vindex, VALUE vkey);
static VALUE rbdpget(VALUE vself, VALUE vindex, VALUE vkey, VALUE vstart, VALUE vmax);
static VALUE rbdpvsiz(VALUE vself, VALUE vindex, VALUE vkey);
static VALUE rbdpiterinit(VALUE vself, VALUE vindex);
static VALUE rbdpiternext(VALUE vself, VALUE vindex);
static VALUE rbdpsetalign(VALUE vself, VALUE vindex, VALUE valign);
static VALUE rbdpsetfbpsiz(VALUE vself, VALUE vindex, VALUE vsize);
static VALUE rbdpsync(VALUE vself, VALUE vindex);
static VALUE rbdpoptimize(VALUE vself, VALUE vindex, VALUE vbnum);
static VALUE rbdpfsiz(VALUE vself, VALUE vindex);
static VALUE rbdpbnum(VALUE vself, VALUE vindex);
static VALUE rbdprnum(VALUE vself, VALUE vindex);
static VALUE rbdpwritable(VALUE vself, VALUE vindex);
static VALUE rbdpfatalerror(VALUE vself, VALUE vindex);



/*************************************************************************************************
 * public objects
 *************************************************************************************************/


Init_mod_depot(){
  dpinit();
  cdepoterror = rb_define_class("DepotError", rb_eStandardError);
  cdepoterror_ENOERR = rb_define_class("DepotError_ENOERR", cdepoterror);
  cdepoterror_EFATAL = rb_define_class("DepotError_EFATAL", cdepoterror);
  cdepoterror_EMODE = rb_define_class("DepotError_EMODE", cdepoterror);
  cdepoterror_EBROKEN = rb_define_class("DepotError_EBROKEN", cdepoterror);
  cdepoterror_EKEEP = rb_define_class("DepotError_EKEEP", cdepoterror);
  cdepoterror_ENOITEM = rb_define_class("DepotError_ENOITEM", cdepoterror);
  cdepoterror_EALLOC = rb_define_class("DepotError_EALLOC", cdepoterror);
  cdepoterror_EMAP = rb_define_class("DepotError_EMAP", cdepoterror);
  cdepoterror_EOPEN = rb_define_class("DepotError_EOPEN", cdepoterror);
  cdepoterror_ECLOSE = rb_define_class("DepotError_ECLOSE", cdepoterror);
  cdepoterror_ETRUNC = rb_define_class("DepotError_ETRUNC", cdepoterror);
  cdepoterror_ESYNC = rb_define_class("DepotError_ESYNC", cdepoterror);
  cdepoterror_ESTAT = rb_define_class("DepotError_ESTAT", cdepoterror);
  cdepoterror_ESEEK = rb_define_class("DepotError_ESEEK", cdepoterror);
  cdepoterror_EREAD = rb_define_class("DepotError_EREAD", cdepoterror);
  cdepoterror_EWRITE = rb_define_class("DepotError_EWRITE", cdepoterror);
  cdepoterror_ELOCK = rb_define_class("DepotError_ELOCK", cdepoterror);
  cdepoterror_EUNLINK = rb_define_class("DepotError_EUNLINK", cdepoterror);
  cdepoterror_EMKDIR = rb_define_class("DepotError_EMKDIR", cdepoterror);
  cdepoterror_ERMDIR = rb_define_class("DepotError_ERMDIR", cdepoterror);
  cdepoterror_EMISC = rb_define_class("DepotError_EMISC", cdepoterror);
  mdepot = rb_define_module("Mod_Depot");
  rb_define_const(mdepot, "EANY", cdepoterror);
  rb_define_const(mdepot, "ENOERR", cdepoterror_ENOERR);
  rb_define_const(mdepot, "EFATAL", cdepoterror_EFATAL);
  rb_define_const(mdepot, "EMODE", cdepoterror_EMODE);
  rb_define_const(mdepot, "EBROKEN", cdepoterror_EBROKEN);
  rb_define_const(mdepot, "EKEEP", cdepoterror_EKEEP);
  rb_define_const(mdepot, "ENOITEM", cdepoterror_ENOITEM);
  rb_define_const(mdepot, "EALLOC", cdepoterror_EALLOC);
  rb_define_const(mdepot, "EMAP", cdepoterror_EMAP);
  rb_define_const(mdepot, "EOPEN", cdepoterror_EOPEN);
  rb_define_const(mdepot, "ECLOSE", cdepoterror_ECLOSE);
  rb_define_const(mdepot, "ETRUNC", cdepoterror_ETRUNC);
  rb_define_const(mdepot, "ESYNC", cdepoterror_ESYNC);
  rb_define_const(mdepot, "ESTAT", cdepoterror_ESTAT);
  rb_define_const(mdepot, "ESEEK", cdepoterror_ESEEK);
  rb_define_const(mdepot, "EREAD", cdepoterror_EREAD);
  rb_define_const(mdepot, "EWRITE", cdepoterror_EWRITE);
  rb_define_const(mdepot, "ELOCK", cdepoterror_ELOCK);
  rb_define_const(mdepot, "EUNLINK", cdepoterror_EUNLINK);
  rb_define_const(mdepot, "EMKDIR", cdepoterror_EMKDIR);
  rb_define_const(mdepot, "ERMDIR", cdepoterror_ERMDIR);
  rb_define_const(mdepot, "EMISC", cdepoterror_EMISC);
  rb_define_const(mdepot, "OREADER", INT2FIX(DP_OREADER));
  rb_define_const(mdepot, "OWRITER", INT2FIX(DP_OWRITER));
  rb_define_const(mdepot, "OCREAT", INT2FIX(DP_OCREAT));
  rb_define_const(mdepot, "OTRUNC", INT2FIX(DP_OTRUNC));
  rb_define_const(mdepot, "ONOLCK", INT2FIX(DP_ONOLCK));
  rb_define_const(mdepot, "OLCKNB", INT2FIX(DP_OLCKNB));
  rb_define_const(mdepot, "OSPARSE", INT2FIX(DP_OSPARSE));
  rb_define_const(mdepot, "DOVER", INT2FIX(DP_DOVER));
  rb_define_const(mdepot, "DKEEP", INT2FIX(DP_DKEEP));
  rb_define_const(mdepot, "DCAT", INT2FIX(DP_DCAT));
  rb_define_module_function(mdepot, "mod_open", rbdpopen, 3);
  rb_define_module_function(mdepot, "mod_close", rbdpclose, 1);
  rb_define_module_function(mdepot, "mod_setsilent", rbdpsetsilent, 2);
  rb_define_module_function(mdepot, "mod_put", rbdpput, 4);
  rb_define_module_function(mdepot, "mod_out", rbdpout, 2);
  rb_define_module_function(mdepot, "mod_get", rbdpget, 4);
  rb_define_module_function(mdepot, "mod_vsiz", rbdpvsiz, 2);
  rb_define_module_function(mdepot, "mod_iterinit", rbdpiterinit, 1);
  rb_define_module_function(mdepot, "mod_iternext", rbdpiternext, 1);
  rb_define_module_function(mdepot, "mod_setalign", rbdpsetalign, 2);
  rb_define_module_function(mdepot, "mod_setfbpsiz", rbdpsetfbpsiz, 2);
  rb_define_module_function(mdepot, "mod_sync", rbdpsync, 1);
  rb_define_module_function(mdepot, "mod_optimize", rbdpoptimize, 2);
  rb_define_module_function(mdepot, "mod_fsiz", rbdpfsiz, 1);
  rb_define_module_function(mdepot, "mod_bnum", rbdpbnum, 1);
  rb_define_module_function(mdepot, "mod_rnum", rbdprnum, 1);
  rb_define_module_function(mdepot, "mod_writable", rbdpwritable, 1);
  rb_define_module_function(mdepot, "mod_fatalerror", rbdpfatalerror, 1);
}



/*************************************************************************************************
 * private objects
 *************************************************************************************************/


static void dpinit(void){
  int i;
  for(i = 0; i < MAXOPEN; i++){
    dptable[i] = NULL;
    dpsltable[i] = 0;
  }
}


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


static void myerror(int ecode){
  VALUE verr;
  switch(ecode){
  case DP_ENOERR: verr = cdepoterror_ENOERR; break;
  case DP_EFATAL: verr = cdepoterror_EFATAL; break;
  case DP_EMODE: verr = cdepoterror_EMODE; break;
  case DP_EBROKEN: verr = cdepoterror_EBROKEN; break;
  case DP_EKEEP: verr = cdepoterror_EKEEP; break;
  case DP_ENOITEM: verr = cdepoterror_ENOITEM; break;
  case DP_EALLOC: verr = cdepoterror_EALLOC; break;
  case DP_EMAP: verr = cdepoterror_EMAP; break;
  case DP_EOPEN: verr = cdepoterror_EOPEN; break;
  case DP_ECLOSE: verr = cdepoterror_ECLOSE; break;
  case DP_ETRUNC: verr = cdepoterror_ETRUNC; break;
  case DP_ESYNC: verr = cdepoterror_ESYNC; break;
  case DP_ESTAT: verr = cdepoterror_ESTAT; break;
  case DP_ESEEK: verr = cdepoterror_ESEEK; break;
  case DP_EREAD: verr = cdepoterror_EREAD; break;
  case DP_EWRITE: verr = cdepoterror_EWRITE; break;
  case DP_ELOCK: verr = cdepoterror_ELOCK; break;
  case DP_EUNLINK: verr = cdepoterror_EUNLINK; break;
  case DP_EMKDIR: verr = cdepoterror_EMKDIR; break;
  case DP_ERMDIR: verr = cdepoterror_ERMDIR; break;
  case DP_EMISC: verr = cdepoterror_EMISC; break;
  default: verr = cdepoterror; break;
  }
  rb_raise(verr, "%s", dperrmsg(ecode));
}


static VALUE rbdpopen(VALUE vself, VALUE vname, VALUE vomode, VALUE vbnum){
  DEPOT *depot;
  const char *name;
  int index, omode, bnum;
  if((index = getnewindex()) == -1) myerror(DP_EMISC);
  name = STR2CSTR(vname);
  FIXNUM_P(vomode);
  omode = FIX2INT(vomode);
  FIXNUM_P(vbnum);
  bnum = FIX2INT(vbnum);
  if(checkdup(name) == -1) myerror(DP_EMISC);
  depot = dpopen(name, omode, bnum);
  if(!depot) myerror(dpecode);
  dptable[index] = depot;
  dpsltable[index] = 0;
  return INT2FIX(index);
}


static VALUE rbdpclose(VALUE vself, VALUE vindex){
  DEPOT *depot;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  depot = dptable[index];
  dptable[index] = NULL;
  if(!dpclose(depot)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbdpsetsilent(VALUE vself, VALUE vindex, VALUE vvalue){
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  dpsltable[index] = FIX2INT(vvalue);
  return Qnil;
}


static VALUE rbdpput(VALUE vself, VALUE vindex, VALUE vkey, VALUE vval, VALUE vdmode){
  DEPOT *depot;
  const char *kbuf, *vbuf;
  int index, ksiz, vsiz, dmode;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  kbuf = STR2CSTR(vkey);
  ksiz = RSTRING(vkey)->len;
  vbuf = STR2CSTR(vval);
  vsiz = RSTRING(vval)->len;
  FIXNUM_P(vdmode);
  dmode = FIX2INT(vdmode);
  depot = dptable[index];
  if(!dpput(depot, kbuf, ksiz, vbuf, vsiz, dmode)){
    if(dpsltable[index] && dpecode == DP_EKEEP) return Qfalse;
    myerror(dpecode);
  }
  return Qtrue;
}


static VALUE rbdpout(VALUE vself, VALUE vindex, VALUE vkey){
  DEPOT *depot;
  const char *kbuf;
  int index, ksiz;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  kbuf = STR2CSTR(vkey);
  ksiz = RSTRING(vkey)->len;
  depot = dptable[index];
  if(!dpout(depot, kbuf, ksiz)){
    if(dpsltable[index] && dpecode == DP_ENOITEM) return Qfalse;
    myerror(dpecode);
  }
  return Qtrue;
}


static VALUE rbdpget(VALUE vself, VALUE vindex, VALUE vkey, VALUE vstart, VALUE vmax){
  DEPOT *depot;
  const char *kbuf;
  char *vbuf;
  int index, ksiz, start, max, vsiz;
  VALUE vval;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  kbuf = STR2CSTR(vkey);
  ksiz = RSTRING(vkey)->len;
  FIXNUM_P(vstart);
  start = FIX2INT(vstart);
  FIXNUM_P(vmax);
  max = FIX2INT(vmax);
  depot = dptable[index];
  if(!(vbuf = dpget(depot, kbuf, ksiz, start, max, &vsiz))){
    if(dpsltable[index] && dpecode == DP_ENOITEM) return Qnil;
    myerror(dpecode);
  }
  vval = rb_str_new(vbuf, vsiz);
  free(vbuf);
  return vval;
}


static VALUE rbdpvsiz(VALUE vself, VALUE vindex, VALUE vkey){
  DEPOT *depot;
  const char *kbuf;
  int index, ksiz, vsiz;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  kbuf = STR2CSTR(vkey);
  ksiz = RSTRING(vkey)->len;
  depot = dptable[index];
  if((vsiz = dpvsiz(depot, kbuf, ksiz)) == -1){
    if(dpsltable[index] && dpecode == DP_ENOITEM) return INT2FIX(-1);
    myerror(dpecode);
  }
  return INT2FIX(vsiz);
}


static VALUE rbdpiterinit(VALUE vself, VALUE vindex){
  DEPOT *depot;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  depot = dptable[index];
  if(!dpiterinit(depot)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbdpiternext(VALUE vself, VALUE vindex){
  DEPOT *depot;
  char *kbuf;
  int index, ksiz;
  VALUE vkey;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  depot = dptable[index];
  if(!(kbuf = dpiternext(depot, &ksiz))){
    if(dpsltable[index] && dpecode == DP_ENOITEM) return Qnil;
    myerror(dpecode);
  }
  vkey = rb_str_new(kbuf, ksiz);
  free(kbuf);
  return vkey;
}


static VALUE rbdpsetalign(VALUE vself, VALUE vindex, VALUE valign){
  DEPOT *depot;
  int index, align;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  FIXNUM_P(valign);
  align = FIX2INT(valign);
  depot = dptable[index];
  if(!dpsetalign(depot, align)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbdpsetfbpsiz(VALUE vself, VALUE vindex, VALUE vsize){
  DEPOT *depot;
  int index, size;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  FIXNUM_P(vsize);
  size = FIX2INT(vsize);
  depot = dptable[index];
  if(!dpsetfbpsiz(depot, size)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbdpsync(VALUE vself, VALUE vindex){
  DEPOT *depot;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  depot = dptable[index];
  if(!dpsync(depot)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbdpoptimize(VALUE vself, VALUE vindex, VALUE vbnum){
  DEPOT *depot;
  int index, bnum;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  FIXNUM_P(vbnum);
  bnum = FIX2INT(vbnum);
  depot = dptable[index];
  if(!dpoptimize(depot, bnum)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbdpfsiz(VALUE vself, VALUE vindex){
  DEPOT *depot;
  int index, fsiz;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  depot = dptable[index];
  if((fsiz = dpfsiz(depot)) == -1) myerror(dpecode);
  return INT2FIX(fsiz);
}


static VALUE rbdpbnum(VALUE vself, VALUE vindex){
  DEPOT *depot;
  int index, bnum;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  depot = dptable[index];
  if((bnum = dpbnum(depot)) == -1) myerror(dpecode);
  return INT2FIX(bnum);
}


static VALUE rbdprnum(VALUE vself, VALUE vindex){
  DEPOT *depot;
  int index, rnum;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  depot = dptable[index];
  if((rnum = dprnum(depot)) == -1) myerror(dpecode);
  return INT2FIX(rnum);
}


static VALUE rbdpwritable(VALUE vself, VALUE vindex){
  DEPOT *depot;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  depot = dptable[index];
  return dpwritable(depot) ? Qtrue : Qfalse;
}


static VALUE rbdpfatalerror(VALUE vself, VALUE vindex){
  DEPOT *depot;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  depot = dptable[index];
  return dpfatalerror(depot) ? Qtrue : Qfalse;
}



/* END OF FILE */
