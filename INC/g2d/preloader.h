#pragma once

#include "g2d.h"
#include "container.h"
#include "rsrcfile.h"
#include "sprite.h"
#include "staticlabel.h"

_G2D_NAMESPACE_BEGIN_

class Preloader : public CContainer,
                  public IEventListener
{
public:
                        ~Preloader              (void);
    virtual     void    init                    (const              SimpleCallback cbOnLoaded,
                                                 const char*        lpszHtml = nullptr);
                void    removeView              (void);
    virtual     void    onEvent                 (int                nEvent,
                                                 void*              pData1,
                                                 void*              pData2,
                                                 void*              pData3) override;
                auto&   filesToLoad             (void) { return _filesToLoad; };
                bool    isInitialPreloadDone    (void) { return _initialPreloadDone; }
protected:
                void    createView              (void);
                void    resizeView              (void);
                void    startLoad               (void);
                void    setProgressVal          (float  fProgress);
private:
        StaticLabelPtr  _ptrLabel;
        SimpleCallback  _cbOnLoaded;
        RsrcFileGroup   _preloader;
        RsrcFileGroup   _filesToLoad;
        std::string     _sHtml;
        float           _fProgress = 0;
        bool            _bListening = false;
        bool            _initialPreloadDone = false;
};

typedef std::shared_ptr<Preloader> PreloaderPtr;

_G2D_NAMESPACE_END_