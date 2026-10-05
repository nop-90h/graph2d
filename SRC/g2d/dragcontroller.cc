#include "dragcontroller.h"
#include "inputcontroller.h"
#include "gfx.h"

_G2D_NAMESPACE_BEGIN_

void DragController::init()
{
    _dragLayer = std::make_shared<CContainer>();
}

void DragController::onEvent(int nEvent, void* pData1, void* pData2, void* pData3)
{
    int x = int32_t(pData1);
    int y = int32_t(pData2);

    switch (nEvent)
    {
        case InputController::EVT_ON_MOUSE_DOWN:
        {
            completeDrag(true);
        }
        break;

        case InputController::EVT_ON_MOUSE_UP:
        {
            if (_draggingObj)
            {
                auto p = _draggingObj->getDefaultDropAcceptor().lock(); 
                if (!p)
                {
                    p = CSprite::findTrackedAtPoint(x, y, nullptr, _dragParentDlg.get());
                    if (p)
                    {
                        auto pp = p->getAncestorByAnyId(true);
                        if (pp)
                            p = pp->shared_from_this();
                        else
                            p = nullptr;
                    }
                }

                if (p && p->onDragDropAccept(_draggingObj))
                {
                    auto dropSaved =_draggingObj;
                    completeDrag(false);
                    p->onDragDropped(x, y, dropSaved);
                }
                else
                {
//                    auto dropSaved =_draggingObj;
                    completeDrag(true);
                    //auto pDlg = CSprite::getDialogAtPoint(x, y);
                    //if (pDlg != _dragParentDlg)
                    //    dropSaved->onDraggedOutOfDialog();
                }
            }
        }
        break;

        case InputController::EVT_ON_MOUSE_MOVE:
        {

            moveDragObjToMousePos(x, y);
            auto p = CSprite::findTrackedAtPoint(x, y, nullptr, _dragParentDlg.get());
            if (p)
            {
                auto pp = p->getAncestorByAnyId(true);
                if (pp)
                    p = pp->shared_from_this();
                else
                    p = nullptr;
            }
            if (p != _currDragOver)
            {
                if (_currDragOver)
                    _currDragOver->onDragOver(_draggingObj, false);
                _currDragOver = p;
                if (_currDragOver)
                    _currDragOver->onDragOver(_draggingObj, true);
            }
        }
        break;
    }
}

void DragController::moveDragObjToMousePos(int32_t xMouse, int32_t yMouse)
{
    assert(_draggingObj);
    float fGmX = CSceneResize::getInstance()->getGameOffsX();
    float fGmY = CSceneResize::getInstance()->getGameOffsY();
    if (_draggingObj)
    {
        _draggingObj->setPos((xMouse + _fDragIconOffsX) - fGmX, 
                             (yMouse + _fDragIconOffsY) - fGmY);
    }
}


void DragController::completeDrag(bool bCanceled)
{
    _draggingObj->setScale(_fOrigScaleX, _fOrigScaleY);
    if (_currDragOver)
        _currDragOver->onDragOver(_draggingObj, false);
    _currDragOver = nullptr;
    _dragParentDlg = nullptr;
    if (bCanceled && _prevObjParent)
    {
        _dragLayer->removeChild(_draggingObj);
        _prevObjParent->addChild(_draggingObj);
        _draggingObj->setPos(_fOrigX, _fOrigY);
    }
    else
    {
        _draggingObj->removeFromParent();
    }
    _draggingObj->popSetSaveTransCoordsRecur();
    _draggingObj = nullptr;
    _prevObjParent = nullptr;
    CGfx::getInstance()->getGameRoot()->removeChild(_dragLayer);
    InputController::getInstance()->setExclusiveMouse(nullptr);
}

bool DragController::beginDrag(CContainerPtr ptrDrag, int32_t x, int32_t y)
{
    bool bRes = false;
    assert(ptrDrag);
    assert(!_draggingObj);
    _currDragOver = nullptr;
    if (!_draggingObj && ptrDrag)
    {
        if (ptrDrag->onDragBegin(x, y))
        {
            auto pParentDlg = ptrDrag->getParentDialog();
            assert(pParentDlg);
            if (pParentDlg)
            {
                _dragParentDlg = pParentDlg;
                _draggingObj = ptrDrag;
                InputController::getInstance()->setExclusiveMouse(this);
                CGfx::getInstance()->getGameIface()->addChild(_dragLayer);
                _prevObjParent = ptrDrag->getParent();

                float fTotalScaleX = 0;
                float fTotalScaleY = 0;
                ptrDrag->calcAncestorsScale(fTotalScaleX, fTotalScaleY);

                fTotalScaleX *= ptrDrag->getScaleX();
                fTotalScaleY *= ptrDrag->getScaleY();

                // _fDragIconOffs — чистое смещение объекта от курсора:
                //   pos = mouse + ofs - gameOffs  (см. moveDragObjToMousePos).
                // Задаётся в ЛОКАЛЬНЫХ юнитах объекта, переводим в screen px
                // через суммарный скейл. Никаких вычитаний боксов —
                // смешение screen px с game coords давало "вылет за экран".
                const Point ptAdjust = ptrDrag->getLocalDragIconAdjust();
                _fDragIconOffsX = ptAdjust.x * fTotalScaleX;
                _fDragIconOffsY = ptAdjust.y * fTotalScaleY;
                //fTotalScaleX /= CGfx::getInstance()->getGameRoot()->getScaleX();
                //fTotalScaleY /= CGfx::getInstance()->getGameRoot()->getScaleY();

                if (_prevObjParent)
                {
                    _prevObjParent->removeChild(ptrDrag);
                
                }

                _draggingObj->pushSetSaveTransCoordsRecur(false);

                _fOrigX      = _draggingObj->getX();
                _fOrigY      = _draggingObj->getY();
                _fOrigScaleX = _draggingObj->getScaleX();
                _fOrigScaleY = _draggingObj->getScaleY();

                _draggingObj->setScale(fTotalScaleX, fTotalScaleY);
                _dragLayer->addChild(_draggingObj);
                moveDragObjToMousePos(x, y);
                bRes = true;
            }
        }
    }
    return bRes;
}

_G2D_NAMESPACE_END_