#ifdef BROKEN_NDBM
  #ifndef QDBM
    #include "../src/gdbm/ndbm.h"
  #else
    #include "../src/qdbm/relic.h"
  #endif
#else
  #include <ndbm.h>
#endif
