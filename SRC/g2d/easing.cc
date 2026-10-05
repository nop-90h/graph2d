#include "easing.h"

_G2D_NAMESPACE_BEGIN_

#ifndef PI
#define PI MY_PI
#endif

float Easing::inSine( float t ) {
	return sin( 1.5707963f * t );
}

float Easing::outSine( float t ) {
	return 1 + sin( 1.5707963f * (--t) );
}

float Easing::inOutSine( float t ) {
	return 0.5f * (1.f + sin( 3.1415926f * (t - 0.5f) ) );
}

float Easing::inQuad( float t ) {
    return t * t;
}

float Easing::outQuad( float t ) { 
    return t * (2.f - t);
}

float Easing::inOutQuad( float t ) {
    return t < 0.5f ? 2.f * t * t : t * (4.f - 2.f * t) - 1.f;
}

float Easing::inCubic( float t ) {
    return t * t * t;
}

float Easing::outCubic( float t ) {
    return 1.f + (--t) * t * t;
}

float Easing::inOutCubic( float t ) {
    return t < 0.5f ? 4.f * t * t * t : 1.f + (--t) * (2.f * (--t)) * (2.f * t);
}

float Easing::inQuart( float t ) {
    t *= t;
    return t * t;
}

float Easing::outQuart( float t ) {
    t = (--t) * t;
    return 1.f - t * t;
}

float Easing::inOutQuart( float t ) {
    if( t < 0.5f ) {
        t *= t;
        return 8.f * t * t;
    } else {
        t = (--t) * t;
        return 1.f - 8.f * t * t;
    }
}

float Easing::inQuint( float t ) {
    float t2 = t * t;
    return t * t2 * t2;
}

float Easing::outQuint( float t ) {
    float t2 = (--t) * t;
    return 1.f + t * t2 * t2;
}

float Easing::inOutQuint( float t ) {
    float t2;
    if( t < 0.5f ) {
        t2 = t * t;
        return 16.f * t * t2 * t2;
    } else {
        t2 = (--t) * t;
        return 1.f + 16.f * t * t2 * t2;
    }
}

float Easing::inExpo( float t ) {
    return (pow( 2.f, 8.f * t ) - 1) / 255.f;
}

float Easing::outExpo( float t ) {
    return 1.f - pow( 2.f, -8.f * t );
}

float Easing::inOutExpo( float t ) {
    if( t < 0.5f ) {
        return (pow( 2.f, 16.f * t ) - 1.f) / 510.f;
    } else {
        return 1.f - 0.5f * pow( 2.f, -16.f * (t - 0.5f) );
    }
}

float Easing::inCirc( float t ) {
    return 1.f - sqrt( 1.f - t );
}

float Easing::outCirc( float t ) {
    return sqrt( t );
}

float Easing::inOutCirc( float t ) {
    if( t < 0.5f ) {
        return (1.f - sqrt( 1.f - 2.f * t )) * 0.5f;
    } else {
        return (1.f + sqrt( 2.f * t - 1.f )) * 0.5f;
    }
}

float Easing::inBack( float t ) {
    return t * t * (2.70158f * t - 1.70158f);
}

float Easing::outBack( float t ) {
    return 1.f + (--t) * t * (2.70158f * t + 1.70158f);
}

float Easing::outBackSoft(float t)
{
    //t = clamp01(t);

    const float c1 = 1.2f;
    const float c3 = c1 + 1.f;

    float x = t - 1.f;
    return 1.f + c3 * x * x * x + c1 * x * x;
}

float Easing::inBackSoft(float t)
{
    const float c1 = 1.2f; // ”меньшенный коэффициент дл€ более м€гкого старта с отт€гиванием назад
    const float c3 = c1 + 1.f;

    return c3 * t * t * t - c1 * t * t;
}

float Easing::inOutBack( float t ) {
    if( t < 0.5f ) {
        return t * t * (7.f * t - 2.5f) * 2.f;
    } else {
        return 1.f + (--t) * t * 2.f * (7.f * t + 2.5f);
    }
}

float Easing::inElastic( float t ) {
    float t2 = t * t;
    return t2 * t2 * sin( t * PI * 4.5f );
}

float Easing::outElastic( float t ) {
    float t2 = (t - 1.f) * (t - 1.f);
    return 1.f - t2 * t2 * cos( t * PI * 4.5f );
}

float Easing::inOutElastic( float t ) {
    float t2;
    if( t < 0.45f ) {
        t2 = t * t;
        return 8.f * t2 * t2 * sin( t * PI * 9.f );
    } else if( t < 0.55f ) {
        return 0.5f + 0.75f * sin( t * PI * 4.f );
    } else {
        t2 = (t - 1.f) * (t - 1.f);
        return 1.f - 8.f * t2 * t2 * sin( t * PI * 9.f );
    }
}

float Easing::inBounce( float t ) {
    return pow( 2.f, 6.f * (t - 1.f) ) * fabs( sin( t * PI * 3.5f ) );
}

float Easing::outBounce( float t ) {
    return 1.f - pow( 2.f, -6.f * t ) * fabs( cos( t * PI * 3.5f ) );
}

float Easing::inOutBounce( float t ) {
    if( t < 0.5f ) {
        return 8.f * pow( 2.f, 8.f * (t - 1.f) ) * fabs( sin( t * PI * 7.f ) );
    } else {
        return 1.f - 8.f * pow( 2.f, -8.f * t ) * fabs( sin( t * PI * 7.f ) );
    }
}

float Easing::linear(float f)
{
    return f;
}

_G2D_NAMESPACE_END_