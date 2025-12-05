#ifndef T
#define T 1
#include <vector>
#endif

               

#ifdef X
#define Y 1
#elif T
#define Y 2
#else
#define Y     3
  #ifdef Z
  #include "test.h"
  #endif
#endif

#line 1
