/* help.h */
#ifndef _M_HELP_H_
#define _M_HELP_H_

#define  LINE_SIZE		90

#define  TOPIC_NAME_LEN		30

typedef struct {
  long pos;			/* index into help file */
  int len;			/* length of help entry */
  char topic[TOPIC_NAME_LEN + 1];	/* topic of help entry */
} help_indx;

#endif
