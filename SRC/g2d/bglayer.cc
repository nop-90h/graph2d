#include "bglayer.h"
#include "sceneresize.h"
#include "spriteloader.h"
#include "gfx.h"
#include "engine.h"

_G2D_NAMESPACE_BEGIN_

void BgLayer::onEvent(int nEvent, void* pData1, void* pData2, void* pData3)
{
    switch (nEvent)
    {
        case CSceneResize::EVT_RESOLUTION_CHANGED:
        {
            onResize();
        }
        break;

        default:
        {
            assert(false);
        }
        break;
    }
}

void BgLayer::init()
{
    CSceneResize::getInstance()->addListener(this);
    onResize();
}

void BgLayer::onResize()
{
    auto& cfg = Engine::getCfg();

    setScale(CSceneResize::getInstance()->getBgWidth()  / cfg.INIT_SCR_CX,
             CSceneResize::getInstance()->getBgHeight() / cfg.INIT_SCR_CY);

}

_G2D_NAMESPACE_END_
