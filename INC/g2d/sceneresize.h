#pragma once

#include "g2d.h"
#include "eventemmiter.h"
#include "eventdef.h"

_G2D_NAMESPACE_BEGIN_

class CSceneResize:public CEventEmmiter
{
public:
        enum Events
        {
            EVT_RESOLUTION_CHANGED = EventsNs::SCREENRESIZE_FIRST_EVT
        };
private:
    uint32_t             _nScrWidthDpi1;
    uint32_t             _nScrHeightDpi1;
    uint32_t             _nScrWidth;
    uint32_t             _nScrHeight;
                         
    uint32_t             _nGameWidth;
    uint32_t             _nGameHeight;
    uint32_t             _nBgWidth;
    uint32_t             _nBgHeight;
    Point                _ptGameOffs;
    Point                _ptBgOffs;
    float                _fTimer = 0;
    std::optional<float> _dpr;
#ifdef TARGET_WIN
    uint32_t _nAvailWidth;
    uint32_t _nAvailHeight;
#endif //TARGET_WIN
private:
    static  CSceneResize*   _instance;
private:
    void                    onResize        (uint32_t       nScrWidth,
                                             uint32_t       nScrHeight);
public:
                            CSceneResize    (void);
    bool                    onFrame         (float          dt);
    void                    init            (void);
    bool                    checkAndEmit    (bool           force  = false);
    static CSceneResize*    getInstance     (void);
    auto                    getGameWidth    (void)  { return _nGameWidth;  }
    auto                    getGameHeight   (void)  { return _nGameHeight; }
    void                    toPixelCoords   (float&         x,
                                             float&         y);
    void                    toScreenCoords  (float&         x,
                                             float&         y);
    void                    toGameCoords    (float&         x,
                                             float&         y);
    float                   getGameOffsX    (void) { return _ptGameOffs.x; }
    float                   getGameOffsY    (void) { return _ptGameOffs.y; }

    auto                    getBgWidth      (void)  { return _nBgWidth;  }
    auto                    getBgHeight     (void)  { return _nBgHeight; }
    float                   getBgOffsX      (void) { return _ptBgOffs.x; }
    float                   getBgOffsY      (void) { return _ptBgOffs.y; }
    auto                    getScreenWidth  (void)  { return _nScrWidth;  }
    auto                    getScreenHeight (void)  { return _nScrHeight; }
    float                   get_dpr         (void);

#ifdef TARGET_WIN
    void                    setAvailWidth   (uint32_t        n) { _nAvailWidth = n; }
    void                    setAvailHeight  (uint32_t        n) { _nAvailHeight = n; }
    auto                    getAvailWidth   (void) { return _nAvailWidth; }
    auto                    getAvailHeight  (void) { return _nAvailHeight; }

#endif
};

_G2D_NAMESPACE_END_