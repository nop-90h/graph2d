#include "pch.h"
#include "panelhtml.h"

_G2D_NAMESPACE_BEGIN_

void PanelHTML::init(LPCTSTR lpszHTML,
                     float cx,
                     float cy,
                     CContainerPtr ptrParent,
                     bool bHasScrollShadows,
                     bool bHasScrollBars)
{
    _bHasScrollShadows = bHasScrollShadows;
    _bHasScrollBar    = bHasScrollBars;

    Rect initialScrollBoxRc{0, 30, cx, cy - 40};

    BaseDialog::init(cx,
                     cy,
                     eScrollType::E_ST_VERT,
                     &initialScrollBoxRc,
                     nullptr,
                     ptrParent.get());

    _ptrLabel = std::make_shared<StaticLabel>();
    _ptrLabel->setBoxMode(eTextRenderType::HTML_BOX, cx - 40);
    _ptrLabel->setText(lpszHTML);
    _ptrLabel->setY(80);
    _ptrLabel->setInteractive(true);
    _ptrLabel->setXPosCentered(cx);

    _root->addChild(_ptrLabel);

    finalizeScroll();
}

void PanelHTML::finalizeScroll()
{
    if (_eScrollType != eScrollType::E_ST_NONE)
    {
        updateMaxScroll();
        updateScrollBox();
    }
}

void PanelHTML::getScrollShadowPos(float& fX1,
                                   float& fY1,
                                   float& fX2,
                                   float& fY2,
                                   float& fCX,
                                   float& fCY)
{
    assert(_eScrollType == E_ST_VERT);

    fY1 -= 5;
    fY2 -= 15;

    fCY = 100;
    fCX = _fDialogCx + 10;
}

void PanelHTML::createScrollBar(eScrollType eScrollType)
{
    if (_bHasScrollBar)
    {
        RenderTracker trackerScoped;

        _ptrScollBar = std::make_shared<ScrollBar>();

        switch (eScrollType)
        {
            case eScrollType::E_ST_VERT:
            {
                _ptrScollBar->create(eScrollType::E_ST_VERT, _fDialogCy - 250);

                _frame->addChild(_ptrScollBar);

                _ptrScollBar->setX(_fDialogCx - 25);
                _ptrScollBar->setYPosCentered(_fDialogCy, 20);
            }
            break;

            default:
                assert(false);
                break;
        }
    }
}

_G2D_NAMESPACE_END_