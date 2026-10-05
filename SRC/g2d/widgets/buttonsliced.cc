#include "widgets/buttonsliced.h"
#include "spriteloader.h"

_G2D_NAMESPACE_BEGIN_

void ButtonSliced::create(const char* lpccNamePrefix, float fA, float fB, float fC, float fD, bool bOnOff, bool bIsOn, CContainerPtr ptrIcon)
{

    std::string strNormal = std::format("{}_normal", lpccNamePrefix);
    std::string strHl     = std::format("{}_hl", lpccNamePrefix);

    _bgNormal             = std::make_shared<NineSlice>();
    _bgGS                 = std::make_shared<NineSlice>();
    _bgHL                 = std::make_shared<NineSlice>();
    _bgNormal->createSlices(SpriteLoader::getInstance()->getSprite(strNormal.c_str()), fA, fB, fC, fD);
    _bgGS->createSlices(SpriteLoader::getInstance()->getSprite(strNormal.c_str()), fA, fB, fC, fD);
    _bgHL->createSlices(SpriteLoader::getInstance()->getSprite(strHl.c_str()), fA, fB, fC, fD);
    _bgGS->setGrayScale();

    _twHandle = _bgHL->getInitialTweenHandle();

    addChild(_bgNormal);
    addChild(_bgHL);
    addChild(_bgGS);

    if (bOnOff)
    {
        assert(!ptrIcon);
        _ptrOnOffCont = std::make_shared<CContainer>();

        _ptrSprOn  = SpriteLoader::getInstance()->getSprite("UI/iconOn");
        _ptrSprOff = SpriteLoader::getInstance()->getSprite("UI/iconOff");
        _ptrOnOffCont->addChild(_ptrSprOn);
        _ptrOnOffCont->addChild(_ptrSprOff);
        _ptrOnOffCont->setYPosCentered(getNotTransCy(), -10);
        _ptrOnOffCont->setX(10.f);
        _ptrOnOffCont->setScale(0.6f, 0.6f);
        addChild(_ptrOnOffCont);
        _eIcon = eButtonSlicedIcon::ONOFF;
        setOnOffState(bIsOn);
        _ptrOnOffCont->setAlpha(0.80f);
    }
    else if (ptrIcon)
    {
        _ptrOnOffCont = std::make_shared<CContainer>();
        _ptrOnOffCont->addChild(ptrIcon);
        _ptrOnOffCont->setX(10.f);
        //_ptrOnOffCont->setScale(0.6f, 0.6f);
        addChild(_ptrOnOffCont);
        _eIcon = eButtonSlicedIcon::ICON;
        _ptrOnOffCont->setAlpha(0.80f);
    }

    setState(E_BS_NORMAL);
}

void ButtonSliced::setOnOffState(bool bIsOn)
{
    assert(_ptrOnOffCont);
    assert(_eIcon == eButtonSlicedIcon::ONOFF);
    if (_ptrOnOffCont && _eIcon == eButtonSlicedIcon::ONOFF)
    {
        _bIsOn = bIsOn;
        _ptrSprOn->setVisible(bIsOn);
        _ptrSprOff->setVisible(!bIsOn);
    }
}

bool ButtonSliced::getOnOffState(void)
{
    bool bRes = false;
    assert(_eIcon == eButtonSlicedIcon::ONOFF);
    if (_eIcon == eButtonSlicedIcon::ONOFF)
        bRes = _bIsOn;
    return bRes;
}

void ButtonSliced::build(float cx, float cy)
{
    _bgNormal->build(cx, cy);
    _bgHL->build(cx, cy);
    _bgGS->build(cx, cy);
    if (_ptrOnOffCont)
    {
        _ptrOnOffCont->setYPosCentered(calcNotTransCy(), _eIcon == eButtonSlicedIcon::ONOFF ? -20 : 0);
        _ptrOnOffCont->setX(15.f);
    }
    setTextPos();
}

void ButtonSliced::setTextRgba(uint32_t rgbaHex)
{
    assert(_ptrText);
    if (_ptrText)
    {
        _ptrText->setRgba(rgbaHex);
    }
}

void ButtonSliced::setTextPos()
{
    if (_ptrText)
    {
        float fOnOffCx = _ptrOnOffCont ?  _ptrOnOffCont->calcNotTransCx() * _ptrOnOffCont->getScaleX() : 0;
        float fOnOffX  = _ptrOnOffCont ?  _ptrOnOffCont->getX() : 0;

        Rect rcText;
        _ptrText->calcNotTransBounds(&rcText);

        float fCx      = getNotTransCx() - (fOnOffCx + fOnOffX);
        float fCy      = calcNotTransCy();

        _ptrText->setX(fOnOffCx + ((fCx + fOnOffX) - rcText.cx - rcText.x) * 0.5f);
        _ptrText->setYPosCentered(fCy, 0.f);
    }
}

void ButtonSliced::setHtmlText(LPCCTEXT lpccText)
{
    if (!_ptrText)
    {
        _ptrText = std::make_shared<StaticLabel>();
        addChild(_ptrText);
    }
    _ptrText->setBoxMode(eTextRenderType::HTML_BOX, _bgNormal->calcNotTransCx());
    _ptrText->setText(lpccText);
    _ptrText->setPosCentered(_bgNormal->calcNotTransCx(), _bgNormal->calcNotTransCy());
}

void ButtonSliced::setText(LPCCTEXT lpccText, float fAddX, float fAddY)
{
    if (!_ptrText)
    {
        _ptrText = std::make_shared<StaticLabel>();
        addChild(_ptrText);
    }
    //_ptrText->setAlign(eTRAlign::TR_ALIGN_MIDDLE);


    _ptrText->setText(lpccText);
    _ptrText->setAlpha(0.9f);
    setTextPos();
}

void ButtonSliced::setState(eButtonState eNewState)
{
    if (eNewState != _eState)
    {
        _eState = eNewState;
        updateState();
    }
}

void ButtonSliced::setEnabled(bool bEnabled)
{   
    if (!bEnabled)
        setState(E_BS_DISABLED);
    else if(!isEnabled())
        setState(E_BS_NORMAL);
}

void ButtonSliced::updateState()
{
    switch (_eState)
    {
        case E_BS_NORMAL:
        {
            _bgHL->setAlpha(0.4f);

            _bgNormal->setVisible(true);
            _bgHL->setVisible(true);
            _bgGS->setVisible(false);
            if (_twHandle)
                _bgHL->removeSelfTween(_twHandle);
            _bgHL->addSelfTween(&_twHandle, eTweenProp::ALPHA, .4f, 0.001f, 0.15f, Easing::linear);
        }
        break;

        case E_BS_HOVER:
        {
            _nBlinkCounter = 10;
            _bgNormal->setVisible(true);
            _bgHL->setVisible(true);
            _bgHL->setAlpha(0.0f);
            _bgGS->setVisible(false);
            if (_twHandle)
                _bgHL->removeSelfTween(_twHandle);
            _bgHL->addSelfTween(&_twHandle, eTweenProp::ALPHA, 0.1f, 0.4f, 0.15f, Easing::linear);
        }
        break;

        case E_BS_PRESSED:
        {
            _nBlinkCounter = 10;
            if (_twHandle)
                _bgHL->removeSelfTween(_twHandle);

            _bgNormal->setVisible(true);
            _bgHL->setVisible(true);
            _bgHL->setAlpha(0.7f);
            _bgGS->setVisible(false);
        }
        break;

        case E_BS_DISABLED:
        {
            _nBlinkCounter = 10;
            if (_twHandle)
                _bgHL->removeSelfTween(_twHandle);

            _bgNormal->setVisible(true);
            _bgHL->setVisible(false);
            _bgGS->setVisible(true);
            _bgGS->setAlpha(0.7f);
        }
        break;

        default:
        {
            assert(false);
        }
        break;
    }
}

void ButtonSliced::doBlink()
{
    if (!_doBlink)
    {
        _doBlink = [this](bool bH)
        {
            if (bH)
            {
                _bgHL->setVisible(true);
                _bgHL->setAlpha(0.0f);
                _bgGS->setVisible(false);

            }
            else
            {
                _bgHL->setAlpha(0.7f);
                _bgHL->setVisible(true);
                _bgGS->setVisible(false);
            }
            if (_twHandle)
                _bgHL->removeSelfTween(_twHandle);
            _bgHL->addSelfTween(&_twHandle, eTweenProp::ALPHA, bH ? 0.1f : .7f, bH ? 0.7f : 0.001f, 0.35f, Easing::linear, 0.f, [this, bH]{
                if (!bH)
                {
                    _nBlinkCounter++;
                    if (_nBlinkCounter < 10)
                    {
                        _doBlink(true);
                    }
                }
                else
                {
                    _doBlink(!bH);
                }

            });
        };
    }
    _doBlink(true);
}

void ButtonSliced::blink()
{
    if (_eState == E_BS_NORMAL)
    {
        _nBlinkCounter = 0;
        doBlink();
    }
}

void ButtonSliced::cancelBlink(void)
{
    if (_eState == E_BS_NORMAL && _nBlinkCounter > 0)
    {
        _nBlinkCounter = 10;
        if (_twHandle)
            _bgHL->removeSelfTween(_twHandle);
        _bgHL->setAlpha(0.4f);

    }
}

void ButtonSliced::onHover()
{
    if (_eState != E_BS_DISABLED)
    {
        setState(E_BS_HOVER);
    }
}

void ButtonSliced::onLeave()
{
    if (_eState != E_BS_DISABLED)
    {
        setState(E_BS_NORMAL);
    }
}

void ButtonSliced::onPointerDown()
{
    if (_eState != E_BS_DISABLED)
    {
        setState(E_BS_PRESSED);
    }
}

void ButtonSliced::onPointerUp()
{
    if (_eState != E_BS_DISABLED)
    {
        setState(E_BS_NORMAL);
    }
}

void ButtonSliced::onClick()
{
    if (_eState != E_BS_DISABLED)
    {
        if (_ptrOnOffCont && _eIcon == eButtonSlicedIcon::ONOFF)
            setOnOffState(!_ptrSprOn->isVisible());

        CContainer::onClick();
    }
}

void ButtonSliced::makeSameWidth(std::span<ButtonSlicedPtr> spn, bool bSetSameX)
{
    float fMaxCx = 0;
    float fXToSet = 0;
    for (auto& it:spn)
    {
        float fCx = it->getNotTransCx();
        if (fCx > fMaxCx)
        {
            fMaxCx  = fCx;
            fXToSet = it->getX();
        }
    }
    for (auto& it:spn)
    {
        it->build(fMaxCx, it->getNotTransCy());
        if (bSetSameX)
            it->setX(fXToSet);
    }
}

void ButtonSliced::makeSameWidth(ButtonSlicedPtrs& v, bool bSetSameX)
{
    makeSameWidth(std::span(v), bSetSameX);
}

bool ButtonSliced::getNotTransBounds(Rect* p)
{
    bool bRes = false;
    if (_bgNormal)
        bRes = _bgNormal->getNotTransBounds(p);
    return bRes;
}

//bool ButtonSliced::getTransBounds(Rect* p)
//{
//    bool bRes = false;
//    if (_bgNormal)
//        bRes = _bgNormal->getNotTransBounds(p);
//    return bRes;
//}

eMouseCursorType ButtonSliced::getMouseCursorType()
{
    eMouseCursorType eRes = eMouseCursorType::E_MCT_NORMAL;
    if (_eState != ButtonSliced::E_BS_DISABLED)
        eRes = eMouseCursorType::E_MCT_POINTER;
    return eRes;
}

_G2D_NAMESPACE_END_