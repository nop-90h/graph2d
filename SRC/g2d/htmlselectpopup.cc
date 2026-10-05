#include "htmlselectpopup.h"
#include "gfx.h"
#include "sceneresize.h"
#include "spriteloader.h"
#include "widgets/nineslice.h"
#include "widgets/staticlabel.h"
#include "inputcontroller.h"
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

_G2D_NAMESPACE_BEGIN_

HTMLSelectPopupPtr HTMLSelectPopup::_current;

static const float HTML_SELECT_POPUP_EDGE_FIRST_TIMEOUT = 0.3f;
static const float HTML_SELECT_POPUP_EDGE_MIN_SPEED     = 180.f;
static const float HTML_SELECT_POPUP_EDGE_MAX_SPEED     = 780.f;
static const int   FORCE_POINTER_MODE                   = -1;

static CSpritePtr getPopupSprite(const std::string& path)
{
    if (!path.empty())
    {
        auto pSpr = SpriteLoader::getInstance()->getSprite(path.c_str());
        if (pSpr)
            return pSpr;
    }
    return SpriteLoader::getInstance()->getSprite("UI/whitebox");
}

static std::string fmtF(float v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", v);
    return buf;
}

static std::string fmtColor(unsigned int c)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "#%08X", c);
    return buf;
}

static std::string escapeHTML(const std::string& s)
{
    std::string r;
    r.reserve(s.size());
    for (char ch : s)
    {
        switch (ch)
        {
        case '<':  r += "&lt;";   break;
        case '>':  r += "&gt;";   break;
        case '&':  r += "&amp;";  break;
        case '"':  r += "&quot;"; break;
        default:   r += ch;       break;
        }
    }
    return r;
}

HTMLSelectPopupPtr HTMLSelectPopup::show(unsigned long long selectId)
{
    auto* dom = HTMLDom::getInstance();

    HTMLSelectView view;
    if (!dom->getSelectView(selectId, view))
        return nullptr;

    if (view.options.empty())
        return nullptr;

    Rect anchor;
    if (!dom->getSelectScreenRect(selectId, anchor))
        return nullptr;

    if (_current && _current->_selectId == selectId)
    {
        closeCurrent();
        return nullptr;
    }

    closeCurrent();

    auto ptr = std::make_shared<HTMLSelectPopup>();

    if (!ptr->initPopup(selectId, view, anchor))
        return nullptr;

    _current = ptr;

    ptr->showAsPanel(E_IH_CAPTUREPOINTER, true);

    // -----------------------------------------------------------------
    // ФИНАЛЬНЫЙ СКЕЙЛ КОМБОБОКСА: отношение экранного размера к
    // локальному. Ровно тот же расчёт, что внутри initPopup.
    // -----------------------------------------------------------------
    float docScale = 1.f;
    if (view.localRect.cx > 0.f && anchor.cx > 0.f)
        docScale = anchor.cx / view.localRect.cx;
    if (docScale <= 0.f)
        docScale = dom->getDocumentScale(dom->getRootIdForNode(selectId));

    ptr->setScale(docScale, docScale);

    float screenW = static_cast<float>(CSceneResize::getInstance()->getGameWidth());
    float screenH = static_cast<float>(CSceneResize::getInstance()->getGameHeight());

    float w = ptr->getDialogCx() * docScale;
    float h = ptr->getDialogCy() * docScale;

    // -----------------------------------------------------------------
    // МАСШТАБ БОЛЬШЕ НЕ ТРОГАЕМ.
    // Размеры уже подобраны в initPopup() в локальных единицах с
    // учётом docScale и границ экрана. Домножение масштаба на k
    // (как было раньше) делало попап УЖЕ селекта и визуально
    // отличным от него.
    // -----------------------------------------------------------------

    float x = anchor.x;
    float y = anchor.y + anchor.cy + 2.f;

    if (y + h > screenH)
        y = anchor.y - h - 2.f;

    x = std::max(0.f, std::min(x, screenW - w));
    y = std::max(0.f, std::min(y, screenH - h));

    ptr->setPos(x, y);

    ptr->_popupScreenX = x;
    ptr->_popupScreenY = y;
    ptr->_popupScaleX  = docScale;
    ptr->_popupScaleY  = docScale;
    ptr->_ptrCoordMode = -1;

    ptr->scrollToSelected();

    return ptr;
}

void HTMLSelectPopup::closeCurrent()
{
    if (_current)
    {
        HTMLSelectPopupPtr p = _current;
        p->close();
    }
}

bool HTMLSelectPopup::isOpen()
{
    return _current != nullptr;
}

bool HTMLSelectPopup::close(void)
{
    resetEdgeScrollState();

    if (_eState != eDialogState::SHOWN_AS_PANEL)
        return false;

    bool bRes = BaseDialog::close();

    if (_current.get() == this)
        _current.reset();

    return bRes;
}

void HTMLSelectPopup::resetEdgeScrollState()
{
    _lastHoveredRow   = -1;
    _hasPointerPos    = false;
    _lastPointerX     = 0.f;
    _lastPointerY     = 0.f;
    _popupHovered     = false;
    _topEdgeVisited   = false;
    _bottomEdgeVisited = false;
    _edgeScrolling    = false;
    _activeEdge       = 0;
    _edgeDelayActive  = false;
    _pendingEdge      = 0;
    _edgeDelayTime    = 0.f;
    _bWasDrag         = false;
    _freeEdgePass     = false;
    _rawPointerX      = 0.f;
    _rawPointerY      = 0.f;
    _ptrCoordMode     = -1;
}

bool HTMLSelectPopup::initPopup(
    unsigned long long selectId,
    const HTMLSelectView& view,
    const Rect& anchor)
{
    auto* dom = HTMLDom::getInstance();

    _selectId      = selectId;
    _selectedIndex = view.selectedIndex;

    // noscrollbar на селекте -> попап без скроллбара (bHasScrollBars
    // пробрасывается ниже в PanelHTML::init)
    bool bNoScrollBar = false;
    if (auto selNode = dom->getNode(selectId))
    {
        if (auto pNoSb = selNode->attrs.find("noscrollbar"))
            bNoScrollBar = pNoSb->empty() || *pNoSb == "true" || *pNoSb == "1" || *pNoSb == "yes";
    }

    _itemH = view.style.itemHeight > 0.f ? view.style.itemHeight : 32.f;
    _pad   = view.style.popupPad > 0.f
               ? view.style.popupPad
               : (view.style.contentPad > 0.f ? view.style.contentPad : 4.f);

    // Рамка попапа рисуется nine-slice от (0,0) на весь попап. Отступы
    // контента обязаны включать толщину декоративных кап рамки, иначе
    // контент налезает на декор.
    const bool bFrameNine =
        view.style.frameA > 0.f || view.style.frameB > 0.f ||
        view.style.frameC > 0.f || view.style.frameD > 0.f;

    const float padL  = std::max(_pad, bFrameNine ? view.style.frameA : 0.f);
    const float padT  = std::max(_pad, bFrameNine ? view.style.frameC : 0.f);
    const float padR  = std::max(_pad, bFrameNine ? view.style.frameB : 0.f);
    const float padB  = std::max(_pad, bFrameNine ? view.style.frameD : 0.f);
    const float padHW = padL + padR;
    const float padVH = padT + padB;

    float screenW = static_cast<float>(CSceneResize::getInstance()->getGameWidth());
    float screenH = static_cast<float>(CSceneResize::getInstance()->getGameHeight());

    // -----------------------------------------------------------------
    // ФИНАЛЬНЫЙ СКЕЙЛ КОМБОБОКСА (тот же расчёт, что в show()).
    // Все ограничения экрана ниже переводятся ЧЕРЕЗ него в локальные
    // единицы попапа.
    // -----------------------------------------------------------------
    float docScale = 1.f;
    if (view.localRect.cx > 0.f && anchor.cx > 0.f)
        docScale = anchor.cx / view.localRect.cx;
    if (docScale <= 0.f)
        docScale = dom->getDocumentScale(dom->getRootIdForNode(selectId));

    // максимально допустимые ЛОКАЛЬНЫЕ размеры попапа при этом скейле
    const float maxLocalW = std::max(32.f, (screenW - 8.f) / docScale);
    const float maxLocalH = std::max(32.f, (screenH - 8.f) / docScale);

    int total = static_cast<int>(view.options.size());
    if (total <= 0)
        return false;

    float w = view.style.popupWidth > 0.f
                ? view.style.popupWidth
                : std::max(view.style.minWidth, 120.f);

    if (w < 32.f)
        w = 32.f;

    // Капы строки — строго из popuprowa..d селекта. Не заданы -> нули
    // -> скин честно растягивается спрайтом (штатное поведение).
    const float rowA = view.style.popupRowA;
    const float rowB = view.style.popupRowB;
    const float rowC = view.style.popupRowC;
    const float rowD = view.style.popupRowD;

    // попап обязан вмещать декор строк, иначе капы сожмутся/вылезут за рамку
    const float rowCapW = rowA + rowB;
    if (w < rowCapW + padHW)
        w = rowCapW + padHW;

    if (bFrameNine)
        w = std::max(w, view.style.frameA + view.style.frameB);

    // -----------------------------------------------------------------
    // Ширина: НЕ УЖЕ селекта (view.style.minWidth), но не шире экрана
    // в локальных единицах. Верхняя граница не может опуститься ниже
    // ширины селекта — иначе попап станет уже комбобокса.
    // -----------------------------------------------------------------
    w = std::min(w, std::max(maxLocalW, view.style.minWidth));

    const float rowCapH = rowC + rowD;

    // высота строки не может быть меньше суммы кап ninebutton
    if (_itemH < rowCapH)
        _itemH = rowCapH;

    float innerW = std::max(1.f, w - padHW);

    std::string fontOpen = "<font";
    if (!view.style.fontName.empty())
        fontOpen += " name=\"" + view.style.fontName + "\"";
    if (view.style.fontSize > 0)
        fontOpen += " size=\"" + std::to_string(view.style.fontSize) + "\"";
    fontOpen += " color=\"" + fmtColor(view.style.color) + "\">";

    std::vector<std::string> rowOnly(total);

    for (int i = 0; i < total; ++i)
    {
        const auto& opt = view.options[i];
        const bool bEnabled = opt.enabled;

        std::string row = "<ninebutton id=\"hsel_opt_" + std::to_string(i) + "\"";
        row += " width=\""  + fmtF(innerW) + "\"";
        row += " height=\"" + fmtF(_itemH) + "\"";
        row += " padding=\"" + fmtF(view.style.itemPad) + "\"";

        if (bEnabled)
        {
            if (rowA > 0.f) row += " a=\"" + fmtF(rowA) + "\"";
            if (rowB > 0.f) row += " b=\"" + fmtF(rowB) + "\"";
            if (rowC > 0.f) row += " c=\"" + fmtF(rowC) + "\"";
            if (rowD > 0.f) row += " d=\"" + fmtF(rowD) + "\"";

            row += " hovercolor=\""   + fmtColor(view.style.popupHoverColor)   + "\"";
            row += " pressedcolor=\"" + fmtColor(view.style.popupPressedColor) + "\"";

            if (i == _selectedIndex && !view.style.popupSelectedSrc.empty())
                row += " src=\"" + view.style.popupSelectedSrc + "\"";

            if (!view.style.popupHoverSrc.empty())
                row += " hover=\"" + view.style.popupHoverSrc + "\"";

            if (!view.style.popupPressedSrc.empty())
                row += " pressed=\"" + view.style.popupPressedSrc + "\"";
        }

        row += ">";
        row += "<font color=\"" + fmtColor(opt.color) + "\">";

        if (i == _selectedIndex)
            row += "<b>";

        if (!opt.html.empty())
            row += opt.html;
        else
            row += escapeHTML(opt.label);

        if (i == _selectedIndex)
            row += "</b>";

        row += "</font>";
        row += "</ninebutton>";

        rowOnly[i] = row;
    }

    _rowY.assign(total, 0.f);
    _rowH.assign(total, _itemH);

    float acc = 0.f;
    for (int i = 0; i < total; ++i)
    {
        std::string wrapped = fontOpen + rowOnly[i] + "</font>";

        unsigned long long tmpRoot = dom->parse(wrapped.c_str());
        float hRow = _itemH;

        if (tmpRoot != 0)
        {
            Rect rc = dom->measure(tmpRoot, innerW);
            hRow = std::max(_itemH, rc.cy);
            dom->removeNode(tmpRoot);
        }

        _rowH[i] = hRow;
        _rowY[i] = acc;
        acc += hRow;
    }

    float contentH = acc;

    // -----------------------------------------------------------------
    // Лимит высоты — в ЛОКАЛЬНЫХ единицах: экран, поделённый на скейл.
    // Пока контент влезает в экран при скейле комбобокса, попап
    // показывает все строки без скроллбара; иначе — режем и скроллим.
    // -----------------------------------------------------------------
    int maxRows = view.style.maxPopupRows > 0 ? view.style.maxPopupRows : 8;

    float rowsLimitH = (maxRows < total)
                         ? (_rowY[maxRows] + padVH)
                         : (contentH + padVH);

    float h = std::min(contentH + padVH, rowsLimitH);
    h = std::min(h, maxLocalH);

    // Если высота была урезана — режем только по границе целых строк,
    // чтобы последняя видимая строка показывалась целиком, а не
    // обрезалась посередине со скроллбаром при запасе по высоте.
    if (h < contentH + padVH - 0.5f)
    {
        int fitRows = 0;
        for (int i = 0; i < total; ++i)
        {
            if (_rowY[i] + _rowH[i] + padVH <= maxLocalH + 0.5f)
                fitRows = i + 1;
            else
                break;
        }

        if (fitRows > 0)
            h = _rowY[fitRows - 1] + _rowH[fitRows - 1] + padVH;
        else
            h = std::min(h, _rowH[0] + padVH);
    }

    h = std::max(h, _itemH + padVH);

    if (bFrameNine)
        h = std::max(h, view.style.frameC + view.style.frameD);

    Rect inner;
    inner.set(padL, padT, innerW, std::max(1.f, h - padVH));

    std::string html = fontOpen;
    for (const auto& s : rowOnly)
        html += s;
    html += "</font>";

    PanelHTML::init(
        html.c_str(),
        w,
        h,
        CGfx::getInstance()->getGameIface()->shared_from_this(),
        false,
        !bNoScrollBar);

    Rect viewRc;
    viewRc.set(
        padL,
        padT,
        innerW,
        std::max(1.f, h - padVH));

    _scrollBoxLocal = viewRc;

    _root->setScrollBox(&viewRc, eScrollType::E_ST_VERT);
    _viewH = viewRc.cy;

    _bNoUpdateScaleAndPos   = true;
    _bAutoHideFgControls    = false;

    _ptrLabel->setBoxMode(eTextRenderType::HTML_BOX, innerW);
    _ptrLabel->setPos(padL, padT);
    _ptrLabel->setText(html.c_str());

    {
        RenderTracker track(true, eMouseCursorType::E_MCT_NORMAL);

        CSpritePtr pSpr = getPopupSprite(view.style.frameSrc);
        CContainerPtr ptrBg;

        if (pSpr)
        {
            if (view.style.frameA > 0.f ||
                view.style.frameB > 0.f ||
                view.style.frameC > 0.f ||
                view.style.frameD > 0.f)
            {
                NineSlicePtr ptrNine = std::make_shared<NineSlice>();
                ptrNine->createSlices(
                    pSpr,
                    view.style.frameA,
                    view.style.frameB,
                    view.style.frameC,
                    view.style.frameD);
                ptrNine->build(w, h);
                ptrBg = ptrNine;
            }
            else
            {
                CSpritePtr ptrPlain = pSpr->cloneInitial();
                ptrPlain->setScaleTo(w, h);
                ptrBg = ptrPlain;
            }
        }

        if (ptrBg)
        {
            ptrBg->setPos(0.f, 0.f);
            _frame->addChildAt(ptrBg, 0);
        }
    }

    unsigned long long rootId = getHTMLRootId();

    for (int i = 0; i < total; ++i)
    {
        std::string rowId = "hsel_opt_" + std::to_string(i);
        unsigned long long rowNodeId = dom->getElementIdById(rootId, rowId.c_str());

        if (rowNodeId == 0)
            continue;

        dom->setOnClick(
            rowNodeId,
            [this, dom, i](unsigned long long, float, float, int)
            {
                dom->selectOptionByIndex(_selectId, i, true);
                close();
            });
    }

    resetEdgeScrollState();
    updateMaxScroll();
    updateScrollBox();
    setupEdgeArrows();

    return true;
}

void HTMLSelectPopup::scrollToSelected()
{
    if (!_root || _selectedIndex < 0)
        return;

    if (_selectedIndex >= static_cast<int>(_rowY.size()))
        return;

    float viewH = _viewH;
    if (viewH <= 0.f)
        viewH = _root->getScrollBox().cy;
    if (viewH <= 0.f)
        return;

    float rowTop = _rowY[_selectedIndex];
    float rowH   = _rowH[_selectedIndex];
    float rowBot = rowTop + rowH;

    float maxScroll = _root->getMaxScroll();

    float desired = -(rowTop + rowH * 0.5f - viewH * 0.5f);
    desired = std::max(maxScroll, std::min(0.f, desired));

    if (rowH <= viewH)
    {
        if (rowTop < -desired)
            desired = -rowTop;
        else if (rowBot > -desired + viewH)
            desired = -(rowBot - viewH);

        desired = std::max(maxScroll, std::min(0.f, desired));
    }

    _root->scrollTo(desired);
}

void HTMLSelectPopup::onEvent(int nEvent, void* pData1, void* pData2, void* pData3)
{
    if (nEvent == CSceneResize::EVT_RESOLUTION_CHANGED)
    {
        close();
        return;
    }

    BaseDialog::onEvent(nEvent, pData1, pData2, pData3);
}

void HTMLSelectPopup::update(float dt)
{
    PanelHTML::update(dt);

    if (_eState != eDialogState::SHOWN_AS_PANEL)
        return;

    updateEdgeScroll(dt);
    updateEdgeArrows(dt);
}

void HTMLSelectPopup::resolvePointerLocal(float& outX, float& outY)
{
    if (FORCE_POINTER_MODE >= 0)
        _ptrCoordMode = FORCE_POINTER_MODE;

    const float rawX = _rawPointerX;
    const float rawY = _rawPointerY;

    const float dlgW = static_cast<float>(getDialogCx());
    const float dlgH = static_cast<float>(getDialogCy());

    const float sx = std::max(1e-4f, _popupScaleX);
    const float sy = std::max(1e-4f, _popupScaleY);

    auto inside = [&](float x, float y) -> bool
    {
        return x >= -2.f && x <= dlgW + 2.f &&
               y >= -2.f && y <= dlgH + 2.f;
    };

    const float scaledX  = rawX / sx;
    const float scaledY  = rawY / sy;
    const float screenX  = (rawX - _popupScreenX) / sx;
    const float screenY  = (rawY - _popupScreenY) / sy;

    if (_ptrCoordMode < 0)
    {
        if (inside(rawX, rawY))
        {
            _ptrCoordMode = 0;
        }
        else if (inside(scaledX, scaledY))
        {
            _ptrCoordMode = 1;
        }
        else if (inside(screenX, screenY))
        {
            _ptrCoordMode = 2;
        }
        else
        {
            _ptrCoordMode = 2;
        }
    }
    else if (_ptrCoordMode == 0 && !inside(rawX, rawY))
    {
        if (inside(scaledX, scaledY))
            _ptrCoordMode = 1;
        else if (inside(screenX, screenY))
            _ptrCoordMode = 2;
    }

    if (_ptrCoordMode == 0)
    {
        outX = rawX;
        outY = rawY;
    }
    else if (_ptrCoordMode == 1)
    {
        outX = scaledX;
        outY = scaledY;
    }
    else
    {
        outX = screenX;
        outY = screenY;
    }
}

void HTMLSelectPopup::updateEdgeScroll(float dt)
{
    if (!_root || dt <= 0.f)
        return;

    if (!_hasPointerPos)
        return;

    float px = 0.f;
    float py = 0.f;
    resolvePointerLocal(px, py);

    if (_isDrag)
    {
        HTMLDom::getInstance()->clearInputState();
        _edgeScrolling   = false;
        _activeEdge     = 0;
        _edgeDelayActive = false;
        _pendingEdge    = 0;
        _bWasDrag       = true;
        return;
    }

    float dialogW = static_cast<float>(getDialogCx());
    float dialogH = static_cast<float>(getDialogCy());

    bool bInsidePopup = px >= -1.f && px <= dialogW + 1.f &&
                        py >= -1.f && py <= dialogH + 1.f;

    if (_bWasDrag)
    {
        _bWasDrag = false;
        if (!bInsidePopup)
            _freeEdgePass = true;
    }

    if (!bInsidePopup)
    {
        _edgeScrolling    = false;
        _activeEdge       = 0;
        _edgeDelayActive  = false;
        _pendingEdge      = 0;
        _popupHovered     = false;
        return;
    }

    _popupHovered = true;
    _lastPointerX = px;
    _lastPointerY = py;

    float maxScroll = _root->getMaxScroll();
    if (maxScroll >= -0.5f)
        return;

    Rect sb = _scrollBoxLocal;
    if (sb.cy <= 0.f)
        return;

    ensureHoveredRowVisible(px, py);

    if (px < sb.x || px > sb.x + sb.cx)
    {
        if (_edgeDelayActive)
        {
            _edgeDelayActive = false;
            _pendingEdge     = 0;
        }
        if (_edgeScrolling)
        {
            _edgeScrolling = false;
            _activeEdge    = 0;
        }
        return;
    }

    float top    = sb.y;
    float bottom = sb.y + sb.cy;

    float edge = std::min(sb.cy * 0.22f, std::max(20.f, _itemH * 1.1f));
    if (edge <= 1.f)
        edge = 1.f;
    if (edge * 2.f >= sb.cy)
        edge = std::max(1.f, sb.cy * 0.5f);

    float topInner    = top + edge;
    float bottomInner = bottom - edge;

    if (topInner >= bottomInner)
    {
        float mid = (top + bottom) * 0.5f;
        topInner    = mid;
        bottomInner = mid;
        edge = std::max(1.f, sb.cy * 0.5f);
    }

    bool inTopZone    = (py < topInner);
    bool inBottomZone = (py > bottomInner);

    if (_edgeDelayActive)
    {
        _edgeDelayTime -= dt;

        bool cancelled = false;
        if (_pendingEdge == 1 && py >= topInner)
            cancelled = true;
        else if (_pendingEdge == -1 && py <= bottomInner)
            cancelled = true;

        if (cancelled)
        {
            _edgeDelayActive = false;
            _pendingEdge     = 0;
        }
        else if (_edgeDelayTime <= 0.f && _pendingEdge != 0)
        {
            _edgeDelayActive = false;
            _edgeScrolling   = true;
            _activeEdge      = _pendingEdge;
            _pendingEdge     = 0;
        }
        else if (_edgeDelayTime <= 0.f)
        {
            _edgeDelayActive = false;
        }
    }

    if (!_edgeScrolling && !_edgeDelayActive)
    {
        if (inTopZone)
        {
            if (_freeEdgePass)
            {
                _freeEdgePass   = false;
                _topEdgeVisited = true;
            }
            else if (_topEdgeVisited)
            {
                _edgeScrolling = true;
                _activeEdge    = 1;
            }
            else
            {
                _topEdgeVisited  = true;
                _edgeDelayActive = true;
                _pendingEdge     = 1;
                _edgeDelayTime   = HTML_SELECT_POPUP_EDGE_FIRST_TIMEOUT;
            }
        }
        else if (inBottomZone)
        {
            if (_freeEdgePass)
            {
                _freeEdgePass      = false;
                _bottomEdgeVisited = true;
            }
            else if (_bottomEdgeVisited)
            {
                _edgeScrolling = true;
                _activeEdge    = -1;
            }
            else
            {
                _bottomEdgeVisited = true;
                _edgeDelayActive   = true;
                _pendingEdge       = -1;
                _edgeDelayTime     = HTML_SELECT_POPUP_EDGE_FIRST_TIMEOUT;
            }
        }
    }
    else if (_edgeScrolling)
    {
        if (inTopZone)
        {
            _activeEdge = 1;
        }
        else if (inBottomZone)
        {
            _activeEdge = -1;
        }
        else
        {
            _edgeScrolling = false;
            _activeEdge    = 0;
        }
    }

    if (!_edgeScrolling || _activeEdge == 0)
        return;

    float t = 0.f;
    if (_activeEdge == 1)
    {
        if (py <= top)
        {
            t = 1.f;
        }
        else
        {
            float span = std::max(1.f, edge);
            t = (topInner - py) / span;
        }
    }
    else if (_activeEdge == -1)
    {
        if (py >= bottom)
        {
            t = 1.f;
        }
        else
        {
            float span = std::max(1.f, edge);
            t = (py - bottomInner) / span;
        }
    }

    if (t < 0.f) t = 0.f;
    else if (t > 1.f) t = 1.f;

    float strength = t * t;

    float minSpeed = HTML_SELECT_POPUP_EDGE_MIN_SPEED;
    float maxSpeed = HTML_SELECT_POPUP_EDGE_MAX_SPEED;
    if (maxSpeed < minSpeed)
        maxSpeed = minSpeed;

    float speed = minSpeed + (maxSpeed - minSpeed) * strength;
    float dir   = (_activeEdge == 1) ? 1.f : -1.f;
    float delta = dir * speed * dt;

    float cur     = _root->getScroll();
    float desired = cur + delta;
    desired = std::max(maxScroll, std::min(0.f, desired));

    if (std::fabs(desired - cur) > 0.01f)
    {
        // updateScrollBox() здесь не нужен: позиция слайдера и так
        // синкается через onScrolled-колбэк от scrollTo. Зовя его,
        // мы ловили CContainer::updateScrollBox() -> _fScroll = 0,
        // который без виджета скроллбара откатывал edge-scroll в ноль.
        _root->scrollTo(desired);
    }
}

static void setTriangleVerts(const CSpritePtr& spr, bool bPointUp)
{
    if (!spr)
        return;

    float* v = spr->_verts;

    if (bPointUp)
    {
        v[CSprite::VERT_BLX] = -0.5f; v[CSprite::VERT_BLY] =  0.5f;
        v[CSprite::VERT_ULX] =  0.0f; v[CSprite::VERT_ULY] = -0.5f;
        v[CSprite::VERT_URX] =  0.5f; v[CSprite::VERT_URY] =  0.5f;
        v[CSprite::VERT_BRX] =  0.5f; v[CSprite::VERT_BRY] =  0.5f;
    }
    else
    {
        v[CSprite::VERT_BLX] =  0.0f; v[CSprite::VERT_BLY] =  0.5f;
        v[CSprite::VERT_ULX] = -0.5f; v[CSprite::VERT_ULY] = -0.5f;
        v[CSprite::VERT_URX] =  0.5f; v[CSprite::VERT_URY] = -0.5f;
        v[CSprite::VERT_BRX] =  0.0f; v[CSprite::VERT_BRY] =  0.5f;
    }
}

void HTMLSelectPopup::setupEdgeArrows()
{
    auto wb = SpriteLoader::getInstance()->getSprite("UI/whitebox");
    if (!wb)
        return;

    auto mk = [&](EdgeArrowPart& part, bool bPointUp)
    {
        part.core    = wb->cloneInitial();
        part.outline = wb->cloneInitial();

        if (!part.core || !part.outline)
            return;

        setTriangleVerts(part.core, bPointUp);
        setTriangleVerts(part.outline, bPointUp);

        part.core->setScale(1.f, 1.f);
        part.outline->setScale(1.f, 1.f);

        part.core->setBlendMode(eSpriteBlendMode::NORMAL);
        part.core->setColor(0.f, 0.f, 0.f, 0.85f);

        part.outline->setBlendMode(eSpriteBlendMode::ADDITIVE);
        part.outline->setColor(1.f, 1.f, 1.f, 0.9f);

        addChild(part.outline);
        addChild(part.core);
    };

    mk(_arrUp, true);
    mk(_arrDn, false);

    _arrowTime = 0.f;
}

void HTMLSelectPopup::updateEdgeArrows(float dt)
{
    if (!_arrUp.core || !_root)
        return;

    _arrowTime += dt;

    float scroll    = _root->getScroll();
    float maxScroll = _root->getMaxScroll();

    bool canUp   = scroll < -0.5f;
    bool canDown = scroll > maxScroll + 0.5f;

    float dScroll = scroll - _lastArrowScroll;
    _lastArrowScroll = scroll;

    bool animUp = dScroll >  0.01f;
    bool animDn = dScroll < -0.01f;

    float pulseUp = animUp ? 1.0f + 0.18f * std::sin(_arrowTime * 9.0f) : 1.0f;
    float pulseDn = animDn ? 1.0f + 0.18f * std::sin(_arrowTime * 9.0f) : 1.0f;

    Rect sb = _root->getScrollBox();

    float cx = sb.x + sb.cx * 0.5f;

    Point ptUp = { getDialogCx() * 0.5f, 12 };
    Point ptDn = { getDialogCx() * 0.5f, getDialogCy() - 12 };

    auto place = [&](EdgeArrowPart& part, const Point& pt, float k)
    {
        if (part.core)
        {
            part.core->setPos((float)pt.x, (float)pt.y);
            part.core->setScale(20.0f * k, 16.0f * k);
        }
        if (part.outline)
        {
            part.outline->setPos((float)pt.x, (float)pt.y);
            part.outline->setScale(28.0f * k, 22.0f * k);
        }
    };

    place(_arrUp, ptUp, pulseUp);
    place(_arrDn, ptDn, pulseDn);

    if (_arrUp.core)    _arrUp.core->setVisible(canUp);
    if (_arrUp.outline) _arrUp.outline->setVisible(canUp);
    if (_arrDn.core)    _arrDn.core->setVisible(canDown);
    if (_arrDn.outline) _arrDn.outline->setVisible(canDown);
}

void HTMLSelectPopup::ensureHoveredRowVisible(float localX, float localY)
{
    if (!_root || _rowY.empty())
        return;

    if (_isDrag)
        return;

    Rect sb = _scrollBoxLocal;
    if (sb.cy <= 0.f)
        return;

    if (localX < sb.x || localX > sb.x + sb.cx)
        return;

    float viewH = _viewH > 0.f ? _viewH : sb.cy;

    float scroll = _root->getScroll();
    float htmlY  = localY - sb.y;
    float contentY = htmlY - scroll;

    int row = -1;
    for (int i = 0; i < static_cast<int>(_rowY.size()); ++i)
    {
        if (contentY >= _rowY[i] && contentY < _rowY[i] + _rowH[i])
        {
            row = i;
            break;
        }
    }

    if (row < 0)
    {
        _lastHoveredRow = -1;
        return;
    }

    if (row == _lastHoveredRow)
        return;

    _lastHoveredRow = row;

    if (_edgeScrolling)
        return;

    float rowTop = _rowY[row];
    float rowBot = rowTop + _rowH[row];

    if (_rowH[row] > viewH)
        return;

    float visibleTop = -scroll;
    float visibleBot = visibleTop + viewH;

    float desired = scroll;

    if (rowTop < visibleTop)
    {
        desired = -rowTop;
    }
    else if (rowBot > visibleBot)
    {
        desired = -(rowBot - viewH);
    }
    else
    {
        return;
    }

    float maxScroll = _root->getMaxScroll();
    desired = std::max(maxScroll, std::min(0.f, desired));

    if (std::fabs(desired - scroll) > 0.5f)
    {
        _root->animateScrollTo(desired, 0.20f, Easing::outExpo);
    }
}

bool HTMLSelectPopup::onPointerDown(int32_t x, int32_t y)
{
    _rawPointerX = static_cast<float>(x);
    _rawPointerY = static_cast<float>(y);
    _lastPointerX = _rawPointerX;
    _lastPointerY = _rawPointerY;
    _hasPointerPos = true;

    bool bRes = BaseDialog::onPointerDown(x, y);

    if (!bRes)
    {
        setTimeout(0.f, [this]()
        {
            close();
        });
    }

    return bRes;
}

bool HTMLSelectPopup::onPointerMove(int32_t x, int32_t y)
{
    _rawPointerX = static_cast<float>(x);
    _rawPointerY = static_cast<float>(y);
    _lastPointerX = _rawPointerX;
    _lastPointerY = _rawPointerY;
    _hasPointerPos = true;

    return BaseDialog::onPointerMove(x, y);
}

static void AttachSelectPopupsRecursive(unsigned long long nodeId)
{
    auto* dom = HTMLDom::getInstance();

    auto node = dom->getNode(nodeId);
    if (!node)
        return;

    if (node->tag == eHTMLTag::Select)
    {
        if (node->attrs.find("data-html-select-popup") == nullptr)
        {
            node->attrs.insert("data-html-select-popup", "1");

            auto oldClick = node->onClick;

            dom->setOnClick(
                nodeId,
                [oldClick](unsigned long long clickedNodeId, float x, float y, int button)
                {
                    HTMLSelectPopup::show(clickedNodeId);

                    if (oldClick)
                        oldClick(clickedNodeId, x, y, button);
                });
        }
    }

    auto children = dom->getChildren(nodeId);
    for (auto childId : children)
        AttachSelectPopupsRecursive(childId);
}

void AttachHTMLSelectPopups(
    unsigned long long rootId,
    std::weak_ptr<CContainer> host)
{
    if (rootId == 0)
        return;

    auto* dom = HTMLDom::getInstance();

    if (auto hostPtr = host.lock())
        dom->setHostContainer(rootId, hostPtr);

    AttachSelectPopupsRecursive(rootId);
}

_G2D_NAMESPACE_END_