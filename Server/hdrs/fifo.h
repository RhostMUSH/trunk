/* fifo.h */

#ifndef FI_BSIZE
#define FI_BSIZE 1024
#define FI_BAND 1023
#endif

typedef struct fi_blk FI_BLK;

struct fi_blk
 {
  char blk[FI_BSIZE];
  FI_BLK *next;
 };

typedef struct fifo FIFO;

struct fifo
 {
  FI_BLK *head;
  int hoff;     /* offset into header block */
  FI_BLK *tail;
  int toff;     /* offset into tail block */
  int blocks;   /* # of blocks currently allocated for fifo */
  int maxblk;   /* max # of blocks allowed */
  int flags;
 };

#define FI_CHANGE 1 /* tells fi gets fifo has been updated */
#define FI_FAILED 2 /* fi_gets could not get a line on last try */
#define FI_FREE 4

FIFO *fi_open();
char *fi_gets();
#define fi_readok(fi)  (((fi)->head!=(fi)->tail) || ((fi)->toff!=(fi)->hoff))
#define fi_writeok(fi) ((fi)->blocks<(fi)->maxblk)
