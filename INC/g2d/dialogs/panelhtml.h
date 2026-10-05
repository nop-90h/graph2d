#pragma once

#include "g2d.h"
#include "dialogs/basedialog.h"
#include "widgets/staticlabel.h"
#include "htmldom.h"

_G2D_NAMESPACE_BEGIN_

class PanelHTML : public BaseDialog
{
protected:
    StaticLabelPtr  _ptrLabel;
    bool            _bHasScrollShadows = true;
    bool            _bHasScrollBar = true;
public:
    void            init                    (LPCTSTR        lpszHTML,
                                             float          cx,
                                             float          cy,
                                             CContainerPtr  ptrParent,
                                             bool           bHasScrollShadows = true,
                                             bool           bHasScrollBar = true);

    void            finalizeScroll          ();
    
    auto&           getHTMLDocument         (void)
    {         
        assert(_ptrLabel);
        return _ptrLabel->getHTMLDocument();
    }

    auto            getHTMLRootId           (void)
    {
        assert(_ptrLabel);
        return _ptrLabel->getHTMLRootId();
    }


protected:
    virtual void    getScrollShadowPos      (float&         fX1,
                                             float&         fY1,
                                             float&         fX2,
                                             float&         fY2,
                                             float&         fCX,
                                             float&         fCY) override;

    virtual void    createScrollBar         (eScrollType    eScrollType) override;

    virtual void    createScrollShadow      (Rect* pRcScroll) override
    {
        if (_bHasScrollShadows)
            BaseDialog::createScrollShadow(pRcScroll);
    }
};

using PanelHTMLPtr = std::shared_ptr<PanelHTML>;

_G2D_NAMESPACE_END_