#ifndef _M_UTILS_H_
#define _M_UTILS_H_

////////////////////////////////////////////////////////////////////
// Lensman:
//    A file to hold utility functions that can be used when adding
//    Rhost features.
//    Add cool stuff here!

int bittype(dbref player);

#define RHOSTassert(x) \
    if (! (x) ) { \
        STARTLOG(LOG_BUGS, "ASSERT", "FAIL"); \
        log_text("Assertion failed, raise a bug report: '#x'");  \
        ENDLOG; \
        mudstate.panicking = 1; \
    }

#define RHOSTpanic(x) \
    STARTLOG(LOG_BUGS, "PANIC", "ERR"); \
    log_text(x); \
    ENDLOG; \
    mudstate.panicking = 1;


#endif
