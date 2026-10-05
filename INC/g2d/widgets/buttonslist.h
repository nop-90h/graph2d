#pragma once

#include "g2d.h"
#include "widgets/widget.h"
#include "widgets/buttonsliced.h"

_G2D_NAMESPACE_BEGIN_

template<typename T>
struct is_button_sliced_tml_instance : std::false_type {};

template<StaticStr U>
struct is_button_sliced_tml_instance<ButtonSlicedTmpl<U>> : std::true_type {};

template<typename T>
concept ButtonSlicedTmplInstance = is_button_sliced_tml_instance<T>::value;

template<ButtonSlicedTmplInstance T, float fListPad = 5.f, float fButtonCy = 80.f>
class ButtonList : public Widget
{
private:
    float   _maxCx;
    float   _fCurrentRowCx = fListPad;
    float   _fCurrentY     = fListPad;
public:
    ButtonList(float maxCx):_maxCx(maxCx)
    {
    }

    std::shared_ptr<T> addButton(LPCTSTR lpszText, SimpleCallback cb)
    {
        auto ptrBtn = T::makeInst(fButtonCy, lpszText, cb);
        if (_fCurrentRowCx > fListPad && _fCurrentRowCx + ptrBtn->getNotTransCx() + fListPad >= _maxCx)
        {
            _fCurrentRowCx  = fListPad;
            _fCurrentY     += fButtonCy + fListPad;
        }
        addChild(ptrBtn);
        ptrBtn->setPos(_fCurrentRowCx, _fCurrentY);
        _fCurrentRowCx += (ptrBtn->getNotTransCx() + fListPad);
        return ptrBtn;
    }
};

typedef ButtonList<OrangeSlicedButton> OrangeButtonList;
typedef std::shared_ptr<OrangeButtonList> OrangeButtonListPtr;

typedef ButtonList<GreenSlicedButton> GreenButtonList;
typedef std::shared_ptr<GreenButtonList> GreenButtonListPtr;

_G2D_NAMESPACE_END_