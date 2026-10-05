#pragma once

#include "basedialog.h"
#include "spriteloader.h"
#include "gfx.h"
#include "buttonsliced.h"
#include "engine.h"
#include "button.h"
#include "particle.h"
#include "checkbox.h"
#include "cardreveal.h"
#include "particlepresets.h"
#include "dialogparticlesview.h"
#include "l10n.h"

#include <string>

class PanelSpriteContDemo : public BaseDialog
{
public:
    void createBgParticles()
    {
        static ParticleSettings s;
        s.spawnRate = 1.5f; s.maxParticles = 135;
        s.lifetimeMin = 6.0f * 10; s.lifetimeMax = 8.0f * 10;
        s.spawnRadius = 450.0f;
        s.speedMin = 30.0f; s.speedMax = 50.0f;
        s.angleMin = 80.0f; s.angleMax = 100.0f;
        s.gravityX = -3.0f;
        s.gravityY = 25.0f;
        s.startScaleMin = 0.6f; s.startScaleMax = 1.1f;
        s.endScaleMin = 0.5f; s.endScaleMax = 1.0f;
        s.startRotationMin = 0.0f; s.startRotationMax = 360.0f;
        s.spinSpeedMin = -45.0f; s.spinSpeedMax = 45.0f;
        s.waveFrequency = 1.6f;
        s.waveAmplitude = 25.0f * 10.f;
        s.use3PhaseColor = true;
        s.startColor[0] = 0.9f; s.startColor[1] = 0.5f; s.startColor[2] = 0.1f; s.startColor[3] = 1.0f;
        s.middleColor[0] = 0.5f; s.middleColor[1] = .2f; s.middleColor[2] = .0f; s.middleColor[3] = 1.f;
        s.endColor[0]   = 0.5f; s.endColor[1]   = 0.2f; s.endColor[2]   = 0.0f; s.endColor[3]   = 0.0f;
        s.useDepth      = true;
        s.depthMin      = 0.35f;
        s.depthScaleMin = 0.40f;
        s.depthSpeedMin = 0.35f;
        s.depthAlphaMin = 0.50f;
        s.blendMode = eSpriteBlendMode::NORMAL;

        std::vector<const char*> vLeaves{"PARTICLES/aleaf_01", "PARTICLES/aleaf_02", "PARTICLES/aleaf_03", "PARTICLES/aleaf_04"};
        auto ptrLeaves = std::make_shared<CParticleSystem<>>(s, vLeaves);
        float cx = getDialogCx();
        ptrLeaves->setPos(cx * 0.5f, -150.f);
        _root->addChild(ptrLeaves);

        auto ptrRain = std::make_shared<CParticleSystem<>>(
            ParticlePresets::getRainPreset(),
            std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::RAIN)).c_str());
        ptrRain->setPos(cx * 0.5f, -40.f);
        ptrRain->getSettings().floorY = 4680;        // земля в локальных координатах эмиттера
        ptrRain->getSettings().lifetimeMax *= 10.f;
        ptrRain->getSettings().lifetimeMin *= 10.f;
        ptrRain->getSettings().gravityX = 10;

        auto ptrSplash = std::make_shared<CParticleSystem<>>(
            ParticlePresets::getRainSplashPreset(),
            std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::RAIN_SPLASH)).c_str());
        ptrSplash->setPos(cx * 0.5f, -40.f);             // тот же якорь, что у дождя

        // Саб-эмиттер: капля умерла о пол -> всплеск в точке удара
        ptrRain->setOnParticleDeath([ptrSplash](float wx, float wy, bool bFloor)
        {
            if (bFloor)
                ptrSplash->burstAtWorld(wx, wy, 3);
        });

        _root->addChild(ptrRain);
        _root->addChild(ptrSplash);
    }

    void init(float cx, float cy, CContainerPtr ptrParent)
    {
        Rect rcScroll{ 0.f, 30.f, cx - 40.f, cy - 40.f };
        BaseDialog::init(cx, cy, eScrollType::E_ST_VERT, &rcScroll, nullptr, ptrParent.get());

        auto ptrSolid = SpriteLoader::getInstance()->getSprite("UI/nineslicedemo4");
        const float left   = 40.f;
        const float innerW = cx - 80.f;
        float y = 20.f;

        auto ptrMove = std::make_shared<CContainer>();

        auto makeHeader = [&](const char* szText)
        {
            static auto ptrSpr = SpriteLoader::getInstance()->getSprite("UI/threeSliceNice");
            auto ptrThree = std::make_shared<ThreeSliceHor>();
            ptrThree->createSlices(ptrSpr, 165, 165);

            auto p = std::make_shared<StaticLabel>();
            p->setFont("condence");
            p->setFontSize(34);
            p->setRgba(0xFFE3A3FF);
            p->setText(szText);

            ptrThree->build(p->calcNotTransCx() + 300);
            p->setPosCentered(ptrThree->calcNotTransCx(), ptrThree->calcNotTransCy());
            p->setY(p->getY() - 30);

            ptrThree->addChild(p);
            ptrThree->setX(left);
            ptrThree->setY(y);
            ptrMove->addChild(ptrThree);

            y += 110.f;
            return ptrThree;
        };

        auto makeFrame = [&](float h)
        {
            auto p = std::make_shared<NineSlice>();
            p->createSlices(ptrSolid, 127, 127, 127, 127);
            p->build(innerW, h);
            p->setPos(left, y - 20);
            p->setAlpha(1.f);
            ptrMove->addChild(p);
            return p;
        };

        auto makeLabel = [&](CContainerPtr ptrTo, const char* szText, float lx, float ly, int size, uint32_t col)
        {
            auto p = std::make_shared<StaticLabel>();
            p->setFont("condence");
            p->setFontSize(size);
            p->setRgba(col);
            p->setText(szText);
            p->setPos(lx, ly);
            ptrTo->addChild(p);
            return p;
        };

        createBgParticles();

        // setPivotCentered() ставит визуальный центр в pos + (W/2,H/2) —
        // компенсируем, чтобы центр спрайта попал ровно в (px,py).
        auto placeCenter = [](CContainerPtr spr, float px, float py)
        {
            spr->setPos(px - spr->getNotTransCx() * 0.5f,
                        py - spr->getNotTransCy() * 0.5f);
        };

        static const char* planetNames[3] = { "PLANETS/planet1", "PLANETS/planet2", "PLANETS/planet3" };

        // Геометрия секций SceneGraph: двухколоночная раскладка
        const float SEC_H   = 640.f;
        const float PAD_X   = 80.f;
        const float SCENE_Y = 80.f;                 // ниже верхнего орнамента
        const float SCENE_H = SEC_H - SCENE_Y * 2.f; // 480 — рабочая высота
        const float BTN_H   = 46.f;
        const float COL_TXT = 0.38f;                // доля ширины под текст

        // =====================================================================
        // СЕКЦИЯ A: вращение родителя
        // =====================================================================
        auto pHeaderA = makeHeader(L10N::getInstance().tr("SG_A_HEADER"));
        {
            auto frame = makeFrame(SEC_H);
            auto ptrScene = std::make_shared<CContainer>();
            frame->addChild(ptrScene);
            ptrScene->setPos(PAD_X, SCENE_Y);

            const float sceneW = innerW - PAD_X * 2.f;
            const float txtW   = sceneW * COL_TXT;
            const float demoX0 = txtW + 40.f;
            const float demoW  = sceneW - txtW - 40.f;
            const float demoCX = demoX0 + demoW * 0.5f;
            const float demoCY = SCENE_H * 0.5f;

            // ---- ЛЕВАЯ КОЛОНКА: текст ----
            makeLabel(ptrScene, L10N::getInstance().tr("SG_A_TITLE"), 0.f, 0.f, 36, 0xC8B898FF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_A_LINE1"), 0.f, 70.f, 24, 0x9A9AAAFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_A_LINE2"), 0.f, 104.f, 24, 0x9A9AAAFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_A_LINE3"), 0.f, 138.f, 24, 0x9A9AAAFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_A_LINE4"), 0.f, 186.f, 24, 0x8A8A9AFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_A_LINE5"), 0.f, 220.f, 24, 0x8A8A9AFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_A_LINE6"), 0.f, 268.f, 24, 0x8A8A9AFF);

            // ---- ПРАВАЯ КОЛОНКА: демо ----
            auto ptrPixie = std::make_shared<CParticleSystem<>>(
                ParticlePresets::getPreset(eParticlePreset::GLOWING_SPORES),
                std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::GLOWING_SPORES)).c_str());
            ptrPixie->setPos(demoCX, demoCY);
            ptrScene->addChild(ptrPixie);

            std::vector<const char*> vStarNames{"PARTICLES/circle_06", "PARTICLES/circle_06", "PARTICLES/circle_06", "PARTICLES/star_01", "PARTICLES/circle_06"};
            auto ptrStars = std::make_shared<CParticleSystem<>>(
                ParticlePresets::getPreset(eParticlePreset::STARFIELD),
                vStarNames);
            ptrStars->setPos(demoCX, demoCY);

            ptrStars->setCustomModifier([](Particle& p, float dt)
            {
                // Используем wavePhaseOffset как уникальный случайный маркер.
                // Он бывает от 0 до 100.
                //
                // 2.5 означает примерно 2.5% звёзд будут "умирающими".
                // Если хочется ещё реже — ставь 1.0 или 0.5.
                //
                const float rareThreshold = 72.5f;
                if (p.wavePhaseOffset >= rareThreshold)
                    return;

                // При рождении делаем таким звёздам короткую жизнь
                if (p.age < 0.2f)
                {
                    float k = p.wavePhaseOffset / rareThreshold;
                    p.lifetime = 5.0f + k * 4.0f; // 5..9 секунд
                }

                float safeLifetime = p.lifetime;
                if (safeLifetime < 0.001f)
                    safeLifetime = 0.001f;

                float lifeK = p.simAge / safeLifetime;

                // Последние 28% жизни: вращаемся, уменьшаемся и гаснем
                if (lifeK > 0.72f)
                {
                    float dieK = (lifeK - 0.72f) / 0.28f;
                    if (dieK > 1.0f)
                        dieK = 1.0f;

                    // Уменьшение
                    float shrink = 1.0f - dieK;
                    if (shrink < 0.0f)
                        shrink = 0.0f;
                    p.currentScale *= shrink;

                    // Вращение.
                    // p.rotation в радианах.
                    // 1.2..4.7 рад/с — это довольно заметно, но не бешено.
                    p.rotation += dt * (1.2f + 3.5f * dieK);

                    // Дополнительное угасание
                    float alphaK = 1.0f - dieK * 0.75f;
                    if (alphaK < 0.0f)
                        alphaK = 0.0f;
                    p.color[3] *= alphaK;
                }
            });

            ptrStars->prewarm(3);
            ptrScene->addChild(ptrStars);

            auto ptrSolar = std::make_shared<CContainer>();   // пивот (0,0) = центр орбит
            ptrSolar->setPos(demoCX, demoCY);

            auto sun = SpriteLoader::getInstance()->getSprite("PLANETS/sun");
            sun->setPivotCentered();
            placeCenter(sun, 0.f, 0.f);
            sun->addSelfTweenEx(eTweenProp::ROTATE, 0.f, 6.28318f, 10.f, Easing::linear, 0.f, {}, nullptr, eTweenLoopMode::REPEAT, -1);
            ptrSolar->addChild(sun);

            for (int i = 0; i < 3; ++i)
            {
                auto p = SpriteLoader::getInstance()->getSprite(planetNames[i]);
                p->setPivotCentered(); //p->setScale(.34f, .34f);
                float ang = (6.28318f / 3.f) * (float)i;
                placeCenter(p, cosf(ang) * 150.f, sinf(ang) * 150.f);
                p->addSelfTweenEx(eTweenProp::ROTATE, 0.f, -6.28318f, 2.f + 0.7f * i, Easing::linear, 0.f, {}, nullptr, eTweenLoopMode::REPEAT, -1);
                ptrSolar->addChild(p);

                if (i == 0)
                {
                    auto moon = SpriteLoader::getInstance()->getSprite("PLANETS/moon");
                    moon->setPivotCentered(); //moon->setScale(.45f, .45f);   // относительно планеты
                    placeCenter(moon, 45.f, 0.f);
                    moon->addSelfTweenEx(eTweenProp::ROTATE, 0.f, 6.28318f, 1.2f, Easing::linear, 0.f, {}, nullptr, eTweenLoopMode::REPEAT, -1);
                    p->addChild(moon);
                }
            }

            ptrSolar->addSelfTweenEx(eTweenProp::ROTATE, 0.f, 6.28318f, 18.f, Easing::linear, 0.f, {}, nullptr, eTweenLoopMode::REPEAT, -1);
            ptrScene->addChild(ptrSolar);

            // ---- КНОПКИ: низ левой колонки, внутри фрейма ----
            auto ptrBtns = std::make_shared<CContainer>();
            ptrBtns->setPos(0.f, SCENE_H - BTN_H - 10.f);

            auto bStop = CyanSlicedButton::makeInst(BTN_H, L10N::getInstance().tr("BTN_STOP"), [ptrSolar]
            {
                ptrSolar->removeSelfTweens();
            });

            auto bPlay = GreenSlicedButton::makeInst(BTN_H, L10N::getInstance().tr("BTN_START"), [ptrSolar]
            {
                ptrSolar->removeSelfTweens();
                ptrSolar->addSelfTweenEx(eTweenProp::ROTATE, ptrSolar->getRotate(), ptrSolar->getRotate() + 6.28318f, 18.f, Easing::linear, 0.f, {}, nullptr, eTweenLoopMode::REPEAT, -1);
            });

            auto bRev = OrangeSlicedButton::makeInst(BTN_H, L10N::getInstance().tr("BTN_BACK"), [ptrSolar]
            {
                ptrSolar->removeSelfTweens();
                ptrSolar->addSelfTweenEx(eTweenProp::ROTATE, ptrSolar->getRotate(), ptrSolar->getRotate() - 6.28318f, 18.f, Easing::linear, 0.f, {}, nullptr, eTweenLoopMode::REPEAT, -1);
            });

            ptrBtns->addChild(bStop); ptrBtns->addChild(bPlay); ptrBtns->addChild(bRev);
            ptrBtns->alignChildren(eChildrenAlign::HORIZONTAL, 10.f, txtW + 120.f);
            ptrScene->addChild(ptrBtns);

            y += SEC_H + 70.f;
            pHeaderA->setX(100);
        }

        // =====================================================================
        // СЕКЦИЯ B: масштабирование родителя
        // =====================================================================
        auto pHeaderB = makeHeader(L10N::getInstance().tr("SG_B_HEADER"));
        {
            auto frame = makeFrame(SEC_H);
            auto ptrScene = std::make_shared<CContainer>();
            frame->addChild(ptrScene);
            ptrScene->setPos(PAD_X, SCENE_Y);

            const float sceneW = innerW - PAD_X * 2.f;
            const float txtW   = sceneW * COL_TXT;
            const float demoX0 = txtW + 40.f;
            const float demoW  = sceneW - txtW - 40.f;
            const float demoCX = demoX0 + demoW * 0.5f;
            const float demoCY = SCENE_H * 0.5f;

            makeLabel(ptrScene, L10N::getInstance().tr("SG_B_TITLE"), 0.f, 0.f, 36, 0xC8B898FF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_B_LINE1"), 0.f, 70.f, 24, 0x9A9AAAFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_B_LINE2"), 0.f, 104.f, 24, 0x9A9AAAFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_B_LINE3"), 0.f, 138.f, 24, 0x9A9AAAFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_B_LINE4"), 0.f, 186.f, 24, 0x8A8A9AFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_B_LINE5"), 0.f, 220.f, 24, 0x8A8A9AFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_B_LINE6"), 0.f, 254.f, 24, 0x8A8A9AFF);

            auto ptrPulse = std::make_shared<CContainer>();   // пивот (0,0) = центр пульса
            ptrPulse->setPos(demoCX, demoCY);

            for (int i = 0; i < 4; ++i)
            {
                auto c = SpriteLoader::getInstance()->getSprite(planetNames[i % 3]);
                c->setPivotCentered(); c->setScale(.34f, .34f);
                float ang = (6.28318f / 4.f) * (float)i + 0.785f;
                placeCenter(c, cosf(ang) * 80.f, sinf(ang) * 80.f);
                c->addSelfTweenEx(eTweenProp::ROTATE, -0.15f, 0.15f, 2.f + 0.3f * i, Easing::inOutSine, 0.f, {}, nullptr, eTweenLoopMode::YOYO, -1, Easing::inOutSine);
                ptrPulse->addChild(c);
            }

            ptrPulse->addSelfTweenEx(eTweenProp::SCALE, 0.8f, 1.3f, 1.6f, Easing::inOutQuad, 0.f, {}, nullptr, eTweenLoopMode::YOYO, -1, Easing::inOutQuad);
            ptrScene->addChild(ptrPulse);

            auto ptrBtns = std::make_shared<CContainer>();
            ptrBtns->setPos(0.f, SCENE_H - BTN_H - 10.f);

            auto bStop = CyanSlicedButton::makeInst(BTN_H, L10N::getInstance().tr("BTN_STOP"), [ptrPulse]
            {
                ptrPulse->removeSelfTweens();
                ptrPulse->setScale(1.f, 1.f);
            });

            auto bPulse = GreenSlicedButton::makeInst(BTN_H, L10N::getInstance().tr("BTN_PULSE"), [ptrPulse]
            {
                ptrPulse->removeSelfTweens();
                ptrPulse->setScale(1.f, 1.f);
                ptrPulse->addSelfTweenEx(eTweenProp::SCALE, 0.8f, 1.3f, 1.6f, Easing::inOutQuad, 0.f, {}, nullptr, eTweenLoopMode::YOYO, -1, Easing::inOutQuad);
            });

            auto bGrow = OrangeSlicedButton::makeInst(BTN_H, L10N::getInstance().tr("BTN_GROW"), [ptrPulse]
            {
                ptrPulse->removeSelfTweens();
                ptrPulse->addSelfTween(eTweenProp::SCALE, ptrPulse->getScaleX(), 1.4f, .6f, Easing::outBack);
            });

            ptrBtns->addChild(bStop); ptrBtns->addChild(bPulse); ptrBtns->addChild(bGrow);
            ptrBtns->alignChildren(eChildrenAlign::HORIZONTAL, 10.f, txtW + 120.f);
            ptrScene->addChild(ptrBtns);

            y += SEC_H + 70.f;
            pHeaderB->setX(100);
        }

        // =====================================================================
        // СЕКЦИЯ C: трёхуровневая иерархия
        // =====================================================================
        auto pHeaderC = makeHeader(L10N::getInstance().tr("SG_C_HEADER"));
        {
            auto frame = makeFrame(SEC_H);
            auto ptrScene = std::make_shared<CContainer>();
            frame->addChild(ptrScene);
            ptrScene->setPos(PAD_X, SCENE_Y);

            const float sceneW = innerW - PAD_X * 2.f;
            const float txtW   = sceneW * COL_TXT;
            const float demoX0 = txtW + 40.f;
            const float demoW  = sceneW - txtW - 40.f;
            const float demoCX = demoX0 + demoW * 0.5f;
            const float demoCY = SCENE_H * 0.5f;

            makeLabel(ptrScene, L10N::getInstance().tr("SG_C_TITLE"), 0.f, 0.f, 36, 0xC8B898FF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_C_LINE1"), 0.f, 70.f, 24, 0x9A9AAAFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_C_LINE2"), 0.f, 104.f, 24, 0x9A9AAAFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_C_LINE3"), 0.f, 152.f, 24, 0x8A8A9AFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_C_LINE4"), 0.f, 186.f, 24, 0x8A8A9AFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_C_LINE5"), 0.f, 220.f, 24, 0x8A8A9AFF);
            makeLabel(ptrScene, L10N::getInstance().tr("SG_C_LINE6"), 0.f, 254.f, 24, 0x8A8A9AFF);

            auto ptrL1 = std::make_shared<CContainer>();      // пивот (0,0) = шарнир
            ptrL1->setPos(demoCX, demoCY);

            auto s1 = SpriteLoader::getInstance()->getSprite("CARDS/card_king");
            s1->setPivotCentered(); s1->setScale(.34f, .34f); s1->setTint(1.f, .5f, .5f);
            placeCenter(s1, 0.f, 0.f);
            ptrL1->addChild(s1);

            auto ptrL2 = std::make_shared<CContainer>();
            ptrL2->setPos(80.f, 0.f);                          // в локальных координатах L1

            auto s2 = SpriteLoader::getInstance()->getSprite("CARDS/card_queen");
            s2->setPivotCentered(); s2->setScale(.34f, .34f); s2->setTint(.5f, .7f, 1.f);
            placeCenter(s2, 0.f, 0.f);
            ptrL2->addChild(s2);

            auto ptrL3 = std::make_shared<CContainer>();
            ptrL3->setPos(60.f, 0.f);                          // в локальных координатах L2

            auto s3 = SpriteLoader::getInstance()->getSprite("CARDS/card_jack");
            s3->setPivotCentered(); s3->setScale(.34f, .34f); s3->setTint(.5f, 1.f, .5f);
            placeCenter(s3, 0.f, 0.f);
            ptrL3->addChild(s3);

            ptrL2->addChild(ptrL3);
            ptrL1->addChild(ptrL2);

            ptrL1->addSelfTweenEx(eTweenProp::ROTATE, 0.f, 6.28318f, 8.f,  Easing::linear, 0.f, {}, nullptr, eTweenLoopMode::REPEAT, -1);
            ptrL2->addSelfTweenEx(eTweenProp::ROTATE, 0.f, -6.28318f, 3.f, Easing::linear, 0.f, {}, nullptr, eTweenLoopMode::REPEAT, -1);
            ptrL3->addSelfTweenEx(eTweenProp::ROTATE, 0.f, 6.28318f, 1.5f, Easing::linear, 0.f, {}, nullptr, eTweenLoopMode::REPEAT, -1);

            ptrScene->addChild(ptrL1);

            auto ptrBtns = std::make_shared<CContainer>();
            ptrBtns->setPos(0.f, SCENE_H - BTN_H - 10.f);

            auto bStopC = CyanSlicedButton::makeInst(BTN_H, L10N::getInstance().tr("BTN_STOP_ALL"), [ptrL1, ptrL2, ptrL3]
            {
                ptrL1->removeSelfTweens(); ptrL2->removeSelfTweens(); ptrL3->removeSelfTweens();
            });

            auto bPlayC = GreenSlicedButton::makeInst(BTN_H, L10N::getInstance().tr("BTN_START"), [ptrL1, ptrL2, ptrL3]
            {
                auto restart = [](CContainerPtr p, float dir, float dur)
                {
                    p->removeSelfTweens();
                    p->addSelfTweenEx(eTweenProp::ROTATE, p->getRotate(), p->getRotate() + dir * 6.28318f, dur, Easing::linear, 0.f, {}, nullptr, eTweenLoopMode::REPEAT, -1);
                };

                restart(ptrL1, 1.f, 8.f); restart(ptrL2, -1.f, 3.f); restart(ptrL3, 1.f, 1.5f);
            });

            auto bOnlyL1 = OrangeSlicedButton::makeInst(BTN_H, L10N::getInstance().tr("BTN_ONLY_L1"), [ptrL1, ptrL2, ptrL3]
            {
                ptrL2->removeSelfTweens(); ptrL2->rotate(0.f);
                ptrL3->removeSelfTweens(); ptrL3->rotate(0.f);

                ptrL1->removeSelfTweens();
                ptrL1->addSelfTweenEx(eTweenProp::ROTATE, ptrL1->getRotate(), ptrL1->getRotate() + 6.28318f, 8.f, Easing::linear, 0.f, {}, nullptr, eTweenLoopMode::REPEAT, -1);
            });

            ptrBtns->addChild(bStopC); ptrBtns->addChild(bPlayC); ptrBtns->addChild(bOnlyL1);
            ptrBtns->alignChildren(eChildrenAlign::HORIZONTAL, 10.f, txtW + 120.f);
            ptrScene->addChild(ptrBtns);

            y += SEC_H + 70.f;
            pHeaderC->setX(100);
        }

        // ---------------- секция 1: клоны / тинты / бленд / альфа ----------------
        auto pHeader = makeHeader(L10N::getInstance().tr("SPRITE_DEMO_TINT_HEADER"));
        {
            const float h = 350.f;
            auto frame = makeFrame(h);

            CSpritePtr base = SpriteLoader::getInstance()->getSprite("CARDS/card_joker");
            auto ptrCont = std::make_shared<CContainer>();

            CSpritePtr cards[5];
            for (int i = 0; i < 5; ++i)
            {
                cards[i] = base->cloneInitial();
                cards[i]->setPivotCentered();
            }

            cards[1]->setTint(1.f, .45f, .45f);
            cards[2]->setGrayScale(true);
            cards[3]->setBlendMode(eSpriteBlendMode::ADDITIVE);
            cards[4]->setAlpha(.45f);

            for (int i = 0; i < 5; ++i)
            {
                cards[i]->setScale(.45f, .45f);
                ptrCont->addChild(cards[i]);
            }

            cards[0]->getParent()->bringChildToFront(cards[0].get());
            ptrCont->alignChildren(eChildrenAlign::HORIZONTAL, 10.f, innerW - 30.f);

            cards[0]->addSelfTweenEx(eTweenProp::ROTATE, 0.f, 6.28318f, 7.f, Easing::linear, 0.f, {}, nullptr, eTweenLoopMode::REPEAT, -1);
            cards[2]->addSelfTweenEx(eTweenProp::SCALE, .45f, .55f, 1.1f, Easing::outQuad, 0.f, {}, nullptr, eTweenLoopMode::YOYO, -1, Easing::outQuad);
            cards[4]->addSelfTweenEx(eTweenProp::ALPHA, .25f, .7f, 1.3f, Easing::linear, 0.f, {}, nullptr, eTweenLoopMode::YOYO, -1, Easing::linear);

            ptrCont->setPosCentered(frame->calcNotTransCx(), frame->calcNotTransCy());
            frame->addChild(ptrCont);

            y += h + 70.f;
            pHeader->setX(100);
        }

        // ---------------- секция 2: интерактив ----------------
        pHeader = makeHeader(L10N::getInstance().tr("SPRITE_DEMO_INTERACTIVE_HEADER"));
        {
            const float h = 350.f;
            auto frame    = makeFrame(h);
            auto ptrCont  = std::make_shared<CContainer>();

            const char* names[3] = { "CARDS/card_queen", "CARDS/card_jack", "CARDS/card_king" };
            for (int i = 0; i < 3; ++i)
            {
                auto spr = SpriteLoader::getInstance()->getSprite(names[i]);
                spr->setPivotCentered();
                spr->setScale(.55f, .55f);
                spr->setInteractive(true);
                spr->setMouseCursorType(eMouseCursorType::E_MCT_POINTER);

                spr->setOnHover([spr]
                {
                    spr->addSelfTween(eTweenProp::SCALE, spr->getScaleX(), .7f, .15f, Easing::outQuad);
                });

                spr->setOnLeave([spr]
                {
                    spr->addSelfTween(eTweenProp::SCALE, spr->getScaleX(), .55f, .15f, Easing::outQuad);
                });

                spr->setOnClick([spr]
                {
                    spr->addSelfTween(eTweenProp::ROTATE, spr->getRotate(), spr->getRotate() + 6.28318f, .6f, Easing::outBack);
                });

                ptrCont->addChild(spr);
            }

            ptrCont->alignChildren(eChildrenAlign::HORIZONTAL, 20.f, innerW - 30.f);
            ptrCont->setPosCentered(frame->calcNotTransCx(), frame->calcNotTransCy());
            frame->addChild(ptrCont);

            y += h + 20.f;
            pHeader->setX(frame->getX() + frame->calcNotTransCx() - (pHeader->calcNotTransCx() + 100));
        }

        // ---------------- секция 3: скролл-контейнер ----------------
        pHeader = makeHeader(L10N::getInstance().tr("SPRITE_DEMO_CLIPPING_HEADER"));
        {
            const float h = 380.f;
            auto frame = makeFrame(h);

            auto ptrScrollCont = std::make_shared<CContainer>();
            ptrScrollCont->setPos(75.f, 55.f);

            for (int i = 0; i < 14; ++i)
            {
                auto p = std::make_shared<StaticLabel>();
                p->setFont("condence");
                p->setFontSize(26);
                p->setRgba(i % 2 ? 0xC8B898FF : 0x9AC8D8FF);

                const int nLine = i + 1;
                const std::string line = std::vformat(L10N::getInstance().tr("SCROLL_LINE_FMT"), std::make_format_args(nLine));
                p->setText(line.c_str());

                p->setY((float)(i * 34));
                ptrScrollCont->addChild(p);
            }

            Rect rcBox{ 0.f, 0.f, innerW - 30.f, h - 130.f };
            ptrScrollCont->setScrollBox(&rcBox, eScrollType::E_ST_VERT);

            y += h;
            frame->addChild(ptrScrollCont);

            auto ptrBtns = std::make_shared<CContainer>();
            ptrMove->addChild(ptrBtns);

            auto b1 = CyanSlicedButton::makeInst(46, L10N::getInstance().tr("BTN_DOWN"), [ptrScrollCont]
            {
                ptrScrollCont->animateScrollTo(ptrScrollCont->getMaxScroll(), .7f, Easing::outExpo);
            });

            auto b2 = GreenSlicedButton::makeInst(46, L10N::getInstance().tr("BTN_UP"), [ptrScrollCont]
            {
                ptrScrollCont->animateScrollTo(0.f, .7f, Easing::outExpo);
            });

            ptrBtns->addChild(b1); ptrBtns->addChild(b2);
            ptrBtns->alignChildren(eChildrenAlign::HORIZONTAL, 10.f, innerW);
            ptrBtns->setXPosCentered(frame->calcNotTransCx(), frame->getX());
            ptrBtns->setY(y);

            y += 70.f;
            pHeader->setX(100);
        }

        // ---------------- секция 4: дети контейнера + твины ----------------
        pHeader = makeHeader(L10N::getInstance().tr("SPRITE_DEMO_CHILDREN_TWEENS_HEADER"));
        {
            const float h = 350.f;
            auto frame = makeFrame(h);
            auto ptrCont = std::make_shared<CContainer>();

            static const char* names[4] = { "CARDS/card_joker", "CARDS/card_king", "CARDS/card_queen", "CARDS/card_jack" };
            static std::array<CSpritePtr, SIZE_OF(names)> vCards;

            for (int i = 0; i < (int)vCards.size(); ++i)
            {
                auto spr = SpriteLoader::getInstance()->getSprite(names[i]);
                spr->setPivotCentered();
                spr->setScale(.5f, .5f);
                ptrCont->addChild(spr);
                vCards[i] = spr;
            }

            y += h + 20.f;

            auto ptrBtns = std::make_shared<CContainer>();
            ptrMove->addChild(ptrBtns);

            auto bWave = CyanSlicedButton::makeInst(46, L10N::getInstance().tr("BTN_WAVE"), []
            {
                for (size_t i = 0; i < vCards.size(); ++i)
                {
                    auto s = vCards[i];
                    s->removeSelfTweens();
                    s->addSelfTweenEx(eTweenProp::Y, s->getY(), s->getY() - 46.f, .45f, Easing::outQuad,
                        .07f * (float)i, {}, nullptr, eTweenLoopMode::YOYO, -1, Easing::outQuad);
                }
            });

            auto bFan = OrangeSlicedButton::makeInst(46, L10N::getInstance().tr("BTN_FAN"), []
            {
                for (size_t i = 0; i < vCards.size(); ++i)
                {
                    auto s = vCards[i];
                    float ang = ((float)i - 1.5f) * .35f * -1.f;
                    s->removeSelfTweens();
                    s->addSelfTween(eTweenProp::ROTATE, s->getRotate(), ang, .5f, Easing::outBack, .05f * (float)i);
                    s->addSelfTween(eTweenProp::Y, s->getY(), 20.f + fabsf(ang) * 60.f, .5f, Easing::outBack, .05f * (float)i);
                }
            });

            auto bReset = GreenSlicedButton::makeInst(46, L10N::getInstance().tr("BTN_RESET"), []
            {
                for (size_t i = 0; i < vCards.size(); ++i)
                {
                    auto s = vCards[i];
                    s->removeSelfTweens();
                    s->addSelfTween(eTweenProp::ROTATE, s->getRotate(), 0.f, .4f, Easing::outQuad, .04f * (float)i);
                    s->addSelfTween(eTweenProp::Y, s->getY(), 20.f, .4f, Easing::outQuad, .04f * (float)i);
                    s->addSelfTween(eTweenProp::SCALE, s->getScaleX(), .5f, .4f, Easing::outQuad, .04f * (float)i);
                }
            });

            ptrBtns->addChild(bWave);
            ptrBtns->addChild(bFan);
            ptrBtns->addChild(bReset);
            ptrBtns->alignChildren(eChildrenAlign::HORIZONTAL, 10.f, innerW);

            ptrCont->alignChildren(eChildrenAlign::HORIZONTAL, 10.f, innerW - 30.f);
            ptrCont->setPosCentered(frame->calcNotTransCx(), frame->calcNotTransCy());
            frame->addChild(ptrCont);

            ptrBtns->setX(left);
            ptrBtns->setY(y);

            y += 70.f;
            pHeader->setX(frame->getX() + frame->calcNotTransCx() - (pHeader->calcNotTransCx() + 100));
        }

        _root->addChild(ptrMove);
        ptrMove->setY(70);
    }

protected:
    virtual void createScrollBar(eScrollType eScrollType) override
    {
        RenderTracker trackerScoped;
        _ptrScollBar = std::make_shared<ScrollBar>();

        switch (eScrollType)
        {
            case eScrollType::E_ST_VERT:
            {
                _ptrScollBar->create(eScrollType::E_ST_VERT, _fDialogCy - 250);
                _frame->addChild(_ptrScollBar);
                _ptrScollBar->setX(_fDialogCx - 25);
                _ptrScollBar->setYPosCentered(_fDialogCy, 40);
            }
            break;

            default:
                assert(false);
            break;
        }
    }

    virtual void getScrollShadowPos(float& fX1, float& fY1, float& fX2, float& fY2, float& fCX, float& fCY) override
    {
        assert(_eScrollType == E_ST_VERT);
        fY1 -= 5;
        fY2 += 5;
        fCY = 100;
        fCX = _fDialogCx + 30;
    }
};