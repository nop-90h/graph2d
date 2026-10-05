#pragma once

#include "g2d.h"
#include "texture.h"
#include "container.h"

_G2D_NAMESPACE_BEGIN_

class CSprite;

//typedef std::shared_ptr<CSprite>                      CSpritePtr;
typedef std::shared_ptr<const CSprite>                CSpritePtrConst;
typedef std::vector<CSpritePtr>                       CSpritePtrs;
typedef CSpritePtrs::iterator                         CSpritePtrsIt;
typedef std::unordered_map<std::string, CSpritePtr>   MapString2CSprite;

enum class eSpriteBlendMode
{
    NORMAL,
    ADDITIVE
};

class CSprite : public CContainer
{
    friend class CGfx;
    friend class CFont;
    friend class SpriteLoader;
    friend class NineSlice;
    friend class ThreeSliceHor;
    friend class ThreeSliceVert;
    friend class RenderTracker;
    friend class CSpine;
    friend class TextRender;
    friend class StaticLabel;
    friend class BloodEmitter;
    //friend class BloodFountain;
public:
    enum Slice
    {
        A = 0,
        B = 1,
        C = 2,
        D = 3
    };
    enum Vert
    {
        VERT_BLX = 0,
        VERT_BLY = 1,
        VERT_ULX = 2,
        VERT_ULY = 3,
        VERT_URX = 4,
        VERT_URY = 5,
        VERT_BRX = 6,
        VERT_BRY = 7,
        VERT_COUNT
    };

public:
            float            _uvs[VERT_COUNT];
            float            _verts[VERT_COUNT];
            bool             _bIsLoaded    = false;

private:
            eSpriteBlendMode _eBlendMode   = eSpriteBlendMode::NORMAL;

            CTexturePtr      _pTex;
            std::string      _sDebug;
            bool             _bScissored = false;
            bool             _bClipAllIfTransbounsNotCalced = true;
            float            _addRgba[4] = {0,0,0,0};
            float            _effectType = 0;
            float            _effectLifeTime = 0;
            float            _tutBoxIntencity = 0.f;
            bool             _bCustomTutBox = false;
            CTexturePtr      _ptrNormalMap;
            struct
            {
                float _fEffectType = 0;
                float _fEffectLifeTime = 0;
                float _fEffectInitialTime = 0;
                bool  _bAuto = false;
            } _autoEffect;
public:
                            CSprite             (void);
    //virtual void            render              (float          dt,
    //                                             float*         rgba) override;
    virtual void            renderSelf          (float          dt,
                                                 float*         rgba) override;
    virtual bool            getNotTransBounds   (Rect*          p) override;
// sprite.h
    static void renderVerts(CTexturePtr pTex, float* verts, float* uvs, float* rgba, eSpriteBlendMode blend, bool bGrayScale = false);
    //virtual bool            getTransBounds      (Rect*          p) override;

            CSpritePtr      cloneInitial        (void);
    inline  CSpritePtr      getPtr              (void) { return std::dynamic_pointer_cast<CSprite>(shared_from_this()); }
            void            setBlendMode        (eSpriteBlendMode e) { _eBlendMode = e; }
            void            setScissored        (bool           bScissored,
                                                 bool           bClipAllIfTransbounsNotCalced = true);
            void            setEffectType       (float          fType) { _effectType = fType; }
            void            setEffectLifeTime   (float          fLifeTime) { _effectLifeTime = fLifeTime; }
            void            setCustomTutBox     (float          fIntencity = 0,
                                                 bool           bCustom = false)  { _tutBoxIntencity = fIntencity; _bCustomTutBox = bCustom; }
            void            setAddRgba          (float r, float g, float b, float a) { _addRgba[0] = r;_addRgba[1] = g;_addRgba[2] = b;_addRgba[3] = a; }
            void            setAutoEffect       (float fEffectType, float fLifeTime) { _autoEffect._bAuto = true; _autoEffect._fEffectType = fEffectType; _autoEffect._fEffectInitialTime = fLifeTime; _autoEffect._fEffectLifeTime = fLifeTime;}
            void            setNormalMap        (CTexturePtr ptrNormalMap) { _ptrNormalMap = ptrNormalMap; }
            const auto      getTexture          (void) const { return  _pTex; }
            CSpritePtr      fromRect            (Rect& rc);
    template <typename Derived>
    auto cloneInitialTmpl()
    {
        std::shared_ptr<Derived> pRes = std::make_shared<Derived>();
        pRes->_pTex = _pTex;
        memcpy(pRes->_uvs,   _uvs,   sizeof(pRes->_uvs));
        memcpy(pRes->_verts, _verts, sizeof(pRes->_verts));
        pRes->_bIsLoaded = true;    
        return pRes;
    }

public:
    void setTexture(CTexturePtr ptrTex) { _pTex =  ptrTex; }
    static  CSpritePtr      fromData            (CTexturePtr    pTex,
                                                 float*         verts,
                                                 float*         uvs);
};

_G2D_NAMESPACE_END_