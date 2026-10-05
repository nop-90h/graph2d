#pragma once

#include "g2d.h"
#include "widgets/widget.h"
#include "spriteloader.h"

_G2D_NAMESPACE_BEGIN_

class SimpleIcon : public Widget
{
public:
    inline static auto makeInst(LPCTSTR lpszSpritePath)
    {
        auto ptrRes = std::make_shared<SimpleIcon>();
        ptrRes->create(SpriteLoader::getInstance()->getSprite(lpszSpritePath));
        return ptrRes;
    }
            void                create              (CSpritePtr         ptrIcon);
    virtual bool                getNotTransBounds   (Rect*              p) override;
            void                setEnabled          (bool               bEnabled);
            bool                isEnabled           (void) { return _bEnabled; }

private:
    CContainerPtr       _root;
    CContainerPtr       _ptrIcon;
    bool                _bEnabled = true;
};

typedef std::shared_ptr<SimpleIcon> SimpleIconPtr;

_G2D_NAMESPACE_END_