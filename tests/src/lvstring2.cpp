#include "lvstring2.h"
#include <stdio.h>

using namespace lv;

namespace lv {

lChar32 fake_null_buffer_32 = 0;

#ifdef DEBUG_TRACK_LSTRING2_ALLOC
lStringStats ls_alloc_stats;
void lStringStats::dump(const char * msg) {
    printf("stats[%s]:"
           " \talloc=%d \tfree=%d \tactive=%d"
           " \tcopyc=%d \tmovec=%d \tcopyass=%d \tmoveass=%d"
           "\n", msg,
                allocCount, freeCount, allocCount-freeCount,
                copyConstr, moveConstr, copyAssign, moveAssign
            );
}
#endif

}
