#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

char* ufs_itoa(uint32_t x, char* buf, size_t buf_size);
std::string toFullFilePath(LPCTSTR lpszRelFilePath);

inline static uint32_t makeRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return (r << 24) | (g << 16) | (b << 8) | a;
}
inline static float degToRad(float d)
{
    return d * (3.14159265358979323846f / 180.0f);
}
inline static float twoPi()
{
    return 6.28318530717958647692f;
}
inline static float clampValue(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

inline static uint32_t fastRandU32()
{
    static uint32_t rngState = 2463534242u;
    rngState = rngState * 1103515245 + 12345;
    return rngState & 0x7FFFFFFF; 
}

inline static float fastRand01()
{
    return static_cast<float>(fastRandU32()) * (1.0f / 2147483647.0f);
}

inline static float randomRange(float lo, float hi)
{
    if (lo > hi) std::swap(lo, hi);
    return lo + fastRand01() * (hi - lo);
}

template <std::ranges::contiguous_range Container>
constexpr auto& getRandomElement(Container& container) 
{
    auto index = fastRandU32() % std::size(container);
    return container[index];
}
inline float fracf(float v)
{
    return v - std::floor(v);
}
inline float hash1(float x)
{
    return fracf(std::sin(x * 12.9898f) * 43758.5453f);
}

inline float hash21(float x, float y)
{
    return fracf(std::sin(x * 127.1f + y * 311.7f) * 43758.5453f);
}
_G2D_NAMESPACE_END_