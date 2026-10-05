#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

class TimeCounter
{
public:
    static float _time;
    static void update(float dt);
    static float getTime(void);
};

_G2D_NAMESPACE_END_