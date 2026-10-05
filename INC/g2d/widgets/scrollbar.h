#pragma once
#include "g2d.h"
#include "widgets/widget.h"
#include "threeslice.h"

_G2D_NAMESPACE_BEGIN_

class ScrollBar : public Widget
{
private:
    CSpritePtr          _ptrSlider;
    CContainerPtr       _ptrFrameV;
    ThreeSliceHorPtr    _ptrFrameH;

    float               _pos = 0;
    float               _fSize = 0;

    LPCCTEXT            _lpccFrame  = nullptr;
    LPCCTEXT            _lpccSlider = nullptr;

    float               _fc = 20;
    float               _fd = 20;

    bool                _bFrameVisible = true;
    bool                _bIsSystem = true;

    eScrollType         _eType = E_ST_NONE;
    SimpleCallback      _cbOnScrolled;

    bool                _bDraggingSlider = false;
    float               _fDragOffsetLocal = 0.f;

private:
    float           getSliderLength         (void);
    float           getSliderTravel         (void);

public:
    void            create                  (eScrollType    eType,
                                             float          fSize);

    void            showSlider              (bool bShow) { if (_ptrSlider) _ptrSlider->setVisible(bShow); }

    void            setSliderPos            (float          fZeroToOnePos);
    float           getSliderPos            (void) { return _pos; }

    bool            isSlider                (CContainer*    pTest);
    float           calcScrollPercent       (float          fScreenXorY);

    void            beginSliderDrag         (float          fScreenXorY);
    float           calcSliderDragPercent   (float          fScreenXorY);
    void            endSliderDrag           ();
    void            setOptions              (bool           bFrameVisible = false,
                                             LPCCTEXT       lpccFrame = nullptr,
                                             LPCCTEXT       lpccSlider = nullptr,
                                             float          fc = 0,
                                             float          fd = 0,
                                             bool           bIsSystem = true);

    void            setOnScrolled           (SimpleCallback cb) { _cbOnScrolled = cb; }
    eScrollType     getType                 (void) { return _eType; }

public:
    virtual bool    getNotTransBounds       (Rect*          p) override;

protected:
    void            onSliderDrag            (int            x,
                                             int            y);
};

typedef std::shared_ptr<ScrollBar> ScrollBarPtr;

_G2D_NAMESPACE_END_