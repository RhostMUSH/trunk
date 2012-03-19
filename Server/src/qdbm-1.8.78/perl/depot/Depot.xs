/*************************************************************************************************
 * Depot.c
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
#include <stdlib.h>


MODULE = Depot		PACKAGE = Depot
PROTOTYPES: DISABLE



##================================================================================================
## public objects
##================================================================================================


const char *
pldperrmsg()
CODE:
	RETVAL = dperrmsg(dpecode);
OUTPUT:
	RETVAL


void *
pldpopen(name, omode, bnum)
	char *	name
	int 	omode
	int 	bnum
CODE:
	RETVAL = dpopen(name, omode, bnum);
OUTPUT:
	RETVAL


int
pldpclose(depot)
	void *depot
CODE:
	RETVAL = dpclose(depot);
OUTPUT:
	RETVAL


int
pldpput(depot, kbuf, ksiz, vbuf, vsiz, dmode)
	void *	depot
	char *	kbuf
	int	ksiz
	char *	vbuf
	int	vsiz
	int	dmode
CODE:
	RETVAL = dpput(depot, kbuf, ksiz, vbuf, vsiz, dmode);
OUTPUT:
	RETVAL


int
pldpout(depot, kbuf, ksiz)
	void *	depot
	char *	kbuf
	int	ksiz
CODE:
	RETVAL = dpout(depot, kbuf, ksiz);
OUTPUT:
	RETVAL


void
pldpget(depot, kbuf, ksiz, start, max)
	void *	depot
	char *	kbuf
	int	ksiz
	int	start
	int	max
PREINIT:
	char *vbuf;
	int vsiz;
PPCODE:
	vbuf = dpget(depot, kbuf, ksiz, start, max, &vsiz);
	if(!vbuf) XSRETURN_UNDEF;
	XPUSHs(sv_2mortal(newSVpv(vbuf, vsiz)));
	free(vbuf);
	XSRETURN(1);


int
pldpvsiz(depot, kbuf, ksiz)
	void *	depot
	char *	kbuf
	int	ksiz
CODE:
	RETVAL = dpvsiz(depot, kbuf, ksiz);
OUTPUT:
	RETVAL


int
pldpiterinit(depot)
	void *	depot
CODE:
	RETVAL = dpiterinit(depot);
OUTPUT:
	RETVAL


void
pldpiternext(depot)
	void *	depot
PREINIT:
	char *kbuf;
	int ksiz;
PPCODE:
	kbuf = dpiternext(depot, &ksiz);
	if(!kbuf) XSRETURN_UNDEF;
	XPUSHs(sv_2mortal(newSVpv(kbuf, ksiz)));
	free(kbuf);
	XSRETURN(1);


int
pldpsetalign(depot, align)
	void *	depot
	int	align
CODE:
	RETVAL = dpsetalign(depot, align);
OUTPUT:
	RETVAL


int
pldpsetfbpsiz(depot, size)
	void *	depot
	int	size
CODE:
	RETVAL = dpsetfbpsiz(depot, size);
OUTPUT:
	RETVAL


int
pldpsync(depot)
	void *	depot
CODE:
	RETVAL = dpsync(depot);
OUTPUT:
	RETVAL


int
pldpoptimize(depot, bnum)
	void *	depot
	int	bnum
CODE:
	RETVAL = dpoptimize(depot, bnum);
OUTPUT:
	RETVAL


int
pldpfsiz(depot)
	void *	depot
CODE:
	RETVAL = dpfsiz(depot);
OUTPUT:
	RETVAL


int
pldpbnum(depot)
	void *	depot
CODE:
	RETVAL = dpbnum(depot);
OUTPUT:
	RETVAL


int
pldprnum(depot)
	void *	depot
CODE:
	RETVAL = dprnum(depot);
OUTPUT:
	RETVAL


int
pldpwritable(depot)
	void *	depot
CODE:
	RETVAL = dpwritable(depot);
OUTPUT:
	RETVAL


int
pldpfatalerror(depot)
	void *	depot
CODE:
	RETVAL = dpfatalerror(depot);
OUTPUT:
	RETVAL



## END OF FILE
