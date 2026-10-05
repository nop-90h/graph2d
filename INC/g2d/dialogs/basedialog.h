#pragma once
#include "g2d.h"
#include "container.h"
#include "sprite.h"
#include "widgets/nineslice.h"
#include "widgets/threeslice.h"
#include "eventemmiter.h"
#include "eventdef.h"
#include "inputcontroller.h"
#include "widgets/staticlabel.h"
#include "widgets/scrollbar.h"
#include "widgets/button.h"
#include "spriteloader.h"

_G2D_NAMESPACE_BEGIN_

class BaseDialog;

typedef std::shared_ptr<BaseDialog>     BaseDialogPtr;
typedef std::deque<BaseDialogPtr>       DialogStack;
typedef std::set<BaseDialogPtr>         Dialogs;

enum eCurrenciesPos
{
    FOREGROUND,
    INSIDE_DIALOG
};

enum eInputHandling
{
    E_IH_CAPTUREPOINTER,
    E_IH_RELAXED,
    E_IH_DISPATCHBYPARENT,
    E_IH_NONE
};

enum eDialogRoot
{
    GAME,
    INTERFACE
};

enum class eDialogCloseStyle
{
    CLOSE_BUTTON,
    CLICK_ANYWHERE,
    CLICK_OUT_OF_BOUNDS,
};

class BaseDialog : public Widget,
                   public IEventListener
{
    friend class CContainer;

protected:
    enum eDialogState
    {
        UNINIT,
        INIT_DONE,
        CLOSED,
        OPENED,
        SHOWN_AS_PANEL,
        REMOVED,
    };

    enum ePointerAction
    {
        E_PA_DOWN,
        E_PA_UP,
        E_PA_MOVE,
        E_PA_WHEEL
    };

    inline static constexpr int SCROLLBAR_SYS_ID    = 1;
    inline static constexpr int CLOSE_BUTTON_SYS_ID = 2;

private:
    static DialogStack                       _stack;
    static Dialogs                           _existing;

    inline static bool                       _hideOvelappingModals = true;
    inline static std::vector<BaseDialogPtr> _savedStack;

private:
    static void onContainerParentChanged(CContainer* pCont);

protected:
    // SCROLLING
    bool                _bNoUpdateScaleAndPos = false;
    eScrollType         _eScrollType          = eScrollType::E_ST_NONE;
    Dialogs             _dispatchTo;
    eInputHandling      _eInputHandling       = E_IH_RELAXED;

    inline static CEventEmmiter _emmiter;

protected:
    bool                _bAutoHideFgControls = true;
    eDialogRoot         _eRoot               = eDialogRoot::INTERFACE;
    eCurrenciesPos      _eCurrPos            = eCurrenciesPos::FOREGROUND;
    bool                _bNoCloseButton      = false;

    CContainerPtr       _frame;

    NineSlicePtr        _ptrFg;

    bool                _bFgAnimated       = false;
    float               _fEffectLifeTime   = 0;
    bool                _bEffectGoBack     = false;

    ThreeSliceHorPtr    _titleFrame;
    CContainerPtr       _titleCont;

    CContainerPtr       _ptrBg;
    CContainerPtr       _ptrBehindBg;
    CContainerPtr       _ptrFgBg;

    CContainerPtr       _root;
    CContainerPtr       _fg;

    eDialogState        _eState            = BaseDialog::UNINIT;

    float               _fDialogCx         = 0;
    float               _fDialogCy         = 0;

    std::string         _strTitle;
    StaticLabelPtr      _ptrTitleLabel;

    bool                _isMouseDnInScrollBox = false;
    bool                _isDrag               = false;
    float               _fDragStartPos        = 0;
    float               _fLastDragTime        = 0;
    Point               _ptLastDrag;

    ScrollBarPtr        _ptrScollBar;

    CSpritePtr          _ptrShadowU;
    CSpritePtr          _ptrShadowB;

    std::weak_ptr<CContainer> _pMouseOwner;
    std::weak_ptr<CContainer> _pHover;

    ButtonPtr           _closeButton;

    CContainer*         _preSavedParent;

    SimpleCallback      _onCloseCb;

    bool                _bDispatchOutOfClientRect = false;
    bool                _bTutorialEventsOnly      = false;
    int                 _nWaitForTutorEvt         = -1;

    bool                _closedByCloseButton      = false;
    eDialogCloseStyle   _eCloseStyle              = eDialogCloseStyle::CLOSE_BUTTON;

    bool                _bCanDrag                 = true;

public:
    enum Events
    {
        EVT_ON_SHOW_ANY = EventsNs::DIALOGS_EVT_FIRST,
        EVT_ON_CLOSE_ANY,
        EVT_ON_CLOSE_ALL,
    };

public:
    virtual                 ~BaseDialog             (void);

    static  void            closeAllModals          (void);

    static  void            saveStackAndHide        (void);
    static  void            restoreStackAndShow     (void);
    static  void            setShowOvelappingDialogs(bool           bShow);

            void            addBehindBgToStage      (void);

            bool            open                    (SimpleCallback onCloseCb = nullptr);
    virtual bool            close                   (void);

            void            createCloseButton       (CSpritePtr ptrNormal,
                                                     CSpritePtr ptrPressed  = nullptr,
                                                     CSpritePtr ptrHover    = nullptr,
                                                     CSpritePtr ptrDisabled = nullptr);

            void            enableDrag              (bool bCanDrag);

    inline  void            setDefBehindBg          (void)
    {
        setBehindBg(SpriteLoader::getInstance()->getSprite("DLG/dlgBg"));
    }

    inline  float           getDialogCx             (void)
    {
        return _fDialogCx;
    }

    inline  float           getDialogCy             (void)
    {
        return _fDialogCy;
    }

    inline  void            setBehindBg             (CContainerPtr p, bool bAdd = false)
    {
        _ptrBehindBg = p;

        if (bAdd)
            addBehindBgToStage();
    }

            auto&           getBehindBg             (void)
    {
        return _ptrBehindBg;
    }

    inline  void            setForegroundBg         (CContainerPtr p, bool bAdd = false)
    {
        _ptrFgBg = p;

        if (bAdd)
            addFgBgToStage();
    }

            auto&           getForegroundBg         (void)
    {
        return _ptrFgBg;
    }

    inline  CContainerPtr   getRoot                 (void)
    {
        return _root;
    }

    inline  static CEventEmmiter* getEmmiter        (void)
    {
        return &_emmiter;
    }

            bool            isModal                 (void)
    {
        return _eState == eDialogState::OPENED;
    }

            void            setFgAnimated           (bool bFgAnimated = true)
    {
        _bFgAnimated = bFgAnimated;
    }

    static  BaseDialogPtr   getTopModal             (void)
    {
        return _stack.empty() ? nullptr : _stack.back();
    }

protected:
            void            _close                  (void);

    virtual void            update                  (float dt) override;

public:
    void    bringChildUnderForeground(CContainerPtr ptrChild) { assert(_ptrFg); if (_ptrFg) _ptrFg->getParent()->bringChildUnder(ptrChild, _ptrFg);}

    bool            isClosedByCloseButton   (void)
    {
        return _closedByCloseButton;
    }

    bool            showAsPanel             (eInputHandling eHandling = E_IH_RELAXED,
                                             bool bNoUpdateScaleAndPos = true);

    void            removeFromStage         (void);

    inline  void    updateMaxScroll         (void)
    {
        _root->updateMaxScroll();
    }

            void    updateScrollBox         (void);

    inline  void    scrollToChild           (int nChildIdx)
    {
        _root->scrollToChild(nChildIdx);
        onScrolled();
    }

    inline  void    scrollToChild           (CContainerPtr pChild)
    {
        _root->scrollToChild(pChild);
        onScrolled();
    }

    inline  float   getScroll               (void)
    {
        return _root->getScroll();
    }

    inline  float   setScroll               (float fScroll)
    {
        return _root->scrollTo(fScroll);
    }

    inline  BaseDialogPtr getPtr            (void)
    {
        return std::dynamic_pointer_cast<BaseDialog>(shared_from_this());
    }

    inline  void    setNoCloseButton        (bool bNoClose = true)
    {
        _bNoCloseButton = bNoClose;
    }

    virtual void    notifyInt               (BaseDialog* pChild, int64_t i)
    {
        assert(false);
    }

            void    notifyParent            (int64_t i);

            void    notifyParentEx          (uint64_t param1 = 0,
                                             uint64_t param2 = 0,
                                             uint64_t param3 = 0,
                                             uint64_t param4 = 0);

            void    removeScroll            (void);

    virtual void    notifyEx                (BaseDialog* pChild,
                                             uint64_t param1,
                                             uint64_t param2,
                                             uint64_t param3,
                                             uint64_t param4)
    {
        assert(false);
    }

    inline static const DialogStack& getStack(void)
    {
        return _stack;
    }

protected:
    CContainer*     getGfxRoot              (void);

    virtual bool    init                    (float          cx,
                                             float          cy,
                                             eScrollType    eScroll,
                                             Rect*          pInnerChildrenRect,
                                             CContainerPtr  ptrBg,
                                             CContainer*    pParent = NULL);

            void    lockWaitTutorial        (int nWaitFor)
    {
        assert(nWaitFor > -1);
        _nWaitForTutorEvt = nWaitFor;
    }

    virtual CContainerPtr createBack        (CContainerPtr ptrFrom,
                                             float cx,
                                             float cy);

            void    animateFg               (float dt);

            void    createFgLayer           (CContainerPtr ptrFrom,
                                             float nineSliceA,
                                             float nineSliceB,
                                             float nineSliceC,
                                             float nineSliceD,
                                             Point* pDims = nullptr);

    virtual void    setTitle                (const char* lpccTitle,
                                             CSpritePtr threeSliceSpr,
                                             float fA,
                                             float fB,
                                             bool  bHtml = false);

    virtual void    createScrollShadow      (Rect* pRcScroll);

            void    scrollByDelta           (float fDelta);

            void    doOnShow                (bool bOnShow);
            void    stopScroll              (void);

            const Rect* getViewRect         (void);
            const Rect* getViewRectRecursive(void);

            void    _updateScaleAndPos      (void);
    virtual void    updateScaleAndPos       (void);

            float   getTopMargin            (void);

            void    addFgBgToStage          (void);

    virtual void    createScrollBar         (eScrollType eScrollType);

            void    setHovered              (CContainerPtr pHovered,
                                             bool bSysWidget = false);

            void    addDispatchedDlg        (BaseDialogPtr pDispatchTo);
            void    removeDispatchedDlg     (BaseDialogPtr pDispatchTo);

    virtual bool    dispatchEvents          (int32_t x,
                                             int32_t y,
                                             ePointerAction eAction);

    virtual void    getScrollShadowPos      (float& fX1,
                                             float& fY1,
                                             float& fX2,
                                             float& fY2,
                                             float& fCX,
                                             float& fCY);

    virtual void    onCloseButtonClick      (void);

    virtual void    onTutorialEventBegin    (int nEvt);
    virtual void    onTutorialEventEnd      (int nEvt);

            void    bringUnderParent        (void);
            void    bringAboveParent        (void);

            CSpritePtr createVertLine       (bool bRight = true);

            BaseDialogPtr getParentDialog   (void);
            bool    hasParentDialog         (void);

            bool    updateHovered           (int32_t x,
                                             int32_t y);

            void    checkElementRefs        (CContainer* pElement);

            CContainerPtr findTrackedAt     (float fx,
                                             float fy,
                                             const Rect* pCrop);

            void    handleEmptyClick        (int32_t x,
                                             int32_t y);

            bool    canPassEventsTo         (CContainer* pCont);

            // === FIX START ===
            float   screenDeltaToScrollDelta(float fScreenDelta);
            // === FIX END ===

protected: // IEventListener
    virtual void    onEvent                 (int nEvent,
                                             void* pData1 = NULL,
                                             void* pData2 = NULL,
                                             void* pData3 = NULL);

protected:
    // Events
    virtual void    onShow                  (bool bShow);
    virtual bool    onMouseWheel            (int32_t deltaY);

    virtual bool    onSysPointer            (int32_t x,
                                             int32_t y,
                                             ePointerAction eAction);

    virtual bool    onPointerDown           (int32_t x,
                                             int32_t y);

    virtual bool    onPointerMove           (int32_t x,
                                             int32_t y);

    virtual bool    onPointerUp             (int32_t x,
                                             int32_t y);

    virtual bool    onDragBegin             (int32_t x,
                                             int32_t y);

    virtual void    onDragMove              (int32_t x,
                                             int32_t y);

    virtual void    onDragEnd               (int32_t x,
                                             int32_t y);

    virtual void    onScrolled              (void) override;

    virtual void    onClick                 (CContainer* pTarget);
    virtual void    onSysClick              (CContainer* pTarget);

    virtual void    onHover                 (CContainer* pTarget,
                                             bool bSysWidget = false);

    virtual void    onLeave                 (CContainer* pTarget,
                                             bool bSysWidget = false);

public: // CContainer
    virtual void    setScale                (float sx, float sy);
    virtual void    setPos                  (float fx, float fy);
    virtual void    setX                    (float x);
    virtual void    setY                    (float y);
    virtual void    setScaleX               (float fScale);
    virtual void    setScaleY               (float fScale);
    virtual void    setVisible              (bool bVisible);
};

_G2D_NAMESPACE_END_