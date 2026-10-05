#pragma once

#include "g2d.h"
#include "sprite.h"

_G2D_NAMESPACE_BEGIN_

class SpriteLoader
{
private:
    static SpriteLoader* _instance;

public:
    static SpriteLoader* getInstance(void);

private:
    SetStrings        _atlasesLoaded;
    MapString2CSprite _map;

public:
    bool addAtlas(const char* lpccAtlas);

    CSpritePtr      getSprite(const char* sprName);
    CSpritePtrConst getProtoSprite(const char* sprName);
};

_G2D_NAMESPACE_END_
