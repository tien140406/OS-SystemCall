#include "types.h"

struct sysinfo {
    uint64 freemem; // unsigned integer 64-bit
    uint64 nproc;
    uint64 nopenfiles;
};