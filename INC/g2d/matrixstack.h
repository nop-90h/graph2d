#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

class CMatrixStack
{
private:
    uint8_t*        _pStackBuff;
    int             _nStackTop;

public:
    inline static constexpr int MAT_CELLS_COUNT = 9;
    inline static constexpr int MAX_ST_SIZE     = (1024 * MAT_CELLS_COUNT * sizeof(float));
    inline static constexpr int MAT33_SIZE      = (MAT_CELLS_COUNT * sizeof(float));

            CMatrixStack    (void);
            ~CMatrixStack   (void);

    void    set             (float*     pMatrix33);
    void    push            (float*     pMatrix33);
    void    pop             (void);
    void    clear           (void);
    float*  top             (void);

    void    restore         (void);
    void    save            (void);
    void    identity        (void);

    void    translate       (float      x, 
                             float      y);
    void    rotate          (float      angleInRadians);
    void    scale           (float      x, 
                             float      y);
    void    shearXAt        (float      shx, 
                             float      pivotY);
    void    projection      (float      cx, 
                             float      cy);
    void    vecMultiply     (float*     pVec);

    inline static constexpr int getMatrixSize() {
        return MAT33_SIZE;
    }
};

_G2D_NAMESPACE_END_