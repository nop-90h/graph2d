#include "widgets/iconedlabel.h"
#include "spriteloader.h"

_G2D_NAMESPACE_BEGIN_

IconedLabel::IconedLabel(CContainerPtr ptrIcon, LPCTSTR lpszText, std::optional<float> fIconScale, std::optional<float> fFontSize, std::optional<std::string> sFontName)
{
    _ptrIcon    = ptrIcon;
    _fIconScale = fIconScale;
    _ptrLabel = std::make_shared<StaticLabel>();
    if (fFontSize.has_value())
        _ptrLabel->setFontSize(fFontSize.value());
    if (sFontName.has_value())
        _ptrLabel->setFont(sFontName.value().c_str());
    addChild(_ptrLabel);
    addChild(_ptrIcon);
    if (lpszText)
        setText(lpszText);
    _ptrIcon->setSaveTransCoords();
    _ptrLabel->setSaveTransCoords();
}

void IconedLabel::updatePositions()
{
    _ptrIcon->setX(0);
    if (_ptrIcon->getParent())
        _ptrIcon->removeFromParent();
    if (_ptrLabel->getParent())
        _ptrLabel->removeFromParent();

    _ptrIcon->setScale(1.f, 1.f);

    if (_fIconScale)
    {
        _ptrIcon->setScale(*_fIconScale, *_fIconScale);
        _ptrLabel->setYPosCentered(_ptrIcon->getNotTransCy() * _ptrIcon->getScaleY());

    }
    else
    {
        _ptrIcon->scaleToFit(_ptrIcon->getNotTransCx(), _ptrLabel->getNotTransCy());
        _ptrIcon->setYPosCentered(_ptrLabel->getNotTransCy());
    }
    addChild(_ptrIcon);
    addChild(_ptrLabel);
    _ptrLabel->setX((_ptrIcon->calcNotTransCx() * _ptrIcon->getScaleX()) + 5);
}

void IconedLabel::setIcon(CContainerPtr ptrIcon)
{
    _ptrIcon->removeFromParent();
    _ptrIcon = ptrIcon;
    updatePositions();
}

void IconedLabel::setText(LPCTSTR lpszText)
{
    _ptrLabel->setText(lpszText);
    updatePositions();
}

void IconedLabel::setTextBox(bool bSet, float fBoxCx)
{
    if (bSet) 
    {
        _ptrLabel->setAlign(eTRAlign::TR_ALIGN_CENTER);
    }
    else 
    {
        _ptrLabel->setAlign(eTRAlign::TR_ALIGN_LEFT);
    }
}

bool IconedLabel::getNotTransBounds(Rect* p)
{
    _ptrLabel->getNotTransBounds(p);
    p->offset(_ptrLabel->getX(), _ptrLabel->getY());
    Rect rcIcon;
    _ptrIcon->calcNotTransBounds(&rcIcon);
    rcIcon.scale(_ptrIcon->getScaleX(), _ptrIcon->getScaleY());
    p->unite(&rcIcon);
    return true;
}

_G2D_NAMESPACE_END_