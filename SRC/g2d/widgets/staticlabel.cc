#include "widgets/staticlabel.h"
#include "widgets/widgetdefs.h"
#include "gfx.h"
#include "spriteloader.h"
#include "sprite.h"
#include "htmldom.h"
#include "htmlselectpopup.h"
#include "dialogs/basedialog.h"

#include <cmath>

_G2D_NAMESPACE_BEGIN_

StaticLabel::StaticLabel()
{
    setRgba(widgetdefs::defaultTextColor);

    _sFontName = Engine::getCfg().DEFAULT_FONT_NAME;
}

StaticLabel::~StaticLabel()
{
    // удаляем root, пока объект полностью жив — removeNode может
    // дёрнуть onLeave/onCursorChanged через fallback-цепочку
    _htmlDoc.reset();
}

bool StaticLabel::notifyParentScrollChanged()
{
    auto ptrDlg = getParentDialog();
    if (!ptrDlg)
        return false;

    auto pDlg = std::dynamic_pointer_cast<BaseDialog>(ptrDlg);
    if (pDlg)
    {
        pDlg->updateMaxScroll();
        pDlg->updateScrollBox();
    }

    return true;
}

void StaticLabel::update(float dt)
{
    Widget::update(dt);

    if (_bHtmlLayoutDirty)
    {
        if (notifyParentScrollChanged())
            _bHtmlLayoutDirty = false;
    }
}

void StaticLabel::measureHtml(LPCTSTR lpszText, Rect* rcOut)
{
    CHTMLDocument doc(lpszText); // временный документ, владеет root'om
    *rcOut = HTMLDom::getInstance()->measure(doc.getRootId(), Engine::getCfg().INIT_SCR_CX);
}

void StaticLabel::measure(Rect* rcOut,
                          LPCTSTR lpszText,
                          LPCTSTR lpszFont,
                          float fFontSize,
                          float fFontHeightMul,
                          int nAlgin,
                          bool bShadow)
{
    TextRender::getInstance()->setFont(lpszFont);
    TextRender::getInstance()->setFontSize(fFontSize);
    TextRender::getInstance()->setTextAlign(nAlgin);
    TextRender::getInstance()->setLineHeightMul(fFontHeightMul);

    Rect rcShadow;

    if (bShadow)
    {
        TextBoxParams params;
        params.align = static_cast<int>(eTRAlign::TR_ALIGN_CENTER);
        params.x = 2;
        params.y = 2;

        TextRender::getInstance()->renderTextBox(lpszText, params, &rcShadow, true);
    }

    TextBoxParams params;
    params.align = static_cast<int>(eTRAlign::TR_ALIGN_CENTER);
    params.x = 0;
    params.y = 0;

    TextRender::getInstance()->renderTextBox(lpszText, params, rcOut, true);

    if (bShadow)
        rcOut->unite(&rcShadow);
}

static inline int getUtf8CharLenStatic(const char* s)
{
    unsigned char c = (unsigned char)*s;

    if (c < 0x80)
        return 1;

    if ((c & 0xe0) == 0xc0)
        return 2;

    if ((c & 0xf0) == 0xe0)
        return 3;

    if ((c & 0xf8) == 0xf0)
        return 4;

    return 1;
}

void StaticLabel::setMouseTrack(bool bTrack)
{
    if (bTrack)
    {
        setOnMove([this](bool bisPressed, int x, int y)
        {
            if (_htmlDoc)
            {
                auto pt = screenToLocal(x, y);

                // важно: рендер/инпут по-прежнему идут через HTMLDom с rootId
                HTMLDom::getInstance()->handleInput(_htmlDoc.getRootId(),
                                                    eHTMLInputEvent::MOUSE_MOVE,
                                                    pt.x,
                                                    pt.y);
            }
        });

        setOnPointerDown([this]
        {
            if (_htmlDoc)
            {
                Point pt = InputController::getInstance()->getMousePos();
                pt = screenToLocal(pt.x, pt.y);

                HTMLDom::getInstance()->handleInput(_htmlDoc.getRootId(),
                                                    eHTMLInputEvent::MOUSE_DOWN,
                                                    pt.x,
                                                    pt.y);
            }
        });

        setOnPointerUp([this]
        {
            if (_htmlDoc)
            {
                Point pt = InputController::getInstance()->getMousePos();
                pt = screenToLocal(pt.x, pt.y);

                HTMLDom::getInstance()->handleInput(_htmlDoc.getRootId(),
                                                    eHTMLInputEvent::MOUSE_UP,
                                                    pt.x,
                                                    pt.y);
            }
        });

        setOnLeave([this]
        {
            if (_htmlDoc)
            {
                HTMLDom::getInstance()->handleMouseLeave(_htmlDoc.getRootId());
            }
        });
    }
    else
    {
        setOnMove({});
        setOnPointerDown({});
        setOnPointerUp({});
    }
}

void StaticLabel::renderSelf(float dt, float* rgba)
{
    if (!_bTextCreated)
        return;

    if (isOffScene())
        return;

    setFontInfo();

    switch (_eRenderType)
    {
        case eTextRenderType::NORMAL:
        {
            if (_bShadow)
            {
                TextRender::getInstance()->setFontColor(getShadowColor(rgba[3]));
                TextRender::getInstance()->renderText(_xOffs + 2.f,
                                                      _yOffs + 2.f,
                                                      _strLabel.c_str());
            }

            TextRender::getInstance()->setFontColor(rgba[0], rgba[1], rgba[2], rgba[3]);
            TextRender::getInstance()->renderText(_xOffs, _yOffs, _strLabel.c_str());
        }
        break;

        case eTextRenderType::BOX:
        {
            Rect rcShadow;

            if (_bShadow)
            {
                TextBoxParams params;
                params.align = static_cast<int>(eTRAlign::TR_ALIGN_CENTER);
                params.color = getShadowColor(rgba[3]);
                params.maxWidth = _fBoxCx;
                params.x = 2;
                params.y = 2;

                TextRender::getInstance()->renderTextBox(_strLabel.c_str(), params, &rcShadow);
            }

            TextBoxParams params;
            params.align = static_cast<int>(eTRAlign::TR_ALIGN_CENTER);
            params.color = TextRender::floatColor2Uint(rgba);
            params.maxWidth = _fBoxCx;
            params.x = 0;
            params.y = 0;

            TextRender::getInstance()->renderTextBox(_strLabel.c_str(), params, &_rcBounds);

            if (_bShadow)
                _rcBounds.unite(&rcShadow);
        }
        break;

        case eTextRenderType::HTML_BOX:
        {
            if (_htmlDoc)
            {
                // цвет документа может меняться каждый кадр
                _htmlDoc.setDocumentColor(TextRender::floatColor2Uint(rgba));

                Rect rc = HTMLDom::getInstance()->render(_htmlDoc.getRootId(),
                                                         _fHTMLCalcedBoxCx);

                const float fEps = 0.01f;

                if (std::fabs(rc.x  - _rcBounds.x)  > fEps ||
                    std::fabs(rc.y  - _rcBounds.y)  > fEps ||
                    std::fabs(rc.cx - _rcBounds.cx) > fEps ||
                    std::fabs(rc.cy - _rcBounds.cy) > fEps)
                {
                    _rcBounds = rc;

                    if (rc.cx > 0.f)
                        _fHTMLCalcedBoxCx = rc.cx;

                    _bHtmlLayoutDirty = true;
                }
            }
        }
        break;

        default:
            assert(false);
            break;
    }
}

void StaticLabel::setShadow(bool bShadow)
{
    _bShadow = bShadow;
}

void StaticLabel::setTextOffs(float x, float y)
{
    _xOffs = x;
    _yOffs = y;

    updateText();
}

unsigned int StaticLabel::getShadowColor(float fAlphaMul)
{
    uint8_t a = static_cast<uint8_t>(0.45f * fAlphaMul * 255.f);
    return 0x10101000 | a;
}

void StaticLabel::setFontInfo()
{
    TextRender::getInstance()->setFont(_sFontName.c_str());
    TextRender::getInstance()->setTextAlign(_eAlign);
    TextRender::getInstance()->setFontSize(_fFontSize);
    TextRender::getInstance()->setLineHeightMul(_fFontHeightMul);
    TextRender::getInstance()->setLineSpacingMul(_fLineSpacingMul);
}

void StaticLabel::updateText()
{
    if (!_bTextCreated)
        return;

    setFontInfo();
    setMouseTrack(false);

    _htmlDoc.reset();

    switch (_eRenderType)
    {
        case eTextRenderType::NORMAL:
        {
            TextRender::getInstance()->getTextBounds(_xOffs,
                                                      _yOffs,
                                                      _strLabel.c_str(),
                                                      &_rcBounds);
        }
        break;

        case eTextRenderType::BOX:
        {
            TextBoxParams params;
            params.align = static_cast<int>(eTRAlign::TR_ALIGN_CENTER) |
                           static_cast<int>(eTRAlign::TR_ALIGN_TOP);
            params.color = 0xFFFFFFFF;
            params.maxWidth = _fBoxCx;
            params.x = 0;
            params.y = 0;

            TextRender::getInstance()->measureTextBox(_strLabel.c_str(), params, &_rcBounds);
        }
        break;

        case eTextRenderType::HTML_BOX:
        {
            _htmlDoc = CHTMLDocument(_strLabel.c_str()); // парсит, владеет root'ом
            _htmlDoc.setHostContainer(weak_from_this());

            subscribeDocEvents();

            _rcBounds = HTMLDom::getInstance()->measure(_htmlDoc.getRootId(), _fBoxCx);
            _fHTMLCalcedBoxCx = _rcBounds.cx;

            setMouseTrack(true);

            _bHtmlLayoutDirty = true;
        }
        break;

        default:
            assert(false);
            break;
    }
}

void StaticLabel::setText(const char* lpccLabelText)
{
    if (_strLabel == lpccLabelText)
        return;

    _bTextCreated = true;
    _strRawLabel = lpccLabelText;
    _strLabel = lpccLabelText;

    updateText();
}

void StaticLabel::setTextFmt(const char* lpccLabelText, ...)
{
    va_list args;
    va_start(args, lpccLabelText);

    setTextFmtV(lpccLabelText, args);

    va_end(args);
}

void StaticLabel::setTextFmtV(const char* lpccLabelText, va_list argptr)
{
    static char pFormated[1024];
    pFormated[0] = 0;

    if (_strLabel == lpccLabelText)
        return;

    _bTextCreated = true;

    vsnprintf(pFormated, SIZE_OF(pFormated), lpccLabelText, argptr);

    _strRawLabel = pFormated;
    _strLabel = pFormated;

    updateText();
}

float StaticLabel::calcCx()
{
    return _rcBounds.cx;
}

float StaticLabel::calcCy()
{
    return _rcBounds.cy;
}

float StaticLabel::getLineHeight(void)
{
    setFontInfo();
    return TextRender::getInstance()->getLineHeight();
}

float StaticLabel::getTextWidth(LPCTSTR lpszText)
{
    setFontInfo();
    return TextRender::getInstance()->getTextWidth(lpszText ? lpszText : _strLabel.c_str());
}

bool StaticLabel::getNotTransBounds(Rect* p)
{
    assert(p);

    *p = _rcBounds;

    return true;
}

void StaticLabel::subscribeDocEvents()
{
    auto rootId = _htmlDoc.getRootId();

    // метка подписывается первой — её мосты гарантированно отработают
    // до пользовательских биндов (конкретная нода -> any node -> документ)
    _htmlDoc.setCursorChangedHandler([this](const char* sz)
    {
        handleCursorChanged(sz);
    });

    _htmlDoc.setDocumentScaleHandler([this]
    {
        return handleDocumentScale();
    });

    _htmlDoc.onHoverAny(
        [this, rootId](unsigned long long nodeId)
        {
            if (_onHoverElement)
                _onHoverElement(rootId, nodeId);
        },
        [this, rootId](unsigned long long nodeId)
        {
            if (_onLeaveElement)
                _onLeaveElement(rootId, nodeId);
        });

    _htmlDoc.onSelectChangedAny([this](unsigned long long selectId, int index, const char* value)
    {
        handleSelectChanged(selectId, index, value);
    });

    _htmlDoc.onSliderChangedAny([this](unsigned long long nodeId, float value)
    {
        handleSliderChanged(nodeId, value);
    });

    _htmlDoc.onDragAny([this](unsigned long long, bool bEnd)
    {
        handleDrag(bEnd);
    });

    _htmlDoc.onDropdownRequestedAny([this](unsigned long long selectId)
    {
        handleDropdownRequested(selectId);
    });
}

void StaticLabel::handleCursorChanged(const char* szNewCursor)
{
    assert(szNewCursor);

    if (!strcmp("hand", szNewCursor))
    {
        Engine::getInstance().setMouseCursor(eMouseCursorType::E_MCT_POINTER);
    }
    else
    {
        Engine::getInstance().setMouseCursor(eMouseCursorType::E_MCT_NORMAL);
    }
}

void StaticLabel::handleDropdownRequested(unsigned long long selectId)
{
    HTMLSelectPopup::show(selectId);
}

void StaticLabel::handleDrag(bool bEnd)
{
    auto ptrDlg = getParentDialog();
    if (!ptrDlg)
        return;

    auto pDlg = std::dynamic_pointer_cast<BaseDialog>(ptrDlg);
    if (pDlg)
        pDlg->enableDrag(bEnd);
}

float StaticLabel::handleDocumentScale()
{
    auto ptrDlg = getParentModalDialog();
    return ptrDlg ? ptrDlg->getScaleX() : 1.f;
}

void StaticLabel::handleSliderChanged(unsigned long long nodeId, float value)
{
    if (_onSliderChangedCb)
        _onSliderChangedCb(value);

    if (_onSliderChangedExCb)
        _onSliderChangedExCb(nodeId, value);
}

void StaticLabel::handleSelectChanged(unsigned long long selectId, int index, const char* value)
{
    if (_onSelectChangedCb)
        _onSelectChangedCb(selectId, index, value);
}

_G2D_NAMESPACE_END_