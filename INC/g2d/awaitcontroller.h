#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

enum class eAwaitControllerShow
{
    CONNECTING,
    CLEINT_ERROR,
    RELOGGEDIN,
};

class AwaitController : public IEventListener
{
public:
                void			init					(bool           bAwait = true);
                void			showWait				(bool			bShow = true);
                void			update					(float			dt);


public:
    //IEventListener
    virtual     void            onEvent                 (int            nEvent,
                                                         void*          pData1 = NULL,
                                                         void*          pData2 = NULL,
                                                         void*          pData3 = NULL);
private:
                void            showWaitScreen          (bool           bShow = true);
                void            onClientBasic           (void);
                void            onClientNonBasic        (void);
                void            onClientError           (void);
                void            onReloggedIn            (void);

private:
    int		                _nWaits		     = 0;
    float	                _fShowTimer      = 0.f;
    bool                    _bIsShowing      = false;
    bool                    _bIsInitDone     = false;
    bool                    _isWaitingClient = false;
    eAwaitControllerShow    _eShow           = eAwaitControllerShow::CONNECTING;
public:
    inline static AwaitController* getInstance()
    {
        static AwaitController res;
        return &res;
    }

};

_G2D_NAMESPACE_END_