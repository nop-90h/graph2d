#include "widgets/button.h"
#include "widgets/nineslice.h"
#include "spriteloader.h"

_G2D_NAMESPACE_BEGIN_

Button::~Button()
{
}

void Button::create(CSpritePtr ptrNormal)
{
    assert(!ptrNormal->getParent());
    _ptrNormal = ptrNormal;
    _root = std::make_shared<CContainer>();
    addChild(_root);
    _root->addChild(_ptrNormal);
    updateState();
}

void Button::create(CSpritePtr ptrNormal, CSpritePtr ptrPressed, CSpritePtr ptrHover, CSpritePtr ptrDisabled, bool bClone)
{
    if (!ptrPressed && bClone)
        ptrPressed = ptrNormal;
    if (!ptrHover && bClone)
        ptrHover = ptrNormal;
    if (!ptrDisabled && bClone)
        ptrDisabled = ptrNormal;

    assert(ptrNormal);
    assert(ptrPressed);
    assert(ptrHover);
    assert(ptrDisabled);

    if (bClone)
    {
        assert(!ptrNormal->getChildrenCount());
        assert(!ptrPressed->getChildrenCount());
        assert(!ptrHover->getChildrenCount());
        assert(!ptrDisabled->getChildrenCount());

        _ptrNormal   = ptrNormal->cloneInitial();
        _ptrPressed  = ptrPressed->cloneInitial();
        _ptrHover    = ptrHover->cloneInitial();
        _ptrDisabled = ptrDisabled->cloneInitial();
    }
    else
    {
        _ptrNormal   = ptrNormal;
        _ptrPressed  = ptrPressed;
        _ptrHover    = ptrHover;
        _ptrDisabled = ptrDisabled;
    }

    removeChild(_root);

    _root = std::make_shared<CContainer>();
    addChild(_root);
    bringChildToBack(_root.get());

    _root->addChild(_ptrNormal);
    _root->addChild(_ptrPressed);
    _root->addChild(_ptrHover);
    _root->addChild(_ptrDisabled);

    _ptrNormal->setVisible(false);
    _ptrPressed->setVisible(false);
    _ptrHover->setVisible(false);
    _ptrDisabled->setVisible(false);

    _twHandle = _ptrHover->getInitialTweenHandle();

    updateState();
}

void Button::setFramedText(LPCTSTR lpszText)
{
    if (!_ptrFramedLabel)
    {
        _ptrFramedLabel = std::make_shared<FramedLabel>();
        _ptrFramedLabel->setScale(0.7f, 0.7f);
        _ptrFramedLabel->setDefFrame();
        _ptrFramedLabel->setText(lpszText, 15, 30);
        _ptrFramedLabel->setTextPadTop(5);
        _ptrFramedLabel->setMinFrameCx(80);
        addChild(_ptrFramedLabel);
    }
    if (lpszText)
    {
        _ptrFramedLabel->setVisible(true);
        _ptrFramedLabel->setText(lpszText, 15, 30);
        _ptrFramedLabel->setTextPadTop(5);
        _ptrFramedLabel->setXPosCentered(_root->calcNotTransCx(), 0);
    }
    else
    {
        _ptrFramedLabel->setVisible(false);
    }
}

void Button::setBgFrame(LPCCTEXT lpccFramePath)
{
    assert(_ptrNormal);
    if (_ptrNormal)
    {
        _ptrBgFrame = SpriteLoader::getInstance()->getSprite(lpccFramePath);
        addChild(_ptrBgFrame);
        swapChildren(_root, _ptrBgFrame);
    }
}

void Button::setIconsPos(float fX, float fY)
{
    if (_ptrNormal)
        _ptrNormal->setPos(fX, fY);
    if (_ptrPressed)
        _ptrPressed->setPos(fX, fY);
    if (_ptrHover)
        _ptrHover->setPos(fX, fY);
    if (_ptrDisabled)
        _ptrDisabled->setPos(fX, fY);
}

void Button::setFgFrame(LPCCTEXT lpccFramePath)
{
    assert(_ptrNormal);
    if (_ptrNormal)
    {
        _ptrFgFrame = SpriteLoader::getInstance()->getSprite(lpccFramePath);
        //_ptrFgFrame->setAlpha(0.6f);
        addChild(_ptrFgFrame);
        //swapChildren(_root, _ptrBgFrame);
    }
}

void Button::setBgFrame(CSpritePtr ptrSpr, float fA, float fB, float fC, float fD)
{
    constexpr float fFrameUp = 24.f;
    assert(_ptrNormal);
    if (_ptrNormal)
    {
        NineSlicePtr ptrFrame = std::make_shared<NineSlice>();
        ptrFrame->createSlices(ptrSpr, fA, fB, fC, fD);
        ptrFrame->build(_ptrNormal->getNotTransCx() + fFrameUp, _ptrNormal->getNotTransCy() + fFrameUp);
        ptrFrame->setPos(-fFrameUp / 2.f, -fFrameUp / 2.f);
        _ptrBgFrame = ptrFrame;
        addChild(_ptrBgFrame);
        swapChildren(_root, _ptrBgFrame);
    }
}

void Button::setBgFrameVisible(bool bIsVisible)
{
    assert(_ptrBgFrame);
    if (_ptrBgFrame)
        _ptrBgFrame->setVisible(bIsVisible);
}

void Button::setTextOffset(float fX, float fY)
{
    assert(_ptrText);
    if (_ptrText)
    {
        _ptrText->setPos(fX, fY);
    }
}

void Button::setTextOffsetY(float fY)
{
    assert(_ptrText);
    if (_ptrText)
    {
        _ptrText->setY(fY);
    }
}

void Button::setTextFont(LPCTSTR lpszFontName, float fontSize)
{
    assert(_ptrText);
    if (_ptrText)
    {
        _ptrText->setFont(lpszFontName);
        _ptrText->setFontSize(fontSize);
        _ptrText->setPosCentered(getNotTransCx(), getNotTransCy());
    }
}

void Button::setTextScale(float fTextScale)
{
    assert(_ptrText);
    if (_ptrText)
    {
        _ptrText->setScale(fTextScale, fTextScale);
        _ptrText->setPosCentered(getNotTransCx(), getNotTransCy());
    }
}

void Button::setTextRgba(uint32_t rgbaHex)
{
    assert(_ptrText);
    if (_ptrText)
    {
        _ptrText->setRgba(rgbaHex);
    }
}

void Button::setText(LPCCTEXT lpccText)
{
    if (!_ptrText)
    {
        _ptrText = std::make_shared<StaticLabel>();
        _ptrText->setShadow(false);
        addChild(_ptrText);
    }
    _ptrText->setText(lpccText);
    _ptrText->setPosCentered(getNotTransCx(), getNotTransCy());
}

void Button::create(const char* lpccNamePrefix)
{
    std::string strNormal("UI/");
    std::string strPressed("UI/");
    std::string strHover("UI/");
    std::string strDisabled("UI/");
    strNormal.append(lpccNamePrefix).append("_normal");
    strPressed.append(lpccNamePrefix).append("_pressed");
    strHover.append(lpccNamePrefix).append("_hover");
    strDisabled.append(lpccNamePrefix).append("_disabled");
    create(SpriteLoader::getInstance()->getSprite(strNormal.c_str()), 
                 SpriteLoader::getInstance()->getSprite(strPressed.c_str()),
                 SpriteLoader::getInstance()->getSprite(strHover.c_str()),
                 SpriteLoader::getInstance()->getSprite(strDisabled.c_str()));
}

void Button::create(LPCCTEXT lpccNormal, LPCCTEXT lpccPressed, LPCCTEXT lpccHover, LPCCTEXT lpccDisabled)
{
    create(SpriteLoader::getInstance()->getSprite(lpccNormal), 
                 SpriteLoader::getInstance()->getSprite(lpccPressed),
                 SpriteLoader::getInstance()->getSprite(lpccHover),
                 SpriteLoader::getInstance()->getSprite(lpccDisabled));
}


void Button::setIcons(CSpritePtr ptrNormal, CSpritePtr ptrPressed, CSpritePtr ptrHover, CSpritePtr ptrDisabled)
{
    CSpritePtr ptrNormalIcon =  ptrNormal->cloneInitial();
    ptrNormalIcon->setPos((_ptrNormal->getNotTransCx() - ptrNormalIcon->getNotTransCx()) / 2.f,
                          (_ptrNormal->getNotTransCy() - ptrNormalIcon->getNotTransCy()) / 2.f);
    _ptrNormal->addChild(ptrNormalIcon);
    CSpritePtr ptrPressedIcon =  ptrPressed->cloneInitial();
    ptrPressedIcon->setPos((_ptrPressed->getNotTransCx() - ptrPressedIcon->getNotTransCx()) / 2.f,
                           (_ptrPressed->getNotTransCy() - ptrPressedIcon->getNotTransCy()) / 2.f);
    _ptrPressed->addChild(ptrPressedIcon);
    CSpritePtr ptrHoverIcon =  ptrHover->cloneInitial();
    ptrHoverIcon->setPos((_ptrHover->getNotTransCx() - ptrHoverIcon->getNotTransCx()) / 2.f,
                         (_ptrHover->getNotTransCy() - ptrHoverIcon->getNotTransCy()) / 2.f);
    _ptrHover->addChild(ptrHoverIcon);
    CSpritePtr ptrDisabledIcon =  ptrDisabled->cloneInitial();
    ptrDisabledIcon->setPos((_ptrDisabled->getNotTransCx() - ptrDisabledIcon->getNotTransCx()) / 2.f,
                            (_ptrDisabled->getNotTransCy() - ptrDisabledIcon->getNotTransCy()) / 2.f);
    _ptrDisabled->addChild(ptrDisabledIcon);    
}

void Button::setIconGrayScale(bool bGrayScale, float fAlpha)
{
    if (_ptrNormal)
        _ptrNormal->removeAll();
    if (_ptrPressed)
        _ptrPressed->removeAll();
    if (_ptrHover)
        _ptrHover->removeAll();
    if (_ptrDisabled)
        _ptrDisabled->removeAll();

    if (bGrayScale)
    {
        if (_ptrNormal)
        {
            auto ptrGS = _ptrNormal->cloneInitial();
            ptrGS->setGrayScale();
            ptrGS->setAlpha(fAlpha);
            _ptrNormal->addChild(ptrGS);
        }
        if (_ptrPressed)
        {
            auto ptrGS = _ptrPressed->cloneInitial();
            ptrGS->setGrayScale();
            ptrGS->setAlpha(fAlpha);
            _ptrPressed->addChild(ptrGS);
        }
        if (_ptrHover)
        {
            auto ptrGS = _ptrHover->cloneInitial();
            ptrGS->setGrayScale();
            ptrGS->setAlpha(fAlpha);
            _ptrHover->addChild(ptrGS);
        }
        if (_ptrDisabled)
        {
            auto ptrGS = _ptrDisabled->cloneInitial();
            ptrGS->setGrayScale();
            ptrGS->setAlpha(fAlpha);
            _ptrDisabled->addChild(ptrGS);
        }
    }
}

void Button::setState(eButtonState eNewState)
{
    if (eNewState != _eState)
    {
        _eState = eNewState;
        updateState();
    }
}

void Button::setEnabled(bool bEnabled)
{   
    if (!bEnabled)
        setState(E_BS_DISABLED);
    else if(!isEnabled())
        setState(E_BS_NORMAL);
}

void Button::updateState()
{
    switch (_eState)
    {
        case E_BS_NORMAL:
        {
            if (!_bHoverAnim || !_bShowHoverAnim)
            {
                _ptrNormal->setVisible(true);
                if (_ptrPressed)
                    _ptrPressed->setVisible(false);
                if (_ptrHover)
                    _ptrHover->setVisible(false);
                if (_ptrDisabled)
                    _ptrDisabled->setVisible(false);
            }
            else if (_ptrHover)
            {
                _ptrHover->removeSelfTweens();
                _ptrHover->setAlpha(1.f);

                _ptrNormal->setVisible(true);
                _ptrHover->setVisible(true);
                if (_ptrPressed)
                    _ptrPressed->setVisible(false);
                if (_ptrDisabled)
                    _ptrDisabled->setVisible(false);
                if (_twHandle)
                    _ptrHover->removeSelfTween(_twHandle);
                _ptrHover->addSelfTween(&_twHandle, eTweenProp::ALPHA, 1.f, 0.1f, 0.15f);
            }
        }
        break;

        case E_BS_HOVER:
        {
            if (_bShowHoverAnim && _ptrHover)
            {
                if (!_bHoverAnim)
                {
                    _ptrNormal->setVisible(false);
                    if (_ptrPressed)
                        _ptrPressed->setVisible(false);
                    _ptrHover->setVisible(true);
                    if (_ptrDisabled)
                        _ptrDisabled->setVisible(false);

                }
                else
                {
                    _ptrHover->removeSelfTweens();
                    _ptrHover->setAlpha(0.1f);
                    _ptrNormal->setVisible(true);
                    if (_ptrPressed)
                        _ptrPressed->setVisible(false);
                    if (_ptrDisabled)
                        _ptrDisabled->setVisible(false);
                    _ptrHover->setVisible(true);
                    if (_twHandle)
                        _ptrHover->removeSelfTween(_twHandle);
                    _ptrHover->addSelfTween(&_twHandle, eTweenProp::ALPHA, 0.1f, 1.f, 0.15f);
                }
            }
            else
            {
                _ptrNormal->setVisible(true);
            }
        }
        break;

        case E_BS_PRESSED:
        {
            if (_bHoverAnim && _ptrHover && _twHandle)
                _ptrHover->removeSelfTween(_twHandle);
            if (_ptrPressed)
            {
                _ptrNormal->setVisible(false);
                if (_ptrPressed)
                    _ptrPressed->setVisible(true);
                if (_ptrHover)
                    _ptrHover->setVisible(false);
                _ptrDisabled->setVisible(false);
            }
            else
            {
                _ptrNormal->setVisible(true);
            }
        }
        break;

        case E_BS_DISABLED:
        {
            if (_bHoverAnim && _ptrHover && _twHandle)
                _ptrHover->removeSelfTween(_twHandle);
            if (_ptrDisabled)
            {
                _ptrNormal->setVisible(false);
                if (_ptrPressed)
                    _ptrPressed->setVisible(false);
                if (_ptrHover)
                    _ptrHover->setVisible(false);
                _ptrDisabled->setVisible(true);
            }
            else
            {
                _ptrNormal->setVisible(true);
            }
        }
        break;

        default:
        {
            assert(false);
        }
        break;
    }
}

void Button::onHover()
{
    if (_eState != E_BS_DISABLED)
    {
        setState(E_BS_HOVER);
    }
}

void Button::onLeave()
{
    if (_eState != E_BS_DISABLED)
    {
        setState(E_BS_NORMAL);
    }
}

void Button::onPointerDown()
{
    if (_eState != E_BS_DISABLED)
    {
        setState(E_BS_PRESSED);
    }
}

void Button::onPointerUp()
{
    if (_eState != E_BS_DISABLED)
    {
        setState(E_BS_NORMAL);
    }
}

void Button::onClick()
{
    if (_eState != E_BS_DISABLED)
    {
        CContainer::onClick();
    }
}

bool Button::getNotTransBounds(Rect* p)
{
    bool bRes = false;
    if (_ptrBgFrame)
        bRes = _ptrBgFrame->getNotTransBounds(p);
    else if (_ptrNormal)
        bRes = _ptrNormal->getNotTransBounds(p);
    return bRes;
}

eMouseCursorType Button::getMouseCursorType()
{
    eMouseCursorType eRes = eMouseCursorType::E_MCT_NORMAL;
    if (_eState != Button::E_BS_DISABLED)
        eRes = eMouseCursorType::E_MCT_POINTER;
    return eRes;
}

void Button::setRgbas(uint32_t rgbNormal, uint32_t rgbPressed, uint32_t rgbHover, uint32_t rgbDisabled)
{
    _ptrNormal->setRgba(rgbNormal);
    if (_ptrPressed)
        _ptrPressed->setRgba(rgbPressed);
    if (_ptrHover)
        _ptrHover->setRgba(rgbHover);
    if (_ptrDisabled)
        _ptrDisabled->setRgba(rgbDisabled);
}

void Button::showLockIcon(bool bShow)
{
    static auto pLock = SpriteLoader::getInstance()->getSprite("UI/iconLock");
    if (bShow)
    {
        if (!_ptrIconLock)
        {
            _ptrIconLock = pLock->cloneInitial();
            _ptrIconLock->setScale(.5f, .5f);
            addChild(_ptrIconLock);
            _ptrIconLock->setXPosCentered(getNotTransCx());
            _ptrIconLock->setY(getNotTransCy() - 30.f );
        }
        _ptrIconLock->setVisible(true);
    }
    else if (_ptrIconLock)
    {
        _ptrIconLock->setVisible(false);
    }
}

_G2D_NAMESPACE_END_