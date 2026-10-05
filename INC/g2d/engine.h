#pragma once

#include "g2d.h"
#include "eventdef.h"
#include "sceneresize.h"
#include "inputcontroller.h"
#include "gfx.h"
#include "ptrsdefs.h"

_G2D_NAMESPACE_BEGIN_

enum class eLang
{
    RU = 0,
    EN,
};

enum class eTimestepMode
{
    VARIABLE = 0,   // dt клампится сверху MAX_DT, симуляция идет с кадровым dt
    FIXED,          // симуляция идет фиксированными шагами FIXED_TIMESTEP,
                    // рендер - один раз с интерполяцией (CGfx::getRenderAlpha)
};

enum eTRAlign : int
{
    TR_ALIGN_LEFT     = 1<<0,
    TR_ALIGN_CENTER   = 1<<1,
    TR_ALIGN_RIGHT    = 1<<2,
    TR_ALIGN_TOP      = 1<<3,
    TR_ALIGN_MIDDLE   = 1<<4,
    TR_ALIGN_BOTTOM   = 1<<5,
    TR_ALIGN_BASELINE = 1<<6,
};
inline constexpr int MAX_FONTS = 30;

using FontsVec = std::array<std::optional<std::pair<std::string, std::string>>, MAX_FONTS>;

struct HTMLRenderConfig
{
    float   fH3FontSize = 45;
    float   fH2FontSize = 65;
    float   fH1FontSize = 85;
};

struct EngineCfg
{
    static constexpr size_t MAX_PARTICLES                       = 1000;
    float                   MAX_ASPECT_RATIO                    = (1270.f * 1.5f) / 720.f;
    float                   ASPECT_RATIO                        = 1270.f / 720.f;
    float                   INIT_SCR_CX                         = 1270.f;
    float                   INIT_SCR_CY                         = 720.f;
    float                   BG_SCALE                            = 1.2f;
    float                   SCROLL_BUMP                         = 80.f;
    float                   SCROLL_BACK_TIME                    = 0.3f;
    float                   DRAG_BEGIN_DISTANCE                 = 10.f;
    std::string             WINDOW_TITLE                        = "Graph2D";
    std::string             DATA_DIR                            = "./";
    eLang                   LANG                                = eLang::EN;
    std::optional<eLang>    LANG_OVERRIDE                       ;// eLang::EN;// если задан, детект языка системы не выполняется
    eTimestepMode           TIMESTEP_MODE                       = eTimestepMode::VARIABLE;
    float                   MAX_DT                              = 0.05f;  // потолок кадрового dt (оба режима)
    float                   FIXED_TIMESTEP                      = 1.f / 60.f;
    int                     MAX_FIXED_STEPS                     = 5;      // защита от spiral of death
    std::string             SPINE_INTERACTIVE_BOUNDING_BOX_NAME = "bounding_box_character";
    std::string             DEFAULT_FONT_NAME                   = "default";
    std::string             NOVEL_FONT                          = "";
    float                   NOVEL_FONT_TEXT_SIZE                = 22;
    float                   DEFAULT_FONT_SIZE                   = 35.f;
    float                   DEFAULT_FONT_HEIGHT_MUL             = 1.f;
    int                     DEFAULT_FONT_ALIGN                  = eTRAlign::TR_ALIGN_LEFT | eTRAlign::TR_ALIGN_TOP;
    bool                    DEFAULT_SHADOW                      = true;
    int                     FONT_TEXTURE_DIM                    = 1024;
    FontsVec                FONTS;
    HTMLRenderConfig        HTMLCfg;
    CSpritePtr              wb;

    void addFont(LPCTSTR lpszFontName, LPCTSTR lpszFontPath)
    {
        bool bAdded = false;
        for (int i = 0; i < FONTS.size(); i++)
        {
            if (!FONTS[i])
            {
                FONTS[i].emplace(lpszFontName, lpszFontPath);
                bAdded = true;
                break;
            }
        }
        assert(bAdded);
    }

    void setResolution(float cx, float cy)
    {
        MAX_ASPECT_RATIO    = (cx * 1.5f) / cy;
        ASPECT_RATIO        = cx / cy;
        INIT_SCR_CX         = cx;
        INIT_SCR_CY         = cy;
    }
};


class App
{
public:
    virtual void        onFrame         (float      dt) = 0;
    virtual void        onResize        (float      cx, 
                                         float      cy) = 0;
    virtual void        init            (void)          = 0;
};

class Engine
{
    friend class Preloader;
private:
    inline static std::stack<EngineCfg>  _cfgStack;
    inline static EngineCfg              _cfg;
    App*                                 _pApp            = nullptr;
    bool                                 _isInitialized   = false;
    bool                                 _isPreloaded     = false;
    eMouseCursorType                     _mouseCursorType = eMouseCursorType::E_MCT_NORMAL;

private:
                                Engine                  (void) = default;
                                ~Engine                 (void) = default;
private:
    void                        setInitialPreloadDone   (void) {_isPreloaded = true;}

public:
                                Engine                  (Engine&&) = delete;
    Engine&                     operator=               (Engine&&) = delete;
                                Engine                  (const              Engine&) = delete;
    Engine&                     operator=               (const              Engine&) = delete;

public:
    void                        initAndRun              (App*               pApp, 
                                                         const EngineCfg*   pCfg = nullptr);
    void                        setMouseCursor          (eMouseCursorType   eType);
    bool                        isInitialPreloadDone    (void) { return _isPreloaded; }
    static App*                 getApp                  (void) { assert(getInstance()._isInitialized); assert(getInstance()._pApp); return getInstance()._pApp; }
    static constexpr EngineCfg& getCfg                  (void) { return _cfg; }
    static Engine&              getInstance             (void) 
    {
        static Engine instance;
        return instance;
    }
    static void                 pushCfg                 (void) {_cfgStack.push(_cfg);}
    static void                 popCfg                  (void) {assert(!_cfgStack.empty()); _cfg = std::move(_cfgStack.top()); _cfgStack.pop();}

};

eLang detectSystemLang();

_G2D_NAMESPACE_END_