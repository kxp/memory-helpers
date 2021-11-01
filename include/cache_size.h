#pragma once

// Author: Nick Strupat
// Date: October 29, 2010
// Returns the cache line size (in bytes) of the processor, or 0 on failure
// Extracted from https://github.com/NickStrupat/CacheLineSize
#ifdef __cplusplus
extern "C" {
#endif


#include <stddef.h>
size_t CacheLineSize();

#ifdef __cplusplus
}
#endif
