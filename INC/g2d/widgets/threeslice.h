#pragma once

#include "g2d.h"
#include "widgets/widget.h"
#include "sprite.h"

_G2D_NAMESPACE_BEGIN_

class ThreeSliceHor : public Widget
{
private:
        float           _fcx = 0;
        float           _fcy = 0;
        
public:
        void    createSlices        (CSpritePtr     pSpr,
                                     float          fA,
                                     float          fB);
        void    build               (float          cx);
        inline float getCx (void) { return _fcx; }
        inline float getCy (void) { return _fcy; }
public:
    virtual bool                getNotTransBounds       (Rect*              p) override;
};  

typedef std::shared_ptr<ThreeSliceHor> ThreeSliceHorPtr;

class ThreeSliceVert : public CContainer
{
private:
        float           _fcx = 0;
        float           _fcy = 0;
        
public:
        void    createSlices        (CSpritePtr     pSpr,
                                     float          fC,
                                     float          fD);
        void    build               (float          cy);
        inline float getCx (void) { return _fcx; }
        inline float getCy (void) { return _fcy; }
public:
    virtual bool                getNotTransBounds       (Rect*              p) override;
};  

typedef std::shared_ptr<ThreeSliceVert> ThreeSliceVertPtr;

_G2D_NAMESPACE_END_