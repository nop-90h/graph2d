#pragma once
#include "basedialog.h"
#include "paneltabs.h"
#include "touchhandler.h"
#include "slotmachine.h"
#include "panelhtmldemo.h"
#include "spinecont.h"
#include <vector>

class PanelSpriteContDemo; // определён в dialogdemo.cc

class DialogDemo : public BaseDialog
{
protected:
enum class eBehindBgKind
{
    SAKURA,
    HEARTS,
    GRENADES,
    FIREWORKS,
    BLIZZARD,
    AUTUMN,
SOLITAIRE_STORM,

};
private:
    CContainerPtr                        _ptrParticlesDemo;
    CContainerPtr                        _ptrTextDemoCont;
    CContainerPtr                        _ptrSlotsDemoCont;
    CContainerPtr                        _ptrCardsDemoCont;
    CContainerPtr                        _ptrSpineDemoCont;
    CContainerPtr                        _ptrBakedDemoCont;
    CContainerPtr                        _ptrBakedSpines;
    std::shared_ptr<PanelSpriteContDemo> _ptrSpriteDemo;    
    PanelHtmlDemoPtr                     _ptrPanel;
    PanelHtmlDemoPtr                     _ptrPanel2;
    PanelHtmlDemoPtr                     _ptrPanel3;
    PanelHtmlDemoPtr                     _ptrPanel4;
    PanelHTMLPtr                         _ptrNetworkingDemo;
    PanelHTMLPtr                         _ptrAboutDemo;
    CSpritePtr                           _ptrArrow;
    int                                  _nCardsDragged = 0;
    CSlotMachinePtr                      _ptrSlotMachine; // Вынесли слоты в отдельный класс
    PanelTabsPtr                         _ptrTabsPanel;
    TouchHandlerPtr                      _ptrTouchHandler;
    int                                  _currentTextConfigIdx = 0;
    CSpinePtr                            _ptrLamp;

    // Вкладка "Диалоговая система": один StaticLabel + HTML-документ
    CContainerPtr                          _ptrDialogsDemoCont;
    StaticLabelPtr                         _ptrDialogsDemoLabel;
    unsigned long long                     _demoStatusNodeId = 0;
    std::vector<std::weak_ptr<BaseDialog>> _demoDialogs;

public:
    bool            init                    (void);

protected:
    void            initBakedSpine          (void);
    void            initNewHtmlDemo         (void);
    void            initParticlesDemo       (void);
    void            initFontsDemo           (void);
    void            spawnScrollText         (size_t         configIdx,
                                             float          startY);
    void            initSlotsDemo           (void);
    void            initCardsDemo           (void);
    void            initSpineAndLight       (void);

    void            updateTitle             (void);
    void            playAboutEntrance       (void);
    void            onActiveTabChanged      (int            nTab);

    virtual void    onShow                  (bool           bShow);
    virtual void    updateScaleAndPos       (void);
    virtual void    onCloseButtonClick      (void) override;

    // Демо диалоговой системы
    void            initDialogsDemo         (void);
    void            openStackLevel          (int            level);
    CContainerPtr   createBehindParticles   (eBehindBgKind  eKind, BaseDialog* pFlowDlg = nullptr);
    void            trackDemoDialog         (const BaseDialogPtr& ptr);
    void            closeDemoDialogs        (void);
};