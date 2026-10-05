#include "pch.h"
#include "panelhtmldemo.h"
#include "htmldom.h"
#include "gfx.h"
#include "particle.h"
#include "particlepresets.h"
#include "utilfuncs.h"
#include "audiomanager.h"
#include "l10n.h"
#include <cstdio>

// =============================================================================
// Настройки шрифтов ТОЛЬКО для страницы nTextIdx == 2.
// =============================================================================
namespace
{
    // ------------------------------------------------------------------
    // Кроссплатформенное чтение embed-файла без стримов (C stdio).
    // Файлы лежат в EMBED/HTML рядом с ресурсами игры.
    // ------------------------------------------------------------------
    std::string loadEmbedText(const char* lpszRelPath)
    {
#if defined(_WIN32)
        std::string fullPath = std::string("EMBED\\HTML\\") + lpszRelPath;
#else
        std::string fullPath = std::string("EMBED/HTML/") + lpszRelPath;
#endif
        std::string out;

        FILE* f = std::fopen(fullPath.c_str(), "rb");
        if (!f)
            return out;

        if (std::fseek(f, 0, SEEK_END) == 0)
        {
            long nSize = std::ftell(f);
            if (nSize > 0)
            {
                out.resize((size_t)nSize);
                std::rewind(f);

                size_t nRead = std::fread(&out[0], 1, (size_t)nSize, f);
                if (nRead != (size_t)nSize)
                    out.resize(nRead);
            }
        }

        std::fclose(f);
        return out;
    }

    struct PPSliderDesc
    {
        const char* group;
        const char* id;
        float       mn, mx, step;
        int         dec;
        float PostProcessSettings::* pf;
        float BloomSettings::*       bf;
    };

    struct PPCheckDesc
    {
        const char* group;
        const char* id;
        bool PostProcessSettings::* pb;
        bool BloomSettings::*       bb;
        bool external;
    };

    static const PPSliderDesc kPPSliders[] =
    {
        // BLOOM
        { "BLOOM", "pp_bloom_int", 0.f, 4.f, 0.05f, 2, nullptr, &BloomSettings::intensity },
        { "BLOOM", "pp_bloom_thr", 0.f, 2.f, 0.05f, 2, nullptr, &BloomSettings::threshold },
        { "BLOOM", "pp_bloom_rad",    0.f, 3.f, 0.05f, 2, nullptr, &BloomSettings::radius },

        // VIGNETTE
        { "VIGNETTE", "pp_vig_int",   0.f, 1.f, 0.01f, 2, &PostProcessSettings::vignetteIntensity, nullptr },
        { "VIGNETTE", "pp_vig_rad",      0.f, 1.f, 0.01f, 2, &PostProcessSettings::vignetteRadius, nullptr },
        { "VIGNETTE", "pp_vig_smo",  0.f, 2.f, 0.01f, 2, &PostProcessSettings::vignetteSmoothness, nullptr },
        { "VIGNETTE", "pp_vig_br",     0.f, 1.f, 0.01f, 2, &PostProcessSettings::vignetteBreatheIntensity, nullptr },

        // COLOR
        { "COLOR", "pp_sat", 0.f,  2.f, 0.01f, 2, &PostProcessSettings::saturation, nullptr },
        { "COLOR", "pp_con",   0.f,  2.f, 0.01f, 2, &PostProcessSettings::contrast, nullptr },
        { "COLOR", "pp_bri", -1.f, 1.f, 0.01f, 2, &PostProcessSettings::brightness, nullptr },
        { "COLOR", "pp_abe", 0.f,  0.05f, 0.001f, 3, &PostProcessSettings::aberration, nullptr },
        { "COLOR", "pp_gra",      0.f,  1.f, 0.01f, 2, &PostProcessSettings::grainIntensity, nullptr },

        // WARP
        { "WARP", "pp_dis", 0.f, 1.f, 0.01f, 2, &PostProcessSettings::distortionIntensity, nullptr },
        { "WARP", "pp_fis",    0.f, 1.f, 0.01f, 2, &PostProcessSettings::fisheyeIntensity, nullptr },

        // WEATHER
        { "WEATHER", "pp_rain",  0.f, 1.f, 0.01f, 2, &PostProcessSettings::fRainIntensity, nullptr },
        { "WEATHER", "pp_blood", 0.f, 1.f, 0.01f, 2, &PostProcessSettings::fBloodDrops, nullptr },
    };

    static const PPCheckDesc kPPChecks[] =
    {
        { "MASTER", "pp_enabled",  &PostProcessSettings::enabled, nullptr, false },
        { "BLOOM",  "pp_bloom_on", nullptr, &BloomSettings::enabled, false },
    };

    std::string ppFmt(float v, int dec = 2)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", dec, v);
        return buf;
    }


    bool ppGetB(const PostProcessSettings& s, const PPCheckDesc& d)
    {
        if (d.bb) return s.bloom.*d.bb;
        if (d.pb) return s.*d.pb;
        return false;
    }

    // =============================================================================
    // Генерация HTML пульта пост-процесса.
    // =============================================================================
    // =============================================================================
    // Генерация демо-страницы 1.
    // =============================================================================

} // namespace

// =============================================================================
// Привязки пульта пост-процесса.
// =============================================================================
void PanelHTMLDemo::initPPPage()
{
    auto& doc = getHTMLDocument();

    _ppSliders.clear();
    _ppChecks.clear();
    _ppMasterCheck = 0;
    _ppBloomCheck  = 0;

    for (const auto& d : kPPSliders)
    {
        PPBind b;

        b.name    = d.id;
        b.group   = d.group;
        b.id      = doc.find(d.id);
        b.valId   = doc.find((std::string(d.id) + "_v").c_str());
        b.labelId = doc.find((std::string(d.id) + "_l").c_str());
        b.min     = d.mn;
        b.max     = d.mx;
        b.dec     = d.dec;
        b.pf      = d.pf;
        b.bf      = d.bf;

        if (b.id)
            _ppSliders[b.id] = b;
    }

    doc.onSliderChangedAny([this](unsigned long long nodeId, float value)
    {
        auto it = _ppSliders.find(nodeId);
        if (it == _ppSliders.end())
            return;

        PPBind& b = it->second;

        float v = std::clamp(value, b.min, b.max);

        if (b.pf)
            _pp.*b.pf = v;
        else if (b.bf)
            _pp.bloom.*b.bf = v;

        updateSliderLabel(b, v);
        applyPP();
    });

    for (const auto& d : kPPChecks)
    {
        PPCheckBind b;

        b.name  = d.id;
        b.group = d.group;
        b.id    = doc.find(d.id);
        b.pb    = d.pb;
        b.bb    = d.bb;

        if (!b.id)
            continue;

        _ppChecks[b.id] = b;

        if (!strcmp(d.id, "pp_enabled"))
            _ppMasterCheck = b.id;

        if (!strcmp(d.id, "pp_bloom_on"))
            _ppBloomCheck = b.id;

        doc.onClick(d.id, [this, name = std::string(d.id), b](float, float, int)
        {
            bool on = getHTMLDocument()[name.c_str()].isChecked();

            if (b.bb)
                _pp.bloom.*b.bb = on;
            else if (b.pb)
                _pp.*b.pb = on;

            applyPP();
            refreshPPEnabled();
        });
    }

    _ppMaskSelect      = doc.find("pp_mask");
    _ppMaskSelectLabel = doc.find("pp_mask_l");

    doc.onSelectChangedAny([this](unsigned long long selectId, int, const char*)
    {
        if (selectId != _ppMaskSelect)
            return;

        applyPP();
    });

    _ppResetBtn = doc.find("pp_reset");

    if (_ppResetBtn)
    {
        doc.onClick("pp_reset", [this](float, float, int)
        {
            _pp = PostProcessSettings();

            pushPPToDom();
            refreshPPEnabled();
            applyPP();
        });
    }

    pushPPToDom();
    refreshPPEnabled();
}

void PanelHTMLDemo::updateSliderLabel(const PPBind& b, float v)
{
    if (!b.valId)
        return;

    CHTMLElement(&getHTMLDocument(), b.valId).setText(ppFmt(v, b.dec));
}

void PanelHTMLDemo::applyPP()
{
    auto* gfx = CGfx::getInstance();
    gfx->setPostProcessSettings(_pp);
}

void PanelHTMLDemo::pushPPToDom()
{
    for (auto& kv : _ppSliders)
    {
        const PPBind& b = kv.second;

        float v = b.bf ? _pp.bloom.*b.bf : (b.pf ? _pp.*b.pf : 0.f);
        v = std::clamp(v, b.min, b.max);

        CHTMLElement(&getHTMLDocument(), b.id).setSliderValue(v, false);
        updateSliderLabel(b, v);
    }

    for (auto& kv : _ppChecks)
    {
        const PPCheckBind& b = kv.second;

        bool on = (b.bb ? _pp.bloom.*b.bb : (b.pb ? _pp.*b.pb : false));
        CHTMLElement(&getHTMLDocument(), b.id).setChecked(on);
    }
}

void PanelHTMLDemo::refreshPPEnabled()
{
    auto& doc = getHTMLDocument();
    auto el = [&doc](unsigned long long nid) { return CHTMLElement(&doc, nid); };

    const bool masterOn = _pp.enabled;
    const bool bloomOn  = _pp.bloom.enabled;

    // Слайдеры: сам слайдер + его метка + его значение.
    for (auto& kv : _ppSliders)
    {
        const PPBind& b = kv.second;

        bool dis = !masterOn;

        if (b.group && !strcmp(b.group, "BLOOM"))
            dis = dis || !bloomOn;

        el(b.id).setDisabled(dis);
        el(b.labelId).setDisabled(dis);
        el(b.valId).setDisabled(dis);
    }

    // Чекбоксы: подпись лежит ВНУТРИ чекбокса и тонируется автоматически,
    // дизейблим только сам узел. Мастер-чекбокс активен всегда.
    for (auto& kv : _ppChecks)
    {
        const PPCheckBind& b = kv.second;

        if (b.id == _ppMasterCheck)
        {
            el(b.id).setDisabled(false);
            continue;
        }

        el(b.id).setDisabled(!masterOn);
    }

    // Комбобокс маски + его метка.
    if (_ppMaskSelect)
        el(_ppMaskSelect).setDisabled(!masterOn);

    if (_ppMaskSelectLabel)
        el(_ppMaskSelectLabel).setDisabled(!masterOn);

    // Кнопка сброса.
    if (_ppResetBtn)
        el(_ppResetBtn).setDisabled(!masterOn);
}

// =============================================================================
// Страница громкости.
// =============================================================================
void PanelHTMLDemo::initVolumePage()
{
    auto& doc = getHTMLDocument();
    auto root = getHTMLRootId();

    _volSliders.clear();
    _volButtonLabels.clear();

    if (!root)
        return;

    struct VolumeSliderDesc
    {
        const char* name;
        const char* id;
        float min;
        float max;
        int dec;
    };

    static const VolumeSliderDesc kVolumeSliders[] =
    {
        { "master", "vol_master", 0.f, 1.f, 2 },
        { "music",  "vol_music",  0.f, 1.f, 2 },
        { "sounds", "vol_sounds", 0.f, 1.f, 2 },
    };

    for (const auto& d : kVolumeSliders)
    {
        VolumeBind b;

        b.name    = d.name;
        b.id      = doc.find(d.id);
        b.valId   = doc.find((std::string(d.id) + "_v").c_str());
        b.labelId = doc.find((std::string(d.id) + "_l").c_str());
        b.min     = d.min;
        b.max     = d.max;
        b.dec     = d.dec;

        if (b.id)
            _volSliders[b.id] = b;
    }

    _volRootId = root;

    doc.onHoverAny([this](unsigned long long nodeId) { onVolumeHover(nodeId); },
                   [this](unsigned long long nodeId) { onVolumeLeave(nodeId); });

    doc.onSliderChangedAny([this](unsigned long long nodeId, float fValue)
    {
        onVolumeSliderChanged(nodeId, fValue);
    });

    auto bindButton = [this, &doc](const char* id)
    {
        unsigned long long bid = doc.find(id);
        unsigned long long fid = doc.find((std::string(id) + "_t").c_str());

        if (bid && fid)
            _volButtonLabels[bid] = fid;

        if (bid)
        {
            doc.onClick(id, [this, id = std::string(id)](float, float, int)
            {
                if (id == "vol_btn_sound1")
                {
                    AudioManager::get().playSound("buff");
                }
                else if (id == "vol_btn_sound2")
                {
                    AudioManager::get().playSound("buff2");
                }
                else if (id == "vol_btn_sound3")
                {
                    AudioManager::get().playSound("buff3");
                }
                else if (id == "vol_btn_music")
                {
                    auto& d = getHTMLDocument();

                    std::vector<std::string> v =
                    {
                        "axe03.mp3",
                        "axe04.mp3",
                        "boss_bg.mp3"
                    };

                    if (!_bIsMusicPlaying)
                    {
                        d["vol_btn_music_t"].setText(L10N::getInstance().tr("VOL_BTN_STOP"));
                        _bIsMusicPlaying = true;

                        AudioManager::get().playMusicPlaylist(v, [w = weak_from_this()]
                        {
                            if (auto p = std::static_pointer_cast<PanelHTMLDemo>(w.lock()))
                            {
                                p->getHTMLDocument()["vol_btn_music_t"].setText(L10N::getInstance().tr("VOL_BTN_MUSIC"));
                                p->_bIsMusicPlaying = false;
                            }
                        });
                    }
                    else
                    {
                        d["vol_btn_music_t"].setText(L10N::getInstance().tr("VOL_BTN_MUSIC"));
                        _bIsMusicPlaying = false;
                        AudioManager::get().stopMusic();
                    }
                }
            });
        }
    };

    bindButton("vol_btn_sound1");
    bindButton("vol_btn_sound2");
    bindButton("vol_btn_sound3");
    bindButton("vol_btn_music");
    bindButton("vol_btn_stop_music");
}

void PanelHTMLDemo::onVolumeHover(unsigned long long nodeId)
{
    auto it = _volButtonLabels.find(nodeId);
    if (it == _volButtonLabels.end())
        return;

    static eAppearStyle arr[] =
    {
        eAppearStyle::POP,
        eAppearStyle::RUNIC,
        eAppearStyle::NEON,
        eAppearStyle::VHS,
        eAppearStyle::LASERSCAN
    };

    auto e = getRandomElement(arr);

    CHTMLElement(&getHTMLDocument(), it->second).setAppearAnimation(e);
}

void PanelHTMLDemo::onVolumeLeave(unsigned long long nodeId)
{
    auto it = _volButtonLabels.find(nodeId);
    if (it == _volButtonLabels.end())
        return;

    CHTMLElement(&getHTMLDocument(), it->second).stopFontAnimation();
}

void PanelHTMLDemo::onVolumeSliderChanged(unsigned long long nodeId, float value)
{
    auto it = _volSliders.find(nodeId);
    if (it == _volSliders.end())
        return;

    auto sId = CHTMLElement(&getHTMLDocument(), nodeId).getAttr("id");

    if (sId == "vol_master")
        AudioManager::get().setMasterVolume(value);
    else if (sId == "vol_music")
        AudioManager::get().setMusicVolume(value);
    else if (sId == "vol_sounds")
        AudioManager::get().setSfxVolume(value);
}

PanelHTMLDemo::~PanelHTMLDemo()
{
}

// =============================================================================
// init
// =============================================================================
void PanelHTMLDemo::init(int nTextIdx, float cx, float cy, CContainerPtr ptrParent)
{
    // ------------------------------------------------------------------
    // Страница 3: панель настройки громкости
    // ------------------------------------------------------------------
    if (nTextIdx == 3)
    {
        // Если у тебя есть сохранённые значения громкости — загружай их здесь.
        _volMaster = 1.f;
        _volMusic  = 1.f;
        _volSounds = 1.f;

        std::string html = loadEmbedText("volume.html");
        PanelHTML::init(html.c_str(), cx, cy, ptrParent);

        auto ptrPixie = std::make_shared<CParticleSystem<>>(
            ParticlePresets::getPreset(eParticlePreset::BOKEH_AMBIENT),
            std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::BOKEH_AMBIENT)).c_str());

        ptrPixie->getSettings().spawnRadius = cx * 1.2f;
        ptrPixie->setPos(cx * 0.5f, 500);
        ptrPixie->getSettings().maxParticles = 200;
        ptrPixie->getSettings().spawnRate    = 20;
        ptrPixie->prewarm(5.f);

        _root->addChild(ptrPixie);
        _root->swapChildren(ptrPixie, _ptrLabel);

        //_ptrLabel->setVisible(false);

        initVolumePage();
        return;
    }

    // ------------------------------------------------------------------
    // Страница 2: пульт пост-процесса
    // ------------------------------------------------------------------
    if (nTextIdx == 2)
    {
        _pp = CGfx::getInstance()->getPostProcessSettings();

        std::string html = loadEmbedText("postprocess.html");
        PanelHTML::init(html.c_str(), cx, cy, ptrParent);

        initPPPage();
        return;
    }

    // ------------------------------------------------------------------
    // Страницы 0 и 1
    // ------------------------------------------------------------------
    std::string html = loadEmbedText(nTextIdx == 0 ? "page0.html" : "page1.html");
    PanelHTML::init(html.c_str(), cx, cy, ptrParent, true, true);

    getHTMLDocument().onSliderChanged("sld_scale", [this](float v)
    {
        auto& d = getHTMLDocument();

        // layout-твин: картинка реально занимает место (reflow)
        auto img = d["img_demo"];
        img.killTweens();
        img.tweenTo(HTMLTweenProps().width(v), 0.25f, HTMLTweenOptions{ .ease = Easing::outQuad });

        d.setTextFmt("sld_scale_v", "{:.0f}", v);
    });

    if (nTextIdx == 0)
    {
        auto clickHandler = [this](bool bIsAlianna)
        {
            eSpineGirlState& egs = bIsAlianna ? _eAlianna : _eAmarantha;

            auto buttonId = bIsAlianna ? "btn_1"   : "btn_2";
            auto spineId  = bIsAlianna ? "spine_1" : "spine_2";

            switch (egs)
            {
            case eSpineGirlState::IDLE:
            {
                egs = eSpineGirlState::CHARGING;

                getHTMLDocument()[spineId].spinePlay("charge", false);
                getHTMLDocument()[spineId].spineAdd("idle_charged", true);
                getHTMLDocument()[buttonId].setVisible(false);
            }
            break;

            case eSpineGirlState::IDLE_CHARGED:
            {
                getHTMLDocument()[spineId].spinePlay("idle", false);
                getHTMLDocument()[buttonId].setVisible(false);
            }
            break;

            default:
                assert(false);
                break;
            }
        };

        getHTMLDocument()["btn_1"].onClick([clickHandler] { clickHandler(true); });
        getHTMLDocument()["btn_2"].onClick([clickHandler] { clickHandler(false); });
        
        //getHTMLDocument()["xxx"].tweenTo(HTMLTweenProps().top(400.2f).scaleY(1.2), 3, {.repeat = -1, .yoyo = true});
    }
}