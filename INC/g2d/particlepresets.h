#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

enum class eParticlePreset
{
    FIRE,
    SMOKE,
    AMBIENT_DUST,
    RAIN,
    SNOW,
    FALLING_LEAVES,
    BUBBLES,
    SPARKS,
    BLOOD_SPLATTER,
    DEBRIS,

    PLASMA_TRAIL,
    HEALING,
    POISON,
    PIXIE_DUST,
    SHADOW_VOID,
    STEAM,
    ELECTRIC_SPARKS,
    ACID_DRIPS,
    FIREWORKS,
    CONFETTI,

    UI_BUTTON_GLOW,
    NEBULA,
    HYPERSPACE,
    ADVANCED_SNOW,
    ADVANCED_SPARKS,
    ROCKET_TRAIL,
    PLASMA_BALL,
    FLAMETHROWER,
    STARLIGHT_FOUNTAIN,
    ETHEREAL_WISP,
    LASER_SPARKS,
    RAILGUN_TRAIL,

    CORE_MELTDOWN,
    SANDSTORM,
    SAKURA,
    GRENADE_EXPLOSION,
    MECH_EXPLOSION,
    COSMIC_NOVA,
    HOLY_BURST,
    CRITICAL_BLOOD,
    PROP_SMASH,
    ICE_NOVA,
    TOXIC_SPORE,
    FIREFLIES,
    DANDELION_FLUFF,
    SUNBEAM_DUST,
    LAKE_MIST,
    CUP_STEAM,
    AETHER_WISPS,
    GLOWING_SPORES,
    PLANKTON,
    DISTANT_EMBERS,
    BOKEH_AMBIENT,
    RAIN_SPLASH,
    STARFIELD,

    // --- пресеты для накладывания на кнопки ---
    BUTTON_GOLD_SPARKLE,
    BUTTON_EMBER_RISE,
    BUTTON_HOLY_AURA,
    BUTTON_ARCANE_SWIRL,
    BUTTON_HEART_PUFF,
    BUTTON_FROST_MIST,
    BUTTON_SHADOW_WISP,
    BUTTON_STAR_TRAIL,
    BUTTON_SAKURA_DRIFT,
    BUTTON_VIOLET_FLAME,
    BUTTON_BLOOD_FOG,
};

struct ParticleSettings
{
    float spawnRate = 50.0f;
    int maxParticles = 500;
    float lifetimeMin = 1.0f;
    float lifetimeMax = 2.0f;
    float spawnRadius = 0.0f;

    float speedMin = 100.0f;
    float speedMax = 150.0f;
    float angleMin = 0.0f;
    float angleMax = 360.0f;
    float gravityX = 0.0f;
    float gravityY = 0.0f;
    float radialAccel = 0.0f;
    float tangentialAccel = 0.0f;

    float startScaleMin = 1.0f;
    float startScaleMax = 1.0f;
    float endScaleMin = 1.0f;
    float endScaleMax = 1.0f;
    float startRotationMin = 0.0f;
    float startRotationMax = 0.0f;
    float spinSpeedMin = 0.0f;
    float spinSpeedMax = 0.0f;
    eSpriteBlendMode blendMode = eSpriteBlendMode::NORMAL;

    bool  useFlicker = false;
    float flickerSpeed = 45.0f;
    float flickerMinAlpha = 0.1f;

    float startColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float endColor[4]   = { 1.0f, 1.0f, 1.0f, 0.0f };

    bool  use3PhaseColor = false;
    float middleColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    float waveFrequency = 0.0f;
    float waveAmplitude = 0.0f;

    float velocityStretch = 0.0f;

    float turbulence = 0.0f;

    float flutterSpeedMin = 0.0f;
    float flutterSpeedMax = 0.0f;
    bool  flutterAffectsFall = true;
    float flutterSlip = 0.0f;

    float drag = 0.0f;
    float windX = 0.0f;
    float gustAmp = 0.0f;
    float gustFreq = 0.0f;

    float spawnAreaCx = 0.0f;
    float spawnAreaCy = 0.0f;

    float floorY = 1e9f;

    float colorJitter = 0.0f;

    bool  useDepth      = false;
    float depthMin      = 0.35f;
    float depthScaleMin = 0.40f;
    float depthSpeedMin = 0.35f;
    float depthAlphaMin = 0.50f;
    float fadeOutTime = 0;
    float fadeInTime = 0;

    float flickerSpeedMin = 0.0f;
    float flickerSpeedMax = 0.0f;
    float flickerMaxAlpha = 1.0f;

    float colorTempJitter = 0.0f;
};

namespace ParticlePresets
{
    static ParticleSettings& getStarfieldPreset()
    {
        static ParticleSettings s;

        s.spawnRate = 18.0f;
        s.maxParticles = 120;

        s.lifetimeMin = 90.0f;
        s.lifetimeMax = 180.0f;

        s.spawnRadius = 250.0f;

        s.speedMin = 0.0f;
        s.speedMax = 0.0f;

        s.angleMin = 0.0f;
        s.angleMax = 360.0f;

        s.gravityX = 0.0f;
        s.gravityY = 0.0f;
        s.turbulence = 0.0f;

        s.startScaleMin = 0.06f;
        s.startScaleMax = 0.55f;
        s.endScaleMin   = 0.06f;
        s.endScaleMax   = 0.55f;

        s.useFlicker = true;

        s.flickerSpeed = 0.55f;

        s.flickerSpeedMin = 0.9f;
        s.flickerSpeedMax = 3.2f;

        s.flickerMinAlpha = 0.15f;
        s.flickerMaxAlpha = .7f;

        s.colorJitter = 0.0f;
        s.colorTempJitter = 0.45f;

        s.startColor[0] = 1.00f;
        s.startColor[1] = 1.00f;
        s.startColor[2] = 1.00f;
        s.startColor[3] = .5f;

        s.endColor[0] = 1.00f;
        s.endColor[1] = 1.00f;
        s.endColor[2] = 1.00f;
        s.endColor[3] = .8f;

        s.fadeInTime = 1.5f;
        s.fadeOutTime = 3.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;

        return s;
    }
    static ParticleSettings& getFirePreset()
    {
        static ParticleSettings s;
        s.spawnRate = 35.0f;
        s.maxParticles = 80;
        s.lifetimeMin = 0.6f;
        s.lifetimeMax = 1.0f;
        s.spawnRadius = 8.0f;

        s.speedMin = 120.0f;
        s.speedMax = 200.0f;

        s.angleMin = 260.0f;
        s.angleMax = 280.0f;

        s.gravityX = 0.0f;
        s.gravityY = -15.0f;
        s.radialAccel = 0.0f;
        s.tangentialAccel = 0.0f;
        s.turbulence = 15.0f;

        s.startScaleMin = 0.6f;
        s.startScaleMax = 0.9f;
        s.endScaleMin = 0.3f;
        s.endScaleMax = 0.6f;

        s.velocityStretch = 0.0f;
        s.use3PhaseColor = true;

        s.startColor[0]  = 1.0f;  s.startColor[1]  = 0.95f; s.startColor[2]  = 0.8f;  s.startColor[3]  = 1.0f;
        s.middleColor[0] = 1.0f;  s.middleColor[1] = 0.35f; s.middleColor[2] = 0.0f;  s.middleColor[3] = 0.8f;
        s.endColor[0]    = 0.2f;  s.endColor[1]    = 0.0f;  s.endColor[2]    = 0.0f;  s.endColor[3]    = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getSmokePreset()
    {
        static ParticleSettings s;
        s.spawnRate = 25.0f;
        s.maxParticles = 60;
        s.lifetimeMin = 1.5f;
        s.lifetimeMax = 2.5f;
        s.spawnRadius = 12.0f;

        s.speedMin = 30.0f;
        s.speedMax = 60.0f;

        s.angleMin = 260.0f;
        s.angleMax = 280.0f;

        s.gravityX = 15.0f;
        s.gravityY = -10.0f;
        s.radialAccel = 0.0f;
        s.tangentialAccel = 0.0f;
        s.turbulence = 0.0f;

        s.startScaleMin = 0.4f;
        s.startScaleMax = 0.7f;
        s.endScaleMin = 2.0f;
        s.endScaleMax = 3.5f;

        s.velocityStretch = 0.0f;
        s.use3PhaseColor = false;

        s.startColor[0] = 0.35f; s.startColor[1] = 0.35f; s.startColor[2] = 0.35f; s.startColor[3] = 0.30f;
        s.endColor[0]   = 0.40f; s.endColor[1]   = 0.40f; s.endColor[2]   = 0.40f; s.endColor[3]   = 0.0f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getAmbientDustPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 15.0f;
        s.maxParticles = 100;
        s.lifetimeMin = 4.0f;
        s.lifetimeMax = 7.0f;
        s.spawnRadius = 250.0f;

        s.speedMin = 5.0f;
        s.speedMax = 15.0f;
        s.angleMin = 0.0f;
        s.angleMax = 360.0f;

        s.startScaleMin = 0.1f;
        s.startScaleMax = 0.3f;
        s.endScaleMin = 0.1f;
        s.endScaleMax = 0.3f;

        s.spinSpeedMin = -5.0f;
        s.spinSpeedMax = 5.0f;

        s.use3PhaseColor = true;

        s.startColor[0]  = 0.9f; s.startColor[1]  = 0.85f; s.startColor[2]  = 0.7f; s.startColor[3]  = 0.0f;
        s.middleColor[0] = 0.9f; s.middleColor[1] = 0.85f; s.middleColor[2] = 0.7f; s.middleColor[3] = 0.4f;
        s.endColor[0]    = 0.9f; s.endColor[1]    = 0.85f; s.endColor[2]    = 0.7f; s.endColor[3]    = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

     static ParticleSettings& getRainSplashPreset()
     {
         static ParticleSettings s;
         s.spawnRate = 0.0f; s.maxParticles = 150;
         s.lifetimeMin = 0.25f; s.lifetimeMax = 0.40f;
         s.speedMin = 30.0f; s.speedMax = 90.0f;
         s.angleMin = 200.0f; s.angleMax = 340.0f;
         s.gravityY = 300.0f;
         s.startScaleMin = 0.05f; s.startScaleMax = 0.10f;
         s.endScaleMin = 0.02f; s.endScaleMax = 0.05f;
         s.velocityStretch = 0.004f;
         s.startColor[0] = 0.8f; s.startColor[1] = 0.85f; s.startColor[2] = 0.9f; s.startColor[3] = 0.9f;
         s.endColor[0]   = 0.8f; s.endColor[1]   = 0.85f; s.endColor[2]   = 0.9f; s.endColor[3]   = 0.0f;
         s.blendMode = eSpriteBlendMode::NORMAL;
         return s;
     }

    static ParticleSettings& getRainPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 180.0f;
        s.maxParticles = 400;
        s.lifetimeMin = 1.0f;
        s.lifetimeMax = 1.6f;
        s.spawnRadius = 450.0f;

        s.speedMin = 750.0f;
        s.speedMax = 1000.0f;

        s.angleMin = 83.0f;
        s.angleMax = 87.0f;

        s.gravityX = 90.0f;
        s.gravityY = 300.0f;
        s.radialAccel = 0.0f;
        s.tangentialAccel = 0.0f;
        s.turbulence = 0.0f;

        s.startScaleMin = 0.03f;
        s.startScaleMax = 0.06f;
        s.endScaleMin = 0.03f;
        s.endScaleMax = 0.06f;

        s.velocityStretch = 0.005f;

        s.use3PhaseColor = true;

        s.startColor[0]  = 0.8f; s.startColor[1]  = 0.85f; s.startColor[2]  = 0.9f; s.startColor[3]  = 1.0f;
        s.middleColor[0] = 0.8f; s.middleColor[1] = 0.85f; s.middleColor[2] = 0.9f; s.middleColor[3] = 1.f;
        s.endColor[0]    = 0.8f; s.endColor[1]    = 0.85f; s.endColor[2]    = 0.9f; s.endColor[3]    = 1.0f;

        s.blendMode = eSpriteBlendMode::NORMAL;

         s.spawnAreaCx = 1400.0f; s.spawnAreaCy = 60.0f;
         s.floorY = 700.0f;
         s.colorJitter = 0.15f;
        return s;
    }

    static ParticleSettings& getSnowPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 25.0f; s.maxParticles = 200;
        s.lifetimeMin = 4.0f; s.lifetimeMax = 6.0f;
        s.spawnRadius = 400.0f;
        s.speedMin = 40.0f; s.speedMax = 70.0f;
        s.angleMin = 80.0f; s.angleMax = 100.0f;
        s.gravityY = 10.0f; s.gravityX = 5.0f;
        s.startScaleMin = 0.1f; s.startScaleMax = 0.2f;
        s.endScaleMin = 0.1f; s.endScaleMax = 0.2f;
        s.spinSpeedMin = -30.0f; s.spinSpeedMax = 30.0f;
        s.use3PhaseColor = true;
        s.startColor[0] = 1.0f; s.startColor[1] = 1.0f; s.startColor[2] = 1.0f; s.startColor[3] = 0.0f;
        s.middleColor[0]   = 1.0f; s.middleColor[1]   = 1.0f; s.middleColor[2]   = 1.0f; s.middleColor[3]   = 0.3f;
        s.endColor[0]   = 1.0f; s.endColor[1]   = 1.0f; s.endColor[2]   = 1.0f; s.endColor[3]   = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

static ParticleSettings& getFallingLeavesPreset()
{
    static ParticleSettings s;
    s.spawnRate = 16.0f;
    s.maxParticles = 240;
    s.lifetimeMin = 7.0f;
    s.lifetimeMax = 11.0f;
    s.spawnRadius = 0.0f;
    s.spawnAreaCx = 1400.0f;
    s.spawnAreaCy = 1000.0f;
    s.speedMin = 30.0f;
    s.speedMax = 70.0f;
    s.angleMin = 75.0f;
    s.angleMax = 105.0f;
    s.gravityX = 6.0f;
    s.gravityY = 40.0f;
    s.drag = 0.30f;
    s.turbulence = 8.0f;
    s.waveFrequency = 1.6f;
    s.waveAmplitude = 26.0f;
    s.flutterSpeedMin = 2.5f;
    s.flutterSpeedMax = 5.5f;
    s.flutterSlip     = 40.0f;
    s.startRotationMin = 0.0f;
    s.startRotationMax = 360.0f;
    s.spinSpeedMin = -40.0f;
    s.spinSpeedMax =  40.0f;
    s.startScaleMin = 0.30f;
    s.startScaleMax = 0.65f;
    s.endScaleMin   = 0.30f;
    s.endScaleMax   = 0.65f;
    s.fadeInTime  = 0.3f;
    s.fadeOutTime = 1.0f;
    s.use3PhaseColor = false;
    s.startColor[0] = 1.00f; s.startColor[1] = 1.00f; s.startColor[2] = 1.00f; s.startColor[3] = 1.0f;
    s.endColor[0]   = 0.85f; s.endColor[1] = 0.70f; s.endColor[2] = 0.50f; s.endColor[3] = 0.0f;
    s.colorJitter   = 0.25f;
    s.useDepth      = true;
    s.depthMin      = 0.40f;
    s.depthScaleMin = 0.40f;
    s.depthSpeedMin = 0.40f;
    s.depthAlphaMin = 0.45f;
    s.gustAmp  = 90.0f;
    s.gustFreq = 0.35f;
    s.floorY = 1100.0f;
    s.blendMode = eSpriteBlendMode::NORMAL;
    return s;
}
    static ParticleSettings& getBubblesPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 14.0f;
        s.maxParticles = 35;
        s.lifetimeMin = 2.5f;
        s.lifetimeMax = 4.0f;
        s.spawnRadius = 40.0f;

        s.speedMin = 70.0f;
        s.speedMax = 120.0f;

        s.angleMin = 265.0f;
        s.angleMax = 275.0f;

        s.gravityX = 10.0f;
        s.gravityY = -40.0f;
        s.radialAccel = 0.0f;
        s.tangentialAccel = 0.0f;
        s.turbulence = 0.0f;

        s.waveFrequency = 0.0f;
        s.waveAmplitude = 0.0f;

        s.startScaleMin = 0.15f;
        s.startScaleMax = 0.35f;
        s.endScaleMin = 0.25f;
        s.endScaleMax = 0.50f;

        s.velocityStretch = 0.0f;
        s.use3PhaseColor = true;

        s.startColor[0]  = 0.9f;  s.startColor[1]  = 0.95f; s.startColor[2]  = 1.0f;  s.startColor[3]  = 0.0f;
        s.middleColor[0] = 0.85f; s.middleColor[1] = 0.90f; s.middleColor[2] = 1.0f;  s.middleColor[3] = 0.55f;
        s.endColor[0]    = 0.85f; s.endColor[1]    = 0.90f; s.endColor[2]    = 1.0f;  s.endColor[3]    = 0.0f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getSparksPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 350.0f;
        s.maxParticles = 60;
        s.lifetimeMin = 0.20f;
        s.lifetimeMax = 0.50f;
        s.spawnRadius = 2.0f;

        s.speedMin = 600.0f;
        s.speedMax = 1200.0f;
        s.angleMin = 0.0f;
        s.angleMax = 360.0f;

        s.gravityX = 0.0f;
        s.gravityY = 900.0f;
        s.radialAccel = 0.0f;
        s.tangentialAccel = 0.0f;
        s.turbulence = 0.0f;

        s.startScaleMin = 0.08f;
        s.startScaleMax = 0.16f;
        s.endScaleMin = 0.02f;
        s.endScaleMax = 0.06f;

        s.velocityStretch = 0.0055f;
        s.use3PhaseColor = true;

        s.startColor[0] = 1.0f;  s.startColor[1] = 1.0f;  s.startColor[2] = 1.0f;  s.startColor[3] = 1.0f;
        s.middleColor[0] = 1.0f; s.middleColor[1] = 0.85f; s.middleColor[2] = 0.15f; s.middleColor[3] = 1.0f;
        s.endColor[0] = 0.8f;    s.endColor[1] = 0.15f;   s.endColor[2] = 0.0f;    s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getBloodSplatterPreset()
    {
        static ParticleSettings s;

        // чистый burst-пресет: burst(n) рядом с раной
        s.spawnRate = 0.0f;
        s.maxParticles = 220;

        s.lifetimeMin = 0.55f;
        s.lifetimeMax = 1.25f;
        s.spawnRadius = 3.0f;

        // артериальный фонтан: струя вверх-в стороны, тяжёлая гравитация
        // роняет капли дугой вниз
        s.speedMin = 450.0f;
        s.speedMax = 1150.0f;
        s.angleMin = 205.0f;
        s.angleMax = 335.0f;

        s.gravityX = 0.0f;
        s.gravityY = 1500.0f;

        s.radialAccel = 60.0f;
        s.tangentialAccel = 0.0f;
        s.turbulence = 35.0f;
        s.drag = 0.15f;

        s.startScaleMin = 0.07f;
        s.startScaleMax = 0.18f;
        s.endScaleMin   = 0.04f;
        s.endScaleMax   = 0.10f;

        // капли тянутся в струи на скорости
        s.velocityStretch = 0.006f;

        // свежая артериалка -> тёмная -> почти чёрный осадок
        s.use3PhaseColor = true;
        s.startColor[0]  = 1.00f; s.startColor[1]  = 0.06f; s.startColor[2]  = 0.04f; s.startColor[3]  = 1.0f;
        s.middleColor[0] = 0.55f; s.middleColor[1]  = 0.00f; s.middleColor[2]  = 0.02f; s.middleColor[3]  = 0.9f;
        s.endColor[0]    = 0.18f; s.endColor[1]    = 0.00f; s.endColor[2]    = 0.01f; s.endColor[3]    = 0.0f;

        s.colorJitter = 0.12f;
        s.fadeOutTime = 0.18f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getDebrisPreset()
{
    static ParticleSettings s;

    s.spawnRate = 0.0f;
    s.maxParticles = 120;
    s.lifetimeMin = 0.9f;
    s.lifetimeMax = 1.5f;
    s.spawnRadius = 4.0f;

    s.speedMin = 500.0f;
    s.speedMax = 1100.0f;
    s.drag = 0.0f;

    s.angleMin = 0.0f;
    s.angleMax = 360.0f;
    s.gravityX = 0.0f;
    s.gravityY = 1600.0f;

    s.radialAccel = 0.0f;
    s.tangentialAccel = 0.0f;
    s.turbulence = 0.0f;

    s.startScaleMin = 0.18f;
    s.startScaleMax = 0.45f;
    s.endScaleMin   = 0.16f;
    s.endScaleMax   = 0.40f;

    s.startRotationMin = 0.0f;
    s.startRotationMax = 360.0f;
    s.spinSpeedMin = -900.0f;
    s.spinSpeedMax =  900.0f;
    s.velocityStretch = 0.0f;

    s.use3PhaseColor = true;
    s.startColor[0]  = 1.00f; s.startColor[1]  = 0.75f; s.startColor[2]  = 0.35f; s.startColor[3]  = 1.0f;
    s.middleColor[0] = 0.62f; s.middleColor[1]  = 0.58f; s.middleColor[2]  = 0.54f; s.middleColor[3]  = 1.0f;
    s.endColor[0]    = 0.35f; s.endColor[1]    = 0.32f; s.endColor[2]    = 0.30f; s.endColor[3]    = 1.0f;
    s.colorJitter = 0.20f;

    s.fadeInTime  = 0.0f;
    s.fadeOutTime = 0.10f;
    s.floorY = 1100.0f;

    s.blendMode = eSpriteBlendMode::NORMAL;
    return s;
}
static ParticleSettings& getPlasmaTrailPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 140.0f;
        s.maxParticles = 150;
        s.lifetimeMin = 0.20f;
        s.lifetimeMax = 0.40f;
        s.spawnRadius = 2.0f;

        s.speedMin = 0.0f;
        s.speedMax = 20.0f;
        s.angleMin = 0.0f;
        s.angleMax = 360.0f;

        s.gravityX = 0.0f;
        s.gravityY = 0.0f;
        s.radialAccel = 0.0f;
        s.tangentialAccel = 0.0f;
        s.turbulence = 0.0f;

        s.startScaleMin = 0.4f;
        s.startScaleMax = 0.7f;
        s.endScaleMin = 0.05f;
        s.endScaleMax = 0.15f;

        s.velocityStretch = 0.0f;
        s.use3PhaseColor = true;

        s.startColor[0] = 1.0f;  s.startColor[1] = 1.0f;  s.startColor[2] = 1.0f;  s.startColor[3] = 1.0f;
        s.middleColor[0] = 0.0f; s.middleColor[1] = 0.6f;  s.middleColor[2] = 1.0f; s.middleColor[3] = 0.9f;
        s.endColor[0] = 0.2f;    s.endColor[1] = 0.0f;    s.endColor[2] = 0.8f;    s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getHealingPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 40.0f; s.maxParticles = 100;
        s.lifetimeMin = 1.2f; s.lifetimeMax = 1.8f;
        s.spawnRadius = 35.0f;
        s.speedMin = 80.0f; s.speedMax = 120.0f;
        s.angleMin = 265.0f; s.angleMax = 275.0f;
        s.gravityY = -20.0f;
        s.startScaleMin = 0.4f; s.startScaleMax = 0.8f;
        s.endScaleMin = 0.1f; s.endScaleMax = 0.3f;

        s.startColor[0] = 1.0f; s.startColor[1] = 1.0f; s.startColor[2] = 0.4f; s.startColor[3] = 1.0f;
        s.endColor[0]   = 1.0f; s.endColor[1]   = 0.7f; s.endColor[2]   = 0.0f; s.endColor[3]   = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getPoisonPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 25.0f; s.maxParticles = 80;
        s.lifetimeMin = 1.8f; s.lifetimeMax = 2.6f;
        s.spawnRadius = 20.0f;
        s.speedMin = 20.0f; s.speedMax = 40.0f;
        s.angleMin = 0.0f; s.angleMax = 360.0f;
        s.tangentialAccel = 30.0f;
        s.startScaleMin = 0.8f; s.startScaleMax = 1.4f;
        s.endScaleMin = 2.5f; s.endScaleMax = 3.5f;
        s.spinSpeedMin = -40.0f; s.spinSpeedMax = 40.0f;

        s.startColor[0] = 0.2f; s.startColor[1] = 0.8f; s.startColor[2] = 0.1f; s.startColor[3] = 0.5f;
        s.endColor[0]   = 0.05f; s.endColor[1]  = 0.3f; s.endColor[2]   = 0.0f;  s.endColor[3]   = 0.0f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getPixieDustPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 50.0f; s.maxParticles = 150;
        s.lifetimeMin = 1.5f; s.lifetimeMax = 2.5f;
        s.spawnRadius = 5.0f;
        s.speedMin = 60.0f; s.speedMax = 140.0f;
        s.angleMin = 0.0f; s.angleMax = 360.0f;
        s.gravityY = 15.0f;
        s.radialAccel = -20.0f;
        s.startScaleMin = 0.3f; s.startScaleMax = 0.6f;
        s.endScaleMin = 0.0f; s.endScaleMax = 0.1f;

        s.startColor[0] = 1.0f; s.startColor[1] = 0.4f; s.startColor[2] = 0.8f; s.startColor[3] = 1.0f;
        s.endColor[0]   = 0.3f; s.endColor[1]   = 0.8f; s.endColor[2]   = 1.0f; s.endColor[3]   = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getShadowVoidPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 40.0f;
        s.maxParticles = 100;
        s.lifetimeMin = 1.0f;
        s.lifetimeMax = 1.8f;
        s.spawnRadius = 90.0f;

        s.speedMin = 20.0f;
        s.speedMax = 50.0f;
        s.angleMin = 0.0f;
        s.angleMax = 360.0f;

        s.gravityX = 0.0f;
        s.gravityY = 0.0f;

        s.radialAccel = -200.0f;
        s.tangentialAccel = 90.0f;
        s.turbulence = 10.0f;

        s.startScaleMin = 0.6f;
        s.startScaleMax = 1.0f;
        s.endScaleMin = 1.5f;
        s.endScaleMax = 2.5f;

        s.velocityStretch = 0.0f;
        s.use3PhaseColor = true;

        s.startColor[0] = 0.4f;  s.startColor[1] = 0.0f;  s.startColor[2] = 0.6f;  s.startColor[3] = 0.0f;
        s.middleColor[0] = 0.15f; s.middleColor[1] = 0.0f;  s.middleColor[2] = 0.25f; s.middleColor[3] = 0.75f;
        s.endColor[0] = 0.0f;    s.endColor[1] = 0.0f;    s.endColor[2] = 0.0f;    s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getSteamPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 90.0f; s.maxParticles = 150;
        s.lifetimeMin = 0.4f; s.lifetimeMax = 0.7f;
        s.spawnRadius = 2.0f;
        s.speedMin = 400.0f; s.speedMax = 600.0f;
        s.angleMin = -5.0f; s.angleMax = 5.0f;
        s.radialAccel = -300.0f;
        s.startScaleMin = 0.4f; s.startScaleMax = 0.7f;
        s.endScaleMin = 2.0f; s.endScaleMax = 3.5f;

        s.startColor[0] = 0.9f; s.startColor[1] = 0.95f; s.startColor[2] = 1.0f; s.startColor[3] = 0.6f;
        s.endColor[0]   = 1.0f; s.endColor[1]   = 1.0f;  s.endColor[2]   = 1.0f; s.endColor[3]   = 0.0f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getElectricSparksPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 15.0f; s.maxParticles = 30;
        s.lifetimeMin = 0.1f; s.lifetimeMax = 0.25f;
        s.spawnRadius = 8.0f;
        s.speedMin = 300.0f; s.speedMax = 500.0f;
        s.angleMin = 0.0f; s.angleMax = 360.0f;
        s.startScaleMin = 0.8f; s.startScaleMax = 1.5f;
        s.endScaleMin = 0.0f; s.endScaleMax = 0.2f;

        s.startColor[0] = 0.2f; s.startColor[1] = 0.6f; s.startColor[2] = 1.0f; s.startColor[3] = 1.0f;
        s.endColor[0]   = 0.8f; s.endColor[1]   = 0.9f; s.endColor[2]   = 1.0f; s.endColor[3]   = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getAcidDripsPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 3.0f; s.maxParticles = 15;
        s.lifetimeMin = 1.2f; s.lifetimeMax = 1.8f;
        s.spawnRadius = 1.0f;
        s.speedMin = 20.0f; s.speedMax = 40.0f;
        s.angleMin = 90.0f; s.angleMax = 90.0f;
        s.gravityY = 500.0f;
        s.startScaleMin = 0.6f; s.startScaleMax = 0.9f;
        s.endScaleMin = 0.4f; s.endScaleMax = 0.6f;

        s.startColor[0] = 0.7f; s.startColor[1] = 1.0f; s.startColor[2] = 0.0f; s.startColor[3] = 0.9f;
        s.endColor[0]   = 0.5f; s.endColor[1]   = 0.9f; s.endColor[2]   = 0.0f; s.endColor[3]   = 0.2f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getFireworksPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 0.0f;
        s.maxParticles = 300;
        s.lifetimeMin = 0.9f;
        s.lifetimeMax = 1.5f;
        s.spawnRadius = 1.0f;

        s.speedMin = 220.0f;
        s.speedMax = 380.0f;
        s.angleMin = 0.0f;
        s.angleMax = 360.0f;

        s.gravityY = 140.0f;
        s.radialAccel = -110.0f;
        s.tangentialAccel = 0.0f;

        s.startScaleMin = 0.7f;
        s.startScaleMax = 1.2f;
        s.endScaleMin = 0.0f;
        s.endScaleMax = 0.1f;

        s.startColor[0] = 1.0f; s.startColor[1] = 0.2f; s.startColor[2] = 0.1f; s.startColor[3] = 1.0f;
        s.endColor[0]   = 0.6f; s.endColor[1]   = 0.0f; s.endColor[2]   = 0.0f; s.endColor[3]   = 0.0f;

        s.useFlicker = true;
        s.flickerSpeed = 55.0f;
        s.flickerMinAlpha = 0.05f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getConfettiPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 35.0f; s.maxParticles = 150;
        s.lifetimeMin = 3.0f; s.lifetimeMax = 5.0f;
        s.spawnRadius = 350.0f;
        s.speedMin = 60.0f; s.speedMax = 120.0f;
        s.angleMin = 70.0f; s.angleMax = 110.0f;
        s.gravityY = 40.0f; s.gravityX = 10.0f;
        s.startScaleMin = 0.5f; s.startScaleMax = 1.2f;
        s.endScaleMin = 0.5f; s.endScaleMax = 1.2f;
        s.startRotationMin = 0.0f; s.startRotationMax = 360.0f;
        s.spinSpeedMin = -250.0f; s.spinSpeedMax = 250.0f;

        s.startColor[0] = 1.0f; s.startColor[1] = 0.84f; s.startColor[2] = 0.0f; s.startColor[3] = 1.0f;
        s.endColor[0]   = 0.9f; s.endColor[1]   = 0.5f;  s.endColor[2]   = 0.0f; s.endColor[3]   = 0.5f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getUIButtonGlowPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 20.0f; s.maxParticles = 40;
        s.lifetimeMin = 0.6f; s.lifetimeMax = 1.0f;
        s.spawnRadius = 45.0f;
        s.speedMin = 15.0f; s.speedMax = 30.0f;
        s.angleMin = 0.0f; s.angleMax = 360.0f;
        s.startScaleMin = 0.3f; s.startScaleMax = 0.5f;
        s.endScaleMin = 0.0f; s.endScaleMax = 0.1f;

        s.startColor[0] = 1.0f; s.startColor[1] = 1.0f; s.startColor[2] = 1.0f; s.startColor[3] = 0.7f;
        s.endColor[0]   = 0.0f; s.endColor[1]   = 0.7f; s.endColor[2]   = 1.0f; s.endColor[3]   = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getNebulaPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 4.0f; s.maxParticles = 20;
        s.lifetimeMin = 4.0f; s.lifetimeMax = 6.0f;
        s.spawnRadius = 80.0f;
        s.speedMin = 5.0f; s.speedMax = 15.0f;
        s.angleMin = 0.0f; s.angleMax = 360.0f;
        s.startScaleMin = 4.0f; s.startScaleMax = 6.0f;
        s.endScaleMin = 5.5f; s.endScaleMax = 8.0f;
        s.spinSpeedMin = -5.0f; s.spinSpeedMax = 5.0f;

        s.startColor[0] = 0.4f; s.startColor[1] = 0.0f; s.startColor[2] = 0.6f; s.startColor[3] = 0.2f;
        s.endColor[0]   = 0.0f; s.endColor[1]   = 0.2f; s.endColor[2]   = 0.5f; s.endColor[3]   = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getHyperspacePreset()
    {
        static ParticleSettings s;
        s.spawnRate = 200.0f; s.maxParticles = 500;
        s.lifetimeMin = 0.3f; s.lifetimeMax = 0.6f;
        s.spawnRadius = 5.0f;
        s.speedMin = 800.0f; s.speedMax = 1400.0f;
        s.angleMin = 0.0f; s.angleMax = 360.0f;
        s.radialAccel = 400.0f;
        s.startScaleMin = 0.1f; s.startScaleMax = 0.3f;
        s.endScaleMin = 1.5f; s.endScaleMax = 3.0f;

        s.startColor[0] = 1.0f; s.startColor[1] = 1.0f; s.startColor[2] = 1.0f; s.startColor[3] = 0.3f;
        s.endColor[0]   = 0.7f; s.endColor[1]   = 0.9f; s.endColor[2]   = 1.0f; s.endColor[3]   = 1.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getAdvancedSnowPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 35.0f; s.maxParticles = 250;
        s.lifetimeMin = 4.0f; s.lifetimeMax = 6.0f;
        s.spawnRadius = 400.0f;
        s.speedMin = 30.0f; s.speedMax = 50.0f;
        s.angleMin = 85.0f; s.angleMax = 95.0f;
        s.gravityY = 15.0f;
        s.startScaleMin = 0.1f; s.startScaleMax = 0.3f;
        s.endScaleMin = 0.1f; s.endScaleMax = 0.3f;

        s.waveFrequency = 3.5f;
        s.waveAmplitude = 25.0f;

        s.turbulence = 5.0f;

        s.startColor[0] = 1.0f; s.startColor[1] = 1.0f; s.startColor[2] = 1.0f; s.startColor[3] = 0.9f;
        s.endColor[0]   = 1.0f; s.endColor[1]   = 1.0f; s.endColor[2]   = 1.0f; s.endColor[3]   = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getAdvancedSparksPreset()
    {
        static ParticleSettings s;

        s.spawnRate = 120.0f;
        s.maxParticles = 200;
        s.lifetimeMin = 0.25f;
        s.lifetimeMax = 0.55f;
        s.spawnRadius = 2.0f;

        s.speedMin = 500.0f;
        s.speedMax = 1100.0f;
        s.angleMin = 0.0f;
        s.angleMax = 360.0f;

        s.gravityX = 0.0f;
        s.gravityY = 850.0f;
        s.radialAccel = 0.0f;
        s.tangentialAccel = 0.0f;
        s.turbulence = 15.0f;

        s.startScaleMin = 0.07f;
        s.startScaleMax = 0.14f;
        s.endScaleMin = 0.02f;
        s.endScaleMax = 0.05f;

        s.velocityStretch = 0.0055f;
        s.use3PhaseColor = true;

        s.startColor[0] = 1.0f;  s.startColor[1] = 1.0f;  s.startColor[2] = 1.0f;  s.startColor[3] = 1.0f;
        s.middleColor[0] = 1.0f; s.middleColor[1] = 0.85f; s.middleColor[2] = 0.15f; s.middleColor[3] = 1.0f;
        s.endColor[0] = 0.7f;    s.endColor[1] = 0.15f;   s.endColor[2] = 0.0f;    s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getRocketTrailPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 80.0f; s.maxParticles = 200;
        s.lifetimeMin = 0.8f; s.lifetimeMax = 1.4f;
        s.spawnRadius = 3.0f;
        s.speedMin = 20.0f; s.speedMax = 40.0f;
        s.angleMin = 0.0f; s.angleMax = 360.0f;

        s.startScaleMin = 0.5f; s.startScaleMax = 0.8f;
        s.endScaleMin = 2.0f; s.endScaleMax = 3.5f;

        s.use3PhaseColor = true;
        s.startColor[0]  = 1.0f;  s.startColor[1]  = 0.7f;  s.startColor[2]  = 0.1f;  s.startColor[3]  = 1.0f;
        s.middleColor[0] = 0.3f;  s.middleColor[1] = 0.3f;  s.middleColor[2] = 0.3f;  s.middleColor[3] = 0.7f;
        s.endColor[0]    = 0.15f; s.endColor[1]    = 0.15f; s.endColor[2]    = 0.15f; s.endColor[3]    = 0.0f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getPlasmaBallPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 120.0f; s.maxParticles = 300;
        s.lifetimeMin = 0.6f; s.lifetimeMax = 1.0f;
        s.spawnRadius = 60.0f;
        s.speedMin = 10.0f; s.speedMax = 30.0f;
        s.angleMin = 0.0f; s.angleMax = 360.0f;

        s.radialAccel = -180.0f;
        s.tangentialAccel = 350.0f;

        s.useFlicker = true;
        s.flickerSpeed = 80.0f;
        s.flickerMinAlpha = 0.05f;

        s.startScaleMin = 0.3f; s.startScaleMax = 0.6f;
        s.endScaleMin = 0.0f; s.endScaleMax = 0.1f;

        s.use3PhaseColor = true;
        s.startColor[0] = 0.0f; s.startColor[1] = 0.8f; s.startColor[2] = 1.0f; s.startColor[3] = 1.0f;
        s.middleColor[0] = 0.7f; s.middleColor[1] = 0.0f; s.middleColor[2] = 1.0f; s.middleColor[3] = 1.0f;
        s.endColor[0] = 1.0f; s.endColor[1] = 0.0f; s.endColor[2] = 0.4f; s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getFlamethrowerPreset()
{
    static ParticleSettings s;
    s.spawnRate = 180.0f; s.maxParticles = 400;
    s.lifetimeMin = 0.7f; s.lifetimeMax = 1.1f;
    s.spawnRadius = 4.0f;
    s.speedMin = 450.0f; s.speedMax = 600.0f;
    s.angleMin = -15.0f; s.angleMax = 15.0f;

    s.radialAccel = 0.0f;
    s.gravityY = 0.0f;
    s.turbulence = 40.0f;

    s.startScaleMin = 0.4f; s.startScaleMax = 0.7f;
    s.endScaleMin = 3.5f; s.endScaleMax = 5.0f;
    s.velocityStretch = 0.0015f;

    s.use3PhaseColor = true;
    s.startColor[0] = 1.0f; s.startColor[1] = 1.0f; s.startColor[2] = 1.0f; s.startColor[3] = 1.0f;
    s.middleColor[0] = 1.0f; s.middleColor[1] = 0.4f; s.middleColor[2] = 0.0f; s.middleColor[3] = 0.9f;
    s.endColor[0] = 0.15f; s.endColor[1] = 0.15f; s.endColor[2] = 0.15f; s.endColor[3] = 0.0f;

    s.blendMode = eSpriteBlendMode::ADDITIVE;
    return s;
}

    static ParticleSettings& getStarlightFountainPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 90.0f; s.maxParticles = 250;
        s.lifetimeMin = 1.6f; s.lifetimeMax = 2.4f;
        s.spawnRadius = 5.0f;
        s.speedMin = 280.0f; s.speedMax = 380.0f;
        s.angleMin = 255.0f; s.angleMax = 285.0f;
        s.gravityY = 320.0f;

        s.waveFrequency = 4.5f;
        s.waveAmplitude = 35.0f;

        s.startScaleMin = 0.5f; s.startScaleMax = 0.9f;
        s.endScaleMin = 0.0f; s.endScaleMax = 0.1f;

        s.use3PhaseColor = true;
        s.startColor[0] = 1.0f; s.startColor[1] = 0.85f; s.startColor[2] = 0.2f; s.startColor[3] = 1.0f;
        s.middleColor[0] = 0.0f; s.middleColor[1] = 1.0f; s.middleColor[2] = 0.5f; s.middleColor[3] = 1.0f;
        s.endColor[0] = 0.0f; s.endColor[1] = 0.3f; s.endColor[2] = 1.0f; s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getEtherealWispPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 60.0f; s.maxParticles = 180;
        s.lifetimeMin = 1.0f; s.lifetimeMax = 1.5f;
        s.spawnRadius = 2.0f;
        s.speedMin = 120.0f; s.speedMax = 180.0f;
        s.angleMin = 0.0f; s.angleMax = 360.0f;

        s.waveFrequency = 14.0f;
        s.waveAmplitude = 75.0f;

        s.useFlicker = true;
        s.flickerSpeed = 30.0f;
        s.flickerMinAlpha = 0.2f;

        s.startScaleMin = 0.6f; s.startScaleMax = 1.0f;
        s.endScaleMin = 0.0f; s.endScaleMax = 0.2f;

        s.startColor[0] = 0.3f; s.startColor[1] = 1.0f; s.startColor[2] = 0.6f; s.startColor[3] = 1.0f;
        s.endColor[0] = 0.0f; s.endColor[1] = 0.2f; s.endColor[2] = 0.4f; s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getLaserSparksPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 800.0f; s.maxParticles = 150;
        s.lifetimeMin = 0.15f; s.lifetimeMax = 0.35f;
        s.spawnRadius = 1.0f;
        s.speedMin = 600.0f; s.speedMax = 950.0f;
        s.angleMin = 240.0f; s.angleMax = 300.0f;

        s.radialAccel = -400.0f;

        s.velocityStretch = 0.006f;

        s.startScaleMin = 0.8f; s.startScaleMax = 1.4f;
        s.endScaleMin = 0.0f; s.endScaleMax = 0.1f;

        s.use3PhaseColor = true;
        s.startColor[0] = 1.0f; s.startColor[1] = 1.0f; s.startColor[2] = 1.0f; s.startColor[3] = 1.0f;
        s.middleColor[0] = 0.0f; s.middleColor[1] = 1.0f; s.middleColor[2] = 0.3f; s.middleColor[3] = 1.0f;
        s.endColor[0] = 0.0f; s.endColor[1] = 0.4f; s.endColor[2] = 0.1f; s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getRailgunTrailPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 180.0f;
        s.maxParticles = 300;
        s.lifetimeMin = 0.10f;
        s.lifetimeMax = 0.25f;
        s.spawnRadius = 0.0f;

        s.speedMin = 1500.0f;
        s.speedMax = 2200.0f;

        s.angleMin = 0.0f;
        s.angleMax = 1.0f;

        s.gravityX = 0.0f;
        s.gravityY = 0.0f;
        s.radialAccel = 0.0f;
        s.tangentialAccel = 0.0f;
        s.turbulence = 0.0f;

        s.startScaleMin = 0.08f;
        s.startScaleMax = 0.18f;
        s.endScaleMin = 0.01f;
        s.endScaleMax = 0.04f;

        s.velocityStretch = 0.005f;
        s.use3PhaseColor = true;

        s.startColor[0]  = 1.0f;  s.startColor[1]  = 1.0f;  s.startColor[2]  = 1.0f;  s.startColor[3]  = 1.0f;
        s.middleColor[0] = 0.0f;  s.middleColor[1] = 0.8f;  s.middleColor[2] = 1.0f;  s.middleColor[3] = 1.0f;
        s.endColor[0]    = 0.4f;  s.endColor[1]    = 0.0f;  s.endColor[2]    = 0.8f;  s.endColor[3]    = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getCoreMeltdownPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 0.0f; s.maxParticles = 500;
        s.lifetimeMin = 1.2f; s.lifetimeMax = 2.0f;
        s.spawnRadius = 5.0f;
        s.speedMin = 500.0f; s.speedMax = 850.0f;

        s.radialAccel = -350.0f;
        s.turbulence = 65.0f;

        s.startScaleMin = 0.8f; s.startScaleMax = 1.5f;
        s.endScaleMin = 6.0f; s.endScaleMax = 9.0f;

        s.velocityStretch = 0.003f;

        s.use3PhaseColor = true;
        s.startColor[0] = 0.0f; s.startColor[1] = 1.0f; s.startColor[2] = 0.9f; s.startColor[3] = 1.0f;
        s.middleColor[0] = 0.8f; s.middleColor[1] = 0.0f; s.middleColor[2] = 1.0f; s.middleColor[3] = 0.6f;
        s.endColor[0] = 0.1f; s.endColor[1] = 0.1f; s.endColor[2] = 0.1f; s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getSandstormPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 140.0f; s.maxParticles = 400;
        s.lifetimeMin = 1.2f; s.lifetimeMax = 2.0f;
        s.spawnRadius = 300.0f;
        s.speedMin = 500.0f; s.speedMax = 750.0f;
        s.angleMin = 175.0f; s.angleMax = 185.0f;

        s.turbulence = 50.0f;

        s.velocityStretch = 0.001f;

        s.startScaleMin = 0.2f; s.startScaleMax = 0.5f;
        s.endScaleMin = 0.2f; s.endScaleMax = 0.5f;

        s.startColor[0] = 0.85f; s.startColor[1] = 0.7f; s.startColor[2] = 0.45f; s.startColor[3] = 0.6f;
        s.endColor[0] = 0.6f; s.endColor[1] = 0.5f; s.endColor[2] = 0.3f; s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

     static ParticleSettings& getSakuraPreset()
     {
         static ParticleSettings s;

         s.spawnRate = 25.0f;
         s.maxParticles = 150;
         s.lifetimeMin = 5.0f;
         s.lifetimeMax = 8.0f;

         s.spawnRadius = 0.0f;
         s.spawnAreaCx = 1200.0f;
         s.spawnAreaCy = 40.0f;

         s.speedMin = 15.0f;
         s.speedMax = 35.0f;
         s.angleMin = 75.0f;
         s.angleMax = 105.0f;
         s.gravityX = 12.0f;
         s.gravityY = 20.0f;

         s.flutterSpeedMin = 1.5f;
         s.flutterSpeedMax = 3.5f;
         s.flutterAffectsFall = true;
         s.flutterSlip = 35.0f;

         s.waveFrequency = 1.2f;
         s.waveAmplitude = 25.0f;

         s.startRotationMin = 0.0f;
         s.startRotationMax = 360.0f;
         s.spinSpeedMin = -25.0f;
         s.spinSpeedMax = 25.0f;

         s.startScaleMin = 0.25f;
         s.startScaleMax = 0.45f;
         s.endScaleMin = 0.20f;
         s.endScaleMax = 0.35f;

         s.use3PhaseColor = false;
         s.startColor[0] = 1.00f; s.startColor[1] = 0.88f; s.startColor[2] = 0.92f; s.startColor[3] = 0.90f;
         s.endColor[0]   = 1.00f; s.endColor[1]   = 0.70f; s.endColor[2]   = 0.80f; s.endColor[3]   = 0.00f;

         s.colorJitter = 0.08f;

         s.blendMode = eSpriteBlendMode::NORMAL;

         return s;
     }
static ParticleSettings& getGrenadeExplosionPreset()
{
    static ParticleSettings s;
    s.spawnRate = 0.0f;
    s.maxParticles = 120;
    s.lifetimeMin = 0.35f;
    s.lifetimeMax = 0.90f;
    s.spawnRadius = 3.0f;

    s.speedMin = 180.0f;
    s.speedMax = 550.0f;
    s.angleMin = 0.0f;
    s.angleMax = 360.0f;
    s.gravityX = 0.0f;
    s.gravityY = -35.0f;
    s.radialAccel = -130.0f;
    s.tangentialAccel = 0.0f;
    s.turbulence = 45.0f;

    s.startScaleMin = 0.35f;
    s.startScaleMax = 0.55f;
    s.endScaleMin = 1.8f;
    s.endScaleMax = 3.2f;
    s.velocityStretch = 0.0f;
    s.use3PhaseColor = true;

    s.startColor[0]  = 1.0f;  s.startColor[1]  = 1.0f;  s.startColor[2]  = 0.85f; s.startColor[3]  = 1.0f;
    s.middleColor[0] = 1.0f;  s.middleColor[1]  = 0.50f; s.middleColor[2]  = 0.08f; s.middleColor[3]  = 0.90f;
    s.endColor[0]    = 0.45f; s.endColor[1]    = 0.18f; s.endColor[2]    = 0.08f; s.endColor[3]    = 0.0f;
    s.fadeInTime  = 0.05f;
    s.fadeOutTime = 0.35f;

    s.blendMode = eSpriteBlendMode::ADDITIVE;
    return s;
}

    static ParticleSettings& getMechExplosionPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 0.0f;
        s.maxParticles = 140;
        s.lifetimeMin = 0.25f;
        s.lifetimeMax = 0.55f;
        s.spawnRadius = 4.0f;

        s.speedMin = 250.0f;
        s.speedMax = 550.0f;
        s.angleMin = 0.0f;
        s.angleMax = 360.0f;

        s.gravityX = 0.0f;
        s.gravityY = 0.0f;
        s.radialAccel = 0.0f;
        s.tangentialAccel = 0.0f;

        s.turbulence = 120.0f;

        s.startScaleMin = 0.12f;
        s.startScaleMax = 0.28f;
        s.endScaleMin = 0.02f;
        s.endScaleMax = 0.06f;

        s.velocityStretch = 0.005f;
        s.use3PhaseColor = true;

        s.startColor[0] = 1.0f;  s.startColor[1] = 1.0f;  s.startColor[2] = 1.0f;  s.startColor[3] = 1.0f;
        s.middleColor[0] = 0.0f; s.middleColor[1] = 0.75f; s.middleColor[2] = 1.0f; s.middleColor[3] = 1.0f;
        s.endColor[0] = 0.4f;    s.endColor[1] = 0.0f;    s.endColor[2] = 0.8f;    s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getCosmicNovaPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 0.0f; s.maxParticles = 400;
        s.lifetimeMin = 0.8f; s.lifetimeMax = 1.5f;
        s.spawnRadius = 0.0f;
        s.speedMin = 600.0f; s.speedMax = 900.0f;

        s.radialAccel = -450.0f;
        s.waveFrequency = 5.0f;
        s.waveAmplitude = 20.0f;

        s.startScaleMin = 0.5f; s.startScaleMax = 1.2f;
        s.endScaleMin = 0.0f; s.endScaleMax = 0.2f;

        s.velocityStretch = 0.004f;

        s.use3PhaseColor = true;
        s.startColor[0] = 1.0f;  s.startColor[1] = 1.0f;  s.startColor[2] = 1.0f;  s.startColor[3] = 1.0f;
        s.middleColor[0] = 0.0f; s.middleColor[1] = 1.0f;  s.middleColor[2] = 0.8f; s.middleColor[3] = 1.0f;
        s.endColor[0] = 0.5f;    s.endColor[1] = 0.0f;    s.endColor[2] = 1.0f;    s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getHolyBurstPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 0.0f; s.maxParticles = 150;
        s.lifetimeMin = 1.0f; s.lifetimeMax = 1.6f;
        s.spawnRadius = 10.0f;
        s.speedMin = 150.0f; s.speedMax = 250.0f;
        s.gravityY = -40.0f;

        s.waveFrequency = 3.0f;
        s.waveAmplitude = 25.0f;

        s.startScaleMin = 0.6f; s.startScaleMax = 1.0f;
        s.endScaleMin = 0.1f; s.endScaleMax = 0.3f;

        s.use3PhaseColor = true;
        s.startColor[0] = 1.0f;  s.startColor[1] = 1.0f;  s.startColor[2] = 1.0f;  s.startColor[3] = 1.0f;
        s.middleColor[0] = 1.0f; s.middleColor[1] = 0.9f;  s.middleColor[2] = 0.3f; s.middleColor[3] = 0.9f;
        s.endColor[0] = 1.0f;    s.endColor[1] = 0.5f;    s.endColor[2] = 0.0f;    s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getCriticalBloodPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 0.0f;
        s.maxParticles = 80;
        s.lifetimeMin = 0.25f;
        s.lifetimeMax = 0.55f;
        s.spawnRadius = 2.0f;

        s.speedMin = 400.0f;
        s.speedMax = 750.0f;

        s.angleMin = 220.0f;
        s.angleMax = 320.0f;

        s.gravityX = 0.0f;
        s.gravityY = 1200.0f;

        s.radialAccel = 0.0f;
        s.tangentialAccel = 0.0f;
        s.turbulence = 0.0f;

        s.startScaleMin = 0.06f;
        s.startScaleMax = 0.15f;
        s.endScaleMin = 0.03f;
        s.endScaleMax = 0.08f;

        s.velocityStretch = 0.0025f;
        s.use3PhaseColor = false;

        s.startColor[0] = 0.50f; s.startColor[1] = 0.0f; s.startColor[2] = 0.0f; s.startColor[3] = 1.0f;
        s.endColor[0]   = 0.30f; s.endColor[1]   = 0.0f; s.endColor[2]   = 0.0f; s.endColor[3]   = 0.0f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getPropSmashPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 400.0f;
        s.maxParticles = 25;
        s.lifetimeMin = 0.3f;
        s.lifetimeMax = 0.6f;
        s.spawnRadius = 4.0f;

        s.speedMin = 250.0f;
        s.speedMax = 500.0f;
        s.angleMin = 0.0f;
        s.angleMax = 360.0f;

        s.gravityX = 0.0f;
        s.gravityY = 700.0f;
        s.radialAccel = 0.0f;
        s.tangentialAccel = 0.0f;
        s.turbulence = 0.0f;

        s.startScaleMin = 0.15f;
        s.startScaleMax = 0.35f;
        s.endScaleMin = 0.10f;
        s.endScaleMax = 0.25f;

        s.velocityStretch = 0.0f;
        s.use3PhaseColor = false;

        s.startColor[0] = 0.40f; s.startColor[1] = 0.25f; s.startColor[2] = 0.12f; s.startColor[3] = 1.0f;
        s.endColor[0]   = 0.40f; s.endColor[1]   = 0.25f; s.endColor[2]   = 0.12f; s.endColor[3]   = 0.0f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getIceNovaPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 0.0f; s.maxParticles = 200;
        s.lifetimeMin = 0.7f; s.lifetimeMax = 1.3f;
        s.spawnRadius = 4.0f;
        s.speedMin = 250.0f; s.speedMax = 400.0f;
        s.radialAccel = -150.0f;

        s.waveFrequency = 5.5f;
        s.waveAmplitude = 25.0f;

        s.useFlicker = true;
        s.flickerSpeed = 40.0f;
        s.flickerMinAlpha = 0.2f;

        s.startScaleMin = 0.4f; s.startScaleMax = 0.9f;
        s.endScaleMin = 0.0f; s.endScaleMax = 0.2f;

        s.startColor[0] = 0.9f; s.startColor[1] = 0.95f; s.startColor[2] = 1.0f; s.startColor[3] = 1.0f;
        s.endColor[0] = 0.2f;   s.endColor[1] = 0.6f;    s.endColor[2] = 1.0f;   s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getToxicSporePreset()
    {
        static ParticleSettings s;
        s.spawnRate = 0.0f; s.maxParticles = 120;
        s.lifetimeMin = 1.4f; s.lifetimeMax = 2.2f;
        s.spawnRadius = 2.0f;
        s.speedMin = 120.0f; s.speedMax = 220.0f;

        s.radialAccel = -100.0f;
        s.turbulence = 20.0f;

        s.spinSpeedMin = -90.0f; s.spinSpeedMax = 90.0f;

        s.startScaleMin = 0.5f; s.startScaleMax = 0.8f;
        s.endScaleMin = 3.0f; s.endScaleMax = 4.5f;

        s.use3PhaseColor = true;
        s.startColor[0] = 0.6f;  s.startColor[1] = 1.0f;  s.startColor[2] = 0.2f;  s.startColor[3] = 0.9f;
        s.middleColor[0] = 0.2f; s.middleColor[1] = 0.6f;  s.middleColor[2] = 0.1f; s.middleColor[3] = 0.6f;
        s.endColor[0] = 0.05f;   s.endColor[1] = 0.2f;    s.endColor[2] = 0.0f;    s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

static ParticleSettings& getFirefliesPreset()
{
    static ParticleSettings s;

    s.spawnRate = 8.5f;
    s.maxParticles = 50;

    s.lifetimeMin = 4.8f;
    s.lifetimeMax = 8.5f;

    s.spawnRadius = 160.0f;
    s.spawnAreaCx = 0.0f;
    s.spawnAreaCy = 0.0f;

    s.speedMin = 5.0f;
    s.speedMax = 20.0f;

    s.angleMin = 0.0f;
    s.angleMax = 360.0f;

    s.gravityX = 0.0f;
    s.gravityY = -1.2f;

    s.radialAccel = 0.0f;
    s.tangentialAccel = 0.0f;

    s.turbulence = 6.0f;

    s.waveFrequency = 1.4f;
    s.waveAmplitude = 18.0f;

    s.drag = 0.78f;

    s.windX = 0.0f;
    s.gustAmp = 0.0f;
    s.gustFreq = 0.0f;

    s.velocityStretch = 0.0f;

    s.floorY = 1e9f;

    s.startScaleMin = 0.07f * 1.75f;
    s.startScaleMax = 0.14f * 1.75f;

    s.endScaleMin = 0.14f * 1.75f;
    s.endScaleMax = 0.24f * 1.75f;

    s.startRotationMin = 0.0f;
    s.startRotationMax = 360.0f;
    s.spinSpeedMin = -8.0f;
    s.spinSpeedMax = 8.0f;

    s.flutterSpeedMin = 0.0f;
    s.flutterSpeedMax = 0.0f;
    s.flutterAffectsFall = true;
    s.flutterSlip = 0.0f;

    s.useFlicker = true;

    s.flickerSpeed = 2.2f;

    s.flickerSpeedMin = 0.6f;
    s.flickerSpeedMax = 6.2f;

    s.flickerMinAlpha = 0.0f;
    s.flickerMaxAlpha = 1.0f;

    s.use3PhaseColor = true;

    s.startColor[0] = 0.68f;
    s.startColor[1] = 1.00f;
    s.startColor[2] = 0.30f;
    s.startColor[3] = 0.00f;

    s.middleColor[0] = 1.00f;
    s.middleColor[1] = 0.78f;
    s.middleColor[2] = 0.34f;
    s.middleColor[3] = 0.88f;

    s.endColor[0] = 1.00f;
    s.endColor[1] = 0.55f;
    s.endColor[2] = 0.10f;
    s.endColor[3] = 0.00f;

    s.fadeInTime = 0.65f;
    s.fadeOutTime = 2.30f;

    s.colorJitter = -0.24f;
    s.colorTempJitter = 0.0f;

    s.useDepth = true;
    s.depthMin = 0.30f;

    s.depthScaleMin = 0.35f;

    s.depthSpeedMin = 0.45f;

    s.depthAlphaMin = 0.35f;

    s.blendMode = eSpriteBlendMode::ADDITIVE;

    return s;
}
    static ParticleSettings& getDandelionFluffPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 6.0f; s.maxParticles = 40;
        s.lifetimeMin = 5.0f; s.lifetimeMax = 8.0f;
        s.spawnRadius = 150.0f;
        s.speedMin = 20.0f; s.speedMax = 45.0f;
        s.angleMin = 160.0f; s.angleMax = 200.0f;
        s.gravityY = 4.0f;
        s.gravityX = -10.0f;

        s.waveFrequency = 2.0f;
        s.waveAmplitude = 15.0f;

        s.startScaleMin = 0.3f; s.startScaleMax = 0.6f;
        s.endScaleMin = 0.3f; s.endScaleMax = 0.6f;

        s.startColor[0] = 1.0f; s.startColor[1] = 1.0f; s.startColor[2] = 1.0f; s.startColor[3] = 0.0f;
        s.middleColor[0] = 1.0f; s.middleColor[1] = 1.0f; s.middleColor[2] = 1.0f; s.middleColor[3] = 0.6f;
        s.endColor[0] = 1.0f;   s.endColor[1] = 1.0f;   s.endColor[2] = 1.0f;   s.endColor[3] = 0.0f;
        s.use3PhaseColor = true;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getSunbeamDustPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 8.0f; s.maxParticles = 50;
        s.lifetimeMin = 4.0f; s.lifetimeMax = 7.0f;
        s.spawnRadius = 200.0f;
        s.speedMin = 5.0f; s.speedMax = 12.0f;
        s.angleMin = 0.0f; s.angleMax = 360.0f;
        s.turbulence = 2.0f;

        s.startScaleMin = 0.1f; s.startScaleMax = 0.3f;
        s.endScaleMin = 0.1f; s.endScaleMax = 0.3f;

        s.startColor[0] = 1.0f; s.startColor[1] = 0.95f; s.startColor[2] = 0.8f; s.startColor[3] = 0.0f;
        s.middleColor[0] = 1.0f; s.middleColor[1] = 0.95f; s.middleColor[2] = 0.8f; s.middleColor[3] = 0.4f;
        s.endColor[0] = 1.0f;   s.endColor[1] = 0.95f;   s.endColor[2] = 0.8f;   s.endColor[3] = 0.0f;
        s.use3PhaseColor = true;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getLakeMistPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 2.0f; s.maxParticles = 15;
        s.lifetimeMin = 6.0f; s.lifetimeMax = 9.0f;
        s.spawnRadius = 100.0f;
        s.speedMin = 8.0f; s.speedMax = 18.0f;
        s.angleMin = -10.0f; s.angleMax = 10.0f;
        s.spinSpeedMin = -2.0f; s.spinSpeedMax = 2.0f;

        s.startScaleMin = 4.0f; s.startScaleMax = 6.0f;
        s.endScaleMin = 5.0f; s.endScaleMax = 7.0f;

        s.startColor[0] = 0.9f; s.startColor[1] = 0.95f; s.startColor[2] = 1.0f; s.startColor[3] = 0.0f;
        s.middleColor[0] = 0.85f; s.middleColor[1] = 0.9f; s.middleColor[2] = 0.95f; s.middleColor[3] = 0.15f;
        s.endColor[0] = 0.8f;   s.endColor[1] = 0.85f;  s.endColor[2] = 0.9f;   s.endColor[3] = 0.0f;
        s.use3PhaseColor = true;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getCupSteamPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 5.0f;
        s.maxParticles = 100;
        s.lifetimeMin = 2.0f;
        s.lifetimeMax = 5.0f;
        s.spawnRadius = 4.0f;

        s.speedMin = 1.10f;
        s.speedMax = 2.10f;

        s.angleMin = 265.0f;
        s.angleMax = 275.0f;

        s.gravityX = 0.0f;
        s.gravityY = 0.0f;
        s.radialAccel = 0.0f;
        s.tangentialAccel = 0.0f;
        s.turbulence = 0.0f;

        s.startScaleMin = 1.3f;
        s.startScaleMax = 2.5f;
        s.endScaleMin = 2.8f;
        s.endScaleMax = 3.3f;

        s.velocityStretch = 0.0f;
        s.use3PhaseColor = false;

        s.startColor[0] = 0.8f; s.startColor[1] = 0.85f; s.startColor[2] = 0.9f; s.startColor[3] = 0.05f;
        s.endColor[0]   = 0.8f;   s.endColor[1] = 0.85f;   s.endColor[2] = 0.9f;   s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getAetherWispsPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 5.0f; s.maxParticles = 20;
        s.lifetimeMin = 2.5f; s.lifetimeMax = 4.0f;
        s.spawnRadius = 30.0f;
        s.speedMin = 15.0f; s.speedMax = 30.0f;
        s.angleMin = 250.0f; s.angleMax = 290.0f;

        s.waveFrequency = 1.8f;
        s.waveAmplitude = 14.0f;

        s.startScaleMin = 0.6f; s.startScaleMax = 1.0f;
        s.endScaleMin = 0.0f; s.endScaleMax = 0.2f;

        s.use3PhaseColor = true;
        s.startColor[0] = 0.1f; s.startColor[1] = 0.0f; s.startColor[2] = 0.6f; s.startColor[3] = 0.0f;
        s.middleColor[0] = 0.4f; s.middleColor[1] = 0.1f; s.middleColor[2] = 1.0f; s.middleColor[3] = 0.5f;
        s.endColor[0] = 0.0f;   s.endColor[1] = 0.5f;   s.endColor[2] = 0.8f;   s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getGlowingSporesPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 7.0f; s.maxParticles = 45;
        s.lifetimeMin = 3.5f; s.lifetimeMax = 5.5f;
        s.spawnRadius = 180.0f;
        s.speedMin = 120.0f; s.speedMax = 120.0f;
        s.gravityY = 8.0f;

        s.useFlicker = true;
        s.flickerSpeed = 6.0f;
        s.flickerMinAlpha = 0.15f;

        s.startScaleMin = 0.2f; s.startScaleMax = 0.5f;
        s.endScaleMin = 0.0f; s.endScaleMax = 0.1f;

        s.startColor[0] = 0.0f; s.startColor[1] = 0.9f; s.startColor[2] = 0.9f; s.startColor[3] = 0.8f;
        s.endColor[0] = 0.0f;   s.endColor[1] = 0.3f;   s.endColor[2] = 0.5f;   s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    static ParticleSettings& getPlanktonPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 9.0f; s.maxParticles = 60;
        s.lifetimeMin = 4.0f; s.lifetimeMax = 6.5f;
        s.spawnRadius = 250.0f;
        s.speedMin = 8.0f; s.speedMax = 20.0f;
        s.angleMin = 0.0f; s.angleMax = 360.0f;

        s.waveFrequency = 1.0f;
        s.waveAmplitude = 6.0f;
        s.turbulence = 1.5f;

        s.startScaleMin = 0.2f; s.startScaleMax = 0.7f;
        s.endScaleMin = 0.2f; s.endScaleMax = 0.7f;

        s.startColor[0] = 0.4f; s.startColor[1] = 0.7f; s.startColor[2] = 0.9f; s.startColor[3] = 0.0f;
        s.middleColor[0] = 0.5f; s.middleColor[1] = 0.8f; s.middleColor[2] = 1.0f; s.middleColor[3] = 0.35f;
        s.endColor[0] = 0.3f;   s.endColor[1] = 0.6f;   s.endColor[2] = 0.8f;   s.endColor[3] = 0.0f;
        s.use3PhaseColor = true;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

static ParticleSettings& getDistantEmbersPreset()
{
    static ParticleSettings s;
    s.spawnRate = 10.0f;
    s.maxParticles = 80;
    s.lifetimeMin = 4.0f;
    s.lifetimeMax = 7.0f;
    s.spawnRadius = 0.0f;
    s.spawnAreaCx = 1400.0f;
    s.spawnAreaCy = 1000.0f;
    s.speedMin = 15.0f;
    s.speedMax = 40.0f;
    s.angleMin = 250.0f;
    s.angleMax = 290.0f;
    s.gravityX = 10.0f;
    s.gravityY = -6.0f;

    s.drag = 0.22f;
    s.turbulence = 25.0f;
    s.waveFrequency = 2.5f;
    s.waveAmplitude = 18.0f;
    s.startScaleMin = 0.35f * 3.f;
    s.startScaleMax = 0.70f * 3.f;
    s.endScaleMin = 0.15f * 3.f;
    s.endScaleMax = 0.35f * 3.f;
    s.startRotationMin = 0.0f;
    s.startRotationMax = 360.0f;
    s.spinSpeedMin = -20.0f;
    s.spinSpeedMax = 20.0f;
    s.blendMode = eSpriteBlendMode::ADDITIVE;
    s.use3PhaseColor = true;
    s.startColor[0]  = 1.00f; s.startColor[1]  = 0.75f; s.startColor[2]  = 0.30f; s.startColor[3]  = 0.90f;
    s.middleColor[0] = 1.00f; s.middleColor[1] = 0.35f; s.middleColor[2] = 0.08f; s.middleColor[3] = 0.75f;
    s.endColor[0]    = 0.25f; s.endColor[1]    = 0.03f; s.endColor[2]    = 0.00f; s.endColor[3]    = 0.00f;
    s.colorJitter = -0.20f;
    s.useFlicker = true;
    s.flickerSpeedMin = 3.0f;
    s.flickerSpeedMax = 9.0f;
    s.flickerMinAlpha = 0.25f;
    s.flickerMaxAlpha = 1.0f;
    s.fadeInTime = 0.4f;
    s.fadeOutTime = 1.2f;
    s.useDepth = true;
    s.depthMin = 0.40f;
    s.depthScaleMin = 0.40f;
    s.depthSpeedMin = 0.40f;
    s.depthAlphaMin = 0.45f;

    return s;
}

    static ParticleSettings& getBokehAmbientPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 4.0f; s.maxParticles = 25;
        s.lifetimeMin = 4.0f; s.lifetimeMax = 6.0f;
        s.spawnRadius = 400.0f;
        s.speedMin = 20.0f; s.speedMax = 45.0f;
        s.angleMin = 260.0f; s.angleMax = 280.0f;
        s.gravityY = 0.0f;

        s.startScaleMin = 0.6f; s.startScaleMax = 1.2f;
        s.endScaleMin = 0.6f;   s.endScaleMax = 1.2f;

        s.use3PhaseColor = true;

        s.startColor[0] = 1.0f;  s.startColor[1] = 0.9f;  s.startColor[2] = 0.95f; s.startColor[3] = 0.0f;
        s.middleColor[0] = 1.0f; s.middleColor[1] = 0.9f;  s.middleColor[2] = 0.95f; s.middleColor[3] = 0.65f;
        s.endColor[0] = 1.0f;    s.endColor[1] = 0.85f; s.endColor[2] = 0.9f;  s.endColor[3] = 0.0f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    // ===================================================================
    //  ПРЕСЕТЫ ДЛЯ КНОПОК (UI overlay)
    //  Все рассчитаны на gentle-накладывание: низкая альфа, мягкое
    //  движение, спред по spawnAreaCx — подгоните под ширину кнопки:
    //      auto& s = ParticlePresets::getButtonGoldSparklePreset();
    //      s.spawnAreaCx = widthBtn;
    // ===================================================================

    // золотые искры, поднимающиеся от нижнего края кнопки
    static ParticleSettings& getButtonGoldSparklePreset()
    {
        static ParticleSettings s;
        s.spawnRate = 14.0f; s.maxParticles = 60;
        s.lifetimeMin = 0.9f; s.lifetimeMax = 1.6f;
        s.spawnRadius = 0.0f;
        s.spawnAreaCx = 220.0f; s.spawnAreaCy = 6.0f;

        s.speedMin = 25.0f; s.speedMax = 55.0f;
        s.angleMin = 265.0f; s.angleMax = 275.0f;
        s.gravityY = -12.0f;
        s.turbulence = 8.0f;

        s.startScaleMin = 0.10f; s.startScaleMax = 0.22f;
        s.endScaleMin = 0.02f;   s.endScaleMax = 0.06f;
        s.spinSpeedMin = -40.0f; s.spinSpeedMax = 40.0f;

        s.useFlicker = true;
        s.flickerSpeedMin = 2.0f; s.flickerSpeedMax = 6.0f;
        s.flickerMinAlpha = 0.20f; s.flickerMaxAlpha = 1.0f;

        s.use3PhaseColor = true;
        s.startColor[0]  = 1.00f; s.startColor[1]  = 0.80f; s.startColor[2]  = 0.30f; s.startColor[3]  = 0.00f;
        s.middleColor[0] = 1.00f; s.middleColor[1]  = 0.90f; s.middleColor[2]  = 0.55f; s.middleColor[3]  = 0.85f;
        s.endColor[0]    = 1.00f; s.endColor[1]    = 0.60f; s.endColor[2]    = 0.15f; s.endColor[3]    = 0.00f;

        s.fadeInTime = 0.15f; s.fadeOutTime = 0.40f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    // тлеющие уголки, медленно взлетающие над кнопкой
    static ParticleSettings& getButtonEmberRisePreset()
    {
        static ParticleSettings s;
        s.spawnRate = 10.0f; s.maxParticles = 40;
        s.lifetimeMin = 1.2f; s.lifetimeMax = 2.2f;
        s.spawnRadius = 0.0f;
        s.spawnAreaCx = 220.0f; s.spawnAreaCy = 6.0f;

        s.speedMin = 30.0f; s.speedMax = 70.0f;
        s.angleMin = 262.0f; s.angleMax = 278.0f;
        s.gravityY = -8.0f;
        s.turbulence = 12.0f;

        s.startScaleMin = 0.08f; s.startScaleMax = 0.16f;
        s.endScaleMin   = 0.02f; s.endScaleMax   = 0.05f;

        s.useFlicker = true;
        s.flickerSpeedMin = 3.0f; s.flickerSpeedMax = 8.0f;
        s.flickerMinAlpha = 0.15f; s.flickerMaxAlpha = 1.0f;

        s.use3PhaseColor = true;
        s.startColor[0]  = 0.90f; s.startColor[1]  = 0.25f; s.startColor[2]  = 0.02f; s.startColor[3]  = 0.00f;
        s.middleColor[0] = 1.00f; s.middleColor[1]  = 0.55f; s.middleColor[2]  = 0.08f; s.middleColor[3]  = 0.80f;
        s.endColor[0]    = 0.60f; s.endColor[1]    = 0.10f; s.endColor[2]    = 0.00f; s.endColor[3]    = 0.00f;

        s.colorJitter = -0.20f;
        s.fadeInTime = 0.25f; s.fadeOutTime = 0.50f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    // торжественная бело-золотая аура над кнопкой (символы медленно всплывают)
    static ParticleSettings& getButtonHolyAuraPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 7.0f; s.maxParticles = 30;
        s.lifetimeMin = 2.0f; s.lifetimeMax = 3.5f;
        s.spawnRadius = 0.0f;
        s.spawnAreaCx = 220.0f; s.spawnAreaCy = 10.0f;

        s.speedMin = 14.0f; s.speedMax = 30.0f;
        s.angleMin = 268.0f; s.angleMax = 272.0f;
        s.gravityY = -6.0f;
        s.turbulence = 4.0f;

        s.waveFrequency = 1.2f; s.waveAmplitude = 10.0f;

        s.startScaleMin = 0.14f; s.startScaleMax = 0.28f;
        s.endScaleMin   = 0.05f; s.endScaleMax   = 0.10f;
        s.spinSpeedMin = -25.0f; s.spinSpeedMax = 25.0f;

        s.use3PhaseColor = true;
        s.startColor[0]  = 1.00f; s.startColor[1]  = 0.95f; s.startColor[2]  = 0.75f; s.startColor[3]  = 0.00f;
        s.middleColor[0] = 1.00f; s.middleColor[1]  = 0.97f; s.middleColor[2]  = 0.85f; s.middleColor[3]  = 0.55f;
        s.endColor[0]    = 1.00f; s.endColor[1]    = 1.00f; s.endColor[2]    = 1.00f; s.endColor[3]    = 0.00f;

        s.fadeInTime = 0.60f; s.fadeOutTime = 1.00f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    // арканные вихри, медленно кружащиеся вокруг кнопки
    static ParticleSettings& getButtonArcaneSwirlPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 20.0f; s.maxParticles = 60;
        s.lifetimeMin = 1.5f; s.lifetimeMax = 2.5f;
        s.spawnRadius = 70.0f;  // радиус орбиты — под ширину кнопки
        s.spawnAreaCx = 0.0f; s.spawnAreaCy = 0.0f;

        s.speedMin = 10.0f; s.speedMax = 25.0f;
        s.angleMin = 0.0f; s.angleMax = 360.0f;

        s.radialAccel = -10.0f;
        s.tangentialAccel = 130.0f;

        s.startScaleMin = 0.18f; s.startScaleMax = 0.34f;
        s.endScaleMin   = 0.04f; s.endScaleMax   = 0.08f;
        s.spinSpeedMin = -120.0f; s.spinSpeedMax = 120.0f;

        s.use3PhaseColor = true;
        s.startColor[0]  = 0.55f; s.startColor[1]  = 0.20f; s.startColor[2]  = 1.00f; s.startColor[3]  = 0.00f;
        s.middleColor[0] = 0.20f; s.middleColor[1]  = 0.80f; s.middleColor[2]  = 1.00f; s.middleColor[3]  = 0.70f;
        s.endColor[0]    = 0.70f; s.endColor[1]    = 0.20f; s.endColor[2]    = 1.00f; s.endColor[3]    = 0.00f;

        s.fadeInTime = 0.30f; s.fadeOutTime = 0.50f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    // милые сердечки, всплывающие над кнопкой
    static ParticleSettings& getButtonHeartPuffPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 2.5f; s.maxParticles = 12;
        s.lifetimeMin = 1.8f; s.lifetimeMax = 2.8f;
        s.spawnRadius = 0.0f;
        s.spawnAreaCx = 160.0f; s.spawnAreaCy = 8.0f;

        s.speedMin = 40.0f; s.speedMax = 80.0f;
        s.angleMin = 240.0f; s.angleMax = 300.0f;
        s.gravityY = -25.0f;

        s.waveFrequency = 2.2f; s.waveAmplitude = 14.0f;
        s.spinSpeedMin = -20.0f; s.spinSpeedMax = 20.0f;

        s.startScaleMin = 0.06f; s.startScaleMax = 0.11f;
        s.endScaleMin   = 0.10f; s.endScaleMax   = 0.17f;

        s.use3PhaseColor = true;
        s.startColor[0]  = 1.00f; s.startColor[1]  = 0.55f; s.startColor[2]  = 0.70f; s.startColor[3]  = 0.00f;
        s.middleColor[0] = 1.00f; s.middleColor[1]  = 0.65f; s.middleColor[2]  = 0.80f; s.middleColor[3]  = 0.85f;
        s.endColor[0]    = 1.00f; s.endColor[1]    = 0.80f; s.endColor[2]    = 0.90f; s.endColor[3]    = 0.00f;

        s.fadeInTime = 0.25f; s.fadeOutTime = 0.60f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    // ледяное дыхание: бледно-голубая дымка над кнопкой
    static ParticleSettings& getButtonFrostMistPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 6.0f; s.maxParticles = 20;
        s.lifetimeMin = 2.5f; s.lifetimeMax = 4.0f;
        s.spawnRadius = 0.0f;
        s.spawnAreaCx = 240.0f; s.spawnAreaCy = 12.0f;

        s.speedMin = 5.0f; s.speedMax = 15.0f;
        s.angleMin = 268.0f; s.angleMax = 272.0f;
        s.turbulence = 3.0f;
        s.spinSpeedMin = -8.0f; s.spinSpeedMax = 8.0f;

        s.startScaleMin = 0.50f; s.startScaleMax = 0.90f;
        s.endScaleMin   = 1.20f; s.endScaleMax   = 1.80f;

        s.use3PhaseColor = true;
        s.startColor[0]  = 0.75f; s.startColor[1]  = 0.90f; s.startColor[2]  = 1.00f; s.startColor[3]  = 0.00f;
        s.middleColor[0] = 0.75f; s.middleColor[1]  = 0.90f; s.middleColor[2]  = 1.00f; s.middleColor[3]  = 0.16f;
        s.endColor[0]    = 0.70f; s.endColor[1]    = 0.85f; s.endColor[2]    = 1.00f; s.endColor[3]    = 0.00f;

        s.fadeInTime = 0.80f; s.fadeOutTime = 1.20f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    // тёмный дымок для «зловещих» кнопок
    static ParticleSettings& getButtonShadowWispPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 5.0f; s.maxParticles = 18;
        s.lifetimeMin = 2.0f; s.lifetimeMax = 3.5f;
        s.spawnRadius = 0.0f;
        s.spawnAreaCx = 200.0f; s.spawnAreaCy = 10.0f;

        s.speedMin = 8.0f; s.speedMax = 20.0f;
        s.angleMin = 265.0f; s.angleMax = 275.0f;
        s.turbulence = 5.0f;
        s.spinSpeedMin = -15.0f; s.spinSpeedMax = 15.0f;

        s.startScaleMin = 0.40f; s.startScaleMax = 0.70f;
        s.endScaleMin   = 0.90f; s.endScaleMax   = 1.40f;

        s.use3PhaseColor = true;
        s.startColor[0]  = 0.25f; s.startColor[1]  = 0.12f; s.startColor[2]  = 0.35f; s.startColor[3]  = 0.00f;
        s.middleColor[0] = 0.15f; s.middleColor[1]  = 0.05f; s.middleColor[2]  = 0.22f; s.middleColor[3]  = 0.28f;
        s.endColor[0]    = 0.05f; s.endColor[1]    = 0.02f; s.endColor[2]    = 0.10f; s.endColor[3]    = 0.00f;

        s.fadeInTime = 0.60f; s.fadeOutTime = 1.00f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    // разноцветные звёздочки, вьющиеся спиралью над кнопкой
    static ParticleSettings& getButtonStarTrailPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 18.0f; s.maxParticles = 70;
        s.lifetimeMin = 1.2f; s.lifetimeMax = 2.0f;
        s.spawnRadius = 0.0f;
        s.spawnAreaCx = 220.0f; s.spawnAreaCy = 8.0f;

        s.speedMin = 30.0f; s.speedMax = 60.0f;
        s.angleMin = 264.0f; s.angleMax = 276.0f;
        s.gravityY = -10.0f;

        s.waveFrequency = 2.5f; s.waveAmplitude = 20.0f;
        s.spinSpeedMin = -90.0f; s.spinSpeedMax = 90.0f;

        s.startScaleMin = 0.08f; s.startScaleMax = 0.16f;
        s.endScaleMin   = 0.02f; s.endScaleMax   = 0.05f;

        s.useFlicker = true;
        s.flickerSpeedMin = 2.0f; s.flickerSpeedMax = 7.0f;
        s.flickerMinAlpha = 0.25f; s.flickerMaxAlpha = 1.0f;

        s.startColor[0] = 1.00f; s.startColor[1] = 1.00f; s.startColor[2] = 1.00f; s.startColor[3] = 0.00f;
        s.endColor[0]   = 0.90f; s.endColor[1]   = 0.90f; s.endColor[2]   = 1.00f; s.endColor[3]   = 0.00f;

        s.colorJitter = 0.35f;
        s.fadeInTime = 0.20f; s.fadeOutTime = 0.45f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    // лепестки сакуры, дрейфующие поперёк кнопки
    static ParticleSettings& getButtonSakuraDriftPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 4.0f; s.maxParticles = 25;
        s.lifetimeMin = 3.0f; s.lifetimeMax = 5.0f;
        s.spawnRadius = 0.0f;
        s.spawnAreaCx = 260.0f; s.spawnAreaCy = 30.0f;

        s.speedMin = 20.0f; s.speedMax = 45.0f;
        s.angleMin = 70.0f; s.angleMax = 110.0f;
        s.gravityX = 8.0f; s.gravityY = 12.0f;

        s.flutterSpeedMin = 2.0f; s.flutterSpeedMax = 4.0f;
        s.flutterSlip = 25.0f;
        s.waveFrequency = 1.5f; s.waveAmplitude = 20.0f;
        s.spinSpeedMin = -60.0f; s.spinSpeedMax = 60.0f;

        s.startScaleMin = 0.10f; s.startScaleMax = 0.18f;
        s.endScaleMin   = 0.08f; s.endScaleMax   = 0.14f;

        s.use3PhaseColor = true;
        s.startColor[0]  = 1.00f; s.startColor[1]  = 0.85f; s.startColor[2]  = 0.90f; s.startColor[3]  = 0.00f;
        s.middleColor[0] = 1.00f; s.middleColor[1]  = 0.82f; s.middleColor[2]  = 0.88f; s.middleColor[3]  = 0.95f;
        s.endColor[0]    = 1.00f; s.endColor[1]    = 0.75f; s.endColor[2]    = 0.82f; s.endColor[3]    = 0.00f;

        s.fadeInTime = 0.40f; s.fadeOutTime = 0.80f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    // фиолетовое пламя, облизывающее верхний край кнопки
    static ParticleSettings& getButtonVioletFlamePreset()
    {
        static ParticleSettings s;
        s.spawnRate = 40.0f; s.maxParticles = 90;
        s.lifetimeMin = 0.7f; s.lifetimeMax = 1.2f;
        s.spawnRadius = 0.0f;
        s.spawnAreaCx = 200.0f; s.spawnAreaCy = 6.0f;

        s.speedMin = 50.0f; s.speedMax = 110.0f;
        s.angleMin = 262.0f; s.angleMax = 278.0f;
        s.gravityY = -20.0f;
        s.turbulence = 10.0f;

        s.startScaleMin = 0.25f; s.startScaleMax = 0.45f;
        s.endScaleMin   = 0.05f; s.endScaleMax   = 0.12f;

        s.useFlicker = true;
        s.flickerSpeedMin = 4.0f; s.flickerSpeedMax = 9.0f;
        s.flickerMinAlpha = 0.30f; s.flickerMaxAlpha = 1.0f;

        s.use3PhaseColor = true;
        s.startColor[0]  = 0.75f; s.startColor[1]  = 0.30f; s.startColor[2]  = 1.00f; s.startColor[3]  = 0.90f;
        s.middleColor[0] = 0.90f; s.middleColor[1]  = 0.10f; s.middleColor[2]  = 0.95f; s.middleColor[3]  = 0.75f;
        s.endColor[0]    = 0.20f; s.endColor[1]    = 0.00f; s.endColor[2]    = 0.40f; s.endColor[3]    = 0.00f;

        s.fadeInTime = 0.10f; s.fadeOutTime = 0.35f;

        s.blendMode = eSpriteBlendMode::ADDITIVE;
        return s;
    }

    // кровавый туман: тяжёлые багровые вихри, вскипающие алыми
    // вспышками; медленно выползает из-под кнопки и разрастается
    static ParticleSettings& getButtonBloodFogPreset()
    {
        static ParticleSettings s;
        s.spawnRate = 9.0f; s.maxParticles = 26;
        s.lifetimeMin = 3.0f; s.lifetimeMax = 5.0f;
        s.spawnRadius = 0.0f;
        s.spawnAreaCx = 220.0f; s.spawnAreaCy = 8.0f;

        s.speedMin = 6.0f; s.speedMax = 18.0f;
        s.angleMin = 264.0f; s.angleMax = 276.0f;
        s.gravityY = -4.0f;
        s.turbulence = 6.0f;
        s.drag = 0.20f;

        s.waveFrequency = 0.9f; s.waveAmplitude = 12.0f;
        s.spinSpeedMin = -18.0f; s.spinSpeedMax = 18.0f;

        s.startScaleMin = 0.55f; s.startScaleMax = 0.95f;
        s.endScaleMin   = 1.70f; s.endScaleMax   = 2.50f;

        // вспышки-«вскипания» внутри тумана
        s.useFlicker = true;
        s.flickerSpeedMin = 0.7f; s.flickerSpeedMax = 2.4f;
        s.flickerMinAlpha = 0.05f; s.flickerMaxAlpha = 0.85f;

        // почти чёрная марь -> густая багровая -> угли
        s.use3PhaseColor = true;
        s.startColor[0]  = 0.16f; s.startColor[1]  = 0.01f; s.startColor[2]  = 0.02f; s.startColor[3]  = 0.00f;
        s.middleColor[0] = 0.62f; s.middleColor[1]  = 0.03f; s.middleColor[2]  = 0.06f; s.middleColor[3]  = 0.38f;
        s.endColor[0]    = 0.10f; s.endColor[1]    = 0.00f; s.endColor[2]    = 0.01f; s.endColor[3]    = 0.00f;

        s.colorJitter = -0.15f;

        s.fadeInTime = 0.90f; s.fadeOutTime = 1.40f;

        s.blendMode = eSpriteBlendMode::NORMAL;
        return s;
    }

    static ParticleSettings& getPreset(eParticlePreset preset)
{

    switch (preset)
    {
        case eParticlePreset::STARFIELD:          return getStarfieldPreset();
        case eParticlePreset::RAIN_SPLASH:        return getRainSplashPreset();
        case eParticlePreset::FIRE:               return getFirePreset();
        case eParticlePreset::SMOKE:              return getSmokePreset();
        case eParticlePreset::AMBIENT_DUST:       return getAmbientDustPreset();
        case eParticlePreset::RAIN:               return getRainPreset();
        case eParticlePreset::SNOW:               return getSnowPreset();
        case eParticlePreset::FALLING_LEAVES:     return getFallingLeavesPreset();
        case eParticlePreset::BUBBLES:            return getBubblesPreset();
        case eParticlePreset::SPARKS:             return getSparksPreset();
        case eParticlePreset::BLOOD_SPLATTER:     return getBloodSplatterPreset();
        case eParticlePreset::DEBRIS:             return getDebrisPreset();
        case eParticlePreset::PLASMA_TRAIL:       return getPlasmaTrailPreset();
        case eParticlePreset::HEALING:            return getHealingPreset();
        case eParticlePreset::POISON:             return getPoisonPreset();
        case eParticlePreset::PIXIE_DUST:         return getPixieDustPreset();
        case eParticlePreset::SHADOW_VOID:        return getShadowVoidPreset();
        case eParticlePreset::STEAM:              return getSteamPreset();
        case eParticlePreset::ELECTRIC_SPARKS:    return getElectricSparksPreset();
        case eParticlePreset::ACID_DRIPS:         return getAcidDripsPreset();
        case eParticlePreset::FIREWORKS:          return getFireworksPreset();
        case eParticlePreset::CONFETTI:           return getConfettiPreset();
        case eParticlePreset::UI_BUTTON_GLOW:     return getUIButtonGlowPreset();
        case eParticlePreset::NEBULA:             return getNebulaPreset();
        case eParticlePreset::HYPERSPACE:         return getHyperspacePreset();
        case eParticlePreset::ADVANCED_SNOW:      return getAdvancedSnowPreset();
        case eParticlePreset::ADVANCED_SPARKS:    return getAdvancedSparksPreset();
        case eParticlePreset::ROCKET_TRAIL:       return getRocketTrailPreset();
        case eParticlePreset::PLASMA_BALL:        return getPlasmaBallPreset();
        case eParticlePreset::FLAMETHROWER:       return getFlamethrowerPreset();
        case eParticlePreset::STARLIGHT_FOUNTAIN: return getStarlightFountainPreset();
        case eParticlePreset::ETHEREAL_WISP:      return getEtherealWispPreset();
        case eParticlePreset::LASER_SPARKS:       return getLaserSparksPreset();
        case eParticlePreset::RAILGUN_TRAIL:      return getRailgunTrailPreset();
        case eParticlePreset::CORE_MELTDOWN:      return getCoreMeltdownPreset();
        case eParticlePreset::SANDSTORM:          return getSandstormPreset();
        case eParticlePreset::SAKURA:             return getSakuraPreset();
        case eParticlePreset::GRENADE_EXPLOSION:  return getGrenadeExplosionPreset();
        case eParticlePreset::MECH_EXPLOSION:     return getMechExplosionPreset();
        case eParticlePreset::COSMIC_NOVA:        return getCosmicNovaPreset();
        case eParticlePreset::HOLY_BURST:         return getHolyBurstPreset();
        case eParticlePreset::CRITICAL_BLOOD:     return getCriticalBloodPreset();
        case eParticlePreset::PROP_SMASH:         return getPropSmashPreset();
        case eParticlePreset::ICE_NOVA:           return getIceNovaPreset();
        case eParticlePreset::TOXIC_SPORE:        return getToxicSporePreset();
        case eParticlePreset::FIREFLIES:          return getFirefliesPreset();
        case eParticlePreset::DANDELION_FLUFF:    return getDandelionFluffPreset();
        case eParticlePreset::SUNBEAM_DUST:       return getSunbeamDustPreset();
        case eParticlePreset::LAKE_MIST:          return getLakeMistPreset();
        case eParticlePreset::CUP_STEAM:          return getCupSteamPreset();
        case eParticlePreset::AETHER_WISPS:       return getAetherWispsPreset();
        case eParticlePreset::GLOWING_SPORES:     return getGlowingSporesPreset();
        case eParticlePreset::PLANKTON:           return getPlanktonPreset();
        case eParticlePreset::DISTANT_EMBERS:     return getDistantEmbersPreset();
        case eParticlePreset::BOKEH_AMBIENT:      return getBokehAmbientPreset();

        case eParticlePreset::BUTTON_GOLD_SPARKLE: return getButtonGoldSparklePreset();
        case eParticlePreset::BUTTON_EMBER_RISE:   return getButtonEmberRisePreset();
        case eParticlePreset::BUTTON_HOLY_AURA:    return getButtonHolyAuraPreset();
        case eParticlePreset::BUTTON_ARCANE_SWIRL: return getButtonArcaneSwirlPreset();
        case eParticlePreset::BUTTON_HEART_PUFF:   return getButtonHeartPuffPreset();
        case eParticlePreset::BUTTON_FROST_MIST:   return getButtonFrostMistPreset();
        case eParticlePreset::BUTTON_SHADOW_WISP:  return getButtonShadowWispPreset();
        case eParticlePreset::BUTTON_STAR_TRAIL:   return getButtonStarTrailPreset();
        case eParticlePreset::BUTTON_SAKURA_DRIFT: return getButtonSakuraDriftPreset();
        case eParticlePreset::BUTTON_VIOLET_FLAME: return getButtonVioletFlamePreset();
        case eParticlePreset::BUTTON_BLOOD_FOG:    return getButtonBloodFogPreset();

        default:
            assert(false);
            return getFirePreset();
    }
}

static const char* getPresetSpriteName(eParticlePreset preset)
{
    switch (preset)
    {
        case eParticlePreset::STARFIELD:          return "star_01";

        case eParticlePreset::FIRE:               return "fire_02";
        case eParticlePreset::FLAMETHROWER:       return "fire_02";
        case eParticlePreset::SPARKS:
        case eParticlePreset::ELECTRIC_SPARKS:    return "circle_05";
        case eParticlePreset::ADVANCED_SPARKS:    return "circle_05";
        case eParticlePreset::LASER_SPARKS:       return "light_01";
        case eParticlePreset::DISTANT_EMBERS:     return "dirt_02";

        case eParticlePreset::SMOKE:
        case eParticlePreset::ROCKET_TRAIL:       return "smoke_04";
        case eParticlePreset::STEAM:
        case eParticlePreset::CUP_STEAM:          return "circle_05";
        case eParticlePreset::LAKE_MIST:
        case eParticlePreset::NEBULA:             return "smoke_08";

        case eParticlePreset::RAIN:               return "circle_05";
        case eParticlePreset::SNOW:
        case eParticlePreset::ADVANCED_SNOW:      return "circle_05";
        case eParticlePreset::FALLING_LEAVES:     return "leaf";
        case eParticlePreset::SAKURA:             return "star_09";
        case eParticlePreset::DANDELION_FLUFF:    return "trace_01";
        case eParticlePreset::PROP_SMASH:
        case eParticlePreset::DEBRIS:             return "dirt_01";
        case eParticlePreset::SANDSTORM:          return "dirt_03";

        case eParticlePreset::BUBBLES:            return "circle_04";
        case eParticlePreset::BLOOD_SPLATTER:
        case eParticlePreset::CRITICAL_BLOOD:     return "circle_05";
        case eParticlePreset::TOXIC_SPORE:
        case eParticlePreset::GLOWING_SPORES:     return "flare_01";
        case eParticlePreset::POISON:             return "smoke_07";
        case eParticlePreset::PLANKTON:           return "circle_04";

        case eParticlePreset::HEALING:            return "symbol_01";
        case eParticlePreset::HOLY_BURST:         return "star_02";
        case eParticlePreset::PIXIE_DUST:
        case eParticlePreset::STARLIGHT_FOUNTAIN: return "star_01";
        case eParticlePreset::FIREFLIES:
        case eParticlePreset::SUNBEAM_DUST:
        case eParticlePreset::AMBIENT_DUST:       return "circle_05";
        case eParticlePreset::BOKEH_AMBIENT:      return "circle_04";
        case eParticlePreset::UI_BUTTON_GLOW:     return "circle_02";

        case eParticlePreset::PLASMA_TRAIL:
        case eParticlePreset::PLASMA_BALL:        return "fire_02";
        case eParticlePreset::HYPERSPACE:         return "light_02";
        case eParticlePreset::RAILGUN_TRAIL:      return "light_01";
        case eParticlePreset::SHADOW_VOID:        return "twirl_03";
        case eParticlePreset::ETHEREAL_WISP:      return "trace_01";
        case eParticlePreset::FIREWORKS:          return "spark_02";
        case eParticlePreset::CONFETTI:           return "star_07";
        case eParticlePreset::GRENADE_EXPLOSION:  return "smoke_04";
        case eParticlePreset::MECH_EXPLOSION:     return "scorch_01";
        case eParticlePreset::COSMIC_NOVA:        return "circle_01";
        case eParticlePreset::CORE_MELTDOWN:      return "twirl_02";
        case eParticlePreset::ICE_NOVA:           return "star_08";
        case eParticlePreset::RAIN_SPLASH:        return "circle_05";

        case eParticlePreset::BUTTON_GOLD_SPARKLE: return "star_01";
        case eParticlePreset::BUTTON_EMBER_RISE:   return "dirt_02";
        case eParticlePreset::BUTTON_HOLY_AURA:    return "symbol_01";
        case eParticlePreset::BUTTON_ARCANE_SWIRL: return "twirl_03";
        case eParticlePreset::BUTTON_HEART_PUFF:   return "heart";
        case eParticlePreset::BUTTON_FROST_MIST:   return "light_02";
        case eParticlePreset::BUTTON_SHADOW_WISP:  return "smoke_08";
        case eParticlePreset::BUTTON_STAR_TRAIL:   return "star_06";
        case eParticlePreset::BUTTON_SAKURA_DRIFT: return "sakura1";
        case eParticlePreset::BUTTON_VIOLET_FLAME: return "fire_01";
        case eParticlePreset::BUTTON_BLOOD_FOG:    return "smoke_07";

        default:
            return "circle_05";
    }
}

};

_G2D_NAMESPACE_END_
