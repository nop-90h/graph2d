#include "g2d.h"
#include "paneltabs.h"
#include "spriteloader.h"
#include "widgets/threeslice.h"

_G2D_NAMESPACE_BEGIN_

void PanelTabs::init(TabInfo_t* pTabs, size_t nCount, OnTabSelectCb cb, BaseDialog* pParent, TabFont& tabFont)
{
    if (BaseDialog::init(0, 0, E_ST_NONE, NULL, nullptr, pParent))
    {
        _bDispatchOutOfClientRect = true;
        _cb = cb;
        std::vector<StaticLabelPtr> labels;
        labels.reserve(nCount);
        float fMaxCx = 0;
        for (int i = 0; i < nCount; i++)
        {
            StaticLabelPtr ptrLabel = std::make_shared<StaticLabel>();
            if (tabFont.bIsHtml)
            {
                ptrLabel->setBoxMode(eTextRenderType::HTML_BOX, 1024);
                ptrLabel->setText(pTabs[i].lpccCaption);
            }
            else
            {
                ptrLabel->setFont(tabFont.lpszFontName);
                ptrLabel->setFontSize(tabFont.fFontSize);
                ptrLabel->setText(pTabs[i].lpccCaption);
            }
            labels.push_back(ptrLabel);
            fMaxCx = std::max(ptrLabel->calcCx(), fMaxCx);
        }
        float fY = 0;
        bool bHasOptionalId = false;
        bool bHasIndexes    = false;
        for (int i = 0; i < nCount; i++)
        {
            float fCy = addTab(labels[i], fMaxCx, pTabs[i].fA, pTabs[i].fB, pTabs[i].lpccActive, pTabs[i].lpccInactive);            
            CContainerPtr ptrTab = _root->getChildAt(i);
            ptrTab->setY(fY);
            if (!bHasOptionalId)
                bHasOptionalId = pTabs[i].tabId.has_value();
            if (!bHasIndexes)
                bHasIndexes = !pTabs[i].tabId.has_value();

            assert(bHasOptionalId != bHasIndexes); //either indexes or ids!

            ptrTab->setInteractive();
            ptrTab->setId(pTabs[i].tabId.has_value() ?  pTabs[i].tabId.value() + 1 : i + 1);
            _root->getChildAt(i)->setOnClick([this, ptrTab](){
                setSelected(ptrTab, true);
            });
            fY += fCy;    
        }
        _fDialogCy = fY;
        setSelected(_root->getChildAt(0), false);
    }
}

void PanelTabs::onShow(bool bShow)
{
    if (bShow)
    {
        constexpr float fAnimTime = 1.25f;
        getParent()->bringChildToBack(this);
        for (int i = 0; i < _root->getChildrenCount(); i++)
        {
            auto& it = _root->getChildren()[i];
            it->setX(it->calcNotTransCx());
            it->addSelfTween(eTweenProp::X, it->getX(), 65.f, fAnimTime + (fAnimTime * 0.1f * i), Easing::outBack, 0, [this, i]{
                auto& it = _root->getChildren()[i];
                it->addSelfTween(eTweenProp::X, it->getX(), 0.f, 0.1f, Easing::linear, 0, [this, i]{
                    if (i == _root->getChildrenCount() - 1)
                        bringAboveParent();
                });
            });
        }
    }
}

float PanelTabs::addTab(StaticLabelPtr pLabel, float cx, float fA, float fB, LPCCTEXT lpccActive, LPCCTEXT lpccInactive)
{
    assert(lpccActive);
    assert(lpccInactive);
    RenderTracker rt;
    CContainerPtr ptrTab = std::make_shared<CContainer>();
    ptrTab->setInteractive(true);
    ThreeSliceHorPtr ptrActive = std::make_shared<ThreeSliceHor>();
    ptrActive->createSlices(SpriteLoader::getInstance()->getSprite(lpccActive), fA, fB);
    ptrActive->build(cx + 85);
    ThreeSliceHorPtr ptrInactive = std::make_shared<ThreeSliceHor>();
    CSpritePtr ptrSprInactive = SpriteLoader::getInstance()->getSprite(lpccInactive);
    ptrInactive->createSlices(ptrSprInactive, 37, 1);
    ptrInactive->build(cx + 60);

    ptrTab->addChild(ptrActive);
    ptrActive->setVisible(false);
    ptrTab->addChild(ptrInactive);
    ptrTab->addChild(pLabel);
    pLabel->setPosCentered(cx + 85, ptrActive->getNotTransCy());
    _root->addChild(ptrTab);

    _fDialogCx = std::max(cx + 70, _fDialogCx);

    return ptrSprInactive->getNotTransCy();
}

void PanelTabs::setSelected(CContainerPtr ptrSelected, bool bCallCb)
{
    if (_ptrSelected != ptrSelected)
    {
        if (_ptrSelected)
        {
            _ptrSelected->getChildAt(0)->setVisible(false);
            _ptrSelected->getChildAt(1)->setVisible(true);
        }
        _ptrSelected = ptrSelected;
        if (_ptrSelected)
        {
            _ptrSelected->getChildAt(0)->setVisible(true);
            _ptrSelected->getChildAt(1)->setVisible(false);
            if (bCallCb)
            {
                assert(_cb);
                if (_cb)
                    _cb(_ptrSelected->getId() - 1);
            }
        }
    }
}

void PanelTabs::updateScaleAndPos()
{
}

_G2D_NAMESPACE_END_
