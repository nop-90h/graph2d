#pragma once

#include "engine.h"

// ------------------------------------------------------------------
// Длинная капля, "вытекающая" сверху экрана
// ------------------------------------------------------------------
struct LongDrop {
    float x = 0.f; float y = 0.f;
    float scaleY = 0.f; float targetScaleY = 1.f; float scaleX = 1.f;
    float alpha = 0.f; float targetAlpha = 0.7f;
    float growDuration = 2.f; float fadeDelay = 0.5f; float fadeDuration = 1.5f;
    float life = 0.f; float startDelay = 0.f;
    float wobblePhase = 0.f; float wobbleFreq = 0.f; float wobbleAmp = 0.f;
    float colorPhase = 0.f; float colorFreq = 0.f;
    CSpritePtr ptrSprite = nullptr;
    enum Phase { DELAY, GROW, HOLD, FADE, DONE };
    Phase phase = DELAY;
};

// ------------------------------------------------------------------
// Капля фонтана с баллистической физикой и trail-шлейфом
// ------------------------------------------------------------------
struct FountainDrop {
    float x = 0.f, y = 0.f;
    float prevX = 0.f, prevY = 0.f;
    float vx = 0.f, vy = 0.f;
    float life = 0.f, maxLife = 1.f;
    float scale = 1.f; float alpha = 1.f;
    float angle = 0.f;
    static constexpr int TRAIL_COUNT = 14;
    float trailX[TRAIL_COUNT] = {0};
    float trailY[TRAIL_COUNT] = {0};
    float trailAngle[TRAIL_COUNT] = {0};
    float trailAlpha[TRAIL_COUNT] = {0};
    float trailDistAccum = 0.f;
    float trailTimer = 0.f;
    float rotationSpeed = 0.f;
    float stretch = 1.f;
    float bounceCount = 0.f;
    float groundStickTimer = 0.f;
    bool bOnGround = false;
    float wobblePhase = 0.f; float wobbleFreq = 0.f; float wobbleAmp = 0.f;
    float squashX = 1.f; float squashY = 1.f;
};

// ------------------------------------------------------------------
// Частица артериального фонтана (пульсирующая)
// ------------------------------------------------------------------
struct ArteryDrop {
    float x = 0.f, y = 0.f; float vx = 0.f, vy = 0.f;
    float life = 0.f; float maxLife = 0.3f;
    float scale = 1.f; float alpha = 1.f;
    float pulsePhase = 0.f; float pulseFreq = 30.f;
    float angle = 0.f;
};

// ------------------------------------------------------------------
// Капля-мега-брызга (крупные капли при ударе)
// ------------------------------------------------------------------
struct MegaDrop {
    float x = 0.f, y = 0.f; float vx = 0.f, vy = 0.f;
    float life = 0.f; float maxLife = 1.5f;
    float scale = 1.f; float alpha = 1.f;
    float rotation = 0.f; float rotSpeed = 0.f;
    int bounceCount = 0; float groundStickTimer = 0.f; bool bOnGround = false;
    static constexpr int TRAIL_COUNT = 10;
    float trailX[TRAIL_COUNT] = {0}; float trailY[TRAIL_COUNT] = {0};
    float trailAlpha[TRAIL_COUNT] = {0}; float trailTimer = 0.f;
    float moveAngle = 0.f; float squashX = 1.f; float squashY = 1.f;
};

// ------------------------------------------------------------------
// Микро-брызги при ударе о землю
// ------------------------------------------------------------------
struct SplashDrop {
    float x = 0.f, y = 0.f; float vx = 0.f, vy = 0.f;
    float life = 0.f, maxLife = 0.25f;
    float scale = 1.f; float alpha = 1.f; float angle = 0.f;
};

class BloodFountain : public CContainer
{
private:
    std::vector<FountainDrop> _drops;
    std::vector<LongDrop> _longDrops;
    std::vector<ArteryDrop> _arteryDrops;
    std::vector<MegaDrop> _megaDrops;
    std::vector<SplashDrop> _splashDrops;

    // [POOL] Лужи крови — бесконечное растекание, без затухания
    struct PoolBlob
    {
        float x = 0.f, y = 0.f;
        float scaleX = 0.f, scaleY = 0.f;
        float maxScale = 1.f;
        float currentScale = 0.f;
        float growSpeed = 0.5f;
        float alpha = 0.6f;
        float age = 0.f;
        float rotation = 0.f;
        float wobblePhase = 0.f;
        float wobbleFreq = 1.f;
        float wobbleAmp = 0.02f;
        float spreadRatio = 0.35f;
    };
    std::vector<PoolBlob> _pools;

    CSpritePtr _ptrBloodSprite;
    CSpritePtr _ptrPoolSprite;        // [NEW] отдельный спрайт для луж
    CSpritePtr _longDropSprites[4];
    CSpinePtr _ptrTargetSpine;
    spine::Bone* _pNeckBone = nullptr;
    spine::Slot* _pHeadSlot = nullptr;
    bool _bBoneCached = false;
    float _fNeckTrackX = 0.f;
    float _fNeckTrackY = 0.f;
    float _fNeckRotation = 0.f;
    bool _bTrackNeck = false;
    float _fResidualTimer = 0.f;
    static constexpr float RESIDUAL_INTERVAL = 0.04f;
    float _fX = 0.f, _fY = 0.f;
    float _fAccum = 0.f;
    float _fPulseTime = 0.f;
    float _fLife = 0.f;
    float _fLongDropAccum = 0.f;
    float _fArteryTimer = 0.f;
    float _fMegaTimer = 0.f;
    int _nPulseCount = 0;
    bool _bStopSpawning = false;
    static constexpr int MAX_PULSES = 15;
    static constexpr float PULSE_PERIOD = 0.22f;
    static constexpr float GRAVITY = 3200.f;
    static constexpr float GROUND_Y_OFFSET = 220.f;
    static constexpr float BOUNCE_DAMPING = 0.4f;
    static constexpr float GROUND_FRICTION = 0.85f;
    static constexpr float MAX_STRETCH = 1.6f;
    static constexpr float STRETCH_FACTOR = 0.0009f;
    static constexpr float AIR_RESISTANCE = 0.93f;
    static constexpr float WOBBLE_FREQ_BASE = 9.f;
    static constexpr float WOBBLE_AMP_BASE = 0.07f;

    static constexpr int MAX_POOLS = 100;

public:

    bool isEffectFinished() const
    {
        return _drops.empty() && _longDrops.empty() && _arteryDrops.empty()
            && _megaDrops.empty() && _splashDrops.empty();
    }

    void init(float x, float y, CSpinePtr ptrTargetSpine = nullptr)
    {
        _fX = x;
        _fY = y;
        _ptrTargetSpine = ptrTargetSpine;

        // Капля для частиц
        static auto ptrBloodSprite = SpriteLoader::getInstance()->getSprite("UI/bloodDrop");
        static auto ptrPoolSprite  = SpriteLoader::getInstance()->getSprite("UI/bloodSplatter");
        static auto longDropSprites0 = SpriteLoader::getInstance()->getSprite("UI/longDrop");
        static auto longDropSprites1 = SpriteLoader::getInstance()->getSprite("UI/longDrop1");
        static auto longDropSprites2 = SpriteLoader::getInstance()->getSprite("UI/longDrop2");
        static auto longDropSprites3 = SpriteLoader::getInstance()->getSprite("UI/longDrop3");

        _ptrBloodSprite = ptrBloodSprite->cloneInitial();
        //_ptrBloodSprite->setReflectionType(eReflectionType::CAN_BE_REFLECTED);
        if (_ptrBloodSprite) _ptrBloodSprite->setPivotCentered();

        // [NEW] Спрайт для луж — плоский, размазанный
        _ptrPoolSprite = ptrPoolSprite->cloneInitial();
        //_ptrPoolSprite->setReflectionType(eReflectionType::MIRROR);
        if (_ptrPoolSprite) _ptrPoolSprite->setPivotCentered();

        _longDropSprites[0] = longDropSprites0->cloneInitial();
        _longDropSprites[1] = longDropSprites1->cloneInitial();
        _longDropSprites[2] = longDropSprites2->cloneInitial();
        _longDropSprites[3] = longDropSprites3->cloneInitial();
        //for (int i = 0; i < SIZE_OF(_longDropSprites); i++)
        //    _longDropSprites[i]->setReflectionType(eReflectionType::CAN_BE_REFLECTED);
        //setReflectionType(eReflectionType::CAN_BE_REFLECTED);
    }

    void startNeckTracking(const std::string& boneName = "neck", const std::string& slotName = "head")
    {
        if (!_ptrTargetSpine) return;
        auto skeleton = _ptrTargetSpine->getSkeleton();
        if (!skeleton) return;

        if (!_bBoneCached)
        {
            for (int i = 0; i < skeleton->getBones().size(); ++i)
            {
                auto bone = skeleton->getBones()[i];
                if (bone && !bone->getData().getName().isEmpty())
                {
                    if (strcmp(bone->getData().getName().buffer(), boneName.c_str()) == 0)
                    {
                        _pNeckBone = bone;
                        _bBoneCached = true;
                        break;
                    }
                }
            }
            if (!_pNeckBone)
            {
                for (int i = 0; i < skeleton->getSlots().size(); ++i)
                {
                    auto slot = skeleton->getSlots()[i];
                    if (slot && !slot->getData().getName().isEmpty())
                    {
                        if (strcmp(slot->getData().getName().buffer(), slotName.c_str()) == 0)
                        {
                            _pHeadSlot = slot;
                            _bBoneCached = true;
                            break;
                        }
                    }
                }
            }
        }
        _bTrackNeck = true;
        _fResidualTimer = 0.f;
    }

    void stopNeckTracking() { _bTrackNeck = false; }
    void stopSpawning() { _bStopSpawning = true; }

    void fadeOutAllLongDrops(float duration)
    {
        _bStopSpawning = true;
        for (auto& ld : _longDrops)
        {
            if (ld.phase == LongDrop::DONE) continue;
            ld.targetAlpha = ld.alpha;
            ld.fadeDuration = duration;
            ld.life = 0.f;
            ld.phase = LongDrop::FADE;
        }
    }

    void fadeOutAllDrops(float duration)
    {
        for (auto& d : _drops) d.maxLife = d.life + duration;
        for (auto& d : _arteryDrops) d.maxLife = d.life + duration;
        for (auto& d : _megaDrops) d.maxLife = d.life + duration;
    }

    bool getNeckTransform(float& outX, float& outY, float& outRotation)
    {
        if (!_bBoneCached) return false;
        if (_pNeckBone)
        {
            outX = _pNeckBone->getWorldX();
            outY = _pNeckBone->getWorldY();
            outRotation = _pNeckBone->getRotation() + _pNeckBone->getAppliedRotation();
            return true;
        }
        else if (_pHeadSlot)
        {
            outX = _pHeadSlot->getBone().getWorldX();
            outY = _pHeadSlot->getBone().getWorldY();
            outRotation = _pHeadSlot->getBone().getRotation() + _pHeadSlot->getBone().getAppliedRotation();
            return true;
        }
        return false;
    }

    // [POOL] Спавн лужи — без rotation, почти круг, мягкая альфа
    void spawnPool(float x, float y, float intensity)
    {
        if (_pools.size() >= MAX_POOLS) return;

        PoolBlob p;
        p.x = x + ((rand() % 12) - 6);
        p.y = y + 2.f;
    
        // [FIX] Уменьшен intensity-множитель и жёсткий потолок
        p.maxScale = 0.05f + intensity * 0.115f;   // было: 2.0 + intensity*0.028

        if (p.maxScale > 0.8f) p.maxScale = .8f; // было: 14.0, стало: 6.0
    
        p.currentScale = 0.06f;
        p.growSpeed = 0.12f + (rand() % 30) * 0.005f; // чуть медленнее рост
        p.alpha = 0.25f + (rand() % 10) * 0.01f;      // мягче альфа (было 0.30)
        p.rotation = 0.f;
        p.wobblePhase = (rand() % 100) * 0.0628f;
        p.wobbleFreq = 0.5f + (rand() % 30) * 0.03f;  // медленнее дыхание
        p.wobbleAmp = 0.008f + (rand() % 8) * 0.001f; // едва заметное
        p.spreadRatio = 0.75f + (rand() % 10) * 0.01f; // чуть круглее
        _pools.push_back(p);
    }
    void spawnLongDrop()
    {
        int idx = rand() % 4;
        CSpritePtr ptrSprite = _longDropSprites[idx];
        if (!ptrSprite) return;

        LongDrop ld;
        ld.ptrSprite = ptrSprite;
        auto& cfg = Engine::getCfg();
        ld.x = (float)(rand() % (int)cfg.INIT_SCR_CX);
        ld.y = -100.f;

        Rect rc;
        ptrSprite->getNotTransBounds(&rc);
        float origW = std::max(1.f, rc.cx);
        float origH = std::max(1.f, rc.cy);
        float desiredWidth = 6.f + (rand() % 10);
        float desiredLength = 80.f + (rand() % 180);

        ld.scaleX = desiredWidth / origW;
        ld.targetScaleY = desiredLength / origH;
        ld.targetAlpha = 0.2f + (rand() % 15) * 0.01f;
        ld.growDuration = 3.0f + (rand() % 300) * 0.01f;
        ld.fadeDelay = 0.3f + (rand() % 40) * 0.01f;
        ld.fadeDuration = 1.2f + (rand() % 80) * 0.01f;
        ld.startDelay = (rand() % 60) * 0.01f;
        ld.wobblePhase = (rand() % 100) * 0.0628f;
        ld.wobbleFreq = 1.0f + (rand() % 50) * 0.06f;
        ld.wobbleAmp = 0.3f + (rand() % 15) * 0.05f;
        ld.colorPhase = (rand() % 100) * 0.0628f;
        ld.colorFreq = 1.5f + (rand() % 40) * 0.05f;
        ld.scaleY = 0.f;
        ld.alpha = 0.f;
        ld.life = 0.f;
        ld.phase = LongDrop::DELAY;
        _longDrops.push_back(ld);
    }

    void spawnArteryPulse(float x, float y, float neckRotation)
    {
        int count = 8 + (rand() % 12);
        float rad = neckRotation * 0.0174533f;
        for (int i = 0; i < count; ++i)
        {
            ArteryDrop d;
            float spread = ((rand() % 120) - 60) * 0.0174533f;
            float finalAngle = rad + spread;
            float speed = 400.f + (rand() % 600);
            float speedVar = 1.0f + (rand() % 50) * 0.02f;

            d.x = x + cosf(finalAngle + 1.5708f) * ((rand() % 20) - 10);
            d.y = y + sinf(finalAngle + 1.5708f) * ((rand() % 20) - 10);
            d.vx = cosf(finalAngle) * speed * speedVar;
            d.vy = sinf(finalAngle) * speed * speedVar;
            d.angle = 1.57079633f - atan2f(d.vy, d.vx);
            d.life = 0.f;
            d.maxLife = 0.15f + (rand() % 20) * 0.01f;
            d.scale = 0.3f + (rand() % 25) * 0.02f;
            d.alpha = 0.9f + (rand() % 10) * 0.01f;
            d.pulsePhase = (rand() % 100) * 0.0628f;
            d.pulseFreq = 20.f + (rand() % 40);
            _arteryDrops.push_back(d);
        }
    }

    void spawnMegaDrop(float x, float y, float impactSpeed)
    {
        int count = 3 + (rand() % 5);
        for (int i = 0; i < count; ++i)
        {
            MegaDrop d;
            d.x = x + ((rand() % 30) - 15);
            d.y = y;
            float angle = ((rand() % 160) - 140) * 0.0174533f;
            float speed = impactSpeed * 0.3f + (rand() % 200);
            d.vx = cosf(angle) * speed;
            d.vy = sinf(angle) * speed * 0.8f;
            d.moveAngle = 1.57079633f - atan2f(d.vy, d.vx);
            d.life = 0.f;
            d.maxLife = 0.8f + (rand() % 30) * 0.02f;
            d.scale = 0.25f + (rand() % 20) * 0.015f;
            d.alpha = 0.95f;
            d.rotation = (rand() % 360) * 0.0174533f;
            d.rotSpeed = ((rand() % 400) - 200) * 0.5f;
            d.bounceCount = 0;
            d.bOnGround = false;
            d.groundStickTimer = 0.f;
            d.squashX = 1.f;
            d.squashY = 1.f;
            for (int k = 0; k < MegaDrop::TRAIL_COUNT; ++k)
            {
                d.trailX[k] = d.x;
                d.trailY[k] = d.y;
                d.trailAlpha[k] = 0.f;
            }
            d.trailTimer = 0.f;
            _megaDrops.push_back(d);
        }
    }

    void spawnResidualDrop(float x, float y, float neckRotation)
    {
        if (!_ptrBloodSprite) return;
        FountainDrop d;
        float rad = neckRotation * 0.0174533f;
        float dirX = sinf(rad);
        float dirY = -cosf(rad);
        float spawnOffset = 8.f;
        d.x = x + dirX * spawnOffset + ((rand() % 6) - 3);
        d.y = y + dirY * spawnOffset + ((rand() % 6) - 3);
        d.prevX = d.x; d.prevY = d.y;
        float speed = 30.f + (rand() % 80);
        float spread = ((rand() % 50) - 25) * 0.0174533f;
        float finalAngle = rad + spread;
        d.vx = sinf(finalAngle) * speed * 0.6f;
        d.vy = -cosf(finalAngle) * speed * 0.4f;
        d.angle = 1.57079633f - atan2f(d.vy, d.vx);
        d.life = 0.f;
        d.maxLife = 0.5f + (rand() % 25) * 0.02f;
        d.scale = 0.15f + (rand() % 20) * 0.01f;
        d.alpha = 0.8f;
        d.trailDistAccum = 0.f;
        d.rotationSpeed = ((rand() % 80) - 40) * 0.15f;
        d.stretch = 1.f;
        d.bounceCount = 0.f;
        d.groundStickTimer = 0.f;
        d.bOnGround = false;
        d.wobblePhase = (rand() % 100) * 0.0628f;
        d.wobbleFreq = WOBBLE_FREQ_BASE + (rand() % 40) * 0.2f;
        d.wobbleAmp = WOBBLE_AMP_BASE + (rand() % 20) * 0.005f;
        d.squashX = 1.f;
        d.squashY = 1.f;
        for (int k = 0; k < FountainDrop::TRAIL_COUNT; ++k)
        {
            d.trailX[k] = d.x;
            d.trailY[k] = d.y;
            d.trailAngle[k] = d.angle;
            d.trailAlpha[k] = 0.f;
        }
        _drops.push_back(d);
    }

    void spawnPulse(float intensity)
    {
        if (!_ptrBloodSprite) return;
        int count = 25 + (int)(intensity * 35.f);
        for (int i = 0; i < count; ++i)
        {
            FountainDrop d;
            d.x = _fX + ((rand() % 20) - 10) * 0.3f;
            d.y = _fY;
            d.prevX = d.x; d.prevY = d.y;
            float angle = -1.5708f + ((rand() % 140) - 70) * 0.0174533f;
            float speed = 500.f + (rand() % 800) + intensity * 600.f;
            d.vx = cosf(angle) * speed;
            d.vy = sinf(angle) * speed;
            d.angle = 1.57079633f - atan2f(d.vy, d.vx);
            d.life = 0.f;
            d.maxLife = 0.9f + (rand() % 40) * 0.02f;
            d.scale = 0.18f + (rand() % 25) * 0.008f + intensity * 0.1f;
            d.alpha = 0.95f;
            d.trailDistAccum = 0.f;
            d.rotationSpeed = ((rand() % 300) - 150) * 0.2f;
            d.stretch = 1.f;
            d.bounceCount = 0.f;
            d.groundStickTimer = 0.f;
            d.bOnGround = false;
            d.wobblePhase = (rand() % 100) * 0.0628f;
            d.wobbleFreq = WOBBLE_FREQ_BASE + (rand() % 40) * 0.2f;
            d.wobbleAmp = WOBBLE_AMP_BASE + (rand() % 20) * 0.005f;
            d.squashX = 1.f;
            d.squashY = 1.f;
            for (int k = 0; k < FountainDrop::TRAIL_COUNT; ++k)
            {
                d.trailX[k] = d.x;
                d.trailY[k] = d.y;
                d.trailAngle[k] = d.angle;
                d.trailAlpha[k] = 0.f;
            }
            _drops.push_back(d);
        }
        spawnArteryPulse(_fX, _fY, 0.f);
        int longCount = 1 + (rand() % 3);
        for (int i = 0; i < longCount; ++i) spawnLongDrop();
    }

    void spawnDrip()
    {
        if (!_ptrBloodSprite) return;
        FountainDrop d;
        d.x = _fX + ((rand() % 10) - 5);
        d.y = _fY + (rand() % 4);
        d.prevX = d.x; d.prevY = d.y;
        d.vx = ((rand() % 40) - 20);
        d.vy = -(150.f + (rand() % 250));
        d.angle = 1.57079633f - atan2f(d.vy, d.vx);
        d.life = 0.f;
        d.maxLife = 0.5f + (rand() % 20) * 0.02f;
        d.scale = 0.15f + (rand() % 15) * 0.01f;
        d.alpha = 0.9f;
        d.trailDistAccum = 0.f;
        d.rotationSpeed = ((rand() % 60) - 30) * 0.1f;
        d.stretch = 1.f;
        d.bounceCount = 0.f;
        d.groundStickTimer = 0.f;
        d.bOnGround = false;
        d.wobblePhase = (rand() % 100) * 0.0628f;
        d.wobbleFreq = WOBBLE_FREQ_BASE + (rand() % 30) * 0.15f;
        d.wobbleAmp = WOBBLE_AMP_BASE * 0.5f + (rand() % 10) * 0.003f;
        d.squashX = 1.f;
        d.squashY = 1.f;
        for (int k = 0; k < FountainDrop::TRAIL_COUNT; ++k)
        {
            d.trailX[k] = d.x;
            d.trailY[k] = d.y;
            d.trailAngle[k] = d.angle;
            d.trailAlpha[k] = 0.f;
        }
        _drops.push_back(d);
    }

    void spawnWeakPulse(float intensity)
    {
        if (!_ptrBloodSprite) return;
        int count = 4 + (int)(intensity * 6.f);
        for (int i = 0; i < count; ++i)
        {
            FountainDrop d;
            d.x = _fX + ((rand() % 12) - 6) * 0.2f;
            d.y = _fY;
            d.prevX = d.x; d.prevY = d.y;
            float angle = -1.5708f + ((rand() % 80) - 40) * 0.0174533f;
            float speed = 120.f + (rand() % 150) + intensity * 180.f;
            d.vx = cosf(angle) * speed;
            d.vy = sinf(angle) * speed;
            d.angle = 1.57079633f - atan2f(d.vy, d.vx);
            d.life = 0.f;
            d.maxLife = 0.6f + (rand() % 20) * 0.01f;
            d.scale = 0.12f + (rand() % 15) * 0.005f + intensity * 0.04f;
            d.alpha = 0.85f;
            d.trailDistAccum = 0.f;
            d.rotationSpeed = ((rand() % 100) - 50) * 0.1f;
            d.stretch = 1.f;
            d.bounceCount = 0.f;
            d.groundStickTimer = 0.f;
            d.bOnGround = false;
            d.wobblePhase = (rand() % 100) * 0.0628f;
            d.wobbleFreq = WOBBLE_FREQ_BASE + (rand() % 20) * 0.1f;
            d.wobbleAmp = WOBBLE_AMP_BASE * 0.4f;
            d.squashX = 1.f;
            d.squashY = 1.f;
            for (int k = 0; k < FountainDrop::TRAIL_COUNT; ++k)
            {
                d.trailX[k] = d.x;
                d.trailY[k] = d.y;
                d.trailAngle[k] = d.angle;
                d.trailAlpha[k] = 0.f;
            }
            _drops.push_back(d);
        }
    }

    void spawnFinalSpurt(float intensity)
    {
        if (!_ptrBloodSprite) return;
        int count = 8 + (int)(intensity * 12.f);
        for (int i = 0; i < count; ++i)
        {
            FountainDrop d;
            d.x = _fX + ((rand() % 16) - 8) * 0.3f;
            d.y = _fY;
            d.prevX = d.x; d.prevY = d.y;
            float angle = -1.5708f + ((rand() % 100) - 50) * 0.0174533f;
            float speed = 250.f + (rand() % 200) + intensity * 300.f;
            d.vx = cosf(angle) * speed;
            d.vy = sinf(angle) * speed;
            d.angle = 1.57079633f - atan2f(d.vy, d.vx);
            d.life = 0.f;
            d.maxLife = 0.7f + (rand() % 20) * 0.01f;
            d.scale = 0.14f + (rand() % 15) * 0.006f + intensity * 0.05f;
            d.alpha = 0.9f;
            d.trailDistAccum = 0.f;
            d.rotationSpeed = ((rand() % 120) - 60) * 0.1f;
            d.stretch = 1.f;
            d.bounceCount = 0.f;
            d.groundStickTimer = 0.f;
            d.bOnGround = false;
            d.wobblePhase = (rand() % 100) * 0.0628f;
            d.wobbleFreq = WOBBLE_FREQ_BASE + (rand() % 20) * 0.1f;
            d.wobbleAmp = WOBBLE_AMP_BASE * 0.5f;
            d.squashX = 1.f;
            d.squashY = 1.f;
            for (int k = 0; k < FountainDrop::TRAIL_COUNT; ++k)
            {
                d.trailX[k] = d.x;
                d.trailY[k] = d.y;
                d.trailAngle[k] = d.angle;
                d.trailAlpha[k] = 0.f;
            }
            _drops.push_back(d);
        }
    }

    void spawnSplash(float x, float y, float impactSpeed)
    {
        int count = 3 + (rand() % 4);
        for (int i = 0; i < count; ++i)
        {
            SplashDrop s;
            s.x = x + ((rand() % 10) - 5);
            s.y = y;
            float a = ((rand() % 360) * 0.0174533f);
            float sp = 40.f + (rand() % 120) * (impactSpeed / 400.f);
            s.vx = cosf(a) * sp;
            s.vy = -sinf(a) * sp * 0.6f;
            s.angle = 1.57079633f - atan2f(s.vy, s.vx);
            s.life = 0.f;
            s.maxLife = 0.15f + (rand() % 15) * 0.01f;
            s.scale = 0.04f + (rand() % 8) * 0.005f;
            s.alpha = 0.85f;
            _splashDrops.push_back(s);
        }
    }

    virtual void update(float dt) override
    {
        CContainer::update(dt);
        _fLife += dt;

        if (_bTrackNeck && _ptrTargetSpine && _bBoneCached)
        {
            float neckX = 0.f, neckY = 0.f, neckRot = 0.f;
            if (getNeckTransform(neckX, neckY, neckRot))
            {
                auto& cfg = Engine::getCfg();
                if (neckX > 80.f && neckX < cfg.INIT_SCR_CX - 80.f &&
                    neckY > 80.f && neckY < cfg.INIT_SCR_CY - 80.f)
                {
                    _fNeckTrackX = neckX;
                    _fNeckTrackY = neckY;
                    _fNeckRotation = neckRot;
                    _fResidualTimer += dt;
                    while (_fResidualTimer >= RESIDUAL_INTERVAL)
                    {
                        _fResidualTimer -= RESIDUAL_INTERVAL;
                        spawnResidualDrop(_fNeckTrackX, _fNeckTrackY, _fNeckRotation);
                    }
                }
            }
        }

        if (!_bStopSpawning)
        {
            _fPulseTime += dt;
            _fAccum += dt;
            _fLongDropAccum += dt;
            _fArteryTimer += dt;
            _fMegaTimer += dt;

            if (_nPulseCount < MAX_PULSES && _fPulseTime >= PULSE_PERIOD)
            {
                _fPulseTime -= PULSE_PERIOD;
                float decay = 1.0f - ((float)_nPulseCount / (float)MAX_PULSES) * 0.85f;
                spawnPulse(decay);
                _nPulseCount++;
            }

            float dripInterval = 0.02f;
            while (_fAccum >= dripInterval)
            {
                _fAccum -= dripInterval;
                spawnDrip();
            }

            float longDropInterval = 0.5f + (rand() % 20) * 0.01f;
            while (_fLongDropAccum >= longDropInterval)
            {
                _fLongDropAccum -= longDropInterval;
                spawnLongDrop();
            }
        }

        float groundY = _fY + GROUND_Y_OFFSET;

        for (auto it = _splashDrops.begin(); it != _splashDrops.end(); )
        {
            auto& s = *it;
            s.vy += GRAVITY * 0.8f * dt;
            s.x += s.vx * dt;
            s.y += s.vy * dt;
            s.life += dt;
            s.alpha = std::max(0.f, s.alpha - dt * 4.f);
            if (s.y > groundY) { s.y = groundY; s.vy = -s.vy * 0.3f; s.vx *= 0.7f; }
            if (s.life >= s.maxLife || s.alpha <= 0.01f) it = _splashDrops.erase(it);
            else ++it;
        }

        for (auto it = _arteryDrops.begin(); it != _arteryDrops.end(); )
        {
            auto& d = *it;
            d.life += dt;
            d.pulsePhase += dt * d.pulseFreq;
            float pulse = sinf(d.pulsePhase) * 0.3f + 0.7f;
            d.alpha = (1.f - d.life / d.maxLife) * pulse;
            if (d.life >= d.maxLife || d.alpha <= 0.01f) it = _arteryDrops.erase(it);
            else ++it;
        }

        for (auto it = _megaDrops.begin(); it != _megaDrops.end(); )
        {
            auto& d = *it;
            if (!d.bOnGround)
            {
                d.vy += GRAVITY * dt;
                d.vx *= AIR_RESISTANCE;
                d.x += d.vx * dt;
                d.y += d.vy * dt;
                d.rotation += d.rotSpeed * dt;
                float speed = sqrtf(d.vx * d.vx + d.vy * d.vy);
                if (speed > 10.f) d.moveAngle = 1.57079633f - atan2f(d.vy, d.vx);
                float impactFactor = std::min(1.f, speed / 800.f);
                d.squashY = 1.f + impactFactor * 0.3f;
                d.squashX = std::max(0.7f, 1.f - impactFactor * 0.15f);
                d.trailTimer += dt;
                if (d.trailTimer >= 0.015f)
                {
                    d.trailTimer = 0.f;
                    for (int i = MegaDrop::TRAIL_COUNT - 1; i > 0; --i)
                    {
                        d.trailX[i] = d.trailX[i - 1];
                        d.trailY[i] = d.trailY[i - 1];
                        d.trailAlpha[i] = d.trailAlpha[i - 1];
                    }
                    d.trailX[0] = d.x;
                    d.trailY[0] = d.y;
                    d.trailAlpha[0] = d.alpha * 0.8f;
                }
                if (d.y > groundY)
                {
                    d.y = groundY;
                    d.vy = -d.vy * BOUNCE_DAMPING;
                    d.vx *= GROUND_FRICTION;
                    d.bounceCount++;
                    d.squashX = std::min(1.3f, 1.4f);
                    d.squashY = 0.6f;
                    spawnPool(d.x, groundY, 250.f);
                    if (d.bounceCount >= 2 || fabs(d.vy) < 50.f)
                    {
                        d.bOnGround = true;
                        d.groundStickTimer = 0.3f + (rand() % 20) * 0.01f;
                        d.squashX = 1.2f;
                        d.squashY = 0.7f;
                    }
                }
            }
            else
            {
                d.squashX += (1.f - d.squashX) * 5.f * dt;
                d.squashY += (1.f - d.squashY) * 5.f * dt;
                d.groundStickTimer -= dt;
                d.alpha *= 0.95f;
                d.scale *= 0.97f;
            }
            d.life += dt;
            if (d.life >= d.maxLife || d.alpha <= 0.01f || (d.bOnGround && d.groundStickTimer <= 0.f))
                it = _megaDrops.erase(it);
            else
                ++it;
        }

        if (_ptrBloodSprite)
        {
            for (auto it = _drops.begin(); it != _drops.end(); )
            {
                auto& d = *it;
                if (d.bOnGround)
                {
                    d.squashX += (1.f - d.squashX) * 6.f * dt;
                    d.squashY += (1.f - d.squashY) * 6.f * dt;
                    d.groundStickTimer -= dt;
                    d.alpha *= 0.90f;
                    d.scale *= 0.96f;
                    if (d.groundStickTimer <= 0.f || d.alpha <= 0.01f)
                    {
                        it = _drops.erase(it);
                        continue;
                    }
                    ++it;
                    continue;
                }

                d.vy += GRAVITY * dt;
                d.vx *= AIR_RESISTANCE;
                d.x += d.vx * dt;
                d.y += d.vy * dt;
                d.life += dt;
                d.wobblePhase += dt * d.wobbleFreq;
                float speed = sqrtf(d.vx * d.vx + d.vy * d.vy);
                float targetStretch = 1.f + std::min(MAX_STRETCH - 1.f, speed * STRETCH_FACTOR);
                d.stretch += (targetStretch - d.stretch) * 10.f * dt;
                float wobbleMul = std::min(1.f, speed / 300.f);
                float wobble = sinf(d.wobblePhase) * d.wobbleAmp * wobbleMul;
                d.angle = 1.57079633f - atan2f(d.vy, d.vx) + wobble;

                float dx = d.x - d.prevX;
                float dy = d.y - d.prevY;
                float distMoved = sqrtf(dx * dx + dy * dy);
                d.trailDistAccum += distMoved;
                d.prevX = d.x; d.prevY = d.y;

                constexpr float TRAIL_SPAWN_DIST = 5.f;
                if (d.trailDistAccum >= TRAIL_SPAWN_DIST)
                {
                    d.trailDistAccum = 0.f;
                    for (int i = FountainDrop::TRAIL_COUNT - 1; i > 0; --i)
                    {
                        d.trailX[i] = d.trailX[i - 1];
                        d.trailY[i] = d.trailY[i - 1];
                        d.trailAngle[i] = d.trailAngle[i - 1];
                        d.trailAlpha[i] = d.trailAlpha[i - 1];
                    }
                    d.trailX[0] = d.x;
                    d.trailY[0] = d.y;
                    d.trailAngle[0] = d.angle;
                    d.trailAlpha[0] = d.alpha;
                }

                if (d.life > d.maxLife * 0.35f)
                {
                    float fadeT = (d.life - d.maxLife * 0.35f) / (d.maxLife * 0.65f);
                    fadeT = fadeT * fadeT;
                    d.alpha = std::max(0.f, 0.95f * (1.f - fadeT));
                }

                if (d.y > groundY)
                {
                    float impactSpeed = sqrtf(d.vx * d.vx + d.vy * d.vy);
                    d.y = groundY;
                    if (d.bounceCount < 2 && fabs(d.vy) > 100.f)
                    {
                        d.vy = -d.vy * BOUNCE_DAMPING;
                        d.vx *= GROUND_FRICTION;
                        d.bounceCount += 1.f;
                        d.squashX = std::min(1.3f, 1.35f + impactSpeed * 0.0003f);
                        d.squashY = 0.65f - impactSpeed * 0.0002f;
                        if (impactSpeed > 300.f && _megaDrops.size() < 50)
                            spawnMegaDrop(d.x, groundY, impactSpeed);
                        if (impactSpeed > 200.f)
                            spawnSplash(d.x, groundY, impactSpeed);
                        if (impactSpeed > 120.f)
                            spawnPool(d.x, groundY, impactSpeed * 0.35f);
                    }
                    else
                    {
                        d.bOnGround = true;
                        d.groundStickTimer = 0.5f + (rand() % 30) * 0.01f;
                        d.vx = 0.f; d.vy = 0.f;
                        d.squashX = 1.3f; d.squashY = 0.65f;
                        if (impactSpeed > 150.f)
                            spawnSplash(d.x, groundY, impactSpeed);
                        spawnPool(d.x, groundY, 90.f);
                    }
                }

                if (d.life >= d.maxLife || d.alpha <= 0.01f)
                    it = _drops.erase(it);
                else
                    ++it;
            }
            if (_drops.size() > 1800)
                _drops.erase(_drops.begin(), _drops.begin() + (_drops.size() - 1800));
        }

        // [POOL] Обновление луж — микро-дыхание, без удаления
        for (auto& p : _pools)
        {
            p.age += dt;
            p.wobblePhase += dt * p.wobbleFreq;

            float remaining = p.maxScale - p.currentScale;
            if (remaining > 0.001f)
            {
                float grow = std::max(0.008f, remaining * p.growSpeed);
                p.currentScale += grow * dt;
            }

            float breathe = sinf(p.wobblePhase) * p.wobbleAmp;
            p.scaleX = p.currentScale * (1.f + breathe);
            p.scaleY = p.currentScale * p.spreadRatio * (1.f - breathe * 0.5f);
        }

        for (auto it = _longDrops.begin(); it != _longDrops.end(); )
        {
            auto& ld = *it;
            ld.life += dt;
            switch (ld.phase)
            {
                case LongDrop::DELAY:
                    if (ld.life >= ld.startDelay) { ld.phase = LongDrop::GROW; ld.life = 0.f; }
                    break;
                case LongDrop::GROW:
                {
                    float growT = std::min(1.f, ld.life / ld.growDuration);
                    float easedT = 1.f - powf(1.f - growT, 3.f);
                    ld.scaleY = ld.targetScaleY * easedT;
                    ld.alpha = ld.targetAlpha * easedT;
                    if (growT >= 1.f) { ld.phase = LongDrop::HOLD; ld.life = 0.f; }
                    break;
                }
                case LongDrop::HOLD:
                    if (ld.life >= ld.fadeDelay) { ld.phase = LongDrop::FADE; ld.life = 0.f; }
                    break;
                case LongDrop::FADE:
                {
                    float fadeT = std::min(1.f, ld.life / ld.fadeDuration);
                    float easedFade = fadeT * fadeT * fadeT;
                    ld.alpha = ld.targetAlpha * (1.f - easedFade);
                    if (fadeT >= 1.f) ld.phase = LongDrop::DONE;
                    break;
                }
                case LongDrop::DONE: break;
            }
            if (ld.phase == LongDrop::DONE) it = _longDrops.erase(it);
            else ++it;
        }
    }

    // [NEW] Отрисовка луж в отдельный слой (за персонажем) с перспективой
    // Теперь использует _ptrPoolSprite (UI/bloodSplatter) вместо _ptrBloodSprite
    void renderPools(float dt, float* rgba)
    {
        if (_pools.empty() || !_ptrPoolSprite) return;
        auto ms = CGfx::getInstance()->getMatrixStack();

        float savedVerts[CSprite::VERT_COUNT];
        memcpy(savedVerts, _ptrPoolSprite->_verts, sizeof(savedVerts));

        Rect rcOrig;
        _ptrPoolSprite->getNotTransBounds(&rcOrig);
        float halfW = rcOrig.cx * 0.5f;
        float halfH = rcOrig.cy * 0.5f;
        float centerVerts[CSprite::VERT_COUNT];
        centerVerts[CSprite::VERT_BLX] = -halfW; centerVerts[CSprite::VERT_BLY] = halfH;
        centerVerts[CSprite::VERT_ULX] = -halfW; centerVerts[CSprite::VERT_ULY] = -halfH;
        centerVerts[CSprite::VERT_URX] = halfW;  centerVerts[CSprite::VERT_URY] = -halfH;
        centerVerts[CSprite::VERT_BRX] = halfW;  centerVerts[CSprite::VERT_BRY] = halfH;
        memcpy(_ptrPoolSprite->_verts, centerVerts, sizeof(centerVerts));
        auto& cfg = Engine::getCfg();
        float screenH = (float)cfg.INIT_SCR_CY;
        const float perspBase = -1.50f;
        const float perspRange = 0.95f;

        for (const auto& p : _pools)
        {
            if (p.alpha <= 0.01f) continue;

            float perspective = perspBase + (p.y / screenH) * perspRange;

            ms->save();
            ms->translate(p.x, p.y);
            ms->scale(p.scaleX, p.scaleY * perspective);

            //float poolRgba[4] = { 0.42f, 0.0f, 0.0f, p.alpha * 0.45f };
            float poolRgba[4] = { 0.92f * rgba[0], .09f * rgba[1], .09f * rgba[2], p.alpha * 0.55f * rgba[3]};
            _ptrPoolSprite->renderSelf(dt, poolRgba);
            ms->restore();
        }

        memcpy(_ptrPoolSprite->_verts, savedVerts, sizeof(savedVerts));
    }

    virtual void renderSelf(float dt, float* rgba) override
    {
        CContainer::renderSelf(dt, rgba);
        auto ms = CGfx::getInstance()->getMatrixStack();

        // Лужи больше не рисуются здесь — они идут через PoolRenderLayer

        if (!_longDrops.empty())
        {
            for (const auto& ld : _longDrops)
            {
                if (!ld.ptrSprite || ld.alpha <= 0.01f || ld.scaleY <= 0.01f) continue;
                float savedVerts[CSprite::VERT_COUNT];
                memcpy(savedVerts, ld.ptrSprite->_verts, sizeof(savedVerts));
                Rect rcOrig;
                ld.ptrSprite->getNotTransBounds(&rcOrig);
                float halfW = rcOrig.cx * 0.5f;
                float centerVerts[CSprite::VERT_COUNT];
                centerVerts[CSprite::VERT_BLX] = -halfW; centerVerts[CSprite::VERT_BLY] = rcOrig.cy;
                centerVerts[CSprite::VERT_ULX] = -halfW; centerVerts[CSprite::VERT_ULY] = 0.f;
                centerVerts[CSprite::VERT_URX] = halfW;  centerVerts[CSprite::VERT_URY] = 0.f;
                centerVerts[CSprite::VERT_BRX] = halfW;  centerVerts[CSprite::VERT_BRY] = rcOrig.cy;
                memcpy(ld.ptrSprite->_verts, centerVerts, sizeof(centerVerts));

                float wobbleX = sinf(ld.life * ld.wobbleFreq + ld.wobblePhase) * ld.wobbleAmp;
                float colorPulse = sinf(ld.life * ld.colorFreq + ld.colorPhase);
                float r = 0.95f + colorPulse * 0.05f;
                float g = 0.005f + colorPulse * 0.003f;
                float b = 0.002f;

                ms->save();
                ms->translate(ld.x + wobbleX, ld.y);
                ms->scale(ld.scaleX, ld.scaleY);
                ms->rotate(sinf(ld.life * 0.5f + ld.wobblePhase) * 0.05f);
                float dropRgba[4] = { r, g, b, ld.alpha * 0.5f };
                ld.ptrSprite->renderSelf(dt, dropRgba);
                ms->restore();

                memcpy(ld.ptrSprite->_verts, savedVerts, sizeof(savedVerts));
            }
        }

        if (!_arteryDrops.empty() && _ptrBloodSprite)
        {
            float savedVerts[CSprite::VERT_COUNT];
            memcpy(savedVerts, _ptrBloodSprite->_verts, sizeof(savedVerts));
            Rect rcOrig;
            _ptrBloodSprite->getNotTransBounds(&rcOrig);
            float halfW = rcOrig.cx * 0.5f;
            float halfH = rcOrig.cy * 0.5f;
            float centerVerts[CSprite::VERT_COUNT];
            centerVerts[CSprite::VERT_BLX] = -halfW; centerVerts[CSprite::VERT_BLY] = halfH;
            centerVerts[CSprite::VERT_ULX] = -halfW; centerVerts[CSprite::VERT_ULY] = -halfH;
            centerVerts[CSprite::VERT_URX] = halfW;  centerVerts[CSprite::VERT_URY] = -halfH;
            centerVerts[CSprite::VERT_BRX] = halfW;  centerVerts[CSprite::VERT_BRY] = halfH;
            memcpy(_ptrBloodSprite->_verts, centerVerts, sizeof(centerVerts));

            for (const auto& d : _arteryDrops)
            {
                if (d.alpha <= 0.01f) continue;
                float pulseScale = 1.f + sinf(d.pulsePhase) * 0.4f;
                ms->save();
                ms->translate(d.x, d.y);
                ms->rotate(d.angle);
                ms->scale(d.scale * pulseScale, d.scale * pulseScale);
                float brightness = 0.3f + d.alpha * 0.7f;
                float dropRgba[4] = { 1.0f * brightness, 0.01f * brightness, 0.005f * brightness, d.alpha };
                _ptrBloodSprite->renderSelf(dt, dropRgba);
                ms->restore();
            }
            memcpy(_ptrBloodSprite->_verts, savedVerts, sizeof(savedVerts));
        }

        if (!_megaDrops.empty() && _ptrBloodSprite)
        {
            float savedVerts[CSprite::VERT_COUNT];
            memcpy(savedVerts, _ptrBloodSprite->_verts, sizeof(savedVerts));
            Rect rcOrig;
            _ptrBloodSprite->getNotTransBounds(&rcOrig);
            float halfW = rcOrig.cx * 0.5f;
            float halfH = rcOrig.cy * 0.5f;
            float centerVerts[CSprite::VERT_COUNT];
            centerVerts[CSprite::VERT_BLX] = -halfW; centerVerts[CSprite::VERT_BLY] = halfH;
            centerVerts[CSprite::VERT_ULX] = -halfW; centerVerts[CSprite::VERT_ULY] = -halfH;
            centerVerts[CSprite::VERT_URX] = halfW;  centerVerts[CSprite::VERT_URY] = -halfH;
            centerVerts[CSprite::VERT_BRX] = halfW;  centerVerts[CSprite::VERT_BRY] = halfH;
            memcpy(_ptrBloodSprite->_verts, centerVerts, sizeof(centerVerts));

            for (const auto& d : _megaDrops)
            {
                if (d.alpha <= 0.01f) continue;
                for (int i = MegaDrop::TRAIL_COUNT - 1; i >= 0; --i)
                {
                    float trailA = d.trailAlpha[i] * powf(0.75f, i);
                    if (trailA <= 0.01f) continue;
                    ms->save();
                    ms->translate(d.trailX[i], d.trailY[i]);
                    if (!d.bOnGround) ms->rotate(d.moveAngle);
                    float trailScale = d.scale * powf(0.85f, i) * 0.6f;
                    ms->scale(trailScale * d.squashX, trailScale * d.squashY);
                    float trailRgba[4] = { 0.98f, 0.02f, 0.005f, trailA };
                    _ptrBloodSprite->renderSelf(dt, trailRgba);
                    ms->restore();
                }
                ms->save();
                ms->translate(d.x, d.y);
                if (!d.bOnGround) ms->rotate(d.moveAngle);
                else ms->rotate(d.rotation);
                ms->scale(d.scale * d.squashX, d.scale * d.squashY * 0.85f);
                float dropRgba[4] = { 1.0f, 0.02f, 0.005f, d.alpha };
                _ptrBloodSprite->renderSelf(dt, dropRgba);
                ms->restore();
            }
            memcpy(_ptrBloodSprite->_verts, savedVerts, sizeof(savedVerts));
        }

        if (_ptrBloodSprite && !_drops.empty())
        {
            float savedVerts[CSprite::VERT_COUNT];
            memcpy(savedVerts, _ptrBloodSprite->_verts, sizeof(savedVerts));
            Rect rcOrig;
            _ptrBloodSprite->getNotTransBounds(&rcOrig);
            float halfW = rcOrig.cx * 0.5f;
            float halfH = rcOrig.cy * 0.5f;
            float centerVerts[CSprite::VERT_COUNT];
            centerVerts[CSprite::VERT_BLX] = -halfW; centerVerts[CSprite::VERT_BLY] = halfH;
            centerVerts[CSprite::VERT_ULX] = -halfW; centerVerts[CSprite::VERT_ULY] = -halfH;
            centerVerts[CSprite::VERT_URX] = halfW;  centerVerts[CSprite::VERT_URY] = -halfH;
            centerVerts[CSprite::VERT_BRX] = halfW;  centerVerts[CSprite::VERT_BRY] = halfH;
            memcpy(_ptrBloodSprite->_verts, centerVerts, sizeof(centerVerts));

            for (const auto& d : _drops)
            {
                if (d.alpha <= 0.01f) continue;
                if (!d.bOnGround)
                {
                    for (int i = FountainDrop::TRAIL_COUNT - 1; i >= 0; --i)
                    {
                        float t = (float)i / FountainDrop::TRAIL_COUNT;
                        float trailA = d.trailAlpha[i] * (1.f - t * t);
                        if (trailA <= 0.01f) continue;
                        float scaleMul = 1.f - t * 0.55f;
                        float speed = sqrtf(d.vx * d.vx + d.vy * d.vy);
                        float bright = std::min(1.f, speed / 1000.f);
                        float r = (1.0f - t * 0.15f) + bright * 0.1f;
                        float g = (0.01f - t * 0.008f) + bright * 0.005f;
                        float b = 0.003f;

                        ms->save();
                        ms->translate(d.trailX[i], d.trailY[i]);
                        ms->rotate(d.trailAngle[i]);
                        float trailScale = d.scale * scaleMul;
                        ms->scale(trailScale * 0.5f, trailScale * 0.85f);
                        float trailRgba[4] = { r, g, b, trailA };
                        _ptrBloodSprite->renderSelf(dt, trailRgba);
                        ms->restore();
                    }

                    ms->save();
                    ms->translate(d.x, d.y);
                    ms->rotate(d.angle);
                    ms->scale(d.scale * d.squashX * 0.7f, d.scale * d.stretch * d.squashY * 0.9f);
                    float speed = sqrtf(d.vx * d.vx + d.vy * d.vy);
                    float brightness = std::min(1.0f, speed / 1100.f);
                    float dropRgba[4] = {
                        1.0f,
                        0.01f + brightness * 0.01f,
                        0.003f + brightness * 0.002f,
                        d.alpha
                    };
                    _ptrBloodSprite->renderSelf(dt, dropRgba);
                    ms->restore();
                }
                else
                {
                    ms->save();
                    ms->translate(d.x, d.y);
                    ms->scale(d.scale * d.squashX * 1.15f, d.scale * d.squashY * 0.45f);
                    float dropRgba[4] = { 0.7f, 0.005f, 0.002f, d.alpha * 0.6f };
                    _ptrBloodSprite->renderSelf(dt, dropRgba);
                    ms->restore();
                }
            }
            memcpy(_ptrBloodSprite->_verts, savedVerts, sizeof(savedVerts));
        }

        if (_ptrBloodSprite && !_splashDrops.empty())
        {
            float savedVerts[CSprite::VERT_COUNT];
            memcpy(savedVerts, _ptrBloodSprite->_verts, sizeof(savedVerts));
            Rect rcOrig;
            _ptrBloodSprite->getNotTransBounds(&rcOrig);
            float halfW = rcOrig.cx * 0.5f;
            float halfH = rcOrig.cy * 0.5f;
            float centerVerts[CSprite::VERT_COUNT];
            centerVerts[CSprite::VERT_BLX] = -halfW; centerVerts[CSprite::VERT_BLY] = halfH;
            centerVerts[CSprite::VERT_ULX] = -halfW; centerVerts[CSprite::VERT_ULY] = -halfH;
            centerVerts[CSprite::VERT_URX] = halfW;  centerVerts[CSprite::VERT_URY] = -halfH;
            centerVerts[CSprite::VERT_BRX] = halfW;  centerVerts[CSprite::VERT_BRY] = halfH;
            memcpy(_ptrBloodSprite->_verts, centerVerts, sizeof(centerVerts));

            for (const auto& s : _splashDrops)
            {
                if (s.alpha <= 0.01f) continue;
                ms->save();
                ms->translate(s.x, s.y);
                ms->rotate(s.angle);
                ms->scale(s.scale * 0.6f, s.scale * 0.9f);
                float splashRgba[4] = { 1.0f * rgba[0], 0.01f * rgba[1], 0.003f * rgba[2], s.alpha * rgba[3] };
                _ptrBloodSprite->renderSelf(dt, splashRgba);
                ms->restore();
            }
            memcpy(_ptrBloodSprite->_verts, savedVerts, sizeof(savedVerts));
        }
    }
};
using BloodFountainPtr = std::shared_ptr<BloodFountain>;

// ------------------------------------------------------------------
// Слой-прокси для отрисовки луж крови за персонажем
// ------------------------------------------------------------------
class PoolRenderLayer : public CContainer {
public:
    BloodFountainPtr fountain;
    virtual void renderSelf(float dt, float* rgba) override {
        if (fountain) fountain->renderPools(dt, rgba);
    }
};

class DialogBeheading : public BaseDialog
{
private:
    CSpinePtr _ptrSpine;
    CContainerPtr _ptrContent;
    BloodFountainPtr _ptrFountain;
    CContainerPtr _ptrPoolLayer;
    float _fNeckX = 0.f, _fNeckY = 0.f;
    Point _neckOffset = {0.f, -190.f};

public:
    void init(CSpinePtr ptrSpine, Point neckOffset = {0.f, -190.f})
    {
        auto& cfg = Engine::getCfg();
        _ptrSpine   = ptrSpine;
        _neckOffset = neckOffset;
        BaseDialog::init(cfg.INIT_SCR_CX, cfg.INIT_SCR_CY, E_ST_NONE, nullptr, nullptr);
        _eCloseStyle = eDialogCloseStyle::CLOSE_BUTTON;
        _ptrContent = std::make_shared<CContainer>();
        //_ptrSpine->setReflectionType(eReflectionType::CAN_BE_REFLECTED);
        ptrSpine->getParentDialog()->addChild(_ptrContent);
    }

    virtual void onShow(bool bShow) override
    {
        if (bShow)
        {
            CGfx::getInstance()->setWorldDarken(false);
            CGfx::getInstance()->getPostProcessSettings().tintB = 1.f;
            CGfx::getInstance()->getPostProcessSettings().tintR = 1.f;
            CGfx::getInstance()->getPostProcessSettings().tintG = 1.f;
            //addSpine();
            Point pt(CSceneResize::getInstance()->getBgWidth() * 0.5f, CSceneResize::getInstance()->getBgHeight() * 0.5f);

            //addSelfTween(eTweenProp::DISTORTION, 0.0f, .05f, 1.f,Easing::linear, 0.f);
            setTimeout(.3f,[this]{
                addSelfTween(eTweenProp::VIGNETTE_RADIUS, CGfx::getInstance()->getPostProcessSettings().vignetteRadius, 0.01f, 5.f);
                addSelfTween(eTweenProp::VIGNETTE_INTENCITY, CGfx::getInstance()->getPostProcessSettings().vignetteIntensity, 1.f, 5.f);
                addSelfTween(eTweenProp::BREATH, CGfx::getInstance()->getPostProcessSettings().vignetteBreatheIntensity, 1.f, 5.f);
                startSequence();
            });
            AudioManager::get().playSound("dark", 1.f);
        }
        else
        {
            removeSelfTweens();
            CGfx::getInstance()->getPostProcessSettings().fBloodDrops = 0.f;
        }
    }

    virtual void update(float dt) override
    {
        BaseDialog::update(dt);
    }

    void startSequence()
    {
        _ptrFountain.reset();
        _ptrPoolLayer.reset();
        
        float spineX = _ptrSpine->getX();
        float spineY = _ptrSpine->getY();

        _fNeckX = spineX + _neckOffset.x;
        _fNeckY = spineY + _neckOffset.y;

        auto ptrAxe = SpriteLoader::getInstance()->getSprite("UI/scythe");
        if (!ptrAxe) { setTimeout(0.6f, [this]{ onImpact(); }); return; }

        auto ptrAxeCont = std::make_shared<CContainer>();
        _ptrContent->addChild(ptrAxeCont);
        ptrAxeCont->addChild(ptrAxe);
        ptrAxe->setPivot(0.5f, 1.0f);

        constexpr float DEG2RAD = 3.14159265f / 180.f;
        float startX = _fNeckX - 140.f;
        float startY = _fNeckY - 520.f;
        ptrAxeCont->setPos(startX, startY);
        ptrAxeCont->setScale(0.45f, 0.45f);
        ptrAxeCont->rotate(-105.f * DEG2RAD);

        ptrAxeCont->addSelfTween(eTweenProp::X, startX, _fNeckX - 50.f, 0.85f, Easing::outSine);
        ptrAxeCont->addSelfTween(eTweenProp::Y, startY, _fNeckY - 190.f, 0.85f, Easing::outSine);
        ptrAxeCont->addSelfTween(eTweenProp::SCALE_ABS, 0.45f, 2.9f, 0.85f, Easing::outCubic);
        ptrAxeCont->addSelfTween(eTweenProp::ROTATE, -105.f * DEG2RAD, -85.f * DEG2RAD, 0.85f, Easing::outSine);

        ptrAxeCont->addSelfTween(eTweenProp::SCALE_ABS, 2.9f, 1.7f, 0.2f, Easing::inBack, 0.85f);
        ptrAxeCont->addSelfTween(eTweenProp::ROTATE, -85.f * DEG2RAD, -140.f * DEG2RAD, 0.25f, Easing::outQuad, 0.85f);
        ptrAxeCont->addSelfTween(eTweenProp::X, _fNeckX - 50.f, _fNeckX - 25.f, 0.25f, Easing::linear, 0.85f);
        ptrAxeCont->addSelfTween(eTweenProp::Y, _fNeckY - 190.f, _fNeckY - 160.f, 0.25f, Easing::linear, 0.85f);

        float chopSX = _fNeckX - 25.f, chopSY = _fNeckY - 160.f;
        float chopEX = _fNeckX + 50.f, chopEY = _fNeckY + 140.f;
        setTimeout(1.16f, [this]{ onImpact(); });
        
        ptrAxeCont->addSelfTween(eTweenProp::X, chopSX, chopEX, 0.20f, Easing::inCubic, 1.10f);
        ptrAxeCont->addSelfTween(eTweenProp::Y, chopSY, chopEY, 0.20f, Easing::inCubic, 1.10f);
        ptrAxeCont->addSelfTween(eTweenProp::ROTATE, -140.f * DEG2RAD, 30.f * DEG2RAD, 0.20f, Easing::inCubic, 1.10f,
        [this, ptrAxeCont]{
            if (ptrAxeCont && ptrAxeCont->getParent()) {
                ptrAxeCont->setVisible(false);
                setTimeout(0.05f, [ptrAxeCont]{
                    if (ptrAxeCont && ptrAxeCont->getParent()) ptrAxeCont->removeFromParent();
                });
            }
        });

        for (int i = 0; i < 8; ++i)
        {
            float trailDelay = 1.12f + i * 0.028f;
            setTimeout(trailDelay, [this, ptrAxeCont, ptrAxe, i]() {
                if (!ptrAxeCont || !ptrAxeCont->getParent() || !ptrAxe) return;
                auto trailSprite = SpriteLoader::getInstance()->getSprite("UI/scythe");
                if (!trailSprite) return;

                auto trailCont = std::make_shared<CContainer>();
                _ptrContent->addChild(trailCont);
                trailCont->addChild(trailSprite);
                trailSprite->setPivot(0.5f, 1.0f);

                trailCont->setPos(ptrAxeCont->getX(), ptrAxeCont->getY());
                trailCont->rotate(ptrAxeCont->getRotate());
                trailCont->setScale(ptrAxeCont->getScaleX(), ptrAxeCont->getScaleY());
                trailCont->setAlpha(1.f);

                float startAlpha = 0.45f - i * 0.07f;
                trailSprite->setAlpha(startAlpha * 3.5f);

                trailSprite->addSelfTween(eTweenProp::ALPHA, startAlpha, 0.f, 0.76f, Easing::linear, 0.f, [trailCont]{
                    if (trailCont && trailCont->getParent()) trailCont->removeFromParent();
                });
            });
        }
    }

    void startDeathTwitch(CSpinePtr ptrSpine, float duration)
    {
        if (!ptrSpine) return;
        float baseX = ptrSpine->getX(), baseY = ptrSpine->getY();
        float elapsed = 0.f, baseInterval = 0.06f;
        int steps = static_cast<int>(duration / baseInterval);
        
        for (int i = 0; i < steps; ++i)
        {
            float delay = elapsed;
            float rhythm = 0.6f + (rand() % 100) * 0.008f;
            elapsed += baseInterval * rhythm;
            
            setTimeout(delay, [this, ptrSpine, baseX, baseY, duration, delay](){
                if (!ptrSpine || !ptrSpine->getParent()) return;
                
                float progress = delay / duration;
                float ampMul, freqMul, timeScale;
                if (progress < 0.25f) {
                    ampMul = 1.0f; freqMul = 0.7f; timeScale = 1.0f - progress * 1.2f;
                } else if (progress < 0.60f) {
                    float t = (progress - 0.25f) / 0.35f;
                    ampMul = 1.0f - t * 0.65f;
                    freqMul = 0.7f + t * 1.1f;
                    timeScale = 0.7f - t * 0.45f;
                } else {
                    float t = (progress - 0.60f) / 0.40f;
                    ampMul = 0.35f - t * 0.20f;
                    freqMul = 1.8f + t * 0.7f;
                    timeScale = 0.25f - t * 0.13f;
                }
                
                if (timeScale < 0.08f) timeScale = 0.08f;
                ptrSpine->setTimeScale(timeScale);
                
                if (ampMul < 0.04f) return;
                ptrSpine->removeSelfTweens();
                
                float ox = ((rand() % 14) - 7) * 0.5f * ampMul;
                float oy = ((rand() % 10) - 5) * 0.35f * ampMul;
                
                float jerk = 0.03f / freqMul;
                float ret  = 0.05f / freqMul;
                
                ptrSpine->addSelfTween(eTweenProp::X, baseX, baseX + ox, jerk, Easing::outQuad);
                ptrSpine->addSelfTween(eTweenProp::X, baseX + ox, baseX, ret, Easing::inOutQuad, jerk);
                ptrSpine->addSelfTween(eTweenProp::Y, baseY, baseY + oy, jerk, Easing::outQuad);
                ptrSpine->addSelfTween(eTweenProp::Y, baseY + oy, baseY, ret, Easing::inOutQuad, jerk);
            });
        }
    }

    void onImpact()
    {
        _ptrSpine->setVisible(false);
        CGfx::getInstance()->startShake(18.f);
        AudioManager::get().playSound("theircoming", 1.f);

        addSelfTween(eTweenProp::FLESH, 0.f, 4.25f, 3.5f);
        addSelfTween(eTweenProp::BLOOD_DROPS, 0.f, 1.f, 17.5f);
        setTimeout(0.08f, [this]{
            _ptrSpine->setVisible(true);
            _ptrSpine->setAttachment("head", nullptr);
            _ptrSpine->setAnimation(0, "idle", true);

            startDeathTwitch(_ptrSpine, 3.5f);
            _ptrFountain = std::make_shared<BloodFountain>();
            _ptrFountain->init(_fNeckX, _fNeckY, _ptrSpine);

            auto bg = _ptrSpine->getParent();

            // === ЛУЖИ: отдельный слой за персонажем ===
            _ptrPoolLayer = std::make_shared<PoolRenderLayer>();
            //std::static_pointer_cast<PoolRenderLayer>(_ptrPoolLayer)->setReflectionType(eReflectionType::MIRROR);
            std::static_pointer_cast<PoolRenderLayer>(_ptrPoolLayer)->fountain = _ptrFountain;
            bg->addChild(_ptrPoolLayer);

            // Перемещаем spine поверх луж
            _ptrSpine->removeFromParent();
            bg->addChild(_ptrSpine);

            // === ФОНТАН (частицы): поверх персонажа ===
            bg->addChild(_ptrFountain);

            _ptrFountain->startNeckTracking("neck", "head");
            _ptrFountain->spawnPulse(1.9f);

            setTimeout(3.0f, [this]{
                if (_ptrFountain) {
                    _ptrFountain->stopSpawning();
                    _ptrFountain->stopNeckTracking();
                }
                setTimeout(.8f, [this]{
                    if (_ptrFountain) _ptrFountain->spawnFinalSpurt(1.75f);
                    setTimeout(0.6f, [this]{
                        if (_ptrFountain) _ptrFountain->spawnFinalSpurt(0.7f);
                        setTimeout(0.3f, [this]{                        
                            if (_ptrFountain) _ptrFountain->spawnFinalSpurt(0.2f);
                            setTimeout(0.35f, [this]{
                                if (_ptrSpine) {
                                    _ptrSpine->removeSelfTweens();
                                    _ptrSpine->setTimeScale(1.0f);
                                }
                                removeSelfTweens();
                                addSelfTween(eTweenProp::BLOOD_DROPS, 1.f, 0.f, 0.8f);
                                setTimeout(0.25f, [this]{
                                    if (_ptrSpine) _ptrSpine->setAnimation(0, "death_stay", false);
                                    if (_ptrFountain) _ptrFountain->startNeckTracking("neck", "neck");
                                    constexpr float FALL_TIME = 2.0f, FADE_TIME = 0.8f;
                                    if (_ptrFountain) {
                                        _ptrFountain->fadeOutAllLongDrops(FADE_TIME);
                                        _ptrFountain->fadeOutAllDrops(FADE_TIME);
                                    }
                                    addSelfTween(eTweenProp::VIGNETTE_RADIUS, CGfx::getInstance()->getPostProcessSettings().vignetteRadius, 0.f, .1f);
                                    addSelfTween(eTweenProp::VIGNETTE_INTENCITY, CGfx::getInstance()->getPostProcessSettings().vignetteIntensity, 0.f, .1f);
                                    addSelfTween(eTweenProp::BREATH, CGfx::getInstance()->getPostProcessSettings().vignetteBreatheIntensity, .0f, .1f);
                                    addSelfTween(eTweenProp::FLESH, 4.25f, 0.f, .2f);
                                    CGfx::getInstance()->setWorldDarken(true, 0.f, 0.f);
                                    _ptrPoolLayer->addSelfTween(eTweenProp::ALPHA, 1.f, 0.f, 1.f, Easing::linear, 0.f, [this]{
                                        close();
                                        //_ptrPoolLayer->removeFromParent();
                                    });
                                    //CGfx::getInstance()->setWorldDarken(true, 0.f, 0.f);
                                        
                                    //setTimeout(0.8f, [this]{
                                    //    _ptrSpine->removeFromParent();
                                    //});
                                });
                            });
                        });
                    });
                });
            });
        });
    }
};