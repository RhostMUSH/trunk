
/* govern.c -- Hierarchical government/control system */

#include "autoconf.h"
#include "mudconf.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "flags.h"
#include "command.h"
#include "alloc.h"
#include "rhost_ansi.h"
#include "strings.h"


/* switches */

#define LEVEL_PLAYER	0x00000001
#define LEVEL_CREATE	0x00000002
#define LEVEL_DESTROY	0x00000004
#define LEVEL_MOVE	0x00000008
#define LEVEL_LIST	0x00000010

/* for a db upgrade, change the code in both the read and write 
   functions and set the write version higher.
   When the server is run for the first time it will use the version
   number in the file, and only read in that data, but upon next
   write, the new data will be put into the file and the version
   number changed. Make sure to initialize all data elements
   as they might not all be read in.
*/
#define LEVWRITEVERSION		3	/* number of items in structure */

#define LEVNAMELEN		40
#define MAX_ID			0x00FFFFFF
#define MAX_LEV			0x000000FF
#define LEVLEV_DECODE(c)	((c & 0xFF000000) >> 24)
#define LEVID_DECODE(c)		(c & 0x00FFFFFF)
#define LEV_ENCODE(l,i)	((l << 24) | i)
#define LEVUNKNOWN 		0
#define LEVDONTKNOW             0x80000000
#define LEV_LISTNAMELEN		15
#define LEV_LISTWIDTH		(79 / (LEV_LISTNAMELEN + 3))

#define FILENAME		"Rhostshyl"  /* fix */

unsigned int nextid = 1;    	/* 0 is reserved for UNKNOWN */

typedef unsigned int levcode;

unsigned int contbits[LEV_LISTWIDTH / 32 + 1];
char levlistbuff[LEV_LISTWIDTH];

#define GETCONTBIT(x)	((contbits[x / 32] >> (x % 32)) & 0x00000001)
#define SETCONTBIT(x)   (contbits[x / 32] = (contbits[x / 32] \
                         | (0x00000001 << (x % 32))))
#define CLEARCONTBIT(x) (contbits[x / 32] = (contbits[x / 32] \
                         & ~(0x00000001 << (x % 32))))

typedef struct LevFreeNode LevFreeNode;
struct LevFreeNode {
  int id;
  LevFreeNode* next;
};

/* make sure you init each of these in levdb_read */

typedef struct LevNode LevNode;
struct LevNode {
  int id;
  char name[LEVNAMELEN];
  unsigned char mark;
  LevNode* nextlateral;
  LevNode* parent;
  LevNode* firstchild;
  LevNode* nextchild;
  LevNode* lastchild;
};

LevNode* lev_lists[MAX_LEV+1] = { 0 };
LevNode* lev_lists_last[MAX_LEV+1] = { 0 };
LevFreeNode* levfree_list = NULL;
LevFreeNode* levfree_list_last = NULL;

void do_level(dbref, dbref, int, char*, char*);
void do_level_player(dbref, dbref, int, char*, char*);
void do_level_create(dbref, dbref, int, char*, char*);
void do_level_destroy(dbref, dbref, int, char*, char*);
void do_level_move(dbref, dbref, int, char*, char*);
void do_level_list(dbref, dbref, int, char*, char*);
int level_diff(levcode, levcode);
levcode levname2levcode(char *);
LevNode* levname2levnode(char *);
LevNode* levcode2levnode(levcode);
void levdb_write( void );
void levdb_read( void );
void mush_getline(char *, int, FILE*);

int level_diff(levcode player1, levcode player2)
/* returns >0 if player1 higher than player2
   returns 0 if player1 is equal to player2
   return <0 if player1 is lower than player2
   return LEVDONTKNOW if player1 has no relation to player2
*/
{
  unsigned int lev[2],
               id[2];
  int levdiff = 0;
  unsigned int deeper = 0,
               shallower = 1;
  LevNode* temp;

  if( player1 == LEVUNKNOWN ||
      player2 == LEVUNKNOWN ) {
    return LEVDONTKNOW;
  }

  lev[0] = LEVLEV_DECODE(player1);
  lev[1] = LEVLEV_DECODE(player2);
  id[0]  = LEVID_DECODE(player1);
  id[1]  = LEVID_DECODE(player2);

  if( lev[0] > MAX_LEV ||
      lev[1] > MAX_LEV ||
      id[0]  > MAX_ID ||
      id[1]  > MAX_ID ) {
    return LEVDONTKNOW;
  }

  if( lev[0] < lev[1] ) {
    deeper = 1;
    shallower = 0;
  }

  for( temp = lev_lists[deeper];
       temp && temp->id != id[deeper];
       temp = temp->nextlateral ) {
    /* scan */
  }

  for( ; 
       temp && temp->id != id[shallower];
       temp = temp->parent ) {
    levdiff++;
    /* scan */
  }

  if( !temp ) 
    return LEVDONTKNOW;

  if( deeper )
    return levdiff;
  else
    return -levdiff;
}

levcode levname2levcode(char *name)
{
  LevNode* temp;
  int lev;

  for( lev = 0, temp = lev_lists[lev]; 
       lev <= MAX_LEV;
       lev++, temp = lev_lists[lev] ) {
    if( !temp )
      return LEVUNKNOWN;
    for( ; temp; temp = temp->nextlateral ) {
      if( !strcasecmp( temp->name, name ) ) {
        return LEV_ENCODE(lev,temp->id);
      }
    }
  }

  return LEVUNKNOWN;
}

LevNode* levname2levnode(char *name)
{
  LevNode* temp;
  int lev;

  for( lev = 0, temp = lev_lists[lev]; 
       lev <= MAX_LEV;
       lev++, temp = lev_lists[lev] ) {
    if( !temp )
      return NULL;
    for( ; temp; temp = temp->nextlateral ) {
      if( !strcasecmp( temp->name, name ) ) {
        return temp;
      }
    }
  }

  return NULL;
}

LevNode* levcode2levnode(levcode code)
{
  LevNode* temp;
  unsigned int lev;
  unsigned int id;

  lev = LEVLEV_DECODE(code);
  if( lev > MAX_LEV )
    return NULL;

  id = LEVID_DECODE(code);

  for( temp = lev_lists[lev]; 
       temp && temp->id != id; 
       temp = temp->nextlateral ) {
    /* scan */
  }

  return temp;  /* NULL if not found */
}

  

void levdb_write( void )
{
  FILE* outfile;
  char filename[MBUF_SIZE];
  int lev;
  LevNode* temp;
  LevNode* ctemp;
  LevFreeNode* ftemp;
  int line;
  int logged_warning = 0;

  strcpy( filename, FILENAME );
  strcat( filename, ".leveldata" );

  outfile = fopen(filename, "w");

  if( !outfile ) {
    STARTLOG(LOG_PROBLEMS, "LEVEL", "FOPEN")
      log_text("Can't write level data.");
    ENDLOG
    return;
  }

  fprintf(outfile, "STRUCT:\n%d\n", LEVWRITEVERSION);
  fprintf(outfile, "NEXTID:\n%d\n", nextid);
  
  if( levfree_list ) {
    fprintf(outfile, "FREEID:\n");
    for( ftemp = levfree_list; ftemp; ftemp = ftemp->next ) {
      fprintf(outfile, "%d\n", ftemp->id);
    }
    fprintf(outfile, "0\n");
  }

  for( lev = MAX_LEV, temp = lev_lists[lev]; 
       lev >= 0; 
       lev--, temp = lev_lists[lev] ) {
    if( !temp )
      continue;
    fprintf(outfile, "LEVEL:\n%d\n", lev);
    for( ; temp; temp = temp->nextlateral ) {
      line = 0;
      if( line++ < LEVWRITEVERSION ) fprintf(outfile, "%d\n", temp->id);
      if( line++ < LEVWRITEVERSION ) fprintf(outfile, "%s\n", temp->name);
      if( line++ < LEVWRITEVERSION ) {
        for( ctemp = temp->firstchild; ctemp; ctemp = ctemp->nextchild ) {
          fprintf(outfile, "%d\n", ctemp->id);
        }
        fprintf(outfile, "0\n");
      }
      /* insert future structure expansions here */
      fprintf(outfile, "*NODEDONE*\n");
      if( line < LEVWRITEVERSION &&
          !logged_warning ) {
        STARTLOG(LOG_PROBLEMS, "LEVEL", "STRUCT")
          log_text("Warning: Write version too high for available data.");
        ENDLOG
        logged_warning = 1;
      }
    }
    fprintf(outfile, "*LEVELDONE*\n");
  }
  fprintf(outfile, "*DONE*\n");
}

void mush_getline(char *linebuff, int size, FILE* infile)
{
  fgets(linebuff, size, infile);
  linebuff[strlen(linebuff) - 1] = '\0';
}

void levdb_read( void )
{
  FILE* infile;
  char filename[MBUF_SIZE];
  char linebuff[MBUF_SIZE];
  char inbuff[MBUF_SIZE];
  int lev;
  int childid;
  LevNode* temp;
  LevNode* ctemp;
  LevFreeNode* ftemp;
  int line;
  int readversion = 0;
  int gotstruct = 0,
      gotnextid = 0;
      

  strcpy( filename, FILENAME );
  strcat( filename, ".leveldata" );

  infile = fopen(filename, "r");

  if( !infile ) {
    STARTLOG(LOG_PROBLEMS, "LEVEL", "FOPEN")
      log_text("Can't read level data.");
    ENDLOG
    abort();
  }

  mush_getline(linebuff, MBUF_SIZE, infile);

  while( !feof(infile) &&
         strcmp(linebuff, "*DONE*") ) {
    if( !strcmp(linebuff, "STRUCT:") ) {
      mush_getline(inbuff, MBUF_SIZE, infile);
      readversion = atoi(inbuff);
      gotstruct = 1;
    }
    else if( !strcmp(linebuff, "NEXTID:") ) {
      mush_getline(inbuff, MBUF_SIZE, infile);
      nextid = atoi(inbuff);
      gotnextid = 1;
    }
    else if( !strcmp(linebuff, "FREEID:") ) {
      for( mush_getline(inbuff, MBUF_SIZE, infile);
           !feof(infile) && atoi(inbuff);
           mush_getline(inbuff, MBUF_SIZE, infile) ) {
        ftemp = (LevFreeNode*)malloc(sizeof(LevFreeNode));
        if(!ftemp) {
          STARTLOG(LOG_PROBLEMS, "LEVEL", "MALLOC")
            log_text("Can't malloc FreeNode.");
          ENDLOG
          abort();
        }
        ftemp->id = atoi(inbuff);
        ftemp->next = NULL;
          
        if( !levfree_list_last ) {
          levfree_list = ftemp;
          levfree_list_last = ftemp;
        }
        else {
          levfree_list_last->next = ftemp;
          levfree_list_last = ftemp;
        }
      }
    }
    else if( !strcmp(linebuff, "LEVEL:") ) {
      if( !gotstruct ) {
        STARTLOG(LOG_PROBLEMS, "LEVEL", "ORDER")
          log_text("LEVEL tag found before STRUCT.");
        ENDLOG
        abort();
      }
      mush_getline(inbuff, MBUF_SIZE, infile);
      lev = atoi(inbuff);
      for( mush_getline(inbuff, MBUF_SIZE, infile);
           !feof(infile) && strcmp(inbuff, "*LEVELDONE*");
           mush_getline(inbuff, MBUF_SIZE, infile) ) {
        
        temp = (LevNode*)malloc(sizeof(LevNode));
  
        if( !temp ) {
          STARTLOG(LOG_PROBLEMS, "LEVEL", "MALLOC")
            log_text("Can't malloc Node.");
          ENDLOG
          abort();
        }

        /* init all stuff here */
        temp->id = 0;
        temp->name[0] = '\0';
        temp->mark = 0;
        temp->nextlateral = NULL;
        temp->parent = NULL;
        temp->firstchild = NULL;
        temp->nextchild = NULL;
        temp->lastchild = NULL;

        /* we loading the levels from bottom up, if not we're hosed. */
        line = 0;
        if( line++ < readversion ) {
          /* this should be in the buffer already from above */
          temp->id = atoi(inbuff);
        }
        if( line++ < readversion ) {
          mush_getline(inbuff, MBUF_SIZE, infile);
          strncpy(temp->name, inbuff, LEVNAMELEN);
          temp->name[LEVNAMELEN-1] = '\0';
        }
        if( line++ < readversion ) {  /* get children and connect them */
          for(mush_getline(inbuff, MBUF_SIZE, infile);
              !feof(infile) && atoi(inbuff);
              mush_getline(inbuff, MBUF_SIZE, infile) ) {
            childid = atoi(inbuff);
            if( lev < MAX_LEV ) {
              for(ctemp = lev_lists[lev+1]; 
                  ctemp && ctemp->id != childid;
                  ctemp = ctemp->nextlateral) {
                /* scan */
              }
              if( !ctemp ) {
                STARTLOG(LOG_PROBLEMS, "LEVEL", "CHILD")
                  log_text("Can't find child ");
                  log_number(childid);
                  log_text(" of parent ");
                  log_number(lev);
                  log_text(",");
                  log_number(temp->id);
                ENDLOG
                abort();
              }
              if( ctemp->parent ) {
                STARTLOG(LOG_PROBLEMS, "LEVEL", "CHILD")
                  log_text("Child ");
                  log_number(childid);
                  log_text(" of parent ");
                  log_number(lev);
                  log_text(",");
                  log_number(temp->id);
                  log_text(" is already parented.");
                ENDLOG
                abort();
              }
              ctemp->parent = temp;
              if( !temp->lastchild ) {
                temp->firstchild = ctemp;
                temp->lastchild = ctemp;
              }
              else {
                temp->lastchild->nextchild = ctemp;
                temp->lastchild = ctemp;
              }
            }
          }
          mush_getline(inbuff, MBUF_SIZE, infile);
          if(strcmp(inbuff, "*NODEDONE*") ) {
            STARTLOG(LOG_PROBLEMS, "LEVEL", "NODE")
              log_text("Node ");
              log_number(lev);
              log_text(",");
              log_number(temp->id);
              log_text(" missing ending marker.");
            ENDLOG
            abort();
          }
        }

        /* insert new db structures here */

        if( !lev_lists_last[lev] ) {
          lev_lists_last[lev] = temp;
          lev_lists[lev] = temp;
        }
        else {
          lev_lists_last[lev]->nextlateral = temp;
          lev_lists_last[lev] = temp;
        }
      } /* *LEVDONE* */
    } /* level: */
    else {
      STARTLOG(LOG_PROBLEMS, "LEVEL", "TAG")
        log_text("Unknown tag '");
        log_text(linebuff);
        log_text("' at offset ");
        log_number(ftell(infile));
      ENDLOG
      abort();
    }
    mush_getline(linebuff, MBUF_SIZE, infile);
  } /* main tag */

  if( strcmp(linebuff, "*DONE*") ) {
    STARTLOG(LOG_PROBLEMS, "LEVEL", "EOF")
      log_text("Unexpected end of file while reading level data.");
    ENDLOG
    abort();
  }

  /* now scan to make sure we've got complete parentage */
  /* level 0 nodes have no parent */

  for( lev = 1; lev <= MAX_LEV; lev++ ) {
    for( temp = lev_lists[lev]; temp; temp = temp->nextlateral ) {
      if( !temp->parent ) {
        STARTLOG(LOG_PROBLEMS, "LEVEL", "PARENTAGE")
          log_text("Node ");
          log_number(lev);
          log_text(",");
          log_number(temp->id);
          log_text(" has no parent.");
        ENDLOG
        abort();
      }
    }
  }
}

#ifndef LEVEL_SOLO
void do_level(dbref player, dbref cause, int key, char *arg1, char *arg2)
{
  switch(key) {
    case LEVEL_PLAYER:
      do_level_player(player, cause, key, arg1, arg2);
      break;
    case LEVEL_CREATE:
      do_level_create(player, cause, key, arg1, arg2);
      break;
    case LEVEL_DESTROY:
      do_level_destroy(player, cause, key, arg1, arg2);
      break;
    case LEVEL_MOVE:
      do_level_move(player, cause, key, arg1, arg2);
      break;
    case LEVEL_LIST:
      do_level_list(player, cause, key, arg1, arg2);
      break;
    default:
      notify_quiet(player, "Illegal switch combination.");
      break;
  }
}
#endif

void do_level_create(dbref player, dbref cause, int key, 
                     char* arg1, char* arg2)
{
  LevNode* parent;
  LevNode* child;
  LevNode* temp;

  if( !arg1 || !*arg1 ) {
    printf("This command requires at least one argument.\n");
    return;
  }

  if( !arg2 || !*arg2 ) {
    /* perhaps the first insertion */
    if( !lev_lists[0] &&
        /* Immortal(player) */    /* fix */
      ) {
      /* do primary insertion */
      temp = (LevNode*)malloc(sizeof(LevNode));

      if( !temp ) {
        printf("Memory is tight, can't allocate a node.\n");
        STARTLOG(LOG_PROBLEMS, "LEVEL", "CREATE")
          log_text("Non-Fatal: Can't allocate level node.");
        ENDLOG
        return;
      }

      if( nextid > MAX_ID ) {
        printf("Sorry, out of node id numbers.\n");
        free(temp);
        return;
      }
      temp->id = nextid;
      nextid++;
      strncpy(temp->name, arg1, MAXLEVNAMELEN);
      temp->name[MAXLEVNAMELEN - 1] = '\0';
      temp->mark = 0;
      temp->nextlateral = NULL;
      temp->parent = NULL;
      temp->firstchild = NULL;
      temp->nextchild = NULL;
      temp->lastchild = NULL;
      lev_lists[0] = temp;
      printf("Root node added.\n");
      return;
    }
    else {
      printf("This command requires two arguments.\n");
      return;
    }
  }

  if( levname2levnode(arg2) ) {
    printf("There is already a level by the name '%s'.", arg2);
    return;
  }

  parentcode = levname2levcode(arg1);

  if( parentcode == LEVDONTKNOW ) {
    printf("Can't locate the parent node '%s'.\n", arg1);
    return;
  }

  /* got a parent, check for access rights */

  if( level_diff(parentcode, LevelCode(player)) < 0 ) {
    



void level_list_recurse(int nest, int indent,
                        int childstat, LevNode* root)
{
  LevNode* temp;
  static int i, j;

  if( !root ) {
    printf("%s\n", levlistbuff);
    *levlistbuff = '\0';
    return;
  }

  if( nest >= LEV_LISTWIDTH ) {
    strcat(levlistbuff, "...");
    printf("%s\n", levlistbuff);
    *levlistbuff = '\0';
    return;
  }

  if( indent ) {
    for( i = 0; i < LEV_LISTNAMELEN; i++ )
      strcat(levlistbuff, " ");
    for( i = 1; i < indent; i++ ) {
      if( GETCONTBIT(i) )
        strcat(levlistbuff, " | ");
      else
        strcat(levlistbuff, "   ");
      for( j = 0; j < LEV_LISTNAMELEN; j++ ) {
        strcat(levlistbuff, " ");
      }
    }
  }

  if( childstat == 1 ) {
    strcat(levlistbuff, "---");
  }
  else if( childstat == 2 ) {
    strcat(levlistbuff, "-.-");
  }
  else if( childstat == 3 ) {
    strcat(levlistbuff, " `-");
  }
  else if( childstat == 4 ) {
    strcat(levlistbuff, " |-");
  }

  sprintf(levlistbuff + strlen(levlistbuff),
          "%-.*s", LEV_LISTNAMELEN, root->name);

  if( !root->firstchild ) {
    printf("%s\n", levlistbuff);
    *levlistbuff = '\0';
    return;
  }
  else {
    for( i = 0; i < LEV_LISTNAMELEN - strlen(root->name); i++ )
      strcat(levlistbuff, "-");
  }

  SETCONTBIT(nest+1);
  for( temp = root->firstchild;
       temp;
       temp = temp->nextchild ) {

    if( !temp->nextchild ) {
      CLEARCONTBIT(nest+1);
    }
    if( temp == root->firstchild &&
        !temp->nextchild )
      level_list_recurse(nest + 1, 0, 1, temp);
    else if(temp == root->firstchild )
      level_list_recurse(nest + 1, 0, 2, temp);
    else if( !temp->nextchild )
      level_list_recurse(nest + 1, nest + 1, 3, temp);
    else 
      level_list_recurse(nest + 1, nest + 1, 4, temp);
  }
}

void do_level_list(dbref player, dbref cause, int key, 
                   char* arg1, char* arg2)
{
  LevNode* root;

  if( !lev_lists[0] ) {
    printf("There is no hierarchy at present.\n");
    return;
  }

  if( arg2 && *arg2 ) {
    printf("This command takes only one argument.\n");
    return;
  }

  if( !arg1 || !*arg1 ) {
    root = lev_lists[0];
  }
  else {
    root = levname2levnode(arg1);
    if( !root ) {
      printf("I can't find that node.\n");
      return;
    }
  }

  printf("Hierarchy of %s:\n", root->name);
  levlistbuff[0] = '\0';
  level_list_recurse(0, 0, 0, root);
}

#ifdef LEVEL_SOLO
int main( void )
{
  cf_init();
  printf("Reading...\n");
  levdb_read();
  do_level_create(0,0,0,"Burrow", "MudBabies");
  do_level_create(0,0,0,"Burrow", "DustBunnies");
  do_level_create(0,0,0,"MudBabies", "Tiny Tots");
  do_level_create(0,0,0,"Tiny Tots", "Ovum");
  do_level_create(0,0,0,"Ovum", "Zygot");
  do_level_list(0,0,0,"Rhostshyl","");
  do_level_list(0,0,0,"Burrow","");
  do_level_list(0,0,0,"Bogus","");
  do_level_list(0,0,0,"Peon","");
  do_level_list(0,0,0,"","");
  do_level_list(0,0,0,"Central","");
  do_level_list(0,0,0,"Axers","");
  printf("Writing...\n");
  levdb_write();
 
  return 0;
}

#endif
