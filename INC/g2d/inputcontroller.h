#pragma once
#include "g2d.h"
#include "eventemmiter.h"
#include "eventdef.h"
#include <string>

_G2D_NAMESPACE_BEGIN_

enum class eSpecialKey : int;

enum eKeyModifier
{
    KEYMOD_NONE  = 0,
    KEYMOD_SHIFT = 1,
    KEYMOD_CTRL  = 2,
    KEYMOD_ALT   = 4,
    KEYMOD_META  = 8
};

typedef std::stack<IEventListener*> StackListeners;

enum class eCoordType
{
    INGAME_COORDS,
    BG_COOIRDS
};

class InputController : public CEventEmmiter
{
    friend class CApp;
    friend class AwaitController;
    friend class CampaignStartView;

private:
    static  InputController*    _instance;

    bool                _isInitDone             = false;
    bool                _isDown                 = false;
    StackListeners      _exclusiveMouseListener;
    Point               _ptDownAt;
    float               _dpr;
    bool                _isMovedWhileDown       = false;
    Point               _lastMousePos;
    Point               _curMousePos;
    int                 _nBlocked               = 0;
    bool                _bFocused               = true;
    void*               _glfwWindow             = nullptr;

public:
    enum Events
    {
        EVT_ON_MOUSE_DOWN = EventsNs::INPUTCONTROLLER_FIRST_EVT,
        EVT_ON_MOUSE_UP,
        EVT_ON_MOUSE_MOVE,
        EVT_ON_MOUSE_WHEEL,
        EVT_ON_BLUR,
        EVT_ON_FOCUS,
        EVT_ON_KEYBOARD,
        EVT_ON_VIRTUAL_KEYBOARD,   // p1: const char* полный UTF-8 текст (nullptr = blur), p2: intptr_t каретка/-1
    };

public:
    static InputController*     getInstance         (void);

    void                        init                (void);

    void                        translateCoords     (int32_t&           x,
                                                     int32_t&           y,
                                                     eCoordType         eTranslate = eCoordType::INGAME_COORDS);

    bool                        isMouseDown         (void) { return _isDown;    }
    float                       getLastDpr          (void) { return _dpr; }
    Point                       getMousePos         (void) { return _curMousePos; }

    void                        onBlur              (void);
    void                        onFocus             (void);

    void                        onPointerDown       (int32_t            x,
                                                     int32_t            y,
                                                     float              dpr);

    void                        onPointerUp         (int32_t            x,
                                                     int32_t            y,
                                                     float              dpr);

    void                        onPointerMove       (int32_t            x,
                                                     int32_t            y,
                                                     float              dpr);

    void                        onMouseWheel        (int32_t            deltaY);

    void                        onKeyboardInput     (const char*        szTextUtf8,
                                                     eSpecialKey        eKey,
                                                     int                modifiers = 0);
    void                        initKeyboard        (void*              pGlfwWindow);

    void                        showVirtualKeyboard (const char*        szInitialUtf8,
                                                     bool               bPassword);

    void                        hideVirtualKeyboard (void);

    void                        emitVirtualKeyboard (const char*        szTextUtf8,
                                                     int                caret);

    void                        setClipboardText    (const char*        text);
    std::string                 getClipboardText    (void);

    void                        setExclusiveMouse   (IEventListener*    pListener = NULL);

    bool                        getDragDistance     (float&             fDragDistance);
    bool                        getDragDistanceDprY (float&             fDragDistance);
    bool                        getDragDistanceDprX (float&             fDragDistance);

    void                        setMouseCursor      (eMouseCursorType   eType);

    bool                        isInputBlocked      (void) { return _nBlocked > 0;}
    auto                        isFocused           (void) { return _bFocused; }

protected:
    void                        blockInput          (bool               bBlock = true);
};

_G2D_NAMESPACE_END_