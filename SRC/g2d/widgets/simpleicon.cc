#include "widgets/simpleicon.h"
#include "sprite.h"
#include "spriteloader.h"
#include "utilfuncs.h"

_G2D_NAMESPACE_BEGIN_

bool SimpleIcon::getNotTransBounds(Rect* p)
{
    _ptrIcon->calcNotTransBounds(p);
    return true;
}

void SimpleIcon::create(CSpritePtr ptrIcon)
{
    float  normal = 0xDF / 255.f;
    float  hl     = 0xFF / 255.f;
    assert(!_root);
    if (!_root)
    {
        _root    = std::make_shared<CContainer>();
        _ptrIcon = ptrIcon;
        _root->addChild(_ptrIcon);
        addChild(_root);
        _root->setRgba(makeRGBA(normal, normal, normal, 0xFF));
        
        setOnHover([this, normal, hl]{
            if (_bEnabled)
            {
                _root->removeSelfTweens();
                _root->setRgba(makeRGBA(normal, normal, normal, 0xFF));
                _root->addSelfTween(eTweenProp::BLACKEN, normal, hl, 0.1f);
            }
        });

        setOnLeave([this, normal, hl]{
            if (_bEnabled)
            {
                _root->removeSelfTweens();
                _root->setTint(hl, hl, hl);
                _root->addSelfTween(eTweenProp::BLACKEN, hl, normal, 0.1f);
            }
        });
    }
}

void SimpleIcon::setEnabled(bool bEnabled)
{
    if (bEnabled != _bEnabled)
    {
        _bEnabled = bEnabled;
        if (!bEnabled)
        {
            uint8_t normal = 0xDF;
            _root->removeSelfTweens();
            _root->setRgba(makeRGBA(normal, normal, normal, 0xFF));
        }
    }
}

_G2D_NAMESPACE_END_