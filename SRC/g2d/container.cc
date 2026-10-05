#include "container.h"
#include "matrixstack.h"
#include "sceneresize.h"
#include "m3.h"
#include "gfx.h"
#include "msched.h"
#include "dialogs/basedialog.h"
#include "tutorialcontroller.h"
#include "engine.h"

_G2D_NAMESPACE_BEGIN_

eMouseCursorType CContainer::_eDefaultCursorType = eMouseCursorType::E_MCT_NORMAL;

CContainer::RenderTracker::~RenderTracker()
{
    assert(_stack.size());
    auto state              = _stack.top();
    auto bTrackRender       = std::get<0>(state);
    auto eDefaultCursorType = std::get<1>(state);
    _stack.pop();
    set(bTrackRender, eDefaultCursorType);
}

void CContainer::setDefaultMouseCursor(eMouseCursorType eMCTDefault)
{
    CContainer::_eDefaultCursorType = eMCTDefault;
}

CContainer::CContainer()
{
    _eMouseCursor = CSprite::_eDefaultCursorType;

    if (RenderTracker::isTracking())
        setInteractive();
}

CContainer::~CContainer()
{
    assert(_tweensRunning == 0);

    _lTweens.clear();

    for (size_t i = 0; i < _vChildren.size(); i++)
        _vChildren[i]->setParent(NULL);
}

void CContainer::addSelfTween(TweenHandle* ptrHandleHolder, eTweenProp eProp, float fFrom, float fTo, float fDurring, easingFunction easing, 
    float fDelay, SimpleCallback cbOnComplete)
{
    addSelfTween(eProp, fFrom, fTo, fDurring, easing, fDelay, cbOnComplete, ptrHandleHolder);
}

void CContainer::addSelfTween(eTweenProp eProp, float fFrom, float fTo, float fDurring, easingFunction easing, float fDelay, 
    SimpleCallback cbOnComplete, TweenHandle* ptrHandleHolder, bool bLoop)
{
    _lTweens.emplace_back(fDelay, fDurring, fFrom, fTo, eProp, easing, cbOnComplete, bLoop);
    _lTweens.back().ptrHandleHolder = (void*)ptrHandleHolder;

    if (ptrHandleHolder)
    {
        assert(ptrHandleHolder->nCounter == 0);
        ptrHandleHolder->nCounter++;
        ptrHandleHolder->it = _lTweens.back_itter();
        assert(ptrHandleHolder->it != _lTweens.end());
    }
}
void CContainer::addSelfTweenEx(eTweenProp eProp, float fFrom, float fTo, float fDurring, easingFunction easing, float fDelay, SimpleCallback cbOnComplete, 
    TweenHandle* ptrHandleHolder, eTweenLoopMode eLoopMode, int nRepeatCount, easingFunction easingBack, SimpleCallback cbOnLoopComplete)
{
    if (eLoopMode == eTweenLoopMode::ONCE)
        nRepeatCount = 0;
    else if (nRepeatCount < -1)
        nRepeatCount = -1;

    _lTweens.emplace_back(fDelay,
                          fDurring,
                          fFrom,
                          fTo,
                          eProp,
                          easing,
                          cbOnComplete,
                          eLoopMode,
                          nRepeatCount,
                          easingBack,
                          cbOnLoopComplete);

    _lTweens.back().ptrHandleHolder = (void*)ptrHandleHolder;

    if (ptrHandleHolder)
    {
        assert(ptrHandleHolder->nCounter == 0);
        ptrHandleHolder->nCounter++;
        ptrHandleHolder->it = _lTweens.back_itter();
        assert(ptrHandleHolder->it != _lTweens.end());
    }
}

void CContainer::addSelfTweenYoyo(eTweenProp eProp, float fFrom, float fTo, float fDurring, easingFunction easing, easingFunction easingBack,
                                  float fDelay, SimpleCallback cbOnComplete, TweenHandle* ptrHandleHolder, int nFullCycles, SimpleCallback cbOnLoopComplete)
{
    int nRepeatCount = 0;

    if (nFullCycles < 0)
    {
        nRepeatCount = -1;
    }
    else if (nFullCycles > 0)
    {
        nRepeatCount = nFullCycles * 2 - 1;
    }
    else
    {
        nRepeatCount = 0;
    }

    addSelfTweenEx(eProp,
                   fFrom,
                   fTo,
                   fDurring,
                   easing,
                   fDelay,
                   cbOnComplete,
                   ptrHandleHolder,
                   eTweenLoopMode::YOYO,
                   nRepeatCount,
                   easingBack,
                   cbOnLoopComplete);
}

void CContainer::removeSelfTween(TweenHandle& th)
{
    assert(th.nCounter > 0);

    if (th.nCounter > 0)
    {
        if (!_bTweenUpdateRunning)
        {
            if (th.it != _lTweens.end())
            {
                auto ptrHolder = (TweenHandle*)th.it->data.ptrHandleHolder;
                _lTweens.erase(th.it);

                if (ptrHolder)
                {
                    ptrHolder->nCounter--;
                    assert(ptrHolder->nCounter > -1);
                    ptrHolder->it = getInitialTweenItter();
                }
            }
        }
        else if (th.it != _lTweens.end())
        {
            th.it->data.bToErase        = true;
            th.it->data.bNoCallBack     = true;
            th.it->data.ptrHandleHolder = nullptr;
            th.nCounter = 0;
            th.it = getInitialTweenItter();
        }
    }
}

void CContainer::setInteractive(bool bSetId)
{
    if (bSetId && !getId() && !getSysId())
        setId();

    setSaveTransCoords();
}

void CContainer::getAncestors(Containers& vOut)
{
    if (_pParent)
    {
        vOut.push_back(_pParent);
        _pParent->getAncestors(vOut);
    }
}

void CContainer::calcTotalScale(float& fOutX, float& fOutY, bool bSkipSelf)
{
    if (_pParent)
        _pParent->calcTotalScale(fOutX, fOutY);

    if (!bSkipSelf)
    {
        fOutX *= _fScaleX;
        fOutY *= _fScaleY;
    }
}

float CContainer::calcScreenX(float fAdd)
{
    static Containers v;
    v.clear();

    float fRes = 0;
    float fMul = 1.f;

    getAncestors(v);

    for (int i = v.size() - 1; i > -1; i--)
    {
        fRes += v[i]->getX() * fMul;
        fMul *= v[i]->getScaleX();
    }

    fRes += (getX() + fAdd) * fMul;

    return fRes;
}

float CContainer::calcScreenY(float fAdd)
{
    static Containers v;
    v.clear();

    float fRes = 0;
    float fMul = 1.f;

    getAncestors(v);

    for (int i = v.size() - 1; i > -1; i--)
    {
        fRes += v[i]->getY() * fMul;
        fMul *= v[i]->getScaleY();
    }

    fRes += (getY() + fAdd) * fMul;

    return fRes;
}

float CContainer::getOverScroll(float fScrollTo)
{
    float fRes = 0;

    if (isless(fScrollTo, _fMaxScroll))
        fRes = fScrollTo - _fMaxScroll;
    else if (isgreater(fScrollTo, 0.f))
        fRes = fScrollTo;

    return fRes;
}

bool CContainer::willOverScroll(float fScrollTo)
{
    return (isless(fScrollTo, _fMaxScroll) || isgreater(fScrollTo, 0.f));
}

bool CContainer::isOverScroll(float* pfScrollBack)
{
    bool bRes = false;

    if (isless(_fScroll, _fMaxScroll))
    {
        bRes = true;

        if (pfScrollBack)
            *pfScrollBack = _fMaxScroll;
    }
    else if (isgreater(_fScroll, 0.f))
    {
        bRes = true;

        if (pfScrollBack)
            *pfScrollBack = 0.f;
    }

    return bRes;
}

float CContainer::getClampedScroll(float fValue) const
{
    return std::max(_fMaxScroll, std::min(0.f, fValue));
}

void CContainer::notifyScrolled()
{
    onScrolled();

    if (_onScrollChanged)
        _onScrollChanged();
}

inline float snapToPixel(float v)
{
    auto pixelScale = CGfx::getInstance()->getDpr();
    return std::floor(v * pixelScale + 0.5f) / pixelScale;
}

void CContainer::setScrollInternal(float fValue)
{
    fValue = snapToPixel(fValue);
    if (_fScroll == fValue)
        return;

    _fScroll = fValue;
    notifyScrolled();
}

void CContainer::stopScrollAnimation()
{
    _scrollAnim.active = false;
    _scrollAnim.onComplete = nullptr;
}

void CContainer::animateScrollTo(float              fScrollTo,
                                 float              fDuration,
                                 easingFunction     ef,
                                 SimpleCallback     onComplete)
{
    if (!_hasScrollBox || _eScrollType == eScrollType::E_ST_NONE)
    {
        if (onComplete)
            onComplete();

        return;
    }

    stopScrollAnimation();

    fScrollTo = getClampedScroll(fScrollTo);

    if (fDuration <= 0.f || fabs(_fScroll - fScrollTo) < 0.01f)
    {
        setScrollInternal(fScrollTo);

        if (onComplete)
            onComplete();

        return;
    }

    _scrollAnim.active      = true;
    _scrollAnim.from        = _fScroll;
    _scrollAnim.to          = fScrollTo;
    _scrollAnim.time        = 0.f;
    _scrollAnim.duration    = fDuration;
    _scrollAnim.easing      = ef ? ef : Easing::outExpo;
    _scrollAnim.onComplete  = std::move(onComplete);
}

void CContainer::updateScrollAnimation(float dt)
{
    if (!_hasScrollBox || !_scrollAnim.active)
        return;

    _scrollAnim.time += dt;

    float fPercent = (_scrollAnim.duration > 0.f)
        ? (_scrollAnim.time / _scrollAnim.duration)
        : 1.f;

    if (fPercent > 1.f)
        fPercent = 1.f;

    easingFunction fnc = _scrollAnim.easing ? _scrollAnim.easing : Easing::outExpo;

    float fVal = _scrollAnim.from + (_scrollAnim.to - _scrollAnim.from) * fnc(fPercent);

    setScrollInternal(fVal);

    if (fPercent >= 1.f)
    {
        SimpleCallback cb = std::move(_scrollAnim.onComplete);

        _scrollAnim.active = false;
        _scrollAnim.onComplete = nullptr;

        if (cb)
            cb();

        if (!_scrollAnim.active)
        {
            float fBack = 0.f;

            if (isOverScroll(&fBack))
            {
                animateScrollTo(fBack, 0.12f, Easing::outExpo);
            }
            else
            {
                float fClamped = getClampedScroll(_fScroll);

                if (fClamped != _fScroll)
                    setScrollInternal(fClamped);
            }
        }
    }
}

void CContainer::settleScroll()
{
    if (!_hasScrollBox || _eScrollType == eScrollType::E_ST_NONE)
        return;

    float fBack = 0.f;

    if (isOverScroll(&fBack))
    {
        float fDuration = Engine::getCfg().SCROLL_BACK_TIME;

        if (fDuration <= 0.f)
        {
            float fDist = fabs(fBack - _fScroll);
            fDuration = fDist / (fDist + 80.f);
        }

        animateScrollTo(fBack, fDuration, Easing::outExpo);
    }
    else
    {
        stopScrollAnimation();

        float fClamped = getClampedScroll(_fScroll);

        if (fClamped != _fScroll)
            setScrollInternal(fClamped);
    }
}

void CContainer::flingScrollByDelta(float fDelta)
{
    if (!_hasScrollBox || _eScrollType == eScrollType::E_ST_NONE)
        return;

    if (isOverScroll())
    {
        settleScroll();
        return;
    }

    float fBase = _scrollAnim.active ? _scrollAnim.to : _fScroll;

    float fTarget = getClampedScroll(fBase - fDelta);
    float fDist   = fabs(fTarget - _fScroll);

    if (fDist < 0.5f)
    {
        stopScrollAnimation();
        setScrollInternal(fTarget);
        return;
    }

    float fDuration = fDist / (fDist + 80.f);
    fDuration = std::max(0.05f, std::min(0.8f, fDuration));

    animateScrollTo(fTarget, fDuration, Easing::outExpo);
}

float CContainer::scrollTo(float fScrollTo, float fBump)
{
    float fRes = 0;

    stopScrollAnimation();

    fScrollTo = std::max(_fMaxScroll - fBump, fScrollTo);
    fScrollTo = std::min(0.f + fBump, fScrollTo);

    if (fScrollTo < _fMaxScroll)
        fRes = _fMaxScroll;

    if (fScrollTo > 0)
        fRes = 0;

    setScrollInternal(fScrollTo);

    return fRes;
}

void CContainer::scrollToChild(int nChildIdx)
{
    assert(nChildIdx > -1 && nChildIdx < _vChildren.size());

    if (nChildIdx > -1 && nChildIdx < _vChildren.size())
    {
        CContainerPtr ptrChild = _vChildren[nChildIdx];
        scrollToChild(ptrChild);
    }
}

void CContainer::scrollToChild(CContainerPtr ptrChild)
{
    Rect rc;
    if (!ptrChild->getNotTransBounds(&rc))
        ptrChild->calcNotTransBounds(&rc);

    float sx  = ptrChild->getScaleX();
    float sy  = ptrChild->getScaleY();
    float pvx = ptrChild->_rotateCenterX;
    float pvy = ptrChild->_rotateCenterY;

    rc.cx *= sx;
    rc.cy *= sy;
    rc.x   = (rc.x + pvx) * sx - pvx + ptrChild->getX();
    rc.y   = (rc.y + pvy) * sy - pvy + ptrChild->getY();

    float fScrollTo = 0.f;
    switch (_eScrollType)
    {
    case eScrollType::E_ST_HOR:
    {
        float childLeft  = rc.x + _fScroll;
        float childRight = rc.right() + _fScroll;
        float viewLeft   = _origScrollBox.x;
        float viewRight  = _origScrollBox.right();
        if (childRight > viewRight)
            fScrollTo = _fScroll - (childRight - viewRight);
        else if (childLeft < viewLeft)
            fScrollTo = _fScroll + (viewLeft - childLeft);
        else
            return;
        break;
    }
    case eScrollType::E_ST_VERT:
    {
        float childTop    = rc.y + _fScroll;
        float childBottom = rc.bottom() + _fScroll;
        float viewTop     = _origScrollBox.y;
        float viewBottom  = _origScrollBox.bottom();
        if (childBottom > viewBottom)
            fScrollTo = _fScroll - (childBottom - viewBottom);
        else if (childTop < viewTop)
            fScrollTo = _fScroll + (viewTop - childTop);
        else
            return;
        break;
    }
    default:
        assert(false);
        break;
    }
    fScrollTo = std::max(_fMaxScroll, fScrollTo);
    fScrollTo = std::min(0.f, fScrollTo);
    scrollTo(fScrollTo);
}

void CContainer::updateMaxScroll()
{
    if (_hasScrollBox)
    {
        Rect rc;
        calcNotTransBounds(&rc);

        float viewCx       = rc.cx;
        float viewCy       = rc.cy;
        float fScrollBoxCx = _origScrollBox.cx;
        float fScrollBoxCy = _origScrollBox.cy;

        switch (_eScrollType)
        {
            case eScrollType::E_ST_HOR:
                _fMaxScroll = fScrollBoxCx - viewCx;
                break;

            case eScrollType::E_ST_VERT:
                _fMaxScroll = fScrollBoxCy - viewCy;
                break;

            default:
                assert(false);
                break;
        }

        if (_fMaxScroll > 0)
            _fMaxScroll = 0;

        scrollTo(_fScroll);
    }
}

void CContainer::calcAncestorsScale(float& fOutX, float& fOutY)
{
    static Containers v;
    v.clear();

    getAncestors(v);

    float fMulX = 1.f;
    float fMulY = 1.f;

    for (size_t i = 0; i < v.size(); i++)
    {
        fMulX *= v[i]->getScaleX();
        fMulY *= v[i]->getScaleY();
    }

    fOutX = fMulX;
    fOutY = fMulY;
}

void CContainer::updateScrollBox()
{
    if (_hasScrollBox)
    {
        stopScrollAnimation();
        _fScroll = 0;
        updateMaxScroll();
    }
}

void CContainer::setScrollBox(Rect* p, eScrollType eScroll)
{
    stopScrollAnimation();

    if (p && eScroll != E_ST_NONE)
    {
        _eScrollType = eScroll;
        _hasScrollBox = true;

        _origScrollBox.copyFrom(p);
        _scrollBox.copyFrom(p);

        updateScrollBox();
    }
    else
    {
        _eScrollType  = eScrollType::E_ST_NONE;
        _hasScrollBox = false;
    }
}

void CContainer::calcTransforms(CMatrixStack* pMatStack)
{
}

bool CContainer::_calcBounds(CMatrixStack* pMs, Rect& rcOut, bool calcTransforms, CContainerPtr stopAt)
{
    if (calcTransforms)
    {
        pMs->save();
        applyTransform(pMs, true);
    }

    bool bRes = getTransBounds(&rcOut);
    Rect rcChild;

    for (size_t i = 0; i < _vChildren.size(); i++)
    {
        rcChild.set(0xFFFF, 0xFFFF, -0xFFFF, -0xFFFF);

        if (_vChildren[i]->_calcBounds(pMs, rcChild, calcTransforms))
        {
            rcOut.unite(&rcChild);
            bRes = true;

            if (_vChildren[i] == stopAt)
                break;
        }
    }

    if (calcTransforms)
        pMs->restore();

    return bRes;
}

void CContainer::calcBounds(Rect& rcOut, bool calcTransforms, CContainerPtr stopAt)
{
    static CMatrixStack ms;

    assert(!stopAt || stopAt->getParent() == this);

    if (!stopAt || stopAt->getParent() == this)
    {
        ms.clear();
        rcOut.set(0xFFFF, 0xFFFF, -0xFFFF, -0xFFFF);

        if (calcTransforms)
        {
            Containers v;
            getAncestors(v);

            float fWidth  = static_cast<float>(CSceneResize::getInstance()->getGameWidth());
            float fHeight = static_cast<float>(CSceneResize::getInstance()->getGameHeight());

            ms.projection(fWidth, fHeight);

            for (auto it : v | std::views::reverse)
                it->applyTransform(&ms, true);

            float vec3Initial[3] = {1.f, 1.f, 1.f};
            ms.vecMultiply(vec3Initial);

            CSceneResize::getInstance()->toGameCoords(vec3Initial[0], vec3Initial[1]);

            rcOut.x = vec3Initial[0];
            rcOut.y = vec3Initial[1];
        }

        _calcBounds(&ms, rcOut, calcTransforms, stopAt);

        if (rcOut.x > 0xFFF0 || rcOut.x < -0xF000)
            rcOut.set(0, 0, 0, 0);
    }
}

Rect& CContainer::getInnerBounds(bool alwaysRecalc, bool recalcTransforms)
{
    if (alwaysRecalc || !_boundsCalced)
    {
        calcBounds(_innerBounds, recalcTransforms);
    }

    return _innerBounds;
}

float CContainer::getCx(bool bRecursive)
{
    float fRes = 0;

    if (_saveVec && _isVecSaved)
    {
        float sceneCx = CSceneResize::getInstance()->getGameWidth();
        float left    = (1.f + _leftTop[0]) * sceneCx / 2.f;
        float right   = (1.f + _rightBot[0]) * sceneCx / 2.f;

        if (left > right)
            std::swap(left, right);

        fRes = right - left;
    }
    else
    {
        float fScaleX = 1.f;
        float fScaleY = 1.f;
        Rect rc;

        getNotTransBounds(&rc);

        if (bRecursive)
            calcTotalScale(fScaleX, fScaleY);
        else
            fScaleX = _fScaleX;

        fRes = rc.cx * fScaleX;
    }

    return fRes;
}

float CContainer::getCy(bool bRecursive)
{
    float fRes = 0;

    if (_saveVec && _isVecSaved)
    {
        float sceneCy = CSceneResize::getInstance()->getGameHeight();
        float top     = (1.f - _leftTop[1]) * sceneCy / 2.f;
        float bot     = (1.f - _rightBot[1]) * sceneCy / 2.f;

        fRes = bot - top;
    }
    else
    {
        float fScaleX = 1.f;
        float fScaleY = 1.f;
        Rect rc;

        getNotTransBounds(&rc);

        if (bRecursive)
            calcTotalScale(fScaleX, fScaleY);
        else
            fScaleY = _fScaleY;

        fRes = rc.cy * fScaleY;
    }

    return fRes;
}

float CContainer::getNotTransCx()
{
    Rect rc;
    getNotTransBounds(&rc);
    return rc.cx;
}

float CContainer::getNotTransCy()
{
    Rect rc;
    getNotTransBounds(&rc);
    return rc.cy;
}

float CContainer::calcNotTransCx()
{
    Rect rc;
    calcNotTransBounds(&rc);
    return rc.cx;
}

float CContainer::calcNotTransCy()
{
    Rect rc;
    calcNotTransBounds(&rc);
    return rc.cy;
}

void CContainer::pushSetSaveTransCoordsRecur(bool bIsInteractive, bool bIsInitialCall)
{
    if (bIsInitialCall)
        _mapInteractivePushed.clear();

    _mapInteractivePushed[shared_from_this()] = isSaveTransCoords();
    setSaveTransCoords(bIsInteractive);

    for (int i = 0; i < _vChildren.size(); i++)
    {
        _vChildren[i]->pushSetSaveTransCoordsRecur(bIsInteractive, false);
    }
}

void CContainer::popSetSaveTransCoordsRecur(void)
{
    for (auto& it : _mapInteractivePushed)
    {
        it.first->setSaveTransCoords(it.second);
    }

    _mapInteractivePushed.clear();
}

bool CContainer::getNotTransBounds(Rect* p)
{
    return false;
}

void CContainer::calcNotTransBounds(Rect* p)
{
    p->init();
    for (auto& it : _vChildren)
    {
        Rect rc;
        if (!it->getNotTransBounds(&rc))
            it->calcNotTransBounds(&rc);
        if (rc.cx > 0 || rc.cy > 0)
        {
            float sx  = it->getScaleX();
            float sy  = it->getScaleY();
            float pvx = it->_rotateCenterX;
            float pvy = it->_rotateCenterY;

            rc.cx *= sx;
            rc.cy *= sy;
            rc.x = (rc.x + pvx) * sx - pvx + it->getX();
            rc.y = (rc.y + pvy) * sy - pvy + it->getY();
            p->unite(&rc);
        }
    }
    Rect rc;
    // ВАЖНО: не unite-ить дефолтный (0,0,0,0) — он тянул начало координат
    // в union и раздувал cx/cy у каждого контейнера.
    if (getNotTransBounds(&rc) && (rc.cx > 0.f || rc.cy > 0.f))
        p->unite(&rc);
}

bool CContainer::getTransBounds(Rect* p)
{
    bool bRes = false;

    if (_isVecSaved)
    {
        float sceneCx = CSceneResize::getInstance()->getScreenWidth();
        float sceneCy = CSceneResize::getInstance()->getScreenHeight();

        float left  = (1.f + _leftTop[0]) * sceneCx / 2.f;
        float top   = (1.f - _leftTop[1]) * sceneCy / 2.f;
        float right = (1.f + _rightBot[0]) * sceneCx / 2.f;
        float bot   = (1.f - _rightBot[1]) * sceneCy / 2.f;

        if (left > right)
            std::swap(left, right);

        if (top > bot)
            std::swap(top, bot);

        p->set(left, top, right - left, bot - top);
        bRes = true;
    }

    return bRes;
}

bool CContainer::getInteractiveBounds(Rect* p)
{
    return getTransBounds(p);
}

bool CContainer::calcInteractiveBounds(Rect* p, bool& bFirstUnite)
{
    Rect res;
    bool bFoundAny = false;

    Rect rcSelf;
    if (getInteractiveBounds(&rcSelf))
    {
        if (rcSelf.cx > 0 && rcSelf.cy > 0)
        {
            res = rcSelf;
            bFoundAny = true;
        }
    }

    for (auto& it : _vChildren)
    {
        Rect rcChild;
        bool bFirstChild = true;

        if (it->calcInteractiveBounds(&rcChild, bFirstChild))
        {
            if (rcChild.cx > 0 && rcChild.cy > 0)
            {
                if (!bFoundAny)
                {
                    res = rcChild;
                    bFoundAny = true;
                }
                else
                {
                    res.unite(&rcChild);
                }
            }
        }
    }

    if (bFoundAny)
    {
        if (bFirstUnite)
        {
            *p = res;
            bFirstUnite = false;
        }
        else
        {
            p->unite(&res);
        }
    }

    return bFoundAny;
}

CContainerPtr CContainer::getChildAt(int idx)
{
    CContainerPtr res;

    assert(idx > -1 && idx < _vChildren.size());

    if (idx > -1 && idx < _vChildren.size())
    {
        res = _vChildren[idx];
    }

    return res;
}

CContainerPtr CContainer::getLastChild()
{
    CContainerPtr res;

    assert(_vChildren.size());

    if (_vChildren.size())
    {
        res = _vChildren[_vChildren.size() - 1];
    }

    return res;
}

CContainer* CContainer::getAncestorBySysId(uint32_t id)
{
    CContainer* pRes = NULL;

    if (_pParent)
    {
        if (_pParent->getSysId() == id)
            pRes = _pParent;
        else
            pRes = _pParent->getAncestorBySysId(id);
    }

    return pRes;
}

CContainer* CContainer::getAncestorByAnySysId(bool testSelf)
{
    CContainer* pRes = NULL;

    if (testSelf)
    {
        if (getSysId())
            return this;
    }

    if (_pParent)
    {
        if (_pParent->getSysId())
            pRes = _pParent;
        else
            pRes = _pParent->getAncestorByAnySysId();
    }

    return pRes;
}

CContainer* CContainer::getAncestorById(int64_t id, bool testSelf)
{
    CContainer* pRes = NULL;

    if (testSelf)
    {
        if (getId() == id)
            return this;
    }

    if (_pParent)
    {
        if (_pParent->getId() == id)
            pRes = _pParent;
        else
            pRes = _pParent->getAncestorById(id);
    }

    return pRes;
}

CContainer* CContainer::getAncestorByAnyId(bool testSelf)
{
    CContainer* pRes = NULL;

    if (testSelf)
    {
        if (getId())
            return this;
    }

    if (_pParent)
    {
        if (_pParent->getId())
            pRes = _pParent;
        else
            pRes = _pParent->getAncestorByAnyId();
    }

    return pRes;
}

bool CContainer::hasAncestor(CContainer* pAnsestor, bool bCheckSelf, bool bStopAtFirstDialog)
{
    bool bRes = false;

    if (bCheckSelf && this == pAnsestor)
        bRes = true;
    else if (getParent())
    {
        if (getParent() == pAnsestor)
            bRes = true;
        else
        {
            if (!bStopAtFirstDialog || (getParent()->getClass() != E_EL_DIALOG))
                bRes = getParent()->hasAncestor(pAnsestor, bStopAtFirstDialog);
        }
    }

    return bRes;
}

CContainerPtr CContainer::getParentDialog()
{
    if (getParent())
    {
        if (getParent()->getClass() == E_EL_DIALOG)
            return getParent()->shared_from_this();
        else
            return getParent()->getParentDialog();
    }

    return nullptr;
}

CContainerPtr CContainer::getParentModalDialog()
{
    auto ptr = getParentDialog();

    if (ptr)
    {
        auto ptrDlg = std::dynamic_pointer_cast<BaseDialog>(ptr);

        if (ptrDlg)
        {
            if (ptrDlg->isModal())
                return ptrDlg;
            else
                return ptrDlg->getParentModalDialog();
        }
        else
        {
            return nullptr;
        }
    }
    else
    {
        return nullptr;
    }
}

CContainerPtr CContainer::getRoot()
{
    if (getParent())
    {
        return getParent()->getRoot();
    }

    return shared_from_this();
}

bool CContainer::isInRenderTree(void)
{
    auto ptr = getRoot();
    return ptr->getClass() == E_EL_ROOT;
}

bool CContainer::isOffScene()
{
    bool bRes = false;

    if (_saveVec && _isVecSaved)
    {
        float sceneCx = CSceneResize::getInstance()->getScreenWidth();
        float sceneCy = CSceneResize::getInstance()->getScreenHeight();

        float left  = (1.f + _leftTop[0]) * sceneCx / 2.f;
        float top   = (1.f - _leftTop[1]) * sceneCy / 2.f;
        float right = (1.f + _rightBot[0]) * sceneCx / 2.f;
        float bot   = (1.f - _rightBot[1]) * sceneCy / 2.f;

        if (left > right)
            std::swap(left, right);

        if (top > bot)
            std::swap(top, bot);

        bRes = !CGfx::getInstance()->isRcOnScene(left, top, right, bot);
    }

    return bRes;
}

bool CContainer::containsPoint(float x, float y)
{
    bool bRes = false;

    assert(_saveVec);

    if (_saveVec && _isVecSaved)
    {
        float sceneCx = CSceneResize::getInstance()->getGameWidth();
        float sceneCy = CSceneResize::getInstance()->getGameHeight();

        float left  = (1.f + _leftTop[0]) * sceneCx / 2.f;
        float top   = (1.f - _leftTop[1]) * sceneCy / 2.f;
        float right = (1.f + _rightBot[0]) * sceneCx / 2.f;
        float bot   = (1.f - _rightBot[1]) * sceneCy / 2.f;

        if (left > right)
            std::swap(left, right);

        if (x > left && x < right && y > top && y < bot)
        {
            bRes = true;
        }
    }

    return bRes;
}

void CContainer::getWorldScale(float& fOutX, float& fOutY)
{
    fOutX *= _fScaleX;
    fOutY *= _fScaleY;

    if (_pParent)
        _pParent->getWorldScale(fOutX, fOutY);
}

CContainerPtr CContainer::findChildAtPos(float x, float y, bool visibilityCheck)
{
    CContainerPtr res;

    float sceneCx = CSceneResize::getInstance()->getGameWidth();
    float sceneCy = CSceneResize::getInstance()->getGameHeight();

    for (int i = _vChildren.size() - 1; i > -1; i--)
    {
        CContainerPtr child = _vChildren[i];

        if (!visibilityCheck || child->isVisible())
        {
            if (child->_saveVec && child->_isVecSaved)
            {
                float left  = (1.f + child->_leftTop[0]) * sceneCx / 2.f;
                float top   = (1.f - child->_leftTop[1]) * sceneCy / 2.f;
                float right = (1.f + child->_rightBot[0]) * sceneCx / 2.f;
                float bot   = (1.f - child->_rightBot[1]) * sceneCy / 2.f;

                if (left > right)
                    std::swap(left, right);

                if (x > left && x < right && y > top && y < bot)
                {
                    res = child;
                    break;
                }
            }
        }
    }

    return res;
}

void CContainer::updateSelfTweens(float dt)
{
    _bTweenUpdateRunning = true;

    for (auto it = _lTweens.begin(); it != _lTweens.end(); )
    {
        if (it->data.bToErase)
        {
            auto ptrHolder = (TweenHandle*)it->data.ptrHandleHolder;
            it = _lTweens.erase(it);

            if (ptrHolder)
            {
                ptrHolder->nCounter--;
                assert(ptrHolder->nCounter > -1);
                ptrHolder->it = getInitialTweenItter();
            }
        }
        else
        {
            ++it;
        }
    }

    for (auto it = _lTweens.begin(); it != _lTweens.end(); )
    {
        if (it->data.bToErase)
        {
            ++it;
        }
        else if (it->data.fDelay > 0.f)
        {
            it->data.fDelay -= dt;
            ++it;
        }
        else
        {
            auto& data = it->data;

            data.fTime += dt;

            bool bSegmentDone = false;
            float fPercent = 0.f;

            if (data.fDurring <= 0.f)
            {
                fPercent = 1.f;
                bSegmentDone = true;
            }
            else
            {
                fPercent = data.fTime / data.fDurring;

                if (fPercent >= 1.f)
                {
                    fPercent = 1.f;
                    bSegmentDone = true;
                }
            }

            easingFunction fnc = data.easingFunc ? data.easingFunc : Easing::linear;

            if (data.eLoopMode == eTweenLoopMode::YOYO &&
                data.bYoyoBackPhase &&
                data.easingFuncBack)
            {
                fnc = data.easingFuncBack;
            }

            float fEasing = fnc(fPercent);
            float fVal = data.fStartVal + ((data.fEndVal - data.fStartVal) * fEasing);

            bool bSet = setTweenPropValue(data.eProp, fVal);
            assert(bSet);

            if (bSegmentDone)
            {
                bool bHasMoreLoops =
                    (data.eLoopMode != eTweenLoopMode::ONCE) &&
                    (data.nLoopsLeft < 0 || data.nLoopsLeft > 0);

                if (bHasMoreLoops)
                {
                    if (data.nLoopsLeft > 0)
                        --data.nLoopsLeft;

                    if (data.eLoopMode == eTweenLoopMode::YOYO)
                    {
                        std::swap(data.fStartVal, data.fEndVal);
                        data.bYoyoBackPhase = !data.bYoyoBackPhase;
                    }
                    else if (data.eLoopMode == eTweenLoopMode::REPEAT)
                    {
                        data.fStartVal = data.fOrigStartVal;
                        data.fEndVal   = data.fOrigEndVal;
                        data.bYoyoBackPhase = false;
                    }

                    data.fTime = 0.f;

                    if (data.cbOnLoopComplete)
                        data.cbOnLoopComplete();
                }
                else
                {
                    if (data.cbOnComplete && !data.bNoCallBack)
                        data.cbOnComplete();

                    data.bToErase = true;
                }
            }

            ++it;
        }
    }

    _bTweenUpdateRunning = false;
}

void CContainer::setTimeout(float fTime, SimpleCallback cb)
{
    addSelfTween(eTweenProp::TIMEOUT, 0.f, 1.f, fTime, Easing::linear, 0, cb);
}

void CContainer::removeSelfTweens()
{
    if (!_bTweenUpdateRunning)
    {
        while (!_lTweens.empty())
        {
            auto ptrHolder = (TweenHandle*)_lTweens.begin()->data.ptrHandleHolder;
            _lTweens.erase(_lTweens.begin());

            if (ptrHolder)
            {
                ptrHolder->nCounter--;
                assert(ptrHolder->nCounter > -1);
                ptrHolder->it = getInitialTweenItter();
            }
        }
    }
    else
    {
        for (auto& it : _lTweens)
        {
            it.data.bToErase    = true;
            it.data.bNoCallBack = true;
        }
    }
}

void CContainer::forceUpdate(float dt)
{
    updateSelfTweens(dt);

    // Независимая система скролла.
    updateScrollAnimation(dt);

    _vChildrenIterating = _vChildren;

    for (size_t i = 0; i < _vChildrenIterating.size(); i++)
    {
        if ((_bIsUpdateInvisible || _vChildrenIterating[i]->isVisible()))
            _vChildrenIterating[i]->update(dt);
    }
}

void CContainer::update(float dt)
{
    if (isVisible())
    {
        static GPULights vInitialLights;
        static GPULights vResultLights;
        vInitialLights.clear();
        vResultLights.clear();
        assert(!_pvCollectedLight);
        if (_bIsLightEmissionEnabled)
        {
            _pvCollectedLight = &vInitialLights;
            collectLights();

            if (_onLightCollectedCb)
                _onLightCollectedCb(vInitialLights);

            if (auto pSettings = std::get_if<LightLayerMinMax>(&_lightEmmiterSettings))
            {
                for (auto& it:*_pvCollectedLight)
                {
                    it.zMax       = pSettings->nLayerMax;
                    it.zMin       = pSettings->nLayerMin;
                    it.intensity *= pSettings->fMul;
                }
            }
            else if (auto pSettings = std::get_if<LightLayerArray>(&_lightEmmiterSettings))
            {
                for (auto& it:*pSettings)
                {
                    if (it.bActive)
                    {
                        for (auto& itL:*_pvCollectedLight)
                        {
                            GPULight l   = itL;
                            l.zMax       = it.nLayer;
                            l.zMin       = it.nLayer;
                            l.intensity *= it.fMul;
                            vResultLights.push_back(l);
                        }
                    }
                }
                _pvCollectedLight = &vResultLights;
            }
            assert(_pvCollectedLight);
            CGfx::getInstance()->appendLights(*_pvCollectedLight);
            _pvCollectedLight = nullptr;
        }
    }

    if (!_bExcludeFromUpdate)
        forceUpdate(dt);
}

void CContainer::renderSelf(float dt, float* rgba)
{
}

Point CContainer::screenToLocal(float x, float y)
{
    Point ptRes;

    Rect rcScreen;
    Rect rcLocal;

    if (getTransBounds(&rcScreen) &&
        rcScreen.cx > 0.f &&
        rcScreen.cy > 0.f &&
        getNotTransBounds(&rcLocal))
    {
        float kx = rcLocal.cx / rcScreen.cx;
        float ky = rcLocal.cy / rcScreen.cy;

        float fx = (static_cast<float>(x) - rcScreen.x) * kx;
        float fy = (static_cast<float>(y) - rcScreen.y) * ky;

        // Это координаты относительно левого верхнего угла видимых границ объекта.
        // Если нужна именно локальная система координат объекта, раскомментируйте:
        //
        // fx += rcLocal.x;
        // fy += rcLocal.y;

        ptRes.x = static_cast<int32_t>(fx >= 0.f ? fx + 0.5f : fx - 0.5f);
        ptRes.y = static_cast<int32_t>(fy >= 0.f ? fy + 0.5f : fy - 0.5f);
        return ptRes;
    }

    // Fallback, если у объекта нет нормальных сохранённых границ.
    float fOriginX = getTransX();
    float fOriginY = getTransY();

    float fScaleX = 1.f;
    float fScaleY = 1.f;
    getWorldScale(fScaleX, fScaleY);

    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);

    if (fabs(fScaleX) > 1e-6f)
        fx = (fx - fOriginX) / fScaleX;
    else
        fx = 0.f;

    if (fabs(fScaleY) > 1e-6f)
        fy = (fy - fOriginY) / fScaleY;
    else
        fy = 0.f;

    ptRes.x = static_cast<int32_t>(fx >= 0.f ? fx + 0.5f : fx - 0.5f);
    ptRes.y = static_cast<int32_t>(fy >= 0.f ? fy + 0.5f : fy - 0.5f);
    return ptRes;
}

void CContainer::setTutorialId(int nId, bool bAddToTutorial)
{
    _nTutorId = nId;

    if (bAddToTutorial)
        TutorialController::getInstance()->addElement(nId);
}

bool CContainer::setTweenPropValue(eTweenProp eProp, float fValue)
{
    bool bRes = true;

    switch (eProp)
    {
        case eTweenProp::SCALE:
            setScale(fValue, fValue);
            break;

        case eTweenProp::SCALE_ABS:
            setScale(isFlippedX() ? -fValue : fValue,
                     isFlippedY() ? -fValue : fValue);
            break;

        case eTweenProp::SCALE_X:
            setScale(fValue, getScaleY());
            break;

        case eTweenProp::SCALE_Y:
            setScale(getScaleX(), fValue);
            break;

        case eTweenProp::X:
            setX(fValue);
            break;

        case eTweenProp::Y:
            setY(fValue);
            break;

        case eTweenProp::ALPHA:
            setAlpha(fValue);
            break;

        case eTweenProp::ROTATE:
            rotate(fValue);
            break;

        case eTweenProp::TIMEOUT:
            break;

        case eTweenProp::BLACKEN:
            setTint(fValue, fValue, fValue);
            break;

        case eTweenProp::GAME_CAMERA_X:
            CGfx::getInstance()->getCameraInfo().gameX = fValue;
            break;

        case eTweenProp::GAME_CAMERA_Y:
            CGfx::getInstance()->getCameraInfo().gameY = fValue;
            break;

        case eTweenProp::BG_CAMERA_X:
            CGfx::getInstance()->getCameraInfo().bgX = fValue;
            break;

        case eTweenProp::BG_CAMERA_Y:
            CGfx::getInstance()->getCameraInfo().bgY = fValue;
            break;

        case eTweenProp::CAMERA_ZOOM:
            CGfx::getInstance()->getCameraInfo().zoom = fValue;
            break;

        case eTweenProp::BLOOM_INTENSITY:
            CGfx::getInstance()->getPostProcessSettings().bloom.intensity = fValue;
            break;

        case eTweenProp::BLOOM_THRESHOLD:
            CGfx::getInstance()->getPostProcessSettings().bloom.threshold = fValue;
            break;

        case eTweenProp::BLOOM_RADIUS:
            CGfx::getInstance()->getPostProcessSettings().bloom.radius = fValue;
            break;

        case eTweenProp::FISH_EYE:
            CGfx::getInstance()->getPostProcessSettings().fisheyeIntensity = fValue;
            break;

        case eTweenProp::DISTORTION:
            CGfx::getInstance()->getPostProcessSettings().distortionIntensity = fValue;
            break;

        case eTweenProp::FLESH:
            CGfx::getInstance()->getPostProcessSettings().fleshLUTIntensity = fValue;
            break;

        case eTweenProp::BLOOD_GHOST:
            CGfx::getInstance()->getPostProcessSettings().ghostTrailIntensity = fValue;
            break;

        case eTweenProp::BREATH:
            CGfx::getInstance()->getPostProcessSettings().vignetteBreatheIntensity = fValue;
            break;

        case eTweenProp::VIGNETTE_RADIUS:
            CGfx::getInstance()->getPostProcessSettings().vignetteRadius = fValue;
            break;

        case eTweenProp::VIGNETTE_INTENCITY:
            CGfx::getInstance()->getPostProcessSettings().vignetteIntensity = fValue;
            break;

        case eTweenProp::BLOOD_DROPS:
            CGfx::getInstance()->getPostProcessSettings().fBloodDrops = fValue;
            break;

        case eTweenProp::STAR_TRAILS:
            CGfx::getInstance()->getPostProcessSettings().fStarTrailIntensity = fValue;
            break;

        case eTweenProp::GRAIN:
            CGfx::getInstance()->getPostProcessSettings().grainIntensity = fValue;
            break;

        case eTweenProp::SATURATION:
            CGfx::getInstance()->getPostProcessSettings().saturation = fValue;
            break;

        case eTweenProp::CONTRAST:
            CGfx::getInstance()->getPostProcessSettings().contrast = fValue;
            break;

        case eTweenProp::BRIGHTNESS:
            CGfx::getInstance()->getPostProcessSettings().brightness = fValue;
            break;

        case eTweenProp::LUT:
            CGfx::getInstance()->getPostProcessSettings().lutIntensity = fValue;
            break;


        default:
            bRes = false;
            break;
    }

    return bRes;
}

void CContainer::applyTransform(CMatrixStack* pMS, bool bForce)
{
    pMS->translate(_fx, _fy);
    pMS->translate(-_rotateCenterX, -_rotateCenterY);
    mRotate(pMS, _fRotate);
    mScale(pMS, _fScaleX * _fScaleMul, _fScaleY * _fScaleMul);
    pMS->translate(_rotateCenterX, _rotateCenterY);

    if (_hasScrollBox)
    {
        float lt[3] = { _origScrollBox.x, _origScrollBox.y, 1.f };
        float rt[3] = { _origScrollBox.right(), _origScrollBox.y, 1.f };
        float lb[3] = { _origScrollBox.x, _origScrollBox.bottom(), 1.f };
        float rb[3] = { _origScrollBox.right(), _origScrollBox.bottom(), 1.f };

        pMS->vecMultiply(lt);
        pMS->vecMultiply(rt);
        pMS->vecMultiply(lb);
        pMS->vecMultiply(rb);

        float screenCx = static_cast<float>(CSceneResize::getInstance()->getScreenWidth());
        float screenCy = static_cast<float>(CSceneResize::getInstance()->getScreenHeight());

        auto toScreenX = [&](float ndcX)
        {
            return (1.f + ndcX) * screenCx * 0.5f;
        };

        auto toScreenY = [&](float ndcY)
        {
            return (1.f - ndcY) * screenCy * 0.5f;
        };

        lt[0] = toScreenX(lt[0]); lt[1] = toScreenY(lt[1]);
        rt[0] = toScreenX(rt[0]); rt[1] = toScreenY(rt[1]);
        lb[0] = toScreenX(lb[0]); lb[1] = toScreenY(lb[1]);
        rb[0] = toScreenX(rb[0]); rb[1] = toScreenY(rb[1]);

        float minX = std::min({lt[0], rt[0], lb[0], rb[0]});
        float maxX = std::max({lt[0], rt[0], lb[0], rb[0]});
        float minY = std::min({lt[1], rt[1], lb[1], rb[1]});
        float maxY = std::max({lt[1], rt[1], lb[1], rb[1]});

        _scrollBox.set(minX, minY, maxX - minX, maxY - minY);
    }

    if (_saveVec || bForce)
    {
        _isVecSaved = true;

        Rect rc;
        getNotTransBounds(&rc);

        _leftTop[0]  = rc.x;
        _leftTop[1]  = rc.y;
        _leftTop[2]  = 1.f;
        _rightBot[0] = rc.right();
        _rightBot[1] = rc.bottom();
        _rightBot[2] = 1.f;

        pMS->vecMultiply(_leftTop);
        pMS->vecMultiply(_rightBot);

        if (_dragable)
        {
            float f[] = { _ptLocalDragOffset.x, _ptLocalDragOffset.y, 1.f };
            pMS->vecMultiply(f);

            _ptScreenDragOffset.x = f[0];
            _ptScreenDragOffset.y = f[1];

            CSceneResize::getInstance()->toScreenCoords(_ptScreenDragOffset.x, _ptScreenDragOffset.y);
        }
    }
    else
    {
        _isVecSaved = false;
    }

    _fxT = pMS->top()[6];
    _fyT = pMS->top()[7];
}

void CContainer::beginRender(float* rgba)
{
    CGfx::getInstance()->setIgnoreTutorialBox(_bIgonoreTutorBox);
    CGfx::getInstance()->getMatrixStack()->save();
    applyTransform(CGfx::getInstance()->getMatrixStack());
}

void CContainer::endRender()
{
    CGfx::getInstance()->setIgnoreTutorialBox(false);
    CGfx::getInstance()->getMatrixStack()->restore();
    TutorialController::getInstance()->onElementRendered(this);
}

void CContainer::render(float dt, float* rgba)
{
    if (isVisible() || _bCalcTransNoRender)
    {
        float globalRGBA[4] = {1.f, 1.f, 1.f, 1.f};
        if (rgba)
        {
            globalRGBA[0] = rgba[0] * _RGBA[0];
            globalRGBA[1] = rgba[1] * _RGBA[1];
            globalRGBA[2] = rgba[2] * _RGBA[2];
            globalRGBA[3] = rgba[3] * _RGBA[3];
        }

        beginRender(rgba);
        {
            auto oldLightLayer = CGfx::getInstance()->getCurrentZLayer();
            CGfx::getInstance()->setCurrentZLayer(_lightLayer);
            float fPrevLightMul = CGfx::getInstance()->setLightIntensityMul(_fLightMul);

            renderSelf(dt, globalRGBA);

            if (_isVecSaved && _saveVec)
            {
                _vRendered.push_back(shared_from_this());
            }
            CGfx::getInstance()->setLightIntensityMul(fPrevLightMul);
            CGfx::getInstance()->setCurrentZLayer(oldLightLayer);
        }

        Rect prevClip = _rcCurrentClip;
        bool hadPrevClip = (prevClip.cx > 0.f && prevClip.cy > 0.f);

        if (_hasScrollBox)
        {
            switch (_eScrollType)
            {
            case eScrollType::E_ST_HOR:
                mTranslate(CGfx::getInstance()->getMatrixStack(), _fScroll, 0);
                break;
            case eScrollType::E_ST_VERT:
                mTranslate(CGfx::getInstance()->getMatrixStack(), 0, _fScroll);
                break;
            default:
                assert(false);
                break;
            }
        }

        if (!_bCalcTransNoRender)
        {
            bool bChildrenVisible = true;

            if (_hasScrollBox)
            {
                Rect clipped = _scrollBox;

                if (_rcGlobalClip.cx > 0.f && _rcGlobalClip.cy > 0.f)
                {
                    float left   = std::max(clipped.x, _rcGlobalClip.x);
                    float top    = std::max(clipped.y, _rcGlobalClip.y);
                    float right  = std::min(clipped.right(), _rcGlobalClip.right());
                    float bottom = std::min(clipped.bottom(), _rcGlobalClip.bottom());
                    if (right > left && bottom > top)
                        clipped.set(left, top, right - left, bottom - top);
                    else
                        clipped.set(0, 0, 0, 0);
                }

                if (hadPrevClip)
                {
                    float left   = std::max(clipped.x, prevClip.x);
                    float top    = std::max(clipped.y, prevClip.y);
                    float right  = std::min(clipped.right(), prevClip.right());
                    float bottom = std::min(clipped.bottom(), prevClip.bottom());
                    if (right > left && bottom > top)
                        clipped.set(left, top, right - left, bottom - top);
                    else
                        clipped.set(0, 0, 0, 0);
                }

                _rcCurrentClip = clipped;

                if (_rcCurrentClip.cx > 0.f && _rcCurrentClip.cy > 0.f)
                {
                    CGfx::getInstance()->setScissor(&_rcCurrentClip);
                }
                else
                {
                    bChildrenVisible = false;
                }
            }

            if (bChildrenVisible)
            {
                for (size_t i = 0; i < _vChildren.size(); i++)
                {
                    if (_vChildren[i]->isVisible() || _vChildren[i]->_bCalcTransNoRender)
                    {
                        _vChildren[i]->render(dt, globalRGBA);
                    }
                }
            }

            if (_hasScrollBox)
            {
                if (hadPrevClip)
                    CGfx::getInstance()->setScissor(&prevClip);
                else
                    CGfx::getInstance()->setScissor(NULL);
                _rcCurrentClip = prevClip;
            }
        }

        endRender();
        _bCalcTransNoRender = false;
    }
}

bool CContainer::onDragBegin(int32_t x, int32_t y)
{
    if (_dragable && _onDragBeginCb)
        return _onDragBeginCb(shared_from_this(), x, y);
    else
        return false;
}

void CContainer::setDragable(CContainerWPtr ptrDefaultDropAcceptor, fOnDragBegin onDragBeginCb)
{
    _ptrDefaultDropAcceptor = ptrDefaultDropAcceptor;
    _onDragBeginCb          = onDragBeginCb;

    if (_onDragBeginCb)
        _dragable = true;
    else
        _dragable = false;
}

void CContainer::onChildAdded(CContainer* pChild)
{
}

void CContainer::setVisible(bool bVisible)
{
    _isVisble = bVisible;
}

void CContainer::setParent(CContainer* pParent)
{
    std::swap(_pParent, pParent);

    if (_pParent != pParent)
    {
        BaseDialog::onContainerParentChanged(this);

        if (_pParent)
            _pParent->onChildAdded(this);
    }
}

void CContainer::addChild(CContainerPtr pChild)
{
    assert(pChild->getParent() == NULL);

    if (pChild->getParent() == NULL)
    {
        _vChildren.push_back(pChild);
        pChild->setParent(this);
        onChildrenChanged();
    }
}

void CContainer::addChildAt(CContainerPtr pChild, size_t nAt)
{
    assert(nAt < _vChildren.size() + 1);
    assert(pChild->getParent() == NULL);

    if (nAt == _vChildren.size())
    {
        _vChildren.push_back(pChild);
        pChild->setParent(this);
        onChildAdded(pChild.get());
        onChildrenChanged();
    }
    else if (nAt < _vChildren.size() && pChild->getParent() == NULL)
    {
        _vChildren.insert(_vChildren.begin() + nAt, pChild);
        pChild->setParent(this);
        onChildrenChanged();
    }
}

void CContainer::addChildAfter(CContainerPtr pChild, CContainerPtr pAfter)
{
    auto idx = getChildIndex(pAfter);
    assert(idx > -1);
    addChildAt(pChild, idx);
}

void CContainer::reserveChildren(int nNum)
{
    _vChildren.reserve(nNum);
}

void CContainer::removeAll()
{
    for (size_t i = 0; i < _vChildren.size(); i++)
        _vChildren[i]->setParent(NULL);

    _vChildren.clear();
}

void CContainer::removeLast()
{
    if (_vChildren.size())
    {
        _vChildren[_vChildren.size() - 1]->setParent(NULL);
        _vChildren.pop_back();
    }
}

void CContainer::removeFromParent()
{
    if (getParent())
        getParent()->removeChildP(this);
}

bool CContainer::removeChildP(CContainer* pChild)
{
    bool bRes = false;

    for (size_t i = 0; i < _vChildren.size(); i++)
    {
        if (_vChildren[i].get() == pChild)
        {
            _vChildren[i]->setParent(NULL);
            _vChildren.erase(_vChildren.begin() + i);
            bRes = true;
            onChildrenChanged();
            break;
        }
    }

    return bRes;
}

bool CContainer::removeChild(CContainerPtr pChild)
{
    bool bRes = false;

    for (size_t i = 0; i < _vChildren.size(); i++)
    {
        if (_vChildren[i] == pChild)
        {
            _vChildren[i]->setParent(NULL);
            _vChildren.erase(_vChildren.begin() + i);
            bRes = true;
            onChildrenChanged();
            break;
        }
    }

    return bRes;
}

bool CContainer::removeChildAt(size_t nAt)
{
    bool bRes = false;

    if (_vChildren.size() > nAt)
    {
        _vChildren[nAt]->setParent(NULL);
        _vChildren.erase(_vChildren.begin() + nAt);
        bRes = true;
        onChildrenChanged();
    }

    return bRes;
}

int CContainer::getChildIndexP(CContainer* pChild)
{
    int nRes = -1;

    for (size_t i = 0; i < _vChildren.size(); i++)
    {
        if (_vChildren[i].get() == pChild)
        {
            nRes = i;
            break;
        }
    }

    return nRes;
}

int CContainer::getChildIndex(CContainerPtr pChild)
{
    return getChildIndexP(pChild.get());
}

CContainerPtr CContainer::getTopMostChild()
{
    CContainerPtr pRes;

    if (_vChildren.size())
        pRes = _vChildren[_vChildren.size() - 1];

    return pRes;
}

void CContainer::saveChildrenZOrder()
{
    assert(_vSaved.size() == 0);

    if (_vSaved.size() == 0)
        _vSaved = _vChildren;
}

void CContainer::restoreChildrenZOrder()
{
    if (_vSaved.size())
    {
        std::map<int, CContainerPtr> mapZorder;
        int lastZ = 0xFFFF;

        for (size_t i = 0; i < _vChildren.size(); i++)
        {
            auto it = std::find(_vSaved.begin(), _vSaved.end(), _vChildren[i]);

            if (it != _vSaved.end())
                mapZorder[it - _vSaved.begin()] = _vChildren[i];
            else
                mapZorder[lastZ++] = _vChildren[i];
        }

        _vSaved.clear();
        _vChildren.clear();

        for (auto it = mapZorder.begin(); it != mapZorder.end(); it++)
            _vChildren.push_back(it->second);

        onChildrenChanged();
    }
}

void CContainer::bringChildToBack(CContainer* pChildA)
{
    int i = 0;

    for (ContainersIt it = _vChildren.begin(); it != _vChildren.end(); it++)
    {
        if (it->get() == pChildA)
        {
            CContainerPtr pCont = *it;
            _vChildren.erase(it);
            _vChildren.insert(_vChildren.begin(), pCont);
            onChildrenChanged();
            break;
        }

        i++;
    }
}

void CContainer::bringChildUnder(CContainerPtr pChildA, CContainerPtr pChildUnder)
{
    int i = 0;

    auto itUnder = std::find(_vChildren.begin(), _vChildren.end(), pChildUnder);
    assert(itUnder != _vChildren.end());

    if (itUnder != _vChildren.end())
    {
        auto itChild = std::find(_vChildren.begin(), _vChildren.end(), pChildA);
        assert(itChild != _vChildren.end());

        if (itChild != _vChildren.end())
        {
            _vChildren.erase(itChild);
            _vChildren.insert(itUnder, pChildA);
        }
    }
}

void CContainer::bringChildToFront(CContainer* pChildA)
{
    int i = 0;

    for (ContainersIt it = _vChildren.begin(); it != _vChildren.end(); it++)
    {
        if (it->get() == pChildA)
        {
            CContainerPtr pCont = *it;
            _vChildren.erase(it);
            _vChildren.push_back(pCont);
            onChildrenChanged();
            break;
        }

        i++;
    }
}

bool CContainer::swapChildren(CContainerPtr pChildA, CContainerPtr pChildB)
{
    bool bRes = false;
    int  idxA = -1;
    int  idxB = -1;

    assert(pChildA != pChildB);
    assert(pChildA->getParent() == this);
    assert(pChildB->getParent() == this);

    for (size_t i = 0; i < _vChildren.size(); i++)
    {
        if (_vChildren[i] == pChildA)
        {
            assert(idxA == -1);
            idxA = i;
        }
        else if (_vChildren[i] == pChildB)
        {
            assert(idxB == -1);
            idxB = i;
        }

        if (idxA > -1 && idxB > -1)
        {
            bRes = true;
            break;
        }
    }

    if (bRes)
    {
        std::swap(_vChildren[idxA], _vChildren[idxB]);
        onChildrenChanged();
    }

    return bRes;
}

bool CContainer::swapChildrenP(CContainer* pChildA, CContainer* pChildB)
{
    bool bRes = false;
    int  idxA = -1;
    int  idxB = -1;

    assert(pChildA != pChildB);
    assert(pChildA->getParent() == this);
    assert(pChildB->getParent() == this);

    for (size_t i = 0; i < _vChildren.size(); i++)
    {
        if (_vChildren[i].get() == pChildA)
        {
            assert(idxA == -1);
            idxA = i;
        }
        else if (_vChildren[i].get() == pChildB)
        {
            assert(idxB == -1);
            idxB = i;
        }

        if (idxA > -1 && idxB > -1)
        {
            bRes = true;
            break;
        }
    }

    if (bRes)
    {
        std::swap(_vChildren[idxA], _vChildren[idxB]);
        onChildrenChanged();
    }

    return bRes;
}

void CContainer::rotate(float fRadians)
{
    _fRotate = fRadians;
}

void CContainer::scaleToFit(float fCx, float fCy)
{
    Rect rc;

    float fScaleCx = 1.f;
    float fScaleCy = 1.f;

    if (!getNotTransBounds(&rc))
        calcNotTransBounds(&rc);

    if (rc.cx > fCx)
        fScaleCx = fCx / rc.cx;

    if (rc.cy > fCy)
        fScaleCy = fCy / rc.cy;

    float fScaleTo = std::min(fScaleCx, fScaleCy);
    setScale(fScaleTo, fScaleTo);
}

void CContainer::scaleToFitCy(float fCy)
{
    Rect rc;
    float fScaleCy = 1.f;

    if (!getNotTransBounds(&rc))
        calcNotTransBounds(&rc);

    if (rc.cy > fCy)
        fScaleCy = fCy / rc.cy;

    setScale(fScaleCy, fScaleCy);
}

void CContainer::setScale(float sx, float sy)
{
    _fScaleX = sx;
    _fScaleY = sy;
}

void CContainer::setScaleTo(float fToCx, float fToCy)
{
    Rect rc;

    if (!getNotTransBounds(&rc))
        calcNotTransBounds(&rc);

    assert(isgreater(rc.cx, 0));
    assert(isgreater(rc.cy, 0));

    if (isgreater(rc.cx, 0) && isgreater(rc.cy, 0))
    {
        _fScaleX = fToCx / rc.cx;
        _fScaleY = fToCy / rc.cy;
    }
}

void CContainer::scaleCx(float fToCx)
{
    Rect rc;

    if (!getNotTransBounds(&rc))
        calcNotTransBounds(&rc);

    assert(isgreater(rc.cx, 0));
    assert(isgreater(rc.cy, 0));

    _fScaleX = fToCx / rc.cx;
}

float CContainer::setScaleToMaxSize(float fMaxCx, float fMaxCy)
{
    Rect rc;
    calcNotTransBounds(&rc);

    float fXScale = 1.f;
    float fYScale = 1.f;

    if (rc.cx > fMaxCx)
        fXScale = fMaxCx / rc.cx;

    if (rc.cy > fMaxCy)
        fYScale = fMaxCy / rc.cy;

    float fRes = std::min(fXScale, fYScale);
    setScale(fRes, fRes);

    return fRes;
}

void CContainer::setScaleToCx(float fToCx, bool bCalc)
{
    float fCx = bCalc ? calcNotTransCx() : getNotTransCx();

    assert(isgreater(fCx, 0));

    if (isgreater(fCx, 0))
    {
        _fScaleX = fToCx / fCx;
        _fScaleY = _fScaleX;
    }
}

void CContainer::setScaleToY(float fToCy)
{
    assert(isgreater(getNotTransCy(), 0));

    if (isgreater(getNotTransCy(), 0))
    {
        _fScaleY = fToCy / getNotTransCy();
    }
}

void CContainer::setPos(float fx, float fy)
{
    _fx = fx;
    _fy = fy;
}

void CContainer::addPos(float fAddX, float fAddY)
{
    _fx += fAddX;
    _fy += fAddY;
}

void CContainer::addX(float fAddX)
{
    _fx += fAddX;
}

void CContainer::addY(float fAddY)
{
    _fy += fAddY;
}

void CContainer::offsetChildren(float fAddX, float fAddY)
{
    for (auto it : _vChildren)
    {
        it->addPos(fAddX, fAddY);
    }
}

void CContainer::getPos(Point* pos)
{
    pos->x = _fx;
    pos->y = _fy;
}

void CContainer::setX(float x)
{
    _fx = x;
}

void CContainer::setY(float y)
{
    _fy = y;
}

void CContainer::setScaleX(float fScale)
{
    _fScaleX = fScale;
}

void CContainer::setScaleY(float fScale)
{
    _fScaleY = fScale;
}

void CContainer::onChildrenChanged()
{
    if (_hasScrollBox)
        updateMaxScroll();
}

void CContainer::mRotate(CMatrixStack* pMS, float fRad)
{
    pMS->rotate(fRad);
}

void CContainer::mTranslate(CMatrixStack* pMS, float fx, float fy)
{
    pMS->translate(fx, fy);
}

void CContainer::mScale(CMatrixStack* pMS, float fx, float fy)
{
    pMS->scale(fx, fy);
}

void CContainer::onHover()
{
    if (_onHoverCb)
        _onHoverCb();
}

void CContainer::onLeave()
{
    if (_onLeaveCb)
        _onLeaveCb();
}

void CContainer::onPointerDown()
{
    if (_onPointerDownCb)
        _onPointerDownCb();
}

void CContainer::onPointerUp()
{
    if (_onPointerUpCb)
        _onPointerUpCb();
}

void CContainer::onPointerMove(bool bIsPressed, int32_t x, int32_t y)
{
    if (_onPointerMoveCb)
        _onPointerMoveCb(bIsPressed, x, y);
}

void CContainer::onClick()
{
    if (_onClickCb)
        _onClickCb();
}

eMouseCursorType CContainer::getMouseCursorType()
{
    return _eMouseCursor;
}

void CContainer::setPosCentered(float fParentCx, float fParentCy, float fAddX, float fAddY)
{
    Rect rc;
    if (!getNotTransBounds(&rc))
        calcNotTransBounds(&rc);
    float fSelfCx = rc.cx * _fScaleX;
    float fSelfCy = rc.cy * _fScaleY;
    setPos((fParentCx - fSelfCx) / 2.f - (rc.x + _rotateCenterX) * _fScaleX + _rotateCenterX + fAddX,
           (fParentCy - fSelfCy) / 2.f - (rc.y + _rotateCenterY) * _fScaleY + _rotateCenterY + fAddY);
}

void CContainer::setPosPivotCentered(float fParentCx, float fParentCy, float fAddX, float fAddY)
{
    Rect rc;
    if (!getNotTransBounds(&rc))
        calcNotTransBounds(&rc);
    float fSelfCx = rc.cx * _fScaleX;
    float fSelfCy = rc.cy * _fScaleY;
    setPos((fParentCx * 0.5f - fSelfCx) / 2.f - (rc.x + _rotateCenterX) * _fScaleX + _rotateCenterX + fAddX,
           (fParentCy * 0.5f - fSelfCy) / 2.f - (rc.y + _rotateCenterY) * _fScaleY + _rotateCenterY + fAddY);
}

void CContainer::setXPosCentered(float fParentCx, float fAddX, bool bCalc)
{
    Rect rc;
    if (!getNotTransBounds(&rc))
        calcNotTransBounds(&rc);
    float fSelfCx = rc.cx * _fScaleX;
    setX((fParentCx - fSelfCx) / 2.f - (rc.x + _rotateCenterX) * _fScaleX + _rotateCenterX + fAddX);
}

void CContainer::setYPosCentered(float fParentCy, float fAddY)
{
    Rect rc;
    if (!getNotTransBounds(&rc))
        calcNotTransBounds(&rc);
    float fSelfCy = rc.cy * _fScaleY;
    setY((fParentCy - fSelfCy) / 2.f - (rc.y + _rotateCenterY) * _fScaleY + _rotateCenterY + fAddY);
}

void CContainer::highlightAsDragTarget(float fHlScale, bool bHighLight)
{
    if (bHighLight)
    {
        assert(!_dragHLStateSaved.bSaved);

        if (!_dragHLStateSaved.bSaved)
        {
            _dragHLStateSaved.fOrigX  = getX();
            _dragHLStateSaved.fOrigY  = getY();
            _dragHLStateSaved.fScaleX = getScaleX();
            _dragHLStateSaved.fScaleY = getScaleY();
            _dragHLStateSaved.bSaved  = true;

            Rect rcPrev;
            calcNotTransBounds(&rcPrev);
            rcPrev.scale(getScaleX(), getScaleY());

            setScale(getScaleX() * fHlScale, getScaleY() * fHlScale);

            Rect rcNew = rcPrev;
            rcNew.scale(fHlScale, fHlScale);

            float fNewX = getX() + (rcPrev.cx - rcNew.cx) / 2.f;
            float fNewY = getY() + (rcPrev.cy - rcNew.cy) / 2.f;

            setPos(fNewX, fNewY);
        }
    }
    else
    {
        assert(_dragHLStateSaved.bSaved);

        if (_dragHLStateSaved.bSaved)
        {
            setPos(_dragHLStateSaved.fOrigX, _dragHLStateSaved.fOrigY);
            setScale(_dragHLStateSaved.fScaleX, _dragHLStateSaved.fScaleY);
            _dragHLStateSaved.bSaved = false;
        }
    }
}

void CContainer::packChildrenCascade(float maxCx, float maxCy, float fPad, bool bDistribute)
{
    if (_vChildren.empty())
        return;

    // Истинные локальные границы: getNotTransBounds, а для контейнеров —
    // рекурсивно по позициям детей (scale=1, rotate=0 => пивот сокращается).
    // НЕ используем calcNotTransBounds: его rc.x — конвенция "pos - pivot",
    // а не визуальное смещение края.
    std::function<bool(CContainer*, Rect&)> trueBounds =
        [&](CContainer* p, Rect& out) -> bool
    {
        if (p->getNotTransBounds(&out))
            return true;
        bool bAny = false;
        out.set(0.f, 0.f, 0.f, 0.f);
        for (auto& g : p->getChildren())
        {
            Rect rg;
            if (trueBounds(g.get(), rg))
            {
                rg.x += g->getX();
                rg.y += g->getY();
                if (!bAny) { out = rg; bAny = true; }
                else       out.unite(&rg);
            }
        }
        return bAny;
    };

    struct PackItem
    {
        CContainerPtr ptrChild;
        float cx;
        float offsX;
        size_t originalIdx;
    };

    std::vector<PackItem> items;
    items.reserve(_vChildren.size());

    for (size_t i = 0; i < _vChildren.size(); ++i)
    {
        _vChildren[i]->setScale(1.0f, 1.0f);

        Rect  rcChild;
        float childCx = 0.f;
        float offsX   = 0.f;
        if (trueBounds(_vChildren[i].get(), rcChild))
        {
            childCx = rcChild.cx;
            offsX   = rcChild.x;
        }
        if (childCx <= 0.f)
            childCx = 140.0f;

        PackItem item;
        item.ptrChild    = _vChildren[i];
        item.cx          = childCx;
        item.offsX       = offsX;
        item.originalIdx = i;
        items.push_back(item);
    }

    std::sort(items.begin(), items.end(), [](const PackItem& a, const PackItem& b)
    {
        return a.cx > b.cx;
    });

    struct RowInfo
    {
        std::vector<PackItem> rowItems;
        float totalWidthWithoutPad = 0.0f;
    };

    std::vector<RowInfo> rows;
    RowInfo currentRow;
    float curX = fPad;

    for (size_t i = 0; i < items.size(); ++i)
    {
        auto& item = items[i];

        if (curX + item.cx + fPad > maxCx)
        {
            if (currentRow.rowItems.empty())
            {
                currentRow.rowItems.push_back(item);
                currentRow.totalWidthWithoutPad += item.cx;
                rows.push_back(currentRow);
                currentRow = RowInfo();
                curX = fPad;
                continue;
            }
            rows.push_back(currentRow);
            currentRow = RowInfo();
            curX = fPad;
        }

        currentRow.rowItems.push_back(item);
        currentRow.totalWidthWithoutPad += item.cx;
        curX += (item.cx + fPad);
    }

    if (!currentRow.rowItems.empty())
        rows.push_back(currentRow);

    const float rowHeight = 42.0f;
    float actualRowPad = fPad;
    float curY = fPad;
    size_t totalRows = rows.size();

    if (bDistribute && totalRows > 1)
    {
        float totalRowsHeight = static_cast<float>(totalRows) * rowHeight;
        float remainingYSpace = maxCy - totalRowsHeight;
        actualRowPad = remainingYSpace / static_cast<float>(totalRows + 1);
        curY = actualRowPad;
    }

    for (size_t r = 0; r < totalRows; ++r)
    {
        auto& row = rows[r];
        size_t count = row.rowItems.size();

        float actualColPad = fPad;
        float startX       = fPad;

        if (bDistribute && count > 1)
        {
            float remainingXSpace = maxCx - row.totalWidthWithoutPad;
            if (remainingXSpace > 0.f)
            {
                actualColPad = remainingXSpace / static_cast<float>(count + 1);
                startX = actualColPad;
            }
            else
            {
                actualColPad = fPad;
                startX = fPad;
            }
        }
        else
        {
            float rowWidth = row.totalWidthWithoutPad
                           + actualColPad * static_cast<float>(count - 1);
            startX = (maxCx - rowWidth) * 0.5f;
            if (startX < fPad)
                startX = fPad;
        }

        float placementX = startX;

        for (size_t i = 0; i < count; ++i)
        {
            auto& item = row.rowItems[i];
            // visual_left = pos + offsX  =>  pos = placementX - offsX
            item.ptrChild->setPos(placementX - item.offsX, curY);
            placementX += (item.cx + actualColPad);
        }

        curY += (rowHeight + actualRowPad);
    }

    onChildrenChanged();
}

void CContainer::packChildrenGuillotine(float maxCx, float maxCy, float fPad)
{
    if (_vChildren.empty())
        return;

    const int numColumns = 3;
    float columnWidth = (maxCx - fPad * (numColumns + 1)) / numColumns;
    float rowHeight = 42.0f;

    float curX = fPad;
    float curY = fPad;
    int currentColumn = 0;

    for (size_t i = 0; i < _vChildren.size(); ++i)
    {
        CContainerPtr ptrChild = _vChildren[i];

        ptrChild->setScale(1.0f, 1.0f);

        float childCx = ptrChild->getCx(false);

        float cellLeftX = fPad + currentColumn * (columnWidth + fPad);
        float alignedX = cellLeftX + (columnWidth - childCx) * 0.5f;

        ptrChild->setPos(alignedX, curY);

        currentColumn++;

        if (currentColumn >= numColumns)
        {
            currentColumn = 0;
            curY += (rowHeight + fPad);
        }
    }

    onChildrenChanged();
}

void CContainer::alignChildren(eChildrenAlign eAlign, float fPad, float fMaxCx, bool bCenter)
{
    if (_vChildren.empty())
        return;

    // Визуальный бокс ребёнка ОТНОСИТЕЛЬНО ЕГО НУЛЯ (без его позиции):
    // размер уже с масштабом ребёнка, пивот скомпенсирован.
    auto childVisualRect = [](CContainerPtr& pChild, Rect& rcOut)
    {
        Rect rc;
        if (!pChild->getNotTransBounds(&rc))
            pChild->calcNotTransBounds(&rc);
        float sx  = pChild->getScaleX();
        float sy  = pChild->getScaleY();
        float pvx = pChild->_rotateCenterX;
        float pvy = pChild->_rotateCenterY;
        rcOut.cx = rc.cx * sx;
        rcOut.cy = rc.cy * sy;
        rcOut.x  = (rc.x + pvx) * sx - pvx;
        rcOut.y  = (rc.y + pvy) * sy - pvy;
    };

    switch (eAlign)
    {
    case eChildrenAlign::VERTICAL_COLUMN:
    {
        std::vector<Rect> rects;
        float fMaxChildCx = 0;
        rects.reserve(_vChildren.size());
        for (auto& it : _vChildren)
        {
            Rect rc;
            childVisualRect(it, rc);
            rects.push_back(rc);
            fMaxChildCx = std::max(rc.cx, fMaxChildCx);
        }
        float fY = fPad;
        for (size_t i = 0; i < _vChildren.size(); i++)
        {
            if (!_vChildren[i]->isVisible())
                continue;
            if (bCenter)
                _vChildren[i]->setXPosCentered(fMaxChildCx);
            else
                _vChildren[i]->setX(-rects[i].x);
            _vChildren[i]->setY(fY - rects[i].y);
            fY += (fPad + rects[i].cy);
        }
    }
    break;
    case eChildrenAlign::HORIZONTAL:
    {
        float fX = 0, fY = 0, fCx = 0, fMaxCy = 0;
        for (size_t i = 0; i < _vChildren.size(); i++)
        {
            auto& ptrChild = _vChildren[i];
            if (!ptrChild->isVisible())
                continue;
            Rect rc;
            childVisualRect(ptrChild, rc);
            if (fCx > fPad && fCx + rc.cx + fPad >= fMaxCx)
            {
                fX  = 0;
                fCx = 0;
                fY += fMaxCy + fPad;
                fMaxCy = 0;
            }
            // ставим так, чтобы ВИЗУАЛЬНЫЙ край встал на (fX, fY)
            ptrChild->setPos(fX - rc.x, fY - rc.y);
            fMaxCy = std::max(fMaxCy, rc.cy);
            fCx += (rc.cx + fPad);
            fX  += (rc.cx + fPad);
        }
    }
    break;
    default:
        assert(false);
        break;
    }
}

void CContainer::calcGlobalScreenClip(Rect& rc)
{
    auto& cfg = Engine::getCfg();

    rc = _rcGlobalClip;
    rc.cx *= (CSceneResize::getInstance()->getScreenWidth()  / cfg.INIT_SCR_CX);
    rc.cy *= (CSceneResize::getInstance()->getScreenHeight() / cfg.INIT_SCR_CY);
}

void CContainer::updateGlobalClip()
{
    if (_rcGlobalClip.cx && _rcGlobalClip.cy)
    {
        Rect rcClip;
        calcGlobalScreenClip(rcClip);
        CGfx::getInstance()->setScissor(&rcClip);
    }
}

void CContainer::setGlobalClip(Rect* pClipRc)
{
    if (!pClipRc || !pClipRc->cx || !pClipRc->cy)
    {
        CGfx::getInstance()->setScissor(nullptr);
        _rcGlobalClip.cx = 0;
        _rcGlobalClip.cy = 0;
    }
    else
    {
        _rcGlobalClip = *pClipRc;
        updateGlobalClip();
    }
}

void CContainer::makeKeyValue(CContainerPtr ptrKeys, CContainerPtr ptrVals, float fPad, float fMaxCx)
{
    assert(ptrKeys->getChildrenCount() == ptrVals->getChildrenCount());

    if (ptrKeys->getChildrenCount() == ptrVals->getChildrenCount())
    {
        ptrVals->alignChildren(eChildrenAlign::VERTICAL_COLUMN, fPad, Engine::getCfg().INIT_SCR_CX);

        for (int i = 0; i < ptrKeys->getChildrenCount(); i++)
        {
            auto ptrKey = ptrKeys->getChildAt(i);
            auto ptrVal = ptrVals->getChildAt(i);

            ptrVal->setX(0);
            ptrKey->setX(0);
            ptrKey->setYPosCentered(ptrVal->calcNotTransCy(), ptrVal->getY());
        }
    }
}

float CContainer::getTransX()
{
    float sceneCx = CSceneResize::getInstance()->getScreenWidth();
    return (1 + _fxT) * sceneCx * 0.5f;
}

float CContainer::getTransY()
{
    float sceneCy = CSceneResize::getInstance()->getScreenHeight();
    return (1 - _fyT) * sceneCy * 0.5f;
}

CContainerPtr CContainer::findTrackedAtPoint(float fx, float fy, const Rect* pCrop, CContainer* pEnsureAncestor)
{
    CContainerPtr pRes;
    static Rect rc;

    if (!pCrop)
    {
        static Rect rcViewport;
        CGfx::getInstance()->getViewportRect(&rcViewport);
        pCrop = &rcViewport;
    }

    if (pCrop->contains(fx, fy))
    {
        for (int i = _vRendered.size() - 1; i > -1; i--)
        {
            if (_vRendered[i]->getInteractiveBounds(&rc))
            {
                if (rc.contains(fx, fy) && pCrop->isIntersect(&rc))
                {
                    if (pEnsureAncestor == NULL || _vRendered[i]->hasAncestor(pEnsureAncestor, true))
                    {
                        pRes = _vRendered[i];
                        break;
                    }
                }
            }
        }
    }

    return pRes;
}

CContainerPtr CContainer::getDialogAtPoint(float fx, float fy)
{
    auto p = findTrackedAtPoint(fx, fy);

    if (p)
        return p->getParentDialog();
    else
        return nullptr;
}

_G2D_NAMESPACE_END_