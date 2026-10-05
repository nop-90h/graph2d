#pragma once

#include "g2d.h"
#include "matrixstack.h"
#include "elementclasses.h"
#include "poollist.h"
#include "easing.h"
#include "inputcontroller.h"
#include "texture.h"
#include "ptrsdefs.h"

_G2D_NAMESPACE_BEGIN_

enum class eLightLayer
{
    GAME,
    BG,
};

inline constexpr int LightMinLayer = -4096;
inline constexpr int LightMaxLayer =  4096;

struct LightForLayerSettings
{
    int   nLayer  = 0;
    float fMul    = 1.f;
    bool  bActive = false;
};

struct LightLayerMinMax
{
    float fMul    = 1.f;
    int nLayerMin = LightMinLayer;
    int nLayerMax = LightMaxLayer;
};

typedef std::array<LightForLayerSettings, 3>            LightLayerArray;
typedef std::variant<LightLayerMinMax, LightLayerArray> LightEmmiterSettings;

struct GPULight
{
    Point       position;
    Point3      color;
    float       intensity          = 1.0f;
    float       radius             = 0.0f;

    bool        bCanAffectGL       = false;
    eLightLayer eLayer             = eLightLayer::GAME;

    CSpritePtr  sprite;

    float       rotate             = 0.0f;

    int         zMin               = LightMinLayer;
    int         zMax               = LightMaxLayer;
    uint16_t    itemCullingMask    = 0xFFFF;

    bool        bScreenSpace       = false;

    float       spritePivotX       = 0.5f;
    float       spritePivotY       = 0.5f;

    float       spriteScaleX       = 0.0f;
    float       spriteScaleY       = 0.0f;

    bool        bExplicitQuad      = false;
    CTexturePtr explicitTexture;

    float       explicitX[4]       = {0.0f, 0.0f, 0.0f, 0.0f};
    float       explicitY[4]       = {0.0f, 0.0f, 0.0f, 0.0f};
    float       explicitU[4]       = {0.0f, 0.0f, 0.0f, 0.0f};
    float       explicitV[4]       = {0.0f, 0.0f, 0.0f, 0.0f};
};

enum eScrollType : int
{
    E_ST_NONE = 0,
    E_ST_VERT,
    E_ST_HOR
};

class CContainer;

typedef std::vector<GPULight>                                           GPULights;

typedef std::shared_ptr<CContainer>                                     CContainerPtr;
typedef std::weak_ptr<CContainer>                                       CContainerWPtr;
typedef std::vector<CContainerPtr>                                      CContainerPtrs;
typedef CContainerPtrs::iterator                                        ContainersIt;
typedef std::vector<CContainer*>                                        Containers;

typedef std::function<void(bool bisPressed, int x, int y)>              FncOnPointerMove;
typedef std::function<bool(CContainerPtr ptrDragObj, float x, float y)> fOnDragBegin;
typedef std::function<void(float x, float y)> fOnDragEvt;
typedef std::function<void(float x, float y, CContainerPtr droppedObj)> FOnDraggingObjDropped;
typedef std::function<void(CContainerPtr droppedObj, bool bIsHover)>    FOnDraggingOver;
typedef std::function<bool(CContainerPtr droppedObj)>                   FOnDragDropAccept;
typedef std::function<void(GPULights& inOutLights)>                     FOnLightCollected;

enum class eChildrenAlign
{
    VERTICAL_COLUMN,
    HORIZONTAL,
};

struct dragTargetHighLightPrevState
{
    float fScaleX = 0;
    float fScaleY = 0;
    float fOrigX  = 0;
    float fOrigY  = 0;
    bool  bSaved  = false;
};

enum class eTweenProp
{
    NONE,
    SCALE,
    SCALE_ABS,
    SCALE_X,
    SCALE_Y,
    X,
    Y,
    ALPHA,
    ROTATE,
    TIMEOUT,
    PROGRESS,
    BLACKEN,
    GAME_CAMERA_X,
    GAME_CAMERA_Y,
    BG_CAMERA_X,
    BG_CAMERA_Y,
    CAMERA_ZOOM,
    BLOOM_INTENSITY,
    BLOOM_THRESHOLD,
    BLOOM_RADIUS,
    FISH_EYE,
    DISTORTION,
    FLESH,
    BLOOD_GHOST,
    BREATH,
    VIGNETTE_RADIUS,
    VIGNETTE_INTENCITY,
    BLOOD_DROPS,
    STAR_TRAILS,
    GRAIN,
    SATURATION,
    CONTRAST,
    BRIGHTNESS,
    LUT,
};

enum class eTweenLoopMode : int
{
    ONCE   = 0,
    REPEAT = 1,
    YOYO   = 2,
};

struct SelfTween
{
    bool                bToErase        = false;
    bool                bNoCallBack     = false;
    bool                bLoop           = false;
    float               fDelay          = 0;
    float               fTime           = 0;
    float               fDurring        = 0;
    float               fStartVal       = 0;
    float               fEndVal         = 0;
    void*               ptrHandleHolder = nullptr;
    eTweenProp          eProp           = eTweenProp::NONE;
    easingFunction      easingFunc      = Easing::linear;
    SimpleCallback      cbOnComplete;

    eTweenLoopMode      eLoopMode       = eTweenLoopMode::ONCE;
    int                 nLoopsLeft      = 0;
    float               fOrigStartVal   = 0.f;
    float               fOrigEndVal     = 0.f;
    easingFunction      easingFuncBack  = nullptr;
    bool                bYoyoBackPhase  = false;
    SimpleCallback      cbOnLoopComplete;

    SelfTween(float              fDelayA,
              float              fDurringA,
              float              fStartValA,
              float              fEndValA,
              eTweenProp         ePropA,
              easingFunction     easingFuncA,
              SimpleCallback     cbOnCompleteA,
              eTweenLoopMode     eLoopModeA,
              int                nLoopsLeftA,
              easingFunction     easingFuncBackA = nullptr,
              SimpleCallback     cbOnLoopCompleteA = {})
        : bToErase        (false)
        , bNoCallBack     (false)
        , bLoop           (eLoopModeA != eTweenLoopMode::ONCE)
        , fDelay          (fDelayA)
        , fTime           (0.f)
        , fDurring        (fDurringA)
        , fStartVal       (fStartValA)
        , fEndVal         (fEndValA)
        , ptrHandleHolder (nullptr)
        , eProp           (ePropA)
        , easingFunc      (easingFuncA)
        , cbOnComplete    (cbOnCompleteA)
        , eLoopMode       (eLoopModeA)
        , nLoopsLeft      (nLoopsLeftA)
        , fOrigStartVal   (fStartValA)
        , fOrigEndVal     (fEndValA)
        , easingFuncBack  (easingFuncBackA)
        , bYoyoBackPhase  (false)
        , cbOnLoopComplete(cbOnLoopCompleteA)
    {
    }

    SelfTween(float              fDelayA,
              float              fDurringA,
              float              fStartValA,
              float              fEndValA,
              eTweenProp         ePropA,
              easingFunction     easingFuncA,
              SimpleCallback     cbOnCompleteA,
              bool               bLoopA)
        : SelfTween(fDelayA,
                    fDurringA,
                    fStartValA,
                    fEndValA,
                    ePropA,
                    easingFuncA,
                    cbOnCompleteA,
                    bLoopA ? eTweenLoopMode::YOYO : eTweenLoopMode::ONCE,
                    bLoopA ? -1 : 0,
                    nullptr,
                    {})
    {
    }
};

typedef StaticPoolList<SelfTween>  TweensPooledList;
typedef TweensPooledList::iterator TweenItter;

struct TweenHandle
{
    int        nCounter = 0;
    TweenItter it;

    bool operator!() const
    {
        return nCounter == 0;
    }

    explicit operator bool() const
    {
        return nCounter != 0;
    }
};

class CContainer : public std::enable_shared_from_this<CContainer>
{
    friend class CGfx;
    friend class BaseDialog;
    friend class FightView;
    friend class RenderTracker;
    friend class TutorialController;
    friend class HTMLSelectPopup;
private:
    int                 _nTutorId               = -1;
    bool                _bReadyForTutorial      = true;
    TweensPooledList    _lTweens;
    Point               _ptLocalDragIconAdjust  = {0.f, 0.f};
    Point               _ptScreenDragIconAdjust = {0.f, 0.f};

protected:
    class RenderTracker
    {
        friend class CContainer;

    private:
        static inline std::stack<std::tuple<bool, eMouseCursorType>> _stack        = {};
        static inline bool                                           _bTrackRender = false;

    private:
        inline static bool isTracking       (void) { return _bTrackRender;}
                      void init             (bool                   bTrack, 
                                             eMouseCursorType       eCur)
        {
            _stack.push(std::make_tuple(_bTrackRender, CContainer::_eDefaultCursorType));
            set(bTrack, eCur);
        }

    public:
                            RenderTracker   (const RenderTracker&) = delete;
                      void  operator =      (const RenderTracker&) = delete;

                            RenderTracker   (void) { init(true, eMouseCursorType::E_MCT_POINTER);}

                            RenderTracker   (bool                   bTrack, 
                                             eMouseCursorType       eCur) { init(bTrack, eCur); }

                       void set             (bool                   bTrack = false, 
                                             eMouseCursorType       eCur = eMouseCursorType::E_MCT_NORMAL) { _bTrackRender = bTrack; }

                            ~RenderTracker  (void);
    };

protected:
    struct ScrollAnimation
    {
        bool            active      = false;
        float           from        = 0.f;
        float           to          = 0.f;
        float           time        = 0.f;
        float           duration    = 0.f;
        easingFunction  easing      = Easing::outExpo;
        SimpleCallback  onComplete;
    };

protected:
    inline static CContainerPtrs   _vRendered;
    LightEmmiterSettings           _lightEmmiterSettings;
    GPULights*                     _pvCollectedLight = nullptr;
    CContainerPtrs                 _vChildren;
    CContainerPtrs                 _vSaved;
    float                          _fRotate             = 0;
    float                          _fx                  = 0;
    float                          _fy                  = 0;
    float                          _fxT                 = 0;
    float                          _fyT                 = 0;
    float                          _fScaleX             = 1.f;
    float                          _fScaleY             = 1.f;
    float                          _fScaleMul           = 1.f;
    float                          _rotateCenterX       = 0;
    float                          _rotateCenterY       = 0;

    bool                           _isVisble            = true;
    float                          _RGBA[4]             = { 1.f, 1.f, 1.f, 1.f };
    float                          _GS[4]               = { .5f, .5f, .5f, .5f };
    bool                           _bIsGs               = false;

    CContainer*                    _pParent             = NULL;
    bool                           _bIgonoreTutorBox    = false;
    bool                           _saveVec             = false;
    bool                           _isVecSaved          = false;
    float                          _leftTop[3]          = {0, 0, 1.f};
    float                          _rightBot[3]         = {0, 0, 1.f};
    Rect                           _innerBounds;
    bool                           _boundsCalced        = false;

    Rect                           _scrollBox;
    Rect                           _origScrollBox;
    bool                           _hasScrollBox        = false;
    bool                           _scissorSet          = false;
    float                          _fScroll             = 0;
    float                          _fMaxScroll          = 0;
    float                          _fLightMul           = 1.f;
    bool                           _bPixelSnap          = false;

    ScrollAnimation                _scrollAnim;
    SimpleCallback                 _onScrollChanged;

    int64_t                        _id                  = 0;
    int64_t                        _tag                 = 0;
    int64_t                        _typeTag             = 0;
    uint32_t                       _nSysId              = 0;

    eScrollType                    _eScrollType         = eScrollType::E_ST_NONE;
    eMouseCursorType               _eMouseCursor        = eMouseCursorType::E_MCT_NORMAL;

    SimpleCallback                 _onClickCb;
    SimpleCallback                 _onPointerDownCb;
    SimpleCallback                 _onPointerUpCb;
    SimpleCallback                 _onHoverCb;
    SimpleCallback                 _onLeaveCb;
    FncOnPointerMove               _onPointerMoveCb;

    eElementClass                  _eElementClass       = E_EL_BASIC;
    size_t                         _tweensRunning       = 0;
    CContainerPtrs                 _vChildrenIterating;
    bool                           _bIsUpdateInvisible  = false;
    CContainer*                    _pDialog             = nullptr;

    bool                           _dragable            = false;
    CContainerWPtr                 _ptrDefaultDropAcceptor;
    fOnDragBegin                   _onDragBeginCb;
    fOnDragEvt                     _onDragMoveCb;
    fOnDragEvt                     _onDragEndCb;
    FOnDraggingObjDropped          _onDragDropped;
    FOnDragDropAccept              _onDragDropAccept;
    FOnDraggingOver                _onDragOver;
    SimpleCallback                 _onDraggedOutOfDialog;

    Point                          _ptLocalDragOffset;
    Point                          _ptScreenDragOffset;

    bool                           _bTweenUpdateRunning = false;
    bool                           _bTutorialIgnored    = false;
    bool                           _bIsInterface        = false;
    bool                           _bExcludeFromUpdate  = false;
    bool                           _bSkipLight          = true;
    int                            _lightLayer = 0;
    FOnLightCollected              _onLightCollectedCb;
    bool                           _bIsLightEmissionEnabled = true;

    inline static bool                           _isEraseHandlerSet = false;
    inline static std::map<CContainerPtr, bool>  _mapInteractivePushed;
    dragTargetHighLightPrevState                 _dragHLStateSaved;
    inline static Rect                           _rcGlobalClip;
    inline static Rect                           _rcCurrentClip;
    static eMouseCursorType                      _eDefaultCursorType;

public:
    bool                  _bCalcTransNoRender = false;

protected:
    static void                 setDefaultMouseCursor           (eMouseCursorType           eMCTDefault = eMouseCursorType::E_MCT_NORMAL);

private:
    auto                        getInitialTweenItter            (void) { return _lTweens.end(); }
    inline static void          clearTrack                      (void) { _vRendered.clear(); }

public:
    TweenHandle                 getInitialTweenHandle           (void)
    {
        TweenHandle th;
        th.it = getInitialTweenItter();
        th.nCounter = 0;
        return th;
    }

    void                        addSelfTween                    (TweenHandle*               ptrHandleHolder,
                                                                 eTweenProp                 eProp,
                                                                 float                      fFrom,
                                                                 float                      fTo,
                                                                 float                      fDurring,
                                                                 easingFunction             easing = Easing::linear,
                                                                 float                      fDelay = 0.f,
                                                                 SimpleCallback             cbOnComplete = {});
    void                        addSelfTween                    (eTweenProp                 eProp,
                                                                 float                      fFrom,
                                                                 float                      fTo,
                                                                 float                      fDurring,
                                                                 easingFunction             easing = Easing::linear,
                                                                 float                      fDelay = 0.f,
                                                                 SimpleCallback             cbOnComplete = {},
                                                                 TweenHandle*               ptrHandleHolder = nullptr,
                                                                 bool                       bLoop = false);
    void                        addSelfTweenEx                  (eTweenProp                 eProp,
                                                                 float                      fFrom,
                                                                 float                      fTo,
                                                                 float                      fDurring,
                                                                 easingFunction             easing = Easing::linear,
                                                                 float                      fDelay = 0.f,
                                                                 SimpleCallback             cbOnComplete = {},
                                                                 TweenHandle*               ptrHandleHolder = nullptr,
                                                                 eTweenLoopMode             eLoopMode = eTweenLoopMode::ONCE,
                                                                 int                        nRepeatCount = 0,
                                                                 easingFunction             easingBack = nullptr,
                                                                 SimpleCallback             cbOnLoopComplete = {});
    void                        addSelfTweenYoyo                (eTweenProp                 eProp,
                                                                 float                      fFrom,
                                                                 float                      fTo,
                                                                 float                      fDurring,
                                                                 easingFunction             easing = Easing::linear,
                                                                 easingFunction             easingBack = nullptr,
                                                                 float                      fDelay = 0.f,
                                                                 SimpleCallback             cbOnComplete = {},
                                                                 TweenHandle*               ptrHandleHolder = nullptr,
                                                                 int                        nFullCycles = -1,
                                                                 SimpleCallback             cbOnLoopComplete = {});
    void                        removeSelfTween                 (TweenHandle&               th);

    void                        setTimeout                      (float                      fTime, 
                                                                 SimpleCallback             cb);
    void                        removeSelfTweens                (void);
    void                        forceUpdate                     (float                      dt);

protected:
    void                        updateSelfTweens                (float                      dt);
    void                        updateScrollAnimation           (float                      dt);
    void                        setScrollInternal               (float                      fValue);
    void                        notifyScrolled                  (void);

public: 
    virtual void                update                          (float                      dt);
    virtual void                renderSelf                      (float                      dt, 
                                                                 float*                     rgba);
            void                render                          (float                      dt, 
                                                                 float*                     rgba);

public:
                                CContainer                      (void);
    virtual                    ~CContainer                      (void);
    Point                       screenToLocal                   (float                      fScreenX, 
                                                                 float                      fScreenY);
    void                        setCalcTransforms               (void) { _bCalcTransNoRender = true; }
    void                        setExcludeFromUpdate            (bool                       bExclude) { _bExcludeFromUpdate = bExclude; }
    bool                        isExcludedFromUpdate            (void) { return _bExcludeFromUpdate; }
    virtual float               getCx                           (bool                       bRecursive = false);
    virtual float               getCy                           (bool                       bRecursive = false);
    virtual bool                getNotTransBounds               (Rect*                      p);
    virtual void                calcNotTransBounds              (Rect*                      p);
    virtual bool                getTransBounds                  (Rect*                      p);
    virtual bool                getInteractiveBounds            (Rect*                      p);
    virtual bool                containsPoint                   (float                      x, 
                                                                 float                      y);
    virtual bool                isOffScene                      (void);
    bool                        calcInteractiveBounds           (Rect*                      p, 
                                                                 bool&                      bFirstUnite);

    CContainerPtr               getRoot                         (void);
    bool                        isInRenderTree                  (void);

    void                        setId                           (int64_t                    id = 1) { _id = id; }
    int64_t                     getId                           (void) { return _id; }
    void                        setTag                          (int64_t                    tag) { _tag = tag; }
    int64_t                     getTag                          (void) { return _tag; }
    void                        setTypeTag                      (int64_t                    tag) { _typeTag = tag; }
    int64_t                     getTypeTag                      (void) { return _typeTag; }
    CContainer*                 getAncestorById                 (int64_t                    id, 
                                                                 bool                       testSelf = false);
    CContainer*                 getAncestorByAnyId              (bool                       testSelf = false);
    bool                        hasAncestor                     (CContainer*                pAnsestor, 
                                                                 bool                       bCheckSelf = false, 
                                                                 bool                       bStopAtFirstDialog = true);
    CContainerPtr               getParentDialog                 (void);
    CContainerPtr               getParentModalDialog            (void);
    float                       calcScreenX                     (float                      fAdd = 0.f);
    float                       calcScreenY                     (float                      fAdd = 0.f);

    void                        setScrollBox                    (Rect*                      p = nullptr, 
                                                                 eScrollType                eScroll = eScrollType::E_ST_HOR);
    const Rect&                 getScrollBox                    (void) { return _scrollBox; }
    void                        scrollToChild                   (CContainerPtr              ptrChild);
    void                        scrollToChild                   (int                        nChildIdx);
    void                        updateMaxScroll                 (void);
    void                        calcAncestorsScale              (float&                     fOutX, 
                                                                 float&                     fOutY);
    void                        updateScrollBox                 (void);
    float                       scrollTo                        (float                      fScrollTo, 
                                                                 float                      fBump = 0);
    bool                        isOverScroll                    (float*                     pfScrollBack = nullptr);
    bool                        willOverScroll                  (float                      fScrollTo);
    float                       getOverScroll                   (float                      fScrollTo);
    float                       getScroll                       (void) { return _fScroll; }
    float                       getMaxScroll                    (void) { return _fMaxScroll; }
    void                        setOnScrollChanged              (SimpleCallback             cb) { _onScrollChanged = cb; }
    void                        stopScrollAnimation             (void);
    bool                        isScrollAnimating               (void) const { return _scrollAnim.active; }
    void                        animateScrollTo                 (float                      fScrollTo,
                                                                 float                      fDuration,
                                                                 easingFunction             ef = Easing::outExpo,
                                                                 SimpleCallback             onComplete = nullptr);
    void                        flingScrollByDelta              (float                      fDelta);
    void                        settleScroll                    (void);
    float                       getClampedScroll                (float                      fValue) const;
    bool                        _calcBounds                     (CMatrixStack*              pMs, 
                                                                 Rect&                      rcOut, 
                                                                 bool                       calcTransforms = false, 
                                                                 CContainerPtr              stopAt = nullptr);
    void                        calcBounds                      (Rect&                      rcOut, 
                                                                 bool                       calcTransforms = false, 
                                                                 CContainerPtr              stopAt = nullptr);
    void                        calcTransforms                  (CMatrixStack*              pMatStack);
    Rect&                       getInnerBounds                  (bool                       alwaysRecalc = false, 
                                                                 bool                       recalcTransforms = false);
    float                       getNotTransCx                   (void);
    float                       getNotTransCy                   (void);
    float                       calcNotTransCx                  (void);
    float                       calcNotTransCy                  (void);

    void                        pushSetSaveTransCoordsRecur     (bool                       bIsInteractive, 
                                                                 bool                       bIsInitialCall = true);
    void                        popSetSaveTransCoordsRecur      (void);
    void                        setSaveTransCoords              (bool                       bIsSave = true) { _saveVec = bIsSave; }
    bool                        isSaveTransCoords               (void) { return _saveVec; }
    void                        setInteractive                  (bool                       bSetId = false);
    void                        getWorldScale                   (float&                     fOutX, 
                                                                 float&                     fOutY);
    CContainerPtr               findChildAtPos                  (float                      x, 
                                                                 float                      y, 
                                                                 bool                       visibilityCheck = false);
    CContainerPtr               getChildAt                      (int                        idx);
    CContainerPtr               getLastChild                    (void);
    bool                        isVisible                       (void) { return _isVisble; }
    virtual void                setVisible                      (bool                       bVisible);
    void                        setParent                       (CContainer*                pParent);
    CContainer*                 getParent                       (void) { return _pParent; }
    size_t                      getChildrenCount                (void) { return _vChildren.size(); }
    auto&                       getChildren                     (void) { return _vChildren; }
    void                        getAncestors                    (Containers&                vOut);
    int                         getChildIndex                   (CContainerPtr              pChild);
    int                         getChildIndexP                  (CContainer*                pChild);
    CContainerPtr               getTopMostChild                 (void);
    void                        bringChildToFront               (CContainer*                pChildA);
    void                        bringChildToBack                (CContainer*                pChildA);
    void                        bringChildUnder                 (CContainerPtr              pChildA, 
                                                                 CContainerPtr              pChildUnder);

    void                        saveChildrenZOrder              (void);
    void                        restoreChildrenZOrder           (void);
    bool                        swapChildren                    (CContainerPtr              pChildA, 
                                                                 CContainerPtr              pChildB);
    bool                        swapChildrenP                   (CContainer*                pChildA, 
                                                                 CContainer*                pChildB);
    void                        addChild                        (CContainerPtr              pChild);
    void                        addChildAt                      (CContainerPtr              pChild, 
                                                                 size_t                     nAt);
    void                        addChildAfter                   (CContainerPtr              pChild, 
                                                                 CContainerPtr              pAfter);
    void                        reserveChildren                 (int                        nNum);
    bool                        removeChildP                    (CContainer*                pChild);
    bool                        removeChild                     (CContainerPtr              pChild);
    bool                        removeChildAt                   (size_t                     nAt);
    void                        removeAll                       (void);
    void                        removeLast                      (void);
    void                        removeFromParent                (void);
    void                        rotate                          (float                      fRadians);
    float                       getRotate                       (void) { return _fRotate; }
    void                        scaleToFit                      (float                      fCx, 
                                                                 float                      fCy);
    void                        scaleToFitCy                    (float                      fCy);
    virtual void                setScale                        (float                      sx, 
                                                                 float                      sy);
    void                        setScaleTo                      (float                      fToCx, 
                                                                 float                      fToCy);
    void                        scaleCx                         (float                      fToCx);
    float                       setScaleToMaxSize               (float                      fMaxCx, 
                                                                 float                      fMaxCy);
    void                        setScaleToCx                    (float                      fToCx, 
                                                                 bool                       bCalc = false);
    void                        setScaleToY                     (float                      fToCy);
    void                        setScaleMul                     (float                      fMul) { _fScaleMul = fMul; }
    float                       getScaleMul                     (void) { return _fScaleMul; }
    virtual void                setPos                          (float                      fx, 
                                                                 float                      fy);
    void                        setPos                          (Point                      pt) { setPos(pt.x, pt.y); }
    virtual void                addPos                          (float                      fAddX, 
                                                                 float                      fAddY);
    void                        addX                            (float                      fAddX);
    void                        addY                            (float                      fAddY);
    void                        offsetChildren                  (float                      fAddX, 
                                                                 float                      fAddY);

    void                        setPosCentered                  (float                      fParentCx, 
                                                                 float                      fParentCy, 
                                                                 float                      fAddX = 0.f, 
                                                                 float                      fAddY = 0.f);
    void                        setPosPivotCentered             (float                      fParentCx, 
                                                                 float                      fParentCy, 
                                                                 float                      fAddX = 0.f, 
                                                                 float                      fAddY = 0.f);
    void                        setXPosCentered                 (float                      fParentCx, 
                                                                 float                      fAddX = 0.f, 
                                                                 bool                       bCalc = false);
    void                        setYPosCentered                 (float                      fParentCy, 
                                                                 float                      fAddY = 0.f);

    void                        highlightAsDragTarget           (float                      fHlScale = 1.2f, 
                                                                 bool                       bHighLight = true);

    void                        packChildrenCascade             (float                      maxCx, 
                                                                 float                      maxCy, 
                                                                 float                      fPad = 5.0f, 
                                                                 bool                       bDistribute = false);
    void                        packChildrenGuillotine          (float                      maxCx, 
                                                                 float                      maxCy, 
                                                                 float                      fPad);
    void                        alignChildren                   (eChildrenAlign             eAlign, 
                                                                 float                      fPad, 
                                                                 float                      fMaxCx = 0.f, 
                                                                 bool                       bCenter = true);
    static void                 makeKeyValue                    (CContainerPtr              ptrKeys, 
                                                                 CContainerPtr              ptrVals, 
                                                                 float                      fPad, 
                                                                 float                      fMaxCx);
    static void                 calcGlobalScreenClip            (Rect&                      rc);
    static void                 updateGlobalClip                (void);
    static void                 setGlobalClip                   (Rect*                      pClipRc);

    void                        setIgnoreTutorialBox            (bool                       bIgnore) { _bIgonoreTutorBox = bIgnore; }
    void                        getPos                          (Point*                     pos);
    float                       getX                            (void) { return _fx; }
    float                       getY                            (void) { return _fy; }
    float                       getTransX                       (void);
    float                       getTransY                       (void);

    virtual void                setX                            (float                      x);
    virtual void                setY                            (float                      y);
    virtual void                setScaleX                       (float                      fScale);
    virtual void                setScaleY                       (float                      fScale);

    void                        flipX                           (void) { _fScaleX *= -1.f; }
    void                        flipY                           (void) { _fScaleY *= -1.f; }

    bool                        isFlippedX                      (void) { return _fScaleX < 0; }
    bool                        isFlippedY                      (void) { return _fScaleY < 0;}

    float                       getScaleX                       (void) { return _fScaleX; }
    float                       getScaleY                       (void) { return _fScaleY; }
    float                       getScaleXAbs                    (void) { return fabs(_fScaleX); }
    float                       getScaleYAbs                    (void) { return fabs(_fScaleY);}
    void                        setPivot                        (float                      fx, 
                                                                 float                      fy) { _rotateCenterX = fx; _rotateCenterY = fy; }
    void                        setPivotCentered                (void) { setPivot(-calcNotTransCx() / 2.f, -calcNotTransCy() / 2.f); }
    void                        setPivotCenterBottom            (void) { setPivot(-calcNotTransCx() / 2.f, -calcNotTransCy()); }
    void                        setPivotCenterTop               (void) { setPivot(-calcNotTransCx() / 2.f, calcNotTransCy()); }
    void                        setAlpha                        (float                      fA) { _RGBA[3] = fA; }
    float                       getAlpha                        (void) { return _RGBA[3]; }
    void                        setTint                         (float                      r, 
                                                                 float                      g, 
                                                                 float                      b) { _RGBA[0] = r; _RGBA[1] = g; _RGBA[2] = b; }
    auto                        getTint                         (void) { return _RGBA; }
    void                        setColor                        (float                      r,
                                                                 float                      g,
                                                                 float                      b,
                                                                 float                      a) { _RGBA[0] = r; _RGBA[1] = g; _RGBA[2] = b; _RGBA[3] = a; }

    void                        setRgba                         (uint32_t                   hexRgba)
    {
        _RGBA[0] = ((hexRgba & 0xFF000000) >> 24) / 255.f;
        _RGBA[1] = ((hexRgba & 0x00FF0000) >> 16) / 255.f;
        _RGBA[2] = ((hexRgba & 0x0000FF00) >> 8) / 255.f;
        _RGBA[3] = (hexRgba & 0x000000FF) / 255.f;
    }

    eScrollType                 getScrollType                   (void) { return _eScrollType; }
    void                        setMouseCursorType              (eMouseCursorType           eMC) { _eMouseCursor = eMC; }
    virtual eMouseCursorType    getMouseCursorType              (void);
    void                        setOnPointerDown                (SimpleCallback             cb) { _onPointerDownCb = cb; }
    void                        setOnPointerUp                  (SimpleCallback             cb) { _onPointerUpCb = cb; }
    virtual void                setOnClick                      (SimpleCallback             cb) { _onClickCb = cb; }
    void                        setOnHover                      (SimpleCallback             cb) { _onHoverCb = cb; }
    void                        setOnLeave                      (SimpleCallback             cb) { _onLeaveCb = cb; }
    void                        setOnMove                       (FncOnPointerMove           cb) { _onPointerMoveCb = cb;}
    void                        setUpdateInvisible              (bool                       bSet) { _bIsUpdateInvisible = bSet; }
    eElementClass               getClass                        (void) { return _eElementClass; }
    virtual void                setGrayScale                    (bool                       gs = true) { _bIsGs = gs; }
    void                        setDragable                     (CContainerWPtr             ptrDefaultAcceptor, 
                                                                 fOnDragBegin               onDragBeginCb);
    void                        setOnDragDropAccept             (FOnDragDropAccept          f) { _onDragDropAccept = f; }

    void                        setOnDragDropped                (FOnDraggingObjDropped      f) { _onDragDropped = f; }

    void                        setOnDragOver                   (FOnDraggingOver            f) { _onDragOver = f; }

    void                        setOnDraggedOutOfDialog         (SimpleCallback             cb) { _onDraggedOutOfDialog = cb; }
    Point                       getScreenDragOffset             (void) { return _ptScreenDragOffset; }
    void                        setDragOffset                   (float                      fX, 
                                                                 float                      fY) { _ptLocalDragOffset.x = fX; _ptLocalDragOffset.y = fY; }
    virtual bool                onDragBegin                     (int32_t                    x, 
                                                                 int32_t                    y);
    virtual bool                onDragDropAccept                (CContainerPtr              pDrop)
    {
        if (_onDragDropAccept)
            return _onDragDropAccept(pDrop);
        else
            return false;
    }

    virtual void                onDragDropped                   (float                      x, 
                                                                 float                      y, 
                                                                 CContainerPtr              pDrop)
    {
        if (_onDragDropped)
            _onDragDropped(x, y, pDrop);
    }

    virtual void                onDragOver                      (CContainerPtr              pDrop, 
                                                                 bool                       bHovered)
    {
        if (_onDragOver)
            _onDragOver(pDrop, bHovered);
    }

    virtual void                onDraggedOutOfDialog            (void)
    {
        if (_onDraggedOutOfDialog)
            _onDraggedOutOfDialog();
    }

    auto                        begin                           (void) { return _vChildren.begin(); }
    auto                        end                             (void) { return _vChildren.end(); }
    void                        setTutorialId                   (int                        nId, 
                                                                 bool                       bAddToTutorial = true);
    bool                        isReadyForTutorial              (void) { return _bReadyForTutorial; }
    void                        setReadyForTutorial             (bool                       bReady = true) { _bReadyForTutorial = bReady; }
    auto                        isTutorialIgnored               (void) { return _bTutorialIgnored; }
    void                        setTutorialIgnored              (bool                       bIgnored = true) { _bTutorialIgnored = bIgnored; }
    void                        setSkipLight                    (bool                       bSkip) { _bSkipLight = bSkip; }
    void                        setLightMul                     (float                      fMul) { _fLightMul = fMul; }
    auto                        getDefaultDropAcceptor          (void) { return _ptrDefaultDropAcceptor; }
    void                        setLightLayer                   (int                        lightLayer) { _lightLayer = lightLayer; }
    void                        setOnLightCollected             (FOnLightCollected          cb) { _onLightCollectedCb = cb; }
    void                        setLightEmmiterSettings         (LightEmmiterSettings       settings) { _lightEmmiterSettings = settings; }
    void                        enableLightEmission             (bool                       bEnable) { _bIsLightEmissionEnabled = bEnable; }
    void                        setPixelSnap                    (bool                       b){ _bPixelSnap = b; }
    void                        setPixelSnapRecur               (bool                       b)
    {
        _bPixelSnap = b;
        for (auto& c : _vChildren) c->setPixelSnapRecur(b);
    }

protected:
    virtual bool                setTweenPropValue               (eTweenProp                 eProp, 
                                                                float                       fValue);
    auto                        getTutorialId                   (void) { return _nTutorId; }
    void                        beginRender                     (float*                     rgba);
    void                        endRender                       (void);
    void                        setClass                        (eElementClass              eClass) { _eElementClass = eClass; }
    void                        onChildAdded                    (CContainer*                pChild);
    virtual void                onHover                         (void);
    virtual void                onLeave                         (void);
    virtual void                onPointerDown                   (void);
    virtual void                onPointerUp                     (void);
    virtual void                onPointerMove                   (bool                       bIsPressed, 
                                                                 int32_t                    x, 
                                                                 int32_t                    y);
    virtual void                onClick                         (void);
    virtual void                onChildrenChanged               (void);
    virtual void                collectLights                   (void){}
    virtual void                onScrolled                      (void){}
    void                        setSysId                        (uint32_t                   id) { _nSysId = id; }
    uint32_t                    getSysId                        (void) { return _nSysId; }
    CContainer*                 getAncestorBySysId              (uint32_t                   id);
    CContainer*                 getAncestorByAnySysId           (bool                       testSelf = false);
    void                        calcTotalScale                  (float&                     fOutX, 
                                                                 float&                     fOutY, 
                                                                 bool                       bSkipSelf = false);
    virtual void                applyTransform                  (CMatrixStack*              pMS, 
                                                                 bool                       bForce = false);
    void                        mRotate                         (CMatrixStack*              pMS, 
                                                                float                       fRad);
    void                        mTranslate                      (CMatrixStack*              pMS, 
                                                                 float                      fx, 
                                                                 float                      fy);
    void                        mScale                          (CMatrixStack*              pMS, 
                                                                 float                      fx, 
                                                                 float                      fy);

public:
    static CContainerPtr        findTrackedAtPoint              (float                      fx, 
                                                                 float                      fy, 
                                                                 const Rect*                pCrop = nullptr, 
                                                                 CContainer*                pEnsureAncestor = nullptr);
    static CContainerPtr        getDialogAtPoint                (float                      fx, 
                                                                 float                      fy);

public:
    void                        setDragIconAdjustLocal          (float                      x, 
                                                                 float                      y)  { _ptLocalDragIconAdjust.set(x, y); }
    void                        setDragIconAdjustLocal          (const Point&               pt) { _ptLocalDragIconAdjust = pt; }
    const Point&                getLocalDragIconAdjust          (void) const { return _ptLocalDragIconAdjust; }
    const Point&                getScreenDragIconAdjust         (void) const { return _ptScreenDragIconAdjust; }

};
_G2D_NAMESPACE_END_