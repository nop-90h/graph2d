#pragma once

#include "g2d.h"
#include "widgets/widget.h"
#include "widgets/staticlabel.h"
#include "widgets/nineslice.h"
#include "widgets/button.h"
#include "widgets/scrollbar.h"

_G2D_NAMESPACE_BEGIN_

class Slider : public Widget
{
public:
                    Slider                  (void);
        void        create                  (int            nFromVal,
                                             int            nToVal,
                                             int            cx);
        void        setVal                  (int            nVal);
 inline int         getVal                  (void) { return _val; }
 inline void        setOnChanged            (SimpleCallback cb) { _cbOnChanged = cb; }

public:
    virtual bool    getNotTransBounds       (Rect*          p) override;

protected:
        void        updateLabel             (void);
        float       calcFrameWidth          (void);
        void        onMinusBtn              (void);
        void        onPlusBtn               (void);

        void        onScrolled              (void);
private:
    NineSlicePtr        _ptrFrame;
    ScrollBarPtr        _ptrScrollbar;
    StaticLabelPtr      _label;
    ButtonPtr           _btnPlus;
    ButtonPtr           _btnMinus;
    CContainerPtr       _ptrCont;
    SimpleCallback      _cbOnChanged;
    int                 _from = 1;
    int                 _to   = 1;
    int                 _val  = 1;
};

typedef std::shared_ptr<Slider> SliderPtr;

_G2D_NAMESPACE_END_