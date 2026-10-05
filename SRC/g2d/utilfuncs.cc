#include "utilfuncs.h"

_G2D_NAMESPACE_BEGIN_

char* ufs_itoa(uint32_t x, char* buf, size_t buf_size) 
{
    const size_t max_digits_and_null = 11;
    if (buf_size < max_digits_and_null) {
        return 0;
    }
    char* p = buf + max_digits_and_null;
    *--p = 0;
    do {
        *--p = '0' + (x % 10);
        x /= 10;
    } while (x != 0);
    return p;
}

std::string toFullFilePath(LPCTSTR lpszRelFilePath)
{
    return std::format("EMBED/{}", lpszRelFilePath);
}

_G2D_NAMESPACE_END_