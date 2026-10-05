#include "sprite.h"
#include "gfx.h"
#include "spinecont.h"

#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"
#undef FONTSTASH_IMPLEMENTATION

#define GLFONTSTASH_IMPLEMENTATION
#include "glfontstash.h"
#undef GLFONTSTASH_IMPLEMENTATION

#include "textrender.h"
#include "textrender_helpers.h"
#include "spriteloader.h"


_G2D_NAMESPACE_BEGIN_

void TextRender::handleFontStashError(void* uptr, int error, int val)
{
    assert(false);
}

void TextRender::init()
{
    auto& cfg = Engine::getCfg();

    _fonsCtx = glfonsCreate(cfg.FONT_TEXTURE_DIM, cfg.FONT_TEXTURE_DIM, FONS_ZERO_TOPLEFT);
    fonsSetErrorCallback(_fonsCtx, TextRender::handleFontStashError, _fonsCtx);

    for (int i = 0; i < (int)cfg.FONTS.size(); i++)
        if (cfg.FONTS[i])
        {
            int nHandle = fonsAddFont(_fonsCtx, "default", cfg.FONTS[i]->second.c_str());
            _handles.insert(cfg.FONTS[i]->first, nHandle);
        }
}

int TextRender::addFont(LPCTSTR lpszFontName, LPCTSTR lpszFontPath)
{
    return fonsAddFont(_fonsCtx, lpszFontName, lpszFontPath);
}

int TextRender::addBoldFont(LPCTSTR lpszBaseFontName, LPCTSTR lpszFontPath)
{
    std::string internalName = std::string(lpszBaseFontName) + "__bold";

    int h = fonsAddFont(_fonsCtx, internalName.c_str(), lpszFontPath);

    if (h != -1)
        _boldHandles.insert(lpszBaseFontName, h);

    return h;
}

int TextRender::addItalicFont(LPCTSTR lpszBaseFontName, LPCTSTR lpszFontPath)
{
    std::string internalName = std::string(lpszBaseFontName) + "__italic";

    int h = fonsAddFont(_fonsCtx, internalName.c_str(), lpszFontPath);

    if (h != -1)
        _italicHandles.insert(lpszBaseFontName, h);

    return h;
}

void TextRender::mapBoldFontHandle(LPCTSTR lpszBaseFontName, int fontHandle)
{
    if (fontHandle != -1)
        _boldHandles.insert(lpszBaseFontName, fontHandle);
}

void TextRender::mapItalicFontHandle(LPCTSTR lpszBaseFontName, int fontHandle)
{
    if (fontHandle != -1)
        _italicHandles.insert(lpszBaseFontName, fontHandle);
}

int TextRender::findBoldFontHandle(LPCTSTR lpszBaseFontName)
{
    auto ptr = _boldHandles.find(lpszBaseFontName);
    return ptr ? *ptr : -1;
}

int TextRender::findItalicFontHandle(LPCTSTR lpszBaseFontName)
{
    auto ptr = _italicHandles.find(lpszBaseFontName);
    return ptr ? *ptr : -1;
}

int TextRender::getFontHandle(LPCTSTR lpszFontName)
{
    int nHandle = -1;

    auto ptrHandle = _handles.find(lpszFontName);
    assert(ptrHandle);

    if (ptrHandle)
        nHandle = *ptrHandle;

    return nHandle;
}

void TextRender::setFont(LPCTSTR lpszFontName)
{
    fonsSetFont(_fonsCtx, getFontHandle(lpszFontName));
}

void TextRender::setFont(int fontHandle)
{
    fonsSetFont(_fonsCtx, fontHandle);
}

void TextRender::setFontSize(int nSize)
{
    fonsSetSize(_fonsCtx, nSize);
}

void TextRender::setFontColor(unsigned int color)
{
    fonsSetColor(_fonsCtx, color);
}

void TextRender::setFontColor(float r, float g, float b, float a)
{
    fonsSetColor(_fonsCtx, packRGBA(
        (unsigned int)(std::clamp(r, 0.f, 1.f) * 255.f),
        (unsigned int)(std::clamp(g, 0.f, 1.f) * 255.f),
        (unsigned int)(std::clamp(b, 0.f, 1.f) * 255.f),
        (unsigned int)(std::clamp(a, 0.f, 1.f) * 255.f)));
}

void TextRender::renderText(float x, float y, LPCTSTR lpszText)
{
    fonsDrawText(_fonsCtx, x, y, lpszText, nullptr);
}

void TextRender::measureText(float x, float y, LPCTSTR lpszText, Rect* pRcOut)
{
    getTextBounds(x, y, lpszText, pRcOut);
}

void TextRender::getTextBounds(float x, float y, LPCTSTR lpszText, Rect* pRcOut)
{
    static float bounds[4] = { 0 };

    fonsTextBounds(_fonsCtx, x, y, lpszText, nullptr, bounds);

    pRcOut->set(bounds[0], bounds[1], bounds[2], bounds[3]);
}

void TextRender::setTextAlign(int eAlign)
{
    static_assert(FONS_ALIGN_LEFT == (int)eTRAlign::TR_ALIGN_LEFT);
    static_assert(FONS_ALIGN_CENTER == (int)eTRAlign::TR_ALIGN_CENTER);
    static_assert(FONS_ALIGN_RIGHT == (int)eTRAlign::TR_ALIGN_RIGHT);
    static_assert(FONS_ALIGN_TOP == (int)eTRAlign::TR_ALIGN_TOP);
    static_assert(FONS_ALIGN_MIDDLE == (int)eTRAlign::TR_ALIGN_MIDDLE);
    static_assert(FONS_ALIGN_BOTTOM == (int)eTRAlign::TR_ALIGN_BOTTOM);
    static_assert(FONS_ALIGN_BASELINE == (int)eTRAlign::TR_ALIGN_BASELINE);

    fonsSetAlign(_fonsCtx, eAlign);
}

void TextRender::renderTextBox(const char* text, const TextBoxParams& params, Rect* rcOut, bool bMeasureOnly)
{
    assert(text);
    assert(_fonsCtx);

    float lineH;

    auto fs = _fonsCtx;

    fonsVertMetrics(_fonsCtx, nullptr, nullptr, &lineH);

    lineH *= _fLineHeightMul * _fLineSpacingMul;

    rcOut->set(0, 0, 0, 0);

    eMesureLine eM = bMeasureOnly ? eMesureLine::MEASURE_INITIAL : eMesureLine::DRAW_INITIAL;

    fonsSetColor(_fonsCtx, params.color);
    fonsSetAlign(_fonsCtx, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);

    float lineHeight = lineH;
    float y = params.y;

    const char* start = text;
    const char* end = text + strlen(text);

    while (start < end)
    {
        const char* lineEnd = start;

        while (lineEnd < end && *lineEnd != '\n')
            lineEnd++;

        if (params.maxWidth > 0)
        {
            const char* lineStart = start;
            const char* wrapPos = start;
            const char* lastSpace = nullptr;

            if (wrapPos >= lineEnd)
                y += lineHeight;

            while (wrapPos < lineEnd)
            {
                float chunkWidth = fonsTextBounds(fs, 0, 0, lineStart, wrapPos + 1, nullptr);

                if (*wrapPos == ' ')
                    lastSpace = wrapPos;

                if (chunkWidth > params.maxWidth)
                {
                    if (lastSpace && lastSpace > lineStart)
                    {
                        renderAlignedLine(fs, lineStart, lastSpace, params.x, y, params.maxWidth, params.align, rcOut, eM);

                        y += lineHeight;

                        lineStart = lastSpace + 1;
                        wrapPos = lineStart;
                        lastSpace = nullptr;

                        continue;
                    }
                    else
                    {
                        renderAlignedLine(fs, lineStart, wrapPos, params.x, y, params.maxWidth, params.align, rcOut, eM);

                        y += lineHeight;

                        lineStart = wrapPos;
                        lastSpace = nullptr;
                    }
                }

                wrapPos++;
            }

            if (lineStart < lineEnd)
            {
                renderAlignedLine(fs, lineStart, lineEnd, params.x, y, params.maxWidth, params.align, rcOut, eM);
                y += lineHeight;
            }
        }
        else
        {
            renderAlignedLine(fs, start, lineEnd, params.x, y, 0, params.align, rcOut, eM);
            y += lineHeight;
        }

        start = lineEnd + 1;
    }
}

float TextRender::getLineHeight()
{
    float lineH;

    fonsVertMetrics(_fonsCtx, nullptr, nullptr, &lineH);

    return lineH * _fLineHeightMul * _fLineSpacingMul;
}

float TextRender::getTextWidth(LPCTSTR lpszText)
{
    return fonsTextBounds(_fonsCtx, 0, 0, lpszText, nullptr, nullptr);
}

unsigned int TextRender::floatColor2Uint(float* rgba)
{
    return packRGBA(
        (unsigned int)(std::clamp(rgba[0], 0.f, 1.f) * 255.f),
        (unsigned int)(std::clamp(rgba[1], 0.f, 1.f) * 255.f),
        (unsigned int)(std::clamp(rgba[2], 0.f, 1.f) * 255.f),
        (unsigned int)(std::clamp(rgba[3], 0.f, 1.f) * 255.f));
}

void TextRender::measureTextBox(const char* text, const TextBoxParams& params, Rect* rcOut)
{
    renderTextBox(text, params, rcOut, true);
}


_G2D_NAMESPACE_END_