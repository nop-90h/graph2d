#include "m3.h"

_G2D_NAMESPACE_BEGIN_

void m3::projection(float width, float height, float* mat) {
    // Note: This matrix flips the Y axis so that 0 is at the top.
    mat[0] = 2.f / width;    mat[1] = 0.f;             mat[2] = 0.f;
    mat[3] = 0.f;            mat[4] = -2.f / height;   mat[5] = 0.f;
    mat[6] = -1.f;           mat[7] = 1.f;             mat[8] = 1.f;
}

void m3::identity(float* mat)
{
    mat[0] = 1.f; mat[1] = 0.f; mat[2] = 0.f;
    mat[3] = 0.f; mat[4] = 1.f; mat[5] = 0.f;
    mat[6] = 0.f; mat[7] = 0.f; mat[8] = 1.f;
}

void m3::translation(float* mat, float tx, float ty) {
    mat[0] = 1.f; mat[1] = 0.f; mat[2] = 0.f;
    mat[3] = 0.f; mat[4] = 1.f; mat[5] = 0.f;
    mat[6] = tx;  mat[7] = ty;  mat[8] = 1.f;
}

void m3::rotation(float* mat, float angleInRadians) {
    float c = cos(angleInRadians);
    float s = sin(angleInRadians);

    mat[0] = c;   mat[1] = -s;  mat[2] = 0.f;
    mat[3] = s;   mat[4] = c;   mat[5] = 0.f;
    mat[6] = 0;   mat[7] = 0;   mat[8] = 1.f;
}

void m3::scaling(float* mat, float sx, float sy) {
    mat[0] = sx;  mat[1] = 0;   mat[2] = 0.f;
    mat[3] = 0;   mat[4] = sy;  mat[5] = 0.f;
    mat[6] = 0;   mat[7] = 0;   mat[8] = 1.f;
}

void m3::shearing(float* mat, float shx, float shy)
{
    mat[0] = 1.f;  mat[1] = shx; mat[2] = 0.f;
    mat[3] = shy;  mat[4] = 1.f;  mat[5] = 0.f;
    mat[6] = 0.f;  mat[7] = 0.f;  mat[8] = 1.f;
}

void m3::multiply(float* outA, float* b)
{
    float f[9] = {
        b[0 * 3 + 0] * outA[0 * 3 + 0] + b[0 * 3 + 1] * outA[1 * 3 + 0] + b[0 * 3 + 2] * outA[2 * 3 + 0],
        b[0 * 3 + 0] * outA[0 * 3 + 1] + b[0 * 3 + 1] * outA[1 * 3 + 1] + b[0 * 3 + 2] * outA[2 * 3 + 1],
        b[0 * 3 + 0] * outA[0 * 3 + 2] + b[0 * 3 + 1] * outA[1 * 3 + 2] + b[0 * 3 + 2] * outA[2 * 3 + 2],

        b[1 * 3 + 0] * outA[0 * 3 + 0] + b[1 * 3 + 1] * outA[1 * 3 + 0] + b[1 * 3 + 2] * outA[2 * 3 + 0],
        b[1 * 3 + 0] * outA[0 * 3 + 1] + b[1 * 3 + 1] * outA[1 * 3 + 1] + b[1 * 3 + 2] * outA[2 * 3 + 1],
        b[1 * 3 + 0] * outA[0 * 3 + 2] + b[1 * 3 + 1] * outA[1 * 3 + 2] + b[1 * 3 + 2] * outA[2 * 3 + 2],

        b[2 * 3 + 0] * outA[0 * 3 + 0] + b[2 * 3 + 1] * outA[1 * 3 + 0] + b[2 * 3 + 2] * outA[2 * 3 + 0],
        b[2 * 3 + 0] * outA[0 * 3 + 1] + b[2 * 3 + 1] * outA[1 * 3 + 1] + b[2 * 3 + 2] * outA[2 * 3 + 1],
        b[2 * 3 + 0] * outA[0 * 3 + 2] + b[2 * 3 + 1] * outA[1 * 3 + 2] + b[2 * 3 + 2] * outA[2 * 3 + 2]
    };

    memcpy(outA, f, sizeof(f));
}

void m3::multiplyVec(float* mat, float* pInOutVec)
{
    float f3[] =
    {
        mat[0] * pInOutVec[0] + mat[3] * pInOutVec[1] + mat[6] * pInOutVec[2],
        mat[3] * pInOutVec[0] + mat[4] * pInOutVec[1] + mat[7] * pInOutVec[2],
        mat[2] * pInOutVec[0] + mat[5] * pInOutVec[1] + mat[8] * pInOutVec[2]
    };

    memcpy(pInOutVec, f3, sizeof(f3));
}

bool m3::invert(float* mOut, float* mIn)
{
    float a00 = mIn[0], a01 = mIn[1], a02 = mIn[2];
    float a10 = mIn[3], a11 = mIn[4], a12 = mIn[5];
    float a20 = mIn[6], a21 = mIn[7], a22 = mIn[8];

    float b01 = a22 * a11 - a12 * a21;
    float b11 = -a22 * a10 + a12 * a20;
    float b21 = a21 * a10 - a11 * a20;

    float det = a00 * b01 + a01 * b11 + a02 * b21;

    if (std::abs(det) < 1e-7f)
        return false;

    float invDet = 1.0f / det;

    float res[9];

    res[0] = b01 * invDet;
    res[1] = (-a22 * a01 + a02 * a21) * invDet;
    res[2] = (a12 * a01 - a02 * a11) * invDet;
    res[3] = b11 * invDet;
    res[4] = (a22 * a00 - a02 * a20) * invDet;
    res[5] = (-a12 * a00 + a02 * a10) * invDet;
    res[6] = b21 * invDet;
    res[7] = (-a21 * a00 + a01 * a20) * invDet;
    res[8] = (a11 * a00 - a01 * a10) * invDet;

    for (int i = 0; i < 9; ++i)
        mOut[i] = res[i];

    return true;
}

void m3::transformPoint(float* m, float x, float y, float& outX, float& outY) {
    // m[6] — tx, m[7] — ty
    outX = m[0] * x + m[1] * y + m[6];
    outY = m[3] * x + m[4] * y + m[7];
}

_G2D_NAMESPACE_END_