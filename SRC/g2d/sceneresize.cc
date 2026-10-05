#include "sceneresize.h"
#include "engine.h"
#include "logger.h"

_G2D_NAMESPACE_BEGIN_

#ifdef TARGET_EMSCRIPTEN
EM_JS(int, call_canvas_getWidth, (void), 
{
    if (!window._glCanv)
        window._glCanv = document.getElementById('canvas');
    return window._glCanv.width;
});

EM_JS(int, call_canvas_getHeight, (void), 
{
    if (!window._glCanv)
        window._glCanv = document.getElementById('canvas');
    return window._glCanv.height;
});

EM_JS(int, call_getAvailWidth, (void), 
{
    return window.innerWidth
        || document.documentElement.clientWidth
        || document.body.clientWidth;

});

EM_JS(int, call_getAvailHeight, (void), 
{
    return window.innerHeight
        || document.documentElement.clientHeight
        || document.body.clientHeight;
});

// Defines a JS-based function accessible from C++
EM_JS(bool, isMobile, (), {
  // 1. Use modern User-Agent Client Hints API if available (Chrome, Edge, etc.)
  if (navigator.userAgentData) {
    return navigator.userAgentData.mobile;
  }

  const ua = navigator.userAgent;

  // 2. Standard regex check for common mobile platforms
  const isMobileUA = /android|iphone|ipad|ipod|iemobile|opera mini/i.test(ua);
  if (isMobileUA) {
    return true;
  }

  // 3. Special check for modern iPads (iPadOS) that mask themselves as Macintosh
  // In 2025, most touch-enabled Apple devices that report as Mac are actually iPads
  if (navigator.maxTouchPoints > 1 && ua.includes('Macintosh')) {
    return true;
  }

  // Otherwise, assume it's a desktop device
  return false;
});

EM_JS(float, getDpr, (), {
    let dpr  = window.devicePixelRatio || 1;
    let tier = (window.GPU_TIER !== null && window.GPU_TIER !== undefined) ? window.GPU_TIER : 1;
    let res  = dpr;
    if (tier < 1)
    {
        res = Math.min(1.0, dpr); 

    }
    else if (tier < 2) {
        // ��� Tier 1 (A03 � ��) ����� 1.0. 
        // 1.5 ����� ���������� 30 FPS � 15 FPS.
        res = Math.min(1.5, dpr); 
    } 
    else if (tier == 2) {
        // ��� �������� ������ 2.0 � ��� ��������� ������.
        res = Math.min(2.0, dpr);
    } 
    else {
        // ��� Tier 3 (iPhone 16, S23) � ��� �����������,
        // �� ���� 3.0 ������� ������ ��� (���� �� �����, � ������� �����).
        res = Math.min(3.0, dpr);
    }
    
    return res;
});

EM_JS(float, call_resize, (int cx, int cy), 
{
    if (!window._glCanv)
        window._glCanv = document.getElementById('canvas');
    let dpr = getDpr();
    window._glCanv.width  = cx * dpr;
    window._glCanv.height = cy * dpr;
    window._glCanv.style.width  = cx.toString() + "px";
    window._glCanv.style.height = cy.toString() + "px";
    return dpr;
});
#endif //TARGET_EMSCRIPTEN

#ifdef TARGET_WIN
int call_getAvailWidth()
{
    return CSceneResize::getInstance()->getAvailWidth();
}
int call_getAvailHeight()
{
    return CSceneResize::getInstance()->getAvailHeight();
}
float call_resize(int cx, int cy)
{
    return 1.f;
}
#endif //TARGET_WIN

CSceneResize* CSceneResize::_instance = NULL;

CSceneResize* CSceneResize::getInstance()
{
    if (!CSceneResize::_instance)
        CSceneResize::_instance = new CSceneResize();
    return CSceneResize::_instance;
}


CSceneResize::CSceneResize():_nScrWidth(0xFFFFFFFF),_nScrHeight(0xFFFFFFFF),_nGameHeight(0xFFFFFFFF),_nGameWidth(0xFFFFFFFF)
{
    auto& cfg     = Engine::getCfg();
    _nBgWidth     = static_cast<uint32_t>(cfg.INIT_SCR_CX);
    _nBgWidth     = static_cast<uint32_t>(cfg.INIT_SCR_CY);
#ifdef TARGET_WIN
    _nAvailWidth  = static_cast<uint32_t>(cfg.INIT_SCR_CX);
    _nAvailHeight = static_cast<uint32_t>(cfg.INIT_SCR_CY);
#endif
}

void CSceneResize::init()
{
    assert(_nGameWidth == 0xFFFFFFFF);
    assert(_nGameHeight == 0xFFFFFFFF);
    assert(_nScrWidth == 0xFFFFFFFF);
    assert(_nScrHeight == 0xFFFFFFFF);
    onResize(call_getAvailWidth(), call_getAvailHeight());
    //CGfx::getInstance()->updateViewPort();
    emit(CSceneResize::EVT_RESOLUTION_CHANGED);
}

float CSceneResize::get_dpr()
{
#ifdef EMSCRIPTEN
    if (!_dpr.has_value())
        _dpr = getDpr();
    return *_dpr;
#else
    return 1.f;
#endif
}

void CSceneResize::toPixelCoords(float& x, float& y)
{
    x = (1.f + x) * _nScrWidth  / 2.f;
    y = (1.f - y) * _nScrHeight / 2.f;
}

void CSceneResize::toScreenCoords(float& x, float& y) 
{
    x = (1.f + x)  * _nScrWidth / 2.f;
    y = (1.f - y)  * _nScrHeight / 2.f;
}

void CSceneResize::toGameCoords(float& x, float& y)
{
    x = (1.f + x) * _nGameWidth  / 2.f;
    y = (1.f - y) * _nGameHeight / 2.f;
}

void CSceneResize::onResize(uint32_t nScrWidth, uint32_t nScrHeight)
{
    _nScrWidthDpi1  = nScrWidth;
    _nScrHeightDpi1 = nScrHeight;

    float w = nScrWidth;
    float h = nScrHeight;
    float newAsp = w / h;
    auto& cfg = Engine::getCfg();
    if (newAsp > cfg.MAX_ASPECT_RATIO)
    {
        nScrWidth = nScrHeight * cfg.MAX_ASPECT_RATIO;
    }
    w = nScrWidth;
    h = nScrHeight;

    _nScrHeight = nScrHeight;
    _nScrWidth  = nScrWidth;        
    float asp = w / cfg.ASPECT_RATIO;
    if (asp > h)
    {
        _nGameWidth  = h * cfg.ASPECT_RATIO;
        _nGameHeight = h;

        //_nBgWidth    = w;
        //_nBgHeight   = asp;
    }
    else
    {
        _nGameWidth  = w;
        _nGameHeight = w / cfg.ASPECT_RATIO;
        //_nBgWidth    = _nGameWidth;
        //_nBgHeight   = _nGameHeight;
    } 
    _nBgWidth  = _nGameWidth *  cfg.BG_SCALE;
    _nBgHeight = _nGameHeight * cfg.BG_SCALE;

#ifdef TARGET_EMSCRIPTEN
    if (_nScrHeight > _nBgHeight)
        _nScrHeight = _nBgHeight;
#endif

    LOG_TRACE_FMT("call_resize(%d, %d)", _nScrWidth, _nScrHeight);

    float fDpr = call_resize(_nScrWidth, _nScrHeight); 
    _dpr = fDpr;
    _nGameWidth *= fDpr;
    _nGameHeight *= fDpr;

    _nBgWidth *= fDpr;
    _nBgHeight *= fDpr;

    _nScrWidth *= fDpr;
    _nScrHeight *= fDpr;

    int64_t sW = _nScrWidth;
    int64_t sH = _nScrHeight;
    int64_t gW = _nGameWidth;
    int64_t gH = _nGameHeight;
    int64_t bW = _nBgWidth;
    int64_t bH = _nBgHeight;

    _ptGameOffs.x = float(sW - gW) / 2.f;
    _ptGameOffs.y = float(sH - gH) / 2.f;

    _ptBgOffs.x = float(sW - bW) / 2.f;
    _ptBgOffs.y = float(sH - bH) / 2.f;
}

bool CSceneResize::checkAndEmit(bool bForce)
{
    bool bRes = false;
    int w = call_getAvailWidth();
    int h = call_getAvailHeight();
    if (bForce || w != _nScrWidthDpi1 || h != _nScrHeightDpi1)
    {
        bRes = true;
        onResize(w, h);
        CGfx::getInstance()->updateViewPort();
        emit(CSceneResize::EVT_RESOLUTION_CHANGED);
    }
    return bRes;
}

bool CSceneResize::onFrame(float dt)
{
    _fTimer += dt;
    bool bRes = false;
    if (_fTimer > 1)
    {
        _fTimer = 0;
        bRes = checkAndEmit();
    }
    return bRes;
}

_G2D_NAMESPACE_END_