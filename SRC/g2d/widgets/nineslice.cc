#include "widgets/nineslice.h"

_G2D_NAMESPACE_BEGIN_

NineSlice::NineSlice(void)
{
    setClass(E_EL_NINESLICE);
}

void NineSlice::createSlices(CSpritePtr pSpr, float fA, float fB, float fC, float fD)
{
    removeAll();
    
    float slice[9][8];
    float sliceUV[9][8];

    float fAuv = fA / pSpr->_pTex->getCx();
    float fBuv = fB / pSpr->_pTex->getCx();
    float fCuv = fC / pSpr->_pTex->getCy();
    float fDuv = fD / pSpr->_pTex->getCy();        
    /*
    A                          B
    +---+----------------------+---+
  C | 1 |          2           | 3 |
    +---+----------------------+---+
    |   |                      |   |
    | 4 |          5           | 6 |
    |   |                      |   |
    +---+----------------------+---+
  D | 7 |          8           | 9 |
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
    |   |                      |   |
    | 4 |          5           | 6 |
    |   |                      |   |
    +---+----------------------+---+
  D | 7 |          8           | 9 |
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
    |   |                      |   |
    | 4 |          5           | 6 |
    |   |                      |   |
    +---+----------------------+---+
  D | 7 |          8           | 9 |
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

    /*
    A                          B
    +---+----------------------+---+
  C | 1 |          2           | 3 |
    +---+----------------------+---+
    |   |                      |   |
    | 4 |          5           | 6 |
    |   |                      |   |
    +---+----------------------+---+
  D | 7 |          8           | 9 |
    +---+----------------------+---+    
    */    
    //4
    slice[3][CSprite::VERT_ULX] = 0;
    slice[3][CSprite::VERT_ULY] = 0;

    slice[3][CSprite::VERT_URX] = fA;
    slice[3][CSprite::VERT_URY] = 0;

    slice[3][CSprite::VERT_BLX] = 0;
    slice[3][CSprite::VERT_BLY] = pSpr->getNotTransCy() - fC - fD;

    slice[3][CSprite::VERT_BRX] = fA;
    slice[3][CSprite::VERT_BRY] = pSpr->getNotTransCy() - fC - fD;


    sliceUV[3][CSprite::VERT_ULX] = pSpr->_uvs[CSprite::VERT_ULX];
    sliceUV[3][CSprite::VERT_ULY] = pSpr->_uvs[CSprite::VERT_ULY] + fCuv;

    sliceUV[3][CSprite::VERT_URX] = pSpr->_uvs[CSprite::VERT_ULX] + fAuv;
    sliceUV[3][CSprite::VERT_URY] = pSpr->_uvs[CSprite::VERT_ULY] + fCuv;

    sliceUV[3][CSprite::VERT_BLX] = pSpr->_uvs[CSprite::VERT_ULX];
    sliceUV[3][CSprite::VERT_BLY] = pSpr->_uvs[CSprite::VERT_BLY] - fDuv;

    sliceUV[3][CSprite::VERT_BRX] = pSpr->_uvs[CSprite::VERT_BLX] + fAuv;
    sliceUV[3][CSprite::VERT_BRY] = pSpr->_uvs[CSprite::VERT_BLY] - fDuv;   

    /*
    A                          B
    +---+----------------------+---+
  C | 1 |          2           | 3 |
    +---+----------------------+---+
    |   |                      |   |
    | 4 |          5           | 6 |
    |   |                      |   |
    +---+----------------------+---+
  D | 7 |          8           | 9 |
    +---+----------------------+---+    
    */    
    //5
    slice[4][CSprite::VERT_ULX] = 0;
    slice[4][CSprite::VERT_ULY] = 0;

    slice[4][CSprite::VERT_URX] = pSpr->getNotTransCx() - fB - fA;
    slice[4][CSprite::VERT_URY] = 0;

    slice[4][CSprite::VERT_BLX] = 0;
    slice[4][CSprite::VERT_BLY] = pSpr->getNotTransCy() - fD - fC;

    slice[4][CSprite::VERT_BRX] = pSpr->getNotTransCx() - fB - fA;
    slice[4][CSprite::VERT_BRY] = pSpr->getNotTransCy() - fD - fC;



    sliceUV[4][CSprite::VERT_ULX] = pSpr->_uvs[CSprite::VERT_ULX] + fAuv;
    sliceUV[4][CSprite::VERT_ULY] = pSpr->_uvs[CSprite::VERT_ULY] + fCuv;

    sliceUV[4][CSprite::VERT_URX] = pSpr->_uvs[CSprite::VERT_URX] - fBuv;
    sliceUV[4][CSprite::VERT_URY] = pSpr->_uvs[CSprite::VERT_ULY] + fCuv;

    sliceUV[4][CSprite::VERT_BLX] = pSpr->_uvs[CSprite::VERT_ULX] + fAuv;
    sliceUV[4][CSprite::VERT_BLY] = pSpr->_uvs[CSprite::VERT_BLY] - fDuv;

    sliceUV[4][CSprite::VERT_BRX] = pSpr->_uvs[CSprite::VERT_BRX] - fBuv;
    sliceUV[4][CSprite::VERT_BRY] = pSpr->_uvs[CSprite::VERT_BRY] - fDuv;


    /*
    A                          B
    +---+----------------------+---+
  C | 1 |          2           | 3 |
    +---+----------------------+---+
    |   |                      |   |
    | 4 |          5           | 6 |
    |   |                      |   |
    +---+----------------------+---+
  D | 7 |          8           | 9 |
    +---+----------------------+---+    
    */    
    //6
    slice[5][CSprite::VERT_ULX] = 0;
    slice[5][CSprite::VERT_ULY] = 0;

    slice[5][CSprite::VERT_URX] = fB;
    slice[5][CSprite::VERT_URY] = 0;

    slice[5][CSprite::VERT_BLX] = 0;
    slice[5][CSprite::VERT_BLY] = pSpr->getNotTransCy() - fD - fC;

    slice[5][CSprite::VERT_BRX] = fB;
    slice[5][CSprite::VERT_BRY] = pSpr->getNotTransCy() - fD - fC;



    sliceUV[5][CSprite::VERT_ULX] = pSpr->_uvs[CSprite::VERT_URX] - fBuv;
    sliceUV[5][CSprite::VERT_ULY] = pSpr->_uvs[CSprite::VERT_URY] + fCuv;

    sliceUV[5][CSprite::VERT_URX] = pSpr->_uvs[CSprite::VERT_URX];
    sliceUV[5][CSprite::VERT_URY] = pSpr->_uvs[CSprite::VERT_URY] + fCuv;

    sliceUV[5][CSprite::VERT_BLX] = pSpr->_uvs[CSprite::VERT_BRX] - fBuv;
    sliceUV[5][CSprite::VERT_BLY] = pSpr->_uvs[CSprite::VERT_BRY] - fDuv;

    sliceUV[5][CSprite::VERT_BRX] = pSpr->_uvs[CSprite::VERT_BRX];
    sliceUV[5][CSprite::VERT_BRY] = pSpr->_uvs[CSprite::VERT_BRY] - fDuv;
    /*
    A                          B
    +---+----------------------+---+
  C | 1 |          2           | 3 |
    +---+----------------------+---+
    |   |                      |   |
    | 4 |          5           | 6 |
    |   |                      |   |
    +---+----------------------+---+
  D | 7 |          8           | 9 |
    +---+----------------------+---+    
    */    
    //7
    slice[6][CSprite::VERT_ULX] = 0;
    slice[6][CSprite::VERT_ULY] = 0;

    slice[6][CSprite::VERT_URX] = fA;
    slice[6][CSprite::VERT_URY] = 0;

    slice[6][CSprite::VERT_BLX] = 0;
    slice[6][CSprite::VERT_BLY] = fD;

    slice[6][CSprite::VERT_BRX] = fA;
    slice[6][CSprite::VERT_BRY] = fD;



    sliceUV[6][CSprite::VERT_ULX] = pSpr->_uvs[CSprite::VERT_BLX];
    sliceUV[6][CSprite::VERT_ULY] = pSpr->_uvs[CSprite::VERT_BLY] - fDuv;

    sliceUV[6][CSprite::VERT_URX] = pSpr->_uvs[CSprite::VERT_ULX] + fAuv;
    sliceUV[6][CSprite::VERT_URY] = pSpr->_uvs[CSprite::VERT_BLY] - fDuv;

    sliceUV[6][CSprite::VERT_BLX] = pSpr->_uvs[CSprite::VERT_BLX];
    sliceUV[6][CSprite::VERT_BLY] = pSpr->_uvs[CSprite::VERT_BLY];

    sliceUV[6][CSprite::VERT_BRX] = pSpr->_uvs[CSprite::VERT_BLX] + fAuv;
    sliceUV[6][CSprite::VERT_BRY] = pSpr->_uvs[CSprite::VERT_BLY];        

    /*
    A                          B
    +---+----------------------+---+
  C | 1 |          2           | 3 |
    +---+----------------------+---+
    |   |                      |   |
    | 4 |          5           | 6 |
    |   |                      |   |
    +---+----------------------+---+
  D | 7 |          8           | 9 |
    +---+----------------------+---+    
    */    
    //8
    slice[7][CSprite::VERT_ULX] = 0;
    slice[7][CSprite::VERT_ULY] = 0;

    slice[7][CSprite::VERT_URX] = pSpr->getNotTransCx() - fA - fB;
    slice[7][CSprite::VERT_URY] = 0;

    slice[7][CSprite::VERT_BLX] = 0;
    slice[7][CSprite::VERT_BLY] = fD;

    slice[7][CSprite::VERT_BRX] = pSpr->getNotTransCx() - fA - fB;
    slice[7][CSprite::VERT_BRY] = fD;


    sliceUV[7][CSprite::VERT_ULX] = pSpr->_uvs[CSprite::VERT_BLX] + fAuv;
    sliceUV[7][CSprite::VERT_ULY] = pSpr->_uvs[CSprite::VERT_BLY] - fDuv;

    sliceUV[7][CSprite::VERT_URX] = pSpr->_uvs[CSprite::VERT_BRX] - fBuv;
    sliceUV[7][CSprite::VERT_URY] = pSpr->_uvs[CSprite::VERT_BLY] - fDuv;

    sliceUV[7][CSprite::VERT_BLX] = pSpr->_uvs[CSprite::VERT_BLX] + fAuv;
    sliceUV[7][CSprite::VERT_BLY] = pSpr->_uvs[CSprite::VERT_BLY];

    sliceUV[7][CSprite::VERT_BRX] = pSpr->_uvs[CSprite::VERT_BRX] - fBuv;
    sliceUV[7][CSprite::VERT_BRY] = pSpr->_uvs[CSprite::VERT_BRY];

    /*
    A                          B
    +---+----------------------+---+
  C | 1 |          2           | 3 |
    +---+----------------------+---+
    |   |                      |   |
    | 4 |          5           | 6 |
    |   |                      |   |
    +---+----------------------+---+
  D | 7 |          8           | 9 |
    +---+----------------------+---+    
    */    
    //9
    slice[8][CSprite::VERT_ULX] = 0;
    slice[8][CSprite::VERT_ULY] = 0;

    slice[8][CSprite::VERT_URX] = fB;
    slice[8][CSprite::VERT_URY] = 0;

    slice[8][CSprite::VERT_BLX] = 0;
    slice[8][CSprite::VERT_BLY] = fD;

    slice[8][CSprite::VERT_BRX] = fB;
    slice[8][CSprite::VERT_BRY] = fD;




    sliceUV[8][CSprite::VERT_ULX] = pSpr->_uvs[CSprite::VERT_BRX] - fBuv;
    sliceUV[8][CSprite::VERT_ULY] = pSpr->_uvs[CSprite::VERT_BRY] - fDuv;

    sliceUV[8][CSprite::VERT_URX] = pSpr->_uvs[CSprite::VERT_BRX];
    sliceUV[8][CSprite::VERT_URY] = pSpr->_uvs[CSprite::VERT_BRY] - fDuv;

    sliceUV[8][CSprite::VERT_BLX] = pSpr->_uvs[CSprite::VERT_BRX] - fBuv;
    sliceUV[8][CSprite::VERT_BLY] = pSpr->_uvs[CSprite::VERT_BRY];

    sliceUV[8][CSprite::VERT_BRX] = pSpr->_uvs[CSprite::VERT_BRX];
    sliceUV[8][CSprite::VERT_BRY] = pSpr->_uvs[CSprite::VERT_BRY];

    for (size_t i = 0; i < 9; i++)
    {
        CSpritePtr p = CSprite::fromData(pSpr->_pTex, slice[i], sliceUV[i]);
        addChild(p);
    }
}
void NineSlice::setGrayScale(bool gs)
{
    Widget::setGrayScale(gs);
    for (int i = 0; i < getChildrenCount(); i++)
        getChildAt(i)->setGrayScale(gs);
}

void NineSlice::build(float cx, float cy)
{
    _fcx = cx;
    _fcy = cy;

    float fSrcCx = _vChildren[0]->getNotTransCx() + _vChildren[1]->getNotTransCx() + _vChildren[2]->getNotTransCx();
    float fSrcCy = _vChildren[0]->getNotTransCy() + _vChildren[3]->getNotTransCy() + _vChildren[6]->getNotTransCy();

    //assert(cx >= fSrcCx);
    //assert(cy >= fSrcCy);

    float stabelCx = _vChildren[0]->getNotTransCx() + _vChildren[2]->getNotTransCx();
    float stabelCy = _vChildren[0]->getNotTransCy() + _vChildren[6]->getNotTransCy();

    float fScaleX = (cx - stabelCx) / (fSrcCx - stabelCx);
    float fScaleY = (cy - stabelCy) / (fSrcCy - stabelCy);

    _vChildren[1]->setScaleX(fScaleX);
    _vChildren[7]->setScaleX(fScaleX);

    //_vChildren[3]->setTransCenter(-_vChildren[3]->getNotTransCx() * fScaleX, 0);
    _vChildren[3]->setScaleY(fScaleY);
    _vChildren[5]->setScaleY(fScaleY);

    _vChildren[4]->setScale(fScaleX, fScaleY);

    /*
    A                          B
    +---+----------------------+---+
  C | 1 |          2           | 3 |
    +---+----------------------+---+
    |   |                      |   |
    | 4 |          5           | 6 |
    |   |                      |   |
    +---+----------------------+---+
  D | 7 |          8           | 9 |
    +---+----------------------+---+    
    */

   //2
   _vChildren[1]->setPos(_vChildren[0]->getNotTransCx(), 0.f);
   //3
   _vChildren[2]->setPos(_vChildren[0]->getNotTransCx() + _vChildren[1]->getNotTransCx() * fScaleX, 0.f);
   //4
   _vChildren[3]->setPos(0.f, _vChildren[0]->getNotTransCy());
   //5
   _vChildren[4]->setPos(_vChildren[0]->getNotTransCx(), _vChildren[0]->getNotTransCy());
   //6
   _vChildren[5]->setPos(_vChildren[0]->getNotTransCx() + _vChildren[1]->getNotTransCx() * fScaleX, _vChildren[0]->getNotTransCy());
   //7
   _vChildren[6]->setPos(0, _vChildren[0]->getNotTransCy() + _vChildren[3]->getNotTransCy() *fScaleY);
   //8
   _vChildren[7]->setPos(_vChildren[0]->getNotTransCx(), _vChildren[0]->getNotTransCy() + _vChildren[3]->getNotTransCy() *fScaleY);
   //9
   _vChildren[8]->setPos(_vChildren[0]->getNotTransCx() + _vChildren[1]->getNotTransCx() * fScaleX, _vChildren[2]->getNotTransCy() + _vChildren[5]->getNotTransCy() *fScaleY);
}

//void NineSlice::applyTransform(CMatrixStack* pMS, bool bForce)
//{
//}
//
//bool NineSlice::getTransBounds(Rect* p)
//{
//
//}

bool NineSlice::getNotTransBounds(Rect* p)
{
    p->set(0.f, 0.f, _fcx, _fcy);
    return true;
}

NineSlicePtr NineSlice::cloneInitial()
{
    NineSlicePtr ptr = std::make_shared<NineSlice>();
    ptr->_vChildren.reserve(_vChildren.size());
    for (auto it: _vChildren)
    {
        assert(it->getClass() == E_EL_SPRITE);
        ptr->addChild(std::dynamic_pointer_cast<CSprite>(it)->cloneInitial());
    }
    return ptr;
}

_G2D_NAMESPACE_END_