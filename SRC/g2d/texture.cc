#include "texture.h"
#include "timecounter.h"

_G2D_NAMESPACE_BEGIN_

CTexture::CTexture():_sampler(-1),_handle(0),_cx(0),_cy(0)
{
    updateLastUse();
}

void CTexture::updateLastUse()
{
    _fLastUse = TimeCounter::getTime();
}

CTexture::~CTexture()
{
    //assert(false);
}

_G2D_NAMESPACE_END_