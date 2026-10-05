#include "inputcontroller.h"
#include "sceneresize.h"
#include "mp3player.h"
#include "engine.h"
#include "htmldom.h"
#include <cstdint>

_G2D_NAMESPACE_BEGIN_

#ifdef TARGET_EMSCRIPTEN

// Скрытый DOM-input для вызова системной виртуальной клавиатуры на
// мобильных. iOS не фокусит display:none-элементы - поэтому opacity 0.01.
EM_JS(void, jsVkShow, (const char* val, int password), {
    var inp = window.__vkInput;

    if (!inp)
    {
        inp = document.createElement('input');
        inp.setAttribute('autocomplete', 'off');
        inp.setAttribute('autocorrect', 'off');
        inp.setAttribute('autocapitalize', 'off');
        inp.setAttribute('spellcheck', 'false');
        inp.setAttribute('enterkeyhint', 'done');

        inp.style.position = 'fixed';
        inp.style.top = '0px';
        inp.style.left = '0px';
        inp.style.width = '2px';
        inp.style.height = '2px';
        inp.style.opacity = '0.01';
        inp.style.pointerEvents = 'none';

        document.body.appendChild(inp);

        inp.addEventListener('input', function() {
            __onVirtualKeyboard(inp.value, inp.selectionStart);
        });

        inp.addEventListener('keydown', function(e) {
            if (e.key === 'Enter') { e.preventDefault(); __onVirtualEnter(); }
        });

        inp.addEventListener('blur', function() {
            __onVirtualKeyboard(0, -1);
        });

        window.__vkInput = inp;
    }

    inp.setAttribute('type', password ? 'password' : 'text');
    inp.value = UTF8ToString(val);
    inp.focus();
});

EM_JS(void, jsVkHide, (void), {
    if (window.__vkInput)
        window.__vkInput.blur();
});

EM_JS(void, jsSetClipboard, (const char* val), {
    var s = UTF8ToString(val);

    if (navigator.clipboard && navigator.clipboard.writeText)
    {
        navigator.clipboard.writeText(s).catch(function(){});
        return;
    }

    var ta = document.createElement('textarea');
    ta.value = s;
    document.body.appendChild(ta);
    ta.select();

    try {
        document.execCommand('copy');
    } catch (e) {
    }

    ta.remove();
});

EM_JS(void, jsRequestClipboardPaste, (void), {
    if (!navigator.clipboard || !navigator.clipboard.readText) {
        console.error("API буфера обмена не поддерживается или контекст небезопасен (нужен HTTPS/localhost)");
        return;
    }

    navigator.clipboard.readText().then(function(t) {
        console.log("Текст успешно прочитан из буфера:", t);
        
        if (typeof stringToNewUTF8 !== 'function') {
            console.error("Ошибка: stringToNewUTF8 не экспортирована из Emscripten!");
            return;
        }
        if (typeof _free !== 'function') {
            console.error("Ошибка: _free не экспортирована из Emscripten!");
            return;
        }

        var buf = stringToNewUTF8(t);
        if (!buf) {
            console.error("Не удалось выделить память под строку (stringToNewUTF8 вернул 0)");
            return;
        }

        console.log("Передаем указатель в C++...");
        if (typeof __onKeyboardInputEx === 'function') {
            __onKeyboardInputEx(buf, 0, 0);
        } else {
            console.error("Ошибка: Функция __onKeyboardInputEx не найдена в JS-контексте!");
        }
        
        _free(buf);
    }).catch(function(err) {
        console.error("Браузер заблокировал чтение буфера обмена:", err);
    });
});

extern "C"
{

EMSCRIPTEN_KEEPALIVE float get_dpr() {
    return CSceneResize::getInstance()->get_dpr();
}

EM_JS(void, initController, (void),
{
    (function() {
        var hiddenProperty = "";
        var visibilityEvent = "";

        // 1. Проверяем поддержку Page Visibility API (с учетом префиксов старых браузеров)
        if (typeof document.hidden !== "undefined") {
            hiddenProperty = "hidden";
            visibilityEvent = "visibilitychange";
        } else if (typeof document.msHidden !== "undefined") {
            hiddenProperty = "msHidden";
            visibilityEvent = "msvisibilitychange";
        } else if (typeof document.webkitHidden !== "undefined") {
            hiddenProperty = "webkitHidden";
            visibilityEvent = "webkitvisibilitychange";
        }

        // Функции-обработчики для логики игры/приложения
        function onAway() {
            __onBlur();
        }

        function onBack() {
            __onFocus();
        }

        // 2. Логика подписки
        if (visibilityEvent) {
            // Если Visibility API доступен
            document.addEventListener(visibilityEvent, function() {
                if (document[hiddenProperty]) {
                    onAway();
                } else {
                    onBack();
                }
            });
        } else {
            // Резервный вариант: использование событий focus/blur (для очень старых браузеров)
            // В iframe blur может не сработать при переключении вкладок, но это единственный fallback
            window.addEventListener("blur", onAway);
            window.addEventListener("focus", onBack);
        }
    })();

    if (!window._glCanv)
        window._glCanv = document.getElementById('canvas');

    window._glCanv.addEventListener("pointerdown", function(e){
        var rect = e.target.getBoundingClientRect();
        var x    = e.clientX - rect.left;
        var y    = e.clientY - rect.top;

        if (!e.target.hasPointerCapture(e.pointerId))
            window._glCanv.setPointerCapture(e.pointerId);

        e.preventDefault();

        __onPointerDown(x, y, _get_dpr());
    });

    window._glCanv.addEventListener("pointerup", function(e){
        var rect = e.target.getBoundingClientRect();
        var x    = e.clientX - rect.left;
        var y    = e.clientY - rect.top;

        e.preventDefault();

        __onPointerUp(x, y, _get_dpr());
    });

    window._glCanv.addEventListener("pointermove", function(e){
        var rect = e.target.getBoundingClientRect();
        var x    = e.clientX - rect.left;
        var y    = e.clientY - rect.top;

        e.preventDefault();

        __onPointerMove(x, y, _get_dpr());
    });

    // === FIX START ===
    function onWheelEvent(e)
    {
        var dy = e.deltaY;

        // 0 = pixels, 1 = lines, 2 = pages
        if (e.deltaMode === 1)
            dy *= 32;
        else if (e.deltaMode === 2)
            dy *= window.innerHeight;

        e.preventDefault();

        __onMouseWheel(dy);
    }

    window._glCanv.addEventListener("wheel", onWheelEvent, { passive: false });
    window._glCanv.addEventListener("mousewheel", onWheelEvent, { passive: false });
    // === FIX END ===

    window._glCanv.addEventListener("click", function(e){
        e.preventDefault();
    }, false);

    window._glCanv.addEventListener("mousedown", function(e){
        e.preventDefault();
    }, false);

    window.addEventListener("keydown", function(e){
        var mods = 0;

        if (e.shiftKey)
            mods |= 1;

        if (e.ctrlKey || e.metaKey)
            mods |= 2;

        if (e.altKey)
            mods |= 4;

        if ((e.ctrlKey || e.metaKey) && !e.altKey)
        {
            var k = (e.key || "").toLowerCase();
            var clipSpecial = 0;

            if (k === "c")
                clipSpecial = 9;       // SK_COPY
            else if (k === "v")
                clipSpecial = 10;      // SK_PASTE
            else if (k === "x")
                clipSpecial = 11;      // SK_CUT
            else if (k === "a")
                clipSpecial = 12;      // SK_SELECT_ALL

            if (clipSpecial)
            {
                e.preventDefault();
                __onKeyboardInputEx(0, clipSpecial, mods);
                return;
            }
        }

        if (e.ctrlKey || e.metaKey || e.altKey)
        {
            if (!e.shiftKey)
                return;
        }

        var special = 0;

        switch (e.key)
        {
        case "Backspace":  special = 1; break;
        case "Delete":     special = 2; break;
        case "ArrowLeft":  special = 3; break;
        case "ArrowRight": special = 4; break;
        case "Home":       special = 5; break;
        case "End":        special = 6; break;
        case "Enter":      special = 7; break;
        case "Escape":     special = 8; break;
        }

        if (special)
        {
            e.preventDefault();
            __onKeyboardInputEx(0, special, mods);
        }
        else if (e.key && e.key.length === 1)
        {
            // e.key - строка (UTF-8), а не код символа: ловим и
            // кириллицу, и всю BMP. Повторы при зажатой клавише
            // идут штатно (как и должно для эдита).
            e.preventDefault();

            var n = lengthBytesUTF8(e.key) + 1;
            var buf = stackAlloc(n);
            stringToUTF8(e.key, buf, n);

            __onKeyboardInputEx(buf, 0, mods);
        }
    }, true);
});

EMSCRIPTEN_KEEPALIVE void _onBlur()
{
    InputController::getInstance()->onBlur();
}

EMSCRIPTEN_KEEPALIVE void _onFocus()
{
    InputController::getInstance()->onFocus();
}

EMSCRIPTEN_KEEPALIVE void _onPointerUp(int32_t x, int32_t y, double dpr)
{
    InputController::getInstance()->onPointerUp(x, y, dpr);
}

EMSCRIPTEN_KEEPALIVE void _onPointerDown(int32_t x, int32_t y, double dpr)
{
    InputController::getInstance()->onPointerDown(x, y, dpr);
}

EMSCRIPTEN_KEEPALIVE void _onPointerMove(int32_t x, int32_t y, double dpr)
{
    InputController::getInstance()->onPointerMove(x, y, dpr);
}

EMSCRIPTEN_KEEPALIVE void _onMouseWheel(int32_t deltaY)
{
    InputController::getInstance()->onMouseWheel(deltaY);
}

EMSCRIPTEN_KEEPALIVE void _onKeyboardInput(const char* szTextUtf8, int specialKey)
{
    InputController::getInstance()->onKeyboardInput(szTextUtf8, (eSpecialKey)specialKey, 0);
}

EMSCRIPTEN_KEEPALIVE void _onKeyboardInputEx(const char* szTextUtf8, int specialKey, int mods)
{
    InputController::getInstance()->onKeyboardInput(szTextUtf8, (eSpecialKey)specialKey, mods);
}

EMSCRIPTEN_KEEPALIVE void _onVirtualKeyboard(const char* szTextUtf8, int caret)
{
    InputController::getInstance()->emitVirtualKeyboard(szTextUtf8, caret);
}

EMSCRIPTEN_KEEPALIVE void _onVirtualEnter(void)
{
    InputController::getInstance()->onKeyboardInput(nullptr, eSpecialKey::SK_ENTER, 0);
}

};

#endif //TARGET_EMSCRIPTEN

InputController* InputController::_instance = NULL;

InputController* InputController::getInstance()
{
    if (InputController::_instance == NULL)
        InputController::_instance = new InputController();

    return InputController::_instance;
}

void InputController::init()
{
    assert(!_isInitDone);

    if (!_isInitDone)
    {
#ifdef TARGET_EMSCRIPTEN
        initController();
#endif//TARGET_EMSCRIPTEN

        _isInitDone = true;
    }
}

void InputController::translateCoords(int32_t& x, int32_t& y, eCoordType eTranslate)
{
    switch (eTranslate)
    {
    case eCoordType::INGAME_COORDS:
    {
        //x -= CSceneResize::getInstance()->getGameOffsX();
        //y -= CSceneResize::getInstance()->getGameOffsY();
    }
    break;

    default:
    {
        assert(false);
    }
    break;
    }
}

void InputController::onBlur()
{
    _bFocused = false;

    emit(InputController::EVT_ON_BLUR);
}

void InputController::onFocus()
{
    _bFocused = true;

    emit(InputController::EVT_ON_FOCUS);
}

void InputController::onPointerUp(int32_t x, int32_t y, float dpr)
{
    int32_t dprX            = x * dpr;
    int32_t dprY            = y * dpr;

    _isDown           = false;
    _isMovedWhileDown = false;

    _curMousePos.set(dprX, dprY);

    if (!_nBlocked)
    {
        translateCoords(dprX, dprY);

        if (_exclusiveMouseListener.size())
            _exclusiveMouseListener.top()->onEvent(InputController::EVT_ON_MOUSE_UP, (void*)dprX, (void*)dprY);
        else
            emit(InputController::EVT_ON_MOUSE_UP, (void*)dprX, (void*)dprY);
    }
}

void InputController::onPointerDown(int32_t x, int32_t y, float dpr)
{
    int32_t dprX            = x * dpr;
    int32_t dprY            = y * dpr;

    _dpr              = dpr;
    _isDown           = true;
    _isMovedWhileDown = false;

    _ptDownAt.set(x, y);
    _lastMousePos.set(x, y);
    _curMousePos.set(dprX, dprY);

    if (!_nBlocked)
    {
        translateCoords(dprX, dprY);

        if (_exclusiveMouseListener.size())
            _exclusiveMouseListener.top()->onEvent(InputController::EVT_ON_MOUSE_DOWN, (void*)dprX, (void*)dprY);
        else
            emit(InputController::EVT_ON_MOUSE_DOWN, (void*)dprX, (void*)dprY);
    }
}

void InputController::onPointerMove(int32_t x, int32_t y, float dpr)
{
    int32_t dprX            = x * dpr;
    int32_t dprY            = y * dpr;

    _lastMousePos.set(x, y);
    _curMousePos.set(dprX, dprY);

    if (!_nBlocked)
    {
        translateCoords(dprX, dprY);

        if (_isDown)
            _isMovedWhileDown = true;

        if (_exclusiveMouseListener.size())
            _exclusiveMouseListener.top()->onEvent(InputController::EVT_ON_MOUSE_MOVE, (void*)dprX, (void*)dprY);
        else
            emit(InputController::EVT_ON_MOUSE_MOVE, (void*)dprX, (void*)dprY);
    }
}

void InputController::onMouseWheel(int32_t deltaY)
{
    if (!_nBlocked)
    {
        if (_exclusiveMouseListener.size())
            _exclusiveMouseListener.top()->onEvent(InputController::EVT_ON_MOUSE_WHEEL, (void*)deltaY);
        else
            emit(InputController::EVT_ON_MOUSE_WHEEL, (void*)deltaY);
    }
}

void InputController::onKeyboardInput(const char* szTextUtf8, eSpecialKey eKey, int modifiers)
{
    if (_nBlocked)
        return;

    if (eKey == eSpecialKey::SK_NONE && (!szTextUtf8 || !*szTextUtf8))
        return;

    // focused <edit> получает ввод сам - функционал контрола,
    // клей с хоста не нужен. Не-фокус = прозрачный no-op.
    HTMLDom::getInstance()->handleKeyboardInput(szTextUtf8, eKey, modifiers);

    if (_exclusiveMouseListener.size())
        _exclusiveMouseListener.top()->onEvent(InputController::EVT_ON_KEYBOARD,
                                               (void*)szTextUtf8, (void*)(intptr_t)eKey);
    else
        emit(InputController::EVT_ON_KEYBOARD,
             (void*)szTextUtf8, (void*)(intptr_t)eKey);
}

void InputController::showVirtualKeyboard(const char* szInitialUtf8, bool bPassword)
{
#ifdef TARGET_EMSCRIPTEN
    jsVkShow(szInitialUtf8 ? szInitialUtf8 : "", bPassword ? 1 : 0);
#else
    (void)szInitialUtf8;
    (void)bPassword;
#endif
}

void InputController::hideVirtualKeyboard(void)
{
#ifdef TARGET_EMSCRIPTEN
    jsVkHide();
#endif
}

void InputController::emitVirtualKeyboard(const char* szTextUtf8, int caret)
{
    if (_nBlocked)
        return;

    // Зеркалим текст виртуальной клавиатуры в focused edit - сам контрол.
    auto dom = HTMLDom::getInstance();

    if (!szTextUtf8 || caret < 0)
        dom->blurFocusedEdit();
    else if (unsigned long long id = dom->getFocusedEditId())
    {
        dom->setEditValue(id, szTextUtf8, true);
        dom->setEditCursor(id, caret);
    }

    if (_exclusiveMouseListener.size())
        _exclusiveMouseListener.top()->onEvent(InputController::EVT_ON_VIRTUAL_KEYBOARD,
                                               (void*)szTextUtf8, (void*)(intptr_t)caret);
    else
        emit(InputController::EVT_ON_VIRTUAL_KEYBOARD,
             (void*)szTextUtf8, (void*)(intptr_t)caret);
}

void InputController::setClipboardText(const char* text)
{
#ifdef TARGET_EMSCRIPTEN
    jsSetClipboard(text ? text : "");
#else
    if (_glfwWindow)
        glfwSetClipboardString((GLFWwindow*)_glfwWindow, text ? text : "");
#endif
}

std::string InputController::getClipboardText(void)
{
#ifdef TARGET_EMSCRIPTEN
    // Синхронно прочитать браузерный клипборд нельзя.
    // Запускаем async-read; текст придёт позже как обычный ввод.
    jsRequestClipboardPaste();
    return std::string();
#else
    if (!_glfwWindow)
        return std::string();

    const char* s = glfwGetClipboardString((GLFWwindow*)_glfwWindow);
    return s ? s : "";
#endif
}

#ifndef TARGET_EMSCRIPTEN

// Desktop: GLFW. Текст приходит codepoint'ами из char-колбэка -
// кодируем в UTF-8 сами. Служебные клавиши - из key-колбэка.
#include <GLFW/glfw3.h>

namespace
{

void encodeUtf8Local(unsigned int cp, char* out, int& len)
{
    if (cp <= 0x7F)
    {
        out[0] = (char)cp;
        len = 1;
    }
    else if (cp <= 0x7FF)
    {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        len = 2;
    }
    else if (cp <= 0xFFFF)
    {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        len = 3;
    }
    else
    {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        len = 4;
    }
}

void glfwCharCallback(GLFWwindow*, unsigned int codepoint)
{
    char buf[5] = { 0 };
    int len = 0;

    encodeUtf8Local(codepoint, buf, len);

    InputController::getInstance()->onKeyboardInput(buf, eSpecialKey::SK_NONE, 0);
}

void glfwKeyCallback(GLFWwindow*, int key, int, int action, int mods)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    int modifiers = KEYMOD_NONE;

    if (mods & GLFW_MOD_SHIFT)
        modifiers |= KEYMOD_SHIFT;

    if (mods & GLFW_MOD_CONTROL)
        modifiers |= KEYMOD_CTRL;

    if (mods & GLFW_MOD_ALT)
        modifiers |= KEYMOD_ALT;

    if (mods & GLFW_MOD_SUPER)
        modifiers |= KEYMOD_META;

    eSpecialKey sk = eSpecialKey::SK_NONE;

    const bool ctrlOrCmd = (mods & GLFW_MOD_CONTROL) || (mods & GLFW_MOD_SUPER);

    if (ctrlOrCmd)
    {
        switch (key)
        {
        case GLFW_KEY_C: sk = eSpecialKey::SK_COPY; break;
        case GLFW_KEY_V: sk = eSpecialKey::SK_PASTE; break;
        case GLFW_KEY_X: sk = eSpecialKey::SK_CUT; break;
        case GLFW_KEY_A: sk = eSpecialKey::SK_SELECT_ALL; break;
        default: break;
        }

        if (sk != eSpecialKey::SK_NONE)
        {
            InputController::getInstance()->onKeyboardInput(nullptr, sk, modifiers);
            return;
        }
    }

    switch (key)
    {
    case GLFW_KEY_BACKSPACE:    sk = eSpecialKey::SK_BACKSPACE; break;
    case GLFW_KEY_DELETE:       sk = eSpecialKey::SK_DELETE; break;
    case GLFW_KEY_LEFT:         sk = eSpecialKey::SK_LEFT; break;
    case GLFW_KEY_RIGHT:        sk = eSpecialKey::SK_RIGHT; break;
    case GLFW_KEY_HOME:         sk = eSpecialKey::SK_HOME; break;
    case GLFW_KEY_END:          sk = eSpecialKey::SK_END; break;

    case GLFW_KEY_ENTER:
    case GLFW_KEY_KP_ENTER:
        sk = eSpecialKey::SK_ENTER;
        break;

    case GLFW_KEY_ESCAPE:
        sk = eSpecialKey::SK_ESCAPE;
        break;

    default:
        return;
    }

    InputController::getInstance()->onKeyboardInput(nullptr, sk, modifiers);
}

}

void InputController::initKeyboard(void* pGlfwWindow)
{
    if (!pGlfwWindow)
        return;

    _glfwWindow = pGlfwWindow;

    GLFWwindow* w = (GLFWwindow*)pGlfwWindow;

    glfwSetCharCallback(w, &glfwCharCallback);
    glfwSetKeyCallback(w, &glfwKeyCallback);
}

#endif // TARGET_EMSCRIPTEN

void InputController::setExclusiveMouse(IEventListener* pListener)
{
    if (pListener)
        _exclusiveMouseListener.push(pListener);
    else
        _exclusiveMouseListener.pop();
}

bool InputController::getDragDistance(float& fDragDistance)
{
    bool bRes = _isMovedWhileDown;

    if (bRes)
    {
        fDragDistance = _ptDownAt.distance(_lastMousePos.x, _lastMousePos.y);
    }

    return bRes;
}

// === FIX START ===
bool InputController::getDragDistanceDprY(float& fDragDistance)
{
    bool bRes = _isMovedWhileDown;

    if (bRes)
    {
        float fDpr = _dpr > 0.f ? _dpr : 1.f;
        fDragDistance = (_lastMousePos.y - _ptDownAt.y) * fDpr;
    }

    return bRes;
}

bool InputController::getDragDistanceDprX(float& fDragDistance)
{
    bool bRes = _isMovedWhileDown;

    if (bRes)
    {
        float fDpr = _dpr > 0.f ? _dpr : 1.f;
        fDragDistance = (_lastMousePos.x - _ptDownAt.x) * fDpr;
    }

    return bRes;
}
// === FIX END ===

void InputController::setMouseCursor(eMouseCursorType eType)
{
    Engine::getInstance().setMouseCursor(eType);
}

void InputController::blockInput(bool bBlock)
{
    if (bBlock)
        _nBlocked++;
    else if (_nBlocked)
        _nBlocked--;

    if (_nBlocked)
    {
        _isDown           = false;
        _isMovedWhileDown = false;
    }
}

_G2D_NAMESPACE_END_