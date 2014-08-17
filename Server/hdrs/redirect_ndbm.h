#ifdef BROKEN_NDBM
  #ifndef QDBM
    #include "../src/gdbm-1.8.3/ndbm.h"
  #else
    #include "../src/qdbm/relic.h"
  #endif
#else
  #include <ndbm.h>
#endif
