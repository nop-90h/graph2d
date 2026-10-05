#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

struct m3
{
    static void projection      (float      width, 
                                 float      height, 
                                 float*     mat);
    static void identity        (float*     mat);
    static void translation     (float*     mat, 
                                 float      tx, 
                                 float      ty);
    static void rotation        (float*     mat, 
                                 float      angleInRadians);
    static void scaling         (float*     mat, 
                                 float      sx, 
                                 float      sy);
    static void shearing        (float*     mat, 
                                 float      shx, 
                                 float      shy);

    static void multiply        (float*     outA, 
                                 float*     b);
    static void multiplyVec     (float*     mat, 
                                 float*     pInOutVec);
    static bool invert          (float*     matOut, 
                                 float*     matIn);
    static void transformPoint  (float*     m, 
                                 float      x, 
                                 float      y, 
                                 float&     outX, 
                                 float&     outY);
};

_G2D_NAMESPACE_END_