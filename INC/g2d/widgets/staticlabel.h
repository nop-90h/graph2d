#pragma once

#include "g2d.h"
#include "sprite.h"
#include "widgets/widget.h"
#include "textrender.h"
#include "engine.h"
#include "htmldom.h"
#include "htmldoc.h"

_G2D_NAMESPACE_BEGIN_

enum class eTextRenderType
{
    NORMAL,
    BOX,
    HTML_BOX,
};

using FOnSliderChanged   = std::function<void(float fValue)>;
using FOnSliderChangedEx = std::function<void(unsigned long long nodeId, float fValue)>;
using FOnSelectChangedCb = std::function<void(unsigned long long selectId, int nIndex, const char* szValue)>;
using FOnHoverElement    = std::function<void(unsigned long long rootId, unsigned long long nodeId)>;

class StaticLabel : public Widget
{
private:
    // HTML-документ обёртки; владеет root'ом, когда создан из html.
    CHTMLDocument           _htmlDoc;

    std::string             _strLabel;
    std::string             _strRawLabel;

    int                     _eAlign             = Engine::getCfg().DEFAULT_FONT_ALIGN;
    float                   _fBoxCx             = 0.f;
    float                   _fHTMLCalcedBoxCx   = 0.f;
    eTextRenderType         _eRenderType        = eTextRenderType::NORMAL;

    bool                    _bShadow            = Engine::getCfg().DEFAULT_SHADOW;
    bool                    _bTextCreated       = false;

    float                   _xOffs              = 0.f;
    float                   _yOffs              = 0.f;
    float                   _fFontHeightMul     = Engine::getCfg().DEFAULT_FONT_HEIGHT_MUL;
    float                   _fFontSize          = Engine::getCfg().DEFAULT_FONT_SIZE;

    CContainerPtr           _ptrTextCont;
    Rect                    _rcBounds;

    float                   _fTime              = 0.f;

    std::string             _sFontName          = Engine::getCfg().DEFAULT_FONT_NAME;
    float                   _fLineSpacingMul    = 1.f;
    float                   _fBloodFallHeight   = 220.f;
    float                   _fBaseTextCy        = 0.f;

    FOnSliderChanged        _onSliderChangedCb;
    FOnSliderChangedEx      _onSliderChangedExCb;
    FOnSelectChangedCb      _onSelectChangedCb;
    FOnHoverElement         _onHoverElement;
    FOnHoverElement         _onLeaveElement;

    // Если HTML-документ изменил размер, нужно обновить скролл родительского диалога.
    bool                    _bHtmlLayoutDirty   = false;

    bool                    notifyParentScrollChanged();

protected:
    void            setFontInfo             (void);
    unsigned int    getShadowColor          (float fAlphaMul);
    void            setMouseTrack           (bool bTrack);

    // мосты событий документа (подписываются в updateText)
    void            subscribeDocEvents      (void);
    void            handleCursorChanged     (const char* szNewCursor);
    void            handleDropdownRequested (unsigned long long selectId);
    void            handleDrag              (bool bEnd);
    float           handleDocumentScale     (void);
    void            handleSliderChanged     (unsigned long long nodeId, float value);
    void            handleSelectChanged     (unsigned long long selectId, int index, const char* value);

public:
                    StaticLabel             (void);
                    ~StaticLabel            (void);

    void            updateText              (void);

    virtual void    update                  (float dt) override;

    virtual void    renderSelf              (float dt, float* rgba) override;

    static void     measure                 (Rect* rcOut,
                                             LPCTSTR lpszText,
                                             LPCTSTR lpszFont       = Engine::getCfg().DEFAULT_FONT_NAME.c_str(),
                                             float fFontSize        = Engine::getCfg().DEFAULT_FONT_SIZE,
                                             float fFontHeightMul   = Engine::getCfg().DEFAULT_FONT_HEIGHT_MUL,
                                             int nAlgin             = Engine::getCfg().DEFAULT_FONT_ALIGN,
                                             bool bShadow           = Engine::getCfg().DEFAULT_SHADOW);

    static void     measureHtml             (LPCTSTR lpszText,
                                             Rect* rcOut);

    void            setShadow               (bool bShadow);

    void            setTextOffs             (float x, float y);
    void            setText                 (const char* lpccLabelText);
    void            setTextFmt              (const char* lpccLabelText, ...);
    void            setTextFmtV             (const char* lpccLabelText, va_list argptr);

    virtual bool    getNotTransBounds       (Rect* p);

    float           calcCx                  (void);
    float           calcCy                  (void);
    float           getLineHeight           (void);
    float           getTextWidth            (LPCTSTR lpszText = nullptr);

    void            setFontSize             (float fFontSize)
    {
        _fFontSize = fFontSize;
        updateText();
    }

    void            setFont                 (LPCTSTR lpszFont)
    {
        _sFontName = lpszFont;
    }

    void            setLineSpacingMul       (float fMul)
    {
        _fLineSpacingMul = fMul;
        updateText();
    }

    float           getLineSpacingMul       () const
    {
        return _fLineSpacingMul;
    }

    inline void     setAlign                (int eAlign)
    {
        _eAlign = eAlign;
        updateText();
    }

    inline void     setFontHeightMul        (float fMul)
    {
        _fFontHeightMul = fMul;
        updateText();
    }

    inline void     setBoxMode              (eTextRenderType eType = eTextRenderType::NORMAL,
                                             float fBoxCx = 0.f)
    {
        _eRenderType = eType;
        _fBoxCx = fBoxCx;
    }

    // доступ к обёртке документа (бинды по id, контролы и т.д.)
    CHTMLDocument&  getHTMLDocument         ()
    {
        return _htmlDoc;
    }

    auto            getHTMLRootId           ()
    {
        return _htmlDoc.getRootId();
    }

    void            setOnSliderChanged      (FOnSliderChanged cb)
    {
        _onSliderChangedCb = cb;
    }

    void            setOnSliderChangedEx    (FOnSliderChangedEx cb)
    {
        _onSliderChangedExCb = cb;
    }

    void            setOnSelectChanged      (FOnSelectChangedCb cb)
    {
        _onSelectChangedCb = cb;
    }

    void            setOnHoverElement       (FOnHoverElement cb)
    {
        _onHoverElement = cb;
    }

    void            setOnLeaveElement       (FOnHoverElement cb)
    {
        _onLeaveElement = cb;
    }
};

typedef std::shared_ptr<StaticLabel> StaticLabelPtr;

_G2D_NAMESPACE_END_