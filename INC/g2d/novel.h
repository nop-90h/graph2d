#include "g2d.h"
#include "engine.h"
#include "audiomanager.h"
#include "widgets/staticlabel.h"

_G2D_NAMESPACE_BEGIN_

struct NovelEntry;

struct AnswerButtonSkin
{
    LPCTSTR lpszNormal;
    LPCTSTR lpszHover;
    LPCTSTR lpszPressed;

};

struct NovelAnswer
{
    LPCTSTR                              lpszAnswerTitle   = nullptr;
    LPCTSTR                              lpszText          = nullptr;
    NovelEntry*                          pNext             = nullptr;
    bool                                 bActive           = true;
    std::function <void(SimpleCallback)> _cbOnSelected     = {};
    std::optional<AnswerButtonSkin>      btnSkin;
};

class Novel;

struct NovelEntry
{
    float   fBgShowPause     = 0.f;
    LPCTSTR lpszBg           = nullptr;
    LPCTSTR lpszBgEn         = nullptr;
    LPCTSTR lpszSpeakerPic   = nullptr;
    struct
    {
        LPCTSTR lpszSpeakerSpine     = nullptr;
        LPCTSTR lpszAnimName         = "idle";
        LPCTSTR lpszAnimNameSwitch   = nullptr;
        LPCTSTR lpszAnimNameNew      = nullptr;
        LPCTSTR lpszFinalAnimName    = nullptr;

    } spineOpts;

    LPCTSTR                                      lpszSpeakerText    = nullptr;
    float                                        fYTextCorrection   = 0.f;
    float                                        fWorldDarken       = 0.f;
    LPCTSTR                                      plszCenterPic      = nullptr;
    float                                        fPicX              = 0.f;
    float                                        fPicY              = 0.f;
    float                                        fPicScale          = 1.f;
    Rect                                         rcSpinePos         = {};
    float                                        fSpineScale        = 1.f;
    bool                                         bNoStartTransition = false;
    NovelAnswer                                  answers[3];
    float                                        fEffectLifeTime    = 0.f;
    float                                        fEffectType        = 0.f;
    std::function <void(Novel*, SimpleCallback)> _cbOnStart;
    std::function <void(Novel*, SimpleCallback)> _cbOnEnd;
    LPCTSTR                                      lpszSpeach         = nullptr;
    
};

class Novel : public BaseDialog
{
private:
    // скин кнопок в HTML — подставьте реальный путь спрайта кнопки
    // (тот, что раньше рисовал GreenSlicedButton)
    static constexpr LPCTSTR NOVEL_BTN_SKIN_NORMAL  = "UI/slicedGreenBtn_normal";
    static constexpr LPCTSTR NOVEL_BTN_SKIN_PRESSED = "UI/slicedGreenBtn_normal";
    static constexpr LPCTSTR NOVEL_BTN_SKIN_HL      = "UI/slicedGreenBtn_hl";

    bool                                         _bAutoUnloadTextures = false;
    std::string                                  _strCurrentBg;
    std::vector<NovelEntry>                      _entries;
    NovelEntry*                                  _pCurrent = nullptr;
    NovelAnswer*                                 _pSelectedAnswer = nullptr;

    CContainerPtr                                _ptrContent;
    CContainerPtr                                _ptrBg;
    CContainerPtr                                _ptrSpeaker;
    CContainerPtr                                _ptrCenterPic;
    StaticLabelPtr                               _ptrText;              // весь текст + все кнопки (HTML)
    std::string                                  _strCurrentSpine;
    int                                          _nSkipTo = -1;
    int                                          _nPlayingSpeach = -1;
    std::string                                  _strContinueText;
    bool                                         _bNoWorldLighten = false;
    float                                        _fPicCx = 0.f;
    float                                        _fTextFontSize = 0;
    std::string                                  _sFontName;
public:

    void setNoWorldLightenOnClose() { _bNoWorldLighten = true; }

    void init(std::span<NovelEntry> entries, int nSkipTo = -1, bool bAutoUnloadTextures = false, LPCTSTR lpszContinueText = "Continue", LPCTSTR lpszFontName = nullptr, std::optional<float> fFontSize = std::nullopt)
    {
        auto& cfg = Engine::getCfg();

        _sFontName                = lpszFontName ? lpszFontName : cfg.NOVEL_FONT;
        _fTextFontSize            = fFontSize    ? *fFontSize   : cfg.NOVEL_FONT_TEXT_SIZE;
        _bAutoUnloadTextures      = bAutoUnloadTextures;
        _strContinueText          = lpszContinueText;
        _bAutoHideFgControls      = false;
        _bDispatchOutOfClientRect = true;
        _nSkipTo = nSkipTo;
        _entries.reserve(entries.size());
        std::copy(entries.begin(), entries.end(), std::back_inserter(_entries));
        for (int i = 0; i < entries.size(); i++)
        {
            auto pEntry = entries[i];
            for (auto j = 0; j < SIZE_OF_T<decltype(pEntry.answers)>; j++)
            {
                auto pNext = pEntry.answers[j].pNext;
                if (pNext)
                {
                    auto dist = std::distance(entries.data(), pNext);
                    assert(dist > -1 && dist < _entries.size());
                    _entries[i].answers[j].pNext = _entries.data() + dist;
                }
            }
        }
        BaseDialog::init(cfg.INIT_SCR_CX, cfg.INIT_SCR_CY,
                         E_ST_NONE, nullptr, nullptr);
        _eCloseStyle = eDialogCloseStyle::CLOSE_BUTTON;

        _ptrContent = std::make_shared<CContainer>();
        _root->addChild(_ptrContent);
    }

    void start()
    {
        if (!_entries.empty())
        {
            if (_nSkipTo >= 0 && _nSkipTo < (int)_entries.size())
                showEntry(&_entries[_nSkipTo]);
            else
                showEntry(&_entries[0]);
        }
    }

    int getEntryIdx(NovelEntry* pEntry)
    {
        int idx = -1;
        for (int i = 0; i < _entries.size(); i++)
        {
            if (&_entries[i] == pEntry)
            {
                idx = i;
                break;
            }
        }
        return idx;
    }

    NineSlicePtr getDialogFrame(float fCx, float fCy)
    {
        NineSlicePtr ptrFrame = std::make_shared<NineSlice>();
        ptrFrame->createSlices(SpriteLoader::getInstance()->getSprite("UI/frameFg"), 60, 60, 46, 46);
        ptrFrame->build(fCx, fCy);
        return ptrFrame;
    }

    void showPreparedEntry(NovelEntry* pEntry)
    {
        if (pEntry->lpszBg)
        {
            auto& cfg = Engine::getCfg();

            auto lpszBg = cfg.LANG == eLang::RU ? pEntry->lpszBg:pEntry->lpszBgEn ? pEntry->lpszBgEn : pEntry->lpszBg;
            assert(lpszBg);
            auto ptrBgImage = CGfx::getInstance()->spriteFromTexture(lpszBg);
            if (ptrBgImage)
            {
                _ptrBg = std::make_shared<CContainer>();
                auto ptrFrame = getDialogFrame(ptrBgImage->calcNotTransCx() + 30, ptrBgImage->calcNotTransCy() + 30);
                _ptrBg->addChild(ptrBgImage);
                ptrBgImage->setPos(15, 15);
                _ptrBg->addChild(ptrFrame);
                _ptrBg->setPos(0, 0);
                _ptrBg->setPosCentered(cfg.INIT_SCR_CX, cfg.INIT_SCR_CY);
                _ptrContent->addChild(_ptrBg);
                if (_strCurrentBg != lpszBg)
                {
                    _ptrBg->setAlpha(0.f);
                    _strCurrentBg = lpszBg;
                    _ptrBg->addSelfTween(eTweenProp::ALPHA, 0.f, 1.f, 0.5f,
                                         Easing::linear, 0.f, [this, pEntry]{
                        setTimeout(pEntry->fBgShowPause, [this, pEntry]{
                            if (_ptrBg)
                            {
                                _ptrBg->addSelfTween(eTweenProp::BLACKEN, 1.f, pEntry->lpszSpeakerPic ? 0.6f : 0.6f,
                                                     0.5f, Easing::linear, 0.f, [this, pEntry]{
                                    showSpeakerAndText(pEntry);
                                });
                            }
                            else
                            {
                                showSpeakerAndText(pEntry);
                            }
                        });
                    });
                }
                else
                {
                    setTimeout(pEntry->fBgShowPause, [this, pEntry]{
                        if (_ptrBg)
                        {
                            _ptrBg->addSelfTween(eTweenProp::BLACKEN, 1.f, pEntry->lpszSpeakerPic ? 0.6f : 0.6f,
                                                    0.5f, Easing::linear, 0.f, [this, pEntry]{
                                showSpeakerAndText(pEntry);
                            });
                        }
                        else
                        {
                            showSpeakerAndText(pEntry);
                        }
                    });
                }
            }
            else
            {
                showSpeakerAndText(pEntry);
            }
        }
        else
        {
            _strCurrentBg.clear();
            showSpeakerAndText(pEntry);
        }
    }

    void showEntry_bgLoaded(NovelEntry* pEntry)
    {
        _pCurrent = pEntry;
        _pSelectedAnswer = nullptr;
        _ptrContent->removeAll();
        _ptrBg.reset();
        _ptrSpeaker.reset();
        _ptrCenterPic.reset();
        _ptrText.reset();

        float fDarken = (pEntry->fWorldDarken > 0.f) ? pEntry->fWorldDarken : 0.2f;
        CGfx::getInstance()->setWorldDarken(true, fDarken, fDarken);
        if (pEntry->_cbOnStart)
        {
            pEntry->_cbOnStart(this, [this, pEntry]{
                showPreparedEntry(pEntry);
            });
        }
        else
        {
            showPreparedEntry(pEntry);
        }
    }

    void showEntry(NovelEntry* pEntry)
    {
        AudioManager::get().stopSound(_nPlayingSpeach);
        if (!pEntry) { close(); return; }
        showEntry_bgLoaded(pEntry);
    }

    // ------------------------------------------------------------------
    // HTML helpers
    // ------------------------------------------------------------------

    static std::string htmlEscape(LPCTSTR lpszText)
    {
        std::string out;
        if (!lpszText)
            return out;
        for (const char* p = lpszText; *p; ++p)
        {
            switch (*p)
            {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            default:   out += *p;       break;
            }
        }
        return out;
    }

    bool hasActiveAnswers(NovelEntry* pEntry)
    {
        for (int i = 0; i < SIZE_OF(pEntry->answers); ++i)
            if (pEntry->answers[i].lpszAnswerTitle && pEntry->answers[i].bActive)
                return true;
        return false;
    }

    // Весь текстовый блок записи: рамка <nine> + текст + кнопки ответов
    // (или кнопка Continue, если ответов нет / bShowContinue).
    std::string buildEntryHtml(NovelEntry* pEntry, bool bShowContinue)
    {
        auto& cfg = Engine::getCfg();

        LPCTSTR lpszText = nullptr;
        if (_pSelectedAnswer && _pSelectedAnswer->lpszText)
            lpszText = _pSelectedAnswer->lpszText;
        else if (pEntry->lpszSpeakerText)
            lpszText = pEntry->lpszSpeakerText;

        const bool bFrame = (lpszText != nullptr);
        const float fFrameW = cfg.INIT_SCR_CX - _fPicCx - 60.f;

        std::string html;

        if (bFrame)
        {
            html += "<nine src=\"UI/mainIconsFrame\" a=\"67\" b=\"67\" c=\"67\" d=\"67\" width=\"";
            html += std::to_string((int)fFrameW);
            html += "\" padding=\"40,55,40,55\">";
        }

        html += "<font name=\"";
        html += _sFontName;
        html += "\" size=\"";
        html += std::to_string((int)_fTextFontSize);
        html += "\">";

        if (lpszText)
        {
            html += "<p align=\"left\">";
            html += lpszText;
            //html += htmlEscape(lpszText);
            html += "</p>";
        }

        if (bShowContinue)
        {
            html += "<ninebutton id=\"nbContinue\" a=\"20\" b=\"20\" c=\"20\" d=\"20\" src=\"";
            html += NOVEL_BTN_SKIN_NORMAL;
            html += "\" hover=\"";
            html += NOVEL_BTN_SKIN_HL;
            html += "\" pressed=\"";
            html += NOVEL_BTN_SKIN_PRESSED;
            html += "\" margin=\"0,25,0,0\" padding=\"20\" >";
            html += _strContinueText.c_str();
            html += "</ninebutton>";
        }
        else
        {
            for (int i = 0; i < SIZE_OF(pEntry->answers); ++i)
            {
                if (!pEntry->answers[i].lpszAnswerTitle || !pEntry->answers[i].bActive)
                    continue;
                LPCTSTR lpszNormal  = NOVEL_BTN_SKIN_NORMAL;
                LPCTSTR lpszHover   = NOVEL_BTN_SKIN_HL;
                LPCTSTR lpszPressed = NOVEL_BTN_SKIN_PRESSED;
                if (pEntry->answers[i].btnSkin.has_value())
                {
                    lpszNormal  = pEntry->answers[i].btnSkin->lpszNormal;
                    lpszHover   = pEntry->answers[i].btnSkin->lpszHover;
                    lpszPressed = pEntry->answers[i].btnSkin->lpszPressed;
                }
                html += "<ninebutton  a=\"20\" b=\"20\" c=\"20\" d=\"20\" id=\"nbAns";
                html += std::to_string(i);
                html += "\" btngroup=\"ans\" src=\"";
                html += lpszNormal;
                html += "\" hover=\"";
                html += lpszHover;
                html += "\" pressed=\"";
                html += lpszPressed;
                html += "\" margin=\"0,25,0,0\" padding=\"20\" >";
                html += pEntry->answers[i].lpszAnswerTitle;
                html += "</ninebutton>";
            }
        }

        html += "</font>";

        if (bFrame)
            html += "</nine>";

        return html;
    }

    void bindEntryButtons(NovelEntry* pEntry)
    {
        if (!_ptrText)
            return;

        auto rootId = _ptrText->getHTMLRootId();
        auto pDom   = HTMLDom::getInstance();

        for (int i = 0; i < SIZE_OF(pEntry->answers); ++i)
        {
            if (!pEntry->answers[i].lpszAnswerTitle || !pEntry->answers[i].bActive)
                continue;

            std::string sId = "nbAns" + std::to_string(i);

            auto nodeId = pDom->getElementIdById(rootId, sId.c_str());

            if (nodeId)
            {
                pDom->setOnClick(nodeId, [this, pEntry, i](unsigned long long, float, float, int){
                    onAnswerSelected(pEntry, &pEntry->answers[i]);
                });
            }
        }

        auto contId = pDom->getElementIdById(rootId, "nbContinue");

        if (contId)
        {
            pDom->setOnClick(contId, [this, pEntry](unsigned long long, float, float, int){
                onContinue(pEntry);
            });
        }
    }

    void positionText(bool bFrame)
    {
        auto& cfg = Engine::getCfg();

        const float fRegionX = bFrame ? _fPicCx : 0.f;
        const float fRegionW = cfg.INIT_SCR_CX - fRegionX;

        float x = fRegionX + (fRegionW - _ptrText->calcCx()) * 0.5f;
        float y = cfg.INIT_SCR_CY - _ptrText->calcCy();

        if (!bFrame)
            y -= 120.f;   

        _ptrText->setPos(x, y);
    }

    void applyTextHtml(NovelEntry* pEntry, const std::string& html, bool bFrame)
    {
        if (!_ptrText)
        {
            _ptrText = std::make_shared<StaticLabel>();
            _ptrText->setInteractive(true);
            _ptrText->setFont(_sFontName.c_str());
            _ptrText->setFontSize(_fTextFontSize);
            _ptrText->setShadow(false);
            _ptrText->setBoxMode(eTextRenderType::HTML_BOX, Engine::getCfg().INIT_SCR_CX - _fPicCx);
            _ptrContent->addChild(_ptrText);
        }

        _ptrText->setVisible(true);
        _ptrText->setText(html.c_str());
        bindEntryButtons(pEntry);
        positionText(bFrame);
    }

    void fadeInText(float fDelay = 0.f)
    {
        if (!_ptrText)
            return;

        _ptrText->setAlpha(0.f);
        _ptrText->addSelfTween(eTweenProp::ALPHA, 0.f, 0.9f, 0.3f, Easing::linear, fDelay);
    }

    // ------------------------------------------------------------------

    void showSpeakerAndText(NovelEntry* pEntry)
    {
        auto& cfg = Engine::getCfg();
        if (pEntry->plszCenterPic)
        {
            _ptrCenterPic = CGfx::getInstance()->spriteFromTexture(pEntry->plszCenterPic);
            if (_ptrCenterPic)
            {
                _ptrCenterPic->setPos(pEntry->fPicX, pEntry->fPicY);
                _ptrCenterPic->setPivotCentered();
                _ptrCenterPic->setScale(pEntry->fPicScale, pEntry->fPicScale);
                _ptrCenterPic->setAlpha(0.f);
                _ptrContent->addChild(_ptrCenterPic);
                _ptrCenterPic->addSelfTween(eTweenProp::ALPHA, 0.f, 1.f,
                                            0.4f, Easing::linear, 0.f);
            }
        }

        _fPicCx = 0.f;
        bool bWaitAnim = false;
        if (pEntry->spineOpts.lpszSpeakerSpine)
        {
            assert(pEntry->spineOpts.lpszAnimName);
            auto ptrSpine = CSpineManager::getInstance()->getNewSpine(pEntry->spineOpts.lpszSpeakerSpine);
            assert(ptrSpine);
            ptrSpine->setAnimation(0, pEntry->spineOpts.lpszAnimName, true);
            ptrSpine->setPos(pEntry->rcSpinePos.x, pEntry->rcSpinePos.y);
            ptrSpine->setSkipLight(true);
            _fPicCx = pEntry->rcSpinePos.cx;
            _ptrSpeaker = ptrSpine;
            _ptrSpeaker->setPos(-pEntry->rcSpinePos.x, pEntry->rcSpinePos.y);
            _ptrSpeaker->setAlpha(0.f);
            _ptrSpeaker->setScale(pEntry->fSpineScale, pEntry->fSpineScale);
            _ptrContent->addChild(_ptrSpeaker);
            if (pEntry->bNoStartTransition)
            {
                _ptrSpeaker->setPos(pEntry->rcSpinePos.x, pEntry->rcSpinePos.y);
                _ptrSpeaker->setAlpha(1.f);
            }
            else
            {
                _ptrSpeaker->addSelfTween(eTweenProp::X, -pEntry->rcSpinePos.x, pEntry->rcSpinePos.x,
                                            0.6f, Easing::outBack, 0.f);
                _ptrSpeaker->addSelfTween(eTweenProp::ALPHA, 0.f, 1.f,
                                            0.4f, Easing::linear, 0.f);
            }
            if (pEntry->spineOpts.lpszAnimNameSwitch && pEntry->spineOpts.lpszAnimNameNew)
            {
                bWaitAnim = true;
                ptrSpine->setAnimation(0, pEntry->spineOpts.lpszAnimNameSwitch, false);
                ptrSpine->addAnimation(0, pEntry->spineOpts.lpszAnimNameNew,    true);
            }

        }
        else if (pEntry->lpszSpeakerPic)
        {
            _ptrSpeaker = CGfx::getInstance()->spriteFromTexture(pEntry->lpszSpeakerPic);
            std::static_pointer_cast<CSprite>(_ptrSpeaker)->setAutoEffect(pEntry->fEffectType, pEntry->fEffectLifeTime);
            if (_ptrSpeaker)
            {
                _ptrSpeaker->setPos(-300.f, cfg.INIT_SCR_CY / 2.f - 200.f + pEntry->rcSpinePos.y);
                _ptrSpeaker->setAlpha(0.f);
                _ptrContent->addChild(_ptrSpeaker);

                _ptrSpeaker->addSelfTween(eTweenProp::X, -300.f, pEntry->rcSpinePos.x,
                                            0.6f, Easing::outBack, 0.f);
                _ptrSpeaker->addSelfTween(eTweenProp::ALPHA, 0.f, 1.f,
                                            0.4f, Easing::linear, 0.f);
                _fPicCx = _ptrSpeaker->calcNotTransCx() + 100;
            }
        }

        if (pEntry->lpszSpeakerText)
        {
            if (pEntry->lpszSpeach)
            {
                _nPlayingSpeach = AudioManager::get().playSound(pEntry->lpszSpeach);
            }

            applyTextHtml(pEntry, buildEntryHtml(pEntry, !hasActiveAnswers(pEntry)), true);

            if (bWaitAnim)
            {
                _ptrText->setVisible(false);
                auto ptrSpine = std::static_pointer_cast<CSpine>(_ptrSpeaker);
                ptrSpine->onComplete([this, pEntry](CSpine* pOwner, const char* lpccAnimName){
                    if (!strcmp(lpccAnimName, pEntry->spineOpts.lpszAnimNameSwitch))
                    {
                        _ptrText->setVisible(true);
                        fadeInText();
                    }
                });
            }
            else
            {
                fadeInText(0.2f);
            }
        }
        else
        {
            // записи без текста: только кнопки (без рамки), как раньше
            applyTextHtml(pEntry, buildEntryHtml(pEntry, !hasActiveAnswers(pEntry)), false);
            fadeInText(0.2f);
        }
    }

    void onAnswerSelected(NovelEntry* pEntry, NovelAnswer* pAnswer)
    {
        _pSelectedAnswer = pAnswer;

        if (pAnswer && pAnswer->lpszText)
        {
            applyTextHtml(pEntry, buildEntryHtml(pEntry, false), pEntry->lpszSpeakerText != nullptr);

            _ptrText->setAlpha(0.f);
            _ptrText->addSelfTween(eTweenProp::ALPHA, 0.f, 0.9f,
                                   0.3f, Easing::linear, 0.f, [this, pEntry]{
                showContinueButton(pEntry);
            });
        }
        else
        {
            onContinue(pEntry);
        }
    }

    void showContinueButton(NovelEntry* pEntry)
    {
        applyTextHtml(pEntry, buildEntryHtml(pEntry, true), pEntry->lpszSpeakerText != nullptr || (_pSelectedAnswer && _pSelectedAnswer->lpszText));
    }

    void handleAnswerCallback(SimpleCallback cb)
    {
        assert(cb);
        if (_pSelectedAnswer && _pSelectedAnswer->_cbOnSelected)
        {
            _pSelectedAnswer->_cbOnSelected(cb);
        }
        else
        {
            cb();
        }

    }
    void doContinue(NovelEntry* pEntry)
    {
        handleAnswerCallback([this, pEntry]{
            if (_pSelectedAnswer && _pSelectedAnswer->pNext)
            {
                showEntry(_pSelectedAnswer->pNext);
            }
            else
            {
                close();
            }
        });
    }

    void onContinue(NovelEntry* pEntry, bool bDontCheckFinalAnim = false)
    {
        if (!bDontCheckFinalAnim && pEntry->spineOpts.lpszFinalAnimName)
        {
            if (_ptrText)
                _ptrText->setVisible(false);
            auto ptrSpine = std::static_pointer_cast<CSpine>(_ptrSpeaker);
            ptrSpine->setAnimation(0, pEntry->spineOpts.lpszFinalAnimName, false);
            ptrSpine->onComplete([this, pEntry](CSpine* pOwner, const char* lpccAnimName){
                if (!strcmp(lpccAnimName, pEntry->spineOpts.lpszFinalAnimName))
                {
                    onContinue(pEntry, true);
                }
            });
        }
        else if (pEntry->_cbOnEnd)
        {
            pEntry->_cbOnEnd(this, [this, pEntry]{
                doContinue(pEntry);
            });
        }
        else
        {
            doContinue(pEntry);
        }
    }

    virtual void onShow(bool bShow) override
    {
        if (bShow)
            start();
        float fDarken = (_pCurrent && _pCurrent->fWorldDarken > 0.f)
                            ? _pCurrent->fWorldDarken
                            : 0.2f;
        if (!_bNoWorldLighten)
            CGfx::getInstance()->setWorldDarken(bShow, fDarken, fDarken);

        if (!bShow && _bAutoUnloadTextures)
        {
            auto& cfg = Engine::getCfg();
            AudioManager::get().stopSound(_nPlayingSpeach);
            for (auto& it:_entries)
            {
                if (it.lpszBg)
                {
                    auto lpszBg = cfg.LANG == eLang::RU ? it.lpszBg:it.lpszBgEn ? it.lpszBgEn : it.lpszBg;
                    assert(lpszBg);
                    CGfx::getInstance()->unloadTexture(lpszBg);
                }
                if (it.lpszSpeakerPic)
                    CGfx::getInstance()->unloadTexture(it.lpszSpeakerPic);
                if (it.plszCenterPic)
                    CGfx::getInstance()->unloadTexture(it.plszCenterPic);
            }
        }
        else
        {
            CGfx::getInstance()->getPostProcessSettings().fBloodDrops       = 0.f;
            CGfx::getInstance()->getPostProcessSettings().fRainIntensity    = 0.f;
            CGfx::getInstance()->getPostProcessSettings().fleshLUTIntensity = 0.f;
        }
    }
};

_G2D_NAMESPACE_END_