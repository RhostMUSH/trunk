#ifdef BROKEN_NDBM
    #include "../src/qdbm/relic.h"
#else
  #include <ndbm.h>
#endif
