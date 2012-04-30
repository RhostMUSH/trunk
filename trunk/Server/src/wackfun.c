/* Thorin's version */
#define JUST_LEFT   0
#define JUST_RIGHT  1
#define JUST_JUST   2
#define JUST_CENTER 3

struct wrapinfo {
  char *left;
  char *right;
  int just;
  int width;
};

void safe_substr( char* src, int numchars, char *buff, char **bufcx ) 
{
  int idx;
 
  for( idx = 0; idx < numchars && src[idx]; idx++ ) {
    safe_chr( src[idx], buff, bufcx );
  }
}

void safe_pad( char padch, int numchars, char *buff, char **bufcx )
{
  int idx;

  for( idx = 0; idx < numchars; idx++ ) {
    safe_chr( padch, buff, bufcx );
  }
}

void wrap_out( char* src, int numchars, struct wrapinfo *wp,
               char *buff, char **bufcx )
{
  if( wp->left ) {
    safe_str( wp->left, buff, bufcx );
  }

  switch( wp->just ) {
    case JUST_LEFT:
      safe_substr(src, numchars, buff, bufcx);
      safe_pad(' ', wp->width - numchars, buff, bufcx);
      break;
    case JUST_RIGHT:
      safe_pad(' ', wp->width - numchars, buff, bufcx);
      safe_substr(src, numchars, buff, bufcx);
      break;
    case JUST_CENTER:
      safe_pad(' ', (wp->width - numchars) / 2, buff, bufcx);
      safe_substr(src, numchars, buff, bufcx);
      /* might need to add one due to roundoff error */
      safe_pad(' ', (wp->width - numchars) / 2 + 
                    ((wp->width - numchars) % 2) ? 1 : 0, buff, bufcx);
      break;
    case JUST_JUST:
      /* leave this for later */
      break;
  }

  if( wp->right ) {
    safe_str( wp->right, buff, bufcx );
  }
  safe_str( "\r\n", buff, bufcx );
}

FUNCTION(fun_wrap)
{
  struct wrapinfo winfo;
  int buffleft;
  char* leftstart;
  char *crp;
  char *pp;

  memset(&winfo, 0, sizeof(winfo));

  if (!fn_range_check("WRAP", nfargs, 2, 5, buff, bufcx))
    return;

  winfo.width = atoi( fargs[1] );

  if( winfo.width < 1 ) {
    safe_str( "#-1 WIDTH MUST BE >= 1", buff, bufcx );
    return;
  }

  if( nfargs >= 3 ) { 
    winfo.left = fargs[2];
  }
  if( nfargs >= 4 ) {
    winfo.right = fargs[3];
  }
  if( nfargs >= 5 ) {
    switch( toupper(*fargs[4]) ) {
      case 'L':
        winfo.just = JUST_LEFT;
        break;
      case 'R':
        winfo.just = JUST_RIGHT;
        break;
      case 'J':
        winfo.just = JUST_JUST;
        break;
      case 'C':
        winfo.just = JUST_CENTER;
        break;
      default:
        safe_str("#-1 INVALID JUSTIFICATION SPECIFIED", buff, bufcx);
        return;
    }
  }
  /* setup phase done */

  for( buffleft = strlen(fargs[0]), leftstart = fargs[0]; buffleft; ) {
    crp = strchr(leftstart, '\r');
    if( crp &&
        crp < leftstart + winfo.width ) { /* split here and start over */
      wrap_out( leftstart, crp - leftstart, &winfo, buff, bufcx );
      buffleft -= crp - leftstart + 2;
      leftstart = crp + 2;
    }
    else { /* no \r\n in interesting proximity */
      if( buffleft <= winfo.width ) { /* last line of output */
        wrap_out( leftstart, buffleft, &winfo, buff, bufcx );
        leftstart += buffleft;
        buffleft = 0;
      }
      else {
        /* start at leftstart + width and backtrack to find where to split the
           line */
        /* two cases could happen here, either we find a space, then we break
           there, or we hit the front of the buffer and we have to split
           the line at exactly 'width' characters thereby breaking a word */
        for( pp = leftstart + winfo.width; pp > leftstart; pp-- ) {
          if( *pp == ' ' ) { /* got a space, split here */
            break;
          }
        }
        if( pp == leftstart ) { /* we hit the front of the buffer */
          wrap_out( leftstart, winfo.width, &winfo, buff, bufcx );
          leftstart += winfo.width;
          buffleft -= winfo.width;
        }
        else { /* we hit a space, chop it there */
          wrap_out( leftstart, pp - leftstart, &winfo, buff, bufcx );
          buffleft -= pp - leftstart + 1;
          leftstart += pp - leftstart + 1; /* eat the space */
        }
      }
    }
  }
}
