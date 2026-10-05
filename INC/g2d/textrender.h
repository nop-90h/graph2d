#pragma once
#include "g2d.h"
#include "sprite.h"
#include "sconts.h"
#include "engine.h"
#include "fontstash.h"

struct FONScontext;

_G2D_NAMESPACE_BEGIN_

class CSpine;

struct TextBoxParams {
    float x = 0;
    float y = 0;
    float maxWidth = 0;
    int align = 0;
    unsigned int color = 0;
    float appearStart = -1.0f;
    unsigned long long animId = 0;
};

using FontsMap = FixedSmallMap<std::string, int, MAX_FONTS>;

class TextRender
{
private:
    FONScontext* _fonsCtx;
    FontsMap _handles;
    FontsMap _boldHandles;
    FontsMap _italicHandles;

    float _fLineHeightMul  = 1.f;
    float _fLineSpacingMul = 1.f;
    float _fAnimTime       = 0.0f;
    bool  _isPaused        = false;

    static void handleFontStashError(void* uptr, int error, int val);
public:
    inline static TextRender* getInstance(void) { static TextRender res; return &res; }
    FONScontext* getFonsContext() { return _fonsCtx; }
    void init(void);

    int  addFont(LPCTSTR lpszFontName, LPCTSTR lpszFontPath);
    int  addBoldFont(LPCTSTR lpszBaseFontName, LPCTSTR lpszFontPath);
    int  addItalicFont(LPCTSTR lpszBaseFontName, LPCTSTR lpszFontPath);
    void mapBoldFontHandle(LPCTSTR lpszBaseFontName, int fontHandle);
    void mapItalicFontHandle(LPCTSTR lpszBaseFontName, int fontHandle);
    int  findBoldFontHandle(LPCTSTR lpszBaseFontName);
    int  findItalicFontHandle(LPCTSTR lpszBaseFontName);
    int  getFontHandle(LPCTSTR lpszFontName);

    void setFont(LPCTSTR lpszFontName);
    void setFont(int fontHandle);
    void setFontSize(int nSize);
    void setFontColor(unsigned int color);
    void setFontColor(float r, float g, float b, float a);

    void renderText(float x, float y, LPCTSTR lpszText);
    void measureText(float x, float y, LPCTSTR lpszText, Rect* pRcOut);
    void renderTextBox(const char* text, const TextBoxParams& params, Rect* rcOut, bool bMeasureOnly = false);
    void measureTextBox(const char* text, const TextBoxParams& params, Rect* rcOut);
    void getTextBounds(float x, float y, LPCTSTR lpszText, Rect* pRcOut);
    void setTextAlign(int eAlign);

    float getLineHeight(void);
    float getTextWidth(LPCTSTR lpszText);

    static unsigned int floatColor2Uint(float* rgba);

    void  setLineHeightMul(float fMul) { _fLineHeightMul = fMul; }
    void  setLineSpacingMul(float fMul) { _fLineSpacingMul = fMul; }
    float getLineSpacingMul(void) const { return _fLineSpacingMul; }

    void update(float dt) { if (!_isPaused) _fAnimTime += dt; }
};

_G2D_NAMESPACE_END_