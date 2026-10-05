#include "tutorialcontroller.h"
#include "spriteloader.h"
#include "gfx.h"
#include "dialogs/basedialog.h"
#include "engine.h"

_G2D_NAMESPACE_BEGIN_

void TutorialLayer::update(float dt)
{
    CContainer::update(dt);
    TutorialController::getInstance()->updatePos();
}

TutorialController::TutorialController()
{
}

void TutorialController::init()
{
    _ptrSprPointer = SpriteLoader::getInstance()->getSprite("UI/iconTutorHand");
    _ptrLayer      = std::make_shared<TutorialLayer>();
    _ptrLayer->setIgnoreTutorialBox(true);
    _ptrLayer->addChild(_ptrSprPointer);
    _ptrSprPointer->setIgnoreTutorialBox(true);
}

void TutorialController::cancelAll(void)
{
    if (!_q.empty())
    {
        releaseElement();
        while (!_q.empty())
            _q.pop();
    }
    _nCurrenEvent  = -1;
    _nScenario     = -1;

    emit(EVT_ON_CANCEL_EVENT);
}

void TutorialController::addElements(std::span<int> elements)
{
    assert(_q.empty());
    assert(!_frontShown);
    if (_q.empty())
    {
        for (auto it:elements)
            _q.push(it);
    }
    assert(!_q.empty());
    _ptrSprPointer->setVisible(false);
    CGfx::getInstance()->getGameRoot()->addChild(_ptrLayer);
    if (!_q.empty())
        emit(EVT_ON_START_EVENT, (void*)_q.front());
}

void TutorialController::addElement(int nTutorialId)
{
    assert(nTutorialId > -1);
    if (_q.empty())
    {
        std::array<int, 1> arr = { nTutorialId };
        addElements(arr);
    }
    else
    {
        _q.push(nTutorialId);
    }
}

void TutorialController::updatePos()
{
    if (!_q.empty() && _ptrTutorialHost)
    {
        auto& cfg = Engine::getCfg();
        Rect bounds;
        bool bFirstUnite = true;
        _ptrTutorialHost->calcInteractiveBounds(&bounds, bFirstUnite);        
        auto ptrRoot = _ptrTutorialHost->getRoot().get();
        if ( ptrRoot == CGfx::getInstance()->getGameRoot() || 
             ptrRoot == CGfx::getInstance()->getGameIface())
            bounds.offset(-CSceneResize::getInstance()->getGameOffsX(), -CSceneResize::getInstance()->getGameOffsY());

        _calcedX = bounds.x + (bounds.cx * 0.5f);
        _calcedY = bounds.y + (bounds.cy * 0.5f);
        
        float cx = CSceneResize::getInstance()->getGameWidth();
        float cy = CSceneResize::getInstance()->getGameHeight();
        float fScaleX = cx / cfg.INIT_SCR_CX;
        float fScaleY = cy / cfg.INIT_SCR_CY;
        float fMinScale = std::min(fScaleX, fScaleY);
        _dx = -40 * fMinScale;
        _dy = -40 * fMinScale;
        _ptrSprPointer->setScale(_ptrSprPointer->isFlippedX() ? -fMinScale : fMinScale, 
            _ptrSprPointer->isFlippedY() ? -fMinScale : fMinScale);
        if (_ptrTutorialHost->getParentModalDialog() != BaseDialog::getTopModal())
        {
            _ptrSprPointer->setVisible(false);
        }
        else
        {
            _ptrSprPointer->setVisible(true);
            _ptrLayer->getParent()->bringChildToFront(_ptrLayer.get());        
        }
    }
}

void TutorialController::onElementRendered(CContainer* ptrRender)
{
    if (!_frontShown && !_q.empty() && _q.front() == ptrRender->getTutorialId() && ptrRender->isReadyForTutorial())
    {
        //if (ptrRender->getParentModalDialog() == BaseDialog::getTopModal())
        {
            _ptrTutorialHost = ptrRender->shared_from_this();
            showFront();
        }
    }
}

void TutorialController::showFront()
{
    if (!_q.empty())
    {
        if (_ptrSprPointer->isFlippedY())
            _ptrSprPointer->flipY();
        if (_ptrSprPointer->isFlippedX())
            _ptrSprPointer->flipX();
        assert(!_frontShown);
        _frontShown = true;
        updatePos();
        float fBottom = _calcedY + (_ptrSprPointer->getNotTransCy() * _ptrSprPointer->getScaleY());
        float fRight  = _calcedX + (_ptrSprPointer->getNotTransCx() * _ptrSprPointer->getScaleX());
        if (fBottom > CSceneResize::getInstance()->getGameHeight()) 
        {
            _calcedY -= _ptrSprPointer->getNotTransCy();
            _ptrSprPointer->flipY();
        }

        if (fRight > CSceneResize::getInstance()->getGameWidth()) 
        {
            _calcedX -= _ptrSprPointer->getNotTransCx();
            _ptrSprPointer->flipX();
        }

        _ptrSprPointer->setVisible(true);
        _ptrSprPointer->setX(_calcedX);
        _ptrSprPointer->setY(_calcedY);
        if (_ptrTutorialHost)
        {
            auto ptrRoot = _ptrTutorialHost->getRoot();
            if (_ptrLayer->getRoot() != ptrRoot)
            {
                _ptrLayer->removeFromParent();
                ptrRoot->addChild(_ptrLayer);
            }
        }
        CGfx::getInstance()->setTutorialElips(_ptrTutorialHost);
        _ptrLayer->getParent()->bringChildToFront(_ptrLayer.get());
        animatePointer();
    }
}

void TutorialController::releaseElement(bool bSilent)
{
    assert(!_q.empty());
    if (!_q.empty())
    {
        _ptrSprPointer->removeSelfTweens();
        CGfx::getInstance()->setTutorialElips(nullptr);
        _ptrSprPointer->setVisible(false);
        _ptrTutorialHost = nullptr;
        _frontShown      = false;

        if (!bSilent)
        {
            auto completedEvt = _q.front();
            _q.pop();

            if (_q.empty())
                _ptrLayer->removeFromParent();
            emit(EVT_ON_COMPLETE_EVENT, (void*)completedEvt);

            if (!_q.empty())
                emit(EVT_ON_START_EVENT, (void*)_q.front());
        }
    }
}

bool TutorialController::isElementActive(CContainer* ptrElement)
{
    if (_q.empty())
        return true;
    else if (_ptrTutorialHost && _ptrTutorialHost.get() == ptrElement)
        return true;
    else if (_ptrTutorialHost && (_ptrTutorialHost->getParentModalDialog() != BaseDialog::getTopModal()))
        return true;
    else if (_ptrTutorialHost && _ptrTutorialHost->isTutorialIgnored())
        return true;
    else return false;
}

void TutorialController::onElementClick(CContainerPtr ptrElement)
{
    if (ptrElement && ptrElement->getTutorialId() == _q.front())
    {
        _nCurrenEvent = _q.front();
        releaseElement();    
    }
}

void TutorialController::onElementClickHandled(CContainerPtr ptrElement)
{
    if (ptrElement && _q.size() && ptrElement->getTutorialId() == _q.front())
        _nCurrenEvent = -1;
}

void TutorialController::animatePointer()
{
    static constexpr float animTime = 0.5f;
    float fToX = _ptrSprPointer->isFlippedX() ? _calcedX + _dx : _calcedX - _dx;
    float fToY = _ptrSprPointer->isFlippedY() ? _calcedY + _dy : _calcedY - _dy;
    _ptrSprPointer->addSelfTween(eTweenProp::Y, _calcedY, fToY, animTime, Easing::linear, 0.f);
    _ptrSprPointer->addSelfTween(eTweenProp::X, _calcedX, fToX, animTime, Easing::linear, 0.f, [this](){
        _ptrSprPointer->removeSelfTweens();
        float fToX = _calcedX;
        float fToY = _calcedY;
        _ptrSprPointer->addSelfTween(eTweenProp::Y, _ptrSprPointer->getY(), fToY, animTime, Easing::linear, 0.f);
        _ptrSprPointer->addSelfTween(eTweenProp::X, _ptrSprPointer->getX(), fToX, animTime, Easing::linear, 0.f, [this](){
            _ptrSprPointer->removeSelfTweens();
            animatePointer();
        });
    });
}

_G2D_NAMESPACE_END_