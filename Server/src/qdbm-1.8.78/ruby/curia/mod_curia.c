/*************************************************************************************************
 * Implementation of Curia for Ruby
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
#include <curia.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAXOPEN 1024


VALUE ccuriaerror;
VALUE ccuriaerror_ENOERR;
VALUE ccuriaerror_EFATAL;
VALUE ccuriaerror_EMODE;
VALUE ccuriaerror_EBROKEN;
VALUE ccuriaerror_EKEEP;
VALUE ccuriaerror_ENOITEM;
VALUE ccuriaerror_EALLOC;
VALUE ccuriaerror_EMAP;
VALUE ccuriaerror_EOPEN;
VALUE ccuriaerror_ECLOSE;
VALUE ccuriaerror_ETRUNC;
VALUE ccuriaerror_ESYNC;
VALUE ccuriaerror_ESTAT;
VALUE ccuriaerror_ESEEK;
VALUE ccuriaerror_EREAD;
VALUE ccuriaerror_EWRITE;
VALUE ccuriaerror_ELOCK;
VALUE ccuriaerror_EUNLINK;
VALUE ccuriaerror_EMKDIR;
VALUE ccuriaerror_ERMDIR;
VALUE ccuriaerror_EMISC;
VALUE mcuria;
CURIA *crtable[MAXOPEN];
char crsltable[MAXOPEN];


static void crinit(void);
static int getnewindex(void);
static int checkdup(const char *name);
static void myerror(int ecode);
static VALUE rbcrversion(VALUE vself);
static VALUE rbcrerrmsg(VALUE vself, VALUE vecode);
static VALUE rbcropen(VALUE vself, VALUE vname, VALUE vomode, VALUE vbnum, VALUE vdnum);
static VALUE rbcrclose(VALUE vself, VALUE vindex);
static VALUE rbcrsetsilent(VALUE vself, VALUE vindex, VALUE vvalue);
static VALUE rbcrput(VALUE vself, VALUE vindex, VALUE vkey, VALUE vval, VALUE vdmode);
static VALUE rbcrout(VALUE vself, VALUE vindex, VALUE vkey);
static VALUE rbcrget(VALUE vself, VALUE vindex, VALUE vkey, VALUE vstart, VALUE vmax);
static VALUE rbcrvsiz(VALUE vself, VALUE vindex, VALUE vkey);
static VALUE rbcriterinit(VALUE vself, VALUE vindex);
static VALUE rbcriternext(VALUE vself, VALUE vindex);
static VALUE rbcrsetalign(VALUE vself, VALUE vindex, VALUE valign);
static VALUE rbcrsetfbpsiz(VALUE vself, VALUE vindex, VALUE vsize);
static VALUE rbcrsync(VALUE vself, VALUE vindex);
static VALUE rbcroptimize(VALUE vself, VALUE vindex, VALUE vbnum);
static VALUE rbcrfsiz(VALUE vself, VALUE vindex);
static VALUE rbcrbnum(VALUE vself, VALUE vindex);
static VALUE rbcrrnum(VALUE vself, VALUE vindex);
static VALUE rbcrwritable(VALUE vself, VALUE vindex);
static VALUE rbcrfatalerror(VALUE vself, VALUE vindex);



/*************************************************************************************************
 * public objects
 *************************************************************************************************/


Init_mod_curia(){
  crinit();
  ccuriaerror = rb_define_class("CuriaError", rb_eStandardError);
  ccuriaerror_ENOERR = rb_define_class("CuriaError_ENOERR", ccuriaerror);
  ccuriaerror_EFATAL = rb_define_class("CuriaError_EFATAL", ccuriaerror);
  ccuriaerror_EMODE = rb_define_class("CuriaError_EMODE", ccuriaerror);
  ccuriaerror_EBROKEN = rb_define_class("CuriaError_EBROKEN", ccuriaerror);
  ccuriaerror_EKEEP = rb_define_class("CuriaError_EKEEP", ccuriaerror);
  ccuriaerror_ENOITEM = rb_define_class("CuriaError_ENOITEM", ccuriaerror);
  ccuriaerror_EALLOC = rb_define_class("CuriaError_EALLOC", ccuriaerror);
  ccuriaerror_EMAP = rb_define_class("CuriaError_EMAP", ccuriaerror);
  ccuriaerror_EOPEN = rb_define_class("CuriaError_EOPEN", ccuriaerror);
  ccuriaerror_ECLOSE = rb_define_class("CuriaError_ECLOSE", ccuriaerror);
  ccuriaerror_ETRUNC = rb_define_class("CuriaError_ETRUNC", ccuriaerror);
  ccuriaerror_ESYNC = rb_define_class("CuriaError_ESYNC", ccuriaerror);
  ccuriaerror_ESTAT = rb_define_class("CuriaError_ESTAT", ccuriaerror);
  ccuriaerror_ESEEK = rb_define_class("CuriaError_ESEEK", ccuriaerror);
  ccuriaerror_EREAD = rb_define_class("CuriaError_EREAD", ccuriaerror);
  ccuriaerror_EWRITE = rb_define_class("CuriaError_EWRITE", ccuriaerror);
  ccuriaerror_ELOCK = rb_define_class("CuriaError_ELOCK", ccuriaerror);
  ccuriaerror_EUNLINK = rb_define_class("CuriaError_EUNLINK", ccuriaerror);
  ccuriaerror_EMKDIR = rb_define_class("CuriaError_EMKDIR", ccuriaerror);
  ccuriaerror_ERMDIR = rb_define_class("CuriaError_ERMDIR", ccuriaerror);
  ccuriaerror_EMISC = rb_define_class("CuriaError_EMISC", ccuriaerror);
  mcuria = rb_define_module("Mod_Curia");
  rb_define_const(mcuria, "EANY", ccuriaerror);
  rb_define_const(mcuria, "ENOERR", ccuriaerror_ENOERR);
  rb_define_const(mcuria, "EFATAL", ccuriaerror_EFATAL);
  rb_define_const(mcuria, "EMODE", ccuriaerror_EMODE);
  rb_define_const(mcuria, "EBROKEN", ccuriaerror_EBROKEN);
  rb_define_const(mcuria, "EKEEP", ccuriaerror_EKEEP);
  rb_define_const(mcuria, "ENOITEM", ccuriaerror_ENOITEM);
  rb_define_const(mcuria, "EALLOC", ccuriaerror_EALLOC);
  rb_define_const(mcuria, "EMAP", ccuriaerror_EMAP);
  rb_define_const(mcuria, "EOPEN", ccuriaerror_EOPEN);
  rb_define_const(mcuria, "ECLOSE", ccuriaerror_ECLOSE);
  rb_define_const(mcuria, "ETRUNC", ccuriaerror_ETRUNC);
  rb_define_const(mcuria, "ESYNC", ccuriaerror_ESYNC);
  rb_define_const(mcuria, "ESTAT", ccuriaerror_ESTAT);
  rb_define_const(mcuria, "ESEEK", ccuriaerror_ESEEK);
  rb_define_const(mcuria, "EREAD", ccuriaerror_EREAD);
  rb_define_const(mcuria, "EWRITE", ccuriaerror_EWRITE);
  rb_define_const(mcuria, "ELOCK", ccuriaerror_ELOCK);
  rb_define_const(mcuria, "EUNLINK", ccuriaerror_EUNLINK);
  rb_define_const(mcuria, "EMKDIR", ccuriaerror_EMKDIR);
  rb_define_const(mcuria, "ERMDIR", ccuriaerror_ERMDIR);
  rb_define_const(mcuria, "EMISC", ccuriaerror_EMISC);
  rb_define_const(mcuria, "OREADER", INT2FIX(CR_OREADER));
  rb_define_const(mcuria, "OWRITER", INT2FIX(CR_OWRITER));
  rb_define_const(mcuria, "OCREAT", INT2FIX(CR_OCREAT));
  rb_define_const(mcuria, "OTRUNC", INT2FIX(CR_OTRUNC));
  rb_define_const(mcuria, "ONOLCK", INT2FIX(CR_ONOLCK));
  rb_define_const(mcuria, "OLCKNB", INT2FIX(CR_OLCKNB));
  rb_define_const(mcuria, "OSPARSE", INT2FIX(CR_OSPARSE));
  rb_define_const(mcuria, "DOVER", INT2FIX(CR_DOVER));
  rb_define_const(mcuria, "DKEEP", INT2FIX(CR_DKEEP));
  rb_define_const(mcuria, "DCAT", INT2FIX(CR_DCAT));
  rb_define_module_function(mcuria, "mod_open", rbcropen, 4);
  rb_define_module_function(mcuria, "mod_close", rbcrclose, 1);
  rb_define_module_function(mcuria, "mod_setsilent", rbcrsetsilent, 2);
  rb_define_module_function(mcuria, "mod_put", rbcrput, 4);
  rb_define_module_function(mcuria, "mod_out", rbcrout, 2);
  rb_define_module_function(mcuria, "mod_get", rbcrget, 4);
  rb_define_module_function(mcuria, "mod_vsiz", rbcrvsiz, 2);
  rb_define_module_function(mcuria, "mod_iterinit", rbcriterinit, 1);
  rb_define_module_function(mcuria, "mod_iternext", rbcriternext, 1);
  rb_define_module_function(mcuria, "mod_setalign", rbcrsetalign, 2);
  rb_define_module_function(mcuria, "mod_setfbpsiz", rbcrsetfbpsiz, 2);
  rb_define_module_function(mcuria, "mod_sync", rbcrsync, 1);
  rb_define_module_function(mcuria, "mod_optimize", rbcroptimize, 2);
  rb_define_module_function(mcuria, "mod_fsiz", rbcrfsiz, 1);
  rb_define_module_function(mcuria, "mod_bnum", rbcrbnum, 1);
  rb_define_module_function(mcuria, "mod_rnum", rbcrrnum, 1);
  rb_define_module_function(mcuria, "mod_writable", rbcrwritable, 1);
  rb_define_module_function(mcuria, "mod_fatalerror", rbcrfatalerror, 1);
}



/*************************************************************************************************
 * private objects
 *************************************************************************************************/


static void crinit(void){
  int i;
  for(i = 0; i < MAXOPEN; i++){
    crtable[i] = NULL;
    crsltable[i] = 0;
  }
}


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


static void myerror(int ecode){
  VALUE verr;
  switch(ecode){
  case DP_ENOERR: verr = ccuriaerror_ENOERR; break;
  case DP_EFATAL: verr = ccuriaerror_EFATAL; break;
  case DP_EMODE: verr = ccuriaerror_EMODE; break;
  case DP_EBROKEN: verr = ccuriaerror_EBROKEN; break;
  case DP_EKEEP: verr = ccuriaerror_EKEEP; break;
  case DP_ENOITEM: verr = ccuriaerror_ENOITEM; break;
  case DP_EALLOC: verr = ccuriaerror_EALLOC; break;
  case DP_EMAP: verr = ccuriaerror_EMAP; break;
  case DP_EOPEN: verr = ccuriaerror_EOPEN; break;
  case DP_ECLOSE: verr = ccuriaerror_ECLOSE; break;
  case DP_ETRUNC: verr = ccuriaerror_ETRUNC; break;
  case DP_ESYNC: verr = ccuriaerror_ESYNC; break;
  case DP_ESTAT: verr = ccuriaerror_ESTAT; break;
  case DP_ESEEK: verr = ccuriaerror_ESEEK; break;
  case DP_EREAD: verr = ccuriaerror_EREAD; break;
  case DP_EWRITE: verr = ccuriaerror_EWRITE; break;
  case DP_ELOCK: verr = ccuriaerror_ELOCK; break;
  case DP_EUNLINK: verr = ccuriaerror_EUNLINK; break;
  case DP_EMKDIR: verr = ccuriaerror_EMKDIR; break;
  case DP_ERMDIR: verr = ccuriaerror_ERMDIR; break;
  case DP_EMISC: verr = ccuriaerror_EMISC; break;
  default: verr = ccuriaerror; break;
  }
  rb_raise(verr, "%s", dperrmsg(ecode));
}


static VALUE rbcropen(VALUE vself, VALUE vname, VALUE vomode, VALUE vbnum, VALUE vdnum){
  CURIA *curia;
  const char *name;
  int index, omode, bnum, dnum;
  if((index = getnewindex()) == -1) myerror(DP_EMISC);
  name = STR2CSTR(vname);
  FIXNUM_P(vomode);
  omode = FIX2INT(vomode);
  FIXNUM_P(vbnum);
  bnum = FIX2INT(vbnum);
  FIXNUM_P(vdnum);
  dnum = FIX2INT(vdnum);
  if(checkdup(name) == -1) myerror(DP_EMISC);
  curia = cropen(name, omode, bnum, dnum);
  if(!curia) myerror(dpecode);
  crtable[index] = curia;
  crsltable[index] = 0;
  return INT2FIX(index);
}


static VALUE rbcrclose(VALUE vself, VALUE vindex){
  CURIA *curia;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  curia = crtable[index];
  crtable[index] = NULL;
  if(!crclose(curia)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbcrsetsilent(VALUE vself, VALUE vindex, VALUE vvalue){
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  crsltable[index] = FIX2INT(vvalue);
  return Qnil;
}


static VALUE rbcrput(VALUE vself, VALUE vindex, VALUE vkey, VALUE vval, VALUE vdmode){
  CURIA *curia;
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
  curia = crtable[index];
  if(!crput(curia, kbuf, ksiz, vbuf, vsiz, dmode)){
    if(crsltable[index] && dpecode == DP_EKEEP) return Qfalse;
    myerror(dpecode);
  }
  return Qtrue;
}


static VALUE rbcrout(VALUE vself, VALUE vindex, VALUE vkey){
  CURIA *curia;
  const char *kbuf;
  int index, ksiz;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  kbuf = STR2CSTR(vkey);
  ksiz = RSTRING(vkey)->len;
  curia = crtable[index];
  if(!crout(curia, kbuf, ksiz)){
    if(crsltable[index] && dpecode == DP_ENOITEM) return Qfalse;
    myerror(dpecode);
  }
  return Qtrue;
}


static VALUE rbcrget(VALUE vself, VALUE vindex, VALUE vkey, VALUE vstart, VALUE vmax){
  CURIA *curia;
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
  curia = crtable[index];
  if(!(vbuf = crget(curia, kbuf, ksiz, start, max, &vsiz))){
    if(crsltable[index] && dpecode == DP_ENOITEM) return Qnil;
    myerror(dpecode);
  }
  vval = rb_str_new(vbuf, vsiz);
  free(vbuf);
  return vval;
}


static VALUE rbcrvsiz(VALUE vself, VALUE vindex, VALUE vkey){
  CURIA *curia;
  const char *kbuf;
  int index, ksiz, vsiz;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  kbuf = STR2CSTR(vkey);
  ksiz = RSTRING(vkey)->len;
  curia = crtable[index];
  if((vsiz = crvsiz(curia, kbuf, ksiz)) == -1){
    if(crsltable[index] && dpecode == DP_ENOITEM) return INT2FIX(-1);
    myerror(dpecode);
  }
  return INT2FIX(vsiz);
}


static VALUE rbcriterinit(VALUE vself, VALUE vindex){
  CURIA *curia;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  curia = crtable[index];
  if(!criterinit(curia)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbcriternext(VALUE vself, VALUE vindex){
  CURIA *curia;
  char *kbuf;
  int index, ksiz;
  VALUE vkey;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  curia = crtable[index];
  if(!(kbuf = criternext(curia, &ksiz))){
    if(crsltable[index] && dpecode == DP_ENOITEM) return Qnil;
    myerror(dpecode);
  }
  vkey = rb_str_new(kbuf, ksiz);
  free(kbuf);
  return vkey;
}


static VALUE rbcrsetalign(VALUE vself, VALUE vindex, VALUE valign){
  CURIA *curia;
  int index, align;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  FIXNUM_P(valign);
  align = FIX2INT(valign);
  curia = crtable[index];
  if(!crsetalign(curia, align)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbcrsetfbpsiz(VALUE vself, VALUE vindex, VALUE vsize){
  CURIA *curia;
  int index, size;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  FIXNUM_P(vsize);
  size = FIX2INT(vsize);
  curia = crtable[index];
  if(!crsetfbpsiz(curia, size)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbcrsync(VALUE vself, VALUE vindex){
  CURIA *curia;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  curia = crtable[index];
  if(!crsync(curia)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbcroptimize(VALUE vself, VALUE vindex, VALUE vbnum){
  CURIA *curia;
  int index, bnum;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  FIXNUM_P(vbnum);
  bnum = FIX2INT(vbnum);
  curia = crtable[index];
  if(!croptimize(curia, bnum)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbcrfsiz(VALUE vself, VALUE vindex){
  CURIA *curia;
  int index, fsiz;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  curia = crtable[index];
  if((fsiz = crfsiz(curia)) == -1) myerror(dpecode);
  return INT2FIX(fsiz);
}


static VALUE rbcrbnum(VALUE vself, VALUE vindex){
  CURIA *curia;
  int index, bnum;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  curia = crtable[index];
  if((bnum = crbnum(curia)) == -1) myerror(dpecode);
  return INT2FIX(bnum);
}


static VALUE rbcrrnum(VALUE vself, VALUE vindex){
  CURIA *curia;
  int index, rnum;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  curia = crtable[index];
  if((rnum = crrnum(curia)) == -1) myerror(dpecode);
  return INT2FIX(rnum);
}


static VALUE rbcrwritable(VALUE vself, VALUE vindex){
  CURIA *curia;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  curia = crtable[index];
  return crwritable(curia) ? Qtrue : Qfalse;
}


static VALUE rbcrfatalerror(VALUE vself, VALUE vindex){
  CURIA *curia;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  curia = crtable[index];
  return crfatalerror(curia) ? Qtrue : Qfalse;
}



/* END OF FILE */
