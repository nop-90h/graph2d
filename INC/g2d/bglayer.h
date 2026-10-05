#pragma once

#include "g2d.h"
#include "container.h"
#include <eventemmiter.h>
#include "gfx.h"
#include "widgets/staticlabel.h"

_G2D_NAMESPACE_BEGIN_

class BgLayer;

typedef std::shared_ptr<BgLayer> BgLayerPtr;

class BgLayer : public IEventListener,
                public CContainer
{
public:
    static BgLayerPtr getInstance(eRenderLayer eLayer = eRenderLayer::BACKGORUND)
    {
        switch (eLayer)
        {
            case eRenderLayer::BACKGORUND:
            {
                static BgLayerPtr ptrBg = std::make_shared<BgLayer>();
                return ptrBg;
            }
            break;
            case eRenderLayer::FOREGROUND:
            {
                static BgLayerPtr ptrFg = std::make_shared<BgLayer>();
                return ptrFg;
            }
            break;
            case eRenderLayer::FOREGROUND_EFFECTS:
            {
                static BgLayerPtr ptrFg = std::make_shared<BgLayer>();
                return ptrFg;
            }
            break;
            default:assert(false);
        }

        return nullptr;
    }
    virtual     void    onEvent             (int                nEvent,
                                             void*              pData1,
                                             void*              pData2,
                                             void*              pData3)override;
                void    init                (void);
                void    onResize            (void);

private:

};

_G2D_NAMESPACE_END_