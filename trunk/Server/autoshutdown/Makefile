
SHELL=/bin/sh
srcdir = .
CC = gcc
CPP = $(CC) -E
OPTIM = -O

# Distribution source files
D_SRC	= autolog.c
D_OBJ	= autolog.o
D_INC	= autolog.h

INSTALL_PROG	= autolog
OUT	= autolog

.c.o:
	$(CC) -c $<

# paths is likely to remain broken
all: $(OUT)

