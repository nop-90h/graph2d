#pragma once

#include "htmldom.h"
#include "sprite.h"
#include "spriteloader.h"
#include "fontstash.h"
#include "utilfuncs.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <span>
#include <stack>
#include <string>
#include <vector>

_G2D_NAMESPACE_BEGIN_

///////////////////////////////////////////////////////////////////////////////
// ������� ������� ����� / ���� / easing
///////////////////////////////////////////////////////////////////////////////

inline unsigned int packRGBA(unsigned int r, unsigned int g, unsigned int b, unsigned int a)
{
    return (r << 24) | (g << 16) | (b << 8) | a;
}

inline float clamp01(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

inline unsigned int lerpColor(unsigned int c1, unsigned int c2, float t)
{
    t = std::clamp(t, 0.f, 1.f);

    auto ch = [](unsigned int c, int sh) {
        return ((c >> sh) & 0xFF) / 255.f;
    };

    return packRGBA(
        (unsigned int)(std::clamp(ch(c1, 24) + (ch(c2, 24) - ch(c1, 24)) * t, 0.f, 1.f) * 255),
        (unsigned int)(std::clamp(ch(c1, 16) + (ch(c2, 16) - ch(c1, 16)) * t, 0.f, 1.f) * 255),
        (unsigned int)(std::clamp(ch(c1, 8)  + (ch(c2, 8)  - ch(c1, 8))  * t, 0.f, 1.f) * 255),
        (unsigned int)(std::clamp(ch(c1, 0)  + (ch(c2, 0)  - ch(c1, 0))  * t, 0.f, 1.f) * 255));
}

inline unsigned int mulAlpha(unsigned int c, float mul)
{
    mul = clamp01(mul);
    return (c & 0xFFFFFF00u) | (unsigned int)(((c & 0xFFu) * mul) + 0.5f);
}

inline float easeOutCubic(float t)
{
    t = clamp01(t);
    float i = 1.0f - t;
    return 1.0f - i * i * i;
}

inline float easeOutBack(float t)
{
    t = clamp01(t);
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float p = t - 1.0f;
    return 1.0f + c3 * p * p * p + c1 * p * p;
}

inline float easeOutBounce(float t)
{
    t = clamp01(t);

    const float n1 = 7.5625f;
    const float d1 = 2.75f;

    if (t < 1.0f / d1)
        return n1 * t * t;

    if (t < 2.0f / d1)
    {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    }

    if (t < 2.5f / d1)
    {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    }

    t -= 2.625f / d1;
    return n1 * t * t + 0.984375f;
}

inline float easeOutExpo(float t)
{
    t = clamp01(t);
    return t >= 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}

inline float appearStagger(float p, int i, int n, float amt)
{
    if (n <= 1)
        return clamp01(p);

    float norm = (float)i / (float)(n - 1);
    return clamp01(p * (1.0f + amt) - amt * norm);
}

///////////////////////////////////////////////////////////////////////////////
// UTF-8 / fake italic
///////////////////////////////////////////////////////////////////////////////

inline constexpr float HTML_FAKE_ITALIC_SKEW = -0.18f;

inline float htmlFakeItalicExtraAdvance(int fontSize)
{
    return std::fabs(HTML_FAKE_ITALIC_SKEW) * (float)fontSize * 0.8f;
}

inline int getUtf8CharLen(const char* s)
{
    unsigned char c = (unsigned char)*s;

    if (c < 0x80) return 1;
    if ((c & 0xe0) == 0xc0) return 2;
    if ((c & 0xf0) == 0xe0) return 3;
    if ((c & 0xf8) == 0xf0) return 4;

    return 1;
}

inline int getOffsetForChars(const char* s, int numChars)
{
    const char* p = s;

    for (int i = 0; i < numChars && *p; i++)
        p += getUtf8CharLen(p);

    return (int)(p - s);
}

///////////////////////////////////////////////////////////////////////////////
// HTML attr / tag helpers
///////////////////////////////////////////////////////////////////////////////

inline HTMLPadding parsePaddingAttr(const HTMLAttrsMap& attrs, float defX, float defY)
{
    HTMLPadding p;
    p.left = p.right = defX;
    p.top = p.bottom = defY;

    auto ptr = attrs.find("padding");
    if (ptr)
    {
        std::string tmp = *ptr;
        for (char& c : tmp)
            if (c == ',' || c == ';')
                c = ' ';

        float v0 = 0, v1 = 0, v2 = 0, v3 = 0;
        int n = sscanf(tmp.c_str(), "%f %f %f %f", &v0, &v1, &v2, &v3);

        if (n == 1)
        {
            p.left = p.right = p.top = p.bottom = v0;
        }
        else if (n == 2)
        {
            p.left = p.right = v0;
            p.top = p.bottom = v1;
        }
        else if (n == 4)
        {
            p.left = v0;
            p.top = v1;
            p.right = v2;
            p.bottom = v3;
        }
        else
        {
            assert(false && "padding supports 1, 2 or 4 values");
        }
    }

    auto one = [&](const char* name, float& dst) {
        auto q = attrs.find(name);
        if (q)
            dst = std::stof(*q);
    };

    one("padding-left",   p.left);
    one("padding-top",    p.top);
    one("padding-right",  p.right);
    one("padding-bottom", p.bottom);

    return p;
}

inline HTMLPadding parseMarginAttr(const HTMLAttrsMap& attrs)
{
    HTMLPadding p;
    p.left = p.right = p.top = p.bottom = 0.f;

    auto ptr = attrs.find("margin");
    if (ptr)
    {
        std::string tmp = *ptr;
        for (char& c : tmp)
            if (c == ',' || c == ';') c = ' ';

        float v[4] = { 0.f, 0.f, 0.f, 0.f };
        int n = sscanf(tmp.c_str(), "%f %f %f %f", &v[0], &v[1], &v[2], &v[3]);

        if (n == 1)      { p.left = p.right = p.top = p.bottom = v[0]; }
        else if (n == 2) { p.left = p.right = v[0]; p.top = p.bottom = v[1]; }
        else if (n == 3) { p.left = v[0]; p.top = p.bottom = v[1]; p.right = v[2]; }
        else if (n == 4) { p.left = v[0]; p.top = v[1]; p.right = v[2]; p.bottom = v[3]; }
    }

    auto one = [&](const char* name, float& dst) {
        auto q = attrs.find(name);
        if (q) dst = std::stof(*q);
    };
    one("margin-left", p.left);
    one("margin-top", p.top);
    one("margin-right", p.right);
    one("margin-bottom", p.bottom);

    return p;
}

inline bool htmlOneOf(const std::string& v, std::initializer_list<const char*> l)
{
    for (auto a : l)
        if (v == a)
            return true;

    return false;
}

inline bool isSupportedHtmlTag(const std::string& t)
{
    return htmlOneOf(t, {
        "br", "hr", "img", "nine", "three", "spine", "table", "tr", "td",
        "font", "b", "strong", "i", "em", "u", "s", "p", "h1", "h2", "h3", "center", "span", "edit",
        "body", "particles"
    });
}

inline bool isSupportedHtmlAttr(const std::string& t, const std::string& a)
{
    if (a == "class")
        return true;
    if (a == "position" || a == "left" || a == "top")
        return true;

    if (t == "particles")
    {
        return htmlOneOf(a, { "preset", "src", "width", "height", "prewarm" });
    }

    if (t == "body")
    {
        return htmlOneOf(a, { "background", "backgroundimage", "bgimage", "bgcolor", "bgopacity",
                              "minwidth", "minheight", "bgsize" });
    }

    if (t == "hr")
    {
        return htmlOneOf(a, { "thickness", "color", "margin", "width" });
    }

    if (t == "img")
    {
        return htmlOneOf(a, {
            "src", "width", "height", "keepaspect",
            "idle", "idledelay", "idledur",
            "glarewidth", "glarespeed", "glaretilt", "glaredir", "glareintensity",
            "pxblock", "pxband", "pxspeed", "pxglow",
            "inline", "valign"
        });
    }

    if (t == "nine")
    {
        return htmlOneOf(a, {
            "src", "width", "height", "a", "b", "c", "d",
            "padding", "padding-left", "padding-top", "padding-right", "padding-bottom"
        });
    }

    if (t == "three")
    {
        return htmlOneOf(a, {
            "src", "width", "height", "a", "b", "c", "d",
            "padding", "padding-left", "padding-top", "padding-right", "padding-bottom",
            "orient", "mirror"
        });
    }

    if (t == "spine")
    {
        return htmlOneOf(a, {
            "src", "path", "anim", "animation", "loop", "delay", "rcbox",
            "width", "height", "keepaspect",
            "padding", "padding-left", "padding-top", "padding-right", "padding-bottom"
        });
    }

    if (t == "table")
    {
        return htmlOneOf(a, {
            "padding", "cellspacing", "align", "valign", "border", "bordercolor",
            "grid", "gridwidth", "gridcolor", "maxwidth"
        });
    }

    if (t == "td")
    {
        return htmlOneOf(a, {
            "bgcolor", "align", "valign", "maxwidth",
            "padding", "padding-left", "padding-top", "padding-right", "padding-bottom"
        });
    }

    if (t == "font")
    {
        return htmlOneOf(a, {
            "name", "size", "color", "fx",
            "shadow", "shadowcolor", "outline", "outlinecolor", "gradient",
            "appear", "dur", "duration", "time", "appearduration",
            "delay", "appeardelay", "fxdelay", "repeatdelay",
            "lineheight", "line-height"
        });
    }

    if (t == "edit")
    {
        return htmlOneOf(a, {
            "width", "height", "src", "a", "b", "c", "d",
            "padding", "padding-left", "padding-top", "padding-right", "padding-bottom",
            "password", "placeholder", "placeholdercolor", "maxlength",
            "font", "name", "fontsize", "size", "color"
        });
    }

    if (t == "span")
    {
        return htmlOneOf(a, { "color", "bgcolor", "size", "name", "bold", "italic" });
    }

    if (t == "u" || t == "s")
    {
        return htmlOneOf(a, { "color", "thickness" });
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
// Text line render helper
///////////////////////////////////////////////////////////////////////////////

enum class eMesureLine
{
    DRAW_INITIAL,
    DRAW,
    MEASURE_INITIAL,
    MEASURE
};

inline void renderAlignedLine(
    FONScontext* fs,
    const char* start,
    const char* end,
    float x,
    float y,
    float maxWidth,
    int align,
    Rect* rc,
    eMesureLine& eM)
{
    float bounds[] = { 0, 0, 0, 0 };

    fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);

    float textWidth = fonsTextBounds(fs, 0, 0, start, end, bounds);
    float renderX = x;

    if (maxWidth > 0)
    {
        switch (align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT))
        {
        case FONS_ALIGN_CENTER:
            renderX += (maxWidth - textWidth) * 0.5f;
            break;

        case FONS_ALIGN_RIGHT:
            renderX += maxWidth - textWidth;
            break;
        }
    }

    Rect renderedRc = {
        renderX + bounds[0],
        y + bounds[1],
        textWidth,
        bounds[3] - bounds[1]
    };

    if (eM == eMesureLine::MEASURE_INITIAL || eM == eMesureLine::DRAW_INITIAL)
    {
        if (eM == eMesureLine::MEASURE_INITIAL)
            eM = eMesureLine::MEASURE;
        else
            eM = eMesureLine::DRAW;

        *rc = renderedRc;
    }
    else
    {
        rc->unite(&renderedRc);
    }

    if (eM == eMesureLine::DRAW_INITIAL || eM == eMesureLine::DRAW)
        fonsDrawText(fs, renderX, y, start, end);
}

///////////////////////////////////////////////////////////////////////////////
// HTML color / image size / rcbox
///////////////////////////////////////////////////////////////////////////////

inline unsigned int parseHTMLColor(const std::string& colorStr)
{
    if (colorStr.empty())
        return 0xFFFFFFFF;

    unsigned int r = 255, g = 255, b = 255, a = 255;

    if (colorStr[0] == '#')
    {
        if (colorStr.length() == 4)
        {
            unsigned int sr, sg, sb;
            sscanf(colorStr.c_str() + 1, "%1x%1x%1x", &sr, &sg, &sb);
            r = sr * 17;
            g = sg * 17;
            b = sb * 17;
        }
        else if (colorStr.length() == 5)
        {
            unsigned int sr, sg, sb, sa;
            sscanf(colorStr.c_str() + 1, "%1x%1x%1x%1x", &sr, &sg, &sb, &sa);
            r = sr * 17;
            g = sg * 17;
            b = sb * 17;
            a = sa * 17;
        }
        else if (colorStr.length() >= 7)
        {
            sscanf(colorStr.c_str() + 1, "%2x%2x%2x", &r, &g, &b);

            if (colorStr.length() >= 9)
                sscanf(colorStr.c_str() + 7, "%2x", &a);
        }
    }

    return packRGBA(r, g, b, a);
}

inline void computeImgSize(
    float origW,
    float origH,
    float reqW,
    float reqH,
    bool keepAspect,
    float& outW,
    float& outH)
{
    if (origW <= 0 || origH <= 0)
    {
        outW = reqW > 0 ? reqW : 0;
        outH = reqH > 0 ? reqH : 0;
        return;
    }

    float ratio = origH / origW;

    if (reqW > 0 && reqH > 0)
    {
        if (keepAspect)
        {
            float s = std::min(reqW / origW, reqH / origH);
            outW = origW * s;
            outH = origH * s;
        }
        else
        {
            outW = reqW;
            outH = reqH;
        }
    }
    else if (reqW > 0)
    {
        outW = reqW;
        outH = reqW * ratio;
    }
    else if (reqH > 0)
    {
        outH = reqH;
        outW = reqH / ratio;
    }
    else
    {
        outW = origW;
        outH = origH;
    }
}

inline unsigned long long s_nextSpineId = 1;

inline bool parseRcBox(const std::string& s, float out[4])
{
    std::string tmp = s;

    for (char& c : tmp)
        if (c == ',' || c == ';')
            c = ' ';

    float v0, v1, v2, v3;

    if (sscanf(tmp.c_str(), "%f %f %f %f", &v0, &v1, &v2, &v3) == 4)
    {
        out[0] = v0;
        out[1] = v1;
        out[2] = v2;
        out[3] = v3;
        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
// Quad / sprite slice render helpers
///////////////////////////////////////////////////////////////////////////////

inline void renderTexQuad(
    CTexturePtr pTex,
    float dx0, float dy0,
    float dx1, float dy1,
    float u0, float v0,
    float u1, float v1,
    float* rgba,
    bool bAdditive, bool bGrayScale = false)
{
    if (!pTex || dx1 <= dx0 || dy1 <= dy0)
        return;

    float verts[8];
    verts[CSprite::VERT_ULX] = dx0; verts[CSprite::VERT_ULY] = dy0;
    verts[CSprite::VERT_URX] = dx1; verts[CSprite::VERT_URY] = dy0;
    verts[CSprite::VERT_BLX] = dx0; verts[CSprite::VERT_BLY] = dy1;
    verts[CSprite::VERT_BRX] = dx1; verts[CSprite::VERT_BRY] = dy1;

    float uvs[8];
    uvs[CSprite::VERT_ULX] = u0; uvs[CSprite::VERT_ULY] = v0;
    uvs[CSprite::VERT_URX] = u1; uvs[CSprite::VERT_URY] = v0;
    uvs[CSprite::VERT_BLX] = u0; uvs[CSprite::VERT_BLY] = v1;
    uvs[CSprite::VERT_BRX] = u1; uvs[CSprite::VERT_BRY] = v1;


    CSprite::renderVerts(pTex, verts, uvs, rgba, bAdditive ? eSpriteBlendMode::ADDITIVE : eSpriteBlendMode::NORMAL, bGrayScale);
}

inline void renderSliceSprite(
    CTexturePtr pTex,
    float uL, float uR, float vT, float vB,
    float srcW, float srcH,
    float x, float y, float w, float h,
    float a, float b, float c, float d,  bool bGrayScale = false)
{
    if (!pTex)
        return;

    if (srcW <= 0 || srcH <= 0 || w <= 0 || h <= 0)
        return;

    // nine-slice has a hard minimum: the caps are unstretchable, so the
    // destination must never be smaller than a+b / c+d. Growing the dest
    // here is a safety net - layout is expected to enforce the minimum
    // already (see HTMLDom Nine/NineButton cases).
    if (w < a + b) w = a + b;
    if (h < c + d) h = c + d;

    if (a + b > srcW) { float k = srcW / (a + b); a *= k; b *= k; }
    if (c + d > srcH) { float k = srcH / (c + d); c *= k; d *= k; }

    float sx[4] = { 0.f, a, srcW - b, srcW };
    float sy[4] = { 0.f, c, srcH - d, srcH };

    float dx[4] = { x, x + a, x + w - b, x + w };
    float dy[4] = { y, y + c, y + h - d, y + h };

    float u[4], v[4];

    for (int i = 0; i < 4; i++)
    {
        u[i] = uL + (uR - uL) * (sx[i] / srcW);
        v[i] = vT + (vB - vT) * (sy[i] / srcH);
    }

    static float white[4] = { 1, 1, 1, 1 };

    for (int ri = 0; ri < 3; ri++)
    {
        for (int ci = 0; ci < 3; ci++)
        {
            renderTexQuad(
                pTex,
                dx[ci], dy[ri], dx[ci + 1], dy[ri + 1],
                u[ci], v[ri], u[ci + 1], v[ri + 1],
                white,
                false, bGrayScale);
        }
    }
}

inline void renderThreeSliceSprite(
    CTexturePtr pTex,
    float uL, float uR, float vT, float vB,
    float srcW, float srcH,
    float x, float y, float w, float h,
    float e0, float e1,
    bool vertical,
    int mirror, bool bGrayScale)
{
    if (!pTex)
        return;

    if (srcW <= 0 || srcH <= 0 || w <= 0 || h <= 0)
        return;

    auto uAt = [&](float sx) {
        return uL + (uR - uL) * (sx / srcW);
    };

    auto vAt = [&](float sy) {
        return vT + (vB - vT) * (sy / srcH);
    };

    float S = vertical ? srcH : srcW;
    float total = vertical ? h : w;

    struct Seg
    {
        float d0, d1, s0, s1;
    };

    Seg seg[3];

    if (mirror == 1)
    {
        float ex = (e1 > 0) ? e1 : e0;
        float cap0 = (e0 > 0) ? e0 : ex;

        if (ex > 0)
        {
            if (cap0 + ex > total)
            {
                float k = total / (cap0 + ex);
                cap0 *= k;
                ex *= k;
            }

            if (ex > S)
                ex = S;

            seg[0] = { 0,          cap0,       S,      S - ex };
            seg[1] = { cap0,       total - ex, 0,      S - ex };
            seg[2] = { total - ex, total,      S - ex, S      };
        }
        else
        {
            mirror = 0;
        }
    }

    if (mirror == 2)
    {
        float ex = (e0 > 0) ? e0 : e1;
        float cap1 = (e1 > 0) ? e1 : ex;

        if (ex > 0)
        {
            if (ex + cap1 > total)
            {
                float k = total / (ex + cap1);
                ex *= k;
                cap1 *= k;
            }

            if (ex > S)
                ex = S;

            seg[0] = { 0,            ex,          0,  ex };
            seg[1] = { ex,           total - cap1, ex, S };
            seg[2] = { total - cap1, total,        ex, 0 };
        }
        else
        {
            mirror = 0;
        }
    }

    if (mirror == 0)
    {
        float a = e0, b = e1;

        if (a + b > total) { float k = total / (a + b); a *= k; b *= k; }
        if (a + b > S)     { float k = S / (a + b);     a *= k; b *= k; }

        seg[0] = { 0,          a,         0,      a     };
        seg[1] = { a,          total - b, a,      S - b };
        seg[2] = { total - b,  total,     S - b,  S     };
    }

    static float white[4] = { 1, 1, 1, 1 };

    for (int i = 0; i < SIZE_OF(seg); i++)
    {
        if (vertical)
        {
            renderTexQuad(
                pTex,
                x, y + seg[i].d0,
                x + w, y + seg[i].d1,
                uL, vAt(seg[i].s0),
                uR, vAt(seg[i].s1),
                white,
                false, bGrayScale);
        }
        else
        {
            renderTexQuad(
                pTex,
                x + seg[i].d0, y,
                x + seg[i].d1, y + h,
                uAt(seg[i].s0), vT,
                uAt(seg[i].s1), vB,
                white,
                false, bGrayScale);
        }
    }
}

inline void renderQuadDirect(
    CTexturePtr pTex,
    float x0, float y0,
    float x1, float y1,
    float u0, float v0,
    float u1, float v1,
    float r, float g, float b, float a,
    eSpriteBlendMode blend, bool bGrayScale = false)
{
    if (!pTex || x1 <= x0 || y1 <= y0)
        return;

    float verts[8] = {
        x0, y0,  // 0: UL
        x1, y0,  // 1: UR
        x1, y1,  // 2: BR
        x0, y1   // 3: BL
    };

    float uvs[8] = {
        u0, v0,  // UL
        u1, v0,  // UR
        u1, v1,  // BR
        u0, v1   // BL
    };

    float rgba[4] = { r, g, b, a };

    CSprite::renderVerts(pTex, verts, uvs, rgba, blend, bGrayScale);
}

///////////////////////////////////////////////////////////////////////////////
// Idle helpers / glare polygon clipping
///////////////////////////////////////////////////////////////////////////////

inline float idleRestartTime(float now, float idleDelay)
{
    if (idleDelay <= 0.0f)
        return now;

    float t = std::fmod(now, idleDelay);

    if (t < 0.0f)
        t += idleDelay;

    return t;
}

struct GlarePoly
{
    enum { MAXV = 16 };

    float x[MAXV];
    float y[MAXV];
    int n = 0;
};

inline void clipGlarePolyEdge(GlarePoly& src, GlarePoly& dst, int axis, float coord, bool keepGreater)
{
    dst.n = 0;

    for (int i = 0; i < src.n; ++i)
    {
        const int j = (i + 1) % src.n;

        const float va = (axis == 0) ? src.x[i] : src.y[i];
        const float vb = (axis == 0) ? src.x[j] : src.y[j];

        const bool ia = keepGreater ? (va >= coord) : (va <= coord);
        const bool ib = keepGreater ? (vb >= coord) : (vb <= coord);

        if (ia && dst.n < GlarePoly::MAXV)
        {
            dst.x[dst.n] = src.x[i];
            dst.y[dst.n] = src.y[i];
            dst.n++;
        }

        if (ia != ib && dst.n < GlarePoly::MAXV)
        {
            const float t = (coord - va) / (vb - va);

            dst.x[dst.n] = src.x[i] + (src.x[j] - src.x[i]) * t;
            dst.y[dst.n] = src.y[i] + (src.y[j] - src.y[i]) * t;
            dst.n++;
        }
    }
}

inline void clipGlarePolyToRect(GlarePoly& poly, float minX, float minY, float maxX, float maxY)
{
    GlarePoly tmp;

    clipGlarePolyEdge(poly, tmp, 0, minX, true);
    poly = tmp;
    if (poly.n < 3) { poly.n = 0; return; }

    clipGlarePolyEdge(poly, tmp, 0, maxX, false);
    poly = tmp;
    if (poly.n < 3) { poly.n = 0; return; }

    clipGlarePolyEdge(poly, tmp, 1, minY, true);
    poly = tmp;
    if (poly.n < 3) { poly.n = 0; return; }

    clipGlarePolyEdge(poly, tmp, 1, maxY, false);
    poly = tmp;
    if (poly.n < 3)
        poly.n = 0;
}

inline void renderAdditivePoly(
    CTexturePtr pTex,
    float uL, float uR, float vT, float vB,
    const GlarePoly& poly,
    float imgX, float imgY, float imgW, float imgH,
    float intensity)
{
    if (!pTex || poly.n < 3)
        return;

    float col[4] = { intensity, intensity, intensity, 1.0f };

    for (int i = 1; i + 1 < poly.n; ++i)
    {
        const float qx[4] = { poly.x[0], poly.x[i], poly.x[i + 1], poly.x[i + 1] };
        const float qy[4] = { poly.y[0], poly.y[i], poly.y[i + 1], poly.y[i + 1] };

        float verts[8];
        verts[CSprite::VERT_ULX] = imgX + qx[0]; verts[CSprite::VERT_ULY] = imgY + qy[0];
        verts[CSprite::VERT_URX] = imgX + qx[1]; verts[CSprite::VERT_URY] = imgY + qy[1];
        verts[CSprite::VERT_BLX] = imgX + qx[2]; verts[CSprite::VERT_BLY] = imgY + qy[2];
        verts[CSprite::VERT_BRX] = imgX + qx[3]; verts[CSprite::VERT_BRY] = imgY + qy[3];

        float uvs[8];
        uvs[CSprite::VERT_ULX] = uL + (uR - uL) * (qx[0] / imgW);
        uvs[CSprite::VERT_ULY] = vT + (vB - vT) * (qy[0] / imgH);

        uvs[CSprite::VERT_URX] = uL + (uR - uL) * (qx[1] / imgW);
        uvs[CSprite::VERT_URY] = vT + (vB - vT) * (qy[1] / imgH);

        uvs[CSprite::VERT_BLX] = uL + (uR - uL) * (qx[2] / imgW);
        uvs[CSprite::VERT_BLY] = vT + (vB - vT) * (qy[2] / imgH);

        uvs[CSprite::VERT_BRX] = uL + (uR - uL) * (qx[3] / imgW);
        uvs[CSprite::VERT_BRY] = vT + (vB - vT) * (qy[3] / imgH);

        CSprite::renderVerts(pTex, verts, uvs, col, eSpriteBlendMode::ADDITIVE);
    }
}

inline float idleAttrDelay(const HTMLNode& node, float defVal)
{
    return node.idleDelay > 0.0f ? node.idleDelay : defVal;
}

///////////////////////////////////////////////////////////////////////////////
// FX / appear style parsing
///////////////////////////////////////////////////////////////////////////////

inline eFxStyle parseFxStyle(const std::string& s)
{
    if (s == "dui") return eFxStyle::DUI;
    if (s == "wave") return eFxStyle::WAVE;
    if (s == "shake") return eFxStyle::SHAKE;
    if (s == "pulse") return eFxStyle::PULSE;
    if (s == "rainbow") return eFxStyle::RAINBOW;
    if (s == "glitch") return eFxStyle::GLITCH;
    if (s == "typewriter") return eFxStyle::TYPEWRITER;
    if (s == "bounce") return eFxStyle::BOUNCE;
    if (s == "rotate") return eFxStyle::ROTATE;
    if (s == "fire") return eFxStyle::FIRE;
    if (s == "blink") return eFxStyle::BLINK;
    if (s == "breathe") return eFxStyle::BREATHE;
    if (s == "sway") return eFxStyle::SWAY;
    if (s == "drift") return eFxStyle::DRIFT;
    if (s == "glow") return eFxStyle::GLOW;
    if (s == "shimmer") return eFxStyle::SHIMMER;
    if (s == "ishimmer") return eFxStyle::ISHIMMER;
    if (s == "candle") return eFxStyle::CANDLE;
    if (s == "mist") return eFxStyle::MIST;
    if (s == "hue") return eFxStyle::HUE;
    if (s == "softfade") return eFxStyle::SOFTFADE;
    if (s == "sparkle") return eFxStyle::SPARKLE;
    if (s == "aurora") return eFxStyle::AURORA;
    if (s == "ocean") return eFxStyle::OCEAN;
    if (s == "sunset") return eFxStyle::SUNSET;
    if (s == "frost") return eFxStyle::FROST;
    if (s == "lava") return eFxStyle::LAVA;
    if (s == "moonlight") return eFxStyle::MOONLIGHT;
    if (s == "starlight") return eFxStyle::STARLIGHT;
    if (s == "neonpulse") return eFxStyle::NEONPULSE;
    if (s == "rose") return eFxStyle::ROSE;
    if (s == "mint") return eFxStyle::MINT;
    if (s == "goldwave") return eFxStyle::GOLDWAVE;
    if (s == "silverwave") return eFxStyle::SILVERWAVE;
    if (s == "pearl") return eFxStyle::PEARL;
    if (s == "opal") return eFxStyle::OPAL;
    if (s == "emerald") return eFxStyle::EMERALD;
    if (s == "sapphire") return eFxStyle::SAPPHIRE;
    if (s == "ruby") return eFxStyle::RUBY;
    if (s == "amethyst") return eFxStyle::AMETHYST;
    if (s == "topaz") return eFxStyle::TOPAZ;
    if (s == "jade") return eFxStyle::JADE;
    if (s == "northern") return eFxStyle::NORTHERN;
    if (s == "tide") return eFxStyle::TIDE;
    if (s == "firefly") return eFxStyle::FIREFLY;
    if (s == "snowfall") return eFxStyle::SNOWFALL;
    if (s == "heartbeat") return eFxStyle::HEARTBEAT;
    if (s == "orbit") return eFxStyle::ORBIT;
    if (s == "blossom") return eFxStyle::BLOSSOM;
    if (s == "ripple") return eFxStyle::RIPPLE;
    if (s == "zen") return eFxStyle::ZEN;
    if (s == "cosmos") return eFxStyle::COSMOS;
    if (s == "electric" || s == "arc" || s == "spark") return eFxStyle::ELECTRIC;
    if (s == "supernova" || s == "nova" || s == "burst") return eFxStyle::SUPERNOVA;
    if (s == "holy" || s == "divine" || s == "blessed") return eFxStyle::HOLY;
    if (s == "poison" || s == "toxic" || s == "acid") return eFxStyle::POISON;
    if (s == "dread" || s == "void" || s == "abyss") return eFxStyle::DREAD;
    if (s == "bloodlust" || s == "blood" || s == "rage" || s == "berserk") return eFxStyle::BLOODLUST;
    if (s == "rift" || s == "chroma" || s == "corrupt") return eFxStyle::RIFT;
    if (s == "phantom" || s == "afterimage" || s == "trail") return eFxStyle::PHANTOM;
    if (s == "mirror" || s == "reflect" || s == "gloss") return eFxStyle::MIRROR;
    if (s == "whirl" || s == "gyrate" || s == "spin") return eFxStyle::WHIRL;
    if (s == "timewarp" || s == "warp") return eFxStyle::TIMEWARP;

    if (s == "spectral" || s == "wraith" || s == "specter") return eFxStyle::SPECTRAL;
    if (s == "magma" || s == "volcano" || s == "embercore") return eFxStyle::MAGMA;
    if (s == "frostbite" || s == "chill" || s == "icebite") return eFxStyle::FROSTBITE;
    if (s == "venom" || s == "venomous" || s == "swamp") return eFxStyle::VENOM;
    if (s == "meteor" || s == "bolide" || s == "shootingstar") return eFxStyle::METEOR;
    if (s == "eclipse" || s == "corona" || s == "umbral") return eFxStyle::ECLIPSE;
    if (s == "tesla" || s == "volt" || s == "coil") return eFxStyle::TESLA;
    if (s == "rosegold" || s == "rosegoldwave" || s == "copper") return eFxStyle::ROSEGOLD;
    if (s == "voidwalk" || s == "shadowstep" || s == "abyssal") return eFxStyle::VOIDWALK;
    if (s == "prism" || s == "iridescent" || s == "chromaprism") return eFxStyle::PRISM;

    return eFxStyle::NONE;
}

inline eAppearStyle parseAppearStyle(const std::string& raw)
{
    std::string s = raw;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });

    if (s == "fade" || s == "alpha") return eAppearStyle::FADE;
    if (s == "rise" || s == "up" || s == "riseup") return eAppearStyle::RISE;
    if (s == "pop" || s == "scale") return eAppearStyle::POP;
    if (s == "cascade" || s == "fall" || s == "drop") return eAppearStyle::CASCADE;
    if (s == "glitch" || s == "glitchin" || s == "glitch_in") return eAppearStyle::GLITCH_IN;
    if (s == "matrix" || s == "code" || s == "digital") return eAppearStyle::MATRIX;
    if (s == "neon" || s == "neonflicker" || s == "flicker") return eAppearStyle::NEON;
    if (s == "typewriter" || s == "type" || s == "print") return eAppearStyle::TYPEWRITER;
    if (s == "comet" || s == "slide" || s == "dash") return eAppearStyle::COMET;
    if (s == "assemble" || s == "materialize" || s == "teleport") return eAppearStyle::ASSEMBLE;
    if (s == "hologram" || s == "holo") return eAppearStyle::HOLOGRAM;
    if (s == "lightning" || s == "thunder" || s == "spark") return eAppearStyle::LIGHTNING;
    if (s == "laserscan" || s == "laser" || s == "scan") return eAppearStyle::LASERSCAN;
    if (s == "vhs" || s == "retro" || s == "vhsretro") return eAppearStyle::VHS;
    if (s == "pixelate" || s == "pixel" || s == "digitize") return eAppearStyle::PIXELATE;
    if (s == "ink" || s == "bleed" || s == "paint") return eAppearStyle::INK;
    if (s == "shatter" || s == "fragments" || s == "reconstruct") return eAppearStyle::SHATTER;
    if (s == "ember" || s == "meteor" || s == "firefly") return eAppearStyle::EMBER;
    if (s == "ghost" || s == "soul" || s == "spirit") return eAppearStyle::GHOST;
    if (s == "runic" || s == "arcane" || s == "magic") return eAppearStyle::RUNIC;

    return eAppearStyle::NONE;
}

///////////////////////////////////////////////////////////////////////////////
// HTML entity / string helpers
///////////////////////////////////////////////////////////////////////////////

inline bool isAsciiSpace(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

inline eVAlign parseVAlign(const std::string& s, eVAlign def)
{
    std::string v;
    v.reserve(s.size());

    for (char c : s)
    {
        unsigned char uc = (unsigned char)c;
        v.push_back(isAsciiSpace(uc) ? ' ' : (char)std::tolower(uc));
    }

    while (!v.empty() && v.front() == ' ')
        v.erase(v.begin());

    while (!v.empty() && v.back() == ' ')
        v.pop_back();

    if (v == "top")
        return eVAlign::TOP;

    if (v == "middle" || v == "center")
        return eVAlign::MIDDLE;

    if (v == "bottom")
        return eVAlign::BOTTOM;

    return def;
}

inline void appendUtf8(std::string& out, unsigned int cp)
{
    if (cp == 0 || cp > 0x10FFFF)
        return;

    if (cp >= 0xD800 && cp <= 0xDFFF)
        return;

    if (cp <= 0x7F)
    {
        out.push_back((char)cp);
    }
    else if (cp <= 0x7FF)
    {
        out.push_back((char)(0xC0 | (cp >> 6)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    }
    else if (cp <= 0xFFFF)
    {
        out.push_back((char)(0xE0 | (cp >> 12)));
        out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    }
    else
    {
        out.push_back((char)(0xF0 | (cp >> 18)));
        out.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    }
}

inline bool parseNumericEntity(const std::string& ent, unsigned int& cp)
{
    if (ent.size() < 2 || ent[0] != '#')
        return false;

    size_t pos = 1;
    int base = 10;

    if (ent[pos] == 'x' || ent[pos] == 'X')
    {
        base = 16;
        pos++;
    }

    if (pos >= ent.size())
        return false;

    unsigned long long val = 0;

    for (; pos < ent.size(); ++pos)
    {
        char c = ent[pos];
        int d;

        if (c >= '0' && c <= '9')
            d = c - '0';
        else if (base == 16 && c >= 'a' && c <= 'f')
            d = 10 + (c - 'a');
        else if (base == 16 && c >= 'A' && c <= 'F')
            d = 10 + (c - 'A');
        else
            return false;

        val = val * base + d;

        if (val > 0x10FFFF)
            return false;
    }

    if (val == 0)
        return false;

    if (val >= 0xD800 && val <= 0xDFFF)
        return false;

    cp = (unsigned int)val;
    return true;
}

inline bool decodeNamedEntity(const std::string& rawName, std::string& out)
{
    std::string n = rawName;
    std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });

    if (n == "amp")    { out += '&';  return true; }
    if (n == "lt")     { out += '<';  return true; }
    if (n == "gt")     { out += '>';  return true; }
    if (n == "quot")   { out += '"';  return true; }
    if (n == "apos")   { out += '\''; return true; }
    if (n == "nbsp")   { out += ' ';  return true; }

    if (n == "ensp")   { out += "\xE2\x80\x82"; return true; }
    if (n == "emsp")   { out += "\xE2\x80\x83"; return true; }
    if (n == "thinsp") { out += "\xE2\x80\x89"; return true; }

    if (n == "copy")   { out += "\xC2\xA9"; return true; }
    if (n == "reg")    { out += "\xC2\xAE"; return true; }
    if (n == "trade")  { out += "\xE2\x84\xA2"; return true; }

    if (n == "deg")    { out += "\xC2\xB0"; return true; }
    if (n == "plusmn") { out += "\xC2\xB1"; return true; }
    if (n == "times")  { out += "\xC3\x97"; return true; }
    if (n == "divide") { out += "\xC3\xB7"; return true; }

    if (n == "hellip") { out += "\xE2\x80\xA6"; return true; }
    if (n == "mdash")  { out += "\xE2\x80\x94"; return true; }
    if (n == "ndash")  { out += "\xE2\x80\x93"; return true; }

    if (n == "laquo")  { out += "\xC2\xAB"; return true; }
    if (n == "raquo")  { out += "\xC2\xBB"; return true; }

    if (n == "lsquo")  { out += "\xE2\x80\x98"; return true; }
    if (n == "rsquo")  { out += "\xE2\x80\x99"; return true; }
    if (n == "ldquo")  { out += "\xE2\x80\x9C"; return true; }
    if (n == "rdquo")  { out += "\xE2\x80\x9D"; return true; }

    if (n == "bull")   { out += "\xE2\x80\xA2"; return true; }
    if (n == "middot") { out += "\xC2\xB7"; return true; }

    if (n == "sect")   { out += "\xC2\xA7"; return true; }
    if (n == "para")   { out += "\xC2\xB6"; return true; }

    if (n == "euro")   { out += "\xE2\x82\xAC"; return true; }
    if (n == "rub")    { out += "\xE2\x82\xBD"; return true; }

    if (n == "frac12") { out += "\xC2\xBD"; return true; }
    if (n == "frac14") { out += "\xC2\xBC"; return true; }
    if (n == "frac34") { out += "\xC2\xBE"; return true; }

    return false;
}

inline std::string decodeHTMLEntities(const std::string& s)
{
    std::string out;
    out.reserve(s.size());

    size_t i = 0;

    while (i < s.size())
    {
        if (s[i] != '&')
        {
            out.push_back(s[i]);
            ++i;
            continue;
        }

        size_t semi = s.find(';', i + 1);

        if (semi == std::string::npos || semi <= i + 1 || semi - i > 32)
        {
            out.push_back(s[i]);
            ++i;
            continue;
        }

        std::string ent = s.substr(i + 1, semi - i - 1);

        if (ent.empty())
        {
            out.push_back(s[i]);
            ++i;
            continue;
        }

        if (ent[0] == '#')
        {
            unsigned int cp;

            if (parseNumericEntity(ent, cp))
            {
                appendUtf8(out, cp);
                i = semi + 1;
                continue;
            }
        }
        else if (decodeNamedEntity(ent, out))
        {
            i = semi + 1;
            continue;
        }

        out.push_back(s[i]);
        ++i;
    }

    return out;
}

inline bool parseHTMLTag(const std::string& tagStr, std::string& name, HTMLAttrsMap& attrs, bool& isClosing)
{
    if (tagStr.empty() || tagStr[0] != '<')
        return false;

    size_t pos = 1;

    isClosing = (pos < tagStr.length() && tagStr[pos] == '/');
    if (isClosing)
        pos++;

    size_t nameStart = pos;

    while (pos < tagStr.length() &&
           !isAsciiSpace((unsigned char)tagStr[pos]) &&
           tagStr[pos] != '>' &&
           tagStr[pos] != '/')
    {
        pos++;
    }

    name = tagStr.substr(nameStart, pos - nameStart);

    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });

    assert(isSupportedHtmlTag(name) && "Unsupported HTML tag");

    int attrCount = 0;

    while (pos < tagStr.length() && tagStr[pos] != '>' && tagStr[pos] != '/')
    {
        while (pos < tagStr.length() && isAsciiSpace((unsigned char)tagStr[pos]))
            pos++;

        if (pos >= tagStr.length() || tagStr[pos] == '>' || tagStr[pos] == '/')
            break;

        size_t attrNameStart = pos;

        while (pos < tagStr.length() &&
               tagStr[pos] != '=' &&
               !isAsciiSpace((unsigned char)tagStr[pos]) &&
               tagStr[pos] != '>' &&
               tagStr[pos] != '/')
        {
            pos++;
        }

        std::string attrName = tagStr.substr(attrNameStart, pos - attrNameStart);

        std::transform(attrName.begin(), attrName.end(), attrName.begin(), [](unsigned char c) {
            return (char)std::tolower(c);
        });

        while (pos < tagStr.length() && isAsciiSpace((unsigned char)tagStr[pos]))
            pos++;

        std::string attrVal = "";

        if (pos < tagStr.length() && tagStr[pos] == '=')
        {
            pos++;

            if (pos < tagStr.length() && (tagStr[pos] == '"' || tagStr[pos] == '\''))
            {
                char quote = tagStr[pos++];
                size_t valStart = pos;

                while (pos < tagStr.length() && tagStr[pos] != quote)
                    pos++;

                attrVal = tagStr.substr(valStart, pos - valStart);

                if (pos < tagStr.length())
                    pos++;
            }
            else
            {
                size_t valStart = pos;

                while (pos < tagStr.length() &&
                       !isAsciiSpace((unsigned char)tagStr[pos]) &&
                       tagStr[pos] != '>')
                {
                    pos++;
                }

                attrVal = tagStr.substr(valStart, pos - valStart);
            }
        }

        if (!attrName.empty())
        {
            assert(isSupportedHtmlAttr(name, attrName) && "Unsupported HTML attribute");
            assert(attrCount < 20 && "HTML tag has too many attributes (max 20)");

            attrs.insert(attrName, decodeHTMLEntities(attrVal));
            attrCount++;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// HTML whitespace trimming
///////////////////////////////////////////////////////////////////////////////

inline bool htmlStringIsWhiteSpace(const std::string& s)
{
    for (char c : s)
        if (!isAsciiSpace((unsigned char)c))
            return false;

    return true;
}

inline bool htmlStringHasNonWhiteSpace(const std::string& s)
{
    for (char c : s)
        if (!isAsciiSpace((unsigned char)c))
            return true;

    return false;
}

inline void htmlTrimLeft(std::string& s)
{
    size_t p = 0;

    while (p < s.size() && isAsciiSpace((unsigned char)s[p]))
        p++;

    if (p > 0)
        s.erase(0, p);
}

inline void htmlTrimRight(std::string& s)
{
    while (!s.empty() && isAsciiSpace((unsigned char)s.back()))
        s.pop_back();
}

inline void htmlCollapseAsciiWhiteSpace(std::string& s)
{
    std::string out;
    out.reserve(s.size());

    bool sp = false;

    for (char c : s)
    {
        if (isAsciiSpace((unsigned char)c))
        {
            if (!sp)
                out.push_back(' ');

            sp = true;
        }
        else
        {
            out.push_back(c);
            sp = false;
        }
    }

    s.swap(out);
}

inline int htmlFindFirstContentIndex(const HTMLDocument& doc)
{
    for (int i = 0; i < (int)doc.size(); ++i)
    {
        const auto& n = doc[i];

        if (n.type == HTMLNode::Type::Tag)
            continue;

        if (n.type == HTMLNode::Type::Text)
        {
            if (htmlStringHasNonWhiteSpace(n.text))
                return i;
        }
        else
        {
            return i;
        }
    }

    return -1;
}

inline int htmlFindLastContentIndex(const HTMLDocument& doc)
{
    for (int i = (int)doc.size() - 1; i >= 0; --i)
    {
        const auto& n = doc[i];

        if (n.type == HTMLNode::Type::Tag)
            continue;

        if (n.type == HTMLNode::Type::Text)
        {
            if (htmlStringHasNonWhiteSpace(n.text))
                return i;
        }
        else
        {
            return i;
        }
    }

    return -1;
}

inline void trimHTMLDocumentWhiteSpace(HTMLDocument& doc, bool collapseInnerSpaces);

inline void trimHTMLNodeWhiteSpace(HTMLNode& node, bool collapseInnerSpaces)
{
    if (node.type == HTMLNode::Type::Text && collapseInnerSpaces)
        htmlCollapseAsciiWhiteSpace(node.text);

    if (node.childDoc)
        trimHTMLDocumentWhiteSpace(*node.childDoc, collapseInnerSpaces);

    if (node.tableData)
    {
        for (auto& row : node.tableData->rows)
            for (auto& cell : row.cells)
                if (cell.contentDoc)
                    trimHTMLDocumentWhiteSpace(*cell.contentDoc, collapseInnerSpaces);
    }
}

inline void trimHTMLDocumentWhiteSpace(HTMLDocument& doc, bool collapseInnerSpaces)
{
    for (auto& node : doc)
        trimHTMLNodeWhiteSpace(node, collapseInnerSpaces);

    bool changed = true;

    while (changed)
    {
        changed = false;

        int first = htmlFindFirstContentIndex(doc);

        if (first < 0)
        {
            for (int i = (int)doc.size() - 1; i >= 0; --i)
            {
                if (doc[i].type == HTMLNode::Type::Text && htmlStringIsWhiteSpace(doc[i].text))
                {
                    doc.erase(doc.begin() + i);
                    changed = true;
                }
            }

            return;
        }

        for (int i = first - 1; i >= 0; --i)
        {
            if (doc[i].type == HTMLNode::Type::Text && htmlStringIsWhiteSpace(doc[i].text))
            {
                doc.erase(doc.begin() + i);
                changed = true;
            }
        }

        first = htmlFindFirstContentIndex(doc);

        if (first >= 0 && doc[first].type == HTMLNode::Type::Text)
        {
            auto& t = doc[first].text;
            size_t old = t.size();

            htmlTrimLeft(t);

            if (t.size() != old)
                changed = true;

            if (htmlStringIsWhiteSpace(t))
            {
                doc.erase(doc.begin() + first);
                changed = true;
            }
        }
    }

    changed = true;

    while (changed)
    {
        changed = false;

        int last = htmlFindLastContentIndex(doc);

        if (last < 0)
        {
            for (int i = (int)doc.size() - 1; i >= 0; --i)
            {
                if (doc[i].type == HTMLNode::Type::Text && htmlStringIsWhiteSpace(doc[i].text))
                {
                    doc.erase(doc.begin() + i);
                    changed = true;
                }
            }

            return;
        }

        for (int i = (int)doc.size() - 1; i > last; --i)
        {
            if (doc[i].type == HTMLNode::Type::Text && htmlStringIsWhiteSpace(doc[i].text))
            {
                doc.erase(doc.begin() + i);
                changed = true;
            }
        }

        last = htmlFindLastContentIndex(doc);

        if (last >= 0 && doc[last].type == HTMLNode::Type::Text)
        {
            auto& t = doc[last].text;
            size_t old = t.size();

            htmlTrimRight(t);

            if (t.size() != old)
                changed = true;

            if (htmlStringIsWhiteSpace(t))
            {
                doc.erase(doc.begin() + last);
                changed = true;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Table marker search helpers
///////////////////////////////////////////////////////////////////////////////

inline bool htmlTagBoundaryChar(char c)
{
    return c == ' ' || c == '>' || c == '\t' || c == '/' || c == '\n' || c == '\r';
}

inline size_t findTableMarker(const std::string& s, const char* pat, size_t from)
{
    const size_t plen = strlen(pat);

    size_t p = from;

    while (p < s.length())
    {
        p = s.find(pat, p);

        if (p == std::string::npos)
            return std::string::npos;

        if (pat[plen - 1] == '>')
            return p;

        char c = (p + plen < s.length()) ? s[p + plen] : '>';

        if (htmlTagBoundaryChar(c))
            return p;

        p += plen;
    }

    return std::string::npos;
}

inline size_t findTopLevelMarker(const std::string& s, const char* target, size_t from)
{
    size_t scan = from;
    int depth = 0;

    while (scan < s.length())
    {
        const size_t openT = findTableMarker(s, "<table", scan);
        const size_t closeT = findTableMarker(s, "</table>", scan);
        const size_t tgt = findTableMarker(s, target, scan);

        if (tgt == std::string::npos)
            return std::string::npos;

        if (openT != std::string::npos && openT < tgt && (closeT == std::string::npos || openT < closeT))
        {
            ++depth;
            scan = openT + 6;
            continue;
        }

        if (closeT != std::string::npos && closeT < tgt)
        {
            if (depth > 0)
                --depth;

            scan = closeT + 8;
            continue;
        }

        if (depth == 0)
            return tgt;

        scan = tgt + strlen(target);
    }

    return std::string::npos;
}

///////////////////////////////////////////////////////////////////////////////
// Render state structures used by renderHTML
///////////////////////////////////////////////////////////////////////////////

struct HTMLRenderState
{
    std::string fontName;

    int fontHandle = -1;
    int baseFontHandle = -1;
    int fontSize = 16;

    unsigned int color = 0xFFFFFFFF;
    int align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;

    eFxStyle fxStyle = eFxStyle::NONE;

    float shadow = 0;
    unsigned int shadowColor = 0x000000FF;

    float outline = 0;
    unsigned int outlineColor = 0x000000FF;

    bool underline = false;
    bool strikethrough = false;

    unsigned int lineColor = 0xFFFFFFFF;
    float lineThickness = 0.0f;

    bool hasGradient = false;
    unsigned int gradientColor1 = 0xFFFFFFFF;
    unsigned int gradientColor2 = 0xFFFFFFFF;

    eAppearStyle appearStyle = eAppearStyle::NONE;
    float appearDur = 0.8f;
    float appearDelay = 0.0f;

    float fxDelay = 0.0f;

    bool bold = false;
    bool italic = false;
    bool fakeBold = false;
    bool fakeItalic = false;

    float lineHeightMul = 0.f;
    unsigned int bgColor = 0;
};

struct TextChunk
{
    std::string text;
    HTMLRenderState state;

    bool isImage = false;
    CTexturePtr imgTex;
    float imgUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float imgW = 0.f;
    float imgH = 0.f;
    int imgValign = 1;
    unsigned int imgColor = 0xFFFFFFFF;
    float imgMarginL = 0.f;
    unsigned long long imgNodeId = 0;
};

struct LineBuffer
{
    std::vector<TextChunk> chunks;
    float totalWidth = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Appear effects
///////////////////////////////////////////////////////////////////////////////

struct AppearFx
{
    bool visible = true;
    float dx = 0;
    float dy = 0;
    int size = 16;
    unsigned int color = 0xFFFFFFFF;
};

inline AppearFx evalAppearEffect(
    eAppearStyle style,
    float dur,
    float delay,
    float startTime,
    float now,
    int charIndex,
    int totalChars,
    int baseSize,
    unsigned int baseColor)
{
    AppearFx r;
    r.color = baseColor;
    r.size = baseSize;

    if (style == eAppearStyle::NONE)
        return r;

    float local = now - startTime - delay;

    if (local < 0)
    {
        r.visible = false;
        return r;
    }

    if (dur <= 0)
        return r;

    float p = clamp01(local / dur);

    if (p >= 1)
        return r;

    float cp = 0;
    float e = 0;

    switch (style)
    {
    case eAppearStyle::FADE:
        r.color = mulAlpha(baseColor, easeOutCubic(appearStagger(p, charIndex, totalChars, 0.25f)));
        break;

    case eAppearStyle::RISE:
        cp = appearStagger(p, charIndex, totalChars, 0.55f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }
        e = easeOutCubic(cp);
        r.dy = (1 - e) * baseSize * 1.35f;
        r.color = mulAlpha(baseColor, e);
        break;

    case eAppearStyle::POP:
    {
        cp = appearStagger(p, charIndex, totalChars, 0.7f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }

        float s = easeOutBack(cp);
        if (s <= 0)
        {
            r.visible = false;
            break;
        }

        r.size = std::max(1, (int)(baseSize * s));
        r.color = mulAlpha(baseColor, clamp01(cp * 3));
        break;
    }

    case eAppearStyle::CASCADE:
        cp = appearStagger(p, charIndex, totalChars, 0.75f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }
        e = easeOutBounce(cp);
        r.dy = -(1 - e) * baseSize * 2.8f;
        r.color = mulAlpha(baseColor, clamp01(cp * 2));
        break;

    case eAppearStyle::GLITCH_IN:
    {
        cp = appearStagger(p, charIndex, totalChars, 0.35f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }

        e = easeOutCubic(cp);

        float fr = std::floor(now * 30);
        float h1 = hash21(charIndex * 13.7f, fr);
        float h2 = hash21(charIndex * 91.3f, fr + 7);

        float mag = (1 - e) * baseSize * 0.65f;

        r.dx = (h1 - .5f) * 2 * mag;
        r.dy = (h2 - .5f) * 2 * mag;

        r.color = lerpColor(baseColor, (h1 > 0.66f) ? 0x00FFFFFFu : 0xFF00FFFFu, (1 - e) * 0.35f);
        r.color = mulAlpha(r.color, e);
        break;
    }

    case eAppearStyle::MATRIX:
        cp = appearStagger(p, charIndex, totalChars, 0.8f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }
        e = easeOutCubic(cp);
        r.color = lerpColor(baseColor, 0x00FF66FFu, (1 - e) * 0.85f);
        r.color = mulAlpha(r.color, e);
        r.dy = -(1 - e) * baseSize * 0.12f;
        break;

    case eAppearStyle::NEON:
    {
        cp = p;
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }

        e = easeOutCubic(cp);

        bool on = hash21(charIndex * 7.77f, std::floor(now * 24)) < (0.22f + 0.78f * e);

        if (!on && p < 0.97f)
        {
            r.visible = false;
            break;
        }

        r.color = lerpColor(baseColor, 0xFFFFFFFFu, (1 - e) * 0.55f);
        r.color = mulAlpha(r.color, e * (on ? 1 : 0.35f));
        break;
    }

    case eAppearStyle::TYPEWRITER:
    {
        if (totalChars <= 0)
            break;

        float reveal = p * totalChars;

        if ((float)charIndex >= reveal)
        {
            r.visible = false;
            break;
        }

        r.color = mulAlpha(baseColor, clamp01((reveal - charIndex) * 4));
        break;
    }

    case eAppearStyle::COMET:
        cp = appearStagger(p, charIndex, totalChars, 0.5f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }
        e = easeOutExpo(cp);
        r.dx = -(1 - e) * baseSize * 3.5f;
        r.color = lerpColor(baseColor, 0xBFEFFFFFu, (1 - e) * 0.45f);
        r.color = mulAlpha(r.color, e);
        break;

    case eAppearStyle::ASSEMBLE:
    {
        cp = appearStagger(p, charIndex, totalChars, 0.75f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }

        e = easeOutCubic(cp);

        float an = hash1((float)charIndex) * 6.2831853f;
        float d = (1 - e) * baseSize * 4;

        r.dx = std::cos(an) * d;
        r.dy = std::sin(an) * d;

        r.color = lerpColor(baseColor, 0x808080FFu, (1 - e) * 0.35f);
        r.color = mulAlpha(r.color, e);
        break;
    }

    case eAppearStyle::HOLOGRAM:
    {
        cp = appearStagger(p, charIndex, totalChars, 0.35f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }

        e = easeOutCubic(cp);

        float h = hash21(charIndex * 3.1f, std::floor(now * 40));

        bool on = h < (0.3f + 0.7f * e);

        if (!on && p < 0.95f)
        {
            r.visible = false;
            break;
        }

        r.dy = -(1 - e) * baseSize * 0.2f;
        r.dx = (h - .5f) * (1 - e) * baseSize * 0.18f;

        r.color = lerpColor(baseColor, 0x00FFFFFFu, (1 - e) * 0.55f);
        r.color = mulAlpha(r.color, e * (on ? 1 : 0.4f));
        break;
    }

    case eAppearStyle::LIGHTNING:
    {
        cp = appearStagger(p, charIndex, totalChars, 0.5f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }

        e = easeOutCubic(cp);

        float fr = std::floor(now * 35);
        float h1 = hash21(charIndex * 7.7f, fr);
        float h2 = hash21(charIndex * 13.1f, fr);
        float h3 = hash21(charIndex * 29.7f, fr + 5);

        bool flash = h1 > 0.78f && p < 0.92f;

        float mag = (1 - e) * baseSize * 0.35f;

        r.dx = (h2 - .5f) * 2 * mag;
        r.dy = (h3 - .5f) * 2 * mag * 0.7f;

        r.color = lerpColor(baseColor, flash ? 0xEFFFFFFFu : 0x80BFFFFFu, flash ? 0.85f : (1 - e) * 0.35f);
        r.color = mulAlpha(r.color, e * (flash ? 1 : 0.9f));
        break;
    }

    case eAppearStyle::LASERSCAN:
    {
        if (totalChars <= 0)
            break;

        float pos = totalChars > 1 ? (float)charIndex / (totalChars - 1) : 0;
        float lc = clamp01((p * 1.25f - pos) * 6);

        if (lc <= 0)
        {
            r.visible = false;
            break;
        }

        e = easeOutCubic(lc);

        if (lc < 0.35f)
        {
            r.size = std::max(1, (int)(baseSize * (1 + 0.18f * (1 - lc))));
            r.color = lerpColor(baseColor, 0xEFFFFFFFu, 0.75f);
        }
        else
        {
            r.color = baseColor;
        }

        r.color = mulAlpha(r.color, e);
        break;
    }

    case eAppearStyle::VHS:
    {
        cp = appearStagger(p, charIndex, totalChars, 0.25f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }

        e = easeOutCubic(cp);

        float fr = std::floor(now * 18);
        float h1 = hash21(charIndex * 17.3f, fr);
        float h2 = hash21(charIndex * 53.7f, fr + 2);
        float h3 = hash21(charIndex * 91.1f, fr + 9);

        if (h1 < 0.12f && p < 0.88f)
        {
            r.visible = false;
            break;
        }

        float mag = (1 - e) * baseSize * 0.55f;

        r.dx = (h2 - .5f) * 2 * mag;
        r.dy = (h3 - .5f) * 2 * mag * 0.25f;

        unsigned int tint = 0xFFFFFFFFu;

        if (h1 > 0.66f)
            tint = 0xFF4D4DFFu;
        else if (h1 > 0.33f)
            tint = 0x4DFFFFFFu;

        r.color = lerpColor(baseColor, tint, (1 - e) * 0.45f);
        r.color = mulAlpha(r.color, e);
        break;
    }

    case eAppearStyle::PIXELATE:
    {
        cp = appearStagger(p, charIndex, totalChars, 0.55f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }

        e = easeOutCubic(cp);

        float q = std::floor(e * 6) / 6;

        if (q <= 0)
        {
            r.visible = false;
            break;
        }

        float h1 = hash1(charIndex * 11.f);
        float h2 = hash1(charIndex * 29.f);

        float d = (1 - q) * baseSize * 2.2f;

        r.dx = (h1 - .5f) * 2 * d;
        r.dy = (h2 - .5f) * 2 * d;

        r.color = lerpColor(baseColor, 0x9F9FFFFFu, (1 - q) * 0.35f);
        r.color = mulAlpha(r.color, q);
        break;
    }

    case eAppearStyle::INK:
    {
        cp = appearStagger(p, charIndex, totalChars, 0.65f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }

        e = easeOutCubic(cp);

        float h = hash1(charIndex * 5.f);
        float reveal = clamp01(e * 1.3f - h * 0.25f);

        if (reveal <= 0)
        {
            r.visible = false;
            break;
        }

        r.dy = (1 - e) * baseSize * 0.25f;
        r.color = lerpColor(0x05070DFFu, baseColor, e);
        r.color = mulAlpha(r.color, reveal);
        break;
    }

    case eAppearStyle::SHATTER:
    {
        cp = appearStagger(p, charIndex, totalChars, 0.7f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }

        float mv = easeOutCubic(cp);
        float sc = easeOutBack(cp);

        if (sc <= 0)
        {
            r.visible = false;
            break;
        }

        float an = hash1(charIndex * 23.f) * 6.2831853f;
        float d = (1 - mv) * baseSize * 5;

        r.dx = std::cos(an) * d;
        r.dy = std::sin(an) * d;

        r.size = std::max(1, (int)(baseSize * sc));

        r.color = lerpColor(baseColor, 0xC0C0C0FFu, (1 - mv) * 0.35f);
        r.color = mulAlpha(r.color, clamp01(cp * 2.5f));
        break;
    }

    case eAppearStyle::EMBER:
    {
        cp = appearStagger(p, charIndex, totalChars, 0.6f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }

        e = easeOutExpo(cp);

        float h = hash21(charIndex * 19.3f, std::floor(now * 28));

        r.dx = (1 - e) * baseSize * 2.2f;
        r.dy = -(1 - e) * baseSize * 3.2f;

        r.color = lerpColor(baseColor, (h > 0.75f) ? 0xFFE08CFFu : 0xFF7A1AFFu, (1 - e) * 0.85f);
        r.color = mulAlpha(r.color, e);
        break;
    }

    case eAppearStyle::GHOST:
    {
        cp = appearStagger(p, charIndex, totalChars, 0.7f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }

        e = easeOutCubic(cp);

        float w1 = std::sin(now * 2.2f + charIndex * 0.7f);
        float w2 = std::sin(now * 1.6f + charIndex);

        r.dx = w1 * (1 - e) * baseSize * 0.18f;
        r.dy = (1 - e) * baseSize * 1.1f + w2 * (1 - e) * baseSize * 0.08f;

        r.color = lerpColor(baseColor, 0xCFFFFFFFu, (1 - e) * 0.45f);
        r.color = mulAlpha(r.color, e * e);
        break;
    }

    case eAppearStyle::RUNIC:
    {
        cp = appearStagger(p, charIndex, totalChars, 0.75f);
        if (cp <= 0)
        {
            r.visible = false;
            break;
        }

        e = easeOutCubic(cp);

        float pulse = .5f + .5f * std::sin(now * 6 + charIndex * 0.7f);

        r.size = std::max(1, (int)(baseSize * (1 + (1 - e) * 0.18f * pulse)));
        r.dy = -(1 - e) * baseSize * 0.12f;

        r.color = lerpColor(baseColor, (pulse > 0.5f) ? 0xB26BFFFFu : 0xFFD166FFu, (1 - e) * 0.75f);
        r.color = mulAlpha(r.color, e);
        break;
    }

    default:
        break;
    }

    if ((r.color & 0xFFu) == 0)
        r.visible = false;

    return r;
}

///////////////////////////////////////////////////////////////////////////////
// FX effects
///////////////////////////////////////////////////////////////////////////////

inline void applyFxEffect(
    FONScontext* fs,
    unsigned int& renderColor,
    int charIndex,
    float animTime,
    eFxStyle fxStyle,
    float repeatDelay,
    float& mtxDX,
    float& mtxDY,
    float& mtxScale,
    float& glow,
    unsigned int& glowColor,
    float& mtxRot,
    float& chromaDX,
    float& echoDX,
    float& mirrorA)
{
    if (fxStyle == eFxStyle::NONE)
        return;

    mtxRot = 0.f;
    chromaDX = 0.f;
    echoDX = 0.f;
    mirrorA = 0.f;

    float t = animTime;
    int idx = charIndex;

    const float PI = 3.14159265f;
    const float TAU = 6.2831853f;

    auto cyclePhase = [&](float dur, float& p) -> bool {
        float cycle = dur + std::max(0.0f, repeatDelay);
        float lt = std::fmod(t, cycle);

        if (lt < 0.0f)
            lt += cycle;

        if (lt >= dur)
            return false;

        p = lt / dur;
        return true;
    };

    auto stepEnv = [&](float p, float steps, float salt, float& out) -> bool {
        float step = std::floor(p * steps);
        float q = p * steps - step;
        float gate = std::max(0.0f, (hash21((float)idx * salt, step) - 0.5f) / 0.5f);

        out = gate * std::sin(q * PI);

        return out > 0.0f;
    };

    switch (fxStyle)
    {
    case eFxStyle::DUI:
        mtxDX = std::sin(t * 2 + idx * 0.5f) * 3;
        mtxDY = std::cos(t * 1.5f + idx * 0.7f) * 2;
        break;

    case eFxStyle::WAVE:
        mtxDY = std::sin(t * 3 + idx * 0.8f) * 5;
        break;

    case eFxStyle::SHAKE:
        mtxDX = std::sin(t * 10 + idx * 1.3f) * 2;
        mtxDY = std::cos(t * 8 + idx * 1.7f) * 1.5f;
        break;

    case eFxStyle::PULSE:
        mtxScale = 1.0f + std::sin(t * 4) * 0.2f;
        break;

    case eFxStyle::RAINBOW:
    {
        float hue = std::fmod(t * 0.5f + idx * 0.1f, 1.f);

        renderColor = packRGBA(
            (unsigned int)((std::sin(hue * 6.28f) * .5f + .5f) * 255),
            (unsigned int)((std::sin(hue * 6.28f + 2.09f) * .5f + .5f) * 255),
            (unsigned int)((std::sin(hue * 6.28f + 4.18f) * .5f + .5f) * 255),
            255);

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::GLITCH:
        if (std::sin(t * 15 + idx) > 0.7f)
            mtxDX = (std::sin(t * 20 + idx * 2) > 0 ? 5 : -5);
        break;

    case eFxStyle::TYPEWRITER:
    {
        float a = std::clamp((t - idx * 0.05f) * 5, 0.f, 1.f);
        renderColor = (renderColor & 0xFFFFFF00u) | (unsigned int)(a * 255);
        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::BOUNCE:
        mtxDY = -std::abs(std::sin(t * 5 + idx * 0.5f)) * 8;
        break;

    case eFxStyle::ROTATE:
    {
        float an = t * 2 + idx * 0.3f;
        mtxDX = std::cos(an) * 2;
        mtxDY = std::sin(an) * 2;
        break;
    }

    case eFxStyle::FIRE:
    {
        float f = std::sin(t * 6 + idx * 0.4f) * .5f + .5f;

        renderColor = packRGBA(255, (unsigned int)(f * 0.8f * 255), 0, 255);
        fonsSetColor(fs, renderColor);

        mtxDY = -f * 3;
        break;
    }

    case eFxStyle::BLINK:
    {
        float a = std::sin(t * 4) * .5f + .5f;
        renderColor = (renderColor & 0xFFFFFF00u) | (unsigned int)(a * 255);
        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::BREATHE:
    {
        float p;
        if (!cyclePhase(3.0f, p))
            return;

        float s = std::sin(p * TAU);
        float win = std::sin(p * PI);

        mtxScale = 1.0f + 0.05f * std::max(0.0f, s);

        renderColor = mulAlpha(renderColor, 1.0f - 0.15f * win);
        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::SWAY:
    {
        float p;
        if (!cyclePhase(4.0f, p))
            return;

        mtxDY = std::sin(p * TAU) * 2.5f;
        break;
    }

    case eFxStyle::DRIFT:
    {
        float p;
        if (!cyclePhase(5.0f, p))
            return;

        mtxDX = std::sin(p * TAU) * 3.0f;
        break;
    }

    case eFxStyle::GLOW:
    {
        float p;
        if (!cyclePhase(3.0f, p))
            return;

        float g = std::max(0.0f, std::sin(p * TAU));

        renderColor = lerpColor(renderColor, 0xFFD070FFu, 0.45f * g);

        glow = 0.9f * g;
        glowColor = 0xFFC040FFu;

        mtxScale = 1.0f + 0.05f * g;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::SHIMMER:
    {
        float p;
        if (!cyclePhase(2.5f, p))
            return;

        float win = std::sin(p * PI);

        float d = std::fabs(((float)idx / 24.0f) - p);
        d = std::min(d, 1.0f - d + 0.15f);

        float hi = std::max(0.0f, 1.0f - d * 5.0f) * win;

        if (hi > 0.0f)
        {
            renderColor = lerpColor(renderColor, 0xFFFFFFFFu, 0.35f * hi);
            fonsSetColor(fs, renderColor);
        }
        break;
    }

    case eFxStyle::ISHIMMER:
    {
        float p;
        if (!cyclePhase(2.5f, p))
            return;

        float win = std::sin(p * PI);

        float d = std::fabs(((float)idx / 24.0f) - p);
        d = std::min(d, 1.0f - d + 0.15f);

        float hi = std::max(0.0f, 1.0f - d * 4.0f) * win;

        if (hi > 0.0f)
        {
            renderColor = lerpColor(renderColor, 0xFFC040FFu, 0.85f * hi);

            glow = 0.9f * hi;
            glowColor = 0xFFD770FFu;

            fonsSetColor(fs, renderColor);
        }
        break;
    }

    case eFxStyle::CANDLE:
    {
        float p;
        if (!cyclePhase(2.0f, p))
            return;

        float win = std::sin(p * PI);
        float fl = 0.5f + 0.5f * std::sin(p * TAU * 3.0f + idx * 1.7f);

        renderColor = mulAlpha(renderColor, 1.0f - 0.25f * win * fl);
        renderColor = lerpColor(renderColor, 0xFFB066FFu, 0.35f * win);

        glow = 0.35f * win * fl;
        glowColor = 0xFF9030FFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::MIST:
    {
        float p;
        if (!cyclePhase(4.0f, p))
            return;

        float m = std::max(0.0f, std::sin(p * TAU));

        renderColor = lerpColor(renderColor, 0x8FA8C0FFu, 0.45f * m);
        renderColor = mulAlpha(renderColor, 1.0f - 0.20f * m);

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::HUE:
    {
        float p;
        if (!cyclePhase(6.0f, p))
            return;

        float s = std::sin(p * TAU);

        if (s > 0)
            renderColor = lerpColor(renderColor, 0xFFB080FFu, 0.35f * s);
        else
            renderColor = lerpColor(renderColor, 0x80C0FFFFu, 0.35f * -s);

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::SOFTFADE:
    {
        float p;
        if (!cyclePhase(3.5f, p))
            return;

        renderColor = mulAlpha(renderColor, 1.0f - 0.45f * std::sin(p * PI));
        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::SPARKLE:
    {
        float p;
        if (!cyclePhase(3.0f, p))
            return;

        float tw;
        if (!stepEnv(p, 6.0f, 13.1f, tw))
            return;

        unsigned int tint = (idx & 1) ? 0xFFD770FFu : 0x9FE8FFFFu;

        renderColor = lerpColor(renderColor, tint, 0.90f * tw);

        glow = 0.8f * tw;
        glowColor = tint;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::AURORA:
    {
        float p;
        if (!cyclePhase(5.0f, p))
            return;

        float win = std::sin(p * PI);

        unsigned int col = lerpColor(0x66FFCCFFu, 0xCC99FFFFu, 0.5f + 0.5f * std::sin(p * TAU + idx * 0.35f));

        renderColor = lerpColor(renderColor, col, 0.60f * win);

        glow = 0.40f * win;
        glowColor = col;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::OCEAN:
    {
        float p;
        if (!cyclePhase(4.5f, p))
            return;

        float win = std::sin(p * PI);

        unsigned int col = lerpColor(0x3FA9FFFFu, 0x2EE6D6FFu, 0.5f + 0.5f * std::sin(p * TAU + idx * 0.30f));

        renderColor = lerpColor(renderColor, col, 0.50f * win);
        mtxDY = std::sin(p * TAU + idx * 0.30f) * 1.5f * win;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::SUNSET:
    {
        float p;
        if (!cyclePhase(5.0f, p))
            return;

        float win = std::sin(p * PI);

        unsigned int col = lerpColor(0xFF8C42FFu, 0xFF5E8AFFu, 0.5f + 0.5f * std::sin(p * TAU + idx * 0.25f));

        renderColor = lerpColor(renderColor, col, 0.50f * win);

        glow = 0.25f * win;
        glowColor = col;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::FROST:
    {
        float p;
        if (!cyclePhase(4.0f, p))
            return;

        float win = std::sin(p * PI);

        renderColor = lerpColor(renderColor, 0xBFE9FFFFu, 0.50f * win);

        mtxScale = 1.0f - 0.03f * win;

        glow = 0.25f * win;
        glowColor = 0xBFE9FFFFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::LAVA:
    {
        float p;
        if (!cyclePhase(3.5f, p))
            return;

        float win = std::sin(p * PI);

        unsigned int col = lerpColor(0xFF4E1FFFu, 0xFFB03BFFu, 0.5f + 0.5f * std::sin(p * TAU * 2 + idx * 0.8f));

        renderColor = lerpColor(renderColor, col, 0.55f * win);

        glow = 0.45f * win;
        glowColor = 0xFF6A2AFFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::MOONLIGHT:
    {
        float p;
        if (!cyclePhase(4.5f, p))
            return;

        float win = std::sin(p * PI);

        renderColor = lerpColor(renderColor, 0xCFE4FFFFu, 0.40f * win);

        mtxScale = 1.0f + 0.02f * std::sin(p * TAU);

        glow = 0.50f * win;
        glowColor = 0xCFE4FFFFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::STARLIGHT:
    {
        float p;
        if (!cyclePhase(4.0f, p))
            return;

        float tw;
        if (!stepEnv(p, 4.0f, 7.7f, tw))
            return;

        unsigned int col = (idx & 1) ? 0xFFF6D8FFu : 0xE8F4FFFFu;

        renderColor = lerpColor(renderColor, col, 0.80f * tw);

        glow = 0.70f * tw;
        glowColor = col;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::NEONPULSE:
    {
        float p;
        if (!cyclePhase(2.5f, p))
            return;

        float win = std::sin(p * PI);
        float pulse = 0.6f + 0.4f * std::sin(p * TAU * 3);

        renderColor = lerpColor(renderColor, 0x39FFF0FFu, 0.50f * win);

        glow = 0.80f * win * pulse;
        glowColor = 0x39FFF0FFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::ROSE:
    {
        float p;
        if (!cyclePhase(3.6f, p))
            return;

        float win = std::sin(p * PI);

        renderColor = lerpColor(renderColor, 0xFF9EC4FFu, 0.50f * win);

        glow = 0.30f * win;
        glowColor = 0xFF9EC4FFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::MINT:
    {
        float p;
        if (!cyclePhase(3.4f, p))
            return;

        float win = std::sin(p * PI);

        renderColor = lerpColor(renderColor, 0x9CFFD8FFu, 0.50f * win);

        glow = 0.30f * win;
        glowColor = 0x9CFFD8FFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::EMERALD:
    {
        float p;
        if (!cyclePhase(3.8f, p))
            return;

        float win = std::sin(p * PI);

        renderColor = lerpColor(renderColor, 0x3BFF9CFFu, 0.50f * win);

        glow = 0.35f * win;
        glowColor = 0x3BFF9CFFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::SAPPHIRE:
    {
        float p;
        if (!cyclePhase(3.8f, p))
            return;

        float win = std::sin(p * PI);

        renderColor = lerpColor(renderColor, 0x3B7BFFFFu, 0.50f * win);

        glow = 0.35f * win;
        glowColor = 0x3B7BFFFFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::RUBY:
    {
        float p;
        if (!cyclePhase(3.5f, p))
            return;

        float win = std::sin(p * PI);

        renderColor = lerpColor(renderColor, 0xFF3B5EFFu, 0.50f * win);

        glow = 0.35f * win;
        glowColor = 0xFF3B5EFFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::AMETHYST:
    {
        float p;
        if (!cyclePhase(4.0f, p))
            return;

        float win = std::sin(p * PI);

        renderColor = lerpColor(renderColor, 0xB44BFFFFu, 0.50f * win);

        glow = 0.35f * win;
        glowColor = 0xB44BFFFFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::TOPAZ:
    {
        float p;
        if (!cyclePhase(3.6f, p))
            return;

        float win = std::sin(p * PI);

        renderColor = lerpColor(renderColor, 0xFFC14BFFu, 0.50f * win);

        glow = 0.35f * win;
        glowColor = 0xFFC14BFFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::JADE:
    {
        float p;
        if (!cyclePhase(3.9f, p))
            return;

        float win = std::sin(p * PI);

        renderColor = lerpColor(renderColor, 0x4BFFD2FFu, 0.50f * win);

        glow = 0.35f * win;
        glowColor = 0x4BFFD2FFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::GOLDWAVE:
    {
        float p;
        if (!cyclePhase(4.0f, p))
            return;

        float win = std::sin(p * PI);

        float d = std::fabs(((float)idx / 24.0f) - p);
        d = std::min(d, 1.0f - d + 0.15f);

        float hi = std::max(0.0f, 1.0f - d * 4.0f) * win;

        if (hi > 0.0f)
        {
            renderColor = lerpColor(renderColor, 0xFFD770FFu, 0.70f * hi);

            glow = 0.50f * hi;
            glowColor = 0xFFD770FFu;

            fonsSetColor(fs, renderColor);
        }
        break;
    }

    case eFxStyle::SILVERWAVE:
    {
        float p;
        if (!cyclePhase(3.5f, p))
            return;

        float win = std::sin(p * PI);

        float d = std::fabs(((float)idx / 24.0f) - p);
        d = std::min(d, 1.0f - d + 0.15f);

        float hi = std::max(0.0f, 1.0f - d * 4.0f) * win;

        if (hi > 0.0f)
        {
            renderColor = lerpColor(renderColor, 0xE8F4FFFFu, 0.70f * hi);

            glow = 0.40f * hi;
            glowColor = 0xE8F4FFFFu;

            fonsSetColor(fs, renderColor);
        }
        break;
    }

    case eFxStyle::PEARL:
    {
        float p;
        if (!cyclePhase(5.0f, p))
            return;

        float win = std::sin(p * PI);

        unsigned int col = lerpColor(0xFFD9E8FFu, 0xE8F4FFFFu, 0.5f + 0.5f * std::sin(p * TAU + idx * 0.2f));

        renderColor = lerpColor(renderColor, col, 0.45f * win);

        glow = 0.20f * win;
        glowColor = col;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::OPAL:
    {
        float p;
        if (!cyclePhase(6.0f, p))
            return;

        float win = std::sin(p * PI);

        float t1 = 0.5f + 0.5f * std::sin(p * TAU + idx * 0.25f);
        float t2 = 0.5f + 0.5f * std::sin(p * TAU * 0.5f + idx * 0.15f + 2.0f);

        unsigned int col = lerpColor(lerpColor(0xFFD9E8FFu, 0xC4FFE3FFu, t1), 0xBFD9FFFFu, t2 * 0.6f);

        renderColor = lerpColor(renderColor, col, 0.50f * win);

        glow = 0.30f * win;
        glowColor = col;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::NORTHERN:
    {
        float p;
        if (!cyclePhase(6.0f, p))
            return;

        float win = std::sin(p * PI);

        unsigned int col = lerpColor(0x59FFB4FFu, 0xB47BFFFFu, 0.5f + 0.5f * std::sin(p * TAU + idx * 0.45f));

        renderColor = lerpColor(renderColor, col, 0.55f * win);

        mtxDY = std::sin(p * TAU + idx * 0.5f) * 2.0f * win;
        mtxScale = 1.0f + 0.02f * std::sin(p * TAU) * win;

        glow = 0.50f * win;
        glowColor = col;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::TIDE:
    {
        float p;
        if (!cyclePhase(5.0f, p))
            return;

        float win = std::sin(p * PI);

        unsigned int col = lerpColor(0x2E7BFFFFu, 0x39E6D0FFu, 0.5f + 0.5f * std::sin(p * TAU * 0.5f + idx * 0.2f));

        renderColor = lerpColor(renderColor, col, 0.45f * win);

        mtxDY = (std::sin(p * TAU + idx * 0.25f) + 0.5f * std::sin(p * TAU * 2 + idx * 0.5f)) * 1.2f * win;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::FIREFLY:
    {
        float p;
        if (!cyclePhase(4.0f, p))
            return;

        float tw;
        if (!stepEnv(p, 3.0f, 11.3f, tw))
            return;

        renderColor = lerpColor(renderColor, 0xFFD770FFu, 0.60f * tw);

        mtxDX = std::sin(p * TAU * 2 + idx * 1.1f) * 1.2f * tw;
        mtxDY = std::cos(p * TAU * 1.5f + idx * 0.9f) * 1.2f * tw;

        glow = 0.90f * tw;
        glowColor = 0xFFC040FFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::SNOWFALL:
    {
        float p;
        if (!cyclePhase(5.0f, p))
            return;

        float win = std::sin(p * PI);

        renderColor = lerpColor(renderColor, 0xDFF2FFFFu, 0.40f * win);

        mtxDX = std::sin(p * TAU + idx * 1.3f) * 1.2f * win;
        mtxDY = std::sin(p * TAU * 0.5f + idx * 0.7f) * 1.0f * win;

        glow = 0.20f * win;
        glowColor = 0xDFF2FFFFu;

        float tw;
        if (stepEnv(p, 5.0f, 5.1f, tw))
        {
            renderColor = lerpColor(renderColor, 0xFFFFFFFFu, 0.70f * tw);

            glow = 0.55f * tw;
            glowColor = 0xE8F4FFFFu;
        }

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::HEARTBEAT:
    {
        float p;
        if (!cyclePhase(2.2f, p))
            return;

        float win = std::sin(p * PI);

        float hb = std::pow(std::max(0.0f, std::sin(p * TAU)), 3.0f)
                 + 0.5f * std::pow(std::max(0.0f, std::sin(p * TAU - 0.9f)), 3.0f);

        mtxScale = 1.0f + 0.05f * hb * win;

        renderColor = lerpColor(renderColor, 0xFF8C96FFu, 0.25f * hb * win);

        glow = 0.40f * hb * win;
        glowColor = 0xFF5E6EFFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::ORBIT:
    {
        float p;
        if (!cyclePhase(4.0f, p))
            return;

        float win = std::sin(p * PI);

        mtxDX = std::cos(p * TAU + idx * 0.4f) * 1.5f * win;
        mtxDY = std::sin(p * TAU + idx * 0.4f) * 1.5f * win;

        renderColor = lerpColor(renderColor, 0x9FB4FFFFu, 0.30f * win);

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::BLOSSOM:
    {
        float p;
        if (!cyclePhase(4.5f, p))
            return;

        float win = std::sin(p * PI);

        float bloom = win * (0.5f + 0.5f * std::sin(p * TAU * 2 + idx * 0.6f));

        mtxScale = 1.0f + 0.06f * bloom;

        renderColor = lerpColor(renderColor, 0xFFC7DDFFu, 0.50f * bloom);

        glow = 0.35f * bloom;
        glowColor = 0xFF9EC4FFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::RIPPLE:
    {
        float p;
        if (!cyclePhase(3.0f, p))
            return;

        float win = std::sin(p * PI);

        float d = std::fabs(((float)idx / 24.0f) - p);
        d = std::min(d, 1.0f - d + 0.15f);

        float bump = std::exp(-d * d * 40.0f) * win;

        mtxDY = -bump * 2.5f;

        renderColor = lerpColor(renderColor, 0xBFEFFFFu, 0.60f * bump);

        glow = 0.50f * bump;
        glowColor = 0xBFEFFFFu;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::ZEN:
    {
        float p;
        if (!cyclePhase(7.0f, p))
            return;

        float win = std::sin(p * PI);

        mtxScale = 1.0f + 0.03f * std::sin(p * TAU);

        unsigned int col = lerpColor(0xA8E6C8FFu, 0xBFD9FFFFu, 0.5f + 0.5f * std::sin(p * TAU * 0.5f));

        renderColor = lerpColor(renderColor, col, 0.40f * win);

        glow = 0.15f * win;
        glowColor = col;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::COSMOS:
    {
        float p;
        if (!cyclePhase(6.0f, p))
            return;

        float win = std::sin(p * PI);

        unsigned int col = lerpColor(0x7B5CFFFFu, 0x3BD0FFFFu, 0.5f + 0.5f * std::sin(p * TAU * 0.5f + idx * 0.15f));

        renderColor = lerpColor(renderColor, col, 0.50f * win);

        glow = 0.40f * win;
        glowColor = col;

        float tw;
        if (stepEnv(p, 6.0f, 9.7f, tw))
        {
            renderColor = lerpColor(renderColor, 0xFFFFFFFFu, 0.80f * tw);

            mtxScale = 1.0f + 0.05f * tw;

            glow = 0.70f * tw;
            glowColor = 0xE8F4FFFFu;
        }

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::RIFT:
    {
        // цифровой сбой: рывки символов + хроматическая аберрация,
        // в момент сбоя RGB-разводка резко расходится
        const float fr = std::floor(t * 18.0f);
        const float gate = hash21((float)idx * 5.13f, fr);

        const float jit = (gate > 0.80f) ? 1.0f : 0.0f;

        mtxDX = (hash21((float)idx * 3.7f, fr + 11.0f) - 0.5f) * 8.0f * jit;
        mtxDY = (hash21((float)idx * 9.1f, fr + 23.0f) - 0.5f) * 6.0f * jit;

        const float flash = clamp01((gate - 0.80f) / 0.20f);

        chromaDX = 1.2f + 4.0f * flash + 2.0f * jit;

        renderColor = lerpColor(renderColor, 0xCFE8FFFF, flash * 0.7f);

        glow = 0.4f * flash;
        glowColor = 0x66CCFFFF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::PHANTOM:
    {
        // призрачный шлейф: за каждым символом тянутся угасающие копии,
        // направление шлейфа медленно меняется
        float p;
        if (!cyclePhase(6.0f, p))
            return;

        const float win = std::sin(p * 3.14159265f);

        echoDX = std::sin(p * 6.2831853f) * (5.0f + 9.0f * win);

        mtxDX = -echoDX * 0.3f;

        glow = 0.3f * win;
        glowColor = 0x9FE8FFFF;

        renderColor = mulAlpha(renderColor, 0.85f + 0.15f * win);

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::MIRROR:
    {
        // глянец: под каждым символом — перевёрнутое призрачное отражение
        // от линии базelines, лёгкое покачивание "над водой"
        mirrorA = 0.28f + 0.14f * std::sin(t * 1.4f + (float)idx * 0.6f);

        mtxDY = std::sin(t * 1.1f + (float)idx * 0.4f) * 1.2f;

        glow = 0.12f;
        glowColor = 0x88CCFFFF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::WHIRL:
    {
        // вихрь: символы раскачиваются с индивидуальными фазами,
        // как парящие в воздухе буквы
        float p;
        if (!cyclePhase(3.0f, p))
            return;

        const float win = std::sin(p * 3.14159265f);

        mtxRot = std::sin(p * 6.2831853f + (float)idx * 0.7f) * 0.38f * (0.4f + 0.6f * win);

        mtxDY = -std::abs(std::sin(p * 6.2831853f + (float)idx * 0.7f)) * 3.0f * win;

        glow = 0.25f * win;
        glowColor = 0xFFB4E6FF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::TIMEWARP:
    {
        // искажение времени: при "разгоне" символ уезжает вбок,
        // за ним тянется RGB-разведённый шлейф
        float p;
        if (!cyclePhase(4.0f, p))
            return;

        const float rush = std::pow(std::sin(p * 3.14159265f), 1.5f);

        chromaDX = 6.0f * rush;
        echoDX = 12.0f * rush;
        mtxDX = -8.0f * rush;

        renderColor = lerpColor(renderColor, 0xBFE9FFFF, rush * 0.4f);

        glow = 0.5f * rush;
        glowColor = 0x66E0FFFF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::ELECTRIC:
    {
        // электрический треск: рывки по hash-гейту, синевато-белые вспышки, искры
        const float fr = std::floor(t * 24.0f);
        const float gate = hash21((float)idx * 5.13f, fr);

        const float jit = (gate > 0.72f) ? 1.0f : 0.0f;

        mtxDX = (hash21((float)idx * 3.7f, fr + 11.0f) - 0.5f) * 5.0f * jit;
        mtxDY = (hash21((float)idx * 9.1f, fr + 23.0f) - 0.5f) * 4.0f * jit;

        const float flash = clamp01((gate - 0.72f) / 0.28f);

        renderColor = lerpColor(renderColor, 0xCFE8FFFF, flash * 0.8f);

        glow = 0.55f * flash;
        glowColor = 0x66CCFFFF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::SUPERNOVA:
    {
        // залповые микровспышки: символы периодически вспыхивают и раздуваются
        const float dur = 2.8f;
        const float cycle = dur + std::max(0.0f, repeatDelay);

        const float lt = std::fmod(t + (float)idx * 0.13f, cycle);

        if (lt < dur * 0.6f)
        {
            const float p = lt / (dur * 0.6f);
            const float env = std::sin(p * 3.14159265f);

            mtxScale = 1.0f + 0.35f * env;

            renderColor = lerpColor(renderColor, 0xFFFFFFFF, env * 0.9f);
            renderColor = mulAlpha(renderColor, 0.55f + 0.45f * env);

            glow = 0.9f * env;
            glowColor = 0xFFEDC9FF;
        }

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::HOLY:
    {
        // благословение: тёплое золото, дыхание свечением, лёгкое набухание
        float p;
        if (!cyclePhase(4.0f, p))
            return;

        const float win = std::sin(p * 3.14159265f);
        const float breathe = 0.5f + 0.5f * std::sin(p * TAU + (float)idx * 0.35f);

        renderColor = lerpColor(renderColor, 0xFFD770FF, 0.35f * win * (0.5f + 0.5f * breathe));

        mtxScale = 1.0f + 0.03f * win * breathe;

        glow = 0.45f * win * (0.4f + 0.6f * breathe);
        glowColor = 0xFFCC55FF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::POISON:
    {
        // яд: токсичный зелёный, судорожное покачивание, пульс свечения
        float p;
        if (!cyclePhase(3.2f, p))
            return;

        const float throb = std::pow(std::max(0.0f, std::sin(p * TAU)), 2.0f);

        renderColor = lerpColor(renderColor, 0x5CFF3FFF, 0.55f);
        renderColor = mulAlpha(renderColor, 0.9f + 0.1f * throb);

        mtxDY = std::sin(t * 3.0f + (float)idx * 1.1f) * 1.6f * (0.5f + throb);

        glow = 0.1f + 0.4f * throb;
        glowColor = 0x66FF33FF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::DREAD:
    {
        // бездна: текст темнеет, символы медленно проваливаются в фиолетовую тьму
        float p;
        if (!cyclePhase(5.0f, p))
            return;

        const float win = std::sin(p * 3.14159265f);

        renderColor = lerpColor(renderColor, 0x1A0A2AFF, 0.25f + 0.5f * win);

        mtxDY = (0.5f + 0.5f * std::sin(p * TAU + (float)idx * 0.5f)) * 4.0f * win;

        glow = 0.5f * win;
        glowColor = 0x7B2BFFFF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::BLOODLUST:
    {
        // жажда крови: тяжёлый сердцебиение-пульс багрового свечения,
        // в припадке буквы дрожат и вспыхивают алым
        float p;
        if (!cyclePhase(4.2f, p))
            return;

        // двойной удар сердца, затем затишье
        const float beat = std::pow(std::max(0.0f, std::sin(p * TAU)), 3.0f)
                         + 0.6f * std::pow(std::max(0.0f, std::sin(p * TAU - 0.85f)), 3.0f);

        const float fury = clamp01(beat * 1.4f);
        const float j = fury * fury;

        // дрожь в припадке
        const float fr = std::floor(t * 30.0f);

        mtxDX = (hash21((float)idx * 3.1f, fr) - 0.5f) * 4.5f * j;
        mtxDY = (hash21((float)idx * 7.7f, fr + 5.0f) - 0.5f) * 3.5f * j;

        mtxScale = 1.0f + 0.06f * fury;

        renderColor = lerpColor(renderColor, 0xB00F0FFF, 0.55f + 0.35f * fury);
        renderColor = mulAlpha(renderColor, 0.9f + 0.1f * fury);

        glow = 0.25f + 0.55f * fury;
        glowColor = 0xC00808FF;

        fonsSetColor(fs, renderColor);
        break;
    }


    case eFxStyle::SPECTRAL:
    {
        // призрак: холодный перелив оттенков, парение, свечение дыханием
        float p;
        if (!cyclePhase(5.0f, p))
            return;

        const float win = std::sin(p * PI);
        const float hue = std::sin(p * TAU + idx * 0.35f);

        unsigned int col = (hue > 0.f) ? 0x9FE8FFFF : 0xC4B5FDFF;

        renderColor = lerpColor(renderColor, col, 0.55f * win);
        mtxDY = std::sin(p * TAU * 2.0f + idx * 0.6f) * 3.0f * win;

        glow = 0.45f * win;
        glowColor = col;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::MAGMA:
    {
        // магма: тёмная корка, в случайных прорывах — раскалённые трещины
        float p;
        if (!cyclePhase(3.6f, p))
            return;

        float tw;
        if (!stepEnv(p, 5.0f, 8.9f, tw))
        {
            renderColor = lerpColor(renderColor, 0x1A0F0AFF, 0.85f);
            fonsSetColor(fs, renderColor);
            return;
        }

        renderColor = lerpColor(0x2A1206FF, 0xFF7A18FF, 0.35f + 0.65f * tw);
        mtxScale = 1.0f + 0.03f * tw;

        glow = 0.85f * tw;
        glowColor = 0xFF6A00FF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::FROSTBITE:
    {
        // ледяное покалывание: синий накат, дрожь, холодное дыхание
        float p;
        if (!cyclePhase(4.2f, p))
            return;

        const float win = std::sin(p * PI);
        const float shiver = win * win;

        renderColor = lerpColor(renderColor, 0x9FD8FFFF, 0.5f * win);

        mtxDX = std::sin(t * 27.0f + idx * 2.1f) * 1.3f * shiver;
        mtxDY = std::cos(t * 23.0f + idx * 1.7f) * 0.8f * shiver;

        glow = 0.3f * win;
        glowColor = 0xBFE9FFFF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::VENOM:
    {
        // яд: фиолетово-зелёный пульс, тяжёлое покачивание капель
        float p;
        if (!cyclePhase(3.4f, p))
            return;

        const float pulse = std::pow(std::max(0.0f, std::sin(p * TAU)), 2.0f);

        unsigned int col = lerpColor(0x7B2BFFFF, 0x5CFF3FFF,
            0.5f + 0.5f * std::sin(p * TAU * 2.0f + idx * 0.5f));

        renderColor = lerpColor(renderColor, col, 0.45f + 0.35f * pulse);
        mtxDY = std::sin(t * 2.4f + idx * 0.9f) * 1.8f;

        glow = 0.2f + 0.5f * pulse;
        glowColor = col;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::METEOR:
    {
        // метеор: горизонтальный рывок с огненным шлейфом и RGB-разводкой
        float p;
        if (!cyclePhase(3.0f, p))
            return;

        const float rush = std::pow(std::sin(p * PI), 1.5f);

        echoDX = -14.0f * rush;
        mtxDX = 6.0f * rush;
        chromaDX = 1.0f + 3.0f * rush;

        renderColor = lerpColor(renderColor, 0xFFB36AFF, rush * 0.5f);

        glow = 0.55f * rush;
        glowColor = 0xFF8C42FF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::ECLIPSE:
    {
        // затмение: буквы гаснут в фазе тьмы, по краям вспыхивает корона
        float p;
        if (!cyclePhase(6.0f, p))
            return;

        const float dark = std::pow(std::max(0.0f, std::sin(p * TAU - 1.2f)), 3.0f);
        const float corona = std::pow(std::max(0.0f, std::sin(p * TAU)), 4.0f);

        renderColor = mulAlpha(renderColor, 1.0f - 0.55f * dark);
        renderColor = lerpColor(renderColor, 0xFFD9A0FF, corona * 0.3f);

        glow = 0.7f * corona;
        glowColor = 0xFFD9A0FF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::TESLA:
    {
        // тесла: синие разряды по hash-гейту, рывки и белые вспышки
        const float fr = std::floor(t * 26.0f);
        const float gate = hash21((float)idx * 4.13f, fr);

        const float jit = (gate > 0.78f) ? 1.0f : 0.0f;

        mtxDX = (hash21((float)idx * 3.7f, fr + 11.0f) - 0.5f) * 6.0f * jit;
        mtxDY = (hash21((float)idx * 9.1f, fr + 23.0f) - 0.5f) * 5.0f * jit;

        const float flash = clamp01((gate - 0.78f) / 0.22f);

        renderColor = lerpColor(renderColor, 0xEAF6FFFF, flash * 0.9f);

        glow = 0.6f * flash;
        glowColor = 0x66CCFFFF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::ROSEGOLD:
    {
        // розовое золото: тёплый металлический блик, пробегающий по буквам
        float p;
        if (!cyclePhase(4.2f, p))
            return;

        const float win = std::sin(p * PI);

        const float d = std::fabs(((float)idx / 24.0f) - p);
        const float hi = std::max(0.0f, 1.0f - d * 5.0f) * win;

        if (hi > 0.0f)
        {
            renderColor = lerpColor(renderColor, 0xFFB8A8FF, 0.65f * hi);

            glow = 0.45f * hi;
            glowColor = 0xFFB8A8FF;

            fonsSetColor(fs, renderColor);
        }
        break;
    }

    case eFxStyle::VOIDWALK:
    {
        // ступень в пустоту: буквы проваливаются в тень и возвращаются
        float p;
        if (!cyclePhase(4.6f, p))
            return;

        const float sink = std::pow(std::max(0.0f, std::sin(p * TAU)), 2.0f);
        const float h = hash1((float)idx * 5.3f);
        const float local = clamp01(sink * 1.35f - h * 0.45f);

        renderColor = lerpColor(renderColor, 0x0A0614FF, 0.30f + 0.55f * local);
        mtxDY = local * 6.0f;

        glow = 0.4f * local;
        glowColor = 0x7B2BFFFF;

        fonsSetColor(fs, renderColor);
        break;
    }

    case eFxStyle::PRISM:
    {
        // призма: перекрученная радуга по буквам + лёгкий пульс размера
        float p;
        if (!cyclePhase(5.5f, p))
            return;

        const float win = std::sin(p * PI);
        const float u = std::fmod(p + (float)idx * 0.09f, 1.f);

        renderColor = packRGBA(
            (unsigned int)((std::sin(u * 6.28f) * .5f + .5f) * 255),
            (unsigned int)((std::sin(u * 6.28f + 2.09f) * .5f + .5f) * 255),
            (unsigned int)((std::sin(u * 6.28f + 4.18f) * .5f + .5f) * 255),
            255);

        mtxScale = 1.0f + 0.06f * std::sin(p * TAU * 2.0f + idx * 0.7f) * win;

        glow = 0.25f * win;
        glowColor = renderColor;

        fonsSetColor(fs, renderColor);
        break;
    }

    default:
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// idle "chase"
///////////////////////////////////////////////////////////////////////////////

inline void renderImgChase(const HTMLNode& node, float imgX, float imgY, float imgW, float imgH, float now)
{
    if (!node.spriteTex || imgW <= 0 || imgH <= 0)
        return;

    const float uL = node.spriteUV[0];
    const float vT = node.spriteUV[1];
    const float uR = node.spriteUV[2];
    const float vB = node.spriteUV[3];

    const float speed = 260.0f;
    const float th = 4.0f;
    const float headLen = 56.0f;
    const float tailLen = 140.0f;

    const float P = 2.0f * (imgW + imgH);

    const float delay = idleAttrDelay(node, 0.0f);

    float s;

    if (delay > 0.0f)
    {
        const float lapTime = P / speed;
        const float totalTime = lapTime + delay;

        float local = std::fmod(now, totalTime);
        if (local < 0.0f)
            local += totalTime;

        if (local >= lapTime)
            return;

        s = std::fmod(local * speed, P);
    }
    else
    {
        s = std::fmod(now * speed, P);
    }

    if (s < 0)
        s += P;

    auto emit = [&](float a, float b, float thick, float alpha)
    {
        if (b <= a || alpha <= 0.001f)
            return;

        for (int edge = 0; edge < 4; ++edge)
        {
            float e0, e1;

            if (edge == 0)      { e0 = 0;               e1 = imgW; }
            else if (edge == 1) { e0 = imgW;            e1 = imgW + imgH; }
            else if (edge == 2) { e0 = imgW + imgH;     e1 = 2 * imgW + imgH; }
            else                { e0 = 2 * imgW + imgH; e1 = P; }

            const float ca = std::max(a, e0);
            const float cb = std::min(b, e1);

            if (cb <= ca)
                continue;

            const float l0 = ca - e0;
            const float l1 = cb - e0;

            float x0, y0, x1, y1;

            if (edge == 0)
            {
                x0 = l0;
                y0 = 0;
                x1 = l1;
                y1 = thick;
            }
            else if (edge == 1)
            {
                x0 = imgW - thick;
                y0 = l0;
                x1 = imgW;
                y1 = l1;
            }
            else if (edge == 2)
            {
                x0 = imgW - l1;
                y0 = imgH - thick;
                x1 = imgW - l0;
                y1 = imgH;
            }
            else
            {
                x0 = 0;
                y0 = imgH - l1;
                x1 = thick;
                y1 = imgH - l0;
            }

            const float u0 = uL + (uR - uL) * (x0 / imgW);
            const float u1 = uL + (uR - uL) * (x1 / imgW);
            const float v0 = vT + (vB - vT) * (y0 / imgH);
            const float v1 = vT + (vB - vT) * (y1 / imgH);

            renderQuadDirect(
                node.spriteTex,
                imgX + x0, imgY + y0,
                imgX + x1, imgY + y1,
                u0, v0, u1, v1,
                alpha, alpha, alpha, 1.0f,
                eSpriteBlendMode::ADDITIVE);
        }
    };

    auto seg = [&](float a, float b, float thick, float alpha)
    {
        emit(a, b, thick, alpha);

        if (a < 0.0f)
            emit(a + P, b + P, thick, alpha);
    };

    seg(s - tailLen, s, th, 0.30f);
    seg(s - headLen, s, th, 0.85f);
}

///////////////////////////////////////////////////////////////////////////////
// idle "lens"
///////////////////////////////////////////////////////////////////////////////

inline void renderImgLens(const HTMLNode& node, float imgX, float imgY, float imgW, float imgH, float now)
{
    if (!node.spriteTex || imgW <= 0 || imgH <= 0)
        return;

    const float uL = node.spriteUV[0], vT = node.spriteUV[1];
    const float uR = node.spriteUV[2], vB = node.spriteUV[3];

    const float mag = 1.25f;
    const float TAU = 6.2831853f;

    const float active = node.idleDur   > 0.0f ? node.idleDur   : 6.0f;
    const float delay  = node.idleDelay > 0.0f ? node.idleDelay : 0.0f;
    const float total  = active + delay;

    float lt = std::fmod(now, total);
    if (lt < 0.0f)
        lt += total;

    if (lt >= active)
        return;

    const float ci = std::floor(now / total);

    const float r1 = hash21(ci * 12.9898f, 78.233f);
    const float r2 = hash21(ci * 39.346f, 11.135f);
    const float r3 = hash21(ci * 73.156f, 52.428f);
    const float r4 = hash21(ci * 27.719f, 93.989f);
    const float r5 = hash21(ci * 55.555f, 37.373f);

    const float p = lt / active;

    const float fadeW = std::min(0.25f, 1.2f / active);

    float fade = std::min(p, 1.0f - p) / fadeW;
    fade = std::clamp(fade, 0.0f, 1.0f);
    fade = fade * fade * (3.0f - 2.0f * fade);

    if (fade <= 0.001f)
        return;

    const float fx = 1.0f + std::floor(r1 * 2.0f);
    const float fy = 2.0f + std::floor(r2 * 2.0f);

    const float u = p * TAU;

    const float wx = std::sin(u * fx + r1 * TAU) * 0.70f + std::sin(u * 3.0f + r3 * TAU) * 0.30f;
    const float wy = std::sin(u * fy + r2 * TAU) * 0.70f + std::sin(u * 2.0f + r4 * TAU) * 0.30f;

    const float R    = std::min(imgW, imgH) * 0.22f;
    const float maxR = R * 1.45f;

    const float ax = std::max(0.0f, 0.5f - maxR / imgW - 0.02f);
    const float ay = std::max(0.0f, 0.5f - maxR / imgH - 0.02f);

    const float cx = imgW * (0.5f + ax * wx);
    const float cy = imgH * (0.5f + ay * wy);

    const float ucx = uL + (uR - uL) * (cx / imgW);
    const float vcy = vT + (vB - vT) * (cy / imgH);

    auto uvAt = [&](float x, float y, float& uo, float& vo)
    {
        uo = ucx + (uR - uL) * ((x - cx) / imgW) / mag;
        vo = vcy + (vB - vT) * ((y - cy) / imgH) / mag;
    };

    auto emitQuad = [&](const float* qx, const float* qy,
                        float cr, float cg, float cb, float ca,
                        eSpriteBlendMode blend)
    {
        float verts[8];
        float uvs[8];

        verts[CSprite::VERT_ULX] = imgX + qx[0]; verts[CSprite::VERT_ULY] = imgY + qy[0];
        verts[CSprite::VERT_URX] = imgX + qx[1]; verts[CSprite::VERT_URY] = imgY + qy[1];
        verts[CSprite::VERT_BLX] = imgX + qx[2]; verts[CSprite::VERT_BLY] = imgY + qy[2];
        verts[CSprite::VERT_BRX] = imgX + qx[3]; verts[CSprite::VERT_BRY] = imgY + qy[3];

        uvAt(qx[0], qy[0], uvs[CSprite::VERT_ULX], uvs[CSprite::VERT_ULY]);
        uvAt(qx[1], qy[1], uvs[CSprite::VERT_URX], uvs[CSprite::VERT_URY]);
        uvAt(qx[2], qy[2], uvs[CSprite::VERT_BLX], uvs[CSprite::VERT_BLY]);
        uvAt(qx[3], qy[3], uvs[CSprite::VERT_BRX], uvs[CSprite::VERT_BRY]);

        float col[4] = { cr, cg, cb, ca };

        CSprite::renderVerts(node.spriteTex, verts, uvs, col, blend);
    };

    const int N = 10;
    const float rot = r5 * TAU + lt * (0.25f + 0.30f * r3);

    float vx[16], vy[16];

    for (int i = 0; i < N; ++i)
    {
        const float ang = rot + TAU * i / N;

        const float m = 1.0f
            + 0.16f * std::sin(ang * 3.0f + lt * 1.9f + r3 * TAU)
            + 0.09f * std::sin(ang * 5.0f - lt * 2.6f + r4 * TAU);

        const float rad = R * m;

        vx[i] = cx + std::cos(ang) * rad;
        vy[i] = cy + std::sin(ang) * rad * 1.10f;
    }

    for (int i = 0; i < N; ++i)
    {
        const int j = (i + 1) % N;

        const float qx[4] = { cx, vx[i], vx[j], vx[j] };
        const float qy[4] = { cy, vy[i], vy[j], vy[j] };

        emitQuad(qx, qy, 1.0f, 1.0f, 1.0f, fade, eSpriteBlendMode::NORMAL);
    }

    const float g    = 0.35f * fade;
    const float rimW = 0.12f;

    for (int i = 0; i < N; ++i)
    {
        const int j = (i + 1) % N;

        const float a2x = vx[i] + (cx - vx[i]) * rimW;
        const float a2y = vy[i] + (cy - vy[i]) * rimW;
        const float b2x = vx[j] + (cx - vx[j]) * rimW;
        const float b2y = vy[j] + (cy - vy[j]) * rimW;

        const float qx[4] = { vx[i], vx[j], b2x, a2x };
        const float qy[4] = { vy[i], vy[j], b2y, a2y };

        emitQuad(qx, qy, g, g, g, 1.0f, eSpriteBlendMode::ADDITIVE);
    }
}

///////////////////////////////////////////////////////////////////////////////
// idle "embers"
///////////////////////////////////////////////////////////////////////////////

inline void renderImgEmbers(const HTMLNode& node, float imgX, float imgY, float imgW, float imgH, float now)
{
    if (imgW <= 0 || imgH <= 0)
        return;

    auto ptrWhiteBox = Engine::getCfg().wb;

    if (!ptrWhiteBox)
        return;

    const float wu0 = ptrWhiteBox->_uvs[CSprite::VERT_ULX];
    const float wv0 = ptrWhiteBox->_uvs[CSprite::VERT_ULY];
    const float wu1 = ptrWhiteBox->_uvs[CSprite::VERT_URX];
    const float wv1 = ptrWhiteBox->_uvs[CSprite::VERT_BLY];

    {
        const float h0 = imgH * 0.16f;

        renderQuadDirect(
            ptrWhiteBox->getTexture(),
            imgX, imgY + imgH - h0,
            imgX + imgW, imgY + imgH,
            wu0, wv0, wu1, wv1,
            0.10f, 0.04f, 0.01f, 1.0f,
            eSpriteBlendMode::ADDITIVE);
    }

    const int K = 14;
    const float delay = idleAttrDelay(node, 0.0f);

    for (int i = 0; i < K; ++i)
    {
        const float h1 = hash21(i * 12.98f + 3.1f, 7.7f);
        const float h2 = hash21(i * 91.17f + 1.3f, 4.7f);
        const float h3 = hash21(i * 47.7f + 9.2f, 12.3f);

        const float speed  = imgH * (0.25f + 0.35f * h2);
        const float period = (imgH + 40.0f) / speed;
        const float totalPeriod = period + delay;

        float lt = std::fmod(now + h1 * 100.0f, totalPeriod);
        if (lt < 0.0f)
            lt += totalPeriod;

        if (lt >= period)
            continue;

        const float u = lt / period;

        const float y = imgH + 10.0f - u * (imgH + 40.0f);
        const float x = imgW * (0.06f + 0.88f * h1) + std::sin(now * (1.5f + 2.0f * h3) + i) * imgW * 0.03f;

        if (y < -6.0f || y > imgH + 6.0f)
            continue;

        const float fade = (1.0f - u) * std::min(1.0f, (imgH - y + 10.0f) / 30.0f);
        const float a = 0.7f * fade * (0.5f + 0.5f * h3);

        if (a <= 0.01f)
            continue;

        const float sz = 2.0f + 3.0f * h3;

        renderQuadDirect(
            ptrWhiteBox->getTexture(),
            imgX + x - sz * 0.5f, imgY + y - sz * 0.5f,
            imgX + x + sz * 0.5f, imgY + y + sz * 0.5f,
            wu0, wv0, wu1, wv1,
            1.0f * a, (0.45f + 0.4f * h1) * a, 0.08f * a, 1.0f,
            eSpriteBlendMode::ADDITIVE);
    }
}

///////////////////////////////////////////////////////////////////////////////
// idle "pixel"
///////////////////////////////////////////////////////////////////////////////

inline void renderImgPixel(const HTMLNode& node,
                           float imgX, float imgY, float imgW, float imgH,
                           float now)
{
    if (!node.spriteTex)
        return;

    if (imgW <= 0 || imgH <= 0)
        return;

    const float uL = node.spriteUV[0];
    const float vT = node.spriteUV[1];
    const float uR = node.spriteUV[2];
    const float vB = node.spriteUV[3];

    const float B     = node.pxBlock > 2.0f  ? node.pxBlock : 10.0f;
    const float band  = node.pxBand  > 8.0f  ? node.pxBand  : 56.0f;
    const float speed = node.pxSpeed > 20.0f ? node.pxSpeed : 260.0f;
    const float tilt  = 0.18f;

    const float travel = imgW + band * 2.0f + std::fabs(tilt) * imgH;
    const float tWave  = travel / speed;

    const float pause = idleAttrDelay(node, 1.2f);
    const float cycle = tWave + pause;

    float t = std::fmod(now, cycle);
    if (t < 0.0f)
        t += cycle;

    if (t > tWave)
        return;

    const float F = -band + speed * t;

    const int nx = (int)std::ceil(imgW / B);
    const int ny = (int)std::ceil(imgH / B);

    for (int iy = 0; iy < ny; ++iy)
    {
        for (int ix = 0; ix < nx; ++ix)
        {
            const float bx = ix * B;
            const float by = iy * B;

            const float cx = bx + B * 0.5f;
            const float cy = by + B * 0.5f;

            if (cx > imgW || cy > imgH)
                continue;

            const float front = F + tilt * (cy - imgH * 0.5f);
            const float q = (front - cx) / band;

            if (q < 0.0f || q > 1.0f)
                continue;

            const float h1 = hash21(ix * 12.98f + 3.7f, iy * 7.13f + 1.3f);
            const float h2 = hash21(ix * 91.17f + 7.7f, iy * 47.7f + 9.1f);

            const float bw = std::min(B, imgW - bx);
            const float bh = std::min(B, imgH - by);

            const float su = uL + (uR - uL) * (cx / imgW);
            const float sv = vT + (vB - vT) * (cy / imgH);

            const float x0 = imgX + bx;
            const float y0 = imgY + by;
            const float x1 = imgX + bx + bw;
            const float y1 = imgY + by + bh;

            if (q < 0.3f && h1 > 0.82f)
            {
                const float fall = (0.3f - q) / 0.3f;
                const float dy = fall * fall * (4.0f + 6.0f * h2);
                const float alpha = 0.9f * (1.0f - 0.5f * fall);

                renderQuadDirect(
                    node.spriteTex,
                    x0, y0 + dy, x1, y1 + dy,
                    su, sv, su, sv,
                    1.0f, 1.0f, 1.0f, alpha,
                    eSpriteBlendMode::NORMAL);

                const float g = 0.5f * fall * node.pxGlow;

                if (g > 0.01f)
                {
                    renderQuadDirect(
                        node.spriteTex,
                        x0, y0 + dy, x1, y1 + dy,
                        su, sv, su, sv,
                        g, g, g, 1.0f,
                        eSpriteBlendMode::ADDITIVE);
                }

                continue;
            }

            float alpha = 1.0f - q * q;
            alpha = std::floor(alpha * 4.0f) / 4.0f;

            if (alpha < 0.06f)
                continue;

            renderQuadDirect(
                node.spriteTex,
                x0, y0, x1, y1,
                su, sv, su, sv,
                1.0f, 1.0f, 1.0f, alpha,
                eSpriteBlendMode::NORMAL);

            if (q < 0.18f && h2 > 0.7f)
            {
                const float g = 0.45f * node.pxGlow * (1.0f - q / 0.18f);

                renderQuadDirect(
                    node.spriteTex,
                    x0, y0, x1, y1,
                    su, sv, su, sv,
                    g, g, g, 1.0f,
                    eSpriteBlendMode::ADDITIVE);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// idle "glare"
///////////////////////////////////////////////////////////////////////////////

inline void renderImgGlare(const HTMLNode& node, float imgX, float imgY, float imgW, float imgH, float now)
{
    if (!node.spriteTex)
        return;

    if (imgW <= 0 || imgH <= 0)
        return;

    const float uL = node.spriteUV[0];
    const float vT = node.spriteUV[1];
    const float uR = node.spriteUV[2];
    const float vB = node.spriteUV[3];

    const bool vertMove = (node.glareDir == 2 || node.glareDir == 3);

    const float speed = node.glareSpeed > 0 ? node.glareSpeed : 150.0f;
    const float W     = node.glareWidth > 0 ? node.glareWidth : 40.0f;
    const float tilt  = std::tan(node.glareTilt * 3.14159265f / 180.0f);

    const float span  = vertMove ? imgH : imgW;
    const float cross = vertMove ? imgW : imgH;

    const float tiltExt = std::fabs(tilt) * cross * 0.5f;

    const float W0 = tiltExt + W;
    const float D  = span + 2.0f * W0;

    const float dq = W / (D + W);
    const float cycle = D + W;

    const float delay = idleAttrDelay(node, 0.0f);

    float local = now;

    if (delay > 0.0f)
    {
        const float activeTime = cycle / speed;
        const float totalTime = activeTime + delay;

        local = std::fmod(now, totalTime);
        if (local < 0.0f)
            local += totalTime;

        if (local >= activeTime)
            return;
    }

    const float q = std::fmod(local * speed, cycle) / cycle;

    float lead  = -W0 + cycle * q;
    float trail = -W0 + cycle * (q - dq);

    if (lead  < -W0) lead  = -W0;
    if (lead  > span + W0) lead  = span + W0;

    if (trail < -W0) trail = -W0;
    if (trail > span + W0) trail = span + W0;

    if (lead - trail <= 0.0f)
        return;

    const float lt0 = tilt * (0.0f  - cross * 0.5f);
    const float lt1 = tilt * (cross - cross * 0.5f);

    GlarePoly poly;

    if (!vertMove)
    {
        poly.x[0] = trail + lt0; poly.y[0] = 0.0f;
        poly.x[1] = lead  + lt0; poly.y[1] = 0.0f;
        poly.x[2] = lead  + lt1; poly.y[2] = imgH;
        poly.x[3] = trail + lt1; poly.y[3] = imgH;
    }
    else
    {
        poly.x[0] = 0.0f; poly.y[0] = trail + lt0;
        poly.x[1] = imgW; poly.y[1] = trail + lt1;
        poly.x[2] = imgW; poly.y[2] = lead  + lt1;
        poly.x[3] = 0.0f; poly.y[3] = lead  + lt0;
    }

    poly.n = 4;

    clipGlarePolyToRect(poly, 0.0f, 0.0f, imgW, imgH);

    if (poly.n < 3)
        return;

    renderAdditivePoly(node.spriteTex, uL, uR, vT, vB, poly, imgX, imgY, imgW, imgH, node.glareIntensity);
}

///////////////////////////////////////////////////////////////////////////////
// idle "confetti"
///////////////////////////////////////////////////////////////////////////////

inline void renderImgConfetti(const HTMLNode& node, float imgX, float imgY, float imgW, float imgH, float now)
{
    if (!node.spriteTex || imgW <= 0 || imgH <= 0)
        return;

    auto ptrWhiteBox = Engine::getCfg().wb;
    if (!ptrWhiteBox)
        return;

    const float wu0 = ptrWhiteBox->_uvs[CSprite::VERT_ULX];
    const float wv0 = ptrWhiteBox->_uvs[CSprite::VERT_ULY];
    const float wu1 = ptrWhiteBox->_uvs[CSprite::VERT_URX];
    const float wv1 = ptrWhiteBox->_uvs[CSprite::VERT_BLY];

    const float uL = node.spriteUV[0], vT = node.spriteUV[1];
    const float uR = node.spriteUV[2], vB = node.spriteUV[3];

    const float active = std::max(0.1f, node.idleDur > 0.0f ? node.idleDur : 5.0f);
    const float pause  = node.idleDelay > 0.0f ? node.idleDelay : 1.2f;
    const float cycle  = active + pause;

    float lt = std::fmod(now, cycle);
    if (lt < 0.0f)
        lt += cycle;

    if (lt >= active)
    {
        renderQuadDirect(node.spriteTex,
                         imgX, imgY, imgX + imgW, imgY + imgH,
                         uL, vT, uR, vB,
                         1.0f, 1.0f, 1.0f, 1.0f,
                         eSpriteBlendMode::NORMAL);
        return;
    }

    const float p = lt / active;
    const float ci = std::floor(now / cycle);

    const float PAL[5][3] = {
        { 1.00f, 0.35f, 0.40f },
        { 1.00f, 0.70f, 0.25f },
        { 0.45f, 0.85f, 0.50f },
        { 0.35f, 0.65f, 1.00f },
        { 0.80f, 0.45f, 1.00f },
    };

    const float pB = 0.18f;
    const float pR = 0.62f;

    float t;
    if (p < pB)      t = 0.0f;
    else if (p < pR) t = (p - pB) / (pR - pB);
    else             t = 1.0f - (p - pR) / (1.0f - pR);

    const float te = 1.0f - (1.0f - t) * (1.0f - t);

    if (p > pB && p < pB + 0.12f)
    {
        const float f = 1.0f - (p - pB) / 0.12f;
        renderQuadDirect(node.spriteTex,
                         imgX, imgY, imgX + imgW, imgY + imgH,
                         uL, vT, uR, vB,
                         1.0f * f * 0.4f, 0.95f * f * 0.4f, 0.9f * f * 0.4f, 1.0f,
                         eSpriteBlendMode::ADDITIVE);
    }

    auto emitQuad = [&](CTexturePtr tex,
                        const float* cx, const float* cy,
                        float u0, float v0, float u1, float v1,
                        float r, float g, float b, float a,
                        eSpriteBlendMode blend)
    {
        if (a <= 0.02f)
            return;

        float verts[8];
        verts[CSprite::VERT_ULX] = cx[0]; verts[CSprite::VERT_ULY] = cy[0];
        verts[CSprite::VERT_URX] = cx[1]; verts[CSprite::VERT_URY] = cy[1];
        verts[CSprite::VERT_BLX] = cx[2]; verts[CSprite::VERT_BLY] = cy[2];
        verts[CSprite::VERT_BRX] = cx[3]; verts[CSprite::VERT_BRY] = cy[3];

        float uvs[8];
        uvs[CSprite::VERT_ULX] = u0; uvs[CSprite::VERT_ULY] = v0;
        uvs[CSprite::VERT_URX] = u1; uvs[CSprite::VERT_URY] = v0;
        uvs[CSprite::VERT_BLX] = u0; uvs[CSprite::VERT_BLY] = v1;
        uvs[CSprite::VERT_BRX] = u1; uvs[CSprite::VERT_BRY] = v1;

        float col[4] = { r, g, b, a };
        CSprite::renderVerts(tex, verts, uvs, col, blend);
    };

    const float B  = std::clamp(std::min(imgW, imgH) * 0.08f, 8.0f, 20.0f);
    const int   nx = (int)std::ceil(imgW / B);
    const int   ny = (int)std::ceil(imgH / B);

    for (int iy = 0; iy < ny; ++iy)
    {
        for (int ix = 0; ix < nx; ++ix)
        {
            const float bx = ix * B, by = iy * B;
            const float bw = std::min(B, imgW - bx);
            const float bh = std::min(B, imgH - by);

            if (bw < 2.0f || bh < 2.0f)
                continue;

            const float h1 = hash21(ix * 12.98f + ci * 3.1f, iy * 7.13f + 1.3f);
            const float h2 = hash21(ix * 91.17f + ci * 7.7f, iy * 47.7f + 9.1f);
            const float h3 = hash21(ix * 47.7f + ci * 5.5f, iy * 23.7f + 5.5f);
            const float h4 = hash21(ix * 55.5f + ci * 1.9f, iy * 77.7f + 2.2f);

            const float u0 = uL + (uR - uL) * (bx / imgW);
            const float u1 = uL + (uR - uL) * ((bx + bw) / imgW);
            const float v0 = vT + (vB - vT) * (by / imgH);
            const float v1 = vT + (vB - vT) * ((by + bh) / imgH);

            if (t < 0.02f)
            {
                const float qx[4] = { imgX + bx, imgX + bx + bw, imgX + bx, imgX + bx + bw };
                const float qy[4] = { imgY + by, imgY + by, imgY + by + bh, imgY + by + bh };

                emitQuad(node.spriteTex, qx, qy, u0, v0, u1, v1,
                         1.0f, 1.0f, 1.0f, 1.0f, eSpriteBlendMode::NORMAL);
                continue;
            }

            const float pcx = bx + bw * 0.5f, pcy = by + bh * 0.5f;

            const float dirx = (pcx - imgW * 0.5f) / (imgW * 0.5f);
            const float diry = (pcy - imgH * 0.5f) / (imgH * 0.5f);

            const float dx = dirx * imgW * 0.35f * te
                           + (h1 - 0.5f) * imgW * 0.25f * te
                           + std::sin(te * 10.0f + h1 * 6.28f) * 4.0f * te;

            const float dy = diry * imgH * 0.35f * te
                           + imgH * (0.9f * te * te - 0.25f * te)
                           + (h2 - 0.5f) * imgH * 0.15f * te;

            const float rot = (h2 - 0.5f) * 2.0f * 4.0f * te
                            + std::sin(te * 8.0f + h2 * 6.28f) * 0.3f * te;

            const float sc = 1.0f - 0.4f * te;
            const float aT = 1.0f - std::clamp((t - 0.7f) / 0.3f, 0.0f, 1.0f);

            if (aT <= 0.02f)
                continue;

            const float c = std::cos(rot) * sc, s = std::sin(rot) * sc;

            const float lx[4] = { bx - pcx, bx + bw - pcx, bx - pcx, bx + bw - pcx };
            const float ly[4] = { by - pcy, by - pcy, by + bh - pcy, by + bh - pcy };

            float Qx[4], Qy[4];

            for (int q = 0; q < 4; ++q)
            {
                Qx[q] = imgX + pcx + (lx[q] * c - ly[q] * s) + dx;
                Qy[q] = imgY + pcy + (lx[q] * s + ly[q] * c) + dy;
            }

            const int pi = (int)(h4 * 5.0f) % 5;

            if (h3 < 0.35f)
            {
                emitQuad(ptrWhiteBox->getTexture(), Qx, Qy, wu0, wv0, wu1, wv1,
                         PAL[pi][0], PAL[pi][1], PAL[pi][2], aT,
                         eSpriteBlendMode::NORMAL);
            }
            else
            {
                emitQuad(node.spriteTex, Qx, Qy, u0, v0, u1, v1,
                         0.5f + 0.5f * PAL[pi][0],
                         0.5f + 0.5f * PAL[pi][1],
                         0.5f + 0.5f * PAL[pi][2],
                         aT, eSpriteBlendMode::NORMAL);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// idle "vortex"
///////////////////////////////////////////////////////////////////////////////

inline void renderImgVortex(const HTMLNode& node, float imgX, float imgY, float imgW, float imgH, float now)
{
    if (!node.spriteTex || imgW <= 0 || imgH <= 0)
        return;

    auto ptrWhiteBox = Engine::getCfg().wb;
    if (!ptrWhiteBox)
        return;

    const float wu0 = ptrWhiteBox->_uvs[CSprite::VERT_ULX];
    const float wv0 = ptrWhiteBox->_uvs[CSprite::VERT_ULY];
    const float wu1 = ptrWhiteBox->_uvs[CSprite::VERT_URX];
    const float wv1 = ptrWhiteBox->_uvs[CSprite::VERT_BLY];

    const float uL = node.spriteUV[0], vT = node.spriteUV[1];
    const float uR = node.spriteUV[2], vB = node.spriteUV[3];

    const float active = std::max(0.1f, node.idleDur > 0.0f ? node.idleDur : 4.5f);
    const float pause  = node.idleDelay > 0.0f ? node.idleDelay : 1.2f;
    const float cycle  = active + pause;

    float lt = std::fmod(now, cycle);
    if (lt < 0.0f)
        lt += cycle;

    if (lt >= active)
    {
        renderQuadDirect(node.spriteTex,
                         imgX, imgY, imgX + imgW, imgY + imgH,
                         uL, vT, uR, vB,
                         1.0f, 1.0f, 1.0f, 1.0f,
                         eSpriteBlendMode::NORMAL);
        return;
    }

    const float p = lt / active;
    const float TAU = 6.2831853f;
    const float ci = std::floor(now / cycle);

    const float r2 = hash21(ci * 17.3f + 9.9f, 55.5f);
    const float r3 = hash21(ci * 43.1f + 7.7f, 21.3f);
    const float dir = (r2 > 0.5f) ? 1.0f : -1.0f;

    float I;
    float alpha;

    if (p < 0.55f)
    {
        const float t = p / 0.55f;
        I = t * t;
        alpha = 1.0f - std::clamp((I - 0.90f) / 0.10f, 0.0f, 1.0f);
    }
    else if (p < 0.62f)
    {
        I = 1.0f;
        alpha = 0.0f;
    }
    else
    {
        const float q = (p - 0.62f) / 0.38f;
        I = 0.0f;
        alpha = q * q * (3.0f - 2.0f * q);
    }

    if (alpha <= 0.01f)
        return;

    const float cx = imgW * 0.5f, cy = imgH * 0.5f;
    const float Rmax = std::sqrt(cx * cx + cy * cy);

    auto shOf = [&](float) {
        return 1.0f - I * I;
    };

    auto twistOf = [&](float prof) {
        return dir * I * I * (3.0f + 15.0f * I * std::pow(prof, 1.3f))
             + dir * (4.0f * p) * I * (0.3f + 0.7f * prof);
    };

    auto fwd = [&](float lx, float ly, float& sx, float& sy) {
        const float dx = lx - cx, dy = ly - cy;
        const float r = std::sqrt(dx * dx + dy * dy);
        const float prof = std::clamp(1.0f - r / Rmax, 0.0f, 1.0f);
        const float ang = std::atan2(dy, dx) + twistOf(prof);
        const float rs = r * shOf(prof);

        sx = imgX + cx + std::cos(ang) * rs;
        sy = imgY + cy + std::sin(ang) * rs;
    };

    auto emitPoly = [&](const float* sx, const float* sy,
                        const float* ux, const float* uy, int n)
    {
        if (n < 3)
            return;

        for (int i = 1; i + 1 < n; ++i)
        {
            float verts[8];
            verts[CSprite::VERT_ULX] = sx[0]; verts[CSprite::VERT_ULY] = sy[0];
            verts[CSprite::VERT_URX] = sx[i]; verts[CSprite::VERT_URY] = sy[i];
            verts[CSprite::VERT_BLX] = sx[i + 1]; verts[CSprite::VERT_BLY] = sy[i + 1];
            verts[CSprite::VERT_BRX] = sx[i + 1]; verts[CSprite::VERT_BRY] = sy[i + 1];

            float uvs[8];
            uvs[CSprite::VERT_ULX] = uL + (uR - uL) * (ux[0] / imgW);
            uvs[CSprite::VERT_ULY] = vT + (vB - vT) * (uy[0] / imgH);
            uvs[CSprite::VERT_URX] = uL + (uR - uL) * (ux[i] / imgW);
            uvs[CSprite::VERT_URY] = vT + (vB - vT) * (uy[i] / imgH);
            uvs[CSprite::VERT_BLX] = uL + (uR - uL) * (ux[i + 1] / imgW);
            uvs[CSprite::VERT_BLY] = vT + (vB - vT) * (uy[i + 1] / imgH);
            uvs[CSprite::VERT_BRX] = uvs[CSprite::VERT_BLX];
            uvs[CSprite::VERT_BRY] = uvs[CSprite::VERT_BLY];

            float col[4] = { 1.0f, 1.0f, 1.0f, alpha };
            CSprite::renderVerts(node.spriteTex, verts, uvs, col, eSpriteBlendMode::NORMAL);
        }
    };

    auto clipSeg = [&](float& ax, float& ay, float& bx, float& by) -> bool
    {
        float t0 = 0.0f, t1 = 1.0f;
        const float dx = bx - ax, dy = by - ay;

        const float pp[4] = { -dx, dx, -dy, dy };
        const float qq[4] = { ax, imgW - ax, ay, imgH - ay };

        for (int i = 0; i < 4; ++i)
        {
            if (pp[i] == 0.0f)
            {
                if (qq[i] < 0.0f)
                    return false;
            }
            else
            {
                const float rr = qq[i] / pp[i];

                if (pp[i] < 0.0f)
                {
                    if (rr > t1) return false;
                    if (rr > t0) t0 = rr;
                }
                else
                {
                    if (rr < t0) return false;
                    if (rr < t1) t1 = rr;
                }
            }
        }

        const float ax0 = ax, ay0 = ay;
        ax = ax0 + t0 * dx; ay = ay0 + t0 * dy;
        bx = ax0 + t1 * dx; by = ay0 + t1 * dy;

        return true;
    };

    auto emitSeg = [&](float ax, float ay, float bx, float by, float w,
                       float r, float g, float b, float a)
    {
        const float dx = bx - ax, dy = by - ay;
        const float len = std::sqrt(dx * dx + dy * dy);

        if (len < 0.5f || a <= 0.02f)
            return;

        const float px = -dy / len * w * 0.5f;
        const float py =  dx / len * w * 0.5f;

        const float qx[4] = { ax + px, bx + px, ax - px, bx - px };
        const float qy[4] = { ay + py, by + py, ay - py, by - py };

        float verts[8];
        verts[CSprite::VERT_ULX] = qx[0]; verts[CSprite::VERT_ULY] = qy[0];
        verts[CSprite::VERT_URX] = qx[1]; verts[CSprite::VERT_URY] = qy[1];
        verts[CSprite::VERT_BLX] = qx[2]; verts[CSprite::VERT_BLY] = qy[2];
        verts[CSprite::VERT_BRX] = qx[3]; verts[CSprite::VERT_BRY] = qy[3];

        float uvs[8] = { wu0, wv0, wu1, wv0, wu0, wv1, wu1, wv1 };
        float col[4] = { r, g, b, a };

        CSprite::renderVerts(ptrWhiteBox->getTexture(), verts, uvs, col, eSpriteBlendMode::ADDITIVE);
    };

    const int M = 16;
    const int K = 24;

    for (int m = 0; m < M; ++m)
    {
        const float r0 = Rmax * (float)m / M;
        const float r1 = Rmax * (float)(m + 1) / M;

        for (int k = 0; k < K; ++k)
        {
            const float a0 = (float)k / K * TAU;
            const float a1 = (float)(k + 1) / K * TAU;

            GlarePoly poly;
            poly.n = 4;

            poly.x[0] = cx + std::cos(a0) * r0; poly.y[0] = cy + std::sin(a0) * r0;
            poly.x[1] = cx + std::cos(a0) * r1; poly.y[1] = cy + std::sin(a0) * r1;
            poly.x[2] = cx + std::cos(a1) * r1; poly.y[2] = cy + std::sin(a1) * r1;
            poly.x[3] = cx + std::cos(a1) * r0; poly.y[3] = cy + std::sin(a1) * r0;

            clipGlarePolyToRect(poly, 0.0f, 0.0f, imgW, imgH);

            if (poly.n < 3)
                continue;

            float sx[16], sy[16], ux[16], uy[16];

            for (int v = 0; v < poly.n; ++v)
            {
                ux[v] = poly.x[v];
                uy[v] = poly.y[v];
                fwd(poly.x[v], poly.y[v], sx[v], sy[v]);
            }

            emitPoly(sx, sy, ux, uy, poly.n);
        }
    }

    const float swA = std::fabs(I);

    if (swA > 0.03f)
    {
        for (int s = 0; s < 3; ++s)
        {
            const float base = s * 2.094f + r3 * TAU;

            for (int q = 0; q < 8; ++q)
            {
                const float t = (float)q / 7.0f;
                const float rad = Rmax * (0.85f - 0.65f * t);
                const float angA = base + dir * (t * 4.5f + 4.0f * p * 0.30f);
                const float angB = angA + 0.25f;

                float ax = cx + std::cos(angA) * rad;
                float ay = cy + std::sin(angA) * rad;
                float bx = cx + std::cos(angB) * rad;
                float by = cy + std::sin(angB) * rad;

                if (!clipSeg(ax, ay, bx, by))
                    continue;

                float sax, say, sbx, sby;
                fwd(ax, ay, sax, say);
                fwd(bx, by, sbx, sby);

                const float ea = 0.22f * swA * (1.0f - 0.5f * t) * alpha;

                emitSeg(sax, say, sbx, sby,
                        2.0f + 3.0f * (1.0f - t),
                        0.55f * ea, 0.75f * ea, 1.0f * ea, 1.0f);
            }
        }

        const float gEnv = std::clamp(I * (1.0f - I) * 4.0f, 0.0f, 1.0f) * alpha;
        const float gs = Rmax * 0.25f * swA;
        const float gsz[3] = { gs, gs * 0.55f, gs * 0.25f };
        const float gal[3] = { 0.06f, 0.10f, 0.16f };

        for (int g = 0; g < 3; ++g)
        {
            const float a = gal[g] * gEnv;

            if (a <= 0.01f)
                continue;

            renderQuadDirect(ptrWhiteBox->getTexture(),
                             imgX + cx - gsz[g] * 0.5f, imgY + cy - gsz[g] * 0.5f,
                             imgX + cx + gsz[g] * 0.5f, imgY + cy + gsz[g] * 0.5f,
                             wu0, wv0, wu1, wv1,
                             0.60f * a, 0.80f * a, 1.0f * a, 1.0f,
                             eSpriteBlendMode::ADDITIVE);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// idle "dust"
///////////////////////////////////////////////////////////////////////////////

inline void renderImgDust(const HTMLNode& node, float imgX, float imgY, float imgW, float imgH, float now)
{
    if (!node.spriteTex || imgW <= 0 || imgH <= 0)
        return;

    const float uL = node.spriteUV[0], vT = node.spriteUV[1];
    const float uR = node.spriteUV[2], vB = node.spriteUV[3];

    const float active = std::max(0.1f, node.idleDur > 0.0f ? node.idleDur : 4.5f);
    const float pause  = node.idleDelay > 0.0f ? node.idleDelay : 1.2f;
    const float cycle  = active + pause;

    float lt = std::fmod(now, cycle);
    if (lt < 0.0f)
        lt += cycle;

    if (lt >= active)
    {
        renderQuadDirect(node.spriteTex,
                         imgX, imgY, imgX + imgW, imgY + imgH,
                         uL, vT, uR, vB,
                         1.0f, 1.0f, 1.0f, 1.0f,
                         eSpriteBlendMode::NORMAL);
        return;
    }

    const float p = lt / active;
    const float ci = std::floor(now / cycle);

    const float r2 = hash21(ci * 17.3f + 9.9f, 55.5f);
    const float r3 = hash21(ci * 43.1f + 7.7f, 21.3f);
    const float r4 = hash21(ci * 55.5f + 2.2f, 77.7f);

    const float dir   = (r2 > 0.5f) ? 1.0f : -1.0f;
    const float windX = dir * (0.7f + 0.6f * r3);
    const float windY = (r4 - 0.5f) * 0.6f;

    auto easeIO = [](float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    };

    const float wd = -0.15f + 1.30f * easeIO(std::clamp(p / 0.45f, 0.0f, 1.0f));
    const float wr = -0.15f + 1.30f * easeIO(std::clamp((p - 0.55f) / 0.45f, 0.0f, 1.0f));

    const float B  = std::clamp(std::min(imgW, imgH) * 0.10f, 10.0f, 28.0f);
    const int   nx = (int)std::ceil(imgW / B);
    const int   ny = (int)std::ceil(imgH / B);

    for (int iy = 0; iy < ny; ++iy)
    {
        for (int ix = 0; ix < nx; ++ix)
        {
            const float bx = ix * B, by = iy * B;
            const float bw = std::min(B, imgW - bx);
            const float bh = std::min(B, imgH - by);

            if (bw < 2.0f || bh < 2.0f)
                continue;

            const float h1 = hash21(ix * 12.98f + 3.7f, iy * 7.13f + 1.3f);
            const float h2 = hash21(ix * 91.17f + 7.7f, iy * 47.7f + 9.1f);
            const float h3 = hash21(ix * 47.7f + 9.2f,  iy * 23.7f + 5.5f);

            const float ax = (dir > 0.0f) ? (bx / imgW) : (1.0f - (bx + bw) / imgW);
            const float a  = ax * 0.85f + (by / imgH) * 0.15f + (h1 - 0.5f) * 0.12f;

            float s;
            if (p < 0.45f)      s = std::clamp((wd - a) / 0.18f, 0.0f, 1.0f);
            else if (p < 0.55f) s = 1.0f;
            else                s = 1.0f - std::clamp((wr - a) / 0.18f, 0.0f, 1.0f);

            const float u0 = uL + (uR - uL) * (bx / imgW);
            const float u1 = uL + (uR - uL) * ((bx + bw) / imgW);
            const float v0 = vT + (vB - vT) * (by / imgH);
            const float v1 = vT + (vB - vT) * ((by + bh) / imgH);

            if (s < 0.98f)
            {
                const float cxm = bx + bw * 0.5f, cym = by + bh * 0.5f;
                const float dx = windX * 14.0f * s;
                const float dy = (windY * 10.0f - 3.0f) * s;
                const float sc = 1.0f - 0.25f * s;
                const float hw = bw * 0.5f * sc, hh = bh * 0.5f * sc;

                renderQuadDirect(node.spriteTex,
                                 imgX + cxm - hw + dx, imgY + cym - hh + dy,
                                 imgX + cxm + hw + dx, imgY + cym + hh + dy,
                                 u0, v0, u1, v1,
                                 1.0f, 1.0f, 1.0f, 1.0f - s,
                                 eSpriteBlendMode::NORMAL);
            }

            const float bell = std::clamp(s * (1.0f - s) * 4.0f, 0.0f, 1.0f);

            if (bell > 0.03f)
            {
                for (int k = 0; k < 3; ++k)
                {
                    const float ph = hash21(ix * 13.7f + k * 57.1f, iy * 7.9f + k * 91.7f + ci * 3.3f);
                    float eph = now * (1.5f + 1.5f * ph) + ph * 100.0f;
                    const float rise = eph - std::floor(eph);
                    const float off = (p < 0.55f) ? rise : (1.0f - rise);

                    const float px = bx + bw * (0.2f + 0.6f * h2)
                                   + windX * off * (18.0f + 26.0f * ph)
                                   + std::sin(now * 4.0f + ph * 6.28f) * 2.0f;

                    const float py = by + bh * (0.2f + 0.6f * h3)
                                   + windY * off * 18.0f - off * 6.0f;

                    const float ea = bell * std::sin(rise * 3.14159265f) * 0.8f;

                    if (ea <= 0.02f)
                        continue;

                    const float es = (1.0f + 2.0f * ph) * (1.0f - 0.5f * rise);

                    renderQuadDirect(node.spriteTex,
                                     imgX + px - es * 0.5f, imgY + py - es * 0.5f,
                                     imgX + px + es * 0.5f, imgY + py + es * 0.5f,
                                     u0, v0, u1, v1,
                                     1.0f * ea, 0.72f * ea, 0.38f * ea, 1.0f,
                                     eSpriteBlendMode::ADDITIVE);
                }
            }

            if (s >= 0.98f && h2 > 0.5f)
            {
                const float ph = hash21(ix * 29.9f + ci * 1.9f, iy * 17.1f);
                float eph = now * (0.6f + 0.8f * ph) + ph * 100.0f;
                const float rise = eph - std::floor(eph);

                const float px = bx + bw * ph + windX * rise * 30.0f;
                const float py = by + bh * (1.0f - ph) + windY * rise * 20.0f - rise * 10.0f;
                const float ea = 0.12f * std::sin(rise * 3.14159265f);

                if (ea > 0.02f)
                {
                    const float es = 1.0f + 1.5f * ph;

                    renderQuadDirect(node.spriteTex,
                                     imgX + px - es * 0.5f, imgY + py - es * 0.5f,
                                     imgX + px + es * 0.5f, imgY + py + es * 0.5f,
                                     u0, v0, u1, v1,
                                     0.9f * ea, 0.65f * ea, 0.35f * ea, 1.0f,
                                     eSpriteBlendMode::ADDITIVE);
                }
            }

            const float frontPos = (p < 0.5f) ? wd : wr;
            const float gd = a - frontPos;
            const float glow = std::exp(-gd * gd * 900.0f);

            if (glow > 0.03f)
            {
                const float ga = glow * 0.25f;

                renderQuadDirect(node.spriteTex,
                                 imgX + bx, imgY + by, imgX + bx + bw, imgY + by + bh,
                                 u0, v0, u1, v1,
                                 1.0f * ga, 0.75f * ga, 0.40f * ga, 1.0f,
                                 eSpriteBlendMode::ADDITIVE);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// idle "melt"
///////////////////////////////////////////////////////////////////////////////

inline void renderImgMelt(const HTMLNode& node, float imgX, float imgY, float imgW, float imgH, float now)
{
    if (!node.spriteTex || imgW <= 0 || imgH <= 0)
        return;

    const float uL = node.spriteUV[0], vT = node.spriteUV[1];
    const float uR = node.spriteUV[2], vB = node.spriteUV[3];

    const float active  = std::max(0.1f, node.idleDur > 0.0f ? node.idleDur : 4.0f);
    const float rebuild = 1.0f;
    const float pause   = node.idleDelay > 0.0f ? node.idleDelay : 1.2f;
    const float cycle   = active + rebuild + pause;

    float lt = std::fmod(now, cycle);
    if (lt < 0.0f)
        lt += cycle;

    if (lt >= active + rebuild)
    {
        renderQuadDirect(node.spriteTex,
                         imgX, imgY, imgX + imgW, imgY + imgH,
                         uL, vT, uR, vB,
                         1.0f, 1.0f, 1.0f, 1.0f,
                         eSpriteBlendMode::NORMAL);
        return;
    }

    if (lt >= active)
    {
        const float r = (lt - active) / rebuild;
        const float a = r * r * (3.0f - 2.0f * r);

        if (a > 0.001f)
        {
            renderQuadDirect(node.spriteTex,
                             imgX, imgY, imgX + imgW, imgY + imgH,
                             uL, vT, uR, vB,
                             1.0f, 1.0f, 1.0f, a,
                             eSpriteBlendMode::NORMAL);
        }

        return;
    }

    const float p = lt / active;
    const float TAU = 6.2831853f;
    const float ci = std::floor(now / cycle);

    const float r1 = hash21(ci * 91.7f + 1.1f, 3.3f);
    const float r2 = hash21(ci * 17.3f + 9.9f, 55.5f);
    const float r3 = hash21(ci * 43.1f + 7.7f, 21.3f);
    const float r4 = hash21(ci * 55.5f + 2.2f, 77.7f);

    const float ph1 = r1 * TAU, ph2 = r2 * TAU, ph3 = r3 * TAU, ph4 = r4 * TAU;

    const int NT = 5;
    float txk[5], twk[5], tak[5];

    for (int k = 0; k < NT; ++k)
    {
        txk[k] = (0.08f + 0.84f * hash21(k * 37.7f + ci * 3.1f, 9.9f)) * imgW;
        twk[k] = imgW * (0.015f + 0.025f * hash21(k * 17.3f + ci, 41.7f));
        tak[k] = 0.35f + 0.45f * hash21(k * 23.9f + ci * 7.7f, 5.5f);
    }

    auto edgeF = [&](float x) {
        const float u = x / imgW;

        float n = 0.45f * std::sin(TAU * 1.3f * u + ph1 + now * 0.10f)
                + 0.30f * std::sin(TAU * 3.7f * u + ph2 - now * 0.07f)
                + 0.15f * std::sin(TAU * 7.9f * u + ph3)
                + 0.10f * std::sin(TAU * 14.0f * u + ph4);

        float f = 0.80f + 0.30f * n;

        for (int k = 0; k < NT; ++k)
        {
            const float d = (x - txk[k]) / twk[k];
            f -= tak[k] * std::exp(-d * d);
        }

        return std::clamp(f, 0.06f, 1.30f);
    };

    const float base  = p * p * imgH * 1.10f;
    const float q     = std::clamp((p - 0.70f) / 0.30f, 0.0f, 1.0f);
    const float extra = q * q * imgH * 1.30f;

    const float softIn = std::clamp(p / 0.35f, 0.0f, 1.0f);
    const float soft = 0.20f * std::sin(softIn * 3.14159265f);

    const float trailGate = std::clamp(p * 4.0f, 0.0f, 1.0f)
                          * std::clamp((1.0f - p) / 0.15f, 0.0f, 1.0f);

    const float trailLen = imgH * 0.35f;
    const int NC = (int)std::clamp(imgW / 4.0f, 40.0f, 140.0f);

    for (int i = 0; i < NC; ++i)
    {
        const float x0 = imgW * i / NC;
        const float x1 = imgW * (i + 1) / NC;
        const float xm = (x0 + x1) * 0.5f;

        const float u0 = uL + (uR - uL) * (x0 / imgW);
        const float u1 = uL + (uR - uL) * (x1 / imgW);

        const float hStreak = hash21(i * 7.7f + ci * 3.1f, 3.3f);
        const float streak = 0.6f + 0.8f * hStreak;

        const float dy = base * edgeF(xm) + extra;

        const float bandTopL = std::max(0.0f, dy - trailLen);
        const float bandBotL = std::min(imgH, dy);

        if (trailGate > 0.01f && bandBotL - bandTopL > 1.0f)
        {
            for (int s = 0; s < 3; ++s)
            {
                const float t0 = bandTopL + (bandBotL - bandTopL) * (float)s / 3.0f;
                const float t1 = bandTopL + (bandBotL - bandTopL) * (float)(s + 1) / 3.0f;
                const float k = (float)(s + 1) / 3.0f;
                const float ta = trailGate * streak * (0.03f + 0.15f * k * k);

                if (ta <= 0.01f)
                    continue;

                const float smear = (dy - t0) * 0.35f + 4.0f;
                const float v0 = vT + (vB - vT) * std::clamp((t0 + smear) / imgH, 0.0f, 1.0f);
                const float v1 = vT + (vB - vT) * std::clamp((t1 + smear) / imgH, 0.0f, 1.0f);

                renderQuadDirect(node.spriteTex,
                                 imgX + x0, imgY + t0, imgX + x1, imgY + t1,
                                 u0, v0, u1, v1,
                                 1.0f, 1.0f, 1.0f, ta,
                                 eSpriteBlendMode::NORMAL);
            }

            const float gh = std::max(2.0f, imgH * 0.01f);
            const float gTop = std::max(0.0f, dy - gh);
            const float gl = trailGate * 0.10f * streak;

            if (gl > 0.01f && dy > 0.0f)
            {
                const float gv0 = vT + (vB - vT) * std::clamp((gTop + 6.0f) / imgH, 0.0f, 1.0f);
                const float gv1 = vT + (vB - vT) * std::clamp((dy + 6.0f) / imgH, 0.0f, 1.0f);

                renderQuadDirect(node.spriteTex,
                                 imgX + x0, imgY + gTop, imgX + x1, imgY + std::min(imgH, dy),
                                 u0, gv0, u1, gv1,
                                 0.80f * gl, 0.90f * gl, 1.0f * gl, 1.0f,
                                 eSpriteBlendMode::ADDITIVE);
            }
        }

        const float yTop = imgY + dy;

        if (yTop >= imgY + imgH - 0.5f)
            continue;

        const float stretch = std::min(imgH * 0.5f, dy * 0.22f);
        const float totalH = imgH + stretch;
        const float visH = (imgY + imgH) - yTop;
        const float vBot = vT + (vB - vT) * std::clamp(visH / totalH, 0.0f, 1.0f);

        renderQuadDirect(node.spriteTex,
                         imgX + x0, yTop, imgX + x1, imgY + imgH,
                         u0, vT, u1, vBot,
                         1.0f - soft, 1.0f - soft, 1.0f - soft * 0.8f, 1.0f,
                         eSpriteBlendMode::NORMAL);

        const float gate = std::sin(std::clamp(p * 1.2f, 0.0f, 1.0f) * 3.14159265f);

        if (gate > 0.05f && dy > 1.0f)
        {
            const float ga = 0.15f * gate;
            const float gh2 = std::max(1.5f, imgH * 0.008f);

            renderQuadDirect(node.spriteTex,
                             imgX + x0, yTop, imgX + x1, yTop + gh2,
                             u0, vT, u1, vT + (vB - vT) * (gh2 / totalH),
                             0.90f * ga, 0.95f * ga, 1.0f * ga, 1.0f,
                             eSpriteBlendMode::ADDITIVE);
        }
    }

    const float dropFade = std::clamp((1.0f - p) / 0.10f, 0.0f, 1.0f);

    if (dropFade > 0.01f)
    {
        for (int k = 0; k < 20; ++k)
        {
            const float phd1 = hash21(k * 57.1f + ci * 3.3f, 91.7f);
            const float phd3 = hash21(k * 29.9f + ci * 1.9f, 17.1f);

            const float dx = phd1 * imgW;
            const float edgeY = imgY + base * edgeF(dx) + extra;

            if (edgeY <= imgY + 2.0f || edgeY >= imgY + imgH - 2.0f)
                continue;

            const float spd = 1.2f + 1.6f * phd3;
            float eph = now * spd + phd1 * 100.0f;
            const float rise = eph - std::floor(eph);
            const float riseE = rise * rise * 0.6f + rise * 0.4f;
            const float y = edgeY + riseE * (imgY + imgH + 10.0f - edgeY);

            if (y > imgY + imgH - 1.0f)
                continue;

            const float ea = (1.0f - rise) * 0.8f * std::clamp(p * 3.0f, 0.0f, 1.0f) * dropFade;

            if (ea <= 0.02f)
                continue;

            const float dw = 1.5f + 2.0f * phd3;
            const float dh = 3.0f + 5.0f * phd3;

            const float ud0 = uL + (uR - uL) * (dx / imgW);
            const float ud1 = uL + (uR - uL) * std::min(1.0f, (dx + 3.0f) / imgW);

            renderQuadDirect(node.spriteTex,
                             imgX + dx - dw * 0.5f, y,
                             imgX + dx + dw * 0.5f, y + dh,
                             ud0, vB - 0.08f, ud1, vB - 0.01f,
                             1.0f, 1.0f, 1.0f, ea,
                             eSpriteBlendMode::NORMAL);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// idle "ice"
///////////////////////////////////////////////////////////////////////////////

inline void renderImgIce(const HTMLNode& node, float imgX, float imgY, float imgW, float imgH, float now)
{
    if (!node.spriteTex || imgW <= 0 || imgH <= 0)
        return;

    auto ptrWhiteBox = Engine::getCfg().wb;
    if (!ptrWhiteBox)
        return;

    const float wu0 = ptrWhiteBox->_uvs[CSprite::VERT_ULX];
    const float wv0 = ptrWhiteBox->_uvs[CSprite::VERT_ULY];
    const float wu1 = ptrWhiteBox->_uvs[CSprite::VERT_URX];
    const float wv1 = ptrWhiteBox->_uvs[CSprite::VERT_BLY];

    const float uL = node.spriteUV[0], vT = node.spriteUV[1];
    const float uR = node.spriteUV[2], vB = node.spriteUV[3];

    const float active  = std::max(0.1f, node.idleDur > 0.0f ? node.idleDur : 5.0f);
    const float rebuild = 1.0f;
    const float pause   = node.idleDelay > 0.0f ? node.idleDelay : 1.2f;
    const float cycle   = active + rebuild + pause;

    float lt = std::fmod(now, cycle);
    if (lt < 0.0f)
        lt += cycle;

    if (lt >= active + rebuild)
    {
        renderQuadDirect(node.spriteTex, imgX, imgY, imgX + imgW, imgY + imgH,
                         uL, vT, uR, vB, 1, 1, 1, 1, eSpriteBlendMode::NORMAL);
        return;
    }

    if (lt >= active)
    {
        const float r = (lt - active) / rebuild;
        const float a = r * r * (3.0f - 2.0f * r);

        if (a > 0.001f)
            renderQuadDirect(node.spriteTex, imgX, imgY, imgX + imgW, imgY + imgH,
                             uL, vT, uR, vB, 1, 1, 1, a, eSpriteBlendMode::NORMAL);

        return;
    }

    const float p = lt / active;
    const float ci = std::floor(now / cycle);

    const float r1 = hash21(ci * 91.7f + 1.1f, 3.3f);
    const float r2 = hash21(ci * 17.3f + 9.9f, 55.5f);
    const float r4 = hash21(ci * 55.5f + 2.2f, 77.7f);

    const float fr = std::floor(now * 30.0f);

    const int corner = (int)(r1 * 4.0f) % 4;
    const float cx0 = (corner == 1 || corner == 3) ? imgW : 0.0f;
    const float cy0 = (corner >= 2) ? imgH : 0.0f;

    const float maxR = std::sqrt(imgW * imgW + imgH * imgH);

    float fzIn = std::clamp(p / 0.45f, 0.0f, 1.0f);
    const float R = (1.0f - (1.0f - fzIn) * (1.0f - fzIn) * (1.0f - fzIn)) * maxR * 1.05f;

    const float ixp = imgW * (0.35f + 0.30f * r2);
    const float iyp = imgH * (0.35f + 0.30f * r2 * 0.7f + 0.1f);

    const float ft = std::clamp((p - 0.60f) / 0.40f, 0.0f, 1.0f);
    const float partA = 1.0f - std::clamp((ft - 0.85f) / 0.15f, 0.0f, 1.0f);

    float crackA = 0.0f;

    if (p > 0.48f && p < 0.66f)
    {
        const float up = std::clamp((p - 0.48f) / 0.03f, 0.0f, 1.0f);
        const float dn = std::clamp((0.66f - p) / 0.06f, 0.0f, 1.0f);
        crackA = up * dn;
    }

    const int K = 8, NB = 4;

    float RAD[5] = { 0.0f, maxR * 0.18f, maxR * 0.42f, maxR * 0.70f, maxR * 1.25f };
    float rayX[8][5], rayY[8][5];

    for (int k = 0; k < K; ++k)
    {
        const float base = (float)k / K * 6.2831853f + r4 * 6.2831853f;

        rayX[k][0] = ixp;
        rayY[k][0] = iyp;

        for (int i = 1; i <= NB; ++i)
        {
            const float ang = base + (hash21(k * 3.3f + i * 11.7f, ci) - 0.5f) * 0.22f;
            const float rad = RAD[i] * (1.0f + (hash21(k * 9.1f + i * 5.5f, ci * 1.3f) - 0.5f) * 0.12f);

            rayX[k][i] = ixp + std::cos(ang) * rad;
            rayY[k][i] = iyp + std::sin(ang) * rad;
        }
    }

    auto emitArb = [&](CTexturePtr tex, const float* cx, const float* cy,
                       float u0, float v0, float u1, float v1,
                       float r, float g, float b, float a, eSpriteBlendMode blend)
    {
        if (a <= 0.02f)
            return;

        float verts[8];
        verts[CSprite::VERT_ULX] = cx[0]; verts[CSprite::VERT_ULY] = cy[0];
        verts[CSprite::VERT_URX] = cx[1]; verts[CSprite::VERT_URY] = cy[1];
        verts[CSprite::VERT_BLX] = cx[2]; verts[CSprite::VERT_BLY] = cy[2];
        verts[CSprite::VERT_BRX] = cx[3]; verts[CSprite::VERT_BRY] = cy[3];

        float uvs[8];
        uvs[CSprite::VERT_ULX] = u0; uvs[CSprite::VERT_ULY] = v0;
        uvs[CSprite::VERT_URX] = u1; uvs[CSprite::VERT_URY] = v0;
        uvs[CSprite::VERT_BLX] = u0; uvs[CSprite::VERT_BLY] = v1;
        uvs[CSprite::VERT_BRX] = u1; uvs[CSprite::VERT_BRY] = v1;

        float col[4] = { r, g, b, a };

        CSprite::renderVerts(tex, verts, uvs, col, blend);
    };

    auto emitSeg = [&](float ax, float ay, float bxx, float byy, float w,
                       float r, float g, float b, float a)
    {
        const float dx = bxx - ax, dy = byy - ay;
        const float len = std::sqrt(dx * dx + dy * dy);

        if (len < 0.5f || a <= 0.02f)
            return;

        const float px = -dy / len * w * 0.5f;
        const float py =  dx / len * w * 0.5f;

        const float qx[4] = { ax + px, bxx + px, ax - px, bxx - px };
        const float qy[4] = { ay + py, byy + py, ay - py, byy - py };

        emitArb(ptrWhiteBox->getTexture(), qx, qy, wu0, wv0, wu1, wv1,
                r, g, b, a, eSpriteBlendMode::ADDITIVE);
    };

    auto clipSeg = [&](float& ax, float& ay, float& bx, float& by) -> bool
    {
        float t0 = 0.0f, t1 = 1.0f;

        const float dx = bx - ax, dy = by - ay;

        const float pp[4] = { -dx, dx, -dy, dy };
        const float qq[4] = { ax, imgW - ax, ay, imgH - ay };

        for (int i = 0; i < 4; ++i)
        {
            if (pp[i] == 0.0f)
            {
                if (qq[i] < 0.0f)
                    return false;
            }
            else
            {
                const float rr = qq[i] / pp[i];

                if (pp[i] < 0.0f)
                {
                    if (rr > t1) return false;
                    if (rr > t0) t0 = rr;
                }
                else
                {
                    if (rr < t0) return false;
                    if (rr < t1) t1 = rr;
                }
            }
        }

        const float ax0 = ax, ay0 = ay;

        ax = ax0 + t0 * dx;
        ay = ay0 + t0 * dy;
        bx = ax0 + t1 * dx;
        by = ay0 + t1 * dy;

        return true;
    };

    auto emitPoly = [&](bool bSpr, const float* sx, const float* sy,
                        const float* ux, const float* uy, int n,
                        float r, float g, float b, float a, eSpriteBlendMode blend)
    {
        if (n < 3 || a <= 0.02f)
            return;

        CTexturePtr tex = bSpr ? node.spriteTex : ptrWhiteBox->getTexture();

        for (int i = 1; i + 1 < n; ++i)
        {
            float verts[8];
            verts[CSprite::VERT_ULX] = sx[0]; verts[CSprite::VERT_ULY] = sy[0];
            verts[CSprite::VERT_URX] = sx[i]; verts[CSprite::VERT_URY] = sy[i];
            verts[CSprite::VERT_BLX] = sx[i + 1]; verts[CSprite::VERT_BLY] = sy[i + 1];
            verts[CSprite::VERT_BRX] = sx[i + 1]; verts[CSprite::VERT_BRY] = sy[i + 1];

            float uvs[8];

            if (bSpr)
            {
                uvs[CSprite::VERT_ULX] = uL + (uR - uL) * (ux[0] / imgW);
                uvs[CSprite::VERT_ULY] = vT + (vB - vT) * (uy[0] / imgH);
                uvs[CSprite::VERT_URX] = uL + (uR - uL) * (ux[i] / imgW);
                uvs[CSprite::VERT_URY] = vT + (vB - vT) * (uy[i] / imgH);
                uvs[CSprite::VERT_BLX] = uL + (uR - uL) * (ux[i + 1] / imgW);
                uvs[CSprite::VERT_BLY] = vT + (vB - vT) * (uy[i + 1] / imgH);
                uvs[CSprite::VERT_BRX] = uvs[CSprite::VERT_BLX];
                uvs[CSprite::VERT_BRY] = uvs[CSprite::VERT_BLY];
            }
            else
            {
                uvs[CSprite::VERT_ULX] = wu0; uvs[CSprite::VERT_ULY] = wv0;
                uvs[CSprite::VERT_URX] = wu1; uvs[CSprite::VERT_URY] = wv0;
                uvs[CSprite::VERT_BLX] = wu0; uvs[CSprite::VERT_BLY] = wv1;
                uvs[CSprite::VERT_BRX] = wu0; uvs[CSprite::VERT_BRY] = wv1;
            }

            float col[4] = { r, g, b, a };

            CSprite::renderVerts(tex, verts, uvs, col, blend);
        }
    };

    if (ft <= 0.0f)
    {
        const float B  = std::clamp(std::min(imgW, imgH) * 0.12f, 16.0f, 42.0f);
        const int nx = (int)std::ceil(imgW / B);
        const int ny = (int)std::ceil(imgH / B);

        for (int iy = 0; iy < ny; ++iy)
        {
            for (int ix = 0; ix < nx; ++ix)
            {
                const float bx = ix * B, by = iy * B;
                const float bw = std::min(B, imgW - bx);
                const float bh = std::min(B, imgH - by);

                if (bw < 2.0f || bh < 2.0f)
                    continue;

                const float ccx = bx + bw * 0.5f, ccy = by + bh * 0.5f;

                const float u0 = uL + (uR - uL) * (bx / imgW);
                const float u1 = uL + (uR - uL) * ((bx + bw) / imgW);
                const float v0 = vT + (vB - vT) * (by / imgH);
                const float v1 = vT + (vB - vT) * ((by + bh) / imgH);

                const float h1 = hash21(ix * 12.98f + 3.7f, iy * 7.13f + 1.3f);

                const float ddx = ccx - cx0, ddy = ccy - cy0;
                const float d = std::sqrt(ddx * ddx + ddy * ddy) + (h1 - 0.5f) * maxR * 0.05f;
                const float frozenA = std::clamp((R - d) / (maxR * 0.08f), 0.0f, 1.0f);

                const float qx[4] = { imgX + bx, imgX + bx + bw, imgX + bx, imgX + bx + bw };
                const float qy[4] = { imgY + by, imgY + by, imgY + by + bh, imgY + by + bh };

                emitArb(node.spriteTex, qx, qy, u0, v0, u1, v1,
                        1, 1, 1, 1, eSpriteBlendMode::NORMAL);

                if (frozenA > 0.02f)
                {
                    emitArb(ptrWhiteBox->getTexture(), qx, qy, wu0, wv0, wu1, wv1,
                            0.55f, 0.75f, 0.95f, 0.40f * frozenA, eSpriteBlendMode::NORMAL);

                    const float band = frozenA * (1.0f - frozenA) * 4.0f;
                    const float flick = 0.6f + 0.4f * hash21(ix * 3.3f + fr, iy * 7.7f);
                    const float ea = band * flick * 0.35f;

                    if (ea > 0.02f)
                        emitArb(ptrWhiteBox->getTexture(), qx, qy, wu0, wv0, wu1, wv1,
                                0.60f * ea, 0.80f * ea, 1.0f * ea, 1.0f, eSpriteBlendMode::ADDITIVE);
                }
            }
        }

        if (R > maxR * 0.05f)
        {
            for (int k = 0; k < 12; ++k)
            {
                const float ph1 = hash21(k * 57.1f + ci * 3.3f, 91.7f);
                const float ph2 = hash21(k * 13.7f + ci * 7.7f, 41.3f);

                const float sx = ph1 * imgW, sy = ph2 * imgH;
                const float ddx = sx - cx0, ddy = sy - cy0;

                if (std::sqrt(ddx * ddx + ddy * ddy) > R)
                    continue;

                const float tw = std::sin(now * (6.0f + 6.0f * ph1) + ph2 * 6.28f);
                const float gate = std::max(0.0f, tw);
                const float sp = gate * gate * gate * gate;

                if (sp < 0.05f)
                    continue;

                const float es = 1.0f + 1.8f * ph2;

                const float px = imgX + sx, py = imgY + sy;

                const float sxx[4] = { px - es * 0.5f, px + es * 0.5f, px - es * 0.5f, px + es * 0.5f };
                const float syy[4] = { py - es * 0.5f, py - es * 0.5f, py + es * 0.5f, py + es * 0.5f };

                emitArb(ptrWhiteBox->getTexture(), sxx, syy, wu0, wv0, wu1, wv1,
                        0.80f * sp, 0.90f * sp, 1.0f * sp, 1.0f, eSpriteBlendMode::ADDITIVE);
            }
        }
    }
    else if (partA > 0.02f)
    {
        for (int i = 0; i < NB; ++i)
        {
            for (int k = 0; k < K; ++k)
            {
                const int k1 = (k + 1) % K;

                float lx[4], ly[4];
                int n0;

                if (i == 0)
                {
                    lx[0] = ixp;         ly[0] = iyp;
                    lx[1] = rayX[k][1];  ly[1] = rayY[k][1];
                    lx[2] = rayX[k1][1]; ly[2] = rayY[k1][1];
                    n0 = 3;
                }
                else
                {
                    lx[0] = rayX[k][i];    ly[0] = rayY[k][i];
                    lx[1] = rayX[k1][i];   ly[1] = rayY[k1][i];
                    lx[2] = rayX[k1][i + 1]; ly[2] = rayY[k1][i + 1];
                    lx[3] = rayX[k][i + 1];  ly[3] = rayY[k][i + 1];
                    n0 = 4;
                }

                GlarePoly poly;
                poly.n = n0;

                for (int v = 0; v < n0; ++v)
                {
                    poly.x[v] = lx[v];
                    poly.y[v] = ly[v];
                }

                clipGlarePolyToRect(poly, 0.0f, 0.0f, imgW, imgH);

                if (poly.n < 3)
                    continue;

                float ccx = 0, ccy = 0;

                for (int v = 0; v < n0; ++v)
                {
                    ccx += lx[v];
                    ccy += ly[v];
                }

                ccx /= n0;
                ccy /= n0;

                const float hF = hash21(k * 17.7f + i * 7.3f, ci * 3.1f);
                const float fs = std::clamp((ft - 0.10f * i - hF * 0.15f) / 0.75f, 0.0f, 1.0f);
                const float e = fs * fs;

                float dirX = ccx - ixp, dirY = ccy - iyp;
                const float dl = std::sqrt(dirX * dirX + dirY * dirY);

                if (dl > 1.0f)
                {
                    dirX /= dl;
                    dirY /= dl;
                }

                const float dx = dirX * imgW * 0.12f * e + (hF - 0.5f) * imgW * 0.06f * e;
                const float dy = dirY * imgH * 0.06f * e + imgH * 1.5f * e;
                const float ang = (hF - 0.5f) * 1.6f * e;

                const float c = std::cos(ang), s = std::sin(ang);

                float sx[16], sy[16];

                for (int v = 0; v < poly.n; ++v)
                {
                    const float rxv = poly.x[v] - ccx, ryv = poly.y[v] - ccy;

                    sx[v] = imgX + ccx + rxv * c - ryv * s + dx;
                    sy[v] = imgY + ccy + rxv * s + ryv * c + dy;
                }

                float ux[16], uy[16];

                for (int v = 0; v < poly.n; ++v)
                {
                    ux[v] = poly.x[v];
                    uy[v] = poly.y[v];
                }

                emitPoly(true, sx, sy, ux, uy, poly.n, 1, 1, 1, partA, eSpriteBlendMode::NORMAL);

                emitPoly(false, sx, sy, ux, uy, poly.n,
                         0.55f, 0.75f, 0.95f, 0.40f * partA, eSpriteBlendMode::NORMAL);
            }
        }
    }

    if (crackA > 0.01f)
    {
        const float fsz = maxR * 0.06f;
        const float fa = crackA * 0.15f;

        if (fa > 0.01f)
        {
            const float fxx[4] = { imgX + ixp - fsz, imgX + ixp + fsz, imgX + ixp - fsz, imgX + ixp + fsz };
            const float fyy[4] = { imgY + iyp - fsz, imgY + iyp - fsz, imgY + iyp + fsz, imgY + iyp + fsz };

            emitArb(ptrWhiteBox->getTexture(), fxx, fyy, wu0, wv0, wu1, wv1,
                    0.70f * fa, 0.85f * fa, 1.0f * fa, 1.0f, eSpriteBlendMode::ADDITIVE);
        }

        const float wCore = std::max(1.0f, imgW * 0.002f);
        const float wGlow = std::max(2.0f, imgW * 0.006f);

        for (int k = 0; k < K; ++k)
        {
            for (int i = 0; i < NB; ++i)
            {
                float ax = rayX[k][i], ay = rayY[k][i];
                float bx = rayX[k][i + 1], by = rayY[k][i + 1];

                if (!clipSeg(ax, ay, bx, by))
                    continue;

                const float ca = crackA * (0.45f + 0.30f * hash21(i * 11.3f + k, ci * 2.7f));

                emitSeg(imgX + ax, imgY + ay, imgX + bx, imgY + by, wGlow,
                        0.30f * ca, 0.50f * ca, 0.75f * ca, 1.0f);

                emitSeg(imgX + ax, imgY + ay, imgX + bx, imgY + by, wCore,
                        0.65f * ca, 0.80f * ca, 1.0f * ca, 1.0f);
            }
        }

        for (int i = 1; i <= 3; ++i)
        {
            for (int k = 0; k < K; ++k)
            {
                const int k1 = (k + 1) % K;

                if (hash21(i * 31.7f + k * 3.3f, ci * 1.7f) < 0.15f)
                    continue;

                float ax = rayX[k][i], ay = rayY[k][i];
                float bx = rayX[k1][i], by = rayY[k1][i];

                if (!clipSeg(ax, ay, bx, by))
                    continue;

                const float ca = crackA * 0.40f;

                emitSeg(imgX + ax, imgY + ay, imgX + bx, imgY + by, wCore,
                        0.55f * ca, 0.72f * ca, 0.95f * ca, 1.0f);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// idle "echo"
///////////////////////////////////////////////////////////////////////////////

inline void renderImgEcho(const HTMLNode& node, float imgX, float imgY, float imgW, float imgH, float now)
{
    if (!node.spriteTex || imgW <= 0 || imgH <= 0)
        return;

    auto ptrWhiteBox = Engine::getCfg().wb;
    if (!ptrWhiteBox)
        return;

    const float wu0 = ptrWhiteBox->_uvs[CSprite::VERT_ULX];
    const float wv0 = ptrWhiteBox->_uvs[CSprite::VERT_ULY];
    const float wu1 = ptrWhiteBox->_uvs[CSprite::VERT_URX];
    const float wv1 = ptrWhiteBox->_uvs[CSprite::VERT_BLY];

    const float uL = node.spriteUV[0], vT = node.spriteUV[1];
    const float uR = node.spriteUV[2], vB = node.spriteUV[3];

    const float active  = std::max(0.1f, node.idleDur > 0.0f ? node.idleDur : 4.0f);
    const float rebuild = 1.0f;
    const float pause   = node.idleDelay > 0.0f ? node.idleDelay : 1.2f;
    const float cycle   = active + rebuild + pause;

    float lt = std::fmod(now, cycle);
    if (lt < 0.0f)
        lt += cycle;

    if (lt >= active + rebuild)
    {
        renderQuadDirect(node.spriteTex,
                         imgX, imgY, imgX + imgW, imgY + imgH,
                         uL, vT, uR, vB,
                         1.0f, 1.0f, 1.0f, 1.0f,
                         eSpriteBlendMode::NORMAL);
        return;
    }

    if (lt >= active)
    {
        const float r = (lt - active) / rebuild;
        const float a = r * r * (3.0f - 2.0f * r);

        if (a > 0.001f)
        {
            renderQuadDirect(node.spriteTex,
                             imgX, imgY, imgX + imgW, imgY + imgH,
                             uL, vT, uR, vB,
                             1.0f, 1.0f, 1.0f, a,
                             eSpriteBlendMode::NORMAL);
        }

        return;
    }

    const float p = lt / active;
    const float TAU = 6.2831853f;
    const float ci = std::floor(now / cycle);

    const float r1 = hash21(ci * 91.7f + 1.1f, 3.3f);
    const float r2 = hash21(ci * 17.3f + 9.9f, 55.5f);
    const float r3 = hash21(ci * 43.1f + 7.7f, 21.3f);

    const float fr = std::floor(now * 30.0f);

    const float wAng  = r1 * TAU;
    const float wStr  = 0.5f + 0.9f * r2;
    const float windX = std::cos(wAng) * wStr;
    const float windY = std::sin(wAng) * wStr;

    const float ixp = imgW * (0.35f + 0.30f * r2);
    const float iyp = imgH * (0.35f + 0.30f * r3);

    const float pBurnEnd = 0.55f;
    const float pFall    = 0.62f;

    float bpIn = std::clamp(p / pBurnEnd, 0.0f, 1.0f);
    const float bp = bpIn * bpIn * (3.0f - 2.0f * bpIn);

    const float ft = std::clamp((p - pFall) / (1.0f - pFall), 0.0f, 1.0f);
    const float partA = 1.0f - std::clamp((ft - 0.85f) / 0.15f, 0.0f, 1.0f);

    if (partA <= 0.02f)
        return;

    const float B  = std::clamp(std::min(imgW, imgH) * 0.12f, 16.0f, 42.0f);
    const int nx = (int)std::ceil(imgW / B);
    const int ny = (int)std::ceil(imgH / B);

    const float gcx   = (nx - 1) * 0.5f;
    const float gcy   = (ny - 1) * 0.5f;
    const float gmaxR = std::sqrt(gcx * gcx + gcy * gcy);

    const float BURN_W = 0.25f;

    auto emitArb = [&](CTexturePtr tex,
                       const float* cx, const float* cy,
                       float u0, float v0, float u1, float v1,
                       float r, float g, float b, float a,
                       eSpriteBlendMode blend)
    {
        if (a <= 0.02f)
            return;

        float verts[8];
        verts[CSprite::VERT_ULX] = cx[0]; verts[CSprite::VERT_ULY] = cy[0];
        verts[CSprite::VERT_URX] = cx[1]; verts[CSprite::VERT_URY] = cy[1];
        verts[CSprite::VERT_BLX] = cx[2]; verts[CSprite::VERT_BLY] = cy[2];
        verts[CSprite::VERT_BRX] = cx[3]; verts[CSprite::VERT_BRY] = cy[3];

        float uvs[8];
        uvs[CSprite::VERT_ULX] = u0; uvs[CSprite::VERT_ULY] = v0;
        uvs[CSprite::VERT_URX] = u1; uvs[CSprite::VERT_URY] = v0;
        uvs[CSprite::VERT_BLX] = u0; uvs[CSprite::VERT_BLY] = v1;
        uvs[CSprite::VERT_BRX] = u1; uvs[CSprite::VERT_BRY] = v1;

        float col[4] = { r, g, b, a };

        CSprite::renderVerts(tex, verts, uvs, col, blend);
    };

    for (int iy = 0; iy < ny; ++iy)
    {
        for (int ix = 0; ix < nx; ++ix)
        {
            const float bx = ix * B, by = iy * B;
            const float bw = std::min(B, imgW - bx);
            const float bh = std::min(B, imgH - by);

            if (bw < 2.0f || bh < 2.0f)
                continue;

            const float ccx = bx + bw * 0.5f, ccy = by + bh * 0.5f;

            const float u0 = uL + (uR - uL) * (bx / imgW);
            const float u1 = uL + (uR - uL) * ((bx + bw) / imgW);
            const float v0 = vT + (vB - vT) * (by / imgH);
            const float v1 = vT + (vB - vT) * ((by + bh) / imgH);

            const float h1 = hash21(ix * 12.98f + 3.7f, iy * 7.13f + 1.3f);
            const float h2 = hash21(ix * 91.17f + 7.7f, iy * 47.7f + 9.1f);
            const float h3 = hash21(ix * 47.7f + 9.2f,  iy * 23.7f + 5.5f);

            const float tdx = (float)ix - gcx;
            const float tdy = (float)iy - gcy;

            const float dn = std::clamp((gmaxR > 0.001f
                                         ? std::sqrt(tdx * tdx + tdy * tdy) / gmaxR
                                         : 0.0f)
                                        + (h1 - 0.5f) * 0.10f, 0.0f, 1.0f);

            const float s = std::clamp((bp * (1.0f + BURN_W) - dn) / BURN_W, 0.0f, 1.0f);

            if (ft <= 0.0f)
            {
                renderQuadDirect(node.spriteTex,
                                 imgX + bx, imgY + by, imgX + bx + bw, imgY + by + bh,
                                 u0, v0, u1, v1,
                                 1.0f, 1.0f, 1.0f, 1.0f,
                                 eSpriteBlendMode::NORMAL);

                const float front = s * s * (3.0f - 2.0f * s);
                const float cf = std::clamp((front - 0.15f) / 0.85f, 0.0f, 1.0f);

                if (cf > 0.01f)
                {
                    const float ua = u0 + (u1 - u0) * (0.5f - cf * 0.5f);
                    const float ub = u0 + (u1 - u0) * (0.5f + cf * 0.5f);
                    const float va = v0 + (v1 - v0) * (0.5f - cf * 0.5f);
                    const float vb = v0 + (v1 - v0) * (0.5f + cf * 0.5f);
                    const float t = 1.0f - 0.5f * std::clamp(s * 1.2f, 0.0f, 1.0f);

                    renderQuadDirect(node.spriteTex,
                                     imgX + ccx - bw * cf * 0.5f, imgY + ccy - bh * cf * 0.5f,
                                     imgX + ccx + bw * cf * 0.5f, imgY + ccy + bh * cf * 0.5f,
                                     ua, va, ub, vb,
                                     t, t, t, 1.0f,
                                     eSpriteBlendMode::NORMAL);
                }

                const float gate = std::clamp(s / 0.05f, 0.0f, 1.0f)
                                 * std::clamp((1.0f - s) / 0.15f, 0.0f, 1.0f);

                const float fw = bw * front, fh = bh * front;

                if (gate > 0.02f && fw > 1.0f && fh > 1.0f)
                {
                    const float fl1 = 0.70f + 0.30f * hash21(ix * 7.7f + fr,        iy * 3.3f);
                    const float fl2 = 0.70f + 0.30f * hash21(ix * 3.1f + fr * 1.7f, iy * 9.9f);
                    const float fl3 = 0.70f + 0.30f * hash21(ix * 5.5f + fr * 2.3f, iy * 7.1f);

                    const float leanX = windX * 10.0f * front;
                    const float leanY = -bh * 0.10f * front;

                    const float a1 = gate * fl1;

                    renderQuadDirect(ptrWhiteBox->getTexture(),
                                     imgX + ccx + leanX * 0.4f - fw * 0.60f, imgY + ccy + leanY * 0.4f - fh * 0.60f,
                                     imgX + ccx + leanX * 0.4f + fw * 0.60f, imgY + ccy + leanY * 0.4f + fh * 0.60f,
                                     wu0, wv0, wu1, wv1,
                                     0.50f * a1, 0.13f * a1, 0.02f * a1, 1.0f,
                                     eSpriteBlendMode::ADDITIVE);

                    const float a2 = gate * fl2;

                    renderQuadDirect(ptrWhiteBox->getTexture(),
                                     imgX + ccx + leanX * 0.7f - fw * 0.42f, imgY + ccy + leanY * 0.7f - fh * 0.42f,
                                     imgX + ccx + leanX * 0.7f + fw * 0.42f, imgY + ccy + leanY * 0.7f + fh * 0.42f,
                                     wu0, wv0, wu1, wv1,
                                     0.95f * a2, 0.42f * a2, 0.06f * a2, 1.0f,
                                     eSpriteBlendMode::ADDITIVE);

                    const float a3 = gate * fl3;

                    renderQuadDirect(ptrWhiteBox->getTexture(),
                                     imgX + ccx + leanX - fw * 0.22f, imgY + ccy + leanY - fh * 0.22f,
                                     imgX + ccx + leanX + fw * 0.22f, imgY + ccy + leanY + fh * 0.22f,
                                     wu0, wv0, wu1, wv1,
                                     1.0f * a3, 0.80f * a3, 0.30f * a3, 1.0f,
                                     eSpriteBlendMode::ADDITIVE);
                }

                const float sparkGate = std::sin(s * 3.14159265f);

                if (sparkGate > 0.10f)
                {
                    for (int k = 0; k < 3; ++k)
                    {
                        const float ph  = hash21(ix * 13.7f + k * 57.1f, iy * 7.9f + k * 91.7f);
                        const float spd = 0.8f + 0.9f * ph;

                        float eph = now * spd + ph * 100.0f;
                        const float rise = eph - std::floor(eph);

                        const float px = imgX + ccx + (hash21(ix + k * 31.7f, iy + k * 17.3f) - 0.5f) * bw * front
                                       + windX * 22.0f * rise * rise
                                       + std::sin(now * 3.0f + ph * 6.28f) * 2.0f;

                        const float py = imgY + ccy - rise * (B * 1.4f) + windY * 8.0f * rise * rise;

                        const float ea = sparkGate * (1.0f - rise) * 0.35f;

                        if (ea <= 0.02f)
                            continue;

                        const float es = 0.8f + 1.5f * ph;

                        renderQuadDirect(ptrWhiteBox->getTexture(),
                                         px - es * 0.5f, py - es * 0.5f,
                                         px + es * 0.5f, py + es * 0.5f,
                                         wu0, wv0, wu1, wv1,
                                         0.55f * ea, 0.53f * ea, 0.50f * ea, 1.0f,
                                         eSpriteBlendMode::ADDITIVE);
                    }
                }
            }
            else
            {
                const float s2 = std::clamp((ft - h2 * 0.25f) / 0.75f, 0.0f, 1.0f);
                const float e2 = s2 * s2;

                const float dy = imgH * 1.7f * e2 * (0.5f + 0.7f * h3)
                               + windY * 0.35f * imgH * ft * ft * ft;

                const float dx = ((ccx - ixp) / imgW) * imgW * 0.15f * ft
                               + (h2 - 0.5f) * imgW * 0.10f * ft
                               + windX * imgW * 0.10f * ft * ft * ft;

                const float ang = (h3 - 0.5f) * 1.4f * e2;
                const float c = std::cos(ang), sn = std::sin(ang);

                auto xf = [&](float x, float y, float& ox, float& oy)
                {
                    const float rx = x - ccx, ry = y - ccy;

                    ox = imgX + ccx + rx * c - ry * sn + dx;
                    oy = imgY + ccy + rx * sn + ry * c + dy;
                };

                float Qx[4], Qy[4];

                xf(bx,      by,      Qx[0], Qy[0]);
                xf(bx + bw, by,      Qx[1], Qy[1]);
                xf(bx,      by + bh, Qx[2], Qy[2]);
                xf(bx + bw, by + bh, Qx[3], Qy[3]);

                const float tint = 0.55f - 0.20f * ft;

                emitArb(node.spriteTex, Qx, Qy, u0, v0, u1, v1,
                        tint, tint, tint, partA, eSpriteBlendMode::NORMAL);

                if (s2 > 0.05f && s2 < 0.95f)
                {
                    for (int k = 0; k < 2; ++k)
                    {
                        const float ph = hash21(ix * 13.7f + k * 57.1f, iy * 7.9f + k * 91.7f);
                        const float es = 1.5f + 2.5f * ph;

                        const float ex = bx + bw * (0.15f + 0.70f * hash21(ix + k * 31.7f, iy + k * 17.3f))
                                       + dx + (ph - 0.5f) * 6.0f * e2;

                        const float ey = by + bh + e2 * imgH * 0.5f * (0.5f + 0.5f * ph) - es;

                        const float ea = (1.0f - s2) * 0.5f * partA;

                        if (ea <= 0.02f)
                            continue;

                        renderQuadDirect(ptrWhiteBox->getTexture(),
                                         imgX + ex - es * 0.5f, imgY + ey - es * 0.5f,
                                         imgX + ex + es * 0.5f, imgY + ey + es * 0.5f,
                                         wu0, wv0, wu1, wv1,
                                         0.45f, 0.45f, 0.45f, ea,
                                         eSpriteBlendMode::NORMAL);
                    }
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// idle "lightning"
///////////////////////////////////////////////////////////////////////////////

inline void renderImgLightning(const HTMLNode& node, float imgX, float imgY, float imgW, float imgH, float now)
{
    if (!node.spriteTex || imgW <= 0 || imgH <= 0)
        return;

    auto ptrWhiteBox = Engine::getCfg().wb;
    if (!ptrWhiteBox)
        return;

    const float wu0 = ptrWhiteBox->_uvs[CSprite::VERT_ULX];
    const float wv0 = ptrWhiteBox->_uvs[CSprite::VERT_ULY];
    const float wu1 = ptrWhiteBox->_uvs[CSprite::VERT_URX];
    const float wv1 = ptrWhiteBox->_uvs[CSprite::VERT_BLY];

    const float uL = node.spriteUV[0], vT = node.spriteUV[1];
    const float uR = node.spriteUV[2], vB = node.spriteUV[3];

    const float active  = std::max(0.1f, node.idleDur > 0.0f ? node.idleDur : 4.0f);
    const float rebuild = 1.0f;
    const float pause   = node.idleDelay > 0.0f ? node.idleDelay : 1.2f;
    const float cycle   = active + rebuild + pause;

    float lt = std::fmod(now, cycle);
    if (lt < 0.0f)
        lt += cycle;

    if (lt >= active + rebuild)
    {
        renderQuadDirect(node.spriteTex,
                         imgX, imgY, imgX + imgW, imgY + imgH,
                         uL, vT, uR, vB,
                         1.0f, 1.0f, 1.0f, 1.0f,
                         eSpriteBlendMode::NORMAL);
        return;
    }

    if (lt >= active)
    {
        const float r = (lt - active) / rebuild;
        const float a = r * r * (3.0f - 2.0f * r);

        if (a > 0.001f)
        {
            renderQuadDirect(node.spriteTex,
                             imgX, imgY, imgX + imgW, imgY + imgH,
                             uL, vT, uR, vB,
                             1.0f, 1.0f, 1.0f, a,
                             eSpriteBlendMode::NORMAL);
        }

        return;
    }

    const float p = lt / active;
    const float ci = std::floor(now / cycle);

    const float r1 = hash21(ci * 91.7f + 1.1f, 3.3f);
    const float r2 = hash21(ci * 17.3f + 9.9f, 55.5f);
    const float r3 = hash21(ci * 43.1f + 7.7f, 21.3f);
    const float r4 = hash21(ci * 55.5f + 2.2f, 77.7f);

    const float fr = std::floor(now * 30.0f);

    const int M = 12;
    float lx[16], ly[16];

    const float xTop = imgW * (0.35f + 0.30f * r1);
    const float xBot = std::clamp(xTop + (r2 - 0.5f) * imgW * 0.40f, imgW * 0.15f, imgW * 0.85f);

    for (int i = 0; i < M; ++i)
    {
        const float t = i / (float)(M - 1);
        const float base = xTop + (xBot - xTop) * t;
        const float jag = (hash21(i * 7.31f + ci * 13.7f, 3.3f) - 0.5f) * imgW * 0.07f;
        const float zig = ((i & 1) ? 1.0f : -1.0f) * imgW * 0.012f;

        lx[i] = std::clamp(base + jag + zig, 2.0f, imgW - 2.0f);
        ly[i] = imgH * t;
    }

    auto crackXAt = [&](float y)
    {
        const float t = std::clamp(y / imgH, 0.0f, 1.0f) * (M - 1);

        int i = (int)t;

        if (i >= M - 1)
            i = M - 2;

        const float f = t - i;

        return lx[i] + (lx[i + 1] - lx[i]) * f;
    };

    const float pStrike = 0.16f;
    const float ft = std::clamp((p - pStrike) / (1.0f - pStrike), 0.0f, 1.0f);
    const float fe = ft * ft;

    const float dyF  = imgH * 2.8f * fe;
    const float dxFL = -imgW * 0.35f * ft;
    const float dxFR = +imgW * 0.35f * ft;

    const float angL = -(0.25f + 0.20f * r2) * ft;
    const float angR = +(0.25f + 0.20f * r3) * ft;

    const float partA = 1.0f - std::clamp((ft - 0.85f) / 0.15f, 0.0f, 1.0f);

    float shA = 0.0f;

    if (p > 0.10f && p < 0.26f)
        shA = 1.0f - (p - 0.10f) / 0.16f;

    const float shX = (hash21(fr * 3.1f, ci) - 0.5f) * 6.0f * shA;
    const float shY = (hash21(fr * 4.7f, ci * 1.7f) - 0.5f) * 5.0f * shA;

    const float bx0 = imgX + shX;
    const float by0 = imgY + shY;

    auto xformHalf = [&](float x, float y, bool bLeft, float& ox, float& oy)
    {
        const float pvx = bLeft ? imgW * 0.25f : imgW * 0.75f;
        const float pvy = imgH * 0.40f;
        const float ang = bLeft ? angL : angR;

        const float c = std::cos(ang), s = std::sin(ang);
        const float rx = x - pvx, ry = y - pvy;

        ox = bx0 + pvx + rx * c - ry * s + (bLeft ? dxFL : dxFR);
        oy = by0 + pvy + rx * s + ry * c + dyF;
    };

    auto emitArb = [&](CTexturePtr tex,
                       const float* cx, const float* cy,
                       float u0, float v0, float u1, float v1,
                       float r, float g, float b, float a,
                       eSpriteBlendMode blend)
    {
        if (a <= 0.02f)
            return;

        float verts[8];
        verts[CSprite::VERT_ULX] = cx[0]; verts[CSprite::VERT_ULY] = cy[0];
        verts[CSprite::VERT_URX] = cx[1]; verts[CSprite::VERT_URY] = cy[1];
        verts[CSprite::VERT_BLX] = cx[2]; verts[CSprite::VERT_BLY] = cy[2];
        verts[CSprite::VERT_BRX] = cx[3]; verts[CSprite::VERT_BRY] = cy[3];

        float uvs[8];
        uvs[CSprite::VERT_ULX] = u0; uvs[CSprite::VERT_ULY] = v0;
        uvs[CSprite::VERT_URX] = u1; uvs[CSprite::VERT_URY] = v0;
        uvs[CSprite::VERT_BLX] = u0; uvs[CSprite::VERT_BLY] = v1;
        uvs[CSprite::VERT_BRX] = u1; uvs[CSprite::VERT_BRY] = v1;

        float col[4] = { r, g, b, a };

        CSprite::renderVerts(tex, verts, uvs, col, blend);
    };

    auto emitSeg = [&](float ax, float ay, float bxx, float byy, float w,
                       float r, float g, float b, float a)
    {
        const float dx = bxx - ax, dy = byy - ay;
        const float len = std::sqrt(dx * dx + dy * dy);

        if (len < 0.5f || a <= 0.02f)
            return;

        const float px = -dy / len * w * 0.5f;
        const float py =  dx / len * w * 0.5f;

        const float cx[4] = { ax + px, bxx + px, ax - px, bxx - px };
        const float cy[4] = { ay + py, byy + py, ay - py, byy - py };

        emitArb(ptrWhiteBox->getTexture(), cx, cy, wu0, wv0, wu1, wv1,
                r, g, b, a, eSpriteBlendMode::ADDITIVE);
    };

    if (ft <= 0.0f)
    {
        renderQuadDirect(node.spriteTex,
                         bx0, by0, bx0 + imgW, by0 + imgH,
                         uL, vT, uR, vB,
                         1.0f, 1.0f, 1.0f, 1.0f,
                         eSpriteBlendMode::NORMAL);
    }
    else if (partA > 0.02f)
    {
        const int NS = (int)std::clamp(imgH / 5.0f, 24.0f, 72.0f);

        for (int i = 0; i < NS; ++i)
        {
            const float y0 = imgH * i / NS;
            const float y1 = imgH * (i + 1) / NS;
            const float ym = (y0 + y1) * 0.5f;

            const float tx = crackXAt(ym);

            const float v0  = vT + (vB - vT) * (y0 / imgH);
            const float v1  = vT + (vB - vT) * (y1 / imgH);
            const float utx = uL + (uR - uL) * (tx / imgW);

            {
                float Lx[4], Ly[4];

                xformHalf(0.0f, y0, true, Lx[0], Ly[0]);
                xformHalf(tx,   y0, true, Lx[1], Ly[1]);
                xformHalf(0.0f, y1, true, Lx[2], Ly[2]);
                xformHalf(tx,   y1, true, Lx[3], Ly[3]);

                emitArb(node.spriteTex, Lx, Ly, uL, v0, utx, v1,
                        1.0f, 1.0f, 1.0f, partA, eSpriteBlendMode::NORMAL);
            }

            {
                float Rx[4], Ry[4];

                xformHalf(tx,     y0, false, Rx[0], Ry[0]);
                xformHalf(imgW,   y0, false, Rx[1], Ry[1]);
                xformHalf(tx,     y1, false, Rx[2], Ry[2]);
                xformHalf(imgW,   y1, false, Rx[3], Ry[3]);

                emitArb(node.spriteTex, Rx, Ry, utx, v0, uR, v1,
                        1.0f, 1.0f, 1.0f, partA, eSpriteBlendMode::NORMAL);
            }

            const float glowA = std::clamp(1.0f - ft * 1.5f, 0.0f, 1.0f) * partA;
            const float charA = std::clamp(1.0f - ft * 0.8f, 0.0f, 1.0f) * 0.7f * partA;
            const float cw = std::max(1.5f, imgW * 0.008f);

            if (charA > 0.02f)
            {
                float Cx[4], Cy[4];

                xformHalf(tx - cw, y0, true, Cx[0], Cy[0]);
                xformHalf(tx,      y0, true, Cx[1], Cy[1]);
                xformHalf(tx - cw, y1, true, Cx[2], Cy[2]);
                xformHalf(tx,      y1, true, Cx[3], Cy[3]);

                emitArb(ptrWhiteBox->getTexture(), Cx, Cy, wu0, wv0, wu1, wv1,
                        0.05f, 0.05f, 0.06f, charA, eSpriteBlendMode::NORMAL);

                float Dx[4], Dy[4];

                xformHalf(tx,      y0, false, Dx[0], Dy[0]);
                xformHalf(tx + cw, y0, false, Dx[1], Dy[1]);
                xformHalf(tx,      y1, false, Dx[2], Dy[2]);
                xformHalf(tx + cw, y1, false, Dx[3], Dy[3]);

                emitArb(ptrWhiteBox->getTexture(), Dx, Dy, wu0, wv0, wu1, wv1,
                        0.05f, 0.05f, 0.06f, charA, eSpriteBlendMode::NORMAL);
            }

            if (glowA > 0.02f)
            {
                float Gx[4], Gy[4];

                xformHalf(tx - cw, y0, true, Gx[0], Gy[0]);
                xformHalf(tx,      y0, true, Gx[1], Gy[1]);
                xformHalf(tx - cw, y1, true, Gx[2], Gy[2]);
                xformHalf(tx,      y1, true, Gx[3], Gy[3]);

                emitArb(ptrWhiteBox->getTexture(), Gx, Gy, wu0, wv0, wu1, wv1,
                        0.60f * glowA, 0.80f * glowA, 1.0f * glowA, 1.0f,
                        eSpriteBlendMode::ADDITIVE);

                float Hx[4], Hy[4];

                xformHalf(tx,      y0, false, Hx[0], Hy[0]);
                xformHalf(tx + cw, y0, false, Hx[1], Hy[1]);
                xformHalf(tx,      y1, false, Hx[2], Hy[2]);
                xformHalf(tx + cw, y1, false, Hx[3], Hy[3]);

                emitArb(ptrWhiteBox->getTexture(), Hx, Hy, wu0, wv0, wu1, wv1,
                        0.60f * glowA, 0.80f * glowA, 1.0f * glowA, 1.0f,
                        eSpriteBlendMode::ADDITIVE);
            }
        }
    }

    float flashA = 0.0f;

    if (p < 0.10f)
    {
        const float u = p / 0.10f;
        const float pl = std::sin(u * 3.14159f * 3.0f);

        flashA = (pl > 0.6f ? 0.16f : 0.0f) * (0.5f + 0.5f * hash21(fr, ci));
    }
    else
    {
        const float v = (p - 0.10f) / 0.14f;

        if (v < 1.0f)
            flashA = 0.55f * (1.0f - v) * (0.7f + 0.3f * hash21(fr * 1.7f, ci));
    }

    if (flashA > 0.01f)
    {
        renderQuadDirect(node.spriteTex,
                         bx0, by0, bx0 + imgW, by0 + imgH,
                         uL, vT, uR, vB,
                         0.75f * flashA, 0.85f * flashA, 1.0f * flashA, 1.0f,
                         eSpriteBlendMode::ADDITIVE);
    }

    float boltA = 0.0f;

    if (p > 0.08f && p < 0.42f)
    {
        const float up = std::clamp((p - 0.08f) / 0.02f, 0.0f, 1.0f);
        const float dn = std::clamp((0.42f - p) / 0.26f, 0.0f, 1.0f);

        boltA = up * dn * (0.75f + 0.25f * hash21(fr * 2.3f, ci));
    }

    if (boltA > 0.01f)
    {
        for (int i = 0; i + 1 < M; ++i)
        {
            emitSeg(bx0 + lx[i], by0 + ly[i], bx0 + lx[i + 1], by0 + ly[i + 1],
                    imgW * 0.030f, 0.20f * boltA, 0.40f * boltA, 1.0f * boltA, 1.0f);

            emitSeg(bx0 + lx[i], by0 + ly[i], bx0 + lx[i + 1], by0 + ly[i + 1],
                    imgW * 0.008f, 0.70f * boltA, 0.85f * boltA, 1.0f * boltA, 1.0f);

            emitSeg(bx0 + lx[i], by0 + ly[i], bx0 + lx[i + 1], by0 + ly[i + 1],
                    imgW * 0.003f, 1.0f * boltA, 1.0f * boltA, 1.0f * boltA, 1.0f);
        }

        for (int k = 0; k < 3; ++k)
        {
            const int s = 2 + k * 3 + (int)(r3 * 2.0f);

            if (s >= M)
                continue;

            float axp = bx0 + lx[s], ayp = by0 + ly[s];

            const float dir = ((k & 1) ? 1.0f : -1.0f);
            const float ang = dir * (0.7f + 0.6f * hash21(k * 17.7f + ci, 5.5f));

            for (int q = 0; q < 3; ++q)
            {
                const float step = imgH * (0.05f + 0.03f * hash21(k * 3.3f + q * 7.7f, ci));

                const float bxp = axp + std::sin(ang) * step + (hash21(q * 9.1f + k, ci) - 0.5f) * imgW * 0.02f;
                const float byp = ayp + std::cos(ang) * step * 0.7f;

                emitSeg(axp, ayp, bxp, byp, imgW * 0.006f,
                        0.5f * boltA, 0.7f * boltA, 1.0f * boltA, 1.0f);

                emitSeg(axp, ayp, bxp, byp, imgW * 0.002f,
                        1.0f * boltA, 1.0f * boltA, 1.0f * boltA, 1.0f);

                axp = bxp;
                ayp = byp;
            }
        }
    }

    if (p > 0.10f && p < 0.55f)
    {
        const float win = std::clamp((0.55f - p) / 0.20f, 0.0f, 1.0f);

        for (int k = 0; k < 6; ++k)
        {
            const float ph = hash21(k * 57.1f + ci * 3.3f, 91.7f);

            float eph = now * (2.0f + 2.0f * ph) + ph * 100.0f;
            const float rise = eph - std::floor(eph);

            const float sy = ph * imgH;
            const float sx = crackXAt(sy);

            const float dir = ((k & 1) ? 1.0f : -1.0f);

            const float px = bx0 + sx + dir * rise * imgW * (0.05f + 0.10f * ph)
                           + std::sin(now * 5.0f + ph * 6.28f) * 2.0f;

            const float py = by0 + sy + rise * rise * imgH * 0.10f;

            const float ea = win * (1.0f - rise) * 0.8f;
            const float es = 1.0f + 2.0f * ph;

            const float dxx[4] = { px - es * 0.5f, px + es * 0.5f,
                                   px - es * 0.5f, px + es * 0.5f };

            const float dyy[4] = { py - es * 0.5f, py - es * 0.5f,
                                   py + es * 0.5f, py + es * 0.5f };

            emitArb(ptrWhiteBox->getTexture(), dxx, dyy,
                    wu0, wv0, wu1, wv1,
                    1.0f * ea, 0.9f * ea, 0.6f * ea, 1.0f,
                    eSpriteBlendMode::ADDITIVE);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// idle "tear"
///////////////////////////////////////////////////////////////////////////////

inline void renderImgTear(const HTMLNode& node, float imgX, float imgY, float imgW, float imgH, float now)
{
    if (!node.spriteTex || imgW <= 0 || imgH <= 0)
        return;

    auto ptrWhiteBox = Engine::getCfg().wb;
    if (!ptrWhiteBox)
        return;

    const float wu0 = ptrWhiteBox->_uvs[CSprite::VERT_ULX];
    const float wv0 = ptrWhiteBox->_uvs[CSprite::VERT_ULY];
    const float wu1 = ptrWhiteBox->_uvs[CSprite::VERT_URX];
    const float wv1 = ptrWhiteBox->_uvs[CSprite::VERT_BLY];

    const float uL = node.spriteUV[0], vT = node.spriteUV[1];
    const float uR = node.spriteUV[2], vB = node.spriteUV[3];

    const float active  = std::max(0.1f, node.idleDur > 0.0f ? node.idleDur : 4.0f);
    const float rebuild = 1.0f;
    const float pause   = node.idleDelay > 0.0f ? node.idleDelay : 1.2f;
    const float cycle   = active + rebuild + pause;

    float lt = std::fmod(now, cycle);
    if (lt < 0.0f)
        lt += cycle;

    if (lt >= active + rebuild)
    {
        renderQuadDirect(node.spriteTex,
                         imgX, imgY, imgX + imgW, imgY + imgH,
                         uL, vT, uR, vB,
                         1.0f, 1.0f, 1.0f, 1.0f,
                         eSpriteBlendMode::NORMAL);
        return;
    }

    if (lt >= active)
    {
        const float r = (lt - active) / rebuild;
        const float a = r * r * (3.0f - 2.0f * r);

        if (a > 0.001f)
        {
            renderQuadDirect(node.spriteTex,
                             imgX, imgY, imgX + imgW, imgY + imgH,
                             uL, vT, uR, vB,
                             1.0f, 1.0f, 1.0f, a,
                             eSpriteBlendMode::NORMAL);
        }

        return;
    }

    const float p = lt / active;
    const float ci = std::floor(now / cycle);

    const float r1 = hash21(ci * 91.7f + 1.1f, 3.3f);
    const float r2 = hash21(ci * 17.3f + 9.9f, 55.5f);
    const float r3 = hash21(ci * 43.1f + 7.7f, 21.3f);

    const float tearX  = imgW * (0.5f + (r1 - 0.5f) * 0.24f);
    const float jagAmp = std::max(2.0f, imgW * (0.015f + 0.02f * r2));

    float tpIn = std::clamp(p / 0.45f, 0.0f, 1.0f);
    const float tp = tpIn * tpIn * (3.0f - 2.0f * tpIn);

    const float ft = std::clamp((p - 0.55f) / 0.45f, 0.0f, 1.0f);
    const float fe = ft * ft;

    const float tipY = tp * imgH;

    const float gapMax = imgW * 0.30f;
    const float liftL  = imgH * 0.06f * (r2 * 2.0f - 1.0f);
    const float liftR  = imgH * 0.06f * (r3 * 2.0f - 1.0f);

    const float dxFL = -imgW * 0.35f * ft;
    const float dxFR = +imgW * 0.35f * ft;
    const float dyF  = imgH * 2.8f * fe;

    const float angL = -(0.25f + 0.20f * r2) * ft;
    const float angR = +(0.25f + 0.20f * r3) * ft;

    const float partA = 1.0f - std::clamp((ft - 0.85f) / 0.15f, 0.0f, 1.0f);

    auto off = [&](float y, float side, float lift, float& dx, float& dy)
    {
        const float wv  = std::max(0.0f, tipY - y) / imgH;
        const float gap = gapMax * tp * wv * (1.0f + 0.6f * wv);

        dx = side * gap * 0.5f;
        dy = -lift * tp * wv * wv;
    };

    auto xformHalf = [&](float x, float y, bool bLeft, float& ox, float& oy)
    {
        float dxT, dyT;

        off(y, bLeft ? -1.0f : 1.0f, bLeft ? liftL : liftR, dxT, dyT);

        float xx = x + dxT;
        float yy = y + dyT;

        if (ft > 0.0f)
        {
            const float pvx = bLeft ? imgW * 0.25f : imgW * 0.75f;
            const float pvy = imgH * 0.40f;
            const float ang = bLeft ? angL : angR;

            const float c = std::cos(ang), s = std::sin(ang);
            const float rx = xx - pvx, ry = yy - pvy;

            xx = pvx + rx * c - ry * s + (bLeft ? dxFL : dxFR);
            yy = pvy + rx * s + ry * c + dyF;
        }

        ox = imgX + xx;
        oy = imgY + yy;
    };

    auto emitArb = [&](CTexturePtr tex,
                       const float* cx, const float* cy,
                       float u0, float v0, float u1, float v1,
                       float r, float g, float b, float a,
                       eSpriteBlendMode blend)
    {
        if (a <= 0.02f)
            return;

        float verts[8];
        verts[CSprite::VERT_ULX] = cx[0]; verts[CSprite::VERT_ULY] = cy[0];
        verts[CSprite::VERT_URX] = cx[1]; verts[CSprite::VERT_URY] = cy[1];
        verts[CSprite::VERT_BLX] = cx[2]; verts[CSprite::VERT_BLY] = cy[2];
        verts[CSprite::VERT_BRX] = cx[3]; verts[CSprite::VERT_BRY] = cy[3];

        float uvs[8];
        uvs[CSprite::VERT_ULX] = u0; uvs[CSprite::VERT_ULY] = v0;
        uvs[CSprite::VERT_URX] = u1; uvs[CSprite::VERT_URY] = v0;
        uvs[CSprite::VERT_BLX] = u0; uvs[CSprite::VERT_BLY] = v1;
        uvs[CSprite::VERT_BRX] = u1; uvs[CSprite::VERT_BRY] = v1;

        float col[4] = { r, g, b, a };

        CSprite::renderVerts(tex, verts, uvs, col, blend);
    };

    const float fr = std::floor(now * 30.0f);

    if (p < 0.10f)
    {
        const float ca = std::clamp(p / 0.02f, 0.0f, 1.0f)
                       * std::clamp((0.10f - p) / 0.03f, 0.0f, 1.0f);

        const float crackLen = imgH * 0.25f * (p / 0.10f);
        const float cw = std::max(1.0f, imgW * 0.004f);

        for (int i = 0; i < 8; ++i)
        {
            const float y0 = crackLen * i / 8.0f;
            const float y1 = crackLen * (i + 1) / 8.0f;
            const float n  = (hash21(i * 7.31f + ci * 13.7f, 3.7f) - 0.5f) * jagAmp;
            const float flick = 0.6f + 0.4f * hash21(i * 3.3f + fr, ci * 7.7f);

            const float ccx[4] = { imgX + tearX + n - cw * 0.5f, imgX + tearX + n + cw * 0.5f,
                                   imgX + tearX + n - cw * 0.5f, imgX + tearX + n + cw * 0.5f };

            const float ccy[4] = { imgY + y0, imgY + y0, imgY + y1, imgY + y1 };

            emitArb(ptrWhiteBox->getTexture(), ccx, ccy,
                    wu0, wv0, wu1, wv1,
                    1.0f * ca * flick, 0.95f * ca * flick, 0.85f * ca * flick, 1.0f,
                    eSpriteBlendMode::ADDITIVE);
        }
    }

    const int NS = (int)std::clamp(imgH / 5.0f, 24.0f, 72.0f);
    const float fibA = std::min(1.0f, tp * 3.0f) * 0.7f * partA;
    const float fw   = std::max(1.5f, imgW * 0.006f);

    for (int i = 0; i < NS; ++i)
    {
        const float y0 = imgH * i / NS;
        const float y1 = imgH * (i + 1) / NS;

        const float n1 = hash21(i * 7.31f + ci * 13.7f, 3.7f) - 0.5f;
        const float n2 = std::sin(i * 0.9f + r1 * 6.28f) * 0.5f;

        const float tx = std::clamp(tearX + (n1 * 1.6f + n2 * 0.8f) * jagAmp, 4.0f, imgW - 4.0f);

        const float v0  = vT + (vB - vT) * (y0 / imgH);
        const float v1  = vT + (vB - vT) * (y1 / imgH);
        const float utx = uL + (uR - uL) * (tx / imgW);

        if (partA <= 0.02f)
            continue;

        const float ya = y0, yb = y1;
        const float va = v0, vb = v1;

        {
            float Lx[4], Ly[4];

            xformHalf(0.0f, ya, true, Lx[0], Ly[0]);
            xformHalf(tx,   ya, true, Lx[1], Ly[1]);
            xformHalf(0.0f, yb, true, Lx[2], Ly[2]);
            xformHalf(tx,   yb, true, Lx[3], Ly[3]);

            emitArb(node.spriteTex, Lx, Ly, uL, va, utx, vb,
                    1.0f, 1.0f, 1.0f, partA, eSpriteBlendMode::NORMAL);

            if (fibA > 0.02f)
            {
                float Fx[4], Fy[4];

                xformHalf(tx - fw, ya, true, Fx[0], Fy[0]);
                xformHalf(tx,      ya, true, Fx[1], Fy[1]);
                xformHalf(tx - fw, yb, true, Fx[2], Fy[2]);
                xformHalf(tx,      yb, true, Fx[3], Fy[3]);

                emitArb(ptrWhiteBox->getTexture(), Fx, Fy, wu0, wv0, wu1, wv1,
                        0.85f, 0.83f, 0.78f, fibA, eSpriteBlendMode::NORMAL);
            }
        }

        {
            float Rx[4], Ry[4];

            xformHalf(tx,     ya, false, Rx[0], Ry[0]);
            xformHalf(imgW,   ya, false, Rx[1], Ry[1]);
            xformHalf(tx,     yb, false, Rx[2], Ry[2]);
            xformHalf(imgW,   yb, false, Rx[3], Ry[3]);

            emitArb(node.spriteTex, Rx, Ry, utx, va, uR, vb,
                    1.0f, 1.0f, 1.0f, partA, eSpriteBlendMode::NORMAL);

            if (fibA > 0.02f)
            {
                float Fx[4], Fy[4];

                xformHalf(tx,      ya, false, Fx[0], Fy[0]);
                xformHalf(tx + fw, ya, false, Fx[1], Fy[1]);
                xformHalf(tx,      yb, false, Fx[2], Fy[2]);
                xformHalf(tx + fw, yb, false, Fx[3], Fy[3]);

                emitArb(ptrWhiteBox->getTexture(), Fx, Fy, wu0, wv0, wu1, wv1,
                        0.10f, 0.10f, 0.10f, fibA * 0.8f, eSpriteBlendMode::NORMAL);
            }
        }
    }

    if (tp > 0.02f && tp < 0.98f && ft <= 0.0f)
    {
        for (int k = 0; k < 4; ++k)
        {
            const float ph = hash21(k * 57.1f + ci * 3.3f, 91.7f);

            float eph = now * (1.5f + ph) + ph * 100.0f;
            const float rise = eph - std::floor(eph);

            const float px = imgX + tearX + (ph - 0.5f) * imgW * 0.10f
                           + std::sin(now * 4.0f + ph * 6.28f) * 3.0f;

            const float py = imgY + tipY - rise * imgH * 0.08f;

            const float ea = (1.0f - rise) * 0.35f;
            const float es = 0.8f + 1.5f * ph;

            const float dxx[4] = { px - es * 0.5f, px + es * 0.5f,
                                   px - es * 0.5f, px + es * 0.5f };

            const float dyy[4] = { py - es * 0.5f, py - es * 0.5f,
                                   py + es * 0.5f, py + es * 0.5f };

            emitArb(ptrWhiteBox->getTexture(), dxx, dyy,
                    wu0, wv0, wu1, wv1,
                    0.55f * ea, 0.53f * ea, 0.50f * ea, 1.0f,
                    eSpriteBlendMode::ADDITIVE);
        }
    }
}
_G2D_NAMESPACE_END_