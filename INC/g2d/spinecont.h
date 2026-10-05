#pragma once

#include "g2d.h"
#include "m3.h"
#include "spinerender.h"
#include "container.h"

#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <string>

_G2D_NAMESPACE_BEGIN_

class CSpine;

typedef std::shared_ptr<CSpine>                CSpinePtr;
typedef std::weak_ptr<CSpine>                  CSpineWeakPtr;
typedef std::shared_ptr<spine::SkeletonData>   SkeletonDataPtr;
typedef std::map<std::string, SkeletonDataPtr> MapStrSkeletonData;
typedef MapStrSkeletonData::iterator           MapStrSkeletonDataIt;

typedef std::function<void(CSpine* pOwner, const char* lpccAnimName)>  onAnimStart_f;
typedef std::function<void(CSpine* pOwner, const char* lpccAnimName)>  onAnimComplete_f;
typedef std::function<void(CSpine* pOwner, const char* lpccEventName)> onAimEvent_f;

class CSpine : public CContainer,
               public spine::AnimationStateListenerObject
{
    friend class CSpineManager;

private:
    int _skipEachNthUpdate = 0;
    int _currUpdate = 0;

public:
    CSpine(spine::SkeletonDrawablePtr ptr, LPCTSTR lpccSpinePath);
    virtual ~CSpine(void);

    inline CSpinePtr getPtr(void)
    {
        return std::dynamic_pointer_cast<CSpine>(shared_from_this());
    }

public:

    virtual void callback(spine::AnimationState* state,
                          spine::EventType type,
                          spine::TrackEntry* entry,
                          spine::Event* event) override;

public:
    virtual bool getNotTransBounds(Rect* p) override;
    virtual bool getTransBounds(Rect* p) override;
    virtual void update(float dt) override;
    virtual void renderSelf(float dt, float* rgba) override;

protected:
    virtual void applyTransform(CMatrixStack* pMS, bool bForce = false);
    virtual void collectLights() override;

public:
    void setSkin(LPCTSTR lpszSkin)
    {
        if (_spinePtr)
            _spinePtr->setSkin(lpszSkin);
    }

    bool recordAnimation(const char* outFilename,
                         const char* animationName,
                         float fps = 30.0f,
                         bool bSkipAlpha = false);

    void addGhostAnim(void);

    auto addGhostAnimCustom(void)
    {
        return _spinePtr->addGhostAnim();
    }

    void setEffect(float fEffect = 0.0f,
                   float fLifeTime = 0.0f,
                   SimpleCallback cb = {})
    {
        if (_spinePtr)
            _spinePtr->setEffect(fEffect, fLifeTime, cb);
    }

    inline bool isInitialized(void)
    {
        return _spinePtr ? _spinePtr->isInitialized() : false;
    }

    void setToSetupPose(void);
    void clearTracks(void);
    void getRect(Rect* pOut);
    void setTimeScale(float timeScale);
    float getTimeScale(void);

    void setAttachment(LPCTSTR lpszBone, LPCTSTR lpszAttachment)
    {
        if (_spinePtr)
            _spinePtr->setAttachment(lpszBone, lpszAttachment);
    }

    spine::TrackEntry* setAnimation(size_t trackIndex,
                                    const char* lpccAnimName,
                                    bool loop);

    spine::TrackEntry* addAnimation(size_t trackIndex,
                                    const char* lpccAnimName,
                                    bool loop,
                                    float delay = 0.0f);

    spine::TrackEntry* setEmptyAnimation(size_t trackIndex, float fTime);

    void getCachedRect(Rect* pOut);

    virtual bool getInteractiveBounds(Rect* p);

    inline void onStart(onAnimStart_f onStart)
    {
        _onStart = onStart;
    }

    inline void onComplete(onAnimComplete_f onComplete)
    {
        _onComplete = onComplete;
    }

    inline void onEvent(onAimEvent_f onEvent)
    {
        _onEvent = onEvent;
    }

    inline bool hasAnimation(LPCTSTR lpszAnim)
    {
        return _spinePtr ? _spinePtr->hasAnimation(lpszAnim) : false;
    }

    inline const Rect* getInteractiveBoxCalced(void)
    {
        return &_rcInteractiveBoxCalced;
    }

    auto getInteractiveBox(void)
    {
        return _spinePtr ? _spinePtr->getInteractiveBox() : nullptr;
    }

    auto getBoundingBoxAtPoint(Point ptCenter, Point pt)
    {
        return _spinePtr ? _spinePtr->getBoundingBoxAtPoint(ptCenter, pt) : nullptr;
    }

    bool getBoundingBoxRect(LPCTSTR lpszBBname, Point ptCenter, Rect& rcOut)
    {
        return _spinePtr ? _spinePtr->getBoundingBoxRect(lpszBBname, ptCenter, rcOut) : false;
    }

    void setCustomInteractiveRc(Rect* rc)
    {
        _rcInterctiveCustom = *rc;
        _bCustomInteractiveRc = true;
    }

    inline void setOnInitDone(SimpleCallback pInitDone)
    {
        if (_spinePtr)
            _spinePtr->setOnInitDone(pInitDone);
    }

    inline void setSkipNthUpdate(int n)
    {
        _skipEachNthUpdate = n;
    }

    inline void setPaused(bool bIsPaused)
    {
        _bIsPaused = bIsPaused;
    }

    void setNoBounds(bool bNoBounds = true)
    {
        _bNoBounds = bNoBounds;
    }

    CSpinePtr cloneInitial(void);

    auto* getSkeleton(void)
    {
        return _spinePtr ? _spinePtr->getSkeleton() : nullptr;
    }

    float getAnimationProgress(void)
    {
        return _spinePtr ? _spinePtr->getAnimationProgress() : 0.0f;
    }

    CTexturePtr getNormalMap()
    {
        return _normalMap;
    }

    void setNormalMap(CTexturePtr ptrNormalMap)
    {
        _normalMap = ptrNormalMap;

        if (_spinePtr)
            _spinePtr->setNormalMap(ptrNormalMap);
    }

    void setSkipLight(bool bSkip)
    {
        if (_spinePtr)
            _spinePtr->setSkipLight(bSkip);
    }

    auto getMat()
    {
        return _mat;
    }

    void setLightAttachment(const char* lpszAttachmentName, bool bDontRender = true);

    spine::SpineRadialLight& addRadialLight(float fRadius,
                                            const char* lpszBoneName,
                                            const char* lpszSlotName = nullptr,
                                            float fIntensity = 1.0f);

    void removeRadialLight(spine::SpineRadialLight& light);
    void clearRadialLights();

private:
    void init(void);

private:
    bool                        _bNoBounds = false;
    bool                        _isInitDone = false;
    bool                        _isFirstRender = true;
    bool                        _bTrack = false;
    bool                        _bIsPaused = false;

    spine::SkeletonDrawablePtr  _spinePtr;

    onAnimStart_f               _onStart;
    onAnimComplete_f            _onComplete;
    onAimEvent_f                _onEvent;

    Rect                        _cachedRc;
    Rect                        _rcInteractiveBoxCalced;
    std::string                 _strPath;

    bool                        _bCustomInteractiveRc = false;
    Rect                        _rcInterctiveCustom;

    float                       _mat[9];

    CTexturePtr                 _normalMap;
};

class CSpineManager
{
private:
    static CSpineManager*       _instance;
    MapStrSkeletonData          _map;

public:
    static CSpineManager* getInstance(void);

    CSpinePtr getNewSpine(const char* lpccSpinePath,
                          bool bAutoSetNormalMap = false);
};

_G2D_NAMESPACE_END_
