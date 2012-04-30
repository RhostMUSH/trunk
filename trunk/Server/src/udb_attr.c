/*
	Attribute handling gear. Shit simple.

	Andrew Molitor, amolitor@eagle.wesleyan.edu

	1991

*/

#include	"autoconf.h"
#include	"udb.h"
#include	"udb_defs.h"

/*
	Hash function for the cache code. Stolen from mjr.
*/
/*
WARNING  - overflows ignored. no big deal
*/
unsigned int
attr_hash(Aname *key, int width)
{
	return((key->object + key->attrnum) % width);
}

/*
	Free an attribute.
*/

int attrfree(Attr *a)
{
	
	if (!a)
		return 0;

	free((char *)a);
	return 0;
}

/*
	Routines for dealing with reading/writing attributes to disk.

	We stow attributes on disk as an int containing the size,
followed by the data proper. This means two disk hits per operation,
but there seems to be no easy way around this.

*/


Attr	*
attrfromFILE(FILE *  f, int size)
{
	Attr	*atr;

	/* Get a new attribute struct */

	/* Get a buffer big enough. */
	if((atr = (Attr *)malloc(size+1)) == NULL){
		free(atr);
		return(ANULL);
	}

	/* Now go get the data */

	if (fread(atr, size, 1, f) != 1) {
		free(atr);
		return(ANULL);
	}
	atr[size] = '\0';
	return(atr);
}


int attrtoFILE(Attr *a, FILE *  f, int size)
{
	/* Write out the data */

	fwrite(a, size, 1, f);
	return 0;
}

int attr_siz(Attr *a)
{
	return(strlen(a));
}
