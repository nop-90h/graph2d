#pragma once

#include "basedialog.h"
#include "paneltabs.h"
#include "touchhandler.h"
#include "slotmachine.h"
#include "panelhtmldemo.h"
#include "spinecont.h"
#include "utilfuncs.h"

// =========================================================================
// COSMIC CARD STORM
// Ћуч в центре экрана + карты, орбитально ход€щие вокруг него, + эмбиент-
// пыль, закрученна€ вокруг луча и расплывающа€с€ от ближних карт.
// ÷ентровка —јћќ—“ќя“≈Ћ№Ќјя: каждый кадр по собственной мировой матрице
// (getTransX/getTransY/getWorldScale) вычисл€ем локальный центр, который
// попадает в центр экрана, - внешн€€ раскладка behind-bg сдвинуть не может.
// =========================================================================
class CosmicCardStorm final : public CContainer
{
private:
    struct CardFx
    {
        CSpritePtr pSpr;
        float baseK   = 1.f;
        float phase   = 0.f;
        float speed   = 0.f;
        float radiusK = 1.f;
        float sway    = 0.f;
    };

public:
    CosmicCardStorm()
    {
        auto ptrSL = SpriteLoader::getInstance();

        // -----------------------------------------------------------------
        // ‘он: два whitebox (позици€/масштаб став€тс€ в update по замеру).
        // -----------------------------------------------------------------
        m_ptrBg = ptrSL->getSprite("UI/whitebox");
        if (m_ptrBg)
        {
            m_ptrBg->setRgba(0x070511ff);
            addChild(m_ptrBg);
        }
        m_ptrBg2 = ptrSL->getSprite("UI/whitebox");
        if (m_ptrBg2)
        {
            m_ptrBg2->setRgba(0x1c0f3aff);
            m_ptrBg2->setAlpha(0.32f);
            addChild(m_ptrBg2);
        }

        // -----------------------------------------------------------------
        // Ћ”„: широкое м€гкое гало + €ркое €дро (аддитивные).
        // -----------------------------------------------------------------
        _ptrBeamHalo = ptrSL->getSprite("PARTICLES/light");
        if (_ptrBeamHalo)
        {
            _ptrBeamHalo->setBlendMode(eSpriteBlendMode::ADDITIVE);
            _ptrBeamHalo->setTint(0.45f, 0.35f, 1.00f);
            _ptrBeamHalo->setPivotCentered();
            _ptrBeamHalo->setAlpha(0.10f);
            addChild(_ptrBeamHalo);
        }
        _ptrBeamCore = ptrSL->getSprite("PARTICLES/light");
        if (_ptrBeamCore)
        {
            _ptrBeamCore->setBlendMode(eSpriteBlendMode::ADDITIVE);
            _ptrBeamCore->setTint(0.75f, 0.85f, 1.00f);
            _ptrBeamCore->setPivotCentered();
            _ptrBeamCore->setAlpha(0.35f);
            addChild(_ptrBeamCore);
        }

        // -----------------------------------------------------------------
        // Ёмбиент-пыль: закручена вокруг луча (tangentialAccel), слегка
        // прит€нута к нему (attractTo), расплываетс€ от ближних карт.
        // -----------------------------------------------------------------
        _ptrDust = std::make_shared<CParticleSystem<>>(
            ParticlePresets::getAmbientDustPreset(),
            std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::AMBIENT_DUST)).c_str());
        {
            auto& ds = _ptrDust->getSettings();
            ds.spawnRate    = 16.f;
            ds.maxParticles = 110;
            ds.lifetimeMin  = 5.f;
            ds.lifetimeMax  = 9.f;
            ds.spawnRadius  = 0.f;
            ds.spawnAreaCx  = Engine::getCfg().INIT_SCR_CX * 1.1f;
            ds.spawnAreaCy  = Engine::getCfg().INIT_SCR_CY * 1.1f;
            ds.speedMin     = 4.f;
            ds.speedMax     = 14.f;
            ds.drag         = 0.45f;
            ds.turbulence   = 8.f;
            ds.radialAccel     = -8.f;    // слегка т€нет к лучу
            ds.tangentialAccel = 35.f;    // закручивает вокруг луча
            ds.startScaleMin = 0.18f;
            ds.startScaleMax = 0.45f;
            ds.endScaleMin   = 0.18f;
            ds.endScaleMax   = 0.45f;
            ds.use3PhaseColor = true;
            ds.startColor[0]  = 0.70f; ds.startColor[1]  = 0.80f; ds.startColor[2]  = 1.00f; ds.startColor[3]  = 0.0f;
            ds.middleColor[0] = 0.85f; ds.middleColor[1] = 0.90f; ds.middleColor[2] = 1.00f; ds.middleColor[3] = 0.55f;
            ds.endColor[0]    = 0.60f; ds.endColor[1]    = 0.70f; ds.endColor[2]    = 1.00f; ds.endColor[3]    = 0.0f;
            ds.useDepth      = true;
            ds.depthMin      = 0.40f;
            ds.depthScaleMin = 0.40f;
            ds.depthSpeedMin = 0.40f;
            ds.depthAlphaMin = 0.45f;
            ds.fadeInTime    = 0.6f;
            ds.fadeOutTime   = 1.4f;
            ds.blendMode     = eSpriteBlendMode::ADDITIVE;
        }
        _ptrDust->attractTo(0.f, 0.f, 14.f);   // в локальных координатах эмиттера
        addChild(_ptrDust);
        _ptrDust->prewarm(6);

        // -----------------------------------------------------------------
        //  арты: 52 штуки, стартуют за лучом.
        // -----------------------------------------------------------------
        static const char* s_facePaths[] =
        {
            "CARDS/card_joker",
            "CARDS/card_king",
            "CARDS/card_queen",
            "CARDS/card_jack",
            "CARDS/card_10"
        };
        for (int i = 0; i < SIZE_OF(s_facePaths); ++i)
            m_faceTemplates[i] = ptrSL->getSprite(s_facePaths[i]);

        for (int i = 0; i < 52; ++i)
            spawnCard();

        _dustTimer = 0.f;
    }

    virtual void calcNotTransBounds(Rect* p) override
    {
        p->set(0.f, 0.f, Engine::getCfg().INIT_SCR_CX, Engine::getCfg().INIT_SCR_CY);
    }

    virtual void update(float dt) override
    {
        CContainer::update(dt);

        _time += dt;

        // -----------------------------------------------------------------
        // —јћќ÷≈Ќ“–»–ќ¬ ј: замер€ем, где наш локальный ноль реально стоит на
        // экране, и находим локальные координаты центра экрана и его углов.
        // ƒальше всЄ раскладываем только относительно них.
        // -----------------------------------------------------------------
        const float fScrCx = static_cast<float>(CSceneResize::getInstance()->getScreenWidth());
        const float fScrCy = static_cast<float>(CSceneResize::getInstance()->getScreenHeight());

        float fWsX = 1.f, fWsY = 1.f;
        getWorldScale(fWsX, fWsY);
        if (std::fabs(fWsX) < 1e-4f) fWsX = 1.f;
        if (std::fabs(fWsY) < 1e-4f) fWsY = 1.f;

        const float fOrgX = getTransX();
        const float fOrgY = getTransY();

        m_localX0 = (0.f     - fOrgX) / fWsX;
        m_localY0 = (0.f     - fOrgY) / fWsY;
        m_localW  = (fScrCx) / fWsX;
        m_localH  = (fScrCy) / fWsY;
        m_centerX = m_localX0 + m_localW * 0.5f;
        m_centerY = m_localY0 + m_localH * 0.5f;

        const float fHalfW = m_localW * 0.5f;
        const float fHalfH = m_localH * 0.5f;

        // -----------------------------------------------------------------
        // ‘он: раст€гиваем ровно на видимый экран.
        // -----------------------------------------------------------------
        if (m_ptrBg)
        {
            m_ptrBg->setPos(m_localX0, m_localY0);
            m_ptrBg->setScaleTo(m_localW, m_localH);
        }
        if (m_ptrBg2)
        {
            m_ptrBg2->setPos(m_localX0, m_localY0);
            m_ptrBg2->setScaleTo(m_localW, m_localH);
        }

        // -----------------------------------------------------------------
        // Ћуч: дыхание + едва заметное покачивание; центр - центр экрана.
        // -----------------------------------------------------------------
        if (_ptrBeamHalo)
        {
            _ptrBeamHalo->setScaleTo(m_localW * 0.30f, m_localH * 1.90f);
            _ptrBeamHalo->setPosCentered(m_localW, m_localH,
                                         m_centerX - fHalfW, m_centerY - fHalfH);
            _ptrBeamHalo->setAlpha(0.08f + 0.04f * std::sin(_time * 0.70f));
            _ptrBeamHalo->rotate(std::sin(_time * 0.10f) * 0.040f);
        }
        if (_ptrBeamCore)
        {
            _ptrBeamCore->setScaleTo(m_localW * 0.10f, m_localH * 1.75f);
            _ptrBeamCore->setPosCentered(m_localW, m_localH,
                                         m_centerX - fHalfW, m_centerY - fHalfH);
            _ptrBeamCore->setAlpha(0.30f + 0.10f * std::sin(_time * 1.30f));
            _ptrBeamCore->rotate(std::sin(_time * 0.13f) * 0.025f);
        }

        // -----------------------------------------------------------------
        // ѕыль: эмиттер в центре экрана.
        // -----------------------------------------------------------------
        if (_ptrDust)
            _ptrDust->setPos(m_centerX, m_centerY);

        // -----------------------------------------------------------------
        //  арты: орбита вокруг центра экрана. Ѕез вращени€ вокруг своей оси.
        // -----------------------------------------------------------------
        const float fMaxR = std::min(m_localW, m_localH) * 0.46f;

        float fNearX = m_centerX, fNearY = m_centerY, fNearZ = -1.f;

        for (auto& c : _cards)
        {
            if (!c.pSpr)
                continue;

            const float fA  = c.phase + _time * c.speed;
            const float fSn = std::sin(fA);
            const float fCs = std::cos(fA);
            const float fZ  = 0.5f + 0.5f * fSn;

            const float fR = fMaxR * c.radiusK * (0.30f + 0.95f * fZ);

            const float tX = m_centerX + fCs * fR * 1.35f;
            const float tY = m_centerY + fSn * fR * 0.85f
                           + std::sin(_time * 0.5f + c.sway) * m_localH * 0.012f;

            const float fS = c.baseK * (0.55f + 0.80f * fZ);
            c.pSpr->setScale(fS, fS);
            c.pSpr->setPosCentered(m_localW, m_localH,
                                   tX - fHalfW, tY - fHalfH);
            c.pSpr->setAlpha(0.22f + 0.78f * fZ);

            // ѕлавный тинт по глубине: холодный сзади -> тЄплый спереди.
            const float fRc = 0.55f + 0.45f * fZ;
            const float fGc = 0.62f + 0.26f * fZ;
            const float fBc = 0.95f - 0.25f * fZ;
            c.pSpr->setTint(fRc, fGc, fBc);

            if (fZ > fNearZ)
            {
                fNearZ = fZ;
                fNearX = tX;
                fNearY = tY;
            }
        }

        // -----------------------------------------------------------------
        // ѕыль реагирует на ближнюю к зрителю карту: расплываетс€ от неЄ.
        // -----------------------------------------------------------------
        _dustTimer -= dt;
        if (_dustTimer <= 0.f && _ptrDust)
        {
            _dustTimer = 0.22f;
            if (fNearZ > 0.55f)
                _ptrDust->repelFrom(fNearX - m_centerX,
                                    fNearY - m_centerY,
                                    900.f, 300.f);
            else
                _ptrDust->disableRepeller();
        }
    }

private:
    void spawnCard()
    {
        const float fCy = Engine::getCfg().INIT_SCR_CY;

        CSpritePtr pTemplate = m_faceTemplates[rand() % SIZE_OF(m_faceTemplates)];
        if (!pTemplate)
            return;

        CSpritePtr pCard = pTemplate->cloneInitial();
        if (!pCard)
            return;

        pCard->setBlendMode(eSpriteBlendMode::NORMAL);
        pCard->setPivotCentered();
        pCard->setVisible(true);

        const float fOrigH = pCard->calcNotTransCy();
        const float fH     = randomRange(fCy * 0.10f, fCy * 0.14f);
        const float fK     = (fOrigH > 1.f) ? (fH / fOrigH) : 1.f;
        pCard->setScale(fK, fK);

        addChild(pCard);

        CardFx c;
        c.pSpr    = pCard;
        c.baseK   = fK;
        c.phase   = randomRange(0.f, 6.2831853f);
        c.speed   = randomRange(0.22f, 0.34f);   // все в одну сторону вокруг луча
        c.radiusK = randomRange(0.75f, 1.15f);
        c.sway    = randomRange(0.f, 6.2831853f);
        _cards.push_back(c);
    }

    std::vector<CardFx>      _cards;

    CSpritePtr               _ptrBeamHalo;
    CSpritePtr               _ptrBeamCore;
    CParticleSystemPtr       _ptrDust;

    CSpritePtr               m_ptrBg;
    CSpritePtr               m_ptrBg2;
    CSpritePtr               m_faceTemplates[5];

    float                    m_localX0 = 0.f;   // локальный X левого кра€ экрана
    float                    m_localY0 = 0.f;   // локальный Y верхнего кра€ экрана
    float                    m_localW  = 0.f;   // локальна€ ширина экрана
    float                    m_localH  = 0.f;   // локальна€ высота экрана
    float                    m_centerX = 0.f;   // локальный центр экрана
    float                    m_centerY = 0.f;

    float                    _time      = 0.f;
    float                    _dustTimer = 0.f;
};