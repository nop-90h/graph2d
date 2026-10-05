#include "spinecont.h"
#include "sprite.h"
#include "m3.h"

#include <cassert>
#include <cstring>
#include <format>
#include <iostream>

_G2D_NAMESPACE_BEGIN_

CSpine::CSpine(spine::SkeletonDrawablePtr ptr, LPCTSTR lpccSpinePath)
    : _strPath(lpccSpinePath)
    , _spinePtr(ptr)
{
    m3::identity(_mat);
}

CSpine::~CSpine()
{
    spine::AnimationStateListenerObject* pNull = NULL;

    _isInitDone = true;

    if (_spinePtr)
        _spinePtr->animationState->setListener(pNull);
}

void CSpine::applyTransform(CMatrixStack* pMS, bool bForce)
{
    CContainer::applyTransform(pMS, bForce);

    const Rect* rcInteractive =
        _bCustomInteractiveRc
            ? &_rcInterctiveCustom
            : (_spinePtr && _spinePtr->hasInteractiveBox()
                   ? _spinePtr->getInteractiveBox()
                   : nullptr);

    if (rcInteractive)
    {
        static float vec3LU[3] = {0};
        static float vec3RB[3] = {0};

        vec3LU[0] = rcInteractive->x;
        vec3LU[1] = rcInteractive->y;
        vec3LU[2] = 1.0f;

        vec3RB[0] = rcInteractive->right();
        vec3RB[1] = rcInteractive->bottom();
        vec3RB[2] = 1.0f;

        pMS->vecMultiply(vec3LU);
        pMS->vecMultiply(vec3RB);

        CSceneResize::getInstance()->toPixelCoords(vec3LU[0], vec3LU[1]);
        CSceneResize::getInstance()->toPixelCoords(vec3RB[0], vec3RB[1]);

        if (vec3RB[0] < vec3LU[0])
            std::swap(vec3RB[0], vec3LU[0]);

        _rcInteractiveBoxCalced.set(vec3LU[0],
                                    vec3LU[1],
                                    vec3RB[0] - vec3LU[0],
                                    vec3RB[1] - vec3LU[1]);
    }

    static_assert(sizeof(_mat) == CMatrixStack::getMatrixSize());
    memcpy(_mat, pMS->top(), pMS->getMatrixSize());
}

bool CSpine::recordAnimation(const char* outFilename,
                             const char* animationName,
                             float fps,
                             bool bSkipAlpha)
{
    if (!_spinePtr)
        return false;

    bool res = _spinePtr->recordAnimation(outFilename,
                                          animationName,
                                          fps,
                                          bSkipAlpha);

    _spinePtr->animationState->setListener(this);

    return res;
}

void CSpine::addGhostAnim()
{
    if (_spinePtr)
        _spinePtr->addGhostAnim(-50);
}

bool CSpine::getTransBounds(Rect* p)
{
    bool bRes = false;

    if (_bNoBounds)
    {
        bRes = false;
    }
    else
    {
        if (_isVecSaved)
        {
            *p = _rcInteractiveBoxCalced;
            bRes = true;
        }
    }

    return bRes;
}

bool CSpine::getNotTransBounds(Rect* p)
{
    if (_spinePtr)
    {
        _spinePtr->getRenderedRect(p);
        return (isfinite(p->x) && isfinite(p->y));
    }
    else
    {
        return false;
    }
}

void CSpine::callback(spine::AnimationState* state,
                      spine::EventType type,
                      spine::TrackEntry* entry,
                      spine::Event* event)
{
    if (type == spine::EventType_Start)
    {
        if (_onStart)
            _onStart(this, entry->getAnimation()->getName().buffer());
    }
    else if (type == spine::EventType_Complete)
    {
        if (_onComplete)
            _onComplete(this, entry->getAnimation()->getName().buffer());
    }
    else if (type == spine::EventType_Event)
    {
        if (_onEvent)
            _onEvent(this, event->getData().getName().buffer());
    }
}

void CSpine::init()
{
    assert(!_isInitDone);

    if (!_isInitDone)
    {
        _skipEachNthUpdate = CGfx::getInstance()->isPotato() ? 2 : 0;
        _isInitDone = true;

        if (_spinePtr)
            _spinePtr->animationState->setListener(this);
    }
}

void CSpine::setToSetupPose()
{
    if (_spinePtr)
        _spinePtr->skeleton->setToSetupPose();
}

void CSpine::clearTracks()
{
    if (_spinePtr)
        _spinePtr->animationState->clearTracks();
}

void CSpine::getRect(Rect* pOut)
{
    assert(pOut);

    if (pOut)
    {
        if (_spinePtr)
        {
            spine::Vector<float> v;

            _spinePtr->skeleton->getBounds(pOut->x,
                                           pOut->y,
                                           pOut->cx,
                                           pOut->cy,
                                           v);
        }
        else
        {
            pOut->set(0, 0, 0, 0);
        }
    }
}

void CSpine::getCachedRect(Rect* pOut)
{
    pOut->copyFrom(&_cachedRc);
}

void CSpine::setTimeScale(float timeScale)
{
    if (_spinePtr && _spinePtr->animationState)
        _spinePtr->animationState->setTimeScale(timeScale);
}

float CSpine::getTimeScale()
{
    float fRes = 1.0f;

    if (_spinePtr && _spinePtr->animationState)
        fRes = _spinePtr->animationState->getTimeScale();

    return fRes;
}

void CSpine::update(float dt)
{
    if (!_isInitDone)
        init();

    if (_skipEachNthUpdate && _currUpdate == _skipEachNthUpdate)
        _currUpdate = 0;
    else if (!_bIsPaused)
    {
        if (_spinePtr)
        {
            if (_skipEachNthUpdate)
                _currUpdate++;

            _spinePtr->update(_skipEachNthUpdate ? dt * _skipEachNthUpdate : dt,
                              spine::Physics_Update);
        }
    }

    CContainer::update(dt);
}

void CSpine::renderSelf(float dt, float* rgba)
{
    static float my_rgba[4];

    my_rgba[0] = rgba[0] * _RGBA[0];
    my_rgba[1] = rgba[1] * _RGBA[1];
    my_rgba[2] = rgba[2] * _RGBA[2];
    my_rgba[3] = rgba[3] * _RGBA[3];

    if (!_isInitDone)
        init();

    if (_spinePtr)
    {
        _spinePtr->setTint(my_rgba);
        _spinePtr->render(dt, getTag(), _bIsGs, _bCalcTransNoRender);
    }

    if (_bTrack)
    {
        CSprite::_vRendered.push_back(getPtr());
    }

    if (_isFirstRender)
    {
        getRect(&_cachedRc);

        if (isgreater(_cachedRc.cx, 0.0f) && isgreater(_cachedRc.cy, 0.0f))
        {
            _isFirstRender = false;
        }
    }
}

spine::TrackEntry* CSpine::setAnimation(size_t trackIndex,
                                        const char* lpccAnimName,
                                        bool loop)
{
    if (_spinePtr)
        return _spinePtr->animationState->setAnimation(trackIndex, lpccAnimName, loop);

    assert(false);
    return nullptr;
}

spine::TrackEntry* CSpine::addAnimation(size_t trackIndex,
                                        const char* lpccAnimName,
                                        bool loop,
                                        float delay)
{
    if (_spinePtr)
        return _spinePtr->animationState->addAnimation(trackIndex, lpccAnimName, loop, delay);

    assert(false);
    return nullptr;
}

spine::TrackEntry* CSpine::setEmptyAnimation(size_t trackIndex, float fTime)
{
    if (_spinePtr)
        return _spinePtr->animationState->setEmptyAnimation(trackIndex, fTime);

    assert(false);
    return nullptr;
}

bool CSpine::getInteractiveBounds(Rect* p)
{
    const Rect* rcInteractive =
        _bCustomInteractiveRc
            ? &_rcInterctiveCustom
            : (_spinePtr && _spinePtr->hasInteractiveBox()
                   ? _spinePtr->getInteractiveBox()
                   : nullptr);

    bool bRes = rcInteractive != nullptr;

    if (bRes)
    {
        *p = _rcInteractiveBoxCalced;
    }

    return bRes;
}

void CSpine::setLightAttachment(const char* lpszAttachmentName, bool bDontRender)
{
    if (_spinePtr)
        _spinePtr->setLightAttachment(lpszAttachmentName, bDontRender);
}

spine::SpineRadialLight& CSpine::addRadialLight(float fRadius,
                                                const char* lpszBoneName,
                                                const char* lpszSlotName,
                                                float fIntensity)
{
    static spine::SpineRadialLight dummy;

    assert(_spinePtr);

    if (!_spinePtr)
        return dummy;

    return _spinePtr->addRadialLight(lpszBoneName,
                                     lpszSlotName,
                                     fRadius,
                                     fIntensity);
}

void CSpine::removeRadialLight(spine::SpineRadialLight& light)
{
    if (_spinePtr)
        _spinePtr->removeRadialLight(light);
}

void CSpine::clearRadialLights()
{
    if (_spinePtr)
        _spinePtr->clearRadialLights();
}

void CSpine::collectLights()
{
    if (_spinePtr && _pvCollectedLight)
    {
        _spinePtr->collectRadialLights(*_pvCollectedLight, _mat);

        auto& vLights = _spinePtr->getPrevFrameLights();

        if (!vLights.empty())
        {
            size_t nToAdd = 0;

            for (const auto& l : vLights)
            {
                if (l.bExplicitQuad)
                    ++nToAdd;
            }

            if (nToAdd)
            {
                _pvCollectedLight->reserve(_pvCollectedLight->size() + nToAdd);

                for (const auto& l : vLights)
                {
                    if (l.bExplicitQuad)
                        _pvCollectedLight->push_back(l);
                }
            }

            vLights.clear();
        }
    }
}

CSpinePtr CSpine::cloneInitial()
{
    return CSpineManager::getInstance()->getNewSpine(_strPath.c_str());
}

CSpineManager* CSpineManager::_instance = NULL;

CSpineManager* CSpineManager::getInstance()
{
    if (!CSpineManager::_instance)
        CSpineManager::_instance = new CSpineManager();

    return CSpineManager::_instance;
}

CSpinePtr CSpineManager::getNewSpine(const char* lpccSpinePath,
                                     bool bAutoSetNormalMap)
{
    CSpinePtr pRes;
    std::string strSpine = lpccSpinePath;

    MapStrSkeletonDataIt it = _map.find(strSpine);

    if (it != _map.end())
    {
        spine::SkeletonDrawablePtr ptrSkDr(
            new spine::SkeletonDrawable(lpccSpinePath, it->second.get()));

        pRes = std::make_shared<CSpine>(ptrSkDr, lpccSpinePath);
        pRes->setToSetupPose();
        pRes->_spinePtr->update(1.0f, spine::Physics_None);
    }
    else
    {
        spine::SkeletonData* pData =
            spine::SkeletonDrawable::createSpineSkeletonData(lpccSpinePath);

        assert(pData);

        if (pData)
        {
            _map[strSpine] = SkeletonDataPtr(pData);
        }

        spine::SkeletonDrawablePtr ptrSkDr(
            new spine::SkeletonDrawable(lpccSpinePath, pData));

        pRes = std::make_shared<CSpine>(ptrSkDr, lpccSpinePath);
        pRes->setToSetupPose();
        pRes->_spinePtr->update(1.0f, spine::Physics_None);
    }

    if (bAutoSetNormalMap)
    {
        auto s = std::format("{}_n.png", lpccSpinePath);
        CGfx::getInstance()->uploadAsset(s.c_str());
        auto tex = CGfx::getInstance()->getTextureById(s.c_str());
        pRes->setNormalMap(tex);
    }

    return pRes;
}

_G2D_NAMESPACE_END_
