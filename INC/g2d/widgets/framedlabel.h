#pragma once

#include "g2d.h"
#include "widgets/widget.h"
#include "widgets/staticlabel.h"
#include "widgets/nineslice.h"
#include "widgets/buttonsliced.h"

_G2D_NAMESPACE_BEGIN_

enum class eControlPos
{
    RIGHT = 0,
    LEFT,
    COUNT
};

struct ControlInfo
{
    CContainerPtr _p;
    float         _fPad = 0.f;

    float getPad()
    {
        return _p ? _fPad: 0.f;
    }

};

class FramedLabel : public Widget
{
public:
                    FramedLabel             (void);
    void            setText                 (const char*            lpccLabelText,
                                             float                  fPadX = 0.f,
                                             float                  fPadY = 0.f);
    void            setTextFmt              (const char*            lpccLabelText, 
                                             float                  fPadX, 
                                             float                  fPadY,
                                             ...);
    void            setFrame                (NineSlicePtr           pFrameProto);
    void            setFrame                (CSpritePtr             pFrame,
                                             float                  fA,
                                             float                  fB,
                                             float                  fC,
                                             float                  fD);
    void            setTextBox              (bool                   bSet,
                                             float                  fBoxCx = 0.f);
    auto            getFrame                (void) { return _frame; }
    void            setDefFrame             (void);
    void            setTextLeftPad          (float                  fLeftPad,
                                             bool                   bUpdate = false);
    void            setTextPadTop           (float                  fTopPad,
                                             bool                   Update = false);
    void            setControl              (eControlPos            ePos,
                                             CContainerPtr          ptrControl,
                                             float                  fControlPad);
    void            removeControl           (eControlPos            ePos);
    void            removeControls          (void);
    void            update                  (void){ if (_bIsTextSet) updateFrame(); }
    inline void     setMinFrameCx           (int                    nCx) { _nMinFrameCx = nCx; }
    inline void     setMinFrameCy           (int                    nCy) { _nMinFrameCy = nCy; }

public: //CContainer
    virtual bool    getNotTransBounds       (Rect*                  p);

protected:
    void            updateFrame             (void);

private:

    ControlInfo     _controls[(int)eControlPos::COUNT];
    NineSlicePtr    _frame;
    StaticLabelPtr  _label;
    float           _fPadX        = 0.f;
    float           _fPadY        = 0.f;
    float           _fTextLeftPad = 0.f;
    float           _fTextTopPad  = 0.f;
    float           _fControlsPad = 0.f;
    bool            _bIsTextSet   = false;
    int             _nMinFrameCx  = 0;
    int             _nMinFrameCy  = 0;
    bool            _bBoxed       = false;
};

typedef std::shared_ptr<FramedLabel> FramedLabelPtr;

_G2D_NAMESPACE_END_