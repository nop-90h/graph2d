#include "widgets/framedlabel.h"
#include "spriteloader.h"

_G2D_NAMESPACE_BEGIN_

FramedLabel::FramedLabel()
{
    _label = std::make_shared<StaticLabel>();
    _frame = std::make_shared<NineSlice>();
    addChild(_frame);
    addChild(_label);
}

void FramedLabel::setText(const char* lpccLabelText, float fPadX, float fPadY)
{
    _label->setText(lpccLabelText);
    _fPadX = fPadX;
    _fPadY = fPadY;
    updateFrame();
    _bIsTextSet = true;
}

void FramedLabel::setTextFmt(const char* lpccLabelText, float fPadX, float fPadY, ...)
{    
    va_list args;
    va_start(args, fPadY);
    _label->setTextFmtV(lpccLabelText, args);
    va_end(args);
    _fPadX = fPadX;
    _fPadY = fPadY;
    updateFrame();
    _bIsTextSet = true;
}

void FramedLabel::setFrame(NineSlicePtr pFrameProto)
{
    if (_frame)
        _frame->removeFromParent();
    _frame = pFrameProto->cloneInitial();
    addChild(_frame);
    bringChildToBack(_frame.get());
    if (_bIsTextSet)
        updateFrame();
}

void FramedLabel::setFrame(CSpritePtr pFrame, float fA, float fB, float fC, float fD)
{
    _frame->createSlices(pFrame, fA, fB, fC, fD);
    if (_bIsTextSet)
        updateFrame();
}

void FramedLabel::setTextBox(bool bSet, float fBoxCx)
{
    _bBoxed = bSet;
    if (bSet) 
    {
        _label->setAlign(eTRAlign::TR_ALIGN_CENTER);
        _label->setBoxMode(eTextRenderType::BOX, fBoxCx); 
    }
    else 
    {
        _label->setAlign(eTRAlign::TR_ALIGN_LEFT);
        _label->setBoxMode();
    }
    if (_bIsTextSet)
        updateFrame();
}

void FramedLabel::setDefFrame(void)
{
    setFrame(SpriteLoader::getInstance()->getSprite("UI/frameNums"), 12, 12, 20, 20);
}

void FramedLabel::setTextLeftPad(float fLeftPad, bool bUpdate)
{
    _fTextLeftPad = fLeftPad;
    if (_bIsTextSet && bUpdate)
        updateFrame();
}

void FramedLabel::setTextPadTop(float fTopPad, bool bUpdate)
{
    _fTextTopPad = fTopPad;
    if (_bIsTextSet && bUpdate)
        updateFrame();
}

void FramedLabel::setControl(eControlPos ePos, CContainerPtr ptrControl, float fControlsPad)
{
    auto &controlInfo = _controls[(int)ePos];
    if (controlInfo._p)
        controlInfo._p->removeFromParent();

    controlInfo._p    = ptrControl;
    controlInfo._fPad = fControlsPad;

    if (_bIsTextSet)
        updateFrame();
}

void FramedLabel::removeControl(eControlPos ePos)
{
    auto &c = _controls[(int)ePos];
    if (c._p)
    {
        c._p->removeFromParent();
        c._p = nullptr;
        if (_bIsTextSet)
            updateFrame();
    }   
}

void FramedLabel::removeControls(void)
{
    bool bChanged = false;
    for (int i = 0; i < SIZE_OF(_controls); i++)
    {
        auto& c = _controls[i];
        if (c._p)
        {
            c._p->removeFromParent();
            c._p     = nullptr;
            bChanged = true;
        }
    }
    if (bChanged && _bIsTextSet)
        updateFrame();
}

bool FramedLabel::getNotTransBounds(Rect* p)
{
    return _frame->getNotTransBounds(p);
}

void FramedLabel::updateFrame()
{
    Rect rc;
    if (_bBoxed)
        _label->getNotTransBounds(&rc);
    else
    {
        rc.cx = _label->getTextWidth();
        rc.cy = _label->getLineHeight();
    }

    float fControlsPad = 0;
    Rect rcControls[SIZE_OF_T <decltype(_controls)> ];
    for (int i = 0; i < SIZE_OF(rcControls); i++)
    {
        auto& c = _controls[i];
        if (c._p)
        {
            c._p->calcNotTransBounds(&rcControls[i]);
            c._p->removeFromParent();
        }
    }
    
    float fLeftControlCx    = rcControls[(int)eControlPos::LEFT].cx;
    float fRightControlCx   = rcControls[(int)eControlPos::RIGHT].cx;
    float fLeftControlCy    = rcControls[(int)eControlPos::LEFT].cy;
    float fRightControlCy   = rcControls[(int)eControlPos::RIGHT].cy;
    float fFrameCx          = std::max(rc.cx, (float)_nMinFrameCx);
    float fFrameCy          = std::max(rc.cy, (float)_nMinFrameCy);
    float fControlsPadLeft  = _controls[(int)eControlPos::LEFT].getPad();
    float fControlsPadRight = _controls[(int)eControlPos::RIGHT].getPad();

    fFrameCx = std::max(fLeftControlCx, fFrameCx);
    fFrameCy = std::max(fLeftControlCy, fFrameCy);
    fFrameCy = std::max(fRightControlCy, fFrameCy);

    float fBoundignFrameCx = fFrameCx + _fPadX + _fTextLeftPad + fControlsPadLeft + fControlsPadRight + fRightControlCx + fLeftControlCx;
    float fBoundignFrameCy = fFrameCy + _fPadY;
    _frame->build(fBoundignFrameCx, fBoundignFrameCy);
    if (!_bBoxed)
    {
        //_label->setAlign(eTRAlign::TR_ALIGN_MIDDLE | eTRAlign::TR_ALIGN_CENTER);
        //_label->setTextOffs(fBoundignFrameCx * 0.5f, fBoundignFrameCy * 0.5f);
        _label->setPosCentered(fFrameCx + _fPadX, fFrameCy + _fPadY, fControlsPadLeft + fLeftControlCx);
        _label->setPos(_label->getX() + _fTextLeftPad, _label->getY() + _fTextTopPad);
    }
    else
    {
        _label->setPosCentered(fFrameCx + _fPadX, fFrameCy + _fPadY, fControlsPadLeft + fLeftControlCx);
        _label->setPos(_label->getX() + _fTextLeftPad, _label->getY() + _fTextTopPad);
    }
    for (int i = 0; i < SIZE_OF(_controls); i++)
    {
        auto& c = _controls[i];
        if (c._p && c._p->isVisible())
        {
            switch ((eControlPos)i)
            {
                case eControlPos::LEFT:
                {
                    c._p->setYPosCentered(fFrameCy + _fPadY);
                    c._p->setX(fControlsPadLeft);
                }
                break;
                case eControlPos::RIGHT:
                {
                    c._p->setYPosCentered(fFrameCy + _fPadY);
                    c._p->setX(fFrameCx + _fPadX + _fTextLeftPad + fControlsPadLeft + fLeftControlCx);
        }
                break;
                default:
                {
                    assert(false);
                }
                break;
            }
            addChild(c._p);
        }
    }
}

_G2D_NAMESPACE_END_