#ifndef GR_HASH_H
#define GR_HASH_H

#include <stdint.h>
#include "uint256.h"

uint256 GhostriderHash(const uint8_t* input, size_t len);

#endif // GR_HASH_H
