#include "sprite.h"
#include "gfx.h"

_G2D_NAMESPACE_BEGIN_

CSprite::CSprite():_uvs{},_verts{}
{   
    setClass(E_EL_SPRITE);
}

void CSprite::renderSelf(float dt, float* rgba)
{
    if (_bIsLoaded)
    {
        static float my_rgba[4];

        if (!isOffScene())
        {
            float sparkLife = 0;
            if (_autoEffect._bAuto)
            {
                if (_autoEffect._fEffectInitialTime > 0.0f) {
                    sparkLife = _autoEffect._fEffectLifeTime / _autoEffect._fEffectInitialTime;
                    _autoEffect._fEffectLifeTime -= dt;

                }
                if (_autoEffect._fEffectLifeTime <= 0.0f || CGfx::getInstance()->isPotato()) {
                    _autoEffect._fEffectType = 0; 
                    _autoEffect._fEffectInitialTime = 0; 
                    sparkLife = 0.0f;
                }
            }
            sparkLife = std::clamp(sparkLife, 0.f, 1.f);

            my_rgba[0] = rgba[0];
            my_rgba[1] = rgba[1];
            my_rgba[2] = rgba[2];
            my_rgba[3] = rgba[3];

            std::optional<float> optTutBoxInt;
            if (_bCustomTutBox)
                optTutBoxInt = _tutBoxIntencity;

            //if (_isVecSaved && _saveVec)
            //{
            //    CGfx::getInstance()->batchSprite(_pTex, _verts, _uvs, my_rgba, _bIsGs ? my_gson : my_gsoff, _eBlendMode == eSpriteBlendMode::NORMAL ? 1.f : 0.f, _addRgba, _autoEffect._bAuto ? _autoEffect._fEffectType : _effectType, _autoEffect._bAuto ? sparkLife : _effectLifeTime, optTutBoxInt, _ptrNormalMap, _bSkipLight);
            //    if (_bTrack)
            //    {
            //        CSprite::
            //    }
            //
            //}
            //else
            //{
             CGfx::getInstance()->batchSprite(_pTex, _verts, _uvs, my_rgba, _bIsGs, _eBlendMode == eSpriteBlendMode::NORMAL ? 1.f : 0.f, _addRgba, _autoEffect._bAuto ? _autoEffect._fEffectType : _effectType, _autoEffect._bAuto ? sparkLife : _effectLifeTime, optTutBoxInt, _ptrNormalMap, _bSkipLight);
            //}
        }
    }
}

bool CSprite::getNotTransBounds(Rect* p)
{
    //assert(_bIsLoaded);
    bool bRes = _bIsLoaded;
    if (_bIsLoaded)
    {
        p->set(_verts[VERT_ULX], _verts[VERT_ULY], 
        _verts[VERT_BRX] - _verts[VERT_ULX], _verts[VERT_BRY] - _verts[VERT_ULY]);
    }
    return bRes;
}

void CSprite::renderVerts(CTexturePtr pTex, float* verts, float* uvs, float* rgba, eSpriteBlendMode blend, bool bGrayScale)
{
    static float my_gsoff[4] = { 0.f, 0.f, 0.f, 0.f };
    static float addRgba[4]  = { 0.f, 0.f, 0.f, 0.f };
    CGfx::getInstance()->batchSprite(
        pTex, verts, uvs, rgba,
        bGrayScale,                                   // <<< вместо бывшего gs/nullptr
        blend == eSpriteBlendMode::NORMAL ? 1.f : 0.f,
        nullptr, 0.f, 0.f, std::nullopt, nullptr, true);
}

CSpritePtr CSprite::cloneInitial()
{
    CSpritePtr pRes = std::make_shared<CSprite>();
    pRes->_pTex = _pTex;
    memcpy(pRes->_uvs,   _uvs,   sizeof(pRes->_uvs));
    memcpy(pRes->_verts, _verts, sizeof(pRes->_verts));
    pRes->_bIsLoaded = true;    
    return pRes;
}

void CSprite::setScissored(bool bScissored, bool bClipAllIfTransbounsNotCalced)
{
     _bScissored = bScissored; 
     _bClipAllIfTransbounsNotCalced  = bClipAllIfTransbounsNotCalced;
     CGfx::getInstance()->enableScissor(bScissored); 
}

CSpritePtr CSprite::fromData(CTexturePtr pTex, float* verts, float* uvs)
{
    CSpritePtr pRes = std::make_shared<CSprite>();
    pRes->_pTex = pTex;
    memcpy(pRes->_uvs,   uvs,   sizeof(pRes->_uvs));
    memcpy(pRes->_verts, verts, sizeof(pRes->_verts));
    pRes->_bIsLoaded = true;    
    return pRes;
}

CSpritePtr CSprite::fromRect(Rect& rc)
{
    // 1. Создаем новый спрайт и передаем ему ту же текстуру (атлас)
    CSpritePtr pRes = std::make_shared<CSprite>();
    pRes->_pTex = _pTex;

    // 2. Вычисляем локальные размеры текущего спрайта
    float sprW = _verts[VERT_URX] - _verts[VERT_ULX];
    float sprH = _verts[VERT_BLY] - _verts[VERT_ULY];

    // Защита от деления на ноль (если спрайт еще не имеет размеров)
    if (sprW == 0.0f) sprW = 1.0f;
    if (sprH == 0.0f) sprH = 1.0f;

    // 3. Берем текущие UV координаты спрайта в атласе
    float u0 = _uvs[VERT_ULX];
    float u1 = _uvs[VERT_URX];
    float v0 = _uvs[VERT_ULY];
    float v1 = _uvs[VERT_BLY];

    // 4. Интерполируем UV для нового спрайта.
    // rc.x и rc.y задаются относительно левого верхнего угла ТЕКУЩЕГО спрайта.
    float new_u0 = u0 + (rc.x / sprW) * (u1 - u0);
    float new_u1 = u0 + (rc.right() / sprW) * (u1 - u0);
    float new_v0 = v0 + (rc.y / sprH) * (v1 - v0);
    float new_v1 = v0 + (rc.bottom() / sprH) * (v1 - v0);

    // Заполняем UV нового спрайта
    pRes->_uvs[VERT_ULX] = new_u0; pRes->_uvs[VERT_ULY] = new_v0;
    pRes->_uvs[VERT_URX] = new_u1; pRes->_uvs[VERT_URY] = new_v0;
    pRes->_uvs[VERT_BLX] = new_u0; pRes->_uvs[VERT_BLY] = new_v1;
    pRes->_uvs[VERT_BRX] = new_u1; pRes->_uvs[VERT_BRY] = new_v1;

    // 5. Заполняем геометрию (вершины) в точности как Rect.
    // Это сохранит смещение кусочка относительно родителя.
    pRes->_verts[VERT_ULX] = 0;      pRes->_verts[VERT_ULY] = 0;
    pRes->_verts[VERT_URX] = rc.cx;  pRes->_verts[VERT_URY] = 0;
    pRes->_verts[VERT_BLX] = 0;      pRes->_verts[VERT_BLY] = rc.cy;
    pRes->_verts[VERT_BRX] = rc.cx;  pRes->_verts[VERT_BRY] = rc.cy;

    /*
    // ПРИМЕЧАНИЕ: Если по логике вашей игры "вырезанный кусок" должен иметь 
    // собственную точку отсчета 0,0 (то есть быть просто прямоугольником rc.cx x rc.cy),
    // то замените блок выше на этот:
    
    pRes->_verts[VERT_ULX] = 0.0f;      pRes->_verts[VERT_ULY] = 0.0f;
    pRes->_verts[VERT_URX] = rc.cx;     pRes->_verts[VERT_URY] = 0.0f;
    pRes->_verts[VERT_BLX] = 0.0f;      pRes->_verts[VERT_BLY] = rc.cy;
    pRes->_verts[VERT_BRX] = rc.cx;     pRes->_verts[VERT_BRY] = rc.cy;
    */

    // 6. Помечаем как загруженный
    pRes->_bIsLoaded = true;

    return pRes;
}
_G2D_NAMESPACE_END_