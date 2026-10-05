#pragma once

#include "g2d.h"
#include "widgets/widget.h"
#include "widgets/threeslice.h"
#include "widgets/staticlabel.h"
#include "widgets/iconedlabel.h"
#include "spriteloader.h"

_G2D_NAMESPACE_BEGIN_

class TitleBar : public Widget
{
private:
    std::string      _strTitle;
    CContainerPtr    _titleCont;
    ThreeSliceHorPtr _titleFrame;
    StaticLabelPtr   _ptrTitleLabel;
public:

    void setTitle(const char* lpccTitle, CSpritePtr threeSliceSpr, float fA, float fB, float fFontSz = 40)
    {
        if (_strTitle != lpccTitle)
        {
            if (_titleCont)
            {
                removeAll();
            }

            _ptrTitleLabel = StaticLabelPtr(new StaticLabel());
            _ptrTitleLabel->setFontSize(fFontSz);
            
            _ptrTitleLabel->setText(lpccTitle);

            float cxText = _ptrTitleLabel->calcCx();

            _titleFrame = ThreeSliceHorPtr(new ThreeSliceHor());
            _titleFrame->createSlices(threeSliceSpr, fA, fB);
            float fRoomForTextCx = cxText + fA + fB;

            _titleFrame->build(fRoomForTextCx);
            _titleCont = CContainerPtr(new CContainer());
            _titleCont->addChild(_titleFrame);
            _titleCont->addChild(_ptrTitleLabel);        
            _ptrTitleLabel->setAlign(eTRAlign::TR_ALIGN_CENTER | eTRAlign::TR_ALIGN_MIDDLE);
            _ptrTitleLabel->setTextOffs(_titleFrame->getNotTransCx() * 0.5f, _titleFrame->getNotTransCy() * 0.5f);
            addChild(_titleCont);
        }
    }

    void setTitle(CContainerPtr ptrTitle, CSpritePtr threeSliceSpr, float fA, float fB)
    {
        _strTitle.clear();
        if (_titleCont)
        {
            removeAll();
        }

        float cxText = ptrTitle->calcNotTransCx();

        _titleFrame = ThreeSliceHorPtr(new ThreeSliceHor());
        _titleFrame->createSlices(threeSliceSpr, fA, fB);
        float fRoomForTextCx = cxText + fA + fB;

        _titleFrame->build(fRoomForTextCx);
        _titleCont = CContainerPtr(new CContainer());
        _titleCont->addChild(_titleFrame);
        _titleCont->addChild(ptrTitle);        
        ptrTitle->setPosCentered(_titleFrame->getNotTransCx(), _titleFrame->getNotTransCy());
        addChild(_titleCont);
    }

    void setTitle(bool bLocked, LPCTSTR lpszTitle, CSpritePtr threeSliceSpr, float fA, float fB, float fFontSz = 40)
    {
        if (bLocked)
        {
            auto ptrCont  = std::make_shared<CContainer>();
            auto ptrLabel = std::make_shared<StaticLabel>();
            ptrLabel->setFontSize(fFontSz);
            ptrLabel->setText(lpszTitle);
            auto ptrLock = SpriteLoader::getInstance()->getSprite("UI/iconLock");
            ptrLock->setScale(0.7f, 0.7f);
            ptrCont->addChild(ptrLock);
            ptrCont->addChild(ptrLabel);
            ptrLabel->setX(ptrLock->getCx() + 10.f);
            ptrLabel->setYPosCentered(ptrLock->calcNotTransCy() * ptrLock->getScaleY());

            setTitle(ptrCont, threeSliceSpr, fA, fB);
        }
        else
        {
            setTitle(lpszTitle, threeSliceSpr, fA, fB, fFontSz);
        }
    }
};

typedef std::shared_ptr<TitleBar> TitleBarPtr;

_G2D_NAMESPACE_END_