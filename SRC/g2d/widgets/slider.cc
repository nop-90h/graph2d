#include "widgets/slider.h"
#include "spriteloader.h"

_G2D_NAMESPACE_BEGIN_

Slider::Slider()
{
}

void Slider::create(int nFromVal, int nToVal, int cx)
{
    _from = nFromVal;
    _to   = nToVal;
    _val  = _from;

    _btnMinus       = std::make_shared<Button>();
    _btnPlus        = std::make_shared<Button>();
    _ptrScrollbar   = std::make_shared<ScrollBar>();
    _ptrCont        = std::make_shared<CContainer>();

    _btnMinus->create("smallBtn");
    _btnMinus->setHoverAnim();
    _btnMinus->setText("-");
    _btnMinus->setTextRgba(0xa5ec48ff);
    _btnMinus->setTextScale(3.5f);
    _btnMinus->setTextOffsetY(-16);
    _btnMinus->setScaleTo(40, 40);
    _btnMinus->setId();
    _btnMinus->setOnClick([this](){ onMinusBtn(); });
    _ptrCont->addChild(_btnMinus);

    _ptrScrollbar->setOptions(true, "UI/slider_frame", "UI/slider_slider", 9, 9, false);
    _ptrScrollbar->create(E_ST_HOR, cx);
    _ptrScrollbar->setX(_btnMinus->getNotTransCx() * _btnMinus->getScaleX() + 10);
    _ptrScrollbar->setOnScrolled([this](){ onScrolled(); });
    _ptrCont->addChild(_ptrScrollbar);

    _btnPlus->create("smallBtn");
    _btnPlus->setHoverAnim();
    _btnPlus->setText("+");
    _btnPlus->setTextRgba(0xa5ec48ff);
    _btnPlus->setTextScale(3.5f);
    _btnPlus->setTextOffsetY(-16);
    _btnPlus->setScaleTo(40, 40);
    _btnPlus->setX(_ptrScrollbar->getX() + _ptrScrollbar->getNotTransCx() + 10);
    _btnPlus->setId();
    _btnPlus->setOnClick([this](){ onPlusBtn(); });
    _ptrCont->addChild(_btnPlus);

    _label = std::make_shared<StaticLabel>();

    _ptrFrame = std::make_shared<NineSlice>();
    _ptrFrame->createSlices(SpriteLoader::getInstance()->getSprite("UI/thinFrameSolid"), 16, 16, 16, 16);
    _ptrFrame->build(calcFrameWidth(), 150.f);
    _ptrFrame->addChild(_ptrCont);
    _ptrFrame->addChild(_label);
    _ptrCont->setPos(20, 60);

    addChild(_ptrFrame);
    setVal(_val);
    updateLabel();
}

float Slider::calcFrameWidth()
{
    float fRes = _btnPlus->getNotTransCx() * _btnPlus->getScaleX() + _btnPlus->getX() + 40;
    return fRes;
}

void Slider::updateLabel()
{
    _label->setTextFmt("%i / %i", _val, _to);
    _label->setXPosCentered(calcFrameWidth());
    _label->setY(105.f);
}

void Slider::setVal(int nVal)
{
    nVal = std::max(_from, nVal);
    nVal = std::min(_to, nVal);
    _val = nVal;
    float fPerc = 0;
    if (_to > _from)
    {
        float f = (_to - _from);
        fPerc = (_val - _from) / f;
    }
    else if (_to == _from)
        fPerc = 1.f;
    _ptrScrollbar->setSliderPos(fPerc);
    updateLabel();
}

bool Slider::getNotTransBounds(Rect* p)
{
    return _ptrFrame->getNotTransBounds(p);
}

void Slider::onMinusBtn(void)
{
    setVal(_val - 1);
    if (_cbOnChanged)
        _cbOnChanged();
}

void Slider::onPlusBtn(void)
{
    setVal(_val + 1);
    if (_cbOnChanged)
        _cbOnChanged();
}

void Slider::onScrolled(void)
{
    float fScrollPos = _ptrScrollbar->getSliderPos();
    float fVal = (_to - _from) * fScrollPos; 
    int nVal = std::round(fVal) + _from;
    setVal(nVal);
    if (_cbOnChanged)
        _cbOnChanged();
}

_G2D_NAMESPACE_END_