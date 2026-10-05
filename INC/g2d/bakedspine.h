#pragma once

#include "g2d.h"
#include "spinepacked.h"
#include "container.h"

_G2D_NAMESPACE_BEGIN_

class CBakedSpine;

typedef std::shared_ptr<CBakedSpine> CBakedSpinePtr;
typedef std::weak_ptr<CBakedSpine>   CBakedSpineWeakPtr;


class SpineVertsDrawable
{
private:
    PackedAnimPtr _anim;

    std::string _name;

    bool _isLooped = false;
    bool _isComplete = false;

    bool _isInterpolated = true;

    bool _ignoreAdditiveBlend = false;
    bool _bSkipLight = false;

    uint32_t _frame = 0;
    uint32_t _nextFrame = 0;

    float _frameTimer = 0.0f;
    float _targetFps = 1.0f / 30.0f;
    float _fTimeScale = 1.0f;
    float _interpolationFactor = 0.0f;

    CTexturePtr _tex;
    CTexturePtr _normalMap;
    SimpleCallback _onComplete;

public:
    bool loadAnimation(const char* filename,
                       const char* animName,
                       bool isLooped,
                       CTexturePtr ptrTex,
                       SimpleCallback cbOnComplete = {},
                       float fpsOverride = 0.0f,
                       float fTimeScale = 1.0f,
                       bool ignoreAdditiveBlend = false);

    bool setAnimation(PackedAnimPtr anim,
                      const char* animName,
                      bool isLooped,
                      CTexturePtr ptrTex,
                      SimpleCallback cbOnComplete = {},
                      float fpsOverride = 0.0f,
                      float fTimeScale = 1.0f,
                      bool ignoreAdditiveBlend = false);

    void renderVerts(float r, float g, float b, float a);
    void update(float dt);

    const std::string& getName() const
    {
        return _name;
    }

    void setTimeScale(float f)
    {
        _fTimeScale = f;
    }

    void setOnComplete(SimpleCallback cbOnComplete)
    {
        _onComplete = cbOnComplete;
    }

    void setNormalMap(CTexturePtr ptrNormalMap)
    {
        _normalMap = ptrNormalMap;
    }

    void setSkipLight(bool bSkipLight)
    {
        _bSkipLight = bSkipLight;
    }

    void setInterpolated(bool bInterpolated)
    {
        _isInterpolated = bInterpolated;
    }

    void setLooped(bool bLooped)
    {
        _isLooped = bLooped;
    }

    void reset()
    {
        _frame = 0;
        _nextFrame = 0;
        _frameTimer = 0.0f;
        _isComplete = false;
        _interpolationFactor = 0.0f;
    }

    PackedAnimPtr getAnim() const
    {
        return _anim;
    }
};

class CBakedSpine : public CContainer
{
public:
    CBakedSpine(std::span<const char*> filenames,
                std::span<const char*> animNames,
                CTexturePtr tex,
                bool isLooped = false);

    virtual ~CBakedSpine() = default;

    inline CBakedSpinePtr getPtr(void)
    {
        return std::dynamic_pointer_cast<CBakedSpine>(shared_from_this());
    }

public:
    virtual void update(float dt) override;
    virtual void renderSelf(float dt, float* rgba) override;

public:
    bool setAnimationByName(const char* lpccAnimName,
                            bool bLoop = false,
                            SimpleCallback cbOnComplete = {});

    void setTimeScale(float timeScale)
    {
        _fTimeScale = timeScale;

        for (auto& d : _vertsDrawer)
            d.setTimeScale(timeScale);
    }

    float getTimeScale()
    {
        return _fTimeScale;
    }

    void setNormalMap(CTexturePtr ptrNormalMap)
    {
        for (auto& d : _vertsDrawer)
            d.setNormalMap(ptrNormalMap);
    }

    void setSkipLight(bool bSkip)
    {
        for (auto& d : _vertsDrawer)
            d.setSkipLight(bSkip);
    }

    SpineVertsDrawable& getDrawer(int idx)
    {
        assert(idx >= 0 && idx < (int)_vertsDrawer.size());
        return _vertsDrawer[idx];
    }

    int getDrawerCount() const
    {
        return (int)_vertsDrawer.size();
    }

private:
    std::vector<SpineVertsDrawable> _vertsDrawer;
    int   _nCurrentAnimIdx = -1;
    float _fTimeScale = 1.0f;
};

_G2D_NAMESPACE_END_
