#pragma once

#include "g2d.h"
#include <spine/spine.h>
#include "gfx.h"
#include "easing.h"
#include "engine.h"

_G2D_NAMESPACE_BEGIN_

typedef std::tuple<std::string, std::string, float> MixEntry;
typedef std::vector<MixEntry> MixEntries;

class SpineMixingOptions
{
public:
    inline static void setMixTime(LPCTSTR lpszSpinePath,
                                  LPCTSTR lpszFromAnim,
                                  LPCTSTR lpszToAnim,
                                  float fMixTime)
    {
        auto s = std::format("{}/{}", Engine::getCfg().RES_DIR, lpszSpinePath);
        auto it = _mapMix.find(s.c_str());

        if (it != _mapMix.end())
        {
            auto key = std::make_tuple(std::string(lpszFromAnim),
                                       std::string(lpszToAnim));

            auto it2 = it->second->find(key);
            if (it2 != it->second->end())
            {
                it2->second = fMixTime;
            }
            else
            {
                it->second->insert(std::make_pair(key, fMixTime));
            }
        }
        else
        {
            MapMixDurationPtr p = std::make_shared<MapMixDuration>();
            p->insert(
                std::make_pair(
                    std::make_tuple(std::string(lpszFromAnim),
                                    std::string(lpszToAnim)),
                    fMixTime));

            _mapMix[lpszSpinePath] = p;
        }
    }

    inline static float getMixTime(LPCTSTR lpszSpinePath,
                                   LPCTSTR lpszFromAnim,
                                   LPCTSTR lpszToAnim)
    {
        float fRes = _defaultMixTime;

        auto it = _mapMix.find(lpszSpinePath);
        if (it != _mapMix.end())
        {
            auto key = std::make_tuple(std::string(lpszFromAnim),
                                       std::string(lpszToAnim));

            auto it2 = it->second->find(key);
            if (it2 != it->second->end())
            {
                fRes = it2->second;
            }
        }

        return fRes;
    }

    inline static void getMixEntries(LPCTSTR lpszSpineName, MixEntries& vOut)
    {
        vOut.clear();

        auto it = _mapMix.find(lpszSpineName);
        if (it != _mapMix.end())
        {
            for (auto it2 = it->second->begin(); it2 != it->second->end(); ++it2)
            {
                vOut.emplace_back(
                    std::make_tuple(std::get<0>(it2->first),
                                    std::get<1>(it2->first),
                                    it2->second));
            }
        }
    }

    inline static void setDefaultMixTime(float fDef)
    {
        _defaultMixTime = fDef;
    }

    inline static float getDefaultMixTime()
    {
        return _defaultMixTime;
    }

private:
    typedef std::map<std::tuple<std::string, std::string>, float> MapMixDuration;
    typedef std::shared_ptr<MapMixDuration> MapMixDurationPtr;

    inline static std::map<std::string, MapMixDurationPtr> _mapMix;
    inline static float _defaultMixTime = 1.0f;
};

_G2D_NAMESPACE_END_

namespace spine
{
using namespace _G2D_NS_;

enum class eSpineRadialLockAxis
{
    NONE,
    X,
    Y
};

struct SpineRadialLight
{
    SpineRadialLight()
    {
        color.set(1.0f, 1.0f, 1.0f);
        ptBoneLocalOffset.set(0.0f, 0.0f);
        ptWorldOffset.set(0.0f, 0.0f);
    }

    void invalidateSetupCache()
    {
        bSetupCached = false;
        pSetupCacheSkeleton = nullptr;
        pSetupCacheBone = nullptr;
    }

    void invalidateCache()
    {
        pBone = nullptr;
        pSlot = nullptr;
        invalidateSetupCache();
    }

    bool          bEnabled = true;
    std::string   sBoneName;
    std::string   sSlotName;
    Bone*         pBone = nullptr;
    Slot*         pSlot = nullptr;

    Point         ptBoneLocalOffset;
    Point         ptWorldOffset;

    float         fRadius = 100.0f;
    float         fIntensity = 1.0f;
    Point3        color;
    eLightLayer   eLayer = eLightLayer::GAME;
    bool          bCanAffectGL = false;
    int           zMin = LightMinLayer;
    int           zMax = LightMaxLayer;
    uint16_t      itemCullingMask = 0xFFFF;

    bool          bIgnoreAlpha = false;
    float         fAlphaThreshold = 0.001f;

    bool          bUseAttachmentOffset = true;
    bool          bScaleRadiusByAlpha = false;

    float         fSwayX = 1.0f;

    eSpineRadialLockAxis lockAxis = eSpineRadialLockAxis::NONE;

    Skeleton*     pSetupCacheSkeleton = nullptr;
    Bone*         pSetupCacheBone = nullptr;
    bool          bSetupCached = false;

    float         setupA = 1.0f;
    float         setupB = 0.0f;
    float         setupC = 0.0f;
    float         setupD = 1.0f;
    float         setupWorldX = 0.0f;
    float         setupWorldY = 0.0f;
    float         setupParentWorldX = 0.0f;
};

struct TweenAnim
{
    easingFunction  func = Easing::linear;
    SimpleCallback  cbOnComplete;
    float           fDuration = 0.0f;
    float           fFromVal = 0.0f;
    float           fToVal = 0.0f;
    float           fTime = 0.0f;
    float           fWaitStart = 0.0f;
    bool            bActive = false;

    void start(float fFrom, float fTo, float fDur, float fWait,
               easingFunction easingF, SimpleCallback cb)
    {
        assert(!bActive);

        fFromVal     = fFrom;
        fToVal       = fTo;
        fDuration    = fDur;
        func         = easingF;
        cbOnComplete = cb;
        bActive      = true;
        fTime        = 0.0f;
        fWaitStart   = fWait;
    }

    bool isActive()
    {
        return bActive;
    }

    bool update(float dt)
    {
        assert(isActive());

        if (fWaitStart > 0.0f)
        {
            fWaitStart -= dt;
            return bActive;
        }

        fTime += dt;

        if (fTime >= fDuration)
        {
            fTime = fDuration;
            bActive = false;
        }

        return bActive;
    }

    float getVal()
    {
        return fFromVal + ((fToVal - fFromVal) * func(fTime / fDuration));
    }
};

enum class eSpineGhostAnimState
{
    INACTIVE,
    MOVING_X,
    MOVING_Y,
    FADING,
};

class SkeletonDrawable : public spine::AnimationStateListenerObject
{
private:
    void callback(spine::AnimationState* state,
                  spine::EventType type,
                  spine::TrackEntry* entry,
                  spine::Event* event) override
    {
        (void)state;
        (void)type;
        (void)entry;
        (void)event;
    }

public:
    static SkeletonData* createSpineSkeletonData(const char* lpccSpineName);

public:
    SkeletonDrawable(LPCTSTR lpszSpinePath,
                     SkeletonData* skeletonData,
                     AnimationStateData* animationStateData = nullptr);

    ~SkeletonDrawable(void);

    void updateTweenAnims(float dt);
    void update(float delta, Physics physics);

    void render(float dt,
                int64_t tag,
                bool bGs,
                bool skipRender);

    bool getRenderedRect(Rect* pOut);
    bool hasAnimation(LPCTSTR lpszAnimName);

    inline void setAlpha(float alpha)
    {
        RGBA[3] = alpha;
    }

    inline float getAlpha(void)
    {
        return RGBA[3];
    }

    inline void setTint(float* tint)
    {
        memcpy(RGBA, tint, sizeof(RGBA));
    }

    inline bool isInitialized(void)
    {
        return _bInitialized;
    }

    LPCTSTR getBoundingBoxAtPoint(Point ptCenter, Point pt);
    bool getBoundingBoxRect(LPCTSTR lpszBBname, Point ptCenter, Rect& rcOut);

    inline const Rect* getInteractiveBox(void)
    {
        updateBoundingBoxes();
        return &_rcInteractiveBox;
    }

    inline bool hasInteractiveBox(void)
    {
        return _bHasInteractiveBox;
    }

    inline void setOnInitDone(SimpleCallback pInitDone)
    {
        _onInitDone = pInitDone;
    }

    void setEffect(float fEffect = 0.0f,
                   float fLifeTime = 0.0f,
                   SimpleCallback cb = {})
    {
        _fEffectType = fEffect;
        _fEffectLifeTime = fLifeTime;
        _fEffectInitialTime = fLifeTime;
        _cbEffectComplete = cb;
        _effectCbCalled = false;
    }

    void addGhostAnim(float nGhostX);
    TweenAnim* addGhostAnim(void);

    bool recordAnimation(const char* outFilename,
                         const char* animationName,
                         float fps = 30.0f,
                         bool bSkipAlpha = false);

    void setAttachment(LPCTSTR lpszBone, LPCTSTR lpszAttachment)
    {
        hideHeadPermanently(skeleton, lpszBone);
    }

    auto* getSkeleton(void)
    {
        return skeleton;
    }

    void setSkin(LPCTSTR lpszSkin)
    {
        skeleton->setSkin(lpszSkin);
    }

    void setNormalMap(CTexturePtr ptrNormalMap)
    {
        _ptrNormalMap = ptrNormalMap;
    }

    void setSkipLight(bool bSkip)
    {
        _bSkipLight = bSkip;
    }

    float getAnimationProgress(void)
    {
        if (animationState->getCurrent(0))
        {
            if (animationState->getCurrent(0)->getAnimation())
            {
                return animationState->getCurrent(0)->getAnimationTime() /
                       animationState->getCurrent(0)->getAnimation()->getDuration();
            }
        }

        return 0.0f;
    }

    bool isBoneChildOf(spine::BoneData* boneData, spine::BoneData* parentTargetData)
    {
        spine::BoneData* current = boneData;

        while (current != nullptr)
        {
            if (current == parentTargetData)
                return true;

            current = current->getParent();
        }

        return false;
    }

    void hideHeadPermanently(spine::Skeleton* skeleton, LPCTSTR rootBoneName)
    {
        if (!skeleton)
            return;

        spine::SkeletonData* skeletonData = skeleton->getData();
        spine::BoneData* rootBoneData = skeletonData->findBone(rootBoneName);

        if (!rootBoneData)
            return;

        spine::Skin* customSkin = new spine::Skin("final_filtered_skin");

        spine::Skin* skinsToMerge[] =
        {
            skeleton->getSkin(),
            skeletonData->getDefaultSkin()
        };

        for (spine::Skin* currentSkin : skinsToMerge)
        {
            if (!currentSkin)
                continue;

            spine::Skin::AttachmentMap::Entries entries = currentSkin->getAttachments();

            while (entries.hasNext())
            {
                spine::Skin::AttachmentMap::Entry& entry = entries.next();

                int slotIndex = entry._slotIndex;
                const spine::String& name = entry._name;
                spine::Attachment* attachment = entry._attachment;

                spine::SlotData* slotData = skeletonData->getSlots()[slotIndex];
                spine::BoneData* boneData = &slotData->getBoneData();

                if (!isBoneChildOf(boneData, rootBoneData))
                {
                    customSkin->setAttachment(slotIndex, name, attachment);
                }
            }
        }

        spine::Skin* emptyDefaultSkin = new spine::Skin("empty_default");

        skeletonData->setDefaultSkin(emptyDefaultSkin);
        skeleton->setSkin(customSkin);
        skeleton->setSlotsToSetupPose();
    }

    struct SpineLightAttachmentInfo
    {
        bool     bDontRender = true;
        GPULight lightTemplate;
    };

    void setLightAttachment(const char* lpszAttachmentName, bool bDontRender);

    SpineRadialLight& addRadialLight(const char* lpszBoneName,
                                     const char* lpszSlotName = nullptr,
                                     float fRadius = 100.0f,
                                     float fIntensity = 1.0f);

    void removeRadialLight(SpineRadialLight& light);
    void clearRadialLights();
    void collectRadialLights(GPULights& out, const float* spineMat);

private:
    void polygonToRect(Polygon* pPoly, Rect* pRcOut);
    void getBBIndexes(SkeletonBounds& sb);
    void updateBoundingBoxes(void);

    bool collectLightAttachment(Vector<float>* worldVertices,
                                Vector<float>* uvs,
                                CTexture* texture,
                                float alpha,
                                float red,
                                float green,
                                float blue,
                                SpineLightAttachmentInfo& info);

public:
    Skeleton*           skeleton;
    AnimationState*     animationState;
    bool                usePremultipliedAlpha;

    bool                _bIndexesGot = false;
    int                 _nBB_BodyIdx = -1;
    bool                _bSkipLight = false;

    struct
    {
        eSpineGhostAnimState eState = eSpineGhostAnimState::INACTIVE;
        float                x = 0.0f;
        float                y = 0.0f;
        float                fAlpha = 1.0f;
        TweenAnim            tw;

        void init(eSpineGhostAnimState e = eSpineGhostAnimState::MOVING_X)
        {
            eState = e;
            x = 0.0f;
            y = 0.0f;
            fAlpha = 1.0f;
        }

        void switchToNextState()
        {
            switch (eState)
            {
                case eSpineGhostAnimState::MOVING_X:
                    eState = eSpineGhostAnimState::FADING;
                    break;

                case eSpineGhostAnimState::MOVING_Y:
                    eState = eSpineGhostAnimState::INACTIVE;
                    break;

                case eSpineGhostAnimState::FADING:
                    eState = eSpineGhostAnimState::INACTIVE;
                    break;

                default:
                    assert(false);
                    break;
            }
        }

        void update(float dt)
        {
            if (tw.isActive())
            {
                bool bActive = tw.update(dt);

                switch (eState)
                {
                    case eSpineGhostAnimState::MOVING_X:
                        x = tw.getVal();
                        break;

                    case eSpineGhostAnimState::MOVING_Y:
                        y = tw.getVal();
                        break;

                    case eSpineGhostAnimState::FADING:
                        fAlpha = tw.getVal();
                        break;

                    default:
                        break;
                }

                if (!bActive)
                {
                    switchToNextState();

                    if (tw.cbOnComplete)
                        tw.cbOnComplete();
                }
            }
        }

        bool isActive()
        {
            return eState != eSpineGhostAnimState::INACTIVE;
        }

    } _ghosts[5];

    auto& getPrevFrameLights()
    {
        return _vPrevFrameLights;
    }

private:
    GPULights  _vPrevFrameLights;
    bool ownsAnimationStateData;

    Vector<float> _worldVertices;
    Vector<unsigned short> _quadIndices;
    SkeletonClipping _clipping;
    Ints sdlIndices;

    float RGBA[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    float _left = INFINITY;
    float _top = INFINITY;
    float _right = -INFINITY;
    float _bottom = -INFINITY;

    bool _bInitialized = false;
    int _timesDrawn = 0;

    Rect _rcInteractiveBox;

    SimpleCallback _onInitDone;
    SimpleCallback _cbEffectComplete;
    bool _effectCbCalled = false;
    bool _bHasInteractiveBox = false;

    float _fEffectType = 0.0f;
    float _fEffectLifeTime = 0.0f;
    float _fEffectInitialTime = 0.0f;

    CTexturePtr _ptrNormalMap;

    std::unordered_map<int, SpineLightAttachmentInfo> _lightAttachments;
    std::deque<SpineRadialLight> _radialLights;
};

typedef std::shared_ptr<SkeletonDrawable> SkeletonDrawablePtr;
typedef std::vector<SkeletonDrawablePtr> SkeletonDrawables;

class GfxTextureLoader : public spine::TextureLoader
{
private:
    static GfxTextureLoader* _instance;

public:
    static GfxTextureLoader* getInstance(void);

    void load(AtlasPage& page, const String& path);
    void unload(void* texture);
};

}
