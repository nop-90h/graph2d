#pragma once

#include "g2d.h"
#include "container.h"

_G2D_NAMESPACE_BEGIN_

class DragController : public IEventListener
{
private:
    CContainer*              _prevObjParent;
    float                    _fOrigX;
    float                    _fOrigY;
    float                    _fOrigScaleX;
    float                    _fOrigScaleY;
    float                    _fDragIconOffsX;
    float                    _fDragIconOffsY;
    CContainerPtr            _draggingObj;
    CContainerPtr            _dragLayer;
    CContainerPtr            _currDragOver;
    CContainerPtr            _dragParentDlg;

private:
    void                            init                    (void);
    void                            moveDragObjToMousePos   (int32_t        xMouse, 
                                                             int32_t        yMouse);
    void                            completeDrag            (bool           bCanceled);

public:
    inline static DragController*   getInstance             (void)
    {
        static bool bShouldInit = true;
        static DragController res;

        if (bShouldInit)
        {
            res.init();
        }
        return &res;
    }

    virtual     void                onEvent                 (int            nEvent,
                                                             void*          pData1,
                                                             void*          pData2,
                                                             void*          pData3 );

    bool                            beginDrag               (CContainerPtr  ptrDrag, 
                                                             int32_t        x, 
                                                             int32_t        y);
};

_G2D_NAMESPACE_END_