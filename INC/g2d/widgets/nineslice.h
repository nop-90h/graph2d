#pragma once

#include "g2d.h"
#include "widgets/widget.h"
#include "sprite.h"

_G2D_NAMESPACE_BEGIN_

class NineSlice;
typedef std::shared_ptr<NineSlice> NineSlicePtr;

class NineSlice : public Widget
{
private:
        float           _fcx = 0;
        float           _fcy = 0;
        Rect            _rcTransformed;
        bool            _bTransformCalced = false;
public:
                        NineSlice           (void);
        void            createSlices        (CSpritePtr     pSpr,
                                             float          fA,
                                             float          fB,
                                             float          fC,
                                             float          fD);
        void            build               (float          cx,
                                             float          cy);
        NineSlicePtr    cloneInitial        (void);
virtual bool            getNotTransBounds   (Rect*          p) override;
//virtual bool            getTransBounds      (Rect*          p) override;

virtual void            setGrayScale        (bool           gs = true) override;

protected:
//virtual void            applyTransform      (CMatrixStack*  pMS,
//                                             bool           bForce = false) override;

};  

_G2D_NAMESPACE_END_