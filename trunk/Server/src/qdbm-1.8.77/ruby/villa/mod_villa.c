/*************************************************************************************************
 * Implementation of Villa for Ruby
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
#include <cabin.h>
#include <villa.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAXOPEN 1024


VALUE cvillaerror;
VALUE cvillaerror_ENOERR;
VALUE cvillaerror_EFATAL;
VALUE cvillaerror_EMODE;
VALUE cvillaerror_EBROKEN;
VALUE cvillaerror_EKEEP;
VALUE cvillaerror_ENOITEM;
VALUE cvillaerror_EALLOC;
VALUE cvillaerror_EMAP;
VALUE cvillaerror_EOPEN;
VALUE cvillaerror_ECLOSE;
VALUE cvillaerror_ETRUNC;
VALUE cvillaerror_ESYNC;
VALUE cvillaerror_ESTAT;
VALUE cvillaerror_ESEEK;
VALUE cvillaerror_EREAD;
VALUE cvillaerror_EWRITE;
VALUE cvillaerror_ELOCK;
VALUE cvillaerror_EUNLINK;
VALUE cvillaerror_EMKDIR;
VALUE cvillaerror_ERMDIR;
VALUE cvillaerror_EMISC;
VALUE mvilla;
ID idcompare;
VILLA *vltable[MAXOPEN];
char vlsltable[MAXOPEN];


static void vlinit(void);
static int getnewindex(void);
static int checkdup(const char *name);
static void myerror(int ecode);
static int objcompare(const char *aptr, int asiz, const char *bptr, int bsiz);
static VALUE rbvlversion(VALUE vself);
static VALUE rbvlerrmsg(VALUE vself, VALUE vecode);
static VALUE rbvlopen(VALUE vself, VALUE vname, VALUE vomode, VALUE vcmode);
static VALUE rbvlclose(VALUE vself, VALUE vindex);
static VALUE rbvlsetsilent(VALUE vself, VALUE vindex, VALUE vvalue);
static VALUE rbvlput(VALUE vself, VALUE vindex, VALUE vkey, VALUE vval, VALUE vdmode);
static VALUE rbvlout(VALUE vself, VALUE vindex, VALUE vkey);
static VALUE rbvlget(VALUE vself, VALUE vindex, VALUE vkey);
static VALUE rbvlvsiz(VALUE vself, VALUE vindex, VALUE vkey);
static VALUE rbvlvnum(VALUE vself, VALUE vindex, VALUE vkey);
static VALUE rbvlcurfirst(VALUE vself, VALUE vindex);
static VALUE rbvlcurlast(VALUE vself, VALUE vindex);
static VALUE rbvlcurprev(VALUE vself, VALUE vindex);
static VALUE rbvlcurnext(VALUE vself, VALUE vindex);
static VALUE rbvlcurjump(VALUE vself, VALUE vindex, VALUE vkey, VALUE vjmode);
static VALUE rbvlcurkey(VALUE vself, VALUE vindex);
static VALUE rbvlcurval(VALUE vself, VALUE vindex);
static VALUE rbvlcurput(VALUE vself, VALUE vindex, VALUE vval, VALUE vjmode);
static VALUE rbvlcurout(VALUE vself, VALUE vindex);
static VALUE rbvlsettuning(VALUE vself, VALUE vindex,
                           VALUE vlrecmax, VALUE vnidxmax, VALUE vlcnum, VALUE vncnum);
static VALUE rbvlsync(VALUE vself, VALUE vindex);
static VALUE rbvloptimize(VALUE vself, VALUE vindex);
static VALUE rbvlfsiz(VALUE vself, VALUE vindex);
static VALUE rbvlrnum(VALUE vself, VALUE vindex);
static VALUE rbvlwritable(VALUE vself, VALUE vindex);
static VALUE rbvlfatalerror(VALUE vself, VALUE vindex);
static VALUE rbvltranbegin(VALUE vself, VALUE vindex);
static VALUE rbvltrancommit(VALUE vself, VALUE vindex);
static VALUE rbvltranabort(VALUE vself, VALUE vindex);



/*************************************************************************************************
 * public objects
 *************************************************************************************************/


Init_mod_villa(){
  vlinit();
  cvillaerror = rb_define_class("VillaError", rb_eStandardError);
  cvillaerror_ENOERR = rb_define_class("VillaError_ENOERR", cvillaerror);
  cvillaerror_EFATAL = rb_define_class("VillaError_EFATAL", cvillaerror);
  cvillaerror_EMODE = rb_define_class("VillaError_EMODE", cvillaerror);
  cvillaerror_EBROKEN = rb_define_class("VillaError_EBROKEN", cvillaerror);
  cvillaerror_EKEEP = rb_define_class("VillaError_EKEEP", cvillaerror);
  cvillaerror_ENOITEM = rb_define_class("VillaError_ENOITEM", cvillaerror);
  cvillaerror_EALLOC = rb_define_class("VillaError_EALLOC", cvillaerror);
  cvillaerror_EMAP = rb_define_class("VillaError_EMAP", cvillaerror);
  cvillaerror_EOPEN = rb_define_class("VillaError_EOPEN", cvillaerror);
  cvillaerror_ECLOSE = rb_define_class("VillaError_ECLOSE", cvillaerror);
  cvillaerror_ETRUNC = rb_define_class("VillaError_ETRUNC", cvillaerror);
  cvillaerror_ESYNC = rb_define_class("VillaError_ESYNC", cvillaerror);
  cvillaerror_ESTAT = rb_define_class("VillaError_ESTAT", cvillaerror);
  cvillaerror_ESEEK = rb_define_class("VillaError_ESEEK", cvillaerror);
  cvillaerror_EREAD = rb_define_class("VillaError_EREAD", cvillaerror);
  cvillaerror_EWRITE = rb_define_class("VillaError_EWRITE", cvillaerror);
  cvillaerror_ELOCK = rb_define_class("VillaError_ELOCK", cvillaerror);
  cvillaerror_EUNLINK = rb_define_class("VillaError_EUNLINK", cvillaerror);
  cvillaerror_EMKDIR = rb_define_class("VillaError_EMKDIR", cvillaerror);
  cvillaerror_ERMDIR = rb_define_class("VillaError_ERMDIR", cvillaerror);
  cvillaerror_EMISC = rb_define_class("VillaError_EMISC", cvillaerror);
  mvilla = rb_define_module("Mod_Villa");
  rb_define_const(mvilla, "EANY", cvillaerror);
  rb_define_const(mvilla, "ENOERR", cvillaerror_ENOERR);
  rb_define_const(mvilla, "EFATAL", cvillaerror_EFATAL);
  rb_define_const(mvilla, "EMODE", cvillaerror_EMODE);
  rb_define_const(mvilla, "EBROKEN", cvillaerror_EBROKEN);
  rb_define_const(mvilla, "EKEEP", cvillaerror_EKEEP);
  rb_define_const(mvilla, "ENOITEM", cvillaerror_ENOITEM);
  rb_define_const(mvilla, "EALLOC", cvillaerror_EALLOC);
  rb_define_const(mvilla, "EMAP", cvillaerror_EMAP);
  rb_define_const(mvilla, "EOPEN", cvillaerror_EOPEN);
  rb_define_const(mvilla, "ECLOSE", cvillaerror_ECLOSE);
  rb_define_const(mvilla, "ETRUNC", cvillaerror_ETRUNC);
  rb_define_const(mvilla, "ESYNC", cvillaerror_ESYNC);
  rb_define_const(mvilla, "ESTAT", cvillaerror_ESTAT);
  rb_define_const(mvilla, "ESEEK", cvillaerror_ESEEK);
  rb_define_const(mvilla, "EREAD", cvillaerror_EREAD);
  rb_define_const(mvilla, "EWRITE", cvillaerror_EWRITE);
  rb_define_const(mvilla, "ELOCK", cvillaerror_ELOCK);
  rb_define_const(mvilla, "EUNLINK", cvillaerror_EUNLINK);
  rb_define_const(mvilla, "EMKDIR", cvillaerror_EMKDIR);
  rb_define_const(mvilla, "ERMDIR", cvillaerror_ERMDIR);
  rb_define_const(mvilla, "EMISC", cvillaerror_EMISC);
  rb_define_const(mvilla, "OREADER", INT2FIX(VL_OREADER));
  rb_define_const(mvilla, "OWRITER", INT2FIX(VL_OWRITER));
  rb_define_const(mvilla, "OCREAT", INT2FIX(VL_OCREAT));
  rb_define_const(mvilla, "OTRUNC", INT2FIX(VL_OTRUNC));
  rb_define_const(mvilla, "ONOLCK", INT2FIX(VL_ONOLCK));
  rb_define_const(mvilla, "OLCKNB", INT2FIX(VL_OLCKNB));
  rb_define_const(mvilla, "OZCOMP", INT2FIX(VL_OZCOMP));
  rb_define_const(mvilla, "OYCOMP", INT2FIX(VL_OYCOMP));
  rb_define_const(mvilla, "OXCOMP", INT2FIX(VL_OXCOMP));
  rb_define_const(mvilla, "CMPLEX", INT2FIX(0));
  rb_define_const(mvilla, "CMPDEC", INT2FIX(1));
  rb_define_const(mvilla, "CMPOBJ", INT2FIX(2));
  rb_define_const(mvilla, "DOVER", INT2FIX(VL_DOVER));
  rb_define_const(mvilla, "DKEEP", INT2FIX(VL_DKEEP));
  rb_define_const(mvilla, "DCAT", INT2FIX(VL_DCAT));
  rb_define_const(mvilla, "DDUP", INT2FIX(VL_DDUP));
  rb_define_const(mvilla, "DDUPR", INT2FIX(VL_DDUPR));
  rb_define_const(mvilla, "JFORWARD", INT2FIX(VL_JFORWARD));
  rb_define_const(mvilla, "JBACKWARD", INT2FIX(VL_JBACKWARD));
  rb_define_const(mvilla, "CPCURRENT", INT2FIX(VL_CPCURRENT));
  rb_define_const(mvilla, "CPBEFORE", INT2FIX(VL_CPBEFORE));
  rb_define_const(mvilla, "CPAFTER", INT2FIX(VL_CPAFTER));
  rb_define_module_function(mvilla, "mod_open", rbvlopen, 3);
  rb_define_module_function(mvilla, "mod_close", rbvlclose, 1);
  rb_define_module_function(mvilla, "mod_setsilent", rbvlsetsilent, 2);
  rb_define_module_function(mvilla, "mod_put", rbvlput, 4);
  rb_define_module_function(mvilla, "mod_out", rbvlout, 2);
  rb_define_module_function(mvilla, "mod_get", rbvlget, 2);
  rb_define_module_function(mvilla, "mod_vsiz", rbvlvsiz, 2);
  rb_define_module_function(mvilla, "mod_vnum", rbvlvnum, 2);
  rb_define_module_function(mvilla, "mod_curfirst", rbvlcurfirst, 1);
  rb_define_module_function(mvilla, "mod_curlast", rbvlcurlast, 1);
  rb_define_module_function(mvilla, "mod_curprev", rbvlcurprev, 1);
  rb_define_module_function(mvilla, "mod_curnext", rbvlcurnext, 1);
  rb_define_module_function(mvilla, "mod_curjump", rbvlcurjump, 3);
  rb_define_module_function(mvilla, "mod_curkey", rbvlcurkey, 1);
  rb_define_module_function(mvilla, "mod_curval", rbvlcurval, 1);
  rb_define_module_function(mvilla, "mod_curput", rbvlcurput, 3);
  rb_define_module_function(mvilla, "mod_curout", rbvlcurout, 1);
  rb_define_module_function(mvilla, "mod_settuning", rbvlsettuning, 5);
  rb_define_module_function(mvilla, "mod_sync", rbvlsync, 1);
  rb_define_module_function(mvilla, "mod_optimize", rbvloptimize, 1);
  rb_define_module_function(mvilla, "mod_fsiz", rbvlfsiz, 1);
  rb_define_module_function(mvilla, "mod_rnum", rbvlrnum, 1);
  rb_define_module_function(mvilla, "mod_writable", rbvlwritable, 1);
  rb_define_module_function(mvilla, "mod_fatalerror", rbvlfatalerror, 1);
  rb_define_module_function(mvilla, "mod_tranbegin", rbvltranbegin, 1);
  rb_define_module_function(mvilla, "mod_trancommit", rbvltrancommit, 1);
  rb_define_module_function(mvilla, "mod_tranabort", rbvltranabort, 1);
  rb_eval_string("def Mod_Villa.compare(astr, bstr)\n"
                 "  aobj = nil\n"
                 "  bobj = nil\n"
                 "  begin\n"
                 "    aobj = Marshal.load(astr);\n"
                 "  rescue\n"
                 "  end\n"
                 "  begin\n"
                 "    bobj = Marshal.load(bstr);\n"
                 "  rescue\n"
                 "  end\n"
                 "  if(aobj && !bobj)\n"
                 "    return 1\n"
                 "  end\n"
                 "  if(!aobj && bobj)\n"
                 "    return -1\n"
                 "  end\n"
                 "  if(!aobj && !bobj)\n"
                 "    return 0\n"
                 "  end\n"
                 "  begin\n"
                 "    aobj <=> bobj\n"
                 "  rescue\n"
                 "    0\n"
                 "  end\n"
                 "end\n");
  idcompare = rb_intern("compare");
}



/*************************************************************************************************
 * private objects
 *************************************************************************************************/


static void vlinit(void){
  int i;
  for(i = 0; i < MAXOPEN; i++){
    vltable[i] = NULL;
    vlsltable[i] = 0;
  }
}


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


static void myerror(int ecode){
  VALUE verr;
  switch(ecode){
  case DP_ENOERR: verr = cvillaerror_ENOERR; break;
  case DP_EFATAL: verr = cvillaerror_EFATAL; break;
  case DP_EMODE: verr = cvillaerror_EMODE; break;
  case DP_EBROKEN: verr = cvillaerror_EBROKEN; break;
  case DP_EKEEP: verr = cvillaerror_EKEEP; break;
  case DP_ENOITEM: verr = cvillaerror_ENOITEM; break;
  case DP_EALLOC: verr = cvillaerror_EALLOC; break;
  case DP_EMAP: verr = cvillaerror_EMAP; break;
  case DP_EOPEN: verr = cvillaerror_EOPEN; break;
  case DP_ECLOSE: verr = cvillaerror_ECLOSE; break;
  case DP_ETRUNC: verr = cvillaerror_ETRUNC; break;
  case DP_ESYNC: verr = cvillaerror_ESYNC; break;
  case DP_ESTAT: verr = cvillaerror_ESTAT; break;
  case DP_ESEEK: verr = cvillaerror_ESEEK; break;
  case DP_EREAD: verr = cvillaerror_EREAD; break;
  case DP_EWRITE: verr = cvillaerror_EWRITE; break;
  case DP_ELOCK: verr = cvillaerror_ELOCK; break;
  case DP_EUNLINK: verr = cvillaerror_EUNLINK; break;
  case DP_EMKDIR: verr = cvillaerror_EMKDIR; break;
  case DP_ERMDIR: verr = cvillaerror_ERMDIR; break;
  case DP_EMISC: verr = cvillaerror_EMISC; break;
  default: verr = cvillaerror; break;
  }
  rb_raise(verr, "%s", dperrmsg(ecode));
}


static int objcompare(const char *aptr, int asiz, const char *bptr, int bsiz){
  VALUE vastr, vbstr, vret;
  vastr = rb_str_new(aptr, asiz);
  vbstr = rb_str_new(bptr, bsiz);
  vret = rb_funcall(mvilla, idcompare, 2, vastr, vbstr);
  return FIX2INT(vret);
}


static VALUE rbvlopen(VALUE vself, VALUE vname, VALUE vomode, VALUE vcmode){
  VILLA *villa;
  const char *name;
  int index, omode, cmode;
  VLCFUNC cmp;
  if((index = getnewindex()) == -1) myerror(DP_EMISC);
  name = STR2CSTR(vname);
  FIXNUM_P(vomode);
  omode = FIX2INT(vomode);
  FIXNUM_P(vcmode);
  cmode = FIX2INT(vcmode);
  cmp = NULL;
  switch(cmode){
  case 0: cmp = VL_CMPLEX; break;
  case 1: cmp = VL_CMPDEC; break;
  case 2: cmp = objcompare; break;
  default: myerror(DP_EMISC);
  }
  if(checkdup(name) == -1) myerror(DP_EMISC);
  villa = vlopen(name, omode, cmp);
  if(!villa) myerror(dpecode);
  vltable[index] = villa;
  vlsltable[index] = 0;
  return INT2FIX(index);
}


static VALUE rbvlclose(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  vltable[index] = NULL;
  if(!vlclose(villa)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbvlsetsilent(VALUE vself, VALUE vindex, VALUE vvalue){
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  vlsltable[index] = FIX2INT(vvalue);
  return Qnil;
}


static VALUE rbvlput(VALUE vself, VALUE vindex, VALUE vkey, VALUE vval, VALUE vdmode){
  VILLA *villa;
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
  villa = vltable[index];
  if(!vlput(villa, kbuf, ksiz, vbuf, vsiz, dmode)){
    if(vlsltable[index] && dpecode == DP_EKEEP) return Qfalse;
    myerror(dpecode);
  }
  return Qtrue;
}


static VALUE rbvlout(VALUE vself, VALUE vindex, VALUE vkey){
  VILLA *villa;
  const char *kbuf;
  int index, ksiz;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  kbuf = STR2CSTR(vkey);
  ksiz = RSTRING(vkey)->len;
  villa = vltable[index];
  if(!vlout(villa, kbuf, ksiz)){
    if(vlsltable[index] && dpecode == DP_ENOITEM) return Qfalse;
    myerror(dpecode);
  }
  return Qtrue;
}


static VALUE rbvlget(VALUE vself, VALUE vindex, VALUE vkey){
  VILLA *villa;
  const char *kbuf, *vbuf;
  int index, ksiz, vsiz;
  VALUE vval;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  kbuf = STR2CSTR(vkey);
  ksiz = RSTRING(vkey)->len;
  villa = vltable[index];
  if(!(vbuf = vlget(villa, kbuf, ksiz, &vsiz))){
    if(vlsltable[index] && dpecode == DP_ENOITEM) return Qnil;
    myerror(dpecode);
  }
  vval = rb_str_new(vbuf, vsiz);
  return vval;
}


static VALUE rbvlvsiz(VALUE vself, VALUE vindex, VALUE vkey){
  VILLA *villa;
  const char *kbuf;
  int index, ksiz, vsiz;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  kbuf = STR2CSTR(vkey);
  ksiz = RSTRING(vkey)->len;
  villa = vltable[index];
  if((vsiz = vlvsiz(villa, kbuf, ksiz)) == -1){
    if(vlsltable[index] && dpecode == DP_ENOITEM) return INT2FIX(-1);
    myerror(dpecode);
  }
  return INT2FIX(vsiz);
}


static VALUE rbvlvnum(VALUE vself, VALUE vindex, VALUE vkey){
  VILLA *villa;
  const char *kbuf;
  int index, ksiz, vnum;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  kbuf = STR2CSTR(vkey);
  ksiz = RSTRING(vkey)->len;
  villa = vltable[index];
  vnum = vlvnum(villa, kbuf, ksiz);
  return INT2FIX(vnum);
}


static VALUE rbvlcurfirst(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if(!vlcurfirst(villa)){
    if(vlsltable[index] && dpecode == DP_ENOITEM) return Qfalse;
    myerror(dpecode);
  }
  return Qtrue;
}


static VALUE rbvlcurlast(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if(!vlcurlast(villa)){
    if(vlsltable[index] && dpecode == DP_ENOITEM) return Qfalse;
    myerror(dpecode);
  }
  return Qtrue;
}


static VALUE rbvlcurprev(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if(!vlcurprev(villa)){
    if(vlsltable[index] && dpecode == DP_ENOITEM) return Qfalse;
    myerror(dpecode);
  }
  return Qtrue;
}


static VALUE rbvlcurnext(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if(!vlcurnext(villa)){
    if(vlsltable[index] && dpecode == DP_ENOITEM) return Qfalse;
    myerror(dpecode);
  }
  return Qtrue;
}


static VALUE rbvlcurjump(VALUE vself, VALUE vindex, VALUE vkey, VALUE vjmode){
  VILLA *villa;
  const char *kbuf;
  int index, ksiz, jmode;
  VALUE vval;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  kbuf = STR2CSTR(vkey);
  ksiz = RSTRING(vkey)->len;
  FIXNUM_P(vjmode);
  jmode = FIX2INT(vjmode);
  villa = vltable[index];
  if(!vlcurjump(villa, kbuf, ksiz, jmode)){
    if(vlsltable[index] && dpecode == DP_ENOITEM) return Qfalse;
    myerror(dpecode);
  }
  return Qtrue;
}



static VALUE rbvlcurkey(VALUE vself, VALUE vindex){
  VILLA *villa;
  const char *kbuf;
  int index, ksiz;
  VALUE vkey;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if(!(kbuf = vlcurkeycache(villa, &ksiz))){
    if(vlsltable[index] && dpecode == DP_ENOITEM) return Qnil;
    myerror(dpecode);
  }
  vkey = rb_str_new(kbuf, ksiz);
  return vkey;
}


static VALUE rbvlcurval(VALUE vself, VALUE vindex){
  VILLA *villa;
  const char *vbuf;
  int index, vsiz;
  VALUE vval;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if(!(vbuf = vlcurvalcache(villa, &vsiz))){
    if(vlsltable[index] && dpecode == DP_ENOITEM) return Qnil;
    myerror(dpecode);
  }
  vval = rb_str_new(vbuf, vsiz);
  return vval;
}


static VALUE rbvlcurput(VALUE vself, VALUE vindex, VALUE vval, VALUE vcpmode){
  VILLA *villa;
  const char *vbuf;
  int index, vsiz, cpmode;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  vbuf = STR2CSTR(vval);
  vsiz = RSTRING(vval)->len;
  FIXNUM_P(vcpmode);
  cpmode = FIX2INT(vcpmode);
  villa = vltable[index];
  if(!vlcurput(villa, vbuf, vsiz, cpmode)){
    if(vlsltable[index] && dpecode == DP_ENOITEM) return Qfalse;
    myerror(dpecode);
  }
  return Qtrue;
}


static VALUE rbvlcurout(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if(!vlcurout(villa)){
    if(vlsltable[index] && dpecode == DP_ENOITEM) return Qfalse;
    myerror(dpecode);
  }
  return Qtrue;
}


static VALUE rbvlsettuning(VALUE vself, VALUE vindex,
                           VALUE vlrecmax, VALUE vnidxmax, VALUE vlcnum, VALUE vncnum){
  VILLA *villa;
  int index, lrecmax, nidxmax, lcnum, ncnum;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  FIXNUM_P(vlrecmax);
  lrecmax = FIX2INT(vlrecmax);
  FIXNUM_P(vnidxmax);
  nidxmax = FIX2INT(vnidxmax);
  FIXNUM_P(vlcnum);
  lcnum = FIX2INT(vlcnum);
  FIXNUM_P(vncnum);
  ncnum = FIX2INT(vncnum);
  villa = vltable[index];
  vlsettuning(villa, lrecmax, nidxmax, lcnum, ncnum);
  return Qtrue;
}


static VALUE rbvlsync(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if(!vlsync(villa)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbvloptimize(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if(!vloptimize(villa)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbvlfsiz(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index, fsiz;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if((fsiz = vlfsiz(villa)) == -1) myerror(dpecode);
  return INT2FIX(fsiz);
}


static VALUE rbvlrnum(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index, rnum;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if((rnum = vlrnum(villa)) == -1) myerror(dpecode);
  return INT2FIX(rnum);
}


static VALUE rbvlwritable(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  return vlwritable(villa) ? Qtrue : Qfalse;
}


static VALUE rbvlfatalerror(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  return vlfatalerror(villa) ? Qtrue : Qfalse;
}


static VALUE rbvltranbegin(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if(!vltranbegin(villa)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbvltrancommit(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if(!vltrancommit(villa)) myerror(dpecode);
  return Qtrue;
}


static VALUE rbvltranabort(VALUE vself, VALUE vindex){
  VILLA *villa;
  int index;
  FIXNUM_P(vindex);
  if((index = FIX2INT(vindex)) == -1) myerror(DP_EMISC);
  villa = vltable[index];
  if(!vltranabort(villa)) myerror(dpecode);
  return Qtrue;
}



/* END OF FILE */
