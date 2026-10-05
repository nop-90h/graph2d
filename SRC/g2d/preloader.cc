#include "preloader.h"
#include "gfx.h"
#include "engine.h"
#include "htmldom.h"
_G2D_NAMESPACE_BEGIN_

Preloader::~Preloader()
{
    removeView();
}

void Preloader::init(SimpleCallback cbOnLoaded, const char* lpszHtml)
{
    const char* defHtml = R"HTML(
<center> 
  <progressbar
   width="300"
   height="90"
   percent="0"
   backcolor="#000000CC"
   bordercolor="#FFFFFFFF"
   fillcolor="#FFCC00FF"
   border="2"
   padding="2"
   id="progress">
  <br>
  <b><font size="96" color="#fff" id="loadingText">0%</font></b>
</center>)HTML";


    _cbOnLoaded = cbOnLoaded;
    _sHtml      = lpszHtml ? lpszHtml : defHtml;
    CGfx::getInstance()->getGameRoot()->addChild(shared_from_this());
    _ptrLabel = std::make_shared<StaticLabel>();
    _preloader.add("whitebox.png");
    _preloader.add("noise.png");
    if (!_preloader.empty())
    {
        _preloader.load([this]{
            Engine::getCfg().wb = CGfx::getInstance()->spriteFromTexture("whitebox.png");
            startLoad();
        });
    }
    else
    {
        startLoad();
    }
}

void Preloader::startLoad()
{
    Engine::getInstance().setInitialPreloadDone();
    createView();
    assert(!_filesToLoad.empty());
    _filesToLoad.load([this]{
        removeView();
        assert(_cbOnLoaded);
        if (_cbOnLoaded)
            _cbOnLoaded();
    }, [this](LPCTSTR p, float fDone, float fTotal){
        assert(fTotal > 0);
        setProgressVal(fDone / fTotal);
    });
}

void Preloader::onEvent(int nEvent, void* pData1, void* pData2, void* pData3)
{
    switch (nEvent)
    {
        case CSceneResize::EVT_RESOLUTION_CHANGED: resizeView(); break;
        default:assert(false);break;
    }
}

void Preloader::removeView()
{
    if (_bListening)
    {
        removeFromParent();
        CSceneResize::getInstance()->removeListener(this);
        _bListening = false;
    }
}

void Preloader::resizeView()
{
    auto& cfg = Engine::getCfg();
    float fScaleX = 1.f;
    float fScaleY = 1.f;

    float cx = static_cast<float>(CSceneResize::getInstance()->getGameWidth());
    float cy = static_cast<float>(CSceneResize::getInstance()->getGameHeight());

    fScaleX = cx / cfg.INIT_SCR_CX;
    fScaleY = cy / cfg.INIT_SCR_CY;

    float fMinScale = std::min(fScaleX, fScaleY);

    setScale(fMinScale, fMinScale);
    setPos((cx - cfg.INIT_SCR_CX * fMinScale) / 2.f,
            (cy - cfg.INIT_SCR_CY * fMinScale) / 2.f);
}

void Preloader::setProgressVal(float fProgress)
{
    if (fProgress > _fProgress)
    {
        auto textNodeId = HTMLDom::getInstance()->getElementIdById(_ptrLabel->getHTMLRootId(), "loadingText");
        assert(textNodeId);
        auto s = std::format("{}%", static_cast<int>(fProgress * 100.f));
        HTMLDom::getInstance()->setTextContent(textNodeId, s.c_str());

        auto progressBarNodeId = HTMLDom::getInstance()->getElementIdById(_ptrLabel->getHTMLRootId(), "progress");
        assert(progressBarNodeId);
        HTMLDom::getInstance()->setProgressValue(progressBarNodeId, fProgress); 
    }

}

void Preloader::createView()
{
    _ptrLabel->setBoxMode(eTextRenderType::HTML_BOX, Engine::getCfg().INIT_SCR_CX);
    _ptrLabel->setText(_sHtml.c_str());
    _ptrLabel->setPosCentered(Engine::getCfg().INIT_SCR_CX, Engine::getCfg().INIT_SCR_CY);
    addChild(_ptrLabel);
    resizeView();
    _bListening = true;
    CSceneResize::getInstance()->addListener(this);
}

_G2D_NAMESPACE_END_