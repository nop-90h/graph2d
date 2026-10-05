#include "dialogs/basedialog.h"
#include "gfx.h"
#include "timecounter.h"
#include "sceneresize.h"
#include "msched.h"
#include "spriteloader.h"
#include "dragcontroller.h"
#include "bglayer.h"
#include "tutorialcontroller.h"
#include "engine.h"

_G2D_NAMESPACE_BEGIN_

DialogStack BaseDialog::_stack;
Dialogs BaseDialog::_existing;

void BaseDialog::onContainerParentChanged(CContainer* pCont)
{
    for (auto& it : _existing)
        it->checkElementRefs(pCont);
}

void BaseDialog::notifyParent(int64_t i)
{
    auto pParent = getParentDialog();

    assert(pParent);

    if (pParent)
    {
        pParent->notifyInt(this, i);
    }
}

void BaseDialog::notifyParentEx(uint64_t param1, uint64_t param2, uint64_t param3, uint64_t param4)
{
    auto pParent = getParentDialog();

    assert(pParent);

    if (pParent)
    {
        pParent->notifyEx(this, param1, param2, param3, param4);
    }
}

void BaseDialog::removeScroll()
{
    assert(_eState == BaseDialog::INIT_DONE);

    if (_eState == BaseDialog::INIT_DONE && _eScrollType != eScrollType::E_ST_NONE)
    {
        setScrollBox();

        if (_root)
        {
            _root->stopScrollAnimation();
            _root->setScrollBox(nullptr, eScrollType::E_ST_NONE);
        }
    }

    _eScrollType = eScrollType::E_ST_NONE;

    if (_ptrScollBar)
        _ptrScollBar->endSliderDrag();

    if (_ptrScollBar && _ptrScollBar->getParent())
    {
        _ptrScollBar->removeFromParent();
    }

    if (_ptrShadowU)
    {
        _ptrShadowU->removeFromParent();
        _ptrShadowU = nullptr;
    }

    if (_ptrShadowB)
    {
        _ptrShadowB->removeFromParent();
        _ptrShadowB = nullptr;
    }
}

CContainer* BaseDialog::getGfxRoot(void)
{
    CContainer* ptrRes = nullptr;

    switch (_eRoot)
    {
    case eDialogRoot::GAME:
        ptrRes = CGfx::getInstance()->getGameRoot();
        break;

    case eDialogRoot::INTERFACE:
        ptrRes = CGfx::getInstance()->getGameIface();
        break;

    default:
        assert(false);
        break;
    }

    return ptrRes;
}

bool BaseDialog::init(float cx, float cy, eScrollType eScroll, Rect* pInnerChildrenRect, CContainerPtr ptrBg, CContainer* pParent)
{
    bool bRes = false;

    assert(_eState == BaseDialog::UNINIT);

    if (_eState == BaseDialog::UNINIT)
    {
        _existing.insert(getPtr());

        setClass(E_EL_DIALOG);

        _eScrollType    = eScroll;
        _fDialogCx      = cx;
        _fDialogCy      = cy;
        _eInputHandling = E_IH_NONE;

        _frame          = std::make_shared<CContainer>();

        if (ptrBg)
            createBack(ptrBg, cx, cy);

        _root = std::make_shared<CContainer>();
        _root->setScrollBox(pInnerChildrenRect, eScroll);

        _eState = BaseDialog::INIT_DONE;

        _fg = std::make_shared<CContainer>();

        _frame->addChild(_root);
        _frame->addChild(_fg);
        addChild(_frame);

        {
            RenderTracker dontTrack(false, eMouseCursorType::E_MCT_NORMAL);
            createScrollShadow(pInnerChildrenRect);
        }

        createScrollBar(eScroll);

        if (_ptrScollBar)
            _ptrScollBar->setSysId(SCROLLBAR_SYS_ID);

        if (_root)
        {
            _root->setOnScrollChanged([this]()
            {
                onScrolled();
            });
        }

        if (pParent)
        {
            if (pParent->getClass() == E_EL_DIALOG)
                pParent = ((BaseDialog*)pParent)->_frame.get();

            _preSavedParent = pParent;
            pParent->addChild(getPtr());
            setVisible(false);
        }

        bRes = true;
    }

    return bRes;
}

void BaseDialog::removeFromStage(void)
{
    if (_eState == BaseDialog::INIT_DONE)
    {
        if (_root)
            _root->stopScrollAnimation();

        auto it = _existing.find(getPtr());
        assert(it != _existing.end());

        if (it != _existing.end())
            _existing.erase(it);

        if (_pParent)
            _pParent->removeChildP(this);

        if (_ptrBehindBg && _ptrBehindBg->getParent())
            _ptrBehindBg->getParent()->removeChild(_ptrBehindBg);

        _eState = BaseDialog::REMOVED;
    }
}

CContainerPtr BaseDialog::createBack(CContainerPtr ptrFrom, float cx, float cy)
{
    assert(ptrFrom);

    RenderTracker scopedTracker(true, eMouseCursorType::E_MCT_NORMAL);

    switch (ptrFrom->getClass())
    {
    case E_EL_SPRITE:
    {
        CSpritePtr ptrBgMy = std::dynamic_pointer_cast<CSprite>(ptrFrom)->cloneInitial();
        ptrBgMy->setScaleTo(cx - 26, cy - 26);
        _frame->addChild(ptrBgMy);
        ptrBgMy->setPos(13, 13);
        _ptrBg = ptrBgMy;
    }
    break;

    case E_EL_NINESLICE:
    {
        NineSlicePtr ptrBgMy = std::dynamic_pointer_cast<NineSlice>(ptrFrom)->cloneInitial();
        ptrBgMy->build(cx - 26, cy - 26);
        _frame->addChild(ptrBgMy);
        ptrBgMy->setPos(13, 13);
        _ptrBg = ptrBgMy;
    }
    break;

    default:
    {
        assert(false);
    }
    break;
    }

    return _ptrBg;
}

void BaseDialog::update(float dt)
{
    animateFg(dt);
    Widget::update(dt);
}

void BaseDialog::animateFg(float dt)
{
    constexpr float fMaxEffectLifeTime = 3.f;

    if (_bFgAnimated && _ptrFg)
    {
        if (_bEffectGoBack)
        {
            _fEffectLifeTime -= dt;

            if (_fEffectLifeTime < 0)
            {
                _fEffectLifeTime = 0;
                _bEffectGoBack   = false;
            }
        }
        else
        {
            _fEffectLifeTime += dt;

            if (_fEffectLifeTime > fMaxEffectLifeTime)
            {
                _fEffectLifeTime = fMaxEffectLifeTime;
                _bEffectGoBack   = true;
            }
        }

        for (int i = 0; i < _ptrFg->getChildrenCount(); i++)
        {
            CSpritePtr ptrSpr = std::dynamic_pointer_cast<CSprite>(_ptrFg->getChildAt(i));
            ptrSpr->setEffectType(23.f);
            ptrSpr->setEffectLifeTime(fMaxEffectLifeTime / _fEffectLifeTime);
        }
    }
}

void BaseDialog::createFgLayer(CContainerPtr ptrFrom, float nineSliceA, float nineSliceB, float nineSliceC, float nineSliceD, Point* pDims)
{
    switch (ptrFrom->getClass())
    {
    case E_EL_SPRITE:
    {
        NineSlicePtr ptrNineSlice = std::make_shared<NineSlice>();
        ptrNineSlice->createSlices(std::dynamic_pointer_cast<CSprite>(ptrFrom), nineSliceA, nineSliceB, nineSliceC, nineSliceD);
        ptrNineSlice->build(pDims ? pDims->x : _fDialogCx, pDims ? pDims->y : _fDialogCy);

        _ptrFg = ptrNineSlice;
        _frame->addChild(ptrNineSlice);
    }
    break;

    case E_EL_NINESLICE:
    {
        _ptrFg = std::dynamic_pointer_cast<NineSlice>(ptrFrom);
        _ptrFg->build(pDims ? pDims->x : _fDialogCx, pDims ? pDims->y : _fDialogCy);
        _frame->addChild(_ptrFg);
    }
    break;

    default:
        assert(false);
        break;
    }
}

CSpritePtr BaseDialog::createVertLine(bool bRight)
{
    CSpritePtr ptrLine = SpriteLoader::getInstance()->getSprite("UI/lineV");

    ptrLine->setPos(bRight ? _fDialogCx : 0, 0);
    ptrLine->setScaleToY(_fDialogCy);

    _frame->addChild(ptrLine);

    return ptrLine;
}

BaseDialogPtr BaseDialog::getParentDialog()
{
    BaseDialogPtr ptrRes;

    CContainer* pParent = getParent();

    assert(pParent && pParent->getClass() != E_EL_DIALOG);

    if (pParent && pParent->getClass() != E_EL_DIALOG)
    {
        pParent = pParent->getParent();

        assert(pParent && pParent->getClass() == E_EL_DIALOG);

        if (pParent && pParent->getClass() == E_EL_DIALOG)
        {
            BaseDialog* pParentDlg = (BaseDialog*)pParent;
            ptrRes                 = pParentDlg->getPtr();
        }
    }

    return ptrRes;
}

bool BaseDialog::hasParentDialog()
{
    bool bRes = false;

    CContainer* pParent = getParent();

    if (pParent && pParent->getClass() != E_EL_DIALOG)
    {
        pParent = pParent->getParent();
        bRes = pParent && pParent->getClass() == E_EL_DIALOG;
    }

    return bRes;
}

void BaseDialog::bringAboveParent()
{
    CContainer* pParent = getParent();

    assert(pParent && pParent->getClass() != E_EL_DIALOG);

    if (pParent && pParent->getClass() != E_EL_DIALOG)
    {
        pParent = pParent->getParent();

        assert(pParent && pParent->getClass() == E_EL_DIALOG);

        if (pParent && pParent->getClass() == E_EL_DIALOG)
        {
            BaseDialog* pParentDlg = (BaseDialog*)pParent;
            pParentDlg->_frame->bringChildToFront(this);
        }
    }
}

void BaseDialog::bringUnderParent()
{
    CContainer* pParent = getParent();

    assert(pParent && pParent->getClass() != E_EL_DIALOG);

    if (pParent && pParent->getClass() != E_EL_DIALOG)
    {
        pParent = pParent->getParent();

        assert(pParent && pParent->getClass() == E_EL_DIALOG);

        if (pParent && pParent->getClass() == E_EL_DIALOG)
        {
            BaseDialog* pParentDlg = (BaseDialog*)pParent;

            CContainerPtr ptrBottom = pParentDlg->_frame->getChildAt(0);

            assert(ptrBottom && ptrBottom.get() != this);

            if (ptrBottom && ptrBottom.get() != this)
            {
                pParentDlg->_frame->bringChildToBack(this);
            }
        }
    }
}

void BaseDialog::getScrollShadowPos(float& fX1, float& fY1, float& fX2, float& fY2, float& fCX, float& fCY)
{
    switch (_eScrollType)
    {
    case E_ST_VERT:
    {
        fX1 += 10;
        fY1 += 15;
        fX2 += 10;
        fY2 -= 0;

        fCY = 100;
        fCX = _fDialogCx - 20;
    }
    break;

    case E_ST_NONE:
        break;

    default:
        assert(false);
        break;
    }
}

void BaseDialog::createScrollShadow(Rect* pScrollRc)
{
    switch (_eScrollType)
    {
    case E_ST_VERT:
    {
        _ptrShadowU = SpriteLoader::getInstance()->getSprite("UI/scrollShadowH");
        _ptrShadowB = SpriteLoader::getInstance()->getSprite("UI/scrollShadowH");

        float fX1 = 0;
        float fY1 = _root->getY() + pScrollRc->y;
        float fX2 = 0;
        float fY2 = _root->getY() + pScrollRc->y + pScrollRc->cy;
        float fCx = 0;
        float fCy = 0;

        getScrollShadowPos(fX1, fY1, fX2, fY2, fCx, fCy);

        _ptrShadowU->setPos(fX1, fY1);
        _ptrShadowU->setScaleTo(fCx, fCy);

        _ptrShadowB->setScaleTo(fCx, fCy);
        _ptrShadowB->setPos(fX2, fY2 + _ptrShadowB->getNotTransCy() * getScaleY());
        _ptrShadowB->flipY();

        _frame->addChild(_ptrShadowU);
        _frame->addChild(_ptrShadowB);
    }
    break;

    case E_ST_NONE:
        break;

    default:
    {
        assert(false);
    }
    break;
    }
}

void BaseDialog::createScrollBar(eScrollType eScrollType)
{
    RenderTracker trackerScoped;

    _ptrScollBar = std::make_shared<ScrollBar>();

    switch (eScrollType)
    {
    case eScrollType::E_ST_HOR:
    {
        _ptrScollBar->create(eScrollType::E_ST_HOR, _fDialogCx);
        _frame->addChild(_ptrScollBar);
        _ptrScollBar->setY(_fDialogCy);
    }
    break;

    case eScrollType::E_ST_VERT:
    {
        _ptrScollBar->create(eScrollType::E_ST_VERT, _fDialogCy);
        _frame->addChild(_ptrScollBar);
        _ptrScollBar->setX(_fDialogCx);
    }
    break;

    default:
        break;
    }
}

float BaseDialog::getTopMargin()
{
    float fTop = 0;
    return fTop;
}

void BaseDialog::addFgBgToStage()
{
    if (_ptrFgBg)
    {
        BgLayer::getInstance(eRenderLayer::FOREGROUND)->addChild(_ptrFgBg);
    }
}

void BaseDialog::addBehindBgToStage()
{
    if (_ptrBehindBg)
    {
        BgLayer::getInstance()->addChild(_ptrBehindBg);
    }
}

void BaseDialog::_updateScaleAndPos()
{
    updateScaleAndPos();
    updateScrollBox();
}

void BaseDialog::updateScaleAndPos()
{
    if (!_bNoUpdateScaleAndPos)
    {
        auto& cfg = Engine::getCfg();

        float fScaleX = 1.f;
        float fScaleY = 1.f;

        float cx = static_cast<float>(CSceneResize::getInstance()->getGameWidth());
        float cy = static_cast<float>(CSceneResize::getInstance()->getGameHeight());

        float fTopMarg = getTopMargin();
        cy -= fTopMarg;

        fScaleX = cx / cfg.INIT_SCR_CX;
        fScaleY = cy / cfg.INIT_SCR_CY;

        float fMinScale = std::min(fScaleX, fScaleY);

        setScale(fMinScale, fMinScale);
        setPos((cx - _fDialogCx * fMinScale) / 2.f,
               (cy - _fDialogCy * fMinScale) / 2.f + fTopMarg);
    }
    else
    {
        updateScrollBox();
    }
}

const Rect* BaseDialog::getViewRect()
{
    const Rect* pRes = NULL;

    if (_root->_hasScrollBox)
    {
        pRes = &(_root->getScrollBox());
    }

    return pRes;
}

const Rect* BaseDialog::getViewRectRecursive()
{
    const Rect* pRes = getViewRect();

    auto p = getParent(); // _root
    if (p)
        p = p->getParent(); // BaseDialog

    if (!pRes && p)
    {
        if (p->getClass() == E_EL_DIALOG)
            pRes = ((BaseDialog*)p)->getViewRectRecursive();
    }

    return pRes;
}

void BaseDialog::createCloseButton(CSpritePtr ptrNormal, CSpritePtr ptrPressed, CSpritePtr ptrHover, CSpritePtr ptrDisabled)
{
    assert(!_closeButton);

    if (!_bNoCloseButton && !_closeButton)
    {
        _closeButton = Button::makeInst(ptrNormal, ptrPressed, ptrHover, ptrDisabled, CLOSE_BUTTON_SYS_ID);
        _closeButton->setScale(1.5f, 1.5f);
        _closeButton->setPivotCentered();

        float fCx = _closeButton->getNotTransCx();
        float fCy = _closeButton->getNotTransCy();

        _closeButton->setPos(_fDialogCx - fCx, 0);

        _closeButton->setOnClick([this]()
        {
            onCloseButtonClick();
        });

        _frame->addChild(_closeButton);
    }
}

void BaseDialog::enableDrag(bool bCanDrag)
{
    _bCanDrag = bCanDrag;
}

void BaseDialog::onCloseButtonClick()
{
    _closedByCloseButton = true;
    close();
    _closedByCloseButton = false;
}

void BaseDialog::setTitle(const char* lpccTitle, CSpritePtr threeSliceSpr, float fA, float fB, bool bHtml)
{
    if (_strTitle != lpccTitle)
    {
        if (_titleCont)
        {
            _frame->removeChild(_titleCont);
        }

        _ptrTitleLabel = std::make_shared<StaticLabel>();

        float cxText   = 0;

        if (bHtml)
        {
            _ptrTitleLabel->setBoxMode(eTextRenderType::HTML_BOX, getDialogCx());
            //_ptrTitleLabel->setText(lpccTitle);
        }
        else
        {
        }

        _ptrTitleLabel->setText(lpccTitle);
        cxText = _ptrTitleLabel->calcCx();

        _titleFrame = std::make_shared<ThreeSliceHor>();
        _titleFrame->createSlices(threeSliceSpr, fA, fB);

        float fRoomForTextCx = cxText + fA + fB;

        _titleFrame->build(fRoomForTextCx);

        _titleCont = std::make_shared<CContainer>();
        _titleCont->addChild(_titleFrame);
        _titleCont->addChild(_ptrTitleLabel);

        if (bHtml)
        {
            _ptrTitleLabel->setPosCentered(_titleFrame->getNotTransCx(), _titleFrame->getNotTransCy());
        }
        else
        {
            _ptrTitleLabel->setAlign(eTRAlign::TR_ALIGN_CENTER | eTRAlign::TR_ALIGN_MIDDLE);
            _ptrTitleLabel->setTextOffs(_titleFrame->getNotTransCx() * 0.5f, _titleFrame->getNotTransCy() * 0.5f);
        }

        _titleCont->setPos((_fDialogCx - fRoomForTextCx) / 2.f, 0);

        _frame->addChild(_titleCont);
    }
}

bool BaseDialog::showAsPanel(eInputHandling eHandling, bool bNoUpdateScaleAndPos)
{
    bool bRes = false;

    if (_eState == BaseDialog::SHOWN_AS_PANEL)
        return true;

    assert(_eState == BaseDialog::INIT_DONE || _eState == BaseDialog::CLOSED);

    if (_eState == BaseDialog::CLOSED)
        _existing.insert(getPtr());

    CContainer* pParent = getParent();

    if (!pParent)
    {
        assert(_preSavedParent);

        if (_preSavedParent)
            _preSavedParent->addChild(getPtr());

        pParent = _preSavedParent;
    }

    assert(pParent);

    if (pParent->getParent() && pParent->getParent()->getClass() == E_EL_DIALOG)
        pParent = pParent->getParent();

    if ((_eState == BaseDialog::INIT_DONE || _eState == BaseDialog::CLOSED) && pParent)
    {
        CSceneResize::getInstance()->addListener(this);
        TutorialController::getInstance()->addListener(this);

        if (_eScrollType != eScrollType::E_ST_NONE)
            CGfx::getInstance()->enableScissor(true);

        _eInputHandling = eHandling;

        switch (_eInputHandling)
        {
        case E_IH_CAPTUREPOINTER:
            InputController::getInstance()->setExclusiveMouse(this);
            break;

        case E_IH_RELAXED:
            InputController::getInstance()->addListener(this);
            break;

        case E_IH_DISPATCHBYPARENT:
        {
            assert(pParent);

            if (pParent)
            {
                assert(pParent->getClass() == E_EL_DIALOG);

                if (pParent->getClass() == E_EL_DIALOG)
                {
                    BaseDialog* pDlg = (BaseDialog*)pParent;
                    pDlg->addDispatchedDlg(getPtr());
                }
            }
        }
        break;

        case E_IH_NONE:
            break;

        default:
            assert(false);
            break;
        }

        _bNoUpdateScaleAndPos = bNoUpdateScaleAndPos;

        setVisible(true);

        _eState = BaseDialog::SHOWN_AS_PANEL;

        _updateScaleAndPos();

        doOnShow(true);
    }

    return bRes;
}

void BaseDialog::updateScrollBox(void)
{
    if (!_root)
        return;

    _root->updateScrollBox();

    if (_ptrScollBar)
    {
        float fPercent = _ptrScollBar->getSliderPos();
        _root->scrollTo(_root->getMaxScroll() * fPercent);

        float fMaxScroll = _root->getMaxScroll();
        _ptrScollBar->showSlider(fMaxScroll != 0);
    }
}

BaseDialog::~BaseDialog(void)
{
}

void BaseDialog::closeAllModals(void)
{
    while (!_stack.empty())
        _stack.back()->close();
}

void BaseDialog::saveStackAndHide()
{
    assert(_savedStack.empty());

    if (_savedStack.empty())
    {
        while (!_stack.empty())
        {
            _savedStack.push_back(_stack.back());
            _stack.back()->setVisible(false);
            InputController::getInstance()->setExclusiveMouse(nullptr);
            _stack.pop_back();
        }
    }
}

void BaseDialog::restoreStackAndShow(void)
{
    assert(_stack.empty());

    if (_stack.empty() && !_savedStack.empty())
    {
        for (auto& value : _savedStack | std::views::reverse)
        {
            _stack.push_back(value);
            InputController::getInstance()->setExclusiveMouse(value.get());
        }

        _savedStack.front()->setVisible(true);
        _savedStack.clear();
    }
}

void BaseDialog::setShowOvelappingDialogs(bool bShow)
{
    _hideOvelappingModals = !bShow;

    if (!_stack.empty())
    {
        for (auto &it:_stack)
            it->setVisible(bShow);

        _stack.back()->setVisible(true);
    }
}

bool BaseDialog::open(SimpleCallback onCloseCb)
{
    bool bRes = false;

    assert(_eState == BaseDialog::INIT_DONE || _eState == BaseDialog::CLOSED);
    assert(getParent() == NULL);

    if ((_eState == BaseDialog::INIT_DONE || _eState == BaseDialog::CLOSED) && getParent() == NULL)
    {
        auto& cfg = Engine::getCfg();

        CSceneResize::getInstance()->addListener(this);
        TutorialController::getInstance()->addListener(this);

        _onCloseCb = onCloseCb;

        CGfx::getInstance()->enableScissor(true);

        if (BaseDialog::_stack.size())
        {
            if (_hideOvelappingModals)
                BaseDialog::_stack.back()->setVisible(false);

            CGfx::getInstance()->setScissor(NULL);
        }

        BaseDialog::_stack.push_back(getPtr());

        setVisible(true);

        if (_ptrBehindBg)
            _ptrBehindBg->setScaleTo(cfg.INIT_SCR_CX, cfg.INIT_SCR_CY);

        addBehindBgToStage();

        getGfxRoot()->addChild(getPtr());

        _eState = BaseDialog::OPENED;
        _eInputHandling = E_IH_CAPTUREPOINTER;

        InputController::getInstance()->setExclusiveMouse(this);

        _updateScaleAndPos();

        doOnShow(true);

        getEmmiter()->emit(EVT_ON_SHOW_ANY);
    }

    return bRes;
}

void BaseDialog::_close()
{
    if (_root)
        _root->stopScrollAnimation();

    CSceneResize::getInstance()->removeListener(this);
    TutorialController::getInstance()->removeListener(this);

    setVisible(false);

    switch (_eInputHandling)
    {
    case E_IH_CAPTUREPOINTER:
        InputController::getInstance()->setExclusiveMouse(NULL);
        break;

    case E_IH_RELAXED:
        InputController::getInstance()->removeListener(this);
        break;

    case E_IH_DISPATCHBYPARENT:
    {
        CContainer* pParent = getParent();

        assert(pParent);

        if (pParent)
        {
            if (pParent->getClass() != E_EL_DIALOG && pParent->getParent())
                pParent = pParent->getParent();

            assert(pParent->getClass() == E_EL_DIALOG);

            if (pParent->getClass() == E_EL_DIALOG)
            {
                BaseDialog* pDlg = (BaseDialog*)pParent;
                pDlg->removeDispatchedDlg(getPtr());
            }
        }
    }
    break;

    case E_IH_NONE:
        break;

    default:
        assert(false);
        break;
    }

    _eState = BaseDialog::CLOSED;

    doOnShow(false);

    auto it = _existing.find(getPtr());
    assert(it != _existing.end());

    if (it != _existing.end())
        _existing.erase(it);

    if (_pParent)
        _pParent->removeChildP(this);

    if (_ptrBehindBg && _ptrBehindBg->getParent())
        _ptrBehindBg->getParent()->removeChild(_ptrBehindBg);

    if (_onCloseCb)
        _onCloseCb();
}

bool BaseDialog::close()
{
    bool bRes = false;

    assert(_eState == BaseDialog::OPENED || _eState == BaseDialog::SHOWN_AS_PANEL);

    if (_eState == BaseDialog::OPENED)
    {
        assert(BaseDialog::_stack.size());

        if (BaseDialog::_stack.size())
        {
            assert(BaseDialog::_stack.back() == getPtr());

            if (BaseDialog::_stack.back() == getPtr())
            {
                bRes = true;

                BaseDialog::_stack.pop_back();

                _close();

                if (BaseDialog::_stack.size())
                {
                    if (_hideOvelappingModals)
                        BaseDialog::_stack.back()->setVisible(true);

                    getEmmiter()->emit(EVT_ON_CLOSE_ANY);
                }
                else
                {
                    CGfx::getInstance()->enableScissor(false);

                    getEmmiter()->emit(EVT_ON_CLOSE_ANY);
                    getEmmiter()->emit(EVT_ON_CLOSE_ALL);
                }
            }
        }
    }
    else if (_eState == BaseDialog::SHOWN_AS_PANEL)
    {
        bRes = true;

        _close();

        if (!BaseDialog::_stack.size())
            CGfx::getInstance()->enableScissor(false);
    }

    return bRes;
}

void BaseDialog::onScrolled()
{
    if (_eScrollType == eScrollType::E_ST_NONE || !_root || !_ptrScollBar)
        return;

    float fMaxScroll = _root->getMaxScroll();

    if (isgreater(fabs(fMaxScroll), 0.f))
    {
        float fScrollPos = std::max(fMaxScroll, std::min(_root->getScroll(), 0.f));
        _ptrScollBar->setSliderPos(fabs(fScrollPos / fMaxScroll));
    }
}

void BaseDialog::stopScroll()
{
    if (_root)
        _root->stopScrollAnimation();
}

void BaseDialog::scrollByDelta(float fDelta)
{
    assert(_eScrollType != eScrollType::E_ST_NONE);

    if (_eScrollType != eScrollType::E_ST_NONE && _root)
    {
        fDelta = -fDelta;
        // ќб€зательно гасим возможную зависшую scroll/fling анимацию.
        _root->stopScrollAnimation();

        float fMax = _root->getMaxScroll();
        float fCur = _root->getScroll();

        // getMaxScroll() может быть отрицательным, поэтому делаем универсальный диапазон.
        float fMin = std::min(0.f, fMax);
        float fMaxLimit = std::max(0.f, fMax);

        // Wheel delta обычно приходит в экранных единицах.
        fDelta = screenDeltaToScrollDelta(fDelta);

        float fNew = fCur + fDelta;

        if (fNew < fMin)
            fNew = fMin;
        else if (fNew > fMaxLimit)
            fNew = fMaxLimit;

        if (fabs(fNew - fCur) > 1e-6f)
        {
            _root->scrollTo(fNew);
        }
        else
        {
            // ≈сли уже в границе, полезно сбросить возможный overscroll.
            _root->settleScroll();
        }

        // ќбновить scrollbar.
        onScrolled();
    }
}

float BaseDialog::screenDeltaToScrollDelta(float fScreenDelta)
{
    if (!_root || _eScrollType == eScrollType::E_ST_NONE)
        return fScreenDelta;

    float fScaleX = 1.f;
    float fScaleY = 1.f;

    _root->calcTotalScale(fScaleX, fScaleY);

    float fScale = (_eScrollType == eScrollType::E_ST_HOR) ? fScaleX : fScaleY;

    if (!isgreater(fabs(fScale), 1e-6f))
        return fScreenDelta;

    return fScreenDelta / fScale;
}

void BaseDialog::doOnShow(bool bShow)
{
    if (_bAutoHideFgControls)
    {
        CGfx::getInstance()->getFgControlsRoot()->setVisible(!bShow);
    }

    onShow(bShow);
}

void BaseDialog::onShow(bool bShow)
{
}

bool BaseDialog::onMouseWheel(int32_t deltaY)
{
    bool bRes = false;

    if (_eScrollType != eScrollType::E_ST_NONE &&
        _root &&
        isVisible() &&
        !InputController::getInstance()->isMouseDown())
    {
        if (!TutorialController::getInstance()->isRunning())
        {
            if (isgreater(fabs(_root->getMaxScroll()), 0.f))
            {
                Point pt = InputController::getInstance()->getMousePos();

                bool bInsideScrollArea = false;

                const Rect* pView = getViewRect();

                if (pView && pView->cx > 0.f && pView->cy > 0.f)
                {
                    bInsideScrollArea = pView->contains(pt.x, pt.y);
                }
                else
                {
                    // Fallback, если scroll box ещЄ не посчитан.
                    auto ptrTarget = findTrackedAt(pt.x, pt.y, nullptr);
                    bInsideScrollArea = ptrTarget && ptrTarget->hasAncestor(this);
                }

                if (bInsideScrollArea)
                {
                    if (_root)
                        _root->stopScrollAnimation();

                    float fDelta = static_cast<float>(deltaY);
                    scrollByDelta(fDelta * 0.5f);

                    bRes = true;
                }
            }
        }
    }

    return bRes;
}

bool BaseDialog::canPassEventsTo(CContainer* pCont)
{
    bool bRes = TutorialController::getInstance()->isElementActive(pCont);

    if (_nWaitForTutorEvt > -1)
    {
        bRes = bRes && TutorialController::getInstance()->isRunning();
    }

    return bRes;
}

bool BaseDialog::onSysPointer(int32_t x, int32_t y, ePointerAction eAction)
{
    bool bRes = false;

    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);

    switch (eAction)
    {
    case E_PA_DOWN:
    {
        if (_eScrollType != eScrollType::E_ST_NONE)
        {
            stopScroll();
        }

        _pMouseOwner.reset();

        auto ptrTarget = CSprite::findTrackedAtPoint(fx, fy, nullptr, this);

        if (ptrTarget)
        {
            CContainer* pSys = ptrTarget->getAncestorByAnySysId(true);

            if (pSys)
            {
                bRes = true;

                if (!TutorialController::getInstance()->isRunning() && pSys->getSysId() == SCROLLBAR_SYS_ID)
                {
                    assert(_eScrollType != eScrollType::E_ST_NONE);

                    ScrollBar* pScrollBar = (ScrollBar*)pSys;

                    if (pScrollBar && pScrollBar->isSlider(ptrTarget.get()))
                    {
                        _pMouseOwner = pScrollBar->shared_from_this();

                        pScrollBar->beginSliderDrag(pScrollBar->getType() == eScrollType::E_ST_VERT ? fy : fx);
                    }
                }
                else if (canPassEventsTo(pSys))
                {
                    _pMouseOwner = pSys->shared_from_this();
                    _pMouseOwner.lock()->onPointerDown();
                }
            }
        }
    }
    break;

    case E_PA_MOVE:
    {
        auto pMouseOwner = _pMouseOwner.lock();

        if (!TutorialController::getInstance()->isRunning() && pMouseOwner && pMouseOwner->getSysId() == SCROLLBAR_SYS_ID)
        {
            bRes = true;

            float fPercent = 0;

            if (_ptrScollBar && _ptrScollBar->getType() == eScrollType::E_ST_VERT)
                fPercent = _ptrScollBar->calcSliderDragPercent(fy);
            else if (_ptrScollBar)
                fPercent = _ptrScollBar->calcSliderDragPercent(fx);

            _root->scrollTo(_root->getMaxScroll() * fPercent);
            onScrolled();
        }
        else if (!pMouseOwner && !InputController::getInstance()->isMouseDown())
        {
            CContainer* pTarget = NULL;

            auto ptr = CSprite::findTrackedAtPoint(fx, fy, nullptr, this);

            if (ptr)
            {
                pTarget = ptr->getAncestorByAnySysId(true);
            }

            if (pTarget && canPassEventsTo(pTarget))
            {
                bRes = true;
                setHovered(pTarget ? pTarget->shared_from_this() : nullptr, true);
            }
            else
            {
                auto p = _pHover.lock();

                if (p && p->getSysId() && canPassEventsTo(p.get()))
                    setHovered(nullptr, true);
            }
        }
    }
    break;

    case E_PA_UP:
    {
        auto pMouseOwner = _pMouseOwner.lock();

        if (pMouseOwner && pMouseOwner->getSysId())
        {
            bRes = true;

            pMouseOwner->onPointerUp();

            auto ptrSpr = findTrackedAt(fx, fy, NULL);

            if (ptrSpr && ptrSpr->getAncestorByAnySysId(true) == pMouseOwner.get() &&
                canPassEventsTo(pMouseOwner.get()))
            {
                bool bTutorRunning = TutorialController::getInstance()->isRunning();

                if (bTutorRunning)
                    TutorialController::getInstance()->onElementClick(pMouseOwner);

                onSysClick(pMouseOwner ? pMouseOwner.get() : nullptr);

                if (bTutorRunning)
                    TutorialController::getInstance()->onElementClickHandled(pMouseOwner);
            }

            if (pMouseOwner->getSysId() == SCROLLBAR_SYS_ID)
            {
                auto pScrollBar = std::dynamic_pointer_cast<ScrollBar>(pMouseOwner);
                if (pScrollBar)
                    pScrollBar->endSliderDrag();
            }

            _pMouseOwner.reset();
        }
    }
    break;

    case E_PA_WHEEL:
        break;
    }

    return bRes;
}

bool BaseDialog::onPointerDown(int32_t x, int32_t y)
{
    bool bRes = false;

    auto p = _pHover.lock();

    if (p)
    {
        onLeave(p.get());
        _pHover.reset();
    }

    bRes = onSysPointer(x, y, E_PA_DOWN);

    if (!bRes)
    {
        auto ptrTarget = findTrackedAt(x, y, getViewRectRecursive());

        if (_eScrollType != eScrollType::E_ST_NONE)
        {
            if (_root)
                _root->stopScrollAnimation();

            _isMouseDnInScrollBox = _root->getScrollBox().contains(x, y);
            _fDragStartPos        = _root->getScroll();

            _pMouseOwner.reset();

            if (ptrTarget)
            {
                auto p = ptrTarget->getAncestorByAnyId(true);

                if (p)
                    _pMouseOwner = p->shared_from_this();
            }

            auto p = _pMouseOwner.lock();

            if (p && canPassEventsTo(p.get()))
            {
                bRes = true;
                p->onPointerDown();
            }
        }
        else if (ptrTarget)
        {
            auto p = ptrTarget->getAncestorByAnyId(true);

            if (p)
                _pMouseOwner = p->shared_from_this();

            if (p && canPassEventsTo(p))
            {
                bRes = true;
                p->onPointerDown();
            }
        }
    }

    return bRes;
}

void BaseDialog::setHovered(CContainerPtr pHovered, bool bSysWidget)
{
    auto ptr = _pHover.lock();

    if (ptr)
    {
        if (pHovered != ptr)
        {
            onLeave(ptr.get(), bSysWidget);

            _pHover = pHovered;

            auto p = _pHover.lock();

            if (p)
            {
                onHover(p.get(), bSysWidget);
            }
            else
            {
                onHover(nullptr, bSysWidget);
            }
        }
    }
    else if (pHovered)
    {
        _pHover = pHovered;

        auto p = _pHover.lock();

        if (p)
            onHover(p.get(), bSysWidget);
        else
            onHover(nullptr, bSysWidget);
    }
}

bool BaseDialog::onPointerMove(int32_t x, int32_t y)
{
    bool bRes = onSysPointer(x, y, E_PA_MOVE);

    if (!bRes)
    {
        auto& cfg = Engine::getCfg();

        if (_eScrollType != eScrollType::E_ST_NONE)
        {
            if (_isMouseDnInScrollBox && !_isDrag && !TutorialController::getInstance()->isRunning() && _nWaitForTutorEvt < 0)
            {
                bRes = true;

                float fDrag = 0;

                bool isDragging = InputController::getInstance()->getDragDistance(fDrag);

                if (isDragging)
                {
                    if (isgreater(fDrag, cfg.DRAG_BEGIN_DISTANCE))
                    {
                        _fLastDragTime = TimeCounter::getTime();
                        _ptLastDrag.set(x, y);

                        _isDrag = onDragBegin(x, y);

                        if (_isDrag)
                            _pMouseOwner.reset();
                        else if (!_bCanDrag)
                        {
                            if (auto p = _pMouseOwner.lock())
                            {
                                p->onPointerMove(InputController::getInstance()->isMouseDown(), x, y);
                            }
                        }
                    }
                }
            }
            else if (_isDrag && !TutorialController::getInstance()->isRunning() && _nWaitForTutorEvt < 0)
            {
                bRes = true;

                float fDraggedX = 0.f;
                float fDraggedY = 0.f;

                InputController::getInstance()->getDragDistanceDprX(fDraggedX);
                InputController::getInstance()->getDragDistanceDprY(fDraggedY);

                onDragMove(fDraggedX, fDraggedY);

                if (TimeCounter::getTime() > _fLastDragTime + 0.3f)
                {
                    _fLastDragTime = TimeCounter::getTime();
                    _ptLastDrag.set(x, y);
                }
            }
        }
        else
        {
            auto pMouseOwner = _pMouseOwner.lock();

            if (pMouseOwner && !TutorialController::getInstance()->isRunning() && _nWaitForTutorEvt < 0)
            {
                float fDrag = 0;

                bool isDragging = InputController::getInstance()->getDragDistance(fDrag);

                if (isDragging)
                {
                    if (isgreater(fDrag, cfg.DRAG_BEGIN_DISTANCE))
                    {
                        _fLastDragTime = TimeCounter::getTime();
                        _ptLastDrag.set(x, y);

                        bRes = DragController::getInstance()->beginDrag(pMouseOwner, x, y);

                        if (bRes)
                            _pMouseOwner.reset();
                    }
                }
            }
        }

        if (!bRes)
        {
            bRes = updateHovered(x, y);
        }
    }

    return bRes;
}

bool BaseDialog::updateHovered(int32_t x, int32_t y)
{
    bool bRes = false;

    auto pMouseOwner = _pMouseOwner.lock();

    if (!pMouseOwner && !InputController::getInstance()->isMouseDown())
    {
        auto ptr = findTrackedAt(x, y, getViewRect());

        CContainer* pTarget = NULL;

        if (ptr)
        {
            pTarget = ptr->getAncestorByAnyId(true);
            bRes = canPassEventsTo(pTarget);
        }

        setHovered(bRes ? (pTarget ? pTarget->shared_from_this() : nullptr) : nullptr);

        if (bRes && pTarget)
        {
            pTarget->onPointerMove(InputController::getInstance()->isMouseDown(), x, y);
        }
    }
    else if (pMouseOwner)
    {
        if (canPassEventsTo(pMouseOwner.get()))
            pMouseOwner->onPointerMove(InputController::getInstance()->isMouseDown(), x, y);
    }
    else
    {
        auto ptr = findTrackedAt(x, y, getViewRect());

        CContainer* pTarget = NULL;

        if (ptr)
        {
            pTarget = ptr->getAncestorByAnyId(true);

            if (pTarget && canPassEventsTo(pTarget))
            {
                pTarget->onPointerMove(InputController::getInstance()->isMouseDown(), x, y);
            }
        }
    }

    return bRes;
}

void BaseDialog::checkElementRefs(CContainer* pElement)
{
    auto p = _pMouseOwner.lock();

    if (p.get() == pElement)
        _pMouseOwner.reset();

    auto pH = _pHover.lock();

    if (!pH || pH.get() == pElement)
    {
        setHovered(nullptr);
    }
}

CContainerPtr BaseDialog::findTrackedAt(float fx, float fy, const Rect* pCrop)
{
    assert(!_bDispatchOutOfClientRect || _eScrollType == eScrollType::E_ST_NONE);

    CContainerPtr ptrRes = findTrackedAtPoint(fx,
                                              fy,
                                              _bDispatchOutOfClientRect ? nullptr : pCrop,
                                              _bDispatchOutOfClientRect ? nullptr : this);

    if (ptrRes && _bDispatchOutOfClientRect)
    {
        auto ptrDlg = ptrRes->getParentDialog();

        if (ptrDlg && ptrDlg.get() != this)
        {
            ptrRes.reset();
        }
    }

    return ptrRes;
}

void BaseDialog::handleEmptyClick(int32_t x, int32_t y)
{
    switch (_eCloseStyle)
    {
    case eDialogCloseStyle::CLICK_ANYWHERE:
    {
        setTimeout(0, [this]
        {
            close();
        });
    }
    break;
    }
}

bool BaseDialog::onPointerUp(int32_t x, int32_t y)
{
    BaseDialogPtr ptrThis = getPtr();

    bool bWasDrag = ptrThis->_isDrag;
    bool bRes = ptrThis->onSysPointer(x, y, E_PA_UP);

    auto pMouseOwner = ptrThis->_pMouseOwner.lock();

    if (!bRes)
    {
        if (ptrThis->_eScrollType != eScrollType::E_ST_NONE)
        {
            if (ptrThis->_isDrag)
            {
                bRes = true;
                ptrThis->onDragEnd(x, y);
            }
            else if (pMouseOwner)
            {
                bRes = true;

                if (canPassEventsTo(pMouseOwner.get()))
                    pMouseOwner->onPointerUp();

                auto ptrSpr = findTrackedAt(x, y, ptrThis->getViewRect());

                if (ptrSpr && ptrSpr->getAncestorByAnyId(true) == pMouseOwner.get())
                {
                    if (TutorialController::getInstance()->isRunning() &&
                        canPassEventsTo(pMouseOwner.get()))
                    {
                        TutorialController::getInstance()->onElementClick(pMouseOwner);
                        ptrThis->onClick(pMouseOwner.get());
                        TutorialController::getInstance()->onElementClickHandled(pMouseOwner);
                    }
                    else if (!TutorialController::getInstance()->isRunning())
                    {
                        ptrThis->onClick(pMouseOwner.get());
                    }
                }
            }
            else
            {
                handleEmptyClick(x, y);
            }

            ptrThis->_isMouseDnInScrollBox = false;
            ptrThis->_pMouseOwner.reset();
            ptrThis->_isDrag               = false;
        }
        else if (pMouseOwner)
        {
            if (ptrThis->_isDrag)
            {
                bRes = true;
                ptrThis->_pMouseOwner.reset();
                ptrThis->updateHovered(x, y);
                ptrThis->_isDrag = false;
            }
            else
            {
                bRes = true;

                if (canPassEventsTo(pMouseOwner.get()))
                    pMouseOwner->onPointerUp();

                auto ptrSpr = findTrackedAt(x, y, ptrThis->getViewRect());

                if (TutorialController::getInstance()->isRunning() &&
                    canPassEventsTo(pMouseOwner.get()))
                {
                    TutorialController::getInstance()->onElementClick(pMouseOwner);
                    ptrThis->onClick(pMouseOwner.get());
                    TutorialController::getInstance()->onElementClickHandled(pMouseOwner);
                }
                else if (!TutorialController::getInstance()->isRunning() && canPassEventsTo(pMouseOwner.get()))
                {
                    ptrThis->onClick(pMouseOwner.get());
                }

                ptrThis->_pMouseOwner.reset();
                ptrThis->updateHovered(x, y);
            }
        }
        else
        {
            handleEmptyClick(x, y);
        }
    }

    if (!bWasDrag &&
        ptrThis->_eScrollType != eScrollType::E_ST_NONE &&
        ptrThis->_root)
    {
        ptrThis->_root->settleScroll();
    }

    return bRes;
}

bool BaseDialog::onDragBegin(int32_t x, int32_t y)
{
    return _bCanDrag && (_eScrollType != eScrollType::E_ST_NONE);
}

void BaseDialog::onDragMove(int32_t x, int32_t y)
{
    if (_eScrollType == eScrollType::E_ST_NONE || !_isDrag || !_root)
        return;

    _root->stopScrollAnimation();

    if (fabs(_root->getMaxScroll()) < 0.001f)
    {
        _root->scrollTo(0.f, 0.f);
        return;
    }

    const bool bHor = (_eScrollType == eScrollType::E_ST_HOR);

    float fDrag = bHor ? static_cast<float>(x) : static_cast<float>(y);

    // Ёкранна€ дельта -> локальна€ дельта скролла.
    fDrag = screenDeltaToScrollDelta(fDrag);

    float fDesired = _fDragStartPos + fDrag;
    float fMax     = _root->getMaxScroll();

    float fDpr = InputController::getInstance()->getLastDpr();

    if (fDpr <= 0.f)
        fDpr = CSceneResize::getInstance()->get_dpr();

    float fLimitScreen = 80.f;

    if (fDpr > 0.f)
        fLimitScreen *= fDpr;

    float fLimit = screenDeltaToScrollDelta(fLimitScreen);

    if (fLimit <= 0.f)
        fLimit = 80.f;

    if (fDesired > 0.f)
    {
        float fOver   = fDesired;
        float fResist = fLimit * (fOver / (fOver + fLimit));
        fDesired = fResist;
    }
    else if (fDesired < fMax)
    {
        float fOver   = fMax - fDesired;
        float fResist = fLimit * (fOver / (fOver + fLimit));
        fDesired = fMax - fResist;
    }

    _root->scrollTo(fDesired, fLimit);
}

void BaseDialog::onDragEnd(int32_t x, int32_t y)
{
    if (_eScrollType == eScrollType::E_ST_NONE || !_root)
        return;

    if (_root->isOverScroll())
    {
        _root->settleScroll();
        return;
    }

    float fStopTime = TimeCounter::getTime();
    float dt        = std::max(fStopTime - _fLastDragTime, 0.016f);

    float delta = (_eScrollType == eScrollType::E_ST_HOR)
                      ? static_cast<float>(x - _ptLastDrag.x)
                      : static_cast<float>(y - _ptLastDrag.y);

    // Ёкранна€ скорость -> локальна€ скорость скролла.
    delta = screenDeltaToScrollDelta(delta);

    float fSpeed = delta / dt;

    float fAdvance = fSpeed * -0.8f;

    if (fabs(fAdvance) > 0.5f)
    {
        _root->flingScrollByDelta(fAdvance);
    }
    else
    {
        _root->settleScroll();
    }
}

void BaseDialog::onSysClick(CContainer* pTarget)
{
    if (pTarget)
    {
        CContainer* p = pTarget->getAncestorByAnySysId(true);

        if (p)
        {
            if (TutorialController::getInstance()->getEvent() > -1)
                pTarget->setTutorialId(-1, false);

            p->onClick();
        }
    }
}

void BaseDialog::onClick(CContainer* pTarget)
{
    if (pTarget)
    {
        CContainer* p = pTarget->getAncestorByAnyId(true);

        assert(pTarget == p);

        if (p)
        {
            if (TutorialController::getInstance()->getEvent() > -1)
                pTarget->setTutorialId(-1, false);

            p->onClick();
        }
    }
}

void BaseDialog::onHover(CContainer* pTarget, bool bSysWidget)
{
    if (pTarget)
    {
        InputController::getInstance()->setMouseCursor(pTarget->getMouseCursorType());
    }

    if (pTarget)
    {
        CContainer* p = bSysWidget ? pTarget->getAncestorByAnySysId(true) : pTarget->getAncestorByAnyId(true);

        if (p && p->isVisible())
        {
            p->onHover();
        }
    }
}

void BaseDialog::onLeave(CContainer* pTarget, bool bSysWidget)
{
    InputController::getInstance()->setMouseCursor(eMouseCursorType::E_MCT_NORMAL);

    if (pTarget)
    {
        CContainer* p = bSysWidget ? pTarget->getAncestorByAnySysId(true) : pTarget->getAncestorByAnyId(true);

        if (p && p->isVisible())
        {
            p->onLeave();
        }
    }
}

void BaseDialog::onTutorialEventBegin(int nEvt)
{
}

void BaseDialog::onTutorialEventEnd(int nEvt)
{
    if (nEvt == _nWaitForTutorEvt)
    {
        _nWaitForTutorEvt = -1;
    }
}

void BaseDialog::onEvent(int nEvent, void* pData1, void* pData2, void* pData3)
{
    switch (nEvent)
    {
    case CSceneResize::EVT_RESOLUTION_CHANGED:
    {
        updateGlobalClip();
        _updateScaleAndPos();
    }
    break;

    case InputController::EVT_ON_MOUSE_DOWN:
    {
        int32_t data1 = static_cast<int32_t>(reinterpret_cast<intptr_t>(pData1));
        int32_t data2 = static_cast<int32_t>(reinterpret_cast<intptr_t>(pData2));

        dispatchEvents(data1, data2, E_PA_DOWN);
    }
    break;

    case InputController::EVT_ON_MOUSE_MOVE:
    {
        int32_t data1 = static_cast<int32_t>(reinterpret_cast<intptr_t>(pData1));
        int32_t data2 = static_cast<int32_t>(reinterpret_cast<intptr_t>(pData2));

        dispatchEvents(data1, data2, E_PA_MOVE);
    }
    break;

    case InputController::EVT_ON_MOUSE_UP:
    {
        int32_t data1 = static_cast<int32_t>(reinterpret_cast<intptr_t>(pData1));
        int32_t data2 = static_cast<int32_t>(reinterpret_cast<intptr_t>(pData2));

        dispatchEvents(data1, data2, E_PA_UP);
    }
    break;

    case InputController::EVT_ON_MOUSE_WHEEL:
    {
        int32_t data1 = static_cast<int32_t>(reinterpret_cast<intptr_t>(pData1));
        int32_t data2 = static_cast<int32_t>(reinterpret_cast<intptr_t>(pData2));

        dispatchEvents(data1, data2, E_PA_WHEEL);
    }
    break;

    case TutorialController::EVT_ON_START_EVENT:
    {
        int evt = static_cast<int>(reinterpret_cast<intptr_t>(pData1));
        onTutorialEventBegin(evt);
    }
    break;

    case TutorialController::EVT_ON_COMPLETE_EVENT:
    {
        int evt = static_cast<int>(reinterpret_cast<intptr_t>(pData1));
        onTutorialEventEnd(evt);
    }
    break;

    case TutorialController::EVT_ON_CANCEL_EVENT:
    {
        int evt = static_cast<int>(reinterpret_cast<intptr_t>(pData1));
        _nWaitForTutorEvt = -1;
    }
    break;
    }
}

bool BaseDialog::dispatchEvents(int32_t x, int32_t y, ePointerAction eAction)
{
    bool bRes = false;

    auto dispatchCopy = _dispatchTo;

    BaseDialogPtr ptrHandled;

    for (BaseDialogPtr ptr : dispatchCopy)
    {
        if (ptr->isVisible())
            bRes = ptr->dispatchEvents(x, y, eAction);

        if (bRes)
        {
            ptrHandled = ptr;
            break;
        }
    }

    if (bRes && ptrHandled)
    {
        for (BaseDialogPtr ptr : dispatchCopy)
        {
            if (ptr != ptrHandled)
                ptr->setHovered(nullptr);
        }
    }

    if (!bRes)
    {
        switch (eAction)
        {
        case E_PA_DOWN:
            bRes = onPointerDown(x, y);
            break;

        case E_PA_UP:
            bRes = onPointerUp(x, y);
            break;

        case E_PA_MOVE:
            bRes = onPointerMove(x, y);
            break;

        case E_PA_WHEEL:
            bRes = onMouseWheel(x);
            break;

        default:
            assert(false);
            break;
        }
    }

    return bRes;
}

void BaseDialog::addDispatchedDlg(BaseDialogPtr pDispatchTo)
{
    _dispatchTo.insert(pDispatchTo);
}

void BaseDialog::removeDispatchedDlg(BaseDialogPtr pDispatchTo)
{
    _dispatchTo.erase(pDispatchTo);
}

void BaseDialog::setScale(float sx, float sy)
{
    CContainer::setScale(sx, sy);

    if (_eScrollType != eScrollType::E_ST_NONE)
        updateScrollBox();
}

void BaseDialog::setPos(float fx, float fy)
{
    CContainer::setPos(fx, fy);

    if (_eScrollType != eScrollType::E_ST_NONE)
        updateScrollBox();
}

void BaseDialog::setX(float x)
{
    CContainer::setX(x);

    if (_eScrollType != eScrollType::E_ST_NONE)
        updateScrollBox();
}

void BaseDialog::setY(float y)
{
    CContainer::setY(y);

    if (_eScrollType != eScrollType::E_ST_NONE)
        updateScrollBox();
}

void BaseDialog::setScaleX(float fScale)
{
    CContainer::setScaleX(fScale);

    if (_eScrollType != eScrollType::E_ST_NONE)
        updateScrollBox();
}

void BaseDialog::setScaleY(float fScale)
{
    CContainer::setScaleY(fScale);

    if (_eScrollType != eScrollType::E_ST_NONE)
        updateScrollBox();
}

void BaseDialog::setVisible(bool bVisible)
{
    if (!bVisible)
    {
        setHovered(nullptr);
        _pMouseOwner.reset();

        if (_ptrScollBar)
            _ptrScollBar->endSliderDrag();

        if (_root)
            _root->stopScrollAnimation();
    }

    if (_ptrBehindBg)
        _ptrBehindBg->setVisible(bVisible);

    CContainer::setVisible(bVisible);
}


_G2D_NAMESPACE_END_