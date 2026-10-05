#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

struct MathUtils
{
    static void straightLine(float sx, float sy, float tx, float ty, float t, Point* pOut);
    static Point getBSplinePoint(float t, const Point& P0, const Point& P1, const Point& P2);
};

_G2D_NAMESPACE_END_