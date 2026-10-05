#pragma once
#include "g2d.h"
#include "container.h"
#include "m3.h"

_G2D_NAMESPACE_BEGIN_

class TouchHandler : public CContainer
{
private:
    Rect _rc = { 0, 0, 0, 0 };
    Rect _rcInteractive;
    bool _bCalced = false;
    float _mat[CMatrixStack::MAT_CELLS_COUNT] = { 0 };
public:

    TouchHandler(float cx, float cy) 
    {
        setInteractive(true);
        _rc.cx = cx; 
        _rc.cy = cy; 
        m3::identity(_mat);
    }

    auto getMat() { return _mat; };

    void toLocal(Point& pt)
    {
        float scrCx = CSceneResize::getInstance()->getScreenWidth();
        float scrCy = CSceneResize::getInstance()->getScreenHeight();

        float ndcX = (pt.x / scrCx) * 2.0f - 1.0f;
        float ndcY = 1.0f - (pt.y / scrCy) * 2.0f;
        float inverted[CMatrixStack::MAT_CELLS_COUNT];
        m3::invert(inverted, _mat);

        pt.x = ndcX * inverted[0] + ndcY * inverted[3] + inverted[6];
        pt.y = ndcX * inverted[1] + ndcY * inverted[4] + inverted[7];
    }
    
    virtual void applyTransform(CMatrixStack* pMS, bool bForce) override
    {
        _bCalced = true;
        CContainer::applyTransform(pMS, bForce);

        static_assert(sizeof(_mat) == CMatrixStack::getMatrixSize());
        memcpy(_mat, pMS->top(), CMatrixStack::getMatrixSize());

        static float vec3LU[3] = { 0 };
        static float vec3RB[3] = { 0 };

        vec3LU[0] = 0;
        vec3LU[1] = 0;
        vec3LU[2] = 1.f;
        vec3RB[0] = _rc.cx;
        vec3RB[1] = _rc.cy;
        vec3RB[2] = 1.f;
        pMS->vecMultiply(vec3LU);
        pMS->vecMultiply(vec3RB);

        CSceneResize::getInstance()->toPixelCoords(vec3LU[0], vec3LU[1]);
        CSceneResize::getInstance()->toPixelCoords(vec3RB[0], vec3RB[1]);

        if (vec3RB[0] < vec3LU[0])
            std::swap(vec3RB[0], vec3LU[0]);

        _rcInteractive.set(vec3LU[0], vec3LU[1], vec3RB[0] - vec3LU[0], vec3RB[1] - vec3LU[1]);
    }

    virtual bool getInteractiveBounds(Rect* p) override
    {
        if (_bCalced)
        {
            *p = _rcInteractive;
        }
        return _bCalced;
    }

    virtual void renderSelf(float dt, float* rgba) override
    {
        CSprite::_vRendered.push_back(shared_from_this());
    }
};

typedef std::shared_ptr<TouchHandler> TouchHandlerPtr;

_G2D_NAMESPACE_END_