#include "engine.h"
#include "assetloader.h"
#include "textrender.h"
#include "timecounter.h"
#include "soundfx.h"
#include "msched.h"
#include "mp3player.h"
#include "htmldom.h"
#include "audiomanager.h"
#include "bglayer.h"
#include "l10n.h"

_G2D_NAMESPACE_BEGIN_

namespace
{
#ifdef TARGET_EMSCRIPTEN
EM_JS(const char*, js_getSystemLang, (),
{
    const lang = (navigator.language || navigator.userLanguage || "en").toLowerCase();
    const len = lengthBytesUTF8(lang) + 1;
    const buf = _malloc(len);
    stringToUTF8(lang, buf, len);
    return buf;
});
#endif //TARGET_EMSCRIPTEN
}

eLang detectSystemLang()
{
#ifdef TARGET_EMSCRIPTEN
    // Emscripten: берем язык из браузера
    const char* lang = js_getSystemLang();
    const eLang result = (std::strstr(lang, "ru") == lang) ? eLang::RU : eLang::EN;
    free((void*)lang); // память выделена через _malloc в JS
    return result;
#else
    // Остальные платформы: через setlocale (читаем окружение ОС)
    const char* loc = std::setlocale(LC_ALL, "");
    if (loc && (std::strstr(loc, "ru") != nullptr || std::strstr(loc, "Russian") != nullptr))
        return eLang::RU;
    return eLang::EN;
#endif
}

void gameLoop(float dt_total)
{
    const auto& cfg = Engine::getInstance().getCfg();

    // Safety - clamp dt if it is too large (tab switch, debugger pause, hiccup)
    const float dt = std::min(dt_total, cfg.MAX_DT);

    AssetLoader::instance().update(dt);
    TimeCounter::update(dt);
    CSched::getInstance()->onFrame(dt);
    AudioManager::get().update(dt);
    HTMLDom::getInstance()->update(dt);
    Engine::getInstance().getApp()->onFrame(dt);

    if (Engine::getInstance().isInitialPreloadDone())
    {
        CGfx::getInstance()->begin(dt);
        CGfx::getInstance()->renderGraph(dt);
        CGfx::getInstance()->end();
    }
}

#ifdef TARGET_EMSCRIPTEN
extern "C"
{
EM_BOOL onAnimationFrame(double time, void* pUserData)
{
    static double startTime = time;
    gameLoop((time - startTime) / 1000.0);
    startTime = time;
    return EM_TRUE;
}
};
#endif //TARGET_EMSCRIPTEN

#ifdef TARGET_EMSCRIPTEN
EM_JS(void, js_setMouseCursor, (const char* lpccStr),
{
    const cursorStr = UTF8ToString(lpccStr);
    if (!window._glCanv)
        window._glCanv = document.getElementById('canvas');
    window._glCanv.style.cursor = cursorStr;
});
#endif

#ifdef TARGET_WIN
GLFWwindow* window;

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

//========================================================================
// Callback function for mouse button events
//========================================================================
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT)
        return;

    if (action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        double cursorX = 0;
        double cursorY = 0;
        glfwGetCursorPos(window, &cursorX, &cursorY);

        if (action == GLFW_PRESS)
            InputController::getInstance()->onPointerDown(cursorX, cursorY, 1);
        else
            InputController::getInstance()->onPointerUp(cursorX, cursorY, 1);
    }
}

//========================================================================
// Callback function for cursor motion events
//========================================================================
void cursor_position_callback(GLFWwindow* window, double x, double y)
{
    InputController::getInstance()->onPointerMove(x, y, 1);
}

//========================================================================
// Callback function for scroll events
//========================================================================
void scroll_callback(GLFWwindow* window, double x, double y)
{
    InputController::getInstance()->onMouseWheel(y * -100.f);
}

//========================================================================
// Callback function for framebuffer resize events
//========================================================================
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    if (width > 0 && height > 0)
    {
        CSceneResize::getInstance()->setAvailWidth(width);
        CSceneResize::getInstance()->setAvailHeight(height);
    }
}

GLFWwindow* initGLFW()
{
    int width, height;
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    auto &cfg = Engine::getInstance().getCfg();
    window = glfwCreateWindow(cfg.INIT_SCR_CX, cfg.INIT_SCR_CY, cfg.WINDOW_TITLE.c_str(), NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    glfwGetFramebufferSize(window, &width, &height);
    framebuffer_size_callback(window, width, height);
    return window;
}

void run_GLFW_loop()
{
    double t, dt_total, t_old, dt;
    t_old = glfwGetTime() - 0.01;
    while (!glfwWindowShouldClose(window))
    {
        t = glfwGetTime();
        dt_total = t - t_old;
        t_old = t;
        gameLoop(dt_total);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
#endif //TARGET_WIN

void Engine::initAndRun(App* pApp, const EngineCfg* pCfg)
{
    assert(pApp);
    _pApp = pApp;
    if (pCfg)
        _cfg = *pCfg;

    // Детектим язык системы, если вызывающий код не задал его явно
    if (!_cfg.LANG_OVERRIDE)
        _cfg.LANG = detectSystemLang();

    L10N::getInstance().load(_cfg.LANG);

    _isInitialized = true;

#ifdef TARGET_EMSCRIPTEN
    //LOG_ALERT("!!! === BREAKPOINTS TIME === !!!\nDWARF loading is async\nso this alert is just for some pause\nto wait DWARF is loaded");
#endif //TARGET_EMSCRIPTEN

#ifdef TARGET_WIN
    auto glfwW = initGLFW();
#endif //TARGET_WIN

    InputController::getInstance()->init();
#ifdef TARGET_WIN
    InputController::getInstance()->initKeyboard(glfwW);
#endif //TARGET_WIN

    bool isinited = CGfx::getInstance()->setup();
    TextRender::getInstance()->init();

    BgLayer::getInstance()->init();
    BgLayer::getInstance(eRenderLayer::FOREGROUND)->init();

    CGfx::getInstance()->getBgRoot()->addChild(BgLayer::getInstance());
    CGfx::getInstance()->getFgRoot()->addChild(BgLayer::getInstance(eRenderLayer::FOREGROUND));

    CSceneResize::getInstance()->init();

    getApp()->init();
    AudioManager::get().init();

#ifdef TARGET_EMSCRIPTEN
    emscripten_request_animation_frame_loop(onAnimationFrame, NULL);
#endif //TARGET_EMSCRIPTEN

#ifdef TARGET_WIN
    run_GLFW_loop();
#endif//TARGET_WIN
}

void Engine::setMouseCursor(eMouseCursorType eType)
{
#ifdef TARGET_WIN
    static GLFWcursor* pCurArrow = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    static GLFWcursor* pCurHand  = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#endif
    if (_mouseCursorType != eType)
    {
        _mouseCursorType = eType;
        switch (eType)
        {
        case eMouseCursorType::E_MCT_NORMAL:
        {
#ifdef TARGET_EMSCRIPTEN
            js_setMouseCursor("default");
#endif //TARGET_EMSCRIPTEN
#ifdef TARGET_WIN
            glfwSetCursor(window, pCurArrow);
#endif //TARGET_WIN
        }
        break;
        case eMouseCursorType::E_MCT_POINTER:
        {
#ifdef TARGET_EMSCRIPTEN
            js_setMouseCursor("pointer");
#endif //TARGET_EMSCRIPTEN
#ifdef TARGET_WIN
            glfwSetCursor(window, pCurHand);
#endif //TARGET_WIN
        }
        break;
        default:
        {
            assert(false);
        }
        break;
        }
    }
}

_G2D_NAMESPACE_END_