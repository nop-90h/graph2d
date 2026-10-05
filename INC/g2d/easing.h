#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

struct Easing
{
    static float inSine         (float  t);
    static float outSine        (float  t);
    static float inOutSine      (float  t);
    static float inQuad         (float  t);
    static float outQuad        (float  t);
    static float inOutQuad      (float  t);
    static float inCubic        (float  t);
    static float outCubic       (float  t);
    static float inOutCubic     (float  t);
    static float inQuart        (float  t);
    static float outQuart       (float  t);
    static float inOutQuart     (float  t);
    static float inQuint        (float  t);
    static float outQuint       (float  t);
    static float inOutQuint     (float  t);
    static float inExpo         (float  t);
    static float outExpo        (float  t);
    static float inOutExpo      (float  t);
    static float inCirc         (float  t);
    static float outCirc        (float  t);
    static float inOutCirc      (float  t);
    static float inBack         (float  t);
    static float outBack        (float  t);
    static float outBackSoft    (float  t);
    static float inBackSoft     (float  t);
    static float inOutBack      (float  t);
    static float inElastic      (float  t);
    static float outElastic     (float  t);
    static float inOutElastic   (float  t);
    static float inBounce       (float  t);
    static float outBounce      (float  t);
    static float inOutBounce    (float  t);
    static float linear         (float  f);
};
typedef float(*easingFunction)(float);

_G2D_NAMESPACE_END_