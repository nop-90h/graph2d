#include "widgets/threeslice.h"

_G2D_NAMESPACE_BEGIN_

void ThreeSliceHor::createSlices(CSpritePtr pSpr, float fA, float fB)
{
    removeAll();
    
    float slice[3][8];
    float sliceUV[3][8];

    float fC   = pSpr->getNotTransCy();

    float fAuv = fA / pSpr->_pTex->getCx();
    float fBuv = fB / pSpr->_pTex->getCx();
    float fCuv = fC / pSpr->_pTex->getCy();
    /*
    A                          B
    +---+----------------------+---+
  C | 1 |          2           | 3 |
    +---+----------------------+---+
    */  
    //1        
    slice[0][CSprite::VERT_ULX] = pSpr->_verts[CSprite::VERT_ULX];
    slice[0][CSprite::VERT_ULY] = pSpr->_verts[CSprite::VERT_ULY];

    slice[0][CSprite::VERT_URX] = pSpr->_verts[CSprite::VERT_ULX] + fA;
    slice[0][CSprite::VERT_URY] = pSpr->_verts[CSprite::VERT_ULY];

    slice[0][CSprite::VERT_BLX] = pSpr->_verts[CSprite::VERT_ULX];
    slice[0][CSprite::VERT_BLY] = pSpr->_verts[CSprite::VERT_ULY] + fC;

    slice[0][CSprite::VERT_BRX] = pSpr->_verts[CSprite::VERT_ULX] + fA;
    slice[0][CSprite::VERT_BRY] = pSpr->_verts[CSprite::VERT_ULY] + fC;

    sliceUV[0][CSprite::VERT_ULX] = pSpr->_uvs[CSprite::VERT_ULX];
    sliceUV[0][CSprite::VERT_ULY] = pSpr->_uvs[CSprite::VERT_ULY];
    sliceUV[0][CSprite::VERT_URX] = pSpr->_uvs[CSprite::VERT_ULX] + fAuv;
    sliceUV[0][CSprite::VERT_URY] = pSpr->_uvs[CSprite::VERT_ULY];
    sliceUV[0][CSprite::VERT_BLX] = pSpr->_uvs[CSprite::VERT_ULX];
    sliceUV[0][CSprite::VERT_BLY] = pSpr->_uvs[CSprite::VERT_ULY] + fCuv;
    sliceUV[0][CSprite::VERT_BRX] = pSpr->_uvs[CSprite::VERT_ULX] + fAuv;
    sliceUV[0][CSprite::VERT_BRY] = pSpr->_uvs[CSprite::VERT_ULY] + fCuv;

    /*
    A                          B
    +---+----------------------+---+
  C | 1 |          2           | 3 |
    +---+----------------------+---+  
    */  
    //2        
    slice[1][CSprite::VERT_ULX] = 0;
    slice[1][CSprite::VERT_ULY] = 0;

    slice[1][CSprite::VERT_URX] = pSpr->getNotTransCx() - fB - fA;
    slice[1][CSprite::VERT_URY] = 0;

    slice[1][CSprite::VERT_BLX] = 0;
    slice[1][CSprite::VERT_BLY] = fC;

    slice[1][CSprite::VERT_BRX] = pSpr->getNotTransCx() - fB - fA;
    slice[1][CSprite::VERT_BRY] = fC;


    sliceUV[1][CSprite::VERT_ULX] = pSpr->_uvs[CSprite::VERT_ULX] + fAuv;
    sliceUV[1][CSprite::VERT_ULY] = pSpr->_uvs[CSprite::VERT_ULY];

    sliceUV[1][CSprite::VERT_URX] = pSpr->_uvs[CSprite::VERT_URX] - fBuv;
    sliceUV[1][CSprite::VERT_URY] = pSpr->_uvs[CSprite::VERT_ULY];

    sliceUV[1][CSprite::VERT_BLX] = pSpr->_uvs[CSprite::VERT_ULX] + fAuv;
    sliceUV[1][CSprite::VERT_BLY] = pSpr->_uvs[CSprite::VERT_ULY] + fCuv;

    sliceUV[1][CSprite::VERT_BRX] = pSpr->_uvs[CSprite::VERT_URX] - fBuv;
    sliceUV[1][CSprite::VERT_BRY] = pSpr->_uvs[CSprite::VERT_ULY] + fCuv;

    /*
    A                          B
    +---+----------------------+---+
  C | 1 |          2           | 3 |
    +---+----------------------+---+ 
    */    
    //3
    slice[2][CSprite::VERT_ULX] = 0;
    slice[2][CSprite::VERT_ULY] = 0;

    slice[2][CSprite::VERT_URX] = fB;
    slice[2][CSprite::VERT_URY] = 0;

    slice[2][CSprite::VERT_BLX] = 0;
    slice[2][CSprite::VERT_BLY] = fC;

    slice[2][CSprite::VERT_BRX] = fB;
    slice[2][CSprite::VERT_BRY] = fC;


    sliceUV[2][CSprite::VERT_ULX] = pSpr->_uvs[CSprite::VERT_URX] - fBuv;
    sliceUV[2][CSprite::VERT_ULY] = pSpr->_uvs[CSprite::VERT_URY];

    sliceUV[2][CSprite::VERT_URX] = pSpr->_uvs[CSprite::VERT_URX];
    sliceUV[2][CSprite::VERT_URY] = pSpr->_uvs[CSprite::VERT_URY];

    sliceUV[2][CSprite::VERT_BLX] = pSpr->_uvs[CSprite::VERT_URX] - fBuv;
    sliceUV[2][CSprite::VERT_BLY] = pSpr->_uvs[CSprite::VERT_URY] + fCuv;

    sliceUV[2][CSprite::VERT_BRX] = pSpr->_uvs[CSprite::VERT_URX];
    sliceUV[2][CSprite::VERT_BRY] = pSpr->_uvs[CSprite::VERT_ULY] + fCuv;


    for (size_t i = 0; i < 3; i++)
    {
        CSpritePtr p = CSprite::fromData(pSpr->_pTex, slice[i], sliceUV[i]);
        addChild(p);
    }
}

void ThreeSliceHor::build(float cx)
{
    float cy   = _vChildren[0]->getNotTransCy();
    _fcx = cx;
    _fcy = cy;

    float fSrcCx = _vChildren[0]->getNotTransCx() + _vChildren[1]->getNotTransCx() + _vChildren[2]->getNotTransCx();
    float fSrcCy = _vChildren[0]->getNotTransCy();

    //assert(cx >= fSrcCx);
    //assert(cy >= fSrcCy);

    float stabelCx = _vChildren[0]->getNotTransCx() + _vChildren[2]->getNotTransCx();
    float stabelCy = _vChildren[0]->getNotTransCy();

    float fScaleX = (cx - stabelCx) / (fSrcCx - stabelCx);

    _vChildren[1]->setScaleX(fScaleX);


    /*
    A                          B
    +---+----------------------+---+
  C | 1 |          2           | 3 |
    +---+----------------------+---+
    */

   //2
   _vChildren[1]->setPos(_vChildren[0]->getNotTransCx(), 0.f);
   //3
   _vChildren[2]->setPos(_vChildren[0]->getNotTransCx() + _vChildren[1]->getNotTransCx() * fScaleX, 0.f);

}

bool ThreeSliceHor::getNotTransBounds(Rect* p)
{
    p->init();
    Rect rc;
    for (size_t i = 0; i < _vChildren.size(); i++)
    {
        if (_vChildren[i]->getNotTransBounds(&rc))
            p->unite(&rc);
        else
        {
            assert(false);
        }
    }
    p->cx = _fcx;
    return true;
}

void ThreeSliceVert::createSlices(CSpritePtr pSpr, float fC, float fD)
{
    removeAll();
    
    float slice[3][8];
    float sliceUV[3][8];
    float fA   = pSpr->getNotTransCx();
    float fAuv = fA / pSpr->_pTex->getCx();
    float fCuv = fC / pSpr->_pTex->getCy();
    float fDuv = fD / pSpr->_pTex->getCy();        
    /*
    A                          
    +---+
  C | 1 |
    +---+
    |   |
    | 4 |
    |   |
    +---+
  D | 7 |
    +---+    
    */  
    //1        
    slice[0][CSprite::VERT_ULX] = pSpr->_verts[CSprite::VERT_ULX];
    slice[0][CSprite::VERT_ULY] = pSpr->_verts[CSprite::VERT_ULY];

    slice[0][CSprite::VERT_URX] = pSpr->_verts[CSprite::VERT_ULX] + fA;
    slice[0][CSprite::VERT_URY] = pSpr->_verts[CSprite::VERT_ULY];

    slice[0][CSprite::VERT_BLX] = pSpr->_verts[CSprite::VERT_ULX];
    slice[0][CSprite::VERT_BLY] = pSpr->_verts[CSprite::VERT_ULY] + fC;

    slice[0][CSprite::VERT_BRX] = pSpr->_verts[CSprite::VERT_ULX] + fA;
    slice[0][CSprite::VERT_BRY] = pSpr->_verts[CSprite::VERT_ULY] + fC;

    sliceUV[0][CSprite::VERT_ULX] = pSpr->_uvs[CSprite::VERT_ULX];
    sliceUV[0][CSprite::VERT_ULY] = pSpr->_uvs[CSprite::VERT_ULY];
    sliceUV[0][CSprite::VERT_URX] = pSpr->_uvs[CSprite::VERT_ULX] + fAuv;
    sliceUV[0][CSprite::VERT_URY] = pSpr->_uvs[CSprite::VERT_ULY];
    sliceUV[0][CSprite::VERT_BLX] = pSpr->_uvs[CSprite::VERT_ULX];
    sliceUV[0][CSprite::VERT_BLY] = pSpr->_uvs[CSprite::VERT_ULY] + fCuv;
    sliceUV[0][CSprite::VERT_BRX] = pSpr->_uvs[CSprite::VERT_ULX] + fAuv;
    sliceUV[0][CSprite::VERT_BRY] = pSpr->_uvs[CSprite::VERT_ULY] + fCuv;
    /*
    A                          
    +---+
  C | 1 |
    +---+
    |   |
    | 4 |
    |   |
    +---+
  D | 7 |
    +---+    
    */    
    //4
    slice[1][CSprite::VERT_ULX] = 0;
    slice[1][CSprite::VERT_ULY] = 0;

    slice[1][CSprite::VERT_URX] = fA;
    slice[1][CSprite::VERT_URY] = 0;

    slice[1][CSprite::VERT_BLX] = 0;
    slice[1][CSprite::VERT_BLY] = pSpr->getNotTransCy() - fC - fD;

    slice[1][CSprite::VERT_BRX] = fA;
    slice[1][CSprite::VERT_BRY] = pSpr->getNotTransCy() - fC - fD;


    sliceUV[1][CSprite::VERT_ULX] = pSpr->_uvs[CSprite::VERT_ULX];
    sliceUV[1][CSprite::VERT_ULY] = pSpr->_uvs[CSprite::VERT_ULY] + fCuv;

    sliceUV[1][CSprite::VERT_URX] = pSpr->_uvs[CSprite::VERT_ULX] + fAuv;
    sliceUV[1][CSprite::VERT_URY] = pSpr->_uvs[CSprite::VERT_ULY] + fCuv;

    sliceUV[1][CSprite::VERT_BLX] = pSpr->_uvs[CSprite::VERT_ULX];
    sliceUV[1][CSprite::VERT_BLY] = pSpr->_uvs[CSprite::VERT_BLY] - fDuv;

    sliceUV[1][CSprite::VERT_BRX] = pSpr->_uvs[CSprite::VERT_BLX] + fAuv;
    sliceUV[1][CSprite::VERT_BRY] = pSpr->_uvs[CSprite::VERT_BLY] - fDuv;   

    /*
    A     
    +---+
  C | 1 |
    +---+
    |   |
    | 4 |
    |   |
    +---+
  D | 7 |
    +---+    
    */    
    //7
    slice[2][CSprite::VERT_ULX] = 0;
    slice[2][CSprite::VERT_ULY] = 0;

    slice[2][CSprite::VERT_URX] = fA;
    slice[2][CSprite::VERT_URY] = 0;

    slice[2][CSprite::VERT_BLX] = 0;
    slice[2][CSprite::VERT_BLY] = fD;

    slice[2][CSprite::VERT_BRX] = fA;
    slice[2][CSprite::VERT_BRY] = fD;



    sliceUV[2][CSprite::VERT_ULX] = pSpr->_uvs[CSprite::VERT_BLX];
    sliceUV[2][CSprite::VERT_ULY] = pSpr->_uvs[CSprite::VERT_BLY] - fDuv;

    sliceUV[2][CSprite::VERT_URX] = pSpr->_uvs[CSprite::VERT_ULX] + fAuv;
    sliceUV[2][CSprite::VERT_URY] = pSpr->_uvs[CSprite::VERT_BLY] - fDuv;

    sliceUV[2][CSprite::VERT_BLX] = pSpr->_uvs[CSprite::VERT_BLX];
    sliceUV[2][CSprite::VERT_BLY] = pSpr->_uvs[CSprite::VERT_BLY];

    sliceUV[2][CSprite::VERT_BRX] = pSpr->_uvs[CSprite::VERT_BLX] + fAuv;
    sliceUV[2][CSprite::VERT_BRY] = pSpr->_uvs[CSprite::VERT_BLY];        

    for (size_t i = 0; i < 3; i++)
    {
        CSpritePtr p = CSprite::fromData(pSpr->_pTex, slice[i], sliceUV[i]);
        addChild(p);
    }
}

void ThreeSliceVert::build(float cy)
{
    float cx   = _vChildren[0]->getNotTransCx();
    _fcx = cx;
    _fcy = cy;

    float fSrcCx = _vChildren[0]->getNotTransCx();
    float fSrcCy = _vChildren[0]->getNotTransCy() + _vChildren[1]->getNotTransCy() + _vChildren[2]->getNotTransCy();

    //assert(cx >= fSrcCx);
    assert(cy >= fSrcCy);
    float stabelCy = _vChildren[0]->getNotTransCy() + _vChildren[2]->getNotTransCy();
    float fScaleY = (cy - stabelCy) / (fSrcCy - stabelCy);
    _vChildren[1]->setScaleY(fScaleY);
    /*
    A    
    +---+
  C | 1 |
    +---+
    |   |
    | 4 |
    |   |
    +---+
  D | 7 |
    +---+
    */

   //4
   _vChildren[1]->setPos(0.f, _vChildren[0]->getNotTransCy());
   //7
   _vChildren[2]->setPos(0, _vChildren[0]->getNotTransCy() + _vChildren[1]->getNotTransCy() * fScaleY);
}

bool ThreeSliceVert::getNotTransBounds(Rect* p)
{
    p->init();
    Rect rc;
    for (size_t i = 0; i < _vChildren.size(); i++)
    {
        if (_vChildren[i]->getNotTransBounds(&rc))
            p->unite(&rc);
        else
        {
            assert(false);
        }
    }
    p->cy = _fcy;
    return true;
}

_G2D_NAMESPACE_END_