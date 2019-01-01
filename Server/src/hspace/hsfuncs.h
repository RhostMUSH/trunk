#ifndef __HSFUNCS_INCLUDED__
#define __HSFUNCS_INCLUDED__

typedef struct
{
	char *name;
	char *(*func)(int, char **);
}HSFUNC;

extern HSFUNC *hsFindFunction(char *);

#define HSPACE_FUNC_PROTO(x)	char *x(int, char **);
#define HSPACE_FUNC_HDR(x)	char *x(int executor, char **args)
#endif // __HSFUNCS_INCLUDED__