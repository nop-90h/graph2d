#pragma once

#include "g2d.h"
#include "widgets/widget.h"
#include "threeslice.h"

_G2D_NAMESPACE_BEGIN_

class ProgressBar : public Widget
{
private:
    ThreeSliceHorPtr    _ptrFrame;
    ThreeSliceHorPtr    _ptrBar;
    CSpritePtr          _ptrSprBar;
    float               _fProgress   = 0;
    float               _fcx         = 0;
    float               _fBarCx      = 0;
    float               _fBarC       = 0;
    float               _fBarD       = 0;
    float               _fMinBarSize = 0;
public:
            void    create                  (float          fFrameCx,
                                             float          fBarCx, 
                                             CSpritePtr     ptrFrame,
                                             CSpritePtr     ptrBar,
                                             float          fFrameC,
                                             float          fFrameD,
                                             float          fBarC,
                                             float          fBarD,
                                             float          fBarOffsX = 0,
                                             float          fBarOffsY = 0);
            void    create                  (float          fFrameCx,
                                             float          fBarCx, 
                                             LPCCTEXT       lpccFrame,
                                             LPCCTEXT       lpccBar,
                                             float          fFrameC,
                                             float          fFrameD,
                                             float          fBarC,
                                             float          fBarD,
                                             float          fBarOffsX = 0,
                                             float          fBarOffsY = 0);
            void    setProgress             (float          fProgress);
    inline  float   getProgress             (void) { return _fProgress; }
protected:
    virtual bool                setTweenPropValue       (eTweenProp         eProp,
                                                         float              fValue)override;

public:
    virtual bool                getNotTransBounds       (Rect*              p) override;

};

typedef std::shared_ptr<ProgressBar> ProgressBarPtr;

_G2D_NAMESPACE_END_