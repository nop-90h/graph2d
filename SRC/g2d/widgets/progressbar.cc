#include "widgets/progressbar.h"
#include "spriteloader.h"

_G2D_NAMESPACE_BEGIN_

void ProgressBar::create(float fFrameCx, float fBarCx, CSpritePtr ptrFrame, CSpritePtr ptrBar, float fFrameC, float fFrameD, float fBarC, float fBarD, float fBarOffsX, float fBarOffsY)
{
    _ptrSprBar = ptrBar;
    _ptrFrame  = std::make_shared<ThreeSliceHor>();
    _ptrFrame->createSlices(ptrFrame, fFrameC, fFrameD);
    _ptrFrame->build(fFrameCx);
    _ptrBar = std::make_shared<ThreeSliceHor>();
    _ptrBar->createSlices(_ptrSprBar, fBarC, fBarD);
    _ptrBar->build(_fMinBarSize);
    _ptrBar->setPos(fBarOffsX, fBarOffsY);
    _fcx    = fFrameCx;
    _fBarCx = fBarCx;
    _fBarC  = fBarC;
    _fBarD  = fBarD;
    _ptrFrame->addChild(_ptrBar);
    addChild(_ptrFrame);
}

void ProgressBar::create(float fFrameCx, float fBarCx, LPCCTEXT lpccFrame, LPCCTEXT lpccBar, float fFrameC, float fFrameD, float fBarC, float fBarD, float fBarOffsX, float fBarOffsY)
{
    _ptrSprBar = SpriteLoader::getInstance()->getSprite(lpccBar);
    _ptrFrame  = std::make_shared<ThreeSliceHor>();
    _ptrFrame->createSlices(SpriteLoader::getInstance()->getSprite(lpccFrame), fFrameC, fFrameD);
    _ptrFrame->build(fFrameCx);
    _ptrBar = std::make_shared<ThreeSliceHor>();
    _ptrBar->createSlices(_ptrSprBar, fBarC, fBarD);
    _ptrBar->build(_fMinBarSize);
    _ptrBar->setPos(fBarOffsX, fBarOffsY);
    _fcx    = fFrameCx;
    _fBarCx = fBarCx;
    _fBarC  = fBarC;
    _fBarD  = fBarD;
    _ptrFrame->addChild(_ptrBar);
    addChild(_ptrFrame);
}

void ProgressBar::setProgress(float fProgress)
{
    _fProgress = std::min(std::max(fProgress, 0.f), 1.f);
    float cx = (_fBarCx - _fMinBarSize) * _fProgress;
    _ptrBar->build(_fMinBarSize + cx);
}

bool ProgressBar::getNotTransBounds(Rect* p)
{
    return _ptrFrame->getNotTransBounds(p);
}

bool ProgressBar::setTweenPropValue(eTweenProp eProp, float fValue)
{
    bool bRes = false;
    if (eProp == eTweenProp::PROGRESS)
    {
        bRes = true;
        setProgress(fValue);
    }
    else
    {
        bRes = Widget::setTweenPropValue(eProp, fValue);
    }
    return bRes;
}

_G2D_NAMESPACE_END_