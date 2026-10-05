#include "widgets/kvlist.h"

_G2D_NAMESPACE_BEGIN_

KVList::KVList()
{
    _ptrKeys  = std::make_shared<CContainer>();
    _ptrVals  = std::make_shared<CContainer>();
    _ptrIcons = std::make_shared<CContainer>();

    this->addChild(_ptrKeys);
    this->addChild(_ptrVals);
    this->addChild(_ptrIcons);
}

void KVList::clear()
{
    _ptrIcons->removeAll();
    _ptrKeys->removeAll();
    _ptrVals->removeAll();
    _v.clear();
}

void KVList::build()
{
    float fYKeyPos  = 0;
    float fYValPos  = 0;
    float fYIconPos = 0;

    float fValsCx  = 0;
    float fKeysCx  = 0;
    float fIconsCx = 0;
    std::vector<float> vCxs;
    vCxs.reserve(_v.size());
    for (auto& it:_v)
    {
        if (it.ptrIcon)
        {
            _ptrIcons->addChild(it.ptrIcon);
            it.ptrIcon->scaleToFitCy(_params.fIconHeight);
            float fh = (it.ptrIcon->calcNotTransCy() * it.ptrIcon->getScaleY());
            float fy = fYIconPos + (_params.fLineHeight - fh) * 0.5f;
            it.ptrIcon->setY(fy);
            fIconsCx = std::max(fIconsCx, it.ptrIcon->calcNotTransCx() * it.ptrIcon->getScaleX());
        }
        fYIconPos += _params.fLineHeight;

        it.ptrKey->scaleToFitCy(_params.fIconHeight);
        it.ptrKey->setY(fYKeyPos + (_params.fLineHeight - it.ptrKey->calcNotTransCy() * it.ptrKey->getScaleY()) * 0.5f);
        fKeysCx = std::max(fKeysCx, it.ptrKey->calcNotTransCx() * it.ptrKey->getScaleX());
        _ptrKeys->addChild(it.ptrKey);
    
        if (it.ptrVal)
        {
            it.ptrVal->scaleToFitCy(_params.fIconHeight);
            it.ptrVal->setY(fYValPos + (_params.fLineHeight - it.ptrVal->calcNotTransCy() * it.ptrVal->getScaleY()) * 0.5f);
            float fCx = it.ptrVal->calcNotTransCx() * it.ptrVal->getScaleX();
            vCxs.push_back(fCx);
            fValsCx = std::max(fValsCx, fCx);
            _ptrVals->addChild(it.ptrVal);
        }

        fYKeyPos += _params.fLineHeight;
        fYValPos += _params.fLineHeight;
    }

    for (auto it:*_ptrIcons)
    {
        it->setXPosCentered(fIconsCx);
    }
    for (int i = 0; i < _ptrVals->getChildrenCount(); i++)
    {
        float fCx = vCxs[i];
        _ptrVals->getChildAt(i)->setX(fValsCx - fCx);
    }
    _ptrKeys->setX(_params.fColumnMargin + fIconsCx);
    _ptrVals->setX(_params.fColumnMargin * 2.f + fKeysCx + fIconsCx);
}


void KVList::addItem(CContainerPtr icon, LPCTSTR lpszKey, LPCTSTR lpszVal)
{
    StaticLabelPtr ptrIL = std::make_shared<StaticLabel>();
    ptrIL->setText(lpszKey);
    StaticLabelPtr ptrSL = std::make_shared<StaticLabel>();
    ptrSL->setText(lpszVal);
    addItem(icon, ptrIL, ptrSL);
}

_G2D_NAMESPACE_END_