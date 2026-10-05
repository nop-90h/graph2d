#include "pch.h"
#include "dialogdemo.h"
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
#include "panelspritecontdemo.h"
#include "spinecont.h"
#include "htmldom.h"
#include "l10n.h"
#include "bgeffects.h"
#include "bakedspine.h"

#define DLG_CX 1200.f
#define DLG_CY 900.f

namespace
{
    constexpr int SIDE_LEFT  = 0;
    constexpr int SIDE_RIGHT = 1;

    static std::string htmlEscape(const char* s)
    {
        std::string out;

        if (!s)
            return out;

        for (const char* p = s; *p; ++p)
        {
            switch (*p)
            {
                case '<':  out += "&lt;";   break;
                case '>':  out += "&gt;";   break;
                case '&':  out += "&amp;";  break;
                case '"':  out += "&quot;"; break;
                case '\'': out += "&#39;";  break;
                default:   out += *p;       break;
            }
        }

        return out;
    }

    static std::string htmlEscape(const std::string& s)
    {
        return htmlEscape(s.c_str());
    }

    static std::string nl2br(const std::string& s)
    {
        std::string out;
        out.reserve(s.size());

        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '\n')
                out += "<br>";
            else
                out += s[i];
        }

        return out;
    }

    static std::string locHtml(const char* key)
    {
        return nl2br(htmlEscape(L10N::getInstance().tr(key)));
    }

    std::string demoButton(const char* id, const char* caption, int nWidth = 280)
    {
        std::string s;

        s += "<ninebutton id=\"";
        s += id;
        s += "\" src=\"UI/slicedCyanBtn_normal\" hover=\"UI/slicedCyanBtn_hl\""
             " pressed=\"UI/slicedCyanBtn_normal\" a=\"20\" b=\"20\" c=\"20\" d=\"20\""
             " width=\"";
        s += std::to_string(nWidth);
        s += "\" padding=\"40\">"
             "<b><font name=\"edugot\" size=\"28\" color=\"#08272B\" shadow=\"1\" shadowcolor=\"#FFFFFF66\">";
        s += htmlEscape(caption);
        s += "</font></b></ninebutton>";

        return s;
    }

    std::string demoLink(const char* id, const char* text, const char* color)
    {
        std::string s;

        s += "<a id=\"";
        s += id;
        s += "\" hovercolor=\"#FFE9A0FF\" pressedcolor=\"#FFC040FF\" cursor=\"hand\">";
        s += "<u color=\"";
        s += color;
        s += "B0\" thickness=\"2\">";
        s += "<b><font fx=\"ripple\" repeatdelay=\"5\" color=\"";
        s += color;
        s += "\" shadow=\"1\" shadowcolor=\"#000000AA\">";
        s += htmlEscape(text);
        s += "</font></b></u></a>";

        return s;
    }

    std::string demoNewHtmlFeaturesHtml()
    {
        std::string h;

        h += "<center>\n";
        h += "<font name=\"marmelad\" size=\"44\" color=\"#CCBBAA\" shadow=\"1\" shadowcolor=\"#000000AA\" appear=\"rise\" dur=\"0.6\">\n";
        h += htmlEscape(L10N::getInstance().tr("NEW_HTML_TITLE"));
        h += "\n</font>\n</center>\n";



        h += "<font name=\"edugot\" size=\"26\" color=\"#E8E0D0\" shadow=\"1\" shadowcolor=\"#00000088\">\n";

        h += "<p margin=\"20 40\">";
        h += locHtml("NEW_HTML_MARGIN_TEXT");
        h += "</p>\n";

        h += "<center><hr width=\"240\" thickness=\"2\" color=\"#40C4D4\"></center>\n";

        h += "<font lineheight=\"2.0\">";
        h += locHtml("NEW_HTML_LINEHEIGHT_TEXT");
        h += "<br>1<br>2<br>3</font>\n<br><br>\n";

        h += locHtml("NEW_HTML_SPAN_TEXT");
        h += "<br><br>\n";

        h += "<b>inline-img</b>: <img src=\"UI/grenade\" inline=\"1\" width=\"68\" valign=\"middle\" margin=\"0 6\"> ";
        h += locHtml("NEW_HTML_INLINE_IMG_TEXT");
        h += "<br><br>\n";

        h += locHtml("NEW_HTML_HR_TEXT");
        h += "\n<hr thickness=\"1\" color=\"#808080\">\n";

        h += "<b>";
        h += htmlEscape(L10N::getInstance().tr("NEW_HTML_SETINNER_TEXT"));
        h += "</b> ";

        h += demoLink("inner_btn",
                      L10N::getInstance().tr("NEW_HTML_LINK_REPLACE"),
                      "#5ED95E");

        h += "\n<p id=\"inner_target\" margin=\"12 0\"><font color=\"#E8A0A0\">";
        h += locHtml("NEW_HTML_OLD_CONTENT");
        h += "</font></p>\n";

        h += "</font>";

        return h;
    }

    static void copyToClipboard(const char* text)
    {
#ifdef __EMSCRIPTEN__
        EM_ASM(
        {
            var s = UTF8ToString($0);

            if (navigator.clipboard && window.isSecureContext)
            {
                navigator.clipboard.writeText(s);
            }
            else
            {
                var ta = document.createElement("textarea");
                ta.value = s;
                ta.style.position = "fixed";
                ta.style.opacity = "0";
                document.body.appendChild(ta);
                ta.focus();
                ta.select();

                try
                {
                    document.execCommand("copy");
                }
                catch (e)
                {
                }

                ta.remove();
            }
        }, text);
#else
        (void)text;
#endif
    }

    static void showCopyToast(CContainer* pParent, float x, float y)
    {
        if (!pParent)
            return;

        auto ptrToast = std::make_shared<StaticLabel>();

        ptrToast->setBoxMode(eTextRenderType::HTML_BOX, 600.f);

        std::string sToast = std::format(
            "<center><font name=\"edugot\" size=\"26\" color=\"#D7FFE0\" shadow=\"1\" shadowcolor=\"#000000CC\" appear=\"rise\" dur=\"0.25\">{}</font></center>",
            htmlEscape(L10N::getInstance().tr("COPIED_TO_CLIPBOARD")));

        ptrToast->setText(sToast.c_str());

        ptrToast->setPivotCentered();
        ptrToast->setPos(x, y - 20.f);
        ptrToast->setAlpha(0.f);

        pParent->addChild(ptrToast);

        ptrToast->addSelfTween(eTweenProp::ALPHA, 0.f, 1.f, 0.2f, Easing::outQuad);
        ptrToast->addSelfTween(eTweenProp::Y, y - 20.f, y - 60.f, 1.3f, Easing::outQuad);
        ptrToast->addSelfTween(eTweenProp::ALPHA, 1.f, 0.f, 0.4f, Easing::linear, 0.9f,
            [w = ptrToast->weak_from_this()]()
            {
                if (auto p = w.lock())
                    p->removeFromParent();
            });
    }

    class DialogDemoExample final : public BaseDialog
    {
    public:
        bool initExample(const char* lpszTitle,
                         float cx,
                         float cy,
                         BaseDialog* pParent,
                         eScrollType eScroll = eScrollType::E_ST_NONE,
                         bool bCloseButton = true,
                         bool bClickAnywhere = false,
                         bool bPanelBg = false)
        {
            auto ptrSL = SpriteLoader::getInstance();

            Rect rc{ 30.f, 100.f, cx - 60.f, cy - 170.f };
            Rect* pRc = (eScroll != eScrollType::E_ST_NONE) ? &rc : nullptr;

            if (bPanelBg)
            {
                auto ptrNine = std::make_shared<NineSlice>();
                ptrNine->createSlices(ptrSL->getSprite("UI/nineslicedemo2"), 138, 138, 138, 138);

                if (!BaseDialog::init(cx, cy, eScroll, pRc, ptrNine, pParent))
                    return false;
            }
            else
            {
                if (!BaseDialog::init(cx, cy, eScroll, pRc, ptrSL->getSprite("UI/frameBg"), pParent))
                    return false;
            }

            if (bClickAnywhere)
                _eCloseStyle = eDialogCloseStyle::CLICK_ANYWHERE;

            if (!bPanelBg)
                createFgLayer(ptrSL->getSprite("UI/frameFg"), 60, 60, 46, 46);

            if (lpszTitle && *lpszTitle)
            {
                setTitle(lpszTitle, ptrSL->getSprite("UI/frameTitle"), 113, 113);
                _titleCont->setY(-20);
            }

            if (bCloseButton)
            {
                createCloseButton(ptrSL->getSprite("UI/btn_close_normal"),
                                  ptrSL->getSprite("UI/btn_close_pressed"),
                                  ptrSL->getSprite("UI/btn_close_hovered"),
                                  ptrSL->getSprite("UI/btn_close_disabled"));
            }

            return true;
        }

        void setHtml(const char* lpszHtml,
                     float y = 90.f,
                     float widthPad = 40.f)
        {
            _ptrHtmlLabel = std::make_shared<StaticLabel>();

            _ptrHtmlLabel->setBoxMode(eTextRenderType::HTML_BOX, _fDialogCx - widthPad);
            _ptrHtmlLabel->setText(lpszHtml);
            _ptrHtmlLabel->setInteractive(true);
            _ptrHtmlLabel->setXPosCentered(_fDialogCx);
            _ptrHtmlLabel->setY(y);

            _root->addChild(_ptrHtmlLabel);
        }

        void bindClick(const char* id, SimpleCallback cb)
        {
            if (!_ptrHtmlLabel || !cb)
                return;

            _ptrHtmlLabel->getHTMLDocument().onClick(id, [cb] { cb(); });
        }

        void openSidePanel(int eSide)
        {
            if (eSide != SIDE_LEFT && eSide != SIDE_RIGHT)
                return;

            if (_sidePanels[eSide] && _sidePanels[eSide]->isVisible())
            {
                closeSidePanel(eSide);
                return;
            }

            closeSidePanel(eSide, true);

            const float pw = 400.f;
            const float ph = _fDialogCy - 40.f;

            auto panel = std::make_shared<DialogDemoExample>();

            if (!panel->initExample(nullptr, pw, ph, this, eScrollType::E_ST_NONE, false, false, true))
                return;

            std::string sHtml;

            sHtml += "<center>\n";
            sHtml += "<font name=\"edugot\" size=\"26\" color=\"#261C14\" shadow=\"1\" shadowcolor=\"#00000022\" appear=\"rise\" dur=\"0.4\">\n";
            sHtml += locHtml("DLG_SIDE_PANEL_TEXT");
            sHtml += "\n</font>\n<br><br>\n";

            sHtml += demoLink("side_close",
                              L10N::getInstance().tr("BTN_CLOSE"),
                              "#D32F2F");

            sHtml += "\n</center>";

            panel->setHtml(sHtml.c_str(), 95.f, 120.f);

            panel->bindClick("side_close", [this, eSide]()
            {
                closeSidePanel(eSide);
            });

            const float cx0 = (_fDialogCx - pw) * 0.5f;
            const float cy0 = (_fDialogCy - ph) * 0.5f;
            const float ex  = (eSide == SIDE_LEFT) ? -400 : _fDialogCx;

            panel->setPos(cx0, cy0);
            panel->showAsPanel(E_IH_DISPATCHBYPARENT, true);
            panel->bringUnderParent();

            panel->addSelfTween(eTweenProp::X, cx0, ex, 0.45f, Easing::outBack);

            _sidePanels[eSide] = panel;
        }

        void closeSidePanel(int eSide, bool bImmediate = false)
        {
            if (eSide != SIDE_LEFT && eSide != SIDE_RIGHT)
                return;

            auto p = _sidePanels[eSide];
            _sidePanels[eSide].reset();

            if (!p || !p->isVisible())
                return;

            if (bImmediate)
            {
                p->close();
                return;
            }

            const float cx0 = (_fDialogCy - p->getDialogCx()) * 0.5f;

            p->removeSelfTweens();
            p->addSelfTween(eTweenProp::X, p->getX(), cx0, 0.3f, Easing::inCubic, 0.f, [p]()
            {
                p->close();
            });
        }

        void closeAllSidePanels(void)
        {
            for (int i = SIDE_LEFT; i <= SIDE_RIGHT; ++i)
                closeSidePanel(i, true);
        }

        void finalizeScroll()
        {
            if (_eScrollType != eScrollType::E_ST_NONE)
            {
                updateMaxScroll();
                updateScrollBox();
            }
        }

        void autoClose(float fSeconds)
        {
            setTimeout(fSeconds, [w = weak_from_this()]()
            {
                if (auto p = std::static_pointer_cast<BaseDialog>(w.lock()))
                {
                    if (p->isVisible())
                        p->close();
                }
            });
        }

    protected:
        virtual void createScrollShadow(Rect* pRcScroll) override
        {
            (void)pRcScroll;
        }

        virtual void onShow(bool bShow) override
        {
            if (bShow && _frame)
            {
                _frame->setAlpha(0.f);
                _frame->setY(25.f);

                _frame->addSelfTween(eTweenProp::ALPHA,
                                     0.f,
                                     1.f,
                                     0.22f,
                                     Easing::linear);

                _frame->addSelfTween(eTweenProp::Y,
                                     25.f,
                                     0.f,
                                     0.32f,
                                     Easing::outBack);
            }
        }

    private:
        StaticLabelPtr _ptrHtmlLabel;
        BaseDialogPtr  _sidePanels[2];
    };
}

void DialogDemo::initParticlesDemo()
{
    static struct
    {
        LPCTSTR           lpszBtnCaption;
        eParticlePreset   ePreset;
    } arr[] =
    {
        { "FIRE", eParticlePreset::FIRE }, { "SMOKE", eParticlePreset::SMOKE }, { "AMBIENT DUST", eParticlePreset::AMBIENT_DUST },
        { "RAIN", eParticlePreset::RAIN }, { "SNOW", eParticlePreset::SNOW }, { "FALLING LEAVES", eParticlePreset::FALLING_LEAVES },
        { "BUBBLES", eParticlePreset::BUBBLES }, { "SPARKS", eParticlePreset::SPARKS }, { "BLOOD SPLATTER", eParticlePreset::BLOOD_SPLATTER },
        { "DEBRIS", eParticlePreset::DEBRIS }, { "PLASMA TRAIL", eParticlePreset::PLASMA_TRAIL }, { "HEALING", eParticlePreset::HEALING },
        { "POISON", eParticlePreset::POISON }, { "PIXIE DUST", eParticlePreset::PIXIE_DUST }, { "SHADOW VOID", eParticlePreset::SHADOW_VOID },
        { "STEAM", eParticlePreset::STEAM }, { "ELECTRIC SPARKS", eParticlePreset::ELECTRIC_SPARKS }, { "ACID DRIPS", eParticlePreset::ACID_DRIPS },
        { "FIREWORKS", eParticlePreset::FIREWORKS }, { "CONFETTI", eParticlePreset::CONFETTI }, { "UI BUTTON GLOW", eParticlePreset::UI_BUTTON_GLOW },
        { "NEBULA", eParticlePreset::NEBULA }, { "HYPERSPACE", eParticlePreset::HYPERSPACE }, { "ADVANCED SNOW", eParticlePreset::ADVANCED_SNOW },
        { "ADVANCED SPARKS", eParticlePreset::ADVANCED_SPARKS }, { "ROCKET TRAIL", eParticlePreset::ROCKET_TRAIL }, { "PLASMA BALL", eParticlePreset::PLASMA_BALL },
        { "FLAME THROWER", eParticlePreset::FLAMETHROWER }, { "STARLIGHT FOUNTAIN", eParticlePreset::STARLIGHT_FOUNTAIN }, { "ETHEREAL WISP", eParticlePreset::ETHEREAL_WISP },
        { "LASER SPARKS", eParticlePreset::LASER_SPARKS }, { "RAILGUN TRAIL", eParticlePreset::RAILGUN_TRAIL }, { "CORE MELTDOWN", eParticlePreset::CORE_MELTDOWN },
        { "SANDSTORM", eParticlePreset::SANDSTORM }, { "SAKURA", eParticlePreset::SAKURA }, { "GRENADE EXPLOSION", eParticlePreset::GRENADE_EXPLOSION },
        { "MECH EXPLOSION", eParticlePreset::MECH_EXPLOSION }, { "COSMIC NOVA", eParticlePreset::COSMIC_NOVA }, { "HOLY BURST", eParticlePreset::HOLY_BURST },
        { "CRITICAL BLOOD", eParticlePreset::CRITICAL_BLOOD }, { "PROP SMASH", eParticlePreset::PROP_SMASH }, { "ICE NOVA", eParticlePreset::ICE_NOVA },
        { "TOXIC SPORE", eParticlePreset::TOXIC_SPORE }, { "FIREFLIES", eParticlePreset::FIREFLIES }, { "DANDELION FLUFF", eParticlePreset::DANDELION_FLUFF },
        { "SUNBEAM DUST", eParticlePreset::SUNBEAM_DUST }, { "LAKE MIST", eParticlePreset::LAKE_MIST }, { "CUP STEAM", eParticlePreset::CUP_STEAM },
        { "AETHER WISPS", eParticlePreset::AETHER_WISPS }, { "GLOWING SPORES", eParticlePreset::GLOWING_SPORES }, { "PLANKTON", eParticlePreset::PLANKTON },
        { "DISTANT EMBERS", eParticlePreset::DISTANT_EMBERS }, { "BOKEH AMBIENT", eParticlePreset::BOKEH_AMBIENT }
    };

    _ptrParticlesDemo->removeAll();

    for (int i = 0; i < SIZE_OF(arr); i++)
    {
        constexpr float fMinAnimTime = 1.3f;
        constexpr float fMaxAnimTime = 2.3f;

        auto r = rand();

        CContainerPtr ptrBtn;

        auto onClickLambda = [this, ePreset = arr[i].ePreset]
        {
            auto ptrDlg = std::make_shared<DialogParticlesView>();
            ptrDlg->initAndShow(ePreset);
        };

        std::string sHtmlCaption = std::format("<font name=\"condence\" size=\"40\"><b>{}</b></font>", arr[i].lpszBtnCaption);

        switch (r % 3)
        {
            case 0:
                ptrBtn = GreenSlicedButton::makeInst(55, sHtmlCaption.c_str(), onClickLambda, nullptr, true);
                break;

            case 1:
                ptrBtn = CyanSlicedButton::makeInst(55, sHtmlCaption.c_str(), onClickLambda, nullptr, true);
                break;

            case 2:
                ptrBtn = OrangeSlicedButton::makeInst(55, sHtmlCaption.c_str(), onClickLambda, nullptr, true);
                break;

            default:
                assert(false);
                break;
        }

        _ptrParticlesDemo->addChild(ptrBtn);

        float fAnimTime = fMinAnimTime + static_cast<float>(r) / static_cast<float>(RAND_MAX) * (fMaxAnimTime - fMinAnimTime);

        easingFunction ease[] = { Easing::outBack, Easing::outBounce, Easing::outElastic };

        ptrBtn->setAlpha(0);
        ptrBtn->setPivotCentered();

        ptrBtn->addSelfTween(eTweenProp::SCALE, 0.1f, 1.f, fAnimTime, ease[r % SIZE_OF(ease)], 0.01f * i);
        ptrBtn->addSelfTween(eTweenProp::ALPHA, 0, 1.f, fAnimTime, Easing::linear, 0.01f * i);
    }

    _ptrParticlesDemo->packChildrenCascade(getDialogCx() - 80, getDialogCy() - 80, 7.f, true);
    _ptrParticlesDemo->setPosCentered(getDialogCx(), getDialogCy());
}

void DialogDemo::initFontsDemo(void)
{
    if (_ptrTextDemoCont)
        _ptrTextDemoCont->removeAll();
    else
        return;

    _currentTextConfigIdx = 0;

    float padding = 40.0f;

    float maxCx = static_cast<float>(getDialogCx()) - padding;
    float maxCy = static_cast<float>(getDialogCy()) - padding;

    const float spawnBottomY = maxCy - 10.0f;

    float cellWidth  = 35.0f;
    float cellHeight = 30.0f;

    int numCols = std::max(1, static_cast<int>(maxCx / cellWidth));
    int numRows = std::max(1, static_cast<int>(maxCy / cellHeight));

    std::string matrixChars = "0123456789ABCDEFXYZ*#@$%";

    uint32_t greenColors[] = { 0x00FF00FF, 0x32CD32FF, 0x00FF7FFF, 0x008B00FF };

    std::vector<StaticLabelPtr> matrixGrid;
    matrixGrid.reserve(numCols * numRows);

    for (int row = numRows - 1; row >= 0; --row)
    {
        for (int col = 0; col < numCols; ++col)
        {
            StaticLabelPtr ptrMatrix = std::make_shared<StaticLabel>();

            ptrMatrix->setFont("edugot");
            ptrMatrix->setFontSize(static_cast<float>(16 + rand() % 8));
            ptrMatrix->setRgba(greenColors[rand() % 4]);

            std::string singleChar(1, matrixChars[rand() % matrixChars.size()]);

            ptrMatrix->setText(singleChar.c_str());
            ptrMatrix->setAlign(eTRAlign::TR_ALIGN_CENTER | eTRAlign::TR_ALIGN_BOTTOM);
            ptrMatrix->setPivotCentered();

            float posX = (static_cast<float>(col) * cellWidth) + (cellWidth * 0.5f);
            float posY = (static_cast<float>(row) * cellHeight) + (cellHeight * 0.5f);

            ptrMatrix->setPos(posX, posY);
            ptrMatrix->setAlpha(0.0f);
            ptrMatrix->setScale(0.0f, 0.0f);

            _ptrTextDemoCont->addChild(ptrMatrix);
            matrixGrid.push_back(ptrMatrix);

            int topToBottomRowIdx = (numRows - 1) - row;
            float spawnDelay = static_cast<float>(topToBottomRowIdx) * 0.04f;

            ptrMatrix->addSelfTween(eTweenProp::SCALE, 0.0f, 1.0f, 0.25f, Easing::outQuad, spawnDelay);
            ptrMatrix->addSelfTween(eTweenProp::ALPHA, 0.0f, 1.0f, 0.25f, Easing::linear, spawnDelay);
        }
    }

    float matrixHoldTime = (static_cast<float>(numRows) * 0.04f) + 1.2f;
    float dropDuration   = 1.8f;

    size_t triggerElementIdx = matrixGrid.size() - 1;

    for (size_t i = 0; i < matrixGrid.size(); ++i)
    {
        auto ptrMatrix = matrixGrid[i];

        Point currentPos;
        ptrMatrix->getPos(&currentPos);

        int colIdx = static_cast<int>(currentPos.x / cellWidth);

        float cascadeDelay = matrixHoldTime + (static_cast<float>(colIdx % 6) * 0.12f);
        float targetDropY = currentPos.y + 500.0f;

        if (i == triggerElementIdx)
        {
            ptrMatrix->addSelfTween(eTweenProp::Y,
                                    currentPos.y,
                                    targetDropY,
                                    dropDuration,
                                    Easing::inCubic,
                                    cascadeDelay);

            ptrMatrix->addSelfTween(eTweenProp::ALPHA,
                                    1.0f,
                                    0.0f,
                                    dropDuration,
                                    Easing::linear,
                                    cascadeDelay,
                                    [this, w = ptrMatrix->weak_from_this(), spawnBottomY]()
                                    {
                                        if (auto ptr = std::static_pointer_cast<StaticLabel>(w.lock()))
                                        {
                                            auto parent = ptr->getParent();

                                            if (parent)
                                                parent->removeChild(ptr);

                                            this->spawnScrollText(_currentTextConfigIdx, spawnBottomY);

                                            _currentTextConfigIdx = (_currentTextConfigIdx + 1) % 15;
                                        }
                                    });
        }
        else
        {
            ptrMatrix->addSelfTween(eTweenProp::Y,
                                    currentPos.y,
                                    targetDropY,
                                    dropDuration,
                                    Easing::inCubic,
                                    cascadeDelay);

            ptrMatrix->addSelfTween(eTweenProp::ALPHA,
                                    1.0f,
                                    0.0f,
                                    dropDuration,
                                    Easing::linear,
                                    cascadeDelay,
                                    [w = ptrMatrix->weak_from_this()]()
                                    {
                                        if (auto ptr = std::static_pointer_cast<StaticLabel>(w.lock()))
                                        {
                                            auto parent = ptr->getParent();

                                            if (parent)
                                                parent->removeChild(ptr);
                                        }
                                    });
        }
    }
}

void DialogDemo::spawnScrollText(size_t configIdx, float startY)
{
    struct TextDemoConfig
    {
        LPCTSTR    text;
        LPCTSTR    fontName;
        float      fontSize;
        uint32_t   colorHex;
    };

    static std::vector<TextDemoConfig> textConfigs =
    {
        { L10N::getInstance().tr("POEM_01"), "greengoth",     36.0f + 10, 0xFF4500FF },
        { L10N::getInstance().tr("POEM_02"), "creepster",     30.0f + 10, 0x00FFFFFF },
        { L10N::getInstance().tr("POEM_03"), "trigramlight",  34.0f + 10, 0x9A32CDFF },
        { L10N::getInstance().tr("POEM_04"), "edugot",        24.0f + 10, 0x32CD32FF },
        { L10N::getInstance().tr("POEM_05"), "secretorigins", 40.0f + 10, 0xFFFFFFFF },
        { L10N::getInstance().tr("POEM_06"), "edugot",        34.0f + 10, 0xFFD700FF },
        { L10N::getInstance().tr("POEM_07"), "trigramlight",  28.0f + 10, 0xDEB887FF },
        { L10N::getInstance().tr("POEM_08"), "creepster",     38.0f + 10, 0xFF00FFFF },
        { L10N::getInstance().tr("POEM_09"), "greengoth",     30.0f + 10, 0x708090FF },
        { L10N::getInstance().tr("POEM_10"), "secretorigins", 34.0f + 10, 0x00FF7FFF },
        { L10N::getInstance().tr("POEM_11"), "edugot",        28.0f + 10, 0xFF8C00FF },
        { L10N::getInstance().tr("POEM_12"), "greengoth",     42.0f + 10, 0xADFF2FFF },
        { L10N::getInstance().tr("POEM_13"), "greengoth",     32.0f + 10, 0xFF6347FF },
        { L10N::getInstance().tr("POEM_14"), "creepster",     26.0f + 10, 0x7FFFD4FF },
        { L10N::getInstance().tr("POEM_15"), "trigramlight",  36.0f + 10, 0xBA55D3FF }
    };

    if (configIdx >= textConfigs.size() || !_ptrTextDemoCont)
        return;

    const auto& config = textConfigs[configIdx];

    float maxCx = static_cast<float>(getDialogCx()) - 40.0f;
    float localCenterX = maxCx * 0.5f;

    const float scrollSpeed = 70.0f;
    const float targetTopY  = 30.0f;
    const float gapY        = 24.0f;

    float labelHeight = config.fontSize * 1.35f;
    float lineTotalStep = labelHeight + gapY;

    StaticLabelPtr ptrLabel = std::make_shared<StaticLabel>();

    ptrLabel->setFont(config.fontName);
    ptrLabel->setFontSize(config.fontSize);
    ptrLabel->setRgba(config.colorHex);
    ptrLabel->setShadow(true);
    ptrLabel->setText(config.text);
    ptrLabel->setAlign(eTRAlign::TR_ALIGN_CENTER | eTRAlign::TR_ALIGN_BOTTOM);
    ptrLabel->setPivotCentered();
    ptrLabel->setPos(localCenterX, startY);
    ptrLabel->setAlpha(0.0f);

    _ptrTextDemoCont->addChild(ptrLabel);

    float introDuration = 0.55f;

    int animationType = static_cast<int>(configIdx) % 4;

    if (animationType == 0)
    {
        ptrLabel->rotate(-1.0f);
        ptrLabel->setScale(0.2f, 0.2f);

        ptrLabel->addSelfTween(eTweenProp::SCALE, 0.2f, 1.0f, introDuration, Easing::outBack);
        ptrLabel->addSelfTween(eTweenProp::ROTATE, -1.0f, 0.0f, introDuration, Easing::outBack);
        ptrLabel->addSelfTween(eTweenProp::ALPHA, 0.0f, 1.0f, introDuration * 0.5f, Easing::linear);
    }
    else if (animationType == 1)
    {
        ptrLabel->setScale(0.0f, 0.0f);

        ptrLabel->addSelfTween(eTweenProp::SCALE, 0.0f, 1.0f, introDuration + 0.15f, Easing::outQuad);
        ptrLabel->addSelfTween(eTweenProp::ALPHA, 0.0f, 1.0f, introDuration + 0.15f, Easing::outQuad);
    }
    else if (animationType == 2)
    {
        float startLeftX = -250.0f;

        ptrLabel->setPos(startLeftX, startY);

        ptrLabel->addSelfTween(eTweenProp::X, startLeftX, localCenterX, introDuration, Easing::outCubic);
        ptrLabel->addSelfTween(eTweenProp::ALPHA, 0.0f, 1.0f, introDuration * 0.6f, Easing::linear);
    }
    else
    {
        ptrLabel->setScale(2.4f, 2.4f);

        ptrLabel->addSelfTween(eTweenProp::SCALE, 2.4f, 1.0f, introDuration, Easing::outQuart);
        ptrLabel->addSelfTween(eTweenProp::ALPHA, 0.0f, 1.0f, introDuration * 0.4f, Easing::linear);
    }

    float intermediateY = startY - lineTotalStep;
    float durationPhase1 = lineTotalStep / scrollSpeed;

    ptrLabel->addSelfTween(eTweenProp::Y,
                           startY,
                           intermediateY,
                           durationPhase1,
                           Easing::linear,
                           0.f,
                           [this, intermediateY, targetTopY, scrollSpeed, w = ptrLabel->weak_from_this()]()
                           {
                               if (auto ptr = std::static_pointer_cast<StaticLabel>(w.lock()))
                               {
                                   float maxCy = static_cast<float>(getDialogCy()) - 40.0f;

                                   this->spawnScrollText(_currentTextConfigIdx, maxCy - 10.0f);

                                   _currentTextConfigIdx = (_currentTextConfigIdx + 1) % 15;

                                   float remainingDistance = intermediateY - targetTopY;
                                   float durationPhase2 = remainingDistance / scrollSpeed;

                                   if (durationPhase2 > 0.0f)
                                   {
                                       ptr->addSelfTween(eTweenProp::Y,
                                                         intermediateY,
                                                         targetTopY,
                                                         durationPhase2,
                                                         Easing::linear);

                                       ptr->addSelfTween(eTweenProp::ALPHA,
                                                         1.0f,
                                                         0.0f,
                                                         durationPhase2 * 0.35f,
                                                         Easing::linear,
                                                         durationPhase2 * 0.65f,
                                                         [w]()
                                                         {
                                                             if (auto ptr = std::static_pointer_cast<StaticLabel>(w.lock()))
                                                             {
                                                                 auto parent = ptr->getParent();

                                                                 if (parent)
                                                                     parent->removeChild(ptr);
                                                             }
                                                         });
                                   }
                               }
                           });
}

void DialogDemo::initSlotsDemo()
{
    if (!_ptrSlotsDemoCont)
        return;

    _ptrSlotsDemoCont->removeAll();

    _ptrSlotMachine = std::make_shared<CSlotMachine>();
    _ptrSlotMachine->initMachine(110.f, 120.f);

    _ptrSlotsDemoCont->addChild(_ptrSlotMachine);

    _ptrSlotMachine->setXPosCentered(getDialogCx());

    auto ptrSprSolid = SpriteLoader::getInstance()->getSprite("UI/mainIconsFrame");

    NineSlicePtr ptrFrameBot = std::make_shared<NineSlice>();

    ptrFrameBot->createSlices(ptrSprSolid, 67, 67, 67, 67);
    ptrFrameBot->build(_ptrSlotMachine->calcNotTransCx(), 140.f);

    _ptrSlotsDemoCont->addChild(ptrFrameBot);

    ptrFrameBot->setXPosCentered(getDialogCx());
    ptrFrameBot->setY(getDialogCy() - 180.f);
    ptrFrameBot->setAlpha(.8f);

    auto ptrSpinBtn = OrangeSlicedButton::makeInst(70, L10N::getInstance().tr("BTN_SPIN"), [this]()
    {
        if (_ptrSlotMachine->isSpinning())
            return;

        std::vector<int> fakeResult(15);

        for (int i = 0; i < 15; ++i)
            fakeResult[i] = rand() % 16;

        _ptrSlotMachine->spin();
    });

    ptrSpinBtn->setPosCentered(ptrFrameBot->calcNotTransCx(), ptrFrameBot->calcNotTransCy());

    ptrFrameBot->addChild(ptrSpinBtn);
}

void DialogDemo::initCardsDemo()
{
    if (!_ptrCardsDemoCont)
        return;

    _ptrCardsDemoCont->removeAll();

    _nCardsDragged = 0;

    CardsArray faces = {};
    CardsArray suits = {};

    faces[0] = SpriteLoader::getInstance()->getSprite("CARDS/card_joker");
    faces[1] = SpriteLoader::getInstance()->getSprite("CARDS/card_king");
    faces[2] = SpriteLoader::getInstance()->getSprite("CARDS/card_queen");
    faces[3] = SpriteLoader::getInstance()->getSprite("CARDS/card_jack");
    faces[4] = SpriteLoader::getInstance()->getSprite("CARDS/card_10");

    suits[0] = SpriteLoader::getInstance()->getSprite("CARDS/card_suit");
    suits[1] = suits[0]->cloneInitial();
    suits[2] = suits[0]->cloneInitial();
    suits[3] = suits[0]->cloneInitial();
    suits[4] = suits[0]->cloneInitial();

    float sceneW = getDialogCx();
    float sceneH = getDialogCy();

    auto ptrCards = std::make_shared<CCardReveal>(sceneW, 303.f);

    ptrCards->setCards(faces, suits);
    ptrCards->setYPosCentered(getDialogCy());

    _ptrCardsDemoCont->addChild(ptrCards);

    ptrCards->play([this, w = ptrCards->weak_from_this(), wc = ptrCards->weak_from_this()]
    {
        if (auto p = w.lock())
        {
            p->addSelfTween(eTweenProp::Y,
                            p->getY(),
                            100.f,
                            1,
                            Easing::outBack,
                            0.f,
                            [this, w, wc]
                            {
                                if (auto p = w.lock())
                                {
                                    std::string sHint = std::format(
                                        "<font size=\"64\" appear=\"shatter\" dur=\"1.5\" color=\"#FFE3A3\">{}</font>",
                                        htmlEscape(L10N::getInstance().tr("CARDS_HINT")));

                                    auto ptrLabel = std::make_shared<StaticLabel>();

                                    ptrLabel->setBoxMode(eTextRenderType::HTML_BOX, getDialogCx() - 100);
                                    ptrLabel->setText(sHint.c_str());
                                    ptrLabel->setY(p->getY() + p->calcNotTransCy() + 20.f);
                                    ptrLabel->setXPosCentered(getDialogCx());

                                    _ptrCardsDemoCont->addChild(ptrLabel);

                                    setTimeout(1.5f, [this, w = ptrLabel->weak_from_this(), wc]
                                    {
                                        if (auto p = std::static_pointer_cast<StaticLabel>(w.lock()))
                                        {
                                            if (auto pc = std::static_pointer_cast<CCardReveal>(wc.lock()))
                                            {
                                                _ptrArrow = SpriteLoader::getInstance()->getSprite("UI/hintArrow");

                                                _ptrCardsDemoCont->addChild(_ptrArrow);

                                                _ptrArrow->setScale(0.5f, 0.5f);
                                                _ptrArrow->setXPosCentered(getDialogCx());
                                                _ptrArrow->setY(p->getY() + p->calcNotTransCy() + 10.f);

                                                constexpr float fLoopTime = 1.f;

                                                _ptrArrow->addSelfTweenEx(eTweenProp::Y,
                                                                          _ptrArrow->getY(),
                                                                          _ptrArrow->getY() + 30.f,
                                                                          fLoopTime,
                                                                          Easing::inBackSoft,
                                                                          0,
                                                                          {},
                                                                          nullptr,
                                                                          eTweenLoopMode::YOYO,
                                                                          -1,
                                                                          Easing::outBackSoft);

                                                p->addSelfTweenEx(eTweenProp::Y,
                                                                  p->getY(),
                                                                  p->getY() + 30.f,
                                                                  fLoopTime,
                                                                  Easing::inBackSoft,
                                                                  .1f,
                                                                  {},
                                                                  nullptr,
                                                                  eTweenLoopMode::YOYO,
                                                                  -1,
                                                                  Easing::outBackSoft);

                                                auto ptrCont = std::make_shared<CContainer>();

                                                auto ptrPlaceHolder = SpriteLoader::getInstance()->getSprite("CARDS/card_suit");

                                                for (int i = 0; i < 5; i++)
                                                {
                                                    auto ph = ptrPlaceHolder->cloneInitial();

                                                    ph->setTint(0.1, 0.1, 0.1);

                                                    ptrCont->addChild(ph);

                                                    ph->setScale(0.5f, 0.5f);

                                                    auto ptrFace = pc->getCardFace(i);

                                                    ptrFace->setDragOffset(ptrFace->getNotTransCx() * .5f, ptrFace->getNotTransCy());
                                                    ptrFace->setInteractive(true);

                                                    ptrFace->setDragable({}, [](CContainerPtr ptrDragObj, float x, float y)
                                                    {
                                                        return true;
                                                    });

                                                    ph->setOnDragDropAccept([this, w = ph->weak_from_this()](CContainerPtr droppedObj)
                                                    {
                                                        if (auto ph = w.lock())
                                                        {
                                                            return !ph->getChildrenCount();
                                                        }

                                                        return false;
                                                    });

                                                    ph->setInteractive(true);

                                                    ph->setOnDragDropped([this, w, ww = ph->weak_from_this()](float x, float y, CContainerPtr droppedObj)
                                                    {
                                                        if (auto ph = ww.lock())
                                                        {
                                                            ph->setTint(1.f, 1.f, 1.f);

                                                            droppedObj->setPos(0, 0);

                                                            ph->addChild(droppedObj);

                                                            _nCardsDragged++;

                                                            if (_nCardsDragged == 5)
                                                            {
                                                                if (auto ptrLabel = std::static_pointer_cast<StaticLabel>(w.lock()))
                                                                {
                                                                    _ptrArrow->setVisible(false);

                                                                    std::string sNice = std::format(
                                                                        "<font size=\"90\" appear=\"cascade\" dur=\"1.5\" color=\"#70FF90\">{}</font>",
                                                                        htmlEscape(L10N::getInstance().tr("CARDS_EXCELLENT")));

                                                                    ptrLabel->removeSelfTweens();
                                                                    ptrLabel->setText(sNice.c_str());
                                                                    ptrLabel->setXPosCentered(getDialogCx());
                                                                }
                                                            }
                                                        }
                                                    });
                                                }

                                                ptrCont->alignChildren(eChildrenAlign::HORIZONTAL, 20, getDialogCx());
                                                ptrCont->setY(getDialogCy() - (ptrCont->calcNotTransCy() + 50));
                                                ptrCont->setXPosCentered(getDialogCx());
                                                ptrCont->setAlpha(0);
                                                ptrCont->addSelfTween(eTweenProp::ALPHA, 0, 1.f, 0.5f);

                                                _ptrCardsDemoCont->addChild(ptrCont);
                                            }
                                        }
                                    });
                                }
                            });
        }
    });
}

void DialogDemo::initSpineAndLight()
{
    auto bgSpr = CGfx::getInstance()->spriteFromTexture(
        CGfx::getInstance()->getTextureById("BBB/shiz_bg.png"));

    bgSpr->setNormalMap(CGfx::getInstance()->getTextureById("BBB/shiz_bg_n.png"));
    bgSpr->setScaleTo(getDialogCx(), getDialogCy());
    bgSpr->setScale(1, 1);
    bgSpr->setSkipLight(false);

    _ptrSpineDemoCont->addChild(bgSpr);

    auto pMan = CSpineManager::getInstance();

    _ptrLamp = pMan->getNewSpine("BBB/streetlamp", true);

    _ptrLamp->setLightAttachment("light", true);
    _ptrLamp->setLightAttachment("lightspot", true);
    _ptrLamp->setLightAttachment("lightspot2", true);

    auto ptrProg = pMan->getNewSpine("BBB/programmer", true);

    _ptrLamp->setScale(0.65, 0.65);
    _ptrLamp->setPos(400, 700);
    _ptrLamp->setAnimation(0, "animation", true);

    ptrProg->setAnimation(0, "animation", true);
    ptrProg->setScale(0.45, 0.45);

    _ptrSpineDemoCont->addChild(_ptrLamp);
    _ptrSpineDemoCont->addChild(ptrProg);

    ptrProg->setPos(650, 700);
}

void DialogDemo::initBakedSpine()
{
    _ptrBakedSpines = std::make_shared<CContainer>();

    auto ptrCount = std::make_shared<int>(0);
    auto ptrShown = std::make_shared<std::set<int>>();

    auto ptrCountLabel = std::make_shared<StaticLabel>();

    ptrCountLabel->setBoxMode(eTextRenderType::HTML_BOX, getDialogCx() - 80.f);

    auto updateCountLabel = [ptrCountLabel, ptrCount]()
    {
        if (!ptrCountLabel)
            return;

        std::string sText = std::vformat(L10N::getInstance().tr("BAKED_RUNNING_FMT"), std::make_format_args(*ptrCount));

        ptrCountLabel->setText(std::format(
            "<center><font name=\"edugot\" size=\"34\" color=\"#9ADCFF\" shadow=\"1\" shadowcolor=\"#000000AA\">"
            "<b><font color=\"#FFE9A0\">{}</font></b></font></center>",
            htmlEscape(sText)).c_str());
    };

    auto showMilestone = [this](int nCount, const char* lpszText)
    {
        if (!_ptrBakedDemoCont)
            return;

        const int nTier = (nCount >= 2000) ? 2 : (nCount >= 1000) ? 1 : 0;

        auto ptrBig = std::make_shared<StaticLabel>();

        ptrBig->setBoxMode(eTextRenderType::HTML_BOX, getDialogCx() - 60.f);

        std::string sHtml;

        switch (nTier)
        {
            case 2:
                sHtml = std::format(
                    "<center><font name=\"edugot\" size=\"132\" color=\"#FFE9A0\" shadow=\"2\" shadowcolor=\"#000000FF\""
                    " appear=\"comet\" dur=\"1.2\" fx=\"rainbow\"><b>{}</b></font></center>",
                    htmlEscape(lpszText));
                break;

            case 1:
                sHtml = std::format(
                    "<center><font name=\"edugot\" size=\"120\" color=\"#FFD54F\" shadow=\"1\" shadowcolor=\"#000000EE\""
                    " appear=\"shatter\" dur=\"1.8\" fx=\"fire\"><b>{}</b></font></center>",
                    htmlEscape(lpszText));
                break;

            default:
                sHtml = std::format(
                    "<center><font name=\"edugot\" size=\"86\" color=\"#010161\" shadow=\"1\" shadowcolor=\"#000000EE\""
                    " appear=\"rise\" dur=\"0.8\"><b>{}</b></font></center>",
                    htmlEscape(lpszText));
                break;
        }

        ptrBig->setText(sHtml.c_str());
        ptrBig->setXPosCentered(getDialogCx());
        ptrBig->setY(getDialogCy() * (nTier == 2 ? 0.34f : 0.40f));
        ptrBig->setScale(0.2f, 0.2f);
        ptrBig->setAlpha(0.f);

        _ptrBakedDemoCont->addChild(ptrBig);

        const float fHold = (nTier == 2) ? 6.5f : (nTier == 1) ? 5.2f : 2.6f;
        const float fBaseX = ptrBig->getX();
        const float fBaseY = ptrBig->getY();

        ptrBig->addSelfTween(eTweenProp::ALPHA, 0.f, 1.f, 0.3f, Easing::outQuad);
        ptrBig->addSelfTween(eTweenProp::SCALE, 0.2f, 1.f, 0.5f, Easing::outBack);

        if (nTier == 2)
        {
            auto ptrFlash = SpriteLoader::getInstance()->getSprite("UI/whitebox");

            if (ptrFlash)
            {
                ptrFlash->setBlendMode(eSpriteBlendMode::ADDITIVE);
                ptrFlash->setScaleTo(getDialogCx(), getDialogCy());
                ptrFlash->setPivotCentered();
                ptrFlash->setPos(getDialogCx() * 0.5f, getDialogCy() * 0.5f);
                ptrFlash->setAlpha(0.f);

                _ptrBakedDemoCont->addChild(ptrFlash);

                ptrFlash->addSelfTween(eTweenProp::ALPHA, 0.f, 0.9f, 0.07f, Easing::outQuad);
                ptrFlash->addSelfTween(eTweenProp::ALPHA, 0.9f, 0.f, 1.1f, Easing::outQuad, 0.08f,
                    [w = ptrFlash->weak_from_this()]()
                    {
                        if (auto p = w.lock())
                            p->removeFromParent();
                    });
            }

            for (int i = 0; i < 2; ++i)
            {
                auto ptrRing = SpriteLoader::getInstance()->getSprite("UI/whitebox");

                if (!ptrRing)
                    break;

                ptrRing->setBlendMode(eSpriteBlendMode::ADDITIVE);
                ptrRing->setPivotCentered();

                const float fSize = 300.f + i * 160.f;

                ptrRing->setScaleTo(fSize, fSize);

                const float fS = std::min(ptrRing->getScaleX(), ptrRing->getScaleY());

                ptrRing->setPos(getDialogCx() * 0.5f, getDialogCy() * 0.5f);
                ptrRing->setAlpha(0.f);

                _ptrBakedDemoCont->addChild(ptrRing);

                const float fDelay = 1.0f + i * 0.18f;

                ptrRing->addSelfTween(eTweenProp::SCALE, fS * 0.3f, fS * 5.5f, 1.0f, Easing::outCubic, fDelay);
                ptrRing->addSelfTween(eTweenProp::ALPHA, 0.85f, 0.f, 1.0f, Easing::outQuad, fDelay,
                    [w = ptrRing->weak_from_this()]()
                    {
                        if (auto p = w.lock())
                            p->removeFromParent();
                    });
            }

            ptrBig->addSelfTween(eTweenProp::SCALE, 1.f, 2.8f, 0.35f, Easing::outQuad, 0.45f);
            ptrBig->addSelfTween(eTweenProp::SCALE, 2.8f, 1.0f, 1.4f, Easing::outElastic, 0.80f);

            ptrBig->rotate(-12.566f);
            ptrBig->addSelfTween(eTweenProp::ROTATE, -12.566f, 0.f, 1.6f, Easing::outCubic, 0.45f);

            float fPrevOff = 0.f;

            for (int i = 0; i < 8; ++i)
            {
                const float fOff    = (i % 2 == 0) ? 1.f : -1.f;
                const float fShakeX = fOff * randomRange(10.f, 22.f);
                const float fShakeY = fOff * randomRange(6.f, 14.f);
                const float fT      = 2.3f + i * 0.08f;

                ptrBig->addSelfTween(eTweenProp::X, fBaseX + fPrevOff * 16.f, fBaseX + fShakeX, 0.045f, Easing::linear, fT);
                ptrBig->addSelfTween(eTweenProp::Y, fBaseY - fPrevOff * 10.f, fBaseY + fShakeY, 0.045f, Easing::linear, fT);

                fPrevOff = fOff;
            }

            ptrBig->addSelfTween(eTweenProp::X, fBaseX + fPrevOff * 16.f, fBaseX, 0.06f, Easing::outQuad, 2.3f + 8 * 0.08f);
            ptrBig->addSelfTween(eTweenProp::Y, fBaseY - fPrevOff * 10.f, fBaseY, 0.06f, Easing::outQuad, 2.3f + 8 * 0.08f);
        }
        else if (nTier == 1)
        {
            auto ptrFlash = SpriteLoader::getInstance()->getSprite("UI/whitebox");

            if (ptrFlash)
            {
                ptrFlash->setBlendMode(eSpriteBlendMode::ADDITIVE);
                ptrFlash->setScaleTo(getDialogCx(), getDialogCy());
                ptrFlash->setPivotCentered();
                ptrFlash->setPos(getDialogCx() * 0.5f, getDialogCy() * 0.5f);
                ptrFlash->setAlpha(0.f);

                _ptrBakedDemoCont->addChild(ptrFlash);

                ptrFlash->addSelfTween(eTweenProp::ALPHA, 0.f, 0.65f, 0.08f, Easing::outQuad);
                ptrFlash->addSelfTween(eTweenProp::ALPHA, 0.65f, 0.f, 0.9f, Easing::outQuad, 0.1f,
                    [w = ptrFlash->weak_from_this()]()
                    {
                        if (auto p = w.lock())
                            p->removeFromParent();
                    });
            }

            ptrBig->addSelfTween(eTweenProp::SCALE, 1.f, 1.8f, 0.28f, Easing::outQuad, 0.45f);
            ptrBig->addSelfTween(eTweenProp::SCALE, 1.8f, 1.0f, 1.1f, Easing::outElastic, 0.73f);

            ptrBig->rotate(-0.2f);
            ptrBig->addSelfTween(eTweenProp::ROTATE, -0.2f, 0.05f, 0.6f, Easing::outBack, 0.45f);
            ptrBig->addSelfTween(eTweenProp::ROTATE, 0.05f, 0.f, 0.8f, Easing::outElastic, 1.05f);

            float fPrevOff = 0.f;

            for (int i = 0; i < 6; ++i)
            {
                const float fOff    = (i % 2 == 0) ? 1.f : -1.f;
                const float fShakeX = fOff * randomRange(6.f, 14.f);
                const float fShakeY = fOff * randomRange(3.f, 8.f);
                const float fT      = 1.5f + i * 0.09f;

                ptrBig->addSelfTween(eTweenProp::X, fBaseX + fPrevOff * 10.f, fBaseX + fShakeX, 0.05f, Easing::linear, fT);
                ptrBig->addSelfTween(eTweenProp::Y, fBaseY - fPrevOff * 6.f,  fBaseY + fShakeY, 0.05f, Easing::linear, fT);

                fPrevOff = fOff;
            }

            ptrBig->addSelfTween(eTweenProp::X, fBaseX + fPrevOff * 10.f, fBaseX, 0.06f, Easing::outQuad, 1.5f + 6 * 0.09f);
            ptrBig->addSelfTween(eTweenProp::Y, fBaseY - fPrevOff * 6.f,  fBaseY, 0.06f, Easing::outQuad, 1.5f + 6 * 0.09f);
        }
        else
        {
            ptrBig->addSelfTween(eTweenProp::SCALE, 1.f, 1.12f, 0.3f, Easing::outQuad, 0.55f);
            ptrBig->addSelfTween(eTweenProp::SCALE, 1.12f, 1.f, 0.35f, Easing::inOutQuad, 0.85f);
        }

        ptrBig->addSelfTween(eTweenProp::ALPHA, 1.f, 0.f, 0.6f, Easing::inQuad, fHold,
            [w = ptrBig->weak_from_this()]()
            {
                if (auto p = w.lock())
                    p->removeFromParent();
            });

        ptrBig->addSelfTween(eTweenProp::SCALE, 1.f, (nTier == 2) ? 1.6f : 1.35f, 0.6f, Easing::inQuad, fHold);
    };

    auto createSpines = [this, ptrCount, ptrShown, updateCountLabel, showMilestone](CContainerPtr ptrTo, int numOfSpines)
    {
        if (numOfSpines > 0)
        {
            auto ptrBarbTex = CGfx::getInstance()->getTextureById("BBB/barbarian.png");

            for (auto i = 0; i < numOfSpines; i++)
            {
                static const char* fileNames[] = { "BBB/barbarian_idle.panm", "BBB/barbarian_run.panm" };
                static const char* animNames[] = { "idle", "run" };

                auto ptrSpine = std::make_shared<CBakedSpine>(fileNames, animNames, ptrBarbTex);

                ptrSpine->setAnimationByName(getRandomElement(animNames), true);
                ptrSpine->setPos(fastRandU32() % 900 + 100, fastRandU32() % 500 + 300);
                ptrSpine->setSkipLight(true);

                ptrTo->addChild(ptrSpine);
            }

            *ptrCount += numOfSpines;

            updateCountLabel();

            static const std::pair<int, const char*> arrMilestones[] =
            {
                { 100,  L10N::getInstance().tr("MILESTONE_100")  },
                { 200,  L10N::getInstance().tr("MILESTONE_200")  },
                { 300,  L10N::getInstance().tr("MILESTONE_300")  },
                { 500,  L10N::getInstance().tr("MILESTONE_500")  },
                { 1000, L10N::getInstance().tr("MILESTONE_1000") },
                { 2000, L10N::getInstance().tr("MILESTONE_2000") },
            };

            for (const auto& ms : arrMilestones)
            {
                if (*ptrCount >= ms.first && !ptrShown->count(ms.first))
                {
                    ptrShown->insert(ms.first);
                    showMilestone(ms.first, ms.second);
                }
            }
        }
        else
        {
            ptrTo->removeAll();

            *ptrCount = 0;
            ptrShown->clear();

            updateCountLabel();
        }
    };

    auto ptrBtn1 = OrangeSlicedButton::makeInst(
        55,
        L10N::getInstance().tr("BAKED_ADD1"),
        [createSpines, ptrTo = _ptrBakedSpines->weak_from_this()]
        {
            if (auto p = ptrTo.lock())
                createSpines(p, 1);
        },
        nullptr,
        true);

    auto ptrBtn10 = OrangeSlicedButton::makeInst(
        55,
        L10N::getInstance().tr("BAKED_ADD10"),
        [createSpines, ptrTo = _ptrBakedSpines->weak_from_this()]
        {
            if (auto p = ptrTo.lock())
                createSpines(p, 10);
        },
        nullptr,
        true);

    auto ptrBtnClear = OrangeSlicedButton::makeInst(
        55,
        L10N::getInstance().tr("BAKED_CLEAR"),
        [createSpines, ptrTo = _ptrBakedSpines->weak_from_this()]
        {
            if (auto p = ptrTo.lock())
                createSpines(p, 0);
        },
        nullptr,
        true);

    auto ptrBtnsCont = std::make_shared<CContainer>();

    ptrBtnsCont->addChild(ptrBtn1);
    ptrBtnsCont->addChild(ptrBtn10);
    ptrBtnsCont->addChild(ptrBtnClear);
    ptrBtnsCont->addChild(ptrCountLabel);

    ptrBtnsCont->alignChildren(eChildrenAlign::HORIZONTAL, 20.f, getDialogCx());

    updateCountLabel();

    _ptrBakedSpines->setPos(0, 0);

    _ptrBakedDemoCont->addChild(_ptrBakedSpines);
    _ptrBakedDemoCont->addChild(ptrBtnsCont);

    ptrBtnsCont->setXPosCentered(getDialogCx());
    ptrBtnsCont->setY(getDialogCy() - (ptrBtnsCont->calcNotTransCy() + 50));
}

bool DialogDemo::init()
{
    float cx = DLG_CX;
    float cy = DLG_CY;

    CSpritePtr pFrameBg = SpriteLoader::getInstance()->getSprite("UI/frameBg");

    bool bRes = BaseDialog::init(cx, cy, eScrollType::E_ST_NONE, NULL, pFrameBg);

    if (bRes)
    {
        _ptrParticlesDemo   = std::make_shared<CContainer>();
        _ptrTextDemoCont    = std::make_shared<CContainer>();
        _ptrSlotsDemoCont   = std::make_shared<CContainer>();
        _ptrCardsDemoCont   = std::make_shared<CContainer>();
        _ptrBakedDemoCont   = std::make_shared<CContainer>();
        _ptrSpriteDemo      = std::make_shared<PanelSpriteContDemo>();
        _ptrPanel           = std::make_shared<PanelHTMLDemo>();
        _ptrPanel2          = std::make_shared<PanelHTMLDemo>();
        _ptrPanel3          = std::make_shared<PanelHTMLDemo>();
        _ptrPanel4          = std::make_shared<PanelHTMLDemo>();
        _ptrSpineDemoCont   = std::make_shared<CContainer>();
        _ptrDialogsDemoCont = std::make_shared<CContainer>();
        _ptrNetworkingDemo  = std::make_shared<PanelHTML>();
        _ptrAboutDemo       = std::make_shared<PanelHTML>();

        _ptrPanel->init(0, cx - 40, cy, shared_from_this());
        _ptrPanel->setX(20);
        _ptrPanel->setY(-10);

        _ptrPanel2->init(1, cx - 40, cy, shared_from_this());
        _ptrPanel2->setX(20);
        _ptrPanel2->setY(-10);

        _ptrPanel3->init(2, cx - 40, cy, shared_from_this());
        _ptrPanel3->setX(20);
        _ptrPanel3->setY(-10);

        _ptrPanel4->init(3, cx - 40, cy, shared_from_this());
        _ptrPanel4->setX(20);
        _ptrPanel4->setY(-10);

        static std::string sNetworkingHtml;

        sNetworkingHtml = std::format(
            "<center>\n"
            "<font name=\"marmelad\" size=\"66\" color=\"#BFE3FF\" shadow=\"1\" shadowcolor=\"#000000AA\" appear=\"rise\" dur=\"0.8\">\n"
            "<b><font fx=\"shimmer\" color=\"#4FC3F7\">{}</font></b>\n"
            "</font>\n"
            "<br><br>\n"
            "<font name=\"edugot\" size=\"48\" color=\"#E8E0D0\" shadow=\"1\" shadowcolor=\"#00000088\">\n"
            "{}\n"
            "</font>\n"
            "</center>",
            htmlEscape(L10N::getInstance().tr("NETWORKING_TITLE")),
            locHtml("NETWORKING_BODY"));

        _ptrNetworkingDemo->init(sNetworkingHtml.c_str(), cx - 40, cy, shared_from_this());
        _ptrNetworkingDemo->setX(20);
        _ptrNetworkingDemo->setY(-10);

        static std::string sAboutHtml;

        sAboutHtml = std::format(
            "<center>\n"
            "<font id=\"ab_title\" name=\"marmelad\" size=\"76\" color=\"#FFE9C9\" shadow=\"2\" shadowcolor=\"#000000FF\" appear=\"rise\" dur=\"0.8\">\n"
            "<b>{}</b>\n"
            "</font>\n"
            "<br>\n"
            "<p id=\"ab_name_p\" margin=\"6 24\">\n"
            "<font id=\"ab_name\" name=\"marmelad\" size=\"120\" color=\"#FFE9A0\" shadow=\"2\" shadowcolor=\"#000000FF\""
            " appear=\"cascade\" dur=\"1.4\" fx=\"goldwave\" fxdelay=\"3.0\">"
            "<b>Nop90h</b></font>\n"
            "</p>\n"
            "<font id=\"ab_dev\" name=\"edugot\" size=\"56\" color=\"#E8E0D0\" shadow=\"1\" shadowcolor=\"#00000088\">\n"
            "{}\n"
            "</font>\n"
            "<br>\n"
            "<font id=\"ab_exp\" name=\"edugot\" size=\"56\" color=\"#E8E0D0\" shadow=\"1\" shadowcolor=\"#00000088\">\n"
            "{}\n"
            "</font>\n"
            "<br><br>\n"
            "<font id=\"ab_contacts\" name=\"edugot\" size=\"58\" color=\"#9ADCFF\" shadow=\"1\" shadowcolor=\"#00000088\">\n"
            "{}\n"
            "</font>\n"
            "<br><br>\n"
            "<table id=\"ab_links\" border=\"0\" grid=\"none\" padding=\"14\" cellspacing=\"4\" align=\"center\">\n"
            "<tr>\n"
            "<td align=\"right\" valign=\"middle\"><font name=\"edugot\" size=\"52\" color=\"#9ADCFF\" shadow=\"1\" shadowcolor=\"#00000088\"><b>{}</b></font></td>\n"
            "<td align=\"left\"  valign=\"middle\"><font size=\"52\"><a id=\"tg_link\" hovercolor=\"#FFE9A0FF\" pressedcolor=\"#FFC040FF\" cursor=\"hand\"><u color=\"#40C4D4B0\" thickness=\"2\"><b>@ubgamez</b></u></a></font></td>\n"
            "</tr>\n"
            "<tr>\n"
            "<td align=\"right\" valign=\"middle\"><font name=\"edugot\" size=\"52\" color=\"#9ADCFF\" shadow=\"1\" shadowcolor=\"#00000088\"><b>{}</b></font></td>\n"
            "<td align=\"left\"  valign=\"middle\"><font size=\"52\"><a id=\"mail_link\" hovercolor=\"#FFE9A0FF\" pressedcolor=\"#FFC040FF\" cursor=\"hand\"><u color=\"#40C4D4B0\" thickness=\"2\"><b>validvalidate@gmail.com</b></u></a></font></td>\n"
            "</tr>\n"
            "</table>\n"
            "</center>",
            htmlEscape(L10N::getInstance().tr("ABOUT_TITLE")),
            htmlEscape(L10N::getInstance().tr("ABOUT_DEV")),
            htmlEscape(L10N::getInstance().tr("ABOUT_EXPERIENCE")),
            htmlEscape(L10N::getInstance().tr("ABOUT_CONTACTS")),
            htmlEscape(L10N::getInstance().tr("ABOUT_TG")),
            htmlEscape(L10N::getInstance().tr("ABOUT_MAIL")));

        _ptrAboutDemo->init(sAboutHtml.c_str(), cx - 40, cy, shared_from_this());
        _ptrAboutDemo->setX(20);
        _ptrAboutDemo->setY(-10);

        auto bindClipboard = [this](const char* id, const char* text)
        {
            _ptrAboutDemo->getHTMLDocument().onClick(id,
                [this, s = std::string(text)](float x, float y, int)
                {
                    copyToClipboard(s.c_str());
                    showCopyToast(_root.get(), x, y);
                });
        };

        bindClipboard("tg_link", "@ubgamez");
        bindClipboard("mail_link", "validvalidate@gmail.com");

        _ptrSpriteDemo->init(cx - 40, cy, shared_from_this());
        _ptrSpriteDemo->setX(20);
        _ptrSpriteDemo->setY(-10);

        _ptrPanel->setVisible(false);
        _ptrPanel2->setVisible(false);
        _ptrPanel3->setVisible(false);
        _ptrPanel4->setVisible(false);
        _ptrSpriteDemo->setVisible(false);
        _ptrSpineDemoCont->setVisible(false);
        _ptrBakedDemoCont->setVisible(false);
        _ptrDialogsDemoCont->setVisible(false);
        _ptrNetworkingDemo->setVisible(false);
        _ptrAboutDemo->setVisible(false);

        _root->addChild(_ptrParticlesDemo);
        _root->addChild(_ptrTextDemoCont);
        _root->addChild(_ptrSlotsDemoCont);
        _root->addChild(_ptrCardsDemoCont);
        _root->addChild(_ptrSpineDemoCont);
        _root->addChild(_ptrDialogsDemoCont);
        _root->addChild(_ptrBakedDemoCont);

        initBakedSpine();

        createFgLayer(SpriteLoader::getInstance()->getSprite("UI/frameFg"), 60, 60, 46, 46);

        auto ptrSL = SpriteLoader::getInstance();

        createCloseButton(ptrSL->getSprite("UI/btn_close_normal"),
                          ptrSL->getSprite("UI/btn_close_pressed"),
                          ptrSL->getSprite("UI/btn_close_hovered"),
                          ptrSL->getSprite("UI/btn_close_disabled"));

        _closeButton->setVisible(false);

        _ptrTextDemoCont->setVisible(false);
        _ptrSlotsDemoCont->setVisible(false);

        _ptrTabsPanel = std::make_shared<PanelTabs>();

        static std::vector<std::string> tabStrings;

        tabStrings.clear();
        tabStrings.reserve(32);

        std::vector<TabInfo_t> vTabs;

        vTabs.reserve(20);

        tabStrings.push_back(std::format(
            "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\"><b>{}</b></font>",
            htmlEscape(L10N::getInstance().tr("TAB_SPRITE_CONTAINER"))));
        vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 7 });

        tabStrings.push_back(std::format(
            "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\"><b>{}</b></font>",
            htmlEscape(L10N::getInstance().tr("TAB_DIALOG_SYSTEM"))));
        vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 10 });

        tabStrings.push_back(std::format(
            "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\"><b>{}</b></font>",
            htmlEscape(L10N::getInstance().tr("TAB_TTF"))));
        vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 1 });

        tabStrings.push_back(std::format(
            "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\"><b>{}</b></font>",
            htmlEscape(L10N::getInstance().tr("TAB_RENDER_HTML"))));
        vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 4 });

        tabStrings.push_back(std::format(
            "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\"><b>{}</b></font>",
            htmlEscape(L10N::getInstance().tr("TAB_MORE_HTML"))));
        vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 5 });

        tabStrings.push_back(std::format(
            "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\"><b>{}</b></font>",
            htmlEscape(L10N::getInstance().tr("TAB_DRAG_DROP"))));
        vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 3 });

        tabStrings.push_back(std::format(
            "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\"><b>{}</b></font>",
            htmlEscape(L10N::getInstance().tr("TAB_PARTICLES"))));
        vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 0 });

        tabStrings.push_back(std::format(
            "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\"><b>{}</b></font>",
            htmlEscape(L10N::getInstance().tr("TAB_SLOT_MACHINE"))));
        vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 2 });

        if (CGfx::getInstance()->isPostProcessAvailable())
        {
            tabStrings.push_back(std::format(
                "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\"><b>{}</b></font>",
                htmlEscape(L10N::getInstance().tr("TAB_POSTPROCESS"))));
            vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 6 });
        }

        tabStrings.push_back(std::format(
            "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\"><b>{}</b></font>",
            htmlEscape(L10N::getInstance().tr("TAB_SOUND"))));
        vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 8 });

        if (CGfx::getInstance()->isLightAvailable())
        {
            tabStrings.push_back(std::format(
                "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\"><b>{}</b></font>",
                htmlEscape(L10N::getInstance().tr("TAB_SPINE_LIGHT"))));
            vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 9 });
        }
        else
        {
            tabStrings.push_back(std::format(
                "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\"><b>{}</b></font>",
                htmlEscape(L10N::getInstance().tr("TAB_SPINE"))));
            vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 9 });
        }

        tabStrings.push_back(std::format(
            "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\" fx=\"rainbow\"><b>{}</b></font>",
            htmlEscape(L10N::getInstance().tr("TAB_BAKED_SPINE"))));
        vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 11 });

        tabStrings.push_back(std::format(
            "<font name=\"edugot\" color=\"#EEDDCC\" size=\"34\"><b>{}</b></font>",
            htmlEscape(L10N::getInstance().tr("TAB_NETWORKING"))));
        vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 12 });

        tabStrings.push_back(std::format(
            "<font name=\"edugot\" color=\"#EEDDCC\" size=\"44\" fx=\"fire\"><b>{}</b></font>",
            htmlEscape(L10N::getInstance().tr("TAB_ABOUT"))));
        vTabs.push_back({ tabStrings.back().c_str(), "UI/tab_active", "UI/tab_inactive", 38, 38, 13 });

        TabFont tf;
        tf.bIsHtml = true;

        _ptrTabsPanel->init(vTabs, [this](int nTabId)
        {
            onActiveTabChanged(nTabId);
        }, this, tf);

        _ptrTabsPanel->setPos(-(_ptrTabsPanel->getDialogCx() - 20),
                              (_fDialogCy - _ptrTabsPanel->getDialogCy()) / 2.f);

        updateTitle();

        float fPad = 60;

        Rect rc = { 0 + fPad * 0.5f, 0 + fPad * 0.5f, getDialogCx() - fPad, getDialogCy() - fPad };

        _root->setScrollBox(&rc);

        initSpineAndLight();
    }

    return bRes;
}

void DialogDemo::updateTitle()
{
    CSpritePtr pTitleFrame = SpriteLoader::getInstance()->getSprite("UI/frameTitle");

    LPCTSTR sTitle = "Demo";

    if (_ptrTabsPanel)
    {
        switch (_ptrTabsPanel->getSelectedTab())
        {
            case 0:
                sTitle = L10N::getInstance().tr("TAB_PARTICLES");
                break;

            case 1:
                sTitle = L10N::getInstance().tr("TAB_TTF");
                break;

            case 2:
                sTitle = L10N::getInstance().tr("TAB_SLOT_MACHINE");
                break;

            case 3:
                sTitle = L10N::getInstance().tr("TAB_DRAG_DROP");
                break;

            case 4:
                sTitle = L10N::getInstance().tr("TAB_RENDER_HTML");
                break;

            case 5:
                sTitle = L10N::getInstance().tr("TAB_MORE_HTML");
                break;

            case 6:
                sTitle = L10N::getInstance().tr("TAB_POSTPROCESS");
                break;

            case 7:
                sTitle = L10N::getInstance().tr("TAB_SPRITE_CONTAINER");
                break;

            case 8:
                sTitle = L10N::getInstance().tr("TAB_SOUND");
                break;

            case 9:
                if (CGfx::getInstance()->isLightAvailable())
                    sTitle = L10N::getInstance().tr("TAB_SPINE_LIGHT");
                else
                    sTitle = L10N::getInstance().tr("TAB_SPINE");
                break;

            case 10:
                sTitle = L10N::getInstance().tr("TAB_DIALOG_SYSTEM");
                break;

            case 11:
                sTitle = L10N::getInstance().tr("TITLE_BAKED_SPINE");
                break;

            case 12:
                sTitle = L10N::getInstance().tr("TAB_NETWORKING");
                break;

            case 13:
                sTitle = L10N::getInstance().tr("TAB_ABOUT");
                break;

            default:
                assert(false);
                break;
        }
    }

    if (_titleCont)
        _titleCont->removeSelfTweens();

    if (_ptrTitleLabel)
        _ptrTitleLabel->removeSelfTweens();

    if (_ptrTabsPanel && _ptrTabsPanel->getSelectedTab() == 10)
    {
        auto sHtml = std::format(
            "<center><font fx=\"ishimmer\" appear=\"comet\" dur=\"2\" repeatdelay=\"5\">{}</font></center>",
            htmlEscape(sTitle));

        setTitle(sHtml.c_str(), pTitleFrame, 113, 113, true);
    }
    else
    {
        setTitle(sTitle, pTitleFrame, 113, 113);
    }

    _titleCont->setY(-20.f);
    _titleCont->setPivotCentered();

    _ptrTitleLabel->setAlpha(0);

    _titleCont->addSelfTween(eTweenProp::SCALE_X, 0.5f, 1.f, 1.f, Easing::outBack);
    _ptrTitleLabel->addSelfTween(eTweenProp::ALPHA, 0.f, 1.f, 1.f, Easing::linear);
}

// ===============================================================
// About: хореография появления
// ===============================================================
void DialogDemo::playAboutEntrance()
{
    if (!_ptrAboutDemo)
        return;

    auto& doc = _ptrAboutDemo->getHTMLDocument();

    if (!doc)
        return;

    auto fadeFrom = [&doc](const char* id, const HTMLTweenProps& props, float dur, float delay, easingFunction ease)
    {
        auto el = doc[id];

        if (!el)
            return;

        el.killTweens();

        HTMLTweenOptions o;
        o.ease = ease;
        o.delay = delay;

        el.tweenFrom(props, dur, o);
    };

    // титул: упругий рост из малого + повтор appear-анимации
    if (auto el = doc["ab_title"])
    {
        el.killTweens();
        el.restartFontAnimation();

        HTMLTweenOptions o;
        o.ease = Easing::outElastic;
        o.delay = 0.05f;

        el.tweenFrom(HTMLTweenProps().alpha(0.f).fontSize(34.f), 0.9f, o);
    }

    // имя: обёртка-блок — выезд снизу, затем дыхание scale
    if (auto el = doc["ab_name_p"])
    {
        el.killTweens();

        HTMLTweenOptions o;
        o.ease = Easing::outCubic;
        o.delay = 0.35f;

        el.tweenFrom(HTMLTweenProps().offY(50.f).alpha(0.f), 0.7f, o);

        HTMLTweenOptions breathe;
        breathe.yoyo = true;
        breathe.repeat = -1;
        breathe.ease = Easing::inOutSine;
        breathe.easeBack = Easing::inOutSine;
        breathe.delay = 1.6f;

        el.tweenTo(HTMLTweenProps().scale(1.05f), 1.8f, breathe);
    }

    // имя: каскад по буквам (перезапуск) + золотая пульсация цветом
    if (auto el = doc["ab_name"])
    {
        el.killTweens();
        el.restartFontAnimation();

        HTMLTweenOptions o;
        o.yoyo = true;
        o.repeat = -1;
        o.ease = Easing::inOutSine;
        o.easeBack = Easing::inOutSine;
        o.delay = 2.2f;

        el.tweenTo(HTMLTweenProps().color(0xFFFFFFFF), 1.6f, o);
    }

    fadeFrom("ab_dev",      HTMLTweenProps().alpha(0.f), 0.55f, 0.55f, Easing::outQuad);
    fadeFrom("ab_exp",      HTMLTweenProps().alpha(0.f), 0.55f, 0.72f, Easing::outQuad);
    fadeFrom("ab_contacts", HTMLTweenProps().alpha(0.f), 0.50f, 0.95f, Easing::outQuad);

    // таблица контактов: выезд снизу с упругим доводом
    if (auto el = doc["ab_links"])
    {
        el.killTweens();

        HTMLTweenOptions o;
        o.ease = Easing::outBack;
        o.delay = 1.1f;

        el.tweenFrom(HTMLTweenProps().offY(60.f).alpha(0.f), 0.7f, o);
    }
}

void DialogDemo::onActiveTabChanged(int nTab)
{
    closeDemoDialogs();

    _ptrParticlesDemo->setVisible(false);
    _ptrTextDemoCont->setVisible(false);
    _ptrSlotsDemoCont->setVisible(false);
    _ptrCardsDemoCont->setVisible(false);
    _ptrSpriteDemo->setVisible(false);
    _ptrSpineDemoCont->setVisible(false);
    _ptrBakedDemoCont->setVisible(false);
    _ptrDialogsDemoCont->setVisible(false);
    _ptrPanel->setVisible(false);
    _ptrPanel2->setVisible(false);
    _ptrPanel3->setVisible(false);
    _ptrPanel4->setVisible(false);
    _ptrNetworkingDemo->setVisible(false);
    _ptrAboutDemo->setVisible(false);

    switch (nTab)
    {
        case 0:
            updateTitle();
            initParticlesDemo();
            _ptrParticlesDemo->setVisible(true);
            break;

        case 1:
            updateTitle();
            initFontsDemo();
            _ptrTextDemoCont->setVisible(true);
            break;

        case 2:
            updateTitle();
            initSlotsDemo();
            _ptrSlotsDemoCont->setVisible(true);
            break;

        case 3:
            updateTitle();
            initCardsDemo();
            _ptrCardsDemoCont->setVisible(true);
            break;

        case 4:
            updateTitle();
            _ptrPanel->setVisible(true);
            break;

        case 5:
            updateTitle();
            _ptrPanel2->setVisible(true);
            break;

        case 6:
            updateTitle();
            _ptrPanel3->setVisible(true);
            break;

        case 7:
            updateTitle();
            _ptrSpriteDemo->setVisible(true);
            break;

        case 8:
            updateTitle();
            _ptrPanel4->setVisible(true);
            break;

        case 9:
            updateTitle();
            _ptrSpineDemoCont->setVisible(true);
            break;

        case 10:
            updateTitle();
            initDialogsDemo();
            _ptrDialogsDemoCont->setVisible(true);
            break;

        case 11:
            updateTitle();
            _ptrBakedDemoCont->setVisible(true);
            break;

        case 12:
            updateTitle();
            _ptrNetworkingDemo->setVisible(true);
            break;

        case 13:
            updateTitle();
            _ptrAboutDemo->setVisible(true);

            setTimeout(0.05f, [this]()
            {
                playAboutEntrance();
            });
            break;
    }
}

void DialogDemo::onShow(bool bShow)
{
    if (bShow)
    {
        CGfx::getInstance()->setWorldDarken();

        _ptrPanel->showAsPanel(E_IH_DISPATCHBYPARENT);
        _ptrPanel2->showAsPanel(E_IH_DISPATCHBYPARENT);
        _ptrPanel3->showAsPanel(E_IH_DISPATCHBYPARENT);
        _ptrPanel4->showAsPanel(E_IH_DISPATCHBYPARENT);
        _ptrSpriteDemo->showAsPanel(E_IH_DISPATCHBYPARENT);
        _ptrTabsPanel->showAsPanel(E_IH_DISPATCHBYPARENT);
        _ptrNetworkingDemo->showAsPanel(E_IH_DISPATCHBYPARENT);
        _ptrAboutDemo->showAsPanel(E_IH_DISPATCHBYPARENT);

        _ptrPanel->setVisible(false);
        _ptrPanel2->setVisible(false);
        _ptrPanel3->setVisible(false);
        _ptrPanel4->setVisible(false);
        _ptrSpriteDemo->setVisible(false);
        _ptrBakedDemoCont->setVisible(false);
        _ptrNetworkingDemo->setVisible(false);
        _ptrAboutDemo->setVisible(false);

        onActiveTabChanged(7);

        setTimeout(1.1f, [this]
        {
            _closeButton->setScale(0.2f, 0.2f);
            _closeButton->addSelfTween(eTweenProp::SCALE, 0.2f, 1.f, 0.5f, Easing::outBack);
            _closeButton->setVisible(true);
        });
    }
    else
    {
        CGfx::getInstance()->setWorldDarken(false);

        closeDemoDialogs();

        _ptrTabsPanel->close();
        _ptrPanel->close();
        _ptrPanel2->close();
        _ptrPanel3->close();
        _ptrPanel4->close();
        _ptrSpriteDemo->close();
        _ptrNetworkingDemo->close();
        _ptrAboutDemo->close();
    }
}

void DialogDemo::updateScaleAndPos()
{
    auto& cfg = Engine::getCfg();

    float scrCx = CSceneResize::getInstance()->getGameWidth();
    float scrCy = CSceneResize::getInstance()->getGameHeight();

    float fTopMarg = getTopMargin();

    scrCy -= fTopMarg;

    float fScaleX = scrCx / cfg.INIT_SCR_CX;
    float fScaleY = scrCy / cfg.INIT_SCR_CY;
    float fMinScale = std::min(fScaleX, fScaleY);

    setScale(fMinScale, fMinScale);

    Rect rcDlg(0, 0, _fDialogCx, _fDialogCy);
    Rect rcTabs(_ptrTabsPanel->getX(),
                _ptrTabsPanel->getY(),
                _ptrTabsPanel->getDialogCx(),
                _ptrTabsPanel->getDialogCy());

    rcDlg.unite(&rcTabs);

    float fDlgCx = rcDlg.cx * fMinScale;
    float fDlgCy = rcDlg.cy * fMinScale;
    float fDlgX  = rcDlg.x * fMinScale;
    float fDlgY  = rcDlg.y * fMinScale;

    setPos((scrCx - fDlgCx) / 2.f + fabs(fDlgX),
           (scrCy - fDlgCy) / 2.f + fabs(fDlgY) + fTopMarg);
}

void DialogDemo::trackDemoDialog(const BaseDialogPtr& ptr)
{
    if (!ptr)
        return;

    for (auto it = _demoDialogs.begin(); it != _demoDialogs.end(); )
    {
        if (it->expired())
            it = _demoDialogs.erase(it);
        else
            ++it;
    }

    _demoDialogs.push_back(ptr);
}

void DialogDemo::closeDemoDialogs()
{
    for (auto it = _demoDialogs.rbegin(); it != _demoDialogs.rend(); ++it)
    {
        if (auto p = it->lock())
        {
            if (p->isVisible())
                p->close();
        }
    }

    _demoDialogs.clear();
}

void DialogDemo::onCloseButtonClick()
{
    closeDemoDialogs();

    BaseDialog::onCloseButtonClick();
}

void DialogDemo::openStackLevel(int level)
{
    std::string strTitle = std::vformat(L10N::getInstance().tr("DLG_STACK_LEVEL_FMT"), std::make_format_args(level));

    auto dlg = std::make_shared<DialogDemoExample>();

    if (!dlg->initExample(strTitle.c_str(), 680.f, 400.f, nullptr))
        return;

    auto ptrCont = createBehindParticles(eBehindBgKind::AUTUMN, dlg.get());
    dlg->setBehindBg(ptrCont);

    std::string sHtml;

    sHtml += R"HTML(<center>
<font name="edugot" size="30" color="#FFFFFF" shadow="1" shadowcolor="#00000088" appear="rise" dur="0.5">
)HTML";

    sHtml += htmlEscape(std::vformat(L10N::getInstance().tr("DLG_STACK_MODAL_FMT"), std::make_format_args(level)));
    sHtml += "<br>";

    if (level < 3)
        sHtml += htmlEscape(L10N::getInstance().tr("DLG_STACK_UNDER"));
    else
        sHtml += htmlEscape(L10N::getInstance().tr("DLG_STACK_LAST"));

    sHtml += R"HTML(
</font>
<br><br>
<table padding="6" border="0" grid="none" cellspacing="2" align="center"><tr>)HTML";

    if (level < 3)
    {
        sHtml += R"HTML(<td>)HTML";
        sHtml += demoButton("btn_next", L10N::getInstance().tr("BTN_NEXT_DIALOG"));
        sHtml += R"HTML(</td>)HTML";
    }

    sHtml += R"HTML(<td>)HTML";
    sHtml += demoButton("btn_close", L10N::getInstance().tr("BTN_CLOSE"));
    sHtml += R"HTML(</td></tr></table>
</center>)HTML";

    dlg->setHtml(sHtml.c_str());

    if (level < 3)
    {
        dlg->bindClick("btn_next", [this, level]()
        {
            openStackLevel(level + 1);
        });
    }

    dlg->bindClick("btn_close", [this, w = dlg->weak_from_this()]()
    {
        if (auto p = std::static_pointer_cast<BaseDialog>(w.lock()))
        {
            p->close();
        }
    });

    trackDemoDialog(dlg);

    dlg->open();
}

static void spawnGrenade(CContainer*          pCont,
                         CParticleSystemPtr   ptrExplosion,
                         CParticleSystemPtr   ptrDebris,
                         CParticleSystemPtr   ptrBg,
                         std::shared_ptr<int> ptrAlive,
                         std::shared_ptr<int> ptrShocks)
{
    auto ptrG = SpriteLoader::getInstance()->getSprite("UI/grenade");

    if (!ptrG)
    {
        --(*ptrAlive);
        return;
    }

    ptrG->setScaleToY(64.f);
    ptrG->setPivotCentered();

    pCont->addChild(ptrG);

    const float fScrCx  = Engine::getCfg().INIT_SCR_CX;
    const float fScrCy  = Engine::getCfg().INIT_SCR_CY;

    const float fGCy    = ptrG->calcNotTransCy();
    const float fGCx    = ptrG->calcNotTransCx();

    const float fX0     = randomRange(fScrCx * 0.12f, fScrCx * 0.88f);
    const float fDrift  = randomRange(-140.f, 140.f);
    const float fStartY = -fGCy - 20.f;
    const float fFloorY = randomRange(fScrCy * 0.60f, fScrCy * 0.85f) - fGCy;

    ptrG->setPos(fX0, fStartY);

    const float fFlight = 1.87f;

    ptrG->addSelfTween(eTweenProp::X, fX0, fX0 + fDrift, fFlight, Easing::linear);

    ptrG->addSelfTween(eTweenProp::ROTATE,
                       0.f,
                       (rand() % 2 ? 1.f : -1.f) * randomRange(7.f, 13.f),
                       fFlight,
                       Easing::linear);

    CContainerWPtr wG = ptrG;

    SimpleCallback explode = [pCont, ptrExplosion, ptrDebris, ptrBg, ptrAlive, ptrShocks, wG, fGCx, fGCy]()
    {
        --(*ptrAlive);

        auto pG = wG.lock();

        if (!pG)
            return;

        const float fExX = pG->getX() + fGCx * 0.5f;
        const float fExY = pG->getY() + fGCy * 0.5f;

        if (ptrBg)
        {
            ++(*ptrShocks);

            ptrBg->repelFrom(fExX - ptrBg->getX(),
                             fExY - ptrBg->getY(),
                             3200.f,
                             580.f);

            ptrBg->setTimeout(0.5f, [ptrBg, ptrShocks]()
            {
                if (--(*ptrShocks) <= 0)
                    ptrBg->disableRepeller();
            });
        }

        pG->removeSelfTweens();

        const float fSx = pG->getScaleX();
        const float fSy = pG->getScaleY();

        pG->addSelfTween(eTweenProp::SCALE_X, fSx, fSx * 1.45f, 0.07f, Easing::outQuad);
        pG->addSelfTween(eTweenProp::SCALE_Y, fSy, fSy * 1.45f, 0.07f, Easing::outQuad);

        pG->addSelfTween(eTweenProp::ROTATE,
                         pG->getRotate(),
                         pG->getRotate() + 7.f,
                         0.26f,
                         Easing::linear);

        pG->addSelfTween(eTweenProp::SCALE_X, fSx * 1.45f, 0.f, 0.19f, Easing::inCubic, 0.07f);
        pG->addSelfTween(eTweenProp::SCALE_Y, fSy * 1.45f, 0.f, 0.19f, Easing::inCubic, 0.07f,
            [wG]()
            {
                if (auto p = wG.lock())
                    p->removeFromParent();
            });

        pG->addSelfTween(eTweenProp::ALPHA, 1.f, 0.f, 0.12f, Easing::linear, 0.14f);

        ptrExplosion->setTimeout(0.05f, [ptrExplosion, ptrDebris, fExX, fExY]()
        {
            ptrExplosion->burstAtLocal(fExX, fExY, 60);
            ptrDebris->burstAtLocal(fExX, fExY, 50);
        });

        ptrExplosion->setTimeout(0.16f, [ptrExplosion, fExX, fExY]()
        {
            ptrExplosion->burstAtLocal(fExX, fExY - 12.f, 22);
        });
    };

    auto fall = [wG](float fFrom, float fTo, float fDur, SimpleCallback cb)
    {
        if (auto pG = wG.lock())
            pG->addSelfTween(eTweenProp::Y, fFrom, fTo, fDur, Easing::inCubic, 0.f, cb);
    };

    auto rise = [wG](float fFrom, float fTo, float fDur, SimpleCallback cb)
    {
        if (auto pG = wG.lock())
            pG->addSelfTween(eTweenProp::Y, fFrom, fTo, fDur, Easing::outCubic, 0.f, cb);
    };

    const float fBounce1 = randomRange(95.f, 130.f);
    const float fBounce2 = randomRange(30.f, 55.f);

    fall(fStartY, fFloorY, 0.75f, [rise, fall, wG, fFloorY, fBounce1, fBounce2, explode]()
    {
        rise(fFloorY, fFloorY - fBounce1, 0.34f, [rise, fall, wG, fFloorY, fBounce1, fBounce2, explode]()
        {
            fall(fFloorY - fBounce1, fFloorY, 0.34f, [rise, fall, wG, fFloorY, fBounce2, explode]()
            {
                rise(fFloorY, fFloorY - fBounce2, 0.22f, [fall, wG, fFloorY, fBounce2, explode]()
                {
                    fall(fFloorY - fBounce2, fFloorY, 0.22f, explode);
                });
            });
        });
    });
}

static void launchFirework(CContainer*          pCont,
                           CParticleSystemPtr   ptrCrackle,
                           std::shared_ptr<int> ptrAlive)
{
    const float fScrCx = Engine::getCfg().INIT_SCR_CX;
    const float fScrCy = Engine::getCfg().INIT_SCR_CY;

    ++(*ptrAlive);

    const float fApexX  = randomRange(fScrCx * 0.15f, fScrCx * 0.85f);
    const float fApexY  = randomRange(fScrCy * 0.12f, fScrCy * 0.45f);
    const float fStartX = fApexX + randomRange(-90.f, 90.f);
    const float fStartY = fScrCy + 40.f;

    static const float s_pal[][3] =
    {
        { 1.00f, 0.75f, 0.25f },
        { 1.00f, 0.30f, 0.25f },
        { 0.35f, 0.85f, 1.00f },
        { 1.00f, 0.35f, 0.75f },
        { 0.45f, 1.00f, 0.45f },
        { 1.00f, 1.00f, 1.00f },
    };

    const int   nCol = rand() % 6;
    const float fCr  = s_pal[nCol][0];
    const float fCg  = s_pal[nCol][1];
    const float fCb  = s_pal[nCol][2];

    auto ptrRocket = SpriteLoader::getInstance()->getSprite("PARTICLES/light_01");

    if (!ptrRocket)
    {
        --(*ptrAlive);
        return;
    }

    ptrRocket->setBlendMode(eSpriteBlendMode::ADDITIVE);
    ptrRocket->setTint(fCr, fCg, fCb);
    ptrRocket->setScaleTo(50.f, 7.f);
    ptrRocket->rotate(-1.5707963f);
    ptrRocket->setPos(fStartX, fStartY);
    ptrRocket->setAlpha(0.9f);

    pCont->addChild(ptrRocket);

    ptrCrackle->burstAtLocal(fStartX, fStartY, 6);

    ptrRocket->addSelfTween(eTweenProp::X, fStartX, fApexX, 0.8f, Easing::linear);
    ptrRocket->addSelfTween(eTweenProp::Y, fStartY, fApexY, 0.8f, Easing::inQuad, 0.f,
        [pCont, ptrCrackle, ptrAlive, fApexX, fApexY, fCr, fCg, fCb, wR = ptrRocket->weak_from_this()]()
        {
            if (auto pR = wR.lock())
                pR->removeFromParent();

            ParticleSettings st = ParticlePresets::getFireworksPreset();

            st.maxParticles = 260;
            st.use3PhaseColor = true;

            st.startColor[0]  = 1.0f;
            st.startColor[1]  = 1.0f;
            st.startColor[2]  = 1.0f;
            st.startColor[3]  = 1.0f;

            st.middleColor[0] = fCr;
            st.middleColor[1] = fCg;
            st.middleColor[2] = fCb;
            st.middleColor[3] = 1.0f;

            st.endColor[0]    = fCr * 0.35f;
            st.endColor[1]    = fCg * 0.35f;
            st.endColor[2]    = fCb * 0.35f;
            st.endColor[3]    = 0.0f;

            auto ptrSalvo = std::make_shared<CParticleSystem<>>(st,
                std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::FIREWORKS)).c_str());

            ptrSalvo->setPos(fApexX, fApexY);

            pCont->addChild(ptrSalvo);

            ptrSalvo->burst(static_cast<int>(randomRange(90.f, 140.f)),
                [ptrAlive, wS = ptrSalvo->weak_from_this()]()
                {
                    --(*ptrAlive);

                    if (auto pS = wS.lock())
                        pS->removeFromParent();
                });

            ptrCrackle->burstAtLocal(fApexX, fApexY, 10);

            for (int i = 0; i < 3; ++i)
            {
                const float fDelay = 0.45f + 0.16f * i;
                const float fOffX  = randomRange(-55.f, 55.f);
                const float fOffY  = randomRange(-30.f, 55.f);

                ptrCrackle->setTimeout(fDelay, [ptrCrackle, fApexX, fApexY, fOffX, fOffY]()
                {
                    ptrCrackle->burstAtLocal(fApexX + fOffX,
                                             fApexY + fOffY,
                                             static_cast<int>(randomRange(14.f, 22.f)));
                });
            }
        });
}

namespace
{
    static void scheduleBgTick(CContainer* pCont,
                               float fMinDelay,
                               float fMaxDelay,
                               std::function<void()> fn)
    {
        if (!pCont || !fn)
            return;

        auto ptrTick = std::make_shared<std::function<void()>>();
        std::weak_ptr<std::function<void()>> wTick = ptrTick;

        *ptrTick = [pCont, fMinDelay, fMaxDelay, fn, wTick]()
        {
            fn();

            if (auto sp = wTick.lock())
            {
                pCont->setTimeout(randomRange(fMinDelay, fMaxDelay), [sp]()
                {
                    (*sp)();
                });
            }
        };

        pCont->setTimeout(randomRange(fMinDelay, fMaxDelay), [sp = ptrTick]()
        {
            (*sp)();
        });
    }
}

struct SakuraCont : CContainer
{
    virtual void calcNotTransBounds(Rect* p) override
    {
        p->set(0, 0, Engine::getCfg().INIT_SCR_CX, Engine::getCfg().INIT_SCR_CY);
    }
};

CContainerPtr DialogDemo::createBehindParticles(eBehindBgKind eKind, BaseDialog* pFlowDlg)
{
    auto ptrSpr = SpriteLoader::getInstance()->getSprite("UI/whitebox");

    ptrSpr->setScaleTo(Engine::getCfg().INIT_SCR_CX, Engine::getCfg().INIT_SCR_CY);
    ptrSpr->setRgba(0xfeb9beff);

    auto ptrCont = std::make_shared<SakuraCont>();

    ptrCont->addChild(ptrSpr);

    CParticleSystemPtr ptrBackPartickes;

    if (eKind == eBehindBgKind::SOLITAIRE_STORM)
        return std::make_shared<CosmicCardStorm>();

    if (eKind == eBehindBgKind::HEARTS)
    {
        ptrBackPartickes = std::make_shared<CParticleSystem<>>(ParticlePresets::getBokehAmbientPreset(), "PARTICLES/heart");

        auto& s = ptrBackPartickes->getSettings();

        s.spawnRadius = Engine::getCfg().INIT_SCR_CX;
        s.spawnRate = 60;
        s.maxParticles = 120;

        ptrBackPartickes->setPos((Engine::getCfg().INIT_SCR_CX - s.spawnRadius) * 0.5f, 0);

        ptrCont->addChild(ptrBackPartickes);

        ptrBackPartickes->prewarm(5);
    }
    else if (eKind == eBehindBgKind::AUTUMN)
    {
        const float fScrCx = Engine::getCfg().INIT_SCR_CX;
        const float fScrCy = Engine::getCfg().INIT_SCR_CY;

        ptrSpr->setRgba(0xc98a3dff);

        auto ptrBottom = SpriteLoader::getInstance()->getSprite("UI/whitebox");

        ptrBottom->setScaleTo(fScrCx, fScrCy);
        ptrBottom->setRgba(0xf5c58cff);
        ptrBottom->setAlpha(0.55f);

        ptrCont->addChild(ptrBottom);

        const int nRays = 6;

        for (int i = 0; i < nRays; ++i)
        {
            auto ptrRay = SpriteLoader::getInstance()->getSprite("PARTICLES/light");

            if (!ptrRay)
                continue;

            ptrRay->setBlendMode(eSpriteBlendMode::ADDITIVE);
            ptrRay->setTint(1.00f, 0.88f, 0.55f);

            const float fRayCy = fScrCy * 1.8f;

            ptrRay->setScaleTo(randomRange(180.f, 420.f), fRayCy);
            ptrRay->setPivotCentered();
            ptrRay->rotate(randomRange(-0.35f, -0.15f));

            const float fSlot  = (static_cast<float>(i) + 0.5f) / static_cast<float>(nRays);
            const float fX0    = fScrCx * (fSlot + randomRange(-0.06f, 0.06f));
            const float fDrift = randomRange(80.f, 160.f);

            ptrRay->setPos(fX0 - fDrift * 0.5f, fScrCy * 0.5f);

            const float fBaseA = randomRange(0.05f, 0.10f);

            ptrRay->setAlpha(fBaseA);

            ptrCont->addChild(ptrRay);

            ptrRay->addSelfTweenYoyo(eTweenProp::ALPHA,
                                     fBaseA * 0.5f,
                                     fBaseA * 1.6f,
                                     randomRange(3.5f, 6.0f),
                                     Easing::inOutSine,
                                     Easing::inOutSine);

            ptrRay->addSelfTweenYoyo(eTweenProp::X,
                                     fX0 - fDrift * 0.5f,
                                     fX0 + fDrift * 0.5f,
                                     randomRange(7.f, 11.f),
                                     Easing::inOutSine,
                                     Easing::inOutSine);
        }

        auto ptrDust = std::make_shared<CParticleSystem<>>(
            ParticlePresets::getAmbientDustPreset(),
            std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::AMBIENT_DUST)).c_str());

        auto& ds = ptrDust->getSettings();

        ds.spawnRate    = 14.f;
        ds.maxParticles = 90;
        ds.lifetimeMin  = 5.f;
        ds.lifetimeMax  = 9.f;
        ds.spawnRadius  = 0.f;
        ds.spawnAreaCx  = fScrCx * 1.15f;
        ds.spawnAreaCy  = fScrCy * 1.15f;
        ds.speedMin     = 4.f;
        ds.speedMax     = 14.f;
        ds.windX        = 18.f;
        ds.drag         = 0.35f;
        ds.turbulence   = 10.f;
        ds.waveFrequency = 1.2f;
        ds.waveAmplitude = 12.f;
        ds.startScaleMin = 0.18f;
        ds.startScaleMax = 0.45f;
        ds.endScaleMin   = 0.18f;
        ds.endScaleMax   = 0.45f;
        ds.use3PhaseColor = true;

        ds.startColor[0]  = 1.00f;
        ds.startColor[1]  = 0.88f;
        ds.startColor[2]  = 0.45f;
        ds.startColor[3]  = 0.90f;

        ds.middleColor[0] = 1.00f;
        ds.middleColor[1] = 0.75f;
        ds.middleColor[2] = 0.30f;
        ds.middleColor[3] = 0.70f;

        ds.endColor[0]    = 0.90f;
        ds.endColor[1]    = 0.60f;
        ds.endColor[2]    = 0.20f;
        ds.endColor[3]    = 0.00f;

        ds.colorJitter    = 0.15f;
        ds.blendMode      = eSpriteBlendMode::ADDITIVE;
        ds.useDepth       = true;
        ds.depthMin       = 0.40f;
        ds.depthScaleMin  = 0.40f;
        ds.depthSpeedMin  = 0.40f;
        ds.depthAlphaMin  = 0.45f;
        ds.fadeInTime     = 0.6f;
        ds.fadeOutTime    = 1.4f;

        ptrDust->setPos(fScrCx * 0.5f, fScrCy * 0.5f);

        ptrCont->addChild(ptrDust);

        ptrDust->prewarm(6);

        const char* leaves[] = { "PARTICLES/aleaf_01", "PARTICLES/aleaf_02", "PARTICLES/aleaf_03", "PARTICLES/aleaf_04" };

        auto ptrLeaves = std::make_shared<CParticleSystem<>>(
            ParticlePresets::getFallingLeavesPreset(), leaves);

        auto& ls = ptrLeaves->getSettings();

        ls.spawnAreaCx = fScrCx * 1.2f;
        ls.spawnAreaCy = fScrCy * 1.2f;
        ls.floorY      = fScrCy * 0.5f + 30.f;

        ptrLeaves->setPos(fScrCx * 0.5f, fScrCy * 0.5f);

        ptrCont->addChild(ptrLeaves);

        ptrLeaves->prewarm(8);

        CContainer* pContRaw = ptrCont.get();

        auto ptrGustTick = std::make_shared<std::function<void()>>();
        std::weak_ptr<std::function<void()>> wGust = ptrGustTick;

        *ptrGustTick = [pContRaw, ptrLeaves, wGust]()
        {
            if (auto sp = wGust.lock())
                pContRaw->setTimeout(randomRange(6.0f, 10.0f), [sp]() { (*sp)(); });

            ptrLeaves->getSettings().gustAmp = 320.f;

            pContRaw->setTimeout(1.6f, [ptrLeaves]()
            {
                ptrLeaves->getSettings().gustAmp = 90.f;
            });
        };

        pContRaw->setTimeout(2.5f, [sp = ptrGustTick]() { (*sp)(); });
    }
    else if (eKind == eBehindBgKind::BLIZZARD)
    {
        ptrSpr->setRgba(0x141c28ff);

        const float fScrCx = Engine::getCfg().INIT_SCR_CX;
        const float fScrCy = Engine::getCfg().INIT_SCR_CY;

        for (int i = 0; i < 4; ++i)
        {
            auto ptrDrift = SpriteLoader::getInstance()->getSprite("PARTICLES/smoke_08");

            if (!ptrDrift)
                continue;

            ptrDrift->setBlendMode(eSpriteBlendMode::NORMAL);
            ptrDrift->setTint(0.72f, 0.78f, 0.86f);
            ptrDrift->setScaleTo(randomRange(650.f, 1100.f), randomRange(140.f, 230.f));
            ptrDrift->setAlpha(0.85f);
            ptrDrift->setPos(randomRange(-200.f, fScrCx - 300.f), fScrCy - randomRange(10.f, 70.f));

            ptrCont->addChild(ptrDrift);
        }

        auto ptrSnow = std::make_shared<CParticleSystem<>>(
            ParticlePresets::getAdvancedSnowPreset(),
            std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::ADVANCED_SNOW)).c_str());

        auto& ss = ptrSnow->getSettings();

        ss.spawnRate    = 60.f;
        ss.maxParticles = 350;
        ss.spawnRadius  = 0.f;
        ss.spawnAreaCx  = fScrCx * 1.3f;
        ss.spawnAreaCy  = fScrCy * 1.3f;
        ss.windX        = 40.f;
        ss.gustAmp      = 120.f;
        ss.gustFreq     = 0.5f;
        ss.floorY       = fScrCy * 0.5f + 20.f;

        ptrSnow->setPos(fScrCx * 0.5f, fScrCy * 0.5f);

        ptrCont->addChild(ptrSnow);

        ptrSnow->prewarm(5);

        auto ptrFog = SpriteLoader::getInstance()->getSprite("UI/whitebox");

        ptrFog->setScaleTo(fScrCx, fScrCy);
        ptrFog->setRgba(0xcfe0f2ff);
        ptrFog->setAlpha(0.f);

        ptrCont->addChild(ptrFog);

        auto ptrCharge = std::make_shared<CParticleSystem<>>(
            ParticlePresets::getSandstormPreset(),
            "PARTICLES/circle_05");

        auto& cs = ptrCharge->getSettings();

        cs.spawnRate    = 70.f;
        cs.maxParticles = 220;
        cs.lifetimeMin  = 2.0f;
        cs.lifetimeMax  = 3.5f;
        cs.spawnRadius  = 0.f;
        cs.spawnAreaCx  = 60.f;
        cs.spawnAreaCy  = fScrCy * 1.25f;
        cs.speedMin     = 400.f;
        cs.speedMax     = 700.f;
        cs.angleMin     = -6.f;
        cs.angleMax     =  6.f;
        cs.gravityX     = 0.f;
        cs.gravityY     = 8.f;
        cs.turbulence   = 30.f;
        cs.gustAmp      = 200.f;
        cs.gustFreq     = 0.5f;
        cs.velocityStretch = 0.0025f;
        cs.startScaleMin = 0.06f;
        cs.startScaleMax = 0.14f;
        cs.endScaleMin   = 0.06f;
        cs.endScaleMax   = 0.14f;
        cs.use3PhaseColor = true;

        cs.startColor[0]  = 0.85f;
        cs.startColor[1]  = 0.92f;
        cs.startColor[2]  = 1.00f;
        cs.startColor[3]  = 0.0f;

        cs.middleColor[0] = 0.85f;
        cs.middleColor[1] = 0.92f;
        cs.middleColor[2] = 1.00f;
        cs.middleColor[3] = 0.45f;

        cs.endColor[0]    = 0.85f;
        cs.endColor[1]    = 0.92f;
        cs.endColor[2]    = 1.00f;
        cs.endColor[3]    = 0.0f;

        cs.blendMode      = eSpriteBlendMode::ADDITIVE;

        ptrCharge->setPos(-60.f, fScrCy * 0.5f);

        ptrCont->addChild(ptrCharge);

        ptrCharge->prewarm(3);

        CContainer* pContRaw = ptrCont.get();

        auto ptrGustTick = std::make_shared<std::function<void()>>();
        std::weak_ptr<std::function<void()>> wGust = ptrGustTick;

        *ptrGustTick = [pContRaw, ptrSnow, ptrCharge, ptrFog, wGust]()
        {
            if (auto sp = wGust.lock())
                pContRaw->setTimeout(randomRange(4.0f, 7.5f), [sp]() { (*sp)(); });

            ptrSnow->getSettings().windX   = 150.f;
            ptrSnow->getSettings().gustAmp = 260.f;
            ptrCharge->getSettings().gustAmp = 340.f;

            ptrFog->removeSelfTweens();
            ptrFog->addSelfTween(eTweenProp::ALPHA, ptrFog->getAlpha(), 0.30f, 0.5f, Easing::linear);
            ptrFog->addSelfTween(eTweenProp::ALPHA, 0.30f, 0.f, 1.7f, Easing::outQuad, 0.6f);

            pContRaw->setTimeout(1.8f, [ptrSnow, ptrCharge]()
            {
                ptrSnow->getSettings().windX   = 40.f;
                ptrSnow->getSettings().gustAmp = 120.f;
                ptrCharge->getSettings().gustAmp = 200.f;
            });
        };

        pContRaw->setTimeout(2.0f, [sp = ptrGustTick]() { (*sp)(); });
    }
    else if (eKind == eBehindBgKind::FIREWORKS)
    {
        ptrSpr->setRgba(0x070a14ff);

        const float fScrCx = Engine::getCfg().INIT_SCR_CX;
        const float fScrCy = Engine::getCfg().INIT_SCR_CY;

        auto ptrStars = std::make_shared<CParticleSystem<>>(
            ParticlePresets::getStarfieldPreset(),
            std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::STARFIELD)).c_str());

        auto& ss = ptrStars->getSettings();

        ss.spawnRadius  = 0.f;
        ss.spawnAreaCx  = fScrCx * 1.2f;
        ss.spawnAreaCy  = fScrCy * 1.2f;
        ss.spawnRate    = 30.f;
        ss.maxParticles = 220;

        ptrStars->setPos(fScrCx * 0.5f, fScrCy * 0.5f);

        ptrCont->addChild(ptrStars);

        ptrStars->prewarm(7);

        auto ptrCrackle = std::make_shared<CParticleSystem<>>(
            ParticlePresets::getSparksPreset(),
            std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::SPARKS)).c_str());

        auto& cs = ptrCrackle->getSettings();

        cs.spawnRate   = 0.f;
        cs.maxParticles = 500;
        cs.speedMin    = 250.f;
        cs.speedMax    = 550.f;
        cs.gravityY    = 500.f;
        cs.lifetimeMin = 0.4f;
        cs.lifetimeMax = 0.8f;
        cs.velocityStretch = 0.004f;

        ptrCont->addChild(ptrCrackle);

        auto ptrAlive = std::make_shared<int>(0);

        CContainer* pContRaw = ptrCont.get();

        auto ptrSpawnTick = std::make_shared<std::function<void()>>();
        std::weak_ptr<std::function<void()>> wSpawnTick = ptrSpawnTick;

        *ptrSpawnTick = [pContRaw, ptrCrackle, ptrAlive, wSpawnTick]()
        {
            if (auto sp = wSpawnTick.lock())
                pContRaw->setTimeout(randomRange(0.55f, 1.15f), [sp]() { (*sp)(); });

            if (*ptrAlive >= 4)
                return;

            launchFirework(pContRaw, ptrCrackle, ptrAlive);

            if ((rand() % 4) == 0)
            {
                pContRaw->setTimeout(randomRange(0.10f, 0.25f), [pContRaw, ptrCrackle, ptrAlive]()
                {
                    if (*ptrAlive < 4)
                        launchFirework(pContRaw, ptrCrackle, ptrAlive);
                });
            }
        };

        pContRaw->setTimeout(0.4f, [sp = ptrSpawnTick]() { (*sp)(); });
    }
    else if (eKind == eBehindBgKind::SAKURA)
    {
        const char* arr[] = { "PARTICLES/sakura1", "PARTICLES/sakura2", "PARTICLES/sakura3" };

        ptrSpr->setRgba(0x1a1a2eff);

        const float fScrCx = Engine::getCfg().INIT_SCR_CX;
        const float fScrCy = Engine::getCfg().INIT_SCR_CY;

        const float fEmX = fScrCx * 0.5f;
        const float fEmY = fScrCy * 0.16f;

        auto ptrBranchBurst = std::make_shared<CParticleSystem<>>(ParticlePresets::getSakuraPreset(), arr);

        auto& bs = ptrBranchBurst->getSettings();

        bs.spawnRate     = 14.f;
        bs.maxParticles  = 400;
        bs.lifetimeMin   = 7.0f;
        bs.lifetimeMax   = 10.0f;
        bs.spawnRadius   = 0.f;
        bs.spawnAreaCx   = fScrCx;
        bs.spawnAreaCy   = fScrCy * 0.36f;
        bs.speedMin      = 60.f;
        bs.speedMax      = 160.f;
        bs.angleMin      = 75.f;
        bs.angleMax      = 105.f;
        bs.gravityX      = 10.f;
        bs.gravityY      = 55.f;
        bs.drag          = 0.25f;
        bs.startScaleMin = 0.22f;
        bs.startScaleMax = 0.38f;
        bs.endScaleMin   = 0.16f;
        bs.endScaleMax   = 0.28f;
        bs.fadeInTime    = 0.08f;
        bs.fadeOutTime   = 1.2f;
        bs.waveAmplitude = 22.f;
        bs.flutterSpeedMin = 2.0f;
        bs.flutterSpeedMax = 4.0f;
        bs.flutterSlip   = 35.f;
        bs.colorJitter   = 0.10f;
        bs.floorY        = (fScrCy - fEmY) + 20.f;

        ptrBranchBurst->setPos(fEmX, fEmY);
        ptrBranchBurst->setEmitting(false);

        ptrBranchBurst->setTimeout(1.3f, [ptrBranchBurst]()
        {
            ptrBranchBurst->setEmitting(true);
        });

        const float fRotFrom = -0.02f;
        const float fRotTo   =  0.03f;
        const float arrHeights[] = { 0.52f, 0.42f, 0.47f };

        float fCursorX = 0.f;
        int   nIdx     = 0;

        while (fCursorX < fScrCx)
        {
            auto ptrBranch = SpriteLoader::getInstance()->getSprite("UI/sakura");

            if (!ptrBranch)
                break;

            ptrBranch->setScaleToY(fScrCy * arrHeights[nIdx % SIZE_OF(arrHeights)]);

            const bool bFlip = (nIdx % 2) != 0;

            if (bFlip)
                ptrBranch->flipX();

            ptrCont->addChild(ptrBranch);

            const float fBranchCx = ptrBranch->calcNotTransCx();
            const float fBranchCy = ptrBranch->calcNotTransCy();

            if (fBranchCx <= 1.f || fBranchCy <= 1.f)
                break;

            const float fNextX = fCursorX + fBranchCx * 0.8f;
            const bool  bLast  = (fNextX >= fScrCx);

            float fX;

            if (bLast)
                fX = bFlip ? fScrCx : (fScrCx - fBranchCx);
            else
                fX = bFlip ? (fCursorX + fBranchCx) : fCursorX;

            const float fYFin   = -12.f;
            const float fYStart = fYFin - fBranchCy;

            ptrBranch->setPos(fX, fYStart);
            ptrBranch->setAlpha(0.f);
            ptrBranch->rotate(fRotFrom);

            const float fDelay = 0.15f + 0.10f * static_cast<float>(nIdx);

            ptrBranch->addSelfTween(eTweenProp::ALPHA, 0.f, 1.f, 0.4f, Easing::linear, fDelay);

            ptrBranch->addSelfTweenYoyo(eTweenProp::ROTATE,
                                        fRotFrom,
                                        fRotTo,
                                        2.6f,
                                        Easing::inOutSine,
                                        Easing::inOutSine,
                                        fDelay + 1.1f);

            const float fMidX = bFlip ? (fX - fBranchCx * 0.5f) : (fX + fBranchCx * 0.5f);
            const float fDx   = fBranchCx * 0.18f * (bFlip ? -1.f : 1.f);

            const float fBx0 = fMidX - fEmX;
            const float fBy0 = (fYFin + fBranchCy * 0.30f) - fEmY;

            const float fBx1 = fMidX + fDx - fEmX;
            const float fBy1 = (fYFin + fBranchCy * 0.62f) - fEmY;

            ptrBranch->addSelfTween(eTweenProp::Y,
                                    fYStart,
                                    fYFin,
                                    1.1f,
                                    Easing::outBack,
                                    fDelay,
                                    [ptrBranchBurst, fBx0, fBy0, fBx1, fBy1]()
                                    {
                                        ptrBranchBurst->burstAtLocal(fBx0, fBy0, 18);

                                        ptrBranchBurst->setTimeout(0.14f, [ptrBranchBurst, fBx1, fBy1]()
                                        {
                                            ptrBranchBurst->burstAtLocal(fBx1, fBy1, 15);
                                        });
                                    });

            fCursorX = fNextX;
            ++nIdx;
        }

        ptrCont->addChild(ptrBranchBurst);
    }
    else
    {
        ptrSpr->setRgba(0x14141cff);

        const float fScrCx = Engine::getCfg().INIT_SCR_CX;
        const float fScrCy = Engine::getCfg().INIT_SCR_CY;

        auto ptrBg = std::make_shared<CParticleSystem<>>(
            ParticlePresets::getDistantEmbersPreset(), "PARTICLES/flare_01");

        auto& bgs = ptrBg->getSettings();

        bgs.spawnAreaCx = fScrCx * 1.15f;
        bgs.spawnAreaCy = fScrCy * 1.15f;
        bgs.spawnRate = 40;
        bgs.maxParticles = 280;

        ptrBg->setPos(fScrCx * 0.5f, fScrCy * 0.5f);

        ptrCont->addChild(ptrBg);

        ptrBg->prewarm(5);

        auto ptrExplosion = std::make_shared<CParticleSystem<>>(
            ParticlePresets::getGrenadeExplosionPreset(),
            std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::GRENADE_EXPLOSION)).c_str());

        ptrCont->addChild(ptrExplosion);

        auto ptrDebris = std::make_shared<CParticleSystem<>>(
            ParticlePresets::getDebrisPreset(),
            std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::DEBRIS)).c_str());

        ptrDebris->getSettings().floorY = fScrCy + 40.f;

        ptrCont->addChild(ptrDebris);

        auto ptrAlive  = std::make_shared<int>(0);
        auto ptrShocks = std::make_shared<int>(0);

        CContainer* pContRaw = ptrCont.get();

        auto ptrSpawnTick = std::make_shared<std::function<void()>>();
        std::weak_ptr<std::function<void()>> wSpawnTick = ptrSpawnTick;

        *ptrSpawnTick = [pContRaw, ptrExplosion, ptrDebris, ptrBg, ptrAlive, ptrShocks, wSpawnTick]()
        {
            if (auto sp = wSpawnTick.lock())
                pContRaw->setTimeout(randomRange(0.4f, 0.9f), [sp]() { (*sp)(); });

            if (*ptrAlive >= 5)
                return;

            ++(*ptrAlive);

            spawnGrenade(pContRaw, ptrExplosion, ptrDebris, ptrBg, ptrAlive, ptrShocks);
        };

        pContRaw->setTimeout(0.4f, [sp = ptrSpawnTick]() { (*sp)(); });
    }

    return ptrCont;
}

void DialogDemo::initDialogsDemo()
{
    if (!_ptrDialogsDemoCont)
        return;

    _ptrDialogsDemoCont->removeAll();
    _ptrDialogsDemoLabel.reset();
    _demoStatusNodeId = 0;
    _demoDialogs.clear();

    std::string html;

    html += "<font name=\"marmelad\" size=\"52\" color=\"#CCBBAA\" shadow=\"1\" shadowcolor=\"#000000AA\" appear=\"rise\" dur=\"0.8\">\n";

    html += "<i>";
    html += locHtml("DLG_DEMO_BASE_TEXT");
    html += "</i><br>";

    html += locHtml("DLG_DEMO_CLICKABLE");
    html += "<br><br>\n";

    html += demoLink("demo_modal",
                     L10N::getInstance().tr("DLG_LINK_MODAL"),
                     "#40C4D4");
    html += " ";
    html += locHtml("DLG_MODAL_CAPTURES");
    html += "<br>";

    html += demoLink("demo_panel",
                     L10N::getInstance().tr("DLG_LINK_PANEL"),
                     "#5ED95E");
    html += " ";
    html += locHtml("DLG_PANEL_LIVES");
    html += "<br>\n";

    html += demoLink("demo_cb",
                     L10N::getInstance().tr("DLG_LINK_CB"),
                     "#FF7A8E");
    html += " ";
    html += locHtml("DLG_CB_CALLED");
    html += "<br>";

    html += demoLink("demo_stack",
                     L10N::getInstance().tr("DLG_LINK_STACK"),
                     "#A594F4");
    html += " ";
    html += locHtml("DLG_STACK_OPENS");
    html += "<br>";

    html += demoLink("demo_click",
                     L10N::getInstance().tr("DLG_LINK_CLICK"),
                     "#FFD54F");

    html += "&nbsp;&nbsp;";

    html += demoLink("demo_scroll",
                     L10N::getInstance().tr("DLG_LINK_SCROLL"),
                     "#4FC3F7");

    html += "\n</font>";

    _ptrDialogsDemoLabel = std::make_shared<StaticLabel>();

    _ptrDialogsDemoLabel->setBoxMode(eTextRenderType::HTML_BOX, getDialogCx() - 100.f);
    _ptrDialogsDemoLabel->setText(html.c_str());
    _ptrDialogsDemoLabel->setInteractive(true);
    _ptrDialogsDemoLabel->setXPosCentered(getDialogCx());
    _ptrDialogsDemoLabel->setY(10.f);

    _ptrDialogsDemoCont->addChild(_ptrDialogsDemoLabel);
    _ptrDialogsDemoCont->setY(70.f);

    auto& doc = _ptrDialogsDemoLabel->getHTMLDocument();

    _demoStatusNodeId = doc.find("demo_status");

    auto bind = [&doc](const char* id, SimpleCallback cb)
    {
        doc.onClick(id, [cb] { cb(); });
    };

    bind("demo_modal", [this]()
    {
        auto dlg = std::make_shared<DialogDemoExample>();

        if (!dlg->initExample(L10N::getInstance().tr("DLG_MODAL_TITLE"), 720.f, 450.f, nullptr))
            return;

        std::string sHtml;

        sHtml += R"HTML(<center>
<font name="edugot" size="30" color="#FFFFFF" shadow="1" shadowcolor="#00000088" appear="rise" dur="0.5">
)HTML";

        sHtml += locHtml("DLG_MODAL_TEXT");

        sHtml += R"HTML(
</font>
<br><br>
<table padding="6" border="0" grid="none" cellspacing="2" align="center"><tr><td>)HTML";

        sHtml += demoButton("btn_ok", L10N::getInstance().tr("BTN_OK"));

        sHtml += R"HTML(</td><td>)HTML";

        sHtml += demoButton("btn_cancel", L10N::getInstance().tr("BTN_CANCEL"));

        sHtml += R"HTML(</td></tr></table>
</center>)HTML";

        auto ptrCont = createBehindParticles(eBehindBgKind::SAKURA);
        dlg->setBehindBg(ptrCont);

        dlg->setHtml(sHtml.c_str());

        dlg->bindClick("btn_ok", [this, w = dlg->weak_from_this()]()
        {
            if (auto p = std::static_pointer_cast<BaseDialog>(w.lock()))
                p->close();
        });

        dlg->bindClick("btn_cancel", [this, w = dlg->weak_from_this()]()
        {
            if (auto p = std::static_pointer_cast<BaseDialog>(w.lock()))
                p->close();
        });

        trackDemoDialog(dlg);

        dlg->open();
    });

    bind("demo_cb", [this]()
    {
        auto dlg = std::make_shared<DialogDemoExample>();

        if (!dlg->initExample(L10N::getInstance().tr("DLG_CB_TITLE"), 720.f, 450.f, nullptr))
            return;

        auto ptrCont = createBehindParticles(eBehindBgKind::SOLITAIRE_STORM, dlg.get());
        dlg->setBehindBg(ptrCont, false);

        std::string sHtml;

        sHtml += R"HTML(<center>
<font name="edugot" size="30" color="#FFFFFF" shadow="1" shadowcolor="#00000088" appear="rise" dur="0.5">
)HTML";

        sHtml += locHtml("DLG_CB_TEXT");

        sHtml += R"HTML(
</font>
<br><br>
)HTML";

        sHtml += demoButton("btn_close", L10N::getInstance().tr("BTN_CLOSE"));

        sHtml += R"HTML(
</center>)HTML";

        dlg->setHtml(sHtml.c_str());

        dlg->bindClick("btn_close", [this, w = dlg->weak_from_this()]()
        {
            if (auto p = std::static_pointer_cast<BaseDialog>(w.lock()))
                p->close();
        });

        trackDemoDialog(dlg);

        dlg->open([this]()
        {
        });
    });

    bind("demo_panel", [this]()
    {
        auto dlg = std::make_shared<DialogDemoExample>();

        if (!dlg->initExample(nullptr,
                              800.f,
                              550.f,
                              nullptr,
                              eScrollType::E_ST_NONE,
                              false,
                              false,
                              true))
        {
            return;
        }

        std::string sHtml;

        sHtml += R"HTML(<center>
<font name="edugot" size="35" color="#000000FF" shadow="1" shadowcolor="#00000044" appear="rise" dur="0.5">
)HTML";

        sHtml += locHtml("DLG_PANEL_MAIN_TEXT");

        sHtml += R"HTML(
</font>
<br><br>
<font size="45">
)HTML";

        sHtml += demoLink("panel_left",
                          L10N::getInstance().tr("DLG_PANEL_LEFT"),
                          "#2E7D32");

        sHtml += "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";

        sHtml += demoLink("panel_right",
                          L10N::getInstance().tr("DLG_PANEL_RIGHT"),
                          "#007A87");

        sHtml += "<br><br>";

        sHtml += demoLink("dlg_close",
                          L10N::getInstance().tr("BTN_CLOSE"),
                          "#D32F2F");

        sHtml += R"HTML(
</font></center>)HTML";

        dlg->setHtml(sHtml.c_str(), 90.f, 120.f);

        auto ptrCont = createBehindParticles(eBehindBgKind::HEARTS);
        dlg->setBehindBg(ptrCont, false);

        dlg->bindClick("dlg_close", [w = dlg->weak_from_this()]()
        {
            if (auto p = std::static_pointer_cast<DialogDemoExample>(w.lock()))
            {
                p->close();
            }
        });

        dlg->bindClick("panel_left", [w = dlg->weak_from_this()]()
        {
            if (auto p = std::static_pointer_cast<DialogDemoExample>(w.lock()))
            {
                p->openSidePanel(SIDE_LEFT);
            }
        });

        dlg->bindClick("panel_right", [w = dlg->weak_from_this()]()
        {
            if (auto p = std::static_pointer_cast<DialogDemoExample>(w.lock()))
                p->openSidePanel(SIDE_RIGHT);
        });

        trackDemoDialog(dlg);

        dlg->open([w = dlg->weak_from_this()]()
        {
            if (auto p = std::static_pointer_cast<DialogDemoExample>(w.lock()))
                p->closeAllSidePanels();
        });
    });

    bind("demo_stack", [this]()
    {
        openStackLevel(1);
    });

    bind("demo_click", [this]()
    {
        auto dlg = std::make_shared<DialogDemoExample>();

        if (!dlg->initExample(L10N::getInstance().tr("DLG_CLICK_TITLE"),
                              620.f,
                              340.f,
                              nullptr,
                              eScrollType::E_ST_NONE,
                              false,
                              true))
        {
            return;
        }

        auto ptrCont = createBehindParticles(eBehindBgKind::GRENADES, dlg.get());
        dlg->setBehindBg(ptrCont, false);

        std::string sHtml;

        sHtml += R"HTML(<center>
<font name="edugot" size="30" color="#FFFFFF" shadow="1" shadowcolor="#00000088" appear="rise" dur="0.5">
)HTML";

        sHtml += locHtml("DLG_CLICK_TEXT");

        sHtml += R"HTML(
</font>
</center>)HTML";

        dlg->setHtml(sHtml.c_str(), 110.f);

        trackDemoDialog(dlg);

        dlg->open();
    });

    bind("demo_scroll", [this]()
    {
        auto dlg = std::make_shared<DialogDemoExample>();

        if (!dlg->initExample(L10N::getInstance().tr("DLG_SCROLL_TITLE"),
                              760.f,
                              600.f,
                              nullptr,
                              eScrollType::E_ST_NONE,
                              true,
                              false))
        {
            return;
        }

        std::string sHtml;

        sHtml += "<font name=\"edugot\" size=\"28\" color=\"#FFFFFF\" shadow=\"1\" shadowcolor=\"#00000088\">";
        sHtml += htmlEscape(L10N::getInstance().tr("DLG_SCROLL_HELP"));
        sHtml += "<br><br>";

        for (int i = 1; i <= 24; ++i)
        {
            sHtml += "<b>";
            sHtml += htmlEscape(std::vformat(L10N::getInstance().tr("DLG_SCROLL_LINE_FMT"), std::make_format_args(i)));
            sHtml += "</b><br>";
        }

        sHtml += "</font>";

        auto ptrCont = createBehindParticles(eBehindBgKind::BLIZZARD, dlg.get());
        dlg->setBehindBg(ptrCont, false);

        const float fPanelCx = dlg->getDialogCx() - 40.f;
        const float fPanelCy = dlg->getDialogCy();

        auto ptrPanel = std::make_shared<PanelHTML>();

        ptrPanel->init(sHtml.c_str(),
                       fPanelCx,
                       fPanelCy,
                       std::static_pointer_cast<CContainer>(dlg),
                       true);

        ptrPanel->setPos(20, -10);
        ptrPanel->showAsPanel(E_IH_DISPATCHBYPARENT);
        ptrPanel->setVisible(true);
        ptrPanel->finalizeScroll();

        dlg->bringChildUnderForeground(ptrPanel);

        trackDemoDialog(dlg);
        trackDemoDialog(std::static_pointer_cast<BaseDialog>(ptrPanel));

        dlg->open([ptrPanel]()
        {
            if (ptrPanel && ptrPanel->isVisible())
                ptrPanel->close();
        });
    });
}