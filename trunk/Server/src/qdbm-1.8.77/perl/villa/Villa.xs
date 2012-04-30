/*************************************************************************************************
 * Villa.c
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


#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include <depot.h>
#include <cabin.h>
#include <villa.h>
#include <stdlib.h>


MODULE = Villa		PACKAGE = Villa
PROTOTYPES: DISABLE



##================================================================================================
## public objects
##================================================================================================


const char *
plvlerrmsg()
CODE:
	RETVAL = dperrmsg(dpecode);
OUTPUT:
	RETVAL


void *
plvlopen(name, omode, cmode)
	char *	name
	int 	omode
	int	cmode
PREINIT:
	VLCFUNC cmp;
CODE:
	switch(cmode){
	case 1:
	  cmp = VL_CMPDEC;
	  break;
	default:
	  cmp = VL_CMPLEX;
	  break;
	}
	RETVAL = vlopen(name, omode, cmp);
OUTPUT:
	RETVAL


int
plvlclose(villa)
	void *villa
CODE:
	RETVAL = vlclose(villa);
OUTPUT:
	RETVAL


int
plvlput(villa, kbuf, ksiz, vbuf, vsiz, dmode)
	void *	villa
	char *	kbuf
	int	ksiz
	char *	vbuf
	int	vsiz
	int	dmode
CODE:
	RETVAL = vlput(villa, kbuf, ksiz, vbuf, vsiz, dmode);
OUTPUT:
	RETVAL


int
plvlout(villa, kbuf, ksiz)
	void *	villa
	char *	kbuf
	int	ksiz
CODE:
	RETVAL = vlout(villa, kbuf, ksiz);
OUTPUT:
	RETVAL


void
plvlget(villa, kbuf, ksiz)
	void *	villa
	char *	kbuf
	int	ksiz
PREINIT:
	const char *vbuf;
	int vsiz;
PPCODE:
	vbuf = vlgetcache(villa, kbuf, ksiz, &vsiz);
	if(!vbuf) XSRETURN_UNDEF;
	XPUSHs(sv_2mortal(newSVpv(vbuf, vsiz)));
	XSRETURN(1);


int
plvlvsiz(villa, kbuf, ksiz)
	void *	villa
	char *	kbuf
	int	ksiz
CODE:
	RETVAL = vlvsiz(villa, kbuf, ksiz);
OUTPUT:
	RETVAL


int
plvlvnum(villa, kbuf, ksiz)
	void *	villa
	char *	kbuf
	int	ksiz
CODE:
	RETVAL = vlvnum(villa, kbuf, ksiz);
OUTPUT:
	RETVAL


int
plvlcurfirst(villa)
	void *	villa
CODE:
	RETVAL = vlcurfirst(villa);
OUTPUT:
	RETVAL


int
plvlcurlast(villa)
	void *	villa
CODE:
	RETVAL = vlcurlast(villa);
OUTPUT:
	RETVAL


int
plvlcurprev(villa)
	void *	villa
CODE:
	RETVAL = vlcurprev(villa);
OUTPUT:
	RETVAL


int
plvlcurnext(villa)
	void *	villa
CODE:
	RETVAL = vlcurnext(villa);
OUTPUT:
	RETVAL


int
plvlcurjump(villa, kbuf, ksiz, jmode)
	void *	villa
	char *	kbuf
	int	ksiz
	int	jmode
CODE:
	RETVAL = vlcurjump(villa, kbuf, ksiz, jmode);
OUTPUT:
	RETVAL


void
plvlcurkey(villa)
	void *	villa
PREINIT:
	const char *kbuf;
	int ksiz;
PPCODE:
	kbuf = vlcurkeycache(villa, &ksiz);
	if(!kbuf) XSRETURN_UNDEF;
	XPUSHs(sv_2mortal(newSVpv(kbuf, ksiz)));
	XSRETURN(1);


void
plvlcurval(villa)
	void *	villa
PREINIT:
	const char *vbuf;
	int vsiz;
PPCODE:
	vbuf = vlcurvalcache(villa, &vsiz);
	if(!vbuf) XSRETURN_UNDEF;
	XPUSHs(sv_2mortal(newSVpv(vbuf, vsiz)));
	XSRETURN(1);


int
plvlcurput(villa, vbuf, vsiz, cpmode)
	void *	villa
	char *	vbuf
	int	vsiz
	int	cpmode
CODE:
	RETVAL = vlcurput(villa, vbuf, vsiz, cpmode);
OUTPUT:
	RETVAL


int
plvlcurout(villa)
	void *	villa
CODE:
	RETVAL = vlcurout(villa);
OUTPUT:
	RETVAL


void
plvlsettuning(villa, lrecmax, nidxmax, lcnum, ncnum)
	void *	villa
	int	lrecmax
	int	nidxmax
	int	lcnum
	int	ncnum
CODE:
	vlsettuning(villa, lrecmax, nidxmax, lcnum, ncnum);


int
plvlsync(villa)
	void *	villa
CODE:
	RETVAL = vlsync(villa);
OUTPUT:
	RETVAL


int
plvloptimize(villa)
	void *	villa
CODE:
	RETVAL = vloptimize(villa);
OUTPUT:
	RETVAL


int
plvlfsiz(villa)
	void *	villa
CODE:
	RETVAL = vlfsiz(villa);
OUTPUT:
	RETVAL


int
plvlrnum(villa)
	void *	villa
CODE:
	RETVAL = vlrnum(villa);
OUTPUT:
	RETVAL


int
plvlwritable(villa)
	void *	villa
CODE:
	RETVAL = vlwritable(villa);
OUTPUT:
	RETVAL


int
plvlfatalerror(villa)
	void *	villa
CODE:
	RETVAL = vlfatalerror(villa);
OUTPUT:
	RETVAL


int
plvltranbegin(villa)
	void *	villa
CODE:
	RETVAL = vltranbegin(villa);
OUTPUT:
	RETVAL


int
plvltrancommit(villa)
	void *	villa
CODE:
	RETVAL = vltrancommit(villa);
OUTPUT:
	RETVAL


int
plvltranabort(villa)
	void *	villa
CODE:
	RETVAL = vltranabort(villa);
OUTPUT:
	RETVAL



## END OF FILE
