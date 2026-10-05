#pragma once

#define HAS_SPINE

#include <cerrno>

#include <span>
#include <vector>
#include <stdexcept>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>
#include <memory>
#include <cstdlib>
#include <new>
#include <iostream>
#include <vector>
#include <typeinfo>
#include <cassert>
#include <variant>
#include <array>
#include <cstddef>
#include <string_view>
#include <utility>
#include <concepts>
#include <type_traits>
#include <functional>
#include <numeric>
#include <float.h>
#include <chrono>
#include <format>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <cmath>
#include <stack>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <ranges>
#include <random>

#define FONS_STATIC

#ifdef TARGET_EMSCRIPTEN
    #include <emscripten.h>
    #include <emscripten/html5.h>
    #include <GLES3/gl3.h>
#endif //TARGET_EMSCRIPTEN

#ifdef TARGET_WIN

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif //TRAGET_WIN

#ifdef TARGET_WIN
    #include <glad/glad.h>
    #define GLFW_INCLUDE_NONE
    #include <GLFW/glfw3.h>
#endif // TARGET_WIN


#define _G2D_NS_ g2d

#define _G2D_NAMESPACE_BEGIN_ namespace _G2D_NS_ {
#define _G2D_NAMESPACE_END_ }


_G2D_NAMESPACE_BEGIN_

#define MY_PI 3.141592653f

typedef const char* LPCCTEXT;
typedef const char* LPCTSTR;

template<typename T>
struct StaticQueue : std::deque<T> {};


typedef std::vector<int> Ints;
typedef Ints::iterator IntsIt;
typedef std::set<std::string> SetStrings;
typedef std::vector<float> Floats;
typedef Floats::iterator   FloatsIt;
typedef std::function<void(void)> SimpleCallback;
typedef std::function<bool(void)> BoolCallback;
typedef std::function<void(bool)> BoolStateCallback;
typedef std::vector<std::string> Strings;

struct Point
{
    float   x;
    float   y;

            Point       (void):x(0),y(0){}
            Point       (const Point&   from){ x = from.x; y = from.y; }
            Point       (float          fx, 
                         float          fy):x(fx),y(fy){};
    Point&  operator =  (const Point&   from){ x = from.x; y = from.y; return (*this);}
    void    set         (float          fx, 
                         float          fy) { x = fx; y = fy;}
    void    copyFrom    (const Point*   pFrom) { x = pFrom->x; y = pFrom->y; }
    float   distance    (float          x2, 
                         float          y2) { return sqrt((x2 - x) * (x2 - x)  +  (y2 - y) * (y2 - y)); }
};
typedef std::vector<Point> Points;

struct Point3
{
    float   x = 0.f;
    float   y = 0.f;
    float   z = 0.f;

    void set(float fx, float fy, float fz) { x = fx; y = fy; z = fz; }
};

struct Rect
{
    float   x;
    float   y;
    float   cx;
    float   cy;
    bool    operator ==             (const Rect&    rc) const { return (x == rc.x && y == rc.y && cx == rc.cx && cy == rc.cy); }
            Rect                    (void):x(0),y(0),cx(0),cy(0){};
            Rect                    (float          fx, 
                                     float          fy, 
                                     float          fcx, 
                                     float          fcy):x(fx),y(fy),cx(fcx),cy(fcy){};
    float   right                   (void) const { return x + cx; };
    float   bottom                  (void) const { return y + cy; };
    void    copyFrom                (Rect*          pFrom){ x = pFrom->x; y = pFrom->y; cx = pFrom->cx; cy = pFrom->cy; }
    void    copyFromRef             (const Rect&    pFrom){ x = pFrom.x; y = pFrom.y; cx = pFrom.cx; cy = pFrom.cy; }
    void    setPos                  (float          fX, 
                                     float          fY) { x = fX; y = fY; }
    void    set                     (float          fx, 
                                     float          fy, 
                                     float          fcx, 
                                     float          fcy) {x = fx; y = fy; cx = fcx; cy = fcy; }
    void    init                    (void) {set(0.f, 0.f, 0.f, 0.f); }
    Point   leftTop                 (void) { return { x, y }; }
    Point   leftMid                 (void) { return { x, y + cy * 0.5f }; }
    Point   centerMid               (void) { return { x + cx * 0.5f, y + cy * 0.5f }; }
    bool    isIntersect             (const Rect*    pRect) const 
    { 
        bool noOverlap = x > pRect->right()  ||
                         pRect->x > right()  ||
                         y > pRect->bottom() ||
                         pRect->y > bottom();
        return !noOverlap;
    };

    bool    checkIntersectByVals    (float          left1, 
                                     float          top1, 
                                     float          right1, 
                                     float          bot1, 
                                     float          left2, 
                                     float          top2, 
                                     float          right2, 
                                     float          bot2)
    { 
        bool noOverlap = left1 > right2  ||
                         left2 > right1  ||
                         top1 > bot2 ||
                         top2 > bot1;
        return !noOverlap;
    };

    void    deflate                 (float          fcx, 
                                     float          fcy){ x += fcx/2.f; y += fcy / 2.f; cx -= fcx; cy -= fcy; };
    void    scale                   (float          fScaleX, float fScaleY) { cx *= fScaleX; cy *= fScaleY; }
    void    offset                  (float          fx, 
                                     float          fy){ x += fx; y += fy;}
    bool    contains                (float          fx, 
                                     float          fy) const {return (fx >= x && fx <= right() && fy >= y && fy <= bottom());}
    void    clipPoint               (Point&         pt) const { if (pt.x < x) pt.x = x; if (pt.y < y) pt.y = y; if (pt.x > x + cx) pt.x = x + cx; if (pt.y > y + cy) pt.y = y + cy;}
    bool    canFit                  (Rect*          pRc) const
    {
        return (x >= pRc->x) && (y >= pRc->y) && (right() <= pRc->right()) && (bottom() <= pRc->bottom());
    }
    void    setRight                (float          newRight){ cx = newRight - x; }
    void    setBottom               (float          newBottom){ cy = newBottom - y; }
    void    unite                   (Rect*          pRect)
    { 
        float fx  = std::min(x, pRect->x); 
        float fy  = std::min(y, pRect->y); 
        float fx1 = std::max(right(), pRect->right()); 
        float fy1 = std::max(bottom(), pRect->bottom()); 
        set(fx, fy, fx1 - fx, fy1 - fy);
    }
    void    normalize               (void)
    {
        float l = x;
        float t = y;
        float r = x + cx;
        float b = y + cy;

        if (l > r)
            std::swap(l, r);
        if (t > b)
            std::swap(t, b);
        set(l, t, r-l, b-t);
    }
};

struct Vec4
{
    float   x1 = 0;
    float   y1 = 0;
    float   x2 = 0;
    float   y2 = 0;

    void    set     (float  x1, 
                     float  y1, 
                     float  x2, 
                     float  y2) { x1 = x1; y1 = y1; x2 = x2; y2 = y2; }
            Vec4    (float  x1 = 0.f, 
                     float  y1 = 0.f, 
                     float  x2 = 0, 
                     float  y2 = 0):x1(x1),y1(y1),x2(x2),y2(y2){}
};

template <typename T, std::size_t N>
constexpr std::size_t SIZE_OF(const T (&)[N]) noexcept {
    static_assert(N > 0, "Array cannot be empty");
    return N;
}

template <typename T>
inline constexpr std::size_t SIZE_OF_T = []{
    static_assert(std::is_array_v<T>, "Template argument must be a raw array type!");
    constexpr std::size_t N = std::extent_v<T>;
    static_assert(N > 0, "Array cannot be empty");
    return N;
}();

_G2D_NAMESPACE_END_