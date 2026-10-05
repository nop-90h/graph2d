#include "matrixstack.h"
#include "m3.h"

_G2D_NAMESPACE_BEGIN_

CMatrixStack::CMatrixStack()
{
    _nStackTop  = MAX_ST_SIZE;
    _pStackBuff = (uint8_t*)malloc(MAX_ST_SIZE);
    restore();
}

CMatrixStack::~CMatrixStack()
{
    free(_pStackBuff);
}

void CMatrixStack::clear()
{
    _nStackTop  = MAX_ST_SIZE;
    restore();
}

void CMatrixStack::restore()
{
    pop();

    // Never let the stack be totally empty
    if (_nStackTop == MAX_ST_SIZE)
    {
        _nStackTop -= MAT33_SIZE;
        m3::identity(top());
    }
}

void CMatrixStack::set(float* pMatrix33)
{
    memcpy(top(), (uint8_t*)pMatrix33, MAT33_SIZE);
}

void CMatrixStack::push(float* pMatrix33)
{
    assert(_nStackTop - MAT33_SIZE > 0);

    if (_nStackTop - MAT33_SIZE > 0)
    {
        _nStackTop -= MAT33_SIZE;
        memcpy(_pStackBuff + _nStackTop, (uint8_t*)pMatrix33, MAT33_SIZE);
    }
}

float* CMatrixStack::top()
{
    float* fRes = 0;

    assert(_nStackTop < MAX_ST_SIZE);

    if (_nStackTop < MAX_ST_SIZE)
    {
        fRes = (float*)(_pStackBuff + _nStackTop);
    }

    return fRes;
}

void CMatrixStack::pop()
{
    _nStackTop += MAT33_SIZE;

    if (_nStackTop > MAX_ST_SIZE)
        _nStackTop = MAX_ST_SIZE;
}

void CMatrixStack::save()
{
    assert(_nStackTop < MAX_ST_SIZE);
    push(top());
}

void CMatrixStack::identity()
{
    assert(_nStackTop < MAX_ST_SIZE);
    m3::identity(top());
}

void CMatrixStack::translate(float x, float y)
{
    static float mTranslate[MAT_CELLS_COUNT] = { 0 };

    assert(_nStackTop < MAX_ST_SIZE);

    m3::translation(mTranslate, x, y);
    m3::multiply(top(), mTranslate);
}

void CMatrixStack::rotate(float angleInRadians)
{
    static float mRotate[MAT_CELLS_COUNT] = { 0 };

    assert(_nStackTop < MAX_ST_SIZE);

    m3::rotation(mRotate, angleInRadians);
    m3::multiply(top(), mRotate);
}

void CMatrixStack::scale(float x, float y)
{
    static float mScale[MAT_CELLS_COUNT] = { 0 };

    assert(_nStackTop < MAX_ST_SIZE);

    m3::scaling(mScale, x, y);
    m3::multiply(top(), mScale);
}

void CMatrixStack::shearXAt(float shx, float pivotY)
{
    assert(_nStackTop < MAX_ST_SIZE);

    float* m = top();
    if (!m)
        return;

    const float m0 = m[0];
    const float m1 = m[1];

    m[3] += shx * m0;              
    m[4] += shx * m1;
    m[6] -= shx * pivotY * m0;     
    m[7] -= shx * pivotY * m1;
}

void CMatrixStack::projection(float cx, float cy)
{
    m3::projection(cx, cy, top());
}

void CMatrixStack::vecMultiply(float* pVec)
{
    m3::multiplyVec(top(), pVec);
}

_G2D_NAMESPACE_END_