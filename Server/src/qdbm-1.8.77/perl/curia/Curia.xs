/*************************************************************************************************
 * Curia.c
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
#include <curia.h>
#include <stdlib.h>


MODULE = Curia		PACKAGE = Curia
PROTOTYPES: DISABLE



##================================================================================================
## public objects
##================================================================================================


const char *
plcrerrmsg()
CODE:
	RETVAL = dperrmsg(dpecode);
OUTPUT:
	RETVAL


void *
plcropen(name, omode, bnum, dnum)
	char *	name
	int 	omode
	int 	bnum
	int 	dnum
CODE:
	RETVAL = cropen(name, omode, bnum, dnum);
OUTPUT:
	RETVAL


int
plcrclose(curia)
	void *curia
CODE:
	RETVAL = crclose(curia);
OUTPUT:
	RETVAL


int
plcrput(curia, kbuf, ksiz, vbuf, vsiz, dmode)
	void *	curia
	char *	kbuf
	int	ksiz
	char *	vbuf
	int	vsiz
	int	dmode
CODE:
	RETVAL = crput(curia, kbuf, ksiz, vbuf, vsiz, dmode);
OUTPUT:
	RETVAL


int
plcrout(curia, kbuf, ksiz)
	void *	curia
	char *	kbuf
	int	ksiz
CODE:
	RETVAL = crout(curia, kbuf, ksiz);
OUTPUT:
	RETVAL


void
plcrget(curia, kbuf, ksiz, start, max)
	void *	curia
	char *	kbuf
	int	ksiz
	int	start
	int	max
PREINIT:
	char *vbuf;
	int vsiz;
PPCODE:
	vbuf = crget(curia, kbuf, ksiz, start, max, &vsiz);
	if(!vbuf) XSRETURN_UNDEF;
	XPUSHs(sv_2mortal(newSVpv(vbuf, vsiz)));
	free(vbuf);
	XSRETURN(1);


int
plcrvsiz(curia, kbuf, ksiz)
	void *	curia
	char *	kbuf
	int	ksiz
CODE:
	RETVAL = crvsiz(curia, kbuf, ksiz);
OUTPUT:
	RETVAL


int
plcriterinit(curia)
	void *	curia
CODE:
	RETVAL = criterinit(curia);
OUTPUT:
	RETVAL


void
plcriternext(curia)
	void *	curia
PREINIT:
	char *kbuf;
	int ksiz;
PPCODE:
	kbuf = criternext(curia, &ksiz);
	if(!kbuf) XSRETURN_UNDEF;
	XPUSHs(sv_2mortal(newSVpv(kbuf, ksiz)));
	free(kbuf);
	XSRETURN(1);


int
plcrsetalign(curia, align)
	void *	curia
	int	align
CODE:
	RETVAL = crsetalign(curia, align);
OUTPUT:
	RETVAL


int
plcrsetfbpsiz(curia, size)
	void *	curia
	int	size
CODE:
	RETVAL = crsetfbpsiz(curia, size);
OUTPUT:
	RETVAL


int
plcrsync(curia)
	void *	curia
CODE:
	RETVAL = crsync(curia);
OUTPUT:
	RETVAL


int
plcroptimize(curia, bnum)
	void *	curia
	int	bnum
CODE:
	RETVAL = croptimize(curia, bnum);
OUTPUT:
	RETVAL


int
plcrfsiz(curia)
	void *	curia
CODE:
	RETVAL = crfsiz(curia);
OUTPUT:
	RETVAL


int
plcrbnum(curia)
	void *	curia
CODE:
	RETVAL = crbnum(curia);
OUTPUT:
	RETVAL


int
plcrrnum(curia)
	void *	curia
CODE:
	RETVAL = crrnum(curia);
OUTPUT:
	RETVAL


int
plcrwritable(curia)
	void *	curia
CODE:
	RETVAL = crwritable(curia);
OUTPUT:
	RETVAL


int
plcrfatalerror(curia)
	void *	curia
CODE:
	RETVAL = crfatalerror(curia);
OUTPUT:
	RETVAL



## END OF FILE
