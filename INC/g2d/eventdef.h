#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

enum class eMouseCursorType:int
{
    E_MCT_NORMAL,
    E_MCT_POINTER
};

namespace EventsNs
{
    inline constexpr int LOADERQUEUE_FIRST_EVT        = 0;
    inline constexpr int CHARBASE_FIRST_EVT           = 100;
    inline constexpr int MODELCHARBASE_FIRST_EVT      = 200;
    inline constexpr int SCREENRESIZE_FIRST_EVT       = 300;
    inline constexpr int INPUTCONTROLLER_FIRST_EVT    = 400;
    inline constexpr int MODELFIGHT_FIRST_EVT         = 500;
    inline constexpr int CLIENT_FIRST_EVT             = 600;
    inline constexpr int WEBSOCKET_EVT_FIRST	      = 700;
    inline constexpr int DIALOGS_EVT_FIRST			  = 800;
    inline constexpr int INVENTORY_EVT_FIRST	      = 900;
    inline constexpr int TUTORIALCONTROLLER_EVT_FIRST = 1000;
    inline constexpr int EXTAPI_EVT_FIRST             = 1100;
    inline constexpr int AUDIO_EVT_FIRST              = 1200;
    inline constexpr int USER_EVT_FIRST               = 1300;

};

_G2D_NAMESPACE_END_