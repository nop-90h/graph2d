#pragma once

#include "g2d.h"
#include "dialogs/basedialog.h"
#include "widgets/staticlabel.h"

_G2D_NAMESPACE_BEGIN_

struct TabInfo_t
{
    LPCCTEXT                lpccCaption   = nullptr;
    LPCCTEXT                lpccActive    = nullptr;
    LPCCTEXT                lpccInactive  = nullptr;
    float                   fA            = 0;
    float                   fB            = 0;
    std::optional<int64_t>  tabId         = std::nullopt;
};

typedef std::function<void(int64_t nTab)> OnTabSelectCb;

struct TabFont
{
    bool    bIsHtml      = false;
    LPCTSTR lpszFontName = Engine::getCfg().DEFAULT_FONT_NAME.c_str();
    float   fFontSize    = Engine::getCfg().DEFAULT_FONT_SIZE;
};

class PanelTabs:public BaseDialog
{
public:
        void                init                    (TabInfo_t*           pTabs,
                                                     size_t               nCount,
                                                     OnTabSelectCb        cb,
                                                     BaseDialog*          pParent,
                                                     TabFont&             tabFont);
        void                init                    (std::span<TabInfo_t> tabs,
                                                     OnTabSelectCb        cb,
                                                     BaseDialog*          pParent,
                                                     TabFont&             tabFont)
        {
            init(tabs.data(), tabs.size(), cb, pParent, tabFont);
        }
        int                 getSelectedTab          (void) { return _ptrSelected ? _ptrSelected->getId() - 1 : -1; }
private:                            
        float               addTab                  (StaticLabelPtr       pLabel,
                                                     float                cx,
                                                     float                fA,
                                                     float                fB,
                                                     LPCCTEXT             lpccActive   = nullptr, 
                                                     LPCCTEXT             lpccInactive = nullptr);
protected:
    virtual void            onShow                  (bool                 bShow);
    virtual void            updateScaleAndPos       (void);
            void            setSelected             (CContainerPtr        ptrSelected,
                                                     bool                 bCallCb);

private:
    OnTabSelectCb   _cb;
    CContainerPtr   _ptrSelected;
};

typedef std::shared_ptr<PanelTabs> PanelTabsPtr;

_G2D_NAMESPACE_END_