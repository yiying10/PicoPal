#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "picopal_crc32.h"

int main(void)
{
    static const uint8_t standard_vector[] = "123456789";
    assert(picopal_crc32(standard_vector, 9) == 0xCBF43926U);
    puts("crc32_test: all tests passed");
    return 0;
}
