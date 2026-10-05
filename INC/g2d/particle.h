#pragma once
#include  <vector>
#include  <functional>
#include  <memory>
#include  <cstring>
#include  <cmath>
#include  <algorithm>
#include  <cstdint>
#include  <cstddef>
#include  <cstdlib>
#include "container.h"
#include "sprite.h"
#include "spriteloader.h"
#include "gfx.h"
#include "particlepresets.h"
#include "sconts.h"
#include "utilfuncs.h"

_G2D_NAMESPACE_BEGIN_

// ============================================================================
//  FAST MATH
// ============================================================================
namespace FastMath
{
    // Fast inverse square root ANY PERF WIN ??? doubted
    static inline float invSqrt(float x)
    {
        float xhalf = 0.5f * x;
        int32_t i;
        std::memcpy(&i, &x, sizeof(i));       // float -> int32_t
        i = 0x5f3759df - (i >> 1);
        std::memcpy(&x, &i, sizeof(x));       // int32_t -> float
        x *= (1.5f - xhalf * x * x);          
        return x;
    }

    static inline float sqrtFast(float x)
    {
        return x * invSqrt(x);
    }

    static inline float normalize2(float x, float y, float& outX, float& outY)
    {
        float lenSq = x * x + y * y;
        if (lenSq < 1e-12f)
        {
            outX = outY = 0.0f;
            return 0.0f;
        }
        float inv = invSqrt(lenSq);
        outX = x * inv;
        outY = y * inv;
        return lenSq * inv;
    }
}

// ============================================================================
//  PARTICLE
// ============================================================================
struct Particle
{
    float x = 0.0f, y = 0.0f;
    float startX = 0.0f, startY = 0.0f;
    float vx = 0.0f, vy = 0.0f;
    float radialAccel = 0.0f;
    float tangentialAccel = 0.0f;
    float age = 0.0f;
    float simAge = 0.0f;
    float lifetime = 0.0f;
    float startScale = 1.0f;
    float endScale = 1.0f;
    float currentScale = 1.0f;
    float rotation = 0.0f;
    float spinSpeed = 0.0f;
    float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool isAlive = false;

    float worldX = 0.0f;
    float worldY = 0.0f;

    float wavePhaseOffset = 0.0f;
    float flickerSpeed = 0.0f;

    uint16_t spriteIdx = 0;

    float flipPhase = 0.0f;
    float flipSpeed = 0.0f;

    float depthScaleK = 1.0f;
    float depthSpeedK = 1.0f;
    float depthAlphaK = 1.0f;

    float jitterMul[3] = { 1.0f, 1.0f, 1.0f };
};

// ============================================================================
//  CParticleSystem
// ============================================================================
template <size_t capacity = 1000>
class CParticleSystem : public CContainer
{
private:
    std::optional<Point> _bounds;
    StaticVector<Particle, capacity> _particles;
    ParticleSettings _settings;
    std::vector<CSpritePtr> _sprites;
    float _emitAccumulator = 0.0f;
    bool _isEmitting = true;
    float _lastEmitterX = 0.0f;
    float _lastEmitterY = 0.0f;
    bool _isFirstUpdate = true;
    bool _useMagnet = false;
    float _magnetX = 0.0f;
    float _magnetY = 0.0f;
    float _magnetForce = 0.0f;
    bool _useRepeller = false;
    float _repelX = 0.0f;
    float _repelY = 0.0f;
    float _repelForce = 0.0f;
    float _repelRadius = 0.0f;
    float _fadeTimer = -1.0f;
    float _fadeDuration = 1.0f;
    float _worldEmitterX = 0.0f;
    float _worldEmitterY = 0.0f;
    float _baseConeHalfWidth = 15.0f;
    float _simTime = 0.0f;
    std::function<void(Particle&, float)> _customModifier = {};
    SimpleCallback _cbOnBurstComplete = {};
    bool _bIsCallbackCalled = false;
    std::function<void(float, float, bool)> _onParticleDeath = {};

    std::vector<ParticleQuadData> _batchQuads;


    std::size_t particleLimit() const noexcept
    {
        const std::size_t hardCapacity = _particles.capacity();
        if (_settings.maxParticles <= 0)
            return hardCapacity;
        const std::size_t settingsLimit =
            static_cast<std::size_t>(_settings.maxParticles);
        return settingsLimit < hardCapacity ? settingsLimit : hardCapacity;
    }
    bool hasFreeSlot() const noexcept
    {
        return _particles.size() < particleLimit();
    }
    void removeParticleAt(std::size_t index)
    {
        const std::size_t last = _particles.size() - 1;
        if (index != last)
            _particles[index] = _particles[last];
        _particles.pop_back();
    }
    void prepareSprite(CSpritePtr pSpr)
    {
        if (pSpr)
        {
            pSpr->setBlendMode(_settings.blendMode);
            pSpr->setPivotCentered();
        }
    }

    void initParticle(Particle& p, float moveAngle)
    {
        p = Particle();
        p.isAlive  = true;
        p.age      = 0.0f;
        p.simAge   = 0.0f;
        p.lifetime = randomRange(_settings.lifetimeMin, _settings.lifetimeMax);

        if (_settings.spawnAreaCx > 0.0f || _settings.spawnAreaCy > 0.0f)
        {
            const float aw = std::max(0.0f, _settings.spawnAreaCx);
            const float ah = std::max(0.0f, _settings.spawnAreaCy);
            p.x = randomRange(-aw * 0.5f, aw * 0.5f);
            p.y = randomRange(-ah * 0.5f, ah * 0.5f);
        }
        else
        {
            float spawnAngle = randomRange(0.0f, twoPi());
            float radius     = randomRange(0.0f, _settings.spawnRadius);
            p.x = radius * cosf(spawnAngle);
            p.y = radius * sinf(spawnAngle);
        }
        p.startX = p.x;
        p.startY = p.y;

        float speed = randomRange(_settings.speedMin, _settings.speedMax);
        p.vx = speed * cosf(moveAngle);
        p.vy = speed * sinf(moveAngle);

        p.radialAccel     = _settings.radialAccel;
        p.tangentialAccel = _settings.tangentialAccel;
        p.startScale   = randomRange(_settings.startScaleMin, _settings.startScaleMax);
        p.endScale     = randomRange(_settings.endScaleMin,   _settings.endScaleMax);
        p.currentScale = p.startScale;
        p.rotation  = degToRad(randomRange(_settings.startRotationMin, _settings.startRotationMax));
        p.spinSpeed = degToRad(randomRange(_settings.spinSpeedMin,     _settings.spinSpeedMax));
        p.wavePhaseOffset = randomRange(0.0f, 100.0f);

        if (_settings.flickerSpeedMax > 0.0f)
        {
            float mn = std::max(0.0f, _settings.flickerSpeedMin);
            float mx = std::max(mn, _settings.flickerSpeedMax);
            p.flickerSpeed = randomRange(mn, mx);
        }
        else
        {
            p.flickerSpeed = _settings.flickerSpeed;
        }

        p.spriteIdx = _sprites.empty()
            ? 0
            : static_cast<uint16_t>(fastRandU32() % _sprites.size());

        p.flipSpeed = randomRange(_settings.flutterSpeedMin, _settings.flutterSpeedMax);
        p.flipPhase = randomRange(0.0f, twoPi());

        if (_settings.useDepth)
        {
            float d  = randomRange(_settings.depthMin, 1.0f);
            float tn = (d - _settings.depthMin) / std::max(1e-4f, 1.0f - _settings.depthMin);
            tn = clampValue(tn, 0.0f, 1.0f);
            p.depthScaleK = _settings.depthScaleMin + (1.0f - _settings.depthScaleMin) * tn;
            p.depthSpeedK = _settings.depthSpeedMin + (1.0f - _settings.depthSpeedMin) * tn;
            p.depthAlphaK = _settings.depthAlphaMin + (1.0f - _settings.depthAlphaMin) * tn;
        }
        else
        {
            p.depthScaleK = 1.0f;
            p.depthSpeedK = 1.0f;
            p.depthAlphaK = 1.0f;
        }

        if (_settings.colorJitter > 0.0f)
        {
            for (int c = 0; c < 3; ++c)
            {
                p.jitterMul[c] = 1.0f + randomRange(-_settings.colorJitter, _settings.colorJitter);
                p.jitterMul[c] = std::max(0.0f, p.jitterMul[c]);
            }
        }
        else if (_settings.colorJitter < 0.0f)
        {
            const float amount = -_settings.colorJitter;
            const float v = randomRange(-1.0f, 1.0f);
            if (v < 0.0f)
            {
                const float a = -v * amount;
                p.jitterMul[0] = 1.0f - 0.30f * a;
                p.jitterMul[1] = 1.0f - 0.08f * a;
                p.jitterMul[2] = 1.0f + 0.25f * a;
            }
            else
            {
                const float a = v * amount;
                p.jitterMul[0] = 1.0f + 0.14f * a;
                p.jitterMul[1] = 1.0f + 0.03f * a;
                p.jitterMul[2] = 1.0f - 0.32f * a;
            }
            for (int c = 0; c < 3; ++c)
                p.jitterMul[c] = std::max(0.0f, p.jitterMul[c]);
        }
        else
        {
            p.jitterMul[0] = 1.0f;
            p.jitterMul[1] = 1.0f;
            p.jitterMul[2] = 1.0f;
        }

        memcpy(p.color, _settings.startColor, sizeof(p.color));
    }

public:

    CParticleSystem(const ParticleSettings& settings, const char* lpszPathToSprite)
        : _settings(settings)
    {
        _baseConeHalfWidth = fabsf(_settings.angleMax - _settings.angleMin) * 0.5f;
        _batchQuads.reserve(capacity);
        addSprite(lpszPathToSprite);
    }

    CParticleSystem(const ParticleSettings& settings, const std::vector<CSpritePtr>& sprites)
        : _settings(settings)
    {
        _baseConeHalfWidth = fabsf(_settings.angleMax - _settings.angleMin) * 0.5f;
        _batchQuads.reserve(capacity);
        for (const auto& s : sprites)
            addSprite(s);
    }

    CParticleSystem(const ParticleSettings& settings, const std::vector<const char*>& paths)
        : _settings(settings)
    {
        _baseConeHalfWidth = fabsf(_settings.angleMax - _settings.angleMin) * 0.5f;
        _batchQuads.reserve(capacity);
        for (const char* p : paths)
            addSprite(p);
    }

    CParticleSystem(const ParticleSettings& settings, const std::span<const char*>& paths)
        : _settings(settings)
    {
        _baseConeHalfWidth = fabsf(_settings.angleMax - _settings.angleMin) * 0.5f;
        _batchQuads.reserve(capacity);
        for (const char* p : paths)
            addSprite(p);
    }


    void setBounds(std::optional<Point> bounds)
    {
        _bounds = bounds;
    }

    virtual bool getNotTransBounds(Rect* p)override
    {
        bool bRes = _bounds.has_value();
        if (bRes)
            p->set(0, 0, _bounds->x, _bounds->y);
        return bRes;
    }


    void prewarm(float seconds, float stepDt = 1.0f / 60.0f)
    {
        if (seconds <= 0.0f) return;
        auto savedOnDeath = _onParticleDeath;
        auto savedBurstCb = _cbOnBurstComplete;
        _onParticleDeath  = nullptr;
        _cbOnBurstComplete = nullptr;

        float elapsed = 0.0f;
        while (elapsed < seconds)
        {
            float dt = std::min(stepDt, seconds - elapsed);
            update(dt);
            elapsed += dt;
        }
        _onParticleDeath  = savedOnDeath;
        _cbOnBurstComplete = savedBurstCb;
    }

    void addSprite(CSpritePtr pSpr)
    {
        prepareSprite(pSpr);
        _sprites.push_back(pSpr);
    }
    void addSprite(const char* lpszPathToSprite)
    {
        addSprite(SpriteLoader::getInstance()->getSprite(lpszPathToSprite));
    }
    ParticleSettings& getSettings() { return _settings; }

    void setOnParticleDeath(std::function<void(float, float, bool)> cb)
    {
        _onParticleDeath = cb;
    }

    void rotateToTarget(float fTargetX, float fTargetY)
    {
        float emitterX = getTransX();
        float emitterY = getTransY();
        float dx = fTargetX - emitterX;
        float dy = fTargetY - emitterY;
        float angleRadians  = atan2f(-dy, dx);
        float angleDegrees  = angleRadians * (180.0f / 3.14159265358979323846f);
        if (angleDegrees < 0.0f) angleDegrees += 360.0f;
        _settings.angleMin = angleDegrees - _baseConeHalfWidth;
        _settings.angleMax = angleDegrees + _baseConeHalfWidth;
    }

    void spawnParticle()
    {
        if (!hasFreeSlot()) return;
        Particle p;
        float moveAngle = degToRad(randomRange(_settings.angleMin, _settings.angleMax));
        initParticle(p, moveAngle);
        _particles.push_back(p);
    }

    void burst(int count, SimpleCallback cbOnBurstComplete = {})
    {
        if (count <= 0) return;
        _cbOnBurstComplete = cbOnBurstComplete;
        _bIsCallbackCalled = false;

        const std::size_t limit = particleLimit();
        if (_particles.size() >= limit) return;

        const std::size_t remaining = limit - _particles.size();
        const std::size_t toSpawn   = std::min<std::size_t>(
            static_cast<std::size_t>(count), remaining);
        if (toSpawn == 0) return;

        const float slice = twoPi() / static_cast<float>(toSpawn);
        for (std::size_t s = 0; s < toSpawn; ++s)
        {
            Particle p;
            float moveAngle = static_cast<float>(s) * slice + randomRange(-0.05f, 0.05f);
            initParticle(p, moveAngle);
            _particles.push_back(p);
        }
    }

    void burstAtWorld(float fWorldX, float fWorldY, int count)
    {
        burstAtLocal(fWorldX - getTransX(), fWorldY - getTransY(), count);
    }

    void burstAtLocal(float fLocalX, float fLocalY, int count)
    {
        if (count <= 0) return;
        const std::size_t limit = particleLimit();
        if (_particles.size() >= limit) return;
        const std::size_t remaining = limit - _particles.size();
        const std::size_t toSpawn   = std::min<std::size_t>(
            static_cast<std::size_t>(count), remaining);

        for (std::size_t n = 0; n < toSpawn; ++n)
        {
            Particle p;
            float moveAngle = randomRange(0.0f, twoPi());
            initParticle(p, moveAngle);
            p.x = fLocalX;  p.y = fLocalY;
            p.startX = fLocalX;  p.startY = fLocalY;
            _particles.push_back(p);
        }
    }

    void attractTo(float targetX, float targetY, float force)
    {
        _useMagnet = true;
        _magnetX = targetX;  _magnetY = targetY;
        _magnetForce = force;
    }
    void disableAttractor() { _useMagnet = false; }

    void repelFrom(float sourceX, float sourceY, float force, float radius)
    {
        _useRepeller = true;
        _repelX = sourceX;  _repelY = sourceY;
        _repelForce = force;  _repelRadius = radius;
    }
    void disableRepeller() { _useRepeller = false; }

    void stopWithFade(float fadeDuration)
    {
        setEmitting(false);
        _fadeTimer = fadeDuration;
        _fadeDuration = fadeDuration;
    }
    void setEmitting(bool emit) { _isEmitting = emit; }

    void setCustomModifier(std::function<void(Particle&, float)> fn)
    {
        _customModifier = fn;
    }

    virtual void applyTransform(CMatrixStack* pMS, bool bForce = false) override
    {
        CContainer::applyTransform(pMS, true);
        _worldEmitterX = getTransX();
        _worldEmitterY = getTransY();
    }

    virtual void update(float dt) override
    {
        CContainer::update(dt);
        _simTime += dt;

        float gust = 0.0f;
        if (_settings.gustFreq > 0.0f && _settings.gustAmp != 0.0f)
        {
            gust =
                sinf(_simTime * _settings.gustFreq) * _settings.gustAmp +
                sinf(_simTime * _settings.gustFreq * 0.37f + 1.7f) * _settings.gustAmp * 0.5f;
        }

        if (_isFirstUpdate)
        {
            _lastEmitterX = getX();
            _lastEmitterY = getY();
            _isFirstUpdate = false;
        }

        if (_isEmitting && _settings.spawnRate > 0.0f)
        {
            _emitAccumulator += dt;
            float interval = 1.0f / _settings.spawnRate;
            while (_emitAccumulator >= interval)
            {
                spawnParticle();
                _emitAccumulator -= interval;
            }
        }

        const float repelRadiusSq = _repelRadius * _repelRadius;

        bool bHaveAlive = false;
        for (std::size_t i = 0; i < _particles.size(); )
        {
            Particle& p = _particles[i];
            if (!p.isAlive) { removeParticleAt(i); continue; }

            bHaveAlive = true;
            bool removeParticle = false;
            bool bFloorDeath    = false;

            float pdt = dt * p.depthSpeedK;
            p.age    += dt;
            p.simAge += pdt;

            if (p.simAge >= p.lifetime)
            {
                p.isAlive = false;
                removeParticle = true;
            }
            else
            {
                float t    = p.simAge / p.lifetime;
                float curX = p.x;
                float curY = p.y;

                float radialX = 0.0f, radialY = 0.0f;
                {
                    float dSq = curX * curX + curY * curY;
                    if (dSq > 1e-8f)
                    {
                        float inv = FastMath::invSqrt(dSq);
                        radialX = curX * inv;
                        radialY = curY * inv;
                    }
                }
                float tangentialX = -radialY;
                float tangentialY =  radialX;

                float turbX = 0.0f, turbY = 0.0f;
                if (_settings.turbulence > 0.0f)
                {
                    turbX = randomRange(-_settings.turbulence, _settings.turbulence);
                    turbY = randomRange(-_settings.turbulence, _settings.turbulence);
                }

                p.vx += (_settings.gravityX + _settings.windX + gust +
                         radialX * p.radialAccel +
                         tangentialX * p.tangentialAccel + turbX) * pdt;
                p.vy += (_settings.gravityY +
                         radialY * p.radialAccel +
                         tangentialY * p.tangentialAccel + turbY) * pdt;

                float face = 1.0f;
                if (p.flipSpeed != 0.0f)
                {
                    p.flipPhase += p.flipSpeed * pdt;
                    face = fabsf(cosf(p.flipPhase));
                    if (_settings.flutterAffectsFall)
                        p.vy += _settings.flutterSlip * (1.0f - face) * pdt;
                }

                if (_settings.drag > 0.0f)
                {
                    float dragK = _settings.drag *
                        (_settings.flutterAffectsFall ? (0.35f + 0.65f * face) : 1.0f);
                    p.vx -= p.vx * dragK * pdt;
                    p.vy -= p.vy * dragK * pdt;
                }

                if (_useMagnet)
                {
                    float dirX = _magnetX - p.x;
                    float dirY = _magnetY - p.y;
                    float dSq  = dirX * dirX + dirY * dirY;
                    if (dSq > 1.0f)
                    {
                        float inv = FastMath::invSqrt(dSq);
                        p.vx += dirX * inv * _magnetForce * pdt;
                        p.vy += dirY * inv * _magnetForce * pdt;
                    }
                }

                if (_useRepeller)
                {
                    float dirX = p.x - _repelX;
                    float dirY = p.y - _repelY;
                    float dSq  = dirX * dirX + dirY * dirY;
                    if (dSq > 0.0f && dSq < repelRadiusSq)
                    {
                        float inv = FastMath::invSqrt(dSq);
                        float d   = dSq * inv;   
                        float forceFactor = (1.0f - d / _repelRadius) * _repelForce;
                        p.vx += dirX * inv * forceFactor * pdt;
                        p.vy += dirY * inv * forceFactor * pdt;
                    }
                }

                p.x += p.vx * pdt;
                p.y += p.vy * pdt;

                if (p.y >= _settings.floorY)
                {
                    p.y = _settings.floorY;
                    p.isAlive = false;
                    removeParticle = true;
                    bFloorDeath = true;
                }

                if (!removeParticle)
                {
                    if (_settings.waveAmplitude > 0.0f)
                    {
                        float sineWave =
                            sinf(p.simAge * _settings.waveFrequency + p.wavePhaseOffset) *
                            _settings.waveAmplitude * pdt;

                        float vLenSq = p.vx * p.vx + p.vy * p.vy;
                        if (vLenSq > 1e-8f)
                        {
                            float invLen = FastMath::invSqrt(vLenSq);
                            p.x += (-p.vy * invLen) * sineWave;
                            p.y += ( p.vx * invLen) * sineWave;
                        }
                    }

                    p.currentScale = p.startScale + (p.endScale - p.startScale) * t;
                    p.rotation += p.spinSpeed * pdt;

                    if (_settings.use3PhaseColor)
                    {
                        if (t < 0.5f)
                        {
                            float f = t * 2.0f;
                            for (int c = 0; c < 4; ++c)
                                p.color[c] = _settings.startColor[c] +
                                    (_settings.middleColor[c] - _settings.startColor[c]) * f;
                        }
                        else
                        {
                            float f = (t - 0.5f) * 2.0f;
                            for (int c = 0; c < 4; ++c)
                                p.color[c] = _settings.middleColor[c] +
                                    (_settings.endColor[c] - _settings.middleColor[c]) * f;
                        }
                    }
                    else
                    {
                        for (int c = 0; c < 4; ++c)
                            p.color[c] = _settings.startColor[c] +
                                (_settings.endColor[c] - _settings.startColor[c]) * t;
                    }

                    if (_settings.fadeInTime > 0.0f)
                        p.color[3] *= clampValue(p.simAge / _settings.fadeInTime, 0.0f, 1.0f);
                    if (_settings.fadeOutTime > 0.0f)
                        p.color[3] *= clampValue((p.lifetime - p.simAge) / _settings.fadeOutTime, 0.0f, 1.0f);

                    if (_settings.useFlicker)
                    {
                        float fs = p.flickerSpeed;
                        if (fs == 0.0f) fs = _settings.flickerSpeed;
                        float phase = p.simAge * fs + p.wavePhaseOffset;
                        float wave  = (sinf(phase) + 1.0f) * 0.5f;
                        wave = clampValue(wave, 0.0f, 1.0f);
                        float fMin = std::min(_settings.flickerMinAlpha, _settings.flickerMaxAlpha);
                        float fMax = std::max(_settings.flickerMinAlpha, _settings.flickerMaxAlpha);
                        p.color[3] = clampValue(p.color[3] * (fMin + (fMax - fMin) * wave), 0.0f, 1.0f);
                    }

                    if (_fadeTimer > 0.0f && _fadeDuration > 0.0f)
                        p.color[3] = clampValue(_fadeTimer / _fadeDuration, 0.0f, 1.0f);

                    if (_customModifier != nullptr)
                    {
                        _customModifier(p, dt);
                        if (!p.isAlive) removeParticle = true;
                    }
                }
            }

            if (removeParticle)
            {
                if (_onParticleDeath)
                    _onParticleDeath(getTransX() + p.x, getTransY() + p.y, bFloorDeath);
                removeParticleAt(i);
                continue;
            }
            ++i;
        }

        if (_fadeTimer > 0.0f)
        {
            _fadeTimer -= dt;
            if (_fadeTimer <= 0.0f)
            {
                removeFromParent();
                return;
            }
        }

        _lastEmitterX = getX();
        _lastEmitterY = getY();

        if (!bHaveAlive && !_bIsCallbackCalled && _cbOnBurstComplete)
        {
            _bIsCallbackCalled = true;
            _cbOnBurstComplete();
        }
    }

    virtual void renderSelf(float dt, float* rgba) override
    {
        if (_sprites.empty() || _particles.size() == 0)
            return;

        CTexturePtr pTex = _sprites[0]->getTexture();
        assert(pTex->isUploaded());
        if (!pTex || !pTex->isUploaded())
            return;
        _batchQuads.clear();

        float isNormalBlend =
            (_settings.blendMode == eSpriteBlendMode::NORMAL) ? 1.0f : 0.0f;

        for (std::size_t i = 0; i < _particles.size(); ++i)
        {
            Particle& p = _particles[i];
            if (!p.isAlive) continue;

            CSpritePtr& pSpr = _sprites[p.spriteIdx % _sprites.size()];
            if (!pSpr || !pSpr->_bIsLoaded) continue;

            float scaleK   = p.currentScale * p.depthScaleK;
            float flutterX = (p.flipSpeed != 0.0f) ? cosf(p.flipPhase) : 1.0f;

            float scaleX, scaleY, angle;

            float speedSq = p.vx * p.vx + p.vy * p.vy;
            if (_settings.velocityStretch > 0.0f && speedSq > 25.0f)
            {
                float speed = speedSq * FastMath::invSqrt(speedSq);
                angle   = atan2f(p.vy, p.vx);
                scaleX  = scaleK * flutterX * (1.0f + speed * _settings.velocityStretch);
                scaleY  = scaleK;
            }
            else
            {
                angle  = p.rotation;
                scaleX = scaleK * flutterX;
                scaleY = scaleK;
            }

            float cosA = cosf(angle);
            float sinA = sinf(angle);

            const float* sv = pSpr->_verts;
            const float* su = pSpr->_uvs;

            ParticleQuadData quad;

            for (int c = 0; c < 4; ++c)
            {
                float cx = sv[c * 2];
                float cy = sv[c * 2 + 1];

                float sx = cx * scaleX;
                float sy = cy * scaleY;

                quad.verts[c * 2]     = sx * cosA - sy * sinA + p.x;
                quad.verts[c * 2 + 1] = sx * sinA + sy * cosA + p.y;

                quad.uvs[c * 2]     = su[c * 2];
                quad.uvs[c * 2 + 1] = su[c * 2 + 1];
            }

            quad.rgba[0] = p.color[0] * p.jitterMul[0] * rgba[0];
            quad.rgba[1] = p.color[1] * p.jitterMul[1] * rgba[1];
            quad.rgba[2] = p.color[2] * p.jitterMul[2] * rgba[2];
            quad.rgba[3] = clampValue(p.color[3] * rgba[3] * p.depthAlphaK, 0.0f, 1.0f);

            _batchQuads.push_back(quad);
        }

        if (!_batchQuads.empty())
        {
            CGfx::getInstance()->batchParticles(
                pTex,
                _batchQuads.data(),
                static_cast<int>(_batchQuads.size()),
                isNormalBlend,
                _bSkipLight);
        }
    }
};

typedef std::shared_ptr<CParticleSystem<>> CParticleSystemPtr;

_G2D_NAMESPACE_END_