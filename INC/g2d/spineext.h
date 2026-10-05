#pragma once

#include "g2d.h"
#include <Extension.h>

_G2D_NAMESPACE_BEGIN_

class SpineExt : public spine::DefaultSpineExtension
{
protected:
    virtual char *_readFile(const spine::String &path, int *length) override; 
};

_G2D_NAMESPACE_END_