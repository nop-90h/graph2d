#include "pch.h"
#include "cardreveal.h"
#include "spriteloader.h"
#include "particle.h"
#include "particlepresets.h"

namespace
{
    constexpr float kPi = 3.14159265358979323846f;

    float clamp01(float t)
    {
        return std::clamp(t, 0.f, 1.f);
    }

    float easeOutCubic(float t)
    {
        t = clamp01(t);
        float u = 1.f - t;
        return 1.f - u * u * u;
    }

    float easeInCubic(float t)
    {
        t = clamp01(t);
        return t * t * t;
    }

    float easeOutQuint(float t)
    {
        t = clamp01(t);
        float u = 1.f - t;
        return 1.f - u * u * u * u * u;
    }

    // Soft overshoot, much gentler than classic easeOutBack.
    float easeOutSoftBack(float t)
    {
        t = clamp01(t);

        const float c1 = 1.2f;
        const float c3 = c1 + 1.f;

        float x = t - 1.f;
        return 1.f + c3 * x * x * x + c1 * x * x;
    }
}

CCardReveal::CCardReveal(float fSceneCx, float fSceneCy)
{
    setSceneSize(fSceneCx, fSceneCy);
}

void CCardReveal::setSceneSize(float fSceneCx, float fSceneCy)
{
    _fSceneCx = std::max(1.f, fSceneCx);
    _fSceneCy = std::max(1.f, fSceneCy);
    _bPrepared = false;
}

std::weak_ptr<CCardReveal> CCardReveal::getWeakThis()
{
    return std::static_pointer_cast<CCardReveal>(shared_from_this());
}

void CCardReveal::clearCards()
{
    for (auto& card : _vCards)
    {
        if (card.ptrRoot)
        {
            card.ptrRoot->removeAll();
            removeChild(card.ptrRoot);
        }
    }

    _vCards.clear();
    _bPrepared = false;
}

void CCardReveal::setCard(int nIndex, CSpritePtr ptrFace, CSpritePtr ptrBack)
{
    if (nIndex < 0 || nIndex >= CARD_COUNT)
        return;

    if (!ptrFace || !ptrBack)
        return;

    if ((int)_vCards.size() <= nIndex)
        _vCards.resize(nIndex + 1);

    auto& card = _vCards[nIndex];

    if (card.ptrRoot)
    {
        card.ptrRoot->removeAll();
        removeChild(card.ptrRoot);
    }

    card = CardItem();

    card.ptrFace = ptrFace;
    card.ptrBack = ptrBack;

    if (card.ptrFace->getParent())
        card.ptrFace->removeFromParent();

    if (card.ptrBack->getParent())
        card.ptrBack->removeFromParent();

    centerSprite(card.ptrFace);
    centerSprite(card.ptrBack);

    card.ptrRoot = std::make_shared<CContainer>();

    card.ptrBack->setVisible(true);
    card.ptrFace->setVisible(false);

    card.ptrRoot->addChild(card.ptrBack);
    card.ptrRoot->addChild(card.ptrFace);
    addChild(card.ptrRoot);

    card.bInit = true;
    _bPrepared = false;
}

void CCardReveal::setCards(const CardsArray& vFaces,
                           const CardsArray& vBacks)
{
    clearCards();

    int nFaces = (int)vFaces.size();
    int nBacks = (int)vBacks.size();
    int nCount = std::min(CARD_COUNT, std::min(nFaces, nBacks));

    for (int i = 0; i < nCount; ++i)
        setCard(i, vFaces[i], vBacks[i]);
}

void CCardReveal::centerSprite(CSpritePtr ptrSprite)
{
    if (!ptrSprite)
        return;

    ptrSprite->removeSelfTweens();
    ptrSprite->setScale(1.f, 1.f);
    ptrSprite->setAlpha(1.f);
    ptrSprite->setAddRgba(0.f, 0.f, 0.f, 0.f);

    Rect rc;
    if (!ptrSprite->getNotTransBounds(&rc))
        ptrSprite->calcNotTransBounds(&rc);

    if (rc.cx <= 0.f || rc.cy <= 0.f)
        return;

    ptrSprite->setPos(-rc.x - rc.cx * 0.5f,
                      -rc.y - rc.cy * 0.5f);
}

void CCardReveal::layout()
{
    _fCardW = 0.f;
    _fCardH = 0.f;

    int nValid = 0;

    for (auto& card : _vCards)
    {
        if (!card.bInit)
            continue;

        ++nValid;

        Rect rc;

        if (card.ptrFace)
        {
            if (!card.ptrFace->getNotTransBounds(&rc))
                card.ptrFace->calcNotTransBounds(&rc);

            _fCardW = std::max(_fCardW, rc.cx);
            _fCardH = std::max(_fCardH, rc.cy);
        }

        if (card.ptrBack)
        {
            if (!card.ptrBack->getNotTransBounds(&rc))
                card.ptrBack->calcNotTransBounds(&rc);

            _fCardW = std::max(_fCardW, rc.cx);
            _fCardH = std::max(_fCardH, rc.cy);
        }
    }

    if (nValid == 0)
        return;

    if (_fCardW <= 0.f)
        _fCardW = 120.f;

    if (_fCardH <= 0.f)
        _fCardH = 180.f;

    const float fPad = _fCardW * 0.18f;
    const float fTotalUnscaled = (float)nValid * _fCardW + (float)(nValid - 1) * fPad;

    const float fAvailW = _fSceneCx * 0.92f;
    const float fAvailH = _fSceneCy * 0.60f;

    _fCardScale = 1.f;

    if (fTotalUnscaled > 0.f)
        _fCardScale = std::min(_fCardScale, fAvailW / fTotalUnscaled);

    if (_fCardH > 0.f)
        _fCardScale = std::min(_fCardScale, fAvailH / _fCardH);

    _fCardScale = std::max(_fCardScale, 0.05f);

    const float fTotalW = fTotalUnscaled * _fCardScale;
    const float fStartX = (_fSceneCx - fTotalW) * 0.5f + (_fCardW * _fCardScale) * 0.5f;
    const float fTargetY = _fSceneCy * 0.5f;

    int nLayoutIdx = 0;

    for (auto& card : _vCards)
    {
        if (!card.bInit)
            continue;

        card.fTargetX = fStartX + (float)nLayoutIdx * (_fCardW + fPad) * _fCardScale;
        card.fTargetY = fTargetY;

        ++nLayoutIdx;
    }

    _bPrepared = true;
}

void CCardReveal::prepareCard(int nIndex)
{
    if (nIndex < 0 || nIndex >= (int)_vCards.size())
        return;

    auto& card = _vCards[nIndex];

    if (!card.bInit || !card.ptrRoot || !card.ptrFace || !card.ptrBack)
        return;

    card.ptrRoot->removeSelfTweens();
    card.ptrFace->removeSelfTweens();
    card.ptrBack->removeSelfTweens();

    card.ptrBack->setVisible(true);
    card.ptrFace->setVisible(false);

    card.ptrBack->setAlpha(1.f);
    card.ptrFace->setAlpha(1.f);

    card.ptrBack->setAddRgba(0.f, 0.f, 0.f, 0.f);
    card.ptrFace->setAddRgba(0.f, 0.f, 0.f, 0.f);

    card.ptrRoot->setPos(card.fTargetX, card.fTargetY);
    card.ptrRoot->setScale(_fCardScale, _fCardScale);
    card.ptrRoot->setAlpha(1.f);
    card.ptrRoot->rotate(0.f);
}

void CCardReveal::reset()
{
    if (!_bPrepared)
        layout();

    removeSelfTweens();

    for (int i = 0; i < (int)_vCards.size(); ++i)
    {
        if (_vCards[i].bInit)
            prepareCard(i);
    }
}

void CCardReveal::play(SimpleCallback onComplete)
{
    if (_vCards.empty())
        return;

    _cb = onComplete;

    if (!_bPrepared)
        layout();
    
    removeSelfTweens();

    int nDealIdx = 0;

    const float fBaseDelay = 0.25f;
    const float fStagger = 0.28f;

    for (int i = (int)_vCards.size() - 1; i >= 0; --i)
    {
        if (!_vCards[i].bInit)
            continue;

        prepareCard(i);

        const float fEntranceDelay = fBaseDelay + (float)nDealIdx * fStagger;
        const float fEntranceDur   = getEntranceDuration(i);

        animateEntrance(i, fEntranceDelay);

        const float fFlipDelay = fEntranceDelay + fEntranceDur + 0.22f;

        scheduleFlip(i, fFlipDelay);

        ++nDealIdx;
    }
}

void CCardReveal::update(float dt)
{
    if (dt > 0.133f)
        dt = 0.133f;

    CContainer::update(dt);
}

float CCardReveal::getEntranceDuration(int nIndex) const
{
    switch (nIndex % CARD_COUNT)
    {
        case 0:  return 1.20f;
        case 1:  return 0.82f;
        case 2:  return 0.92f;
        case 3:  return 0.92f;
        case 4:  return 0.82f;
        default: return 0.85f;
    }
}

void CCardReveal::animateEntrance(int nIndex, float fDelay)
{
    if (nIndex < 0 || nIndex >= (int)_vCards.size())
        return;

    auto& card = _vCards[nIndex];

    if (!card.bInit || !card.ptrRoot)
        return;

    if (!_bPrepared)
        layout();

    auto root = card.ptrRoot;

    const float fTargetX = card.fTargetX;
    const float fTargetY = card.fTargetY;
    const float fScale = _fCardScale;
    const float fDur = getEntranceDuration(nIndex);

    switch (nIndex % CARD_COUNT)
    {
        // ------------------------------------------------------------
        // Final card: falls from above like a 3D plane, then splash.
        // ------------------------------------------------------------
        case 0:
        {
            const float fFallDur = fDur * 0.68f;

            const float fStartX = fTargetX + _fCardW * fScale * 0.12f;
            const float fStartY = -_fCardH * fScale * 2.25f;

            const float fStartRot = 0.95f;
            const float fStartScaleX = fScale * 0.78f;
            const float fStartScaleY = fScale * 0.16f;

            const float fCW = _fCardW;
            const float fCH = _fCardH;

            root->setPos(fStartX, fStartY);
            root->setAlpha(0.f);
            root->setScale(fStartScaleX, fStartScaleY);
            root->rotate(fStartRot);

            std::weak_ptr<CContainer> weakRoot = root;

            root->addSelfTween(eTweenProp::Y,
                               fStartY, fTargetY,
                               fFallDur, easeInCubic, fDelay,
                               [this, weakRoot, fScale, fCW, fCH]()
            {
                auto pRoot = weakRoot.lock();
                if (!pRoot)
                    return;
                auto ptrSettings = ParticlePresets::getPreset(eParticlePreset::STEAM);
                ptrSettings.gravityX = 0;
                ptrSettings.gravityY = 0;
                 ptrSettings.spawnRate = 0;
                auto ptrParticles = std::make_shared<CParticleSystem<>>(ptrSettings, std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::STEAM)).c_str());        
                pRoot->addChild(ptrParticles);
                ptrParticles->burst(400);
                ptrParticles->setPos(-150, -120);
                pRoot->bringChildToBack(ptrParticles.get());
                // Landing settle.
                pRoot->addSelfTween(eTweenProp::ROTATE,
                                    0.16f, 0.f,
                                    0.30f, easeOutCubic, 0.005f);

                pRoot->addSelfTween(eTweenProp::SCALE_Y,
                                    fScale * 0.90f, fScale,
                                    0.34f, easeOutSoftBack, 0.005f);
            });

            root->addSelfTween(eTweenProp::X,
                               fStartX, fTargetX,
                               fFallDur, easeOutCubic, fDelay);

            root->addSelfTween(eTweenProp::ALPHA,
                               0.f, 1.f,
                               0.22f, easeOutCubic, fDelay);

            root->addSelfTween(eTweenProp::ROTATE,
                               fStartRot, 0.16f,
                               fFallDur, easeOutCubic, fDelay);

            root->addSelfTween(eTweenProp::SCALE_Y,
                               fStartScaleY, fScale * 0.90f,
                               fFallDur, easeOutCubic, fDelay);

            root->addSelfTween(eTweenProp::SCALE_X,
                               fStartScaleX, fScale,
                               fFallDur, easeOutCubic, fDelay);
        }
        break;

        // ------------------------------------------------------------
        // Left-side entrance.
        // ------------------------------------------------------------
        case 1:
        {
            const float fStartX = -_fCardW * fScale * 1.45f;
            const float fStartY = fTargetY + _fCardH * fScale * 0.18f;

            const float fStartRot = -1.05f;
            const float fStartScale = fScale * 0.82f;

            root->setPos(fStartX, fStartY);
            root->setAlpha(0.f);
            root->setScale(fStartScale, fStartScale);
            root->rotate(fStartRot);

            root->addSelfTween(eTweenProp::X,
                               fStartX, fTargetX,
                               fDur, easeOutQuint, fDelay);

            root->addSelfTween(eTweenProp::Y,
                               fStartY, fTargetY,
                               fDur, easeOutCubic, fDelay);

            root->addSelfTween(eTweenProp::ALPHA,
                               0.f, 1.f,
                               0.18f, easeOutCubic, fDelay);

            root->addSelfTween(eTweenProp::ROTATE,
                               fStartRot, 0.f,
                               fDur * 0.95f, easeOutSoftBack, fDelay);

            root->addSelfTween(eTweenProp::SCALE,
                               fStartScale, fScale,
                               fDur, easeOutSoftBack, fDelay);
        }
        break;

        // ------------------------------------------------------------
        // Top entrance.
        // ------------------------------------------------------------
        case 2:
        {
            const float fStartX = fTargetX + _fCardW * fScale * 0.24f;
            const float fStartY = -_fCardH * fScale * 1.45f;

            const float fStartRot = 0.65f;
            const float fStartScale = fScale * 0.84f;

            root->setPos(fStartX, fStartY);
            root->setAlpha(0.f);
            root->setScale(fStartScale, fStartScale);
            root->rotate(fStartRot);

            root->addSelfTween(eTweenProp::Y,
                               fStartY, fTargetY,
                               fDur, easeOutQuint, fDelay);

            root->addSelfTween(eTweenProp::X,
                               fStartX, fTargetX,
                               fDur * 0.80f, easeOutCubic, fDelay);

            root->addSelfTween(eTweenProp::ALPHA,
                               0.f, 1.f,
                               0.20f, easeOutCubic, fDelay);

            root->addSelfTween(eTweenProp::ROTATE,
                               fStartRot, 0.f,
                               fDur * 0.90f, easeOutSoftBack, fDelay);

            root->addSelfTween(eTweenProp::SCALE,
                               fStartScale, fScale,
                               fDur, easeOutSoftBack, fDelay);
        }
        break;

        // ------------------------------------------------------------
        // Bottom entrance.
        // ------------------------------------------------------------
        case 3:
        {
            const float fStartX = fTargetX - _fCardW * fScale * 0.24f;
            const float fStartY = _fSceneCy + _fCardH * fScale * 1.45f;

            const float fStartRot = -0.65f;
            const float fStartScale = fScale * 0.84f;

            root->setPos(fStartX, fStartY);
            root->setAlpha(0.f);
            root->setScale(fStartScale, fStartScale);
            root->rotate(fStartRot);

            root->addSelfTween(eTweenProp::Y,
                               fStartY, fTargetY,
                               fDur, easeOutQuint, fDelay);

            root->addSelfTween(eTweenProp::X,
                               fStartX, fTargetX,
                               fDur * 0.80f, easeOutCubic, fDelay);

            root->addSelfTween(eTweenProp::ALPHA,
                               0.f, 1.f,
                               0.20f, easeOutCubic, fDelay);

            root->addSelfTween(eTweenProp::ROTATE,
                               fStartRot, 0.f,
                               fDur * 0.90f, easeOutSoftBack, fDelay);

            root->addSelfTween(eTweenProp::SCALE,
                               fStartScale, fScale,
                               fDur, easeOutSoftBack, fDelay);
        }
        break;

        // ------------------------------------------------------------
        // Right-side entrance.
        // ------------------------------------------------------------
        case 4:
        default:
        {
            const float fStartX = _fSceneCx + _fCardW * fScale * 1.45f;
            const float fStartY = fTargetY - _fCardH * fScale * 0.16f;

            const float fStartRot = 1.05f;
            const float fStartScale = fScale * 0.82f;

            root->setPos(fStartX, fStartY);
            root->setAlpha(0.f);
            root->setScale(fStartScale, fStartScale);
            root->rotate(fStartRot);

            root->addSelfTween(eTweenProp::X,
                               fStartX, fTargetX,
                               fDur, easeOutQuint, fDelay);

            root->addSelfTween(eTweenProp::Y,
                               fStartY, fTargetY,
                               fDur, easeOutCubic, fDelay);

            root->addSelfTween(eTweenProp::ALPHA,
                               0.f, 1.f,
                               0.18f, easeOutCubic, fDelay);

            root->addSelfTween(eTweenProp::ROTATE,
                               fStartRot, 0.f,
                               fDur * 0.95f, easeOutSoftBack, fDelay);

            root->addSelfTween(eTweenProp::SCALE,
                               fStartScale, fScale,
                               fDur, easeOutSoftBack, fDelay);
        }
        break;
    }
}

void CCardReveal::scheduleFlip(int nIndex, float fDelay)
{
    if (nIndex < 0 || nIndex >= (int)_vCards.size())
        return;

    auto& card = _vCards[nIndex];

    if (!card.bInit || !card.ptrRoot)
        return;

    auto root = card.ptrRoot;

    std::weak_ptr<CCardReveal> weakSelf = getWeakThis();
    std::weak_ptr<CContainer>  weakRoot = root;

    root->setTimeout(fDelay,
                     [weakSelf, weakRoot, nIndex]()
    {
        auto pThis = weakSelf.lock();
        auto pRoot = weakRoot.lock();

        if (!pThis || !pRoot)
            return;

        pThis->flipCardNow(nIndex);
    });
}

void CCardReveal::flipCardNow(int nIndex)
{
    if (nIndex < 0 || nIndex >= (int)_vCards.size())
        return;

    auto& card = _vCards[nIndex];

    if (!card.bInit || !card.ptrRoot)
        return;

    if (!_bPrepared)
        layout();

    auto root = card.ptrRoot;

    root->setPos(card.fTargetX, card.fTargetY);
    root->setScale(_fCardScale, _fCardScale);
    root->setAlpha(1.f);
    root->rotate(0.f);

    const float fLift = _fCardH * _fCardScale * 0.045f;

    std::weak_ptr<CCardReveal> weakSelf = getWeakThis();
    std::weak_ptr<CContainer>  weakRoot = root;

    root->addSelfTween(eTweenProp::Y,
                       card.fTargetY, card.fTargetY - fLift,
                       0.14f, easeOutCubic, 0.f);

    root->addSelfTween(eTweenProp::SCALE_Y,
                       _fCardScale, _fCardScale * 1.05f,
                       0.14f, easeOutCubic, 0.f);

    root->addSelfTween(eTweenProp::ROTATE,
                       0.f, 0.035f,
                       0.14f, easeOutCubic, 0.f);

    root->addSelfTween(eTweenProp::SCALE_X,
                       _fCardScale, 0.f,
                       0.16f, easeInCubic, 0.f,
                       [weakSelf, weakRoot, nIndex]()
    {
        auto pThis = weakSelf.lock();
        auto pRoot = weakRoot.lock();

        if (!pThis || !pRoot)
            return;

        pThis->onFlipMidpoint(nIndex);
    });
}

void CCardReveal::onFlipMidpoint(int nIndex)
{
    if (nIndex < 0 || nIndex >= (int)_vCards.size())
        return;

    auto& card = _vCards[nIndex];

    if (!card.bInit || !card.ptrRoot || !card.ptrFace || !card.ptrBack)
        return;

    if (!_bPrepared)
        layout();

    auto root = card.ptrRoot;

    const float fLift = _fCardH * _fCardScale * 0.045f;
    if (nIndex == 0)
    {
        auto& ptrSettings = ParticlePresets::getPreset(eParticlePreset::FIREWORKS);
        auto ptrParticles = std::make_shared<CParticleSystem<>>(ptrSettings, std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::FIREWORKS)).c_str());        
        card.ptrRoot->addChild(ptrParticles);
        ptrParticles->burst(400, _cb);    
        ptrParticles->setPos(-50, -100);
    }

    card.ptrBack->setVisible(false);
    card.ptrFace->setVisible(true);

    card.ptrFace->setAlpha(0.75f);
    card.ptrFace->addSelfTween(eTweenProp::ALPHA,
                               0.75f, 1.f,
                               0.22f, easeOutCubic, 0.f);

    root->addSelfTween(eTweenProp::SCALE_X,
                       0.f, _fCardScale,
                       0.28f, easeOutSoftBack, 0.f);

    root->addSelfTween(eTweenProp::SCALE_Y,
                       root->getScaleY(), _fCardScale,
                       0.24f, easeOutCubic, 0.f);

    root->addSelfTween(eTweenProp::ROTATE,
                       root->getRotate(), 0.f,
                       0.24f, easeOutCubic, 0.f);

    root->addSelfTween(eTweenProp::Y,
                       card.fTargetY - fLift, card.fTargetY,
                       0.26f, easeOutCubic, 0.f);
}