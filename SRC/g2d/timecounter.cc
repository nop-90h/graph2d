#include "timecounter.h"

_G2D_NAMESPACE_BEGIN_

float TimeCounter::_time = 0.f;

void TimeCounter::update(float dt)
{
    TimeCounter::_time += dt;
}

float TimeCounter::getTime(void)
{
    return TimeCounter::_time;
}

_G2D_NAMESPACE_END_