#include <x86intrin.h>

#define READ_ONCE(ptr) *(&ptr)
#define spin_until_cond(condition) \
  {                                \
    while (!condition) {           \
      _mm_pause();                 \
    }                              \
  }
#define smp_wmb() _mm_sfence()
#define smp_rmb() _mm_lfence()
