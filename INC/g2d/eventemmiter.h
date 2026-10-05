#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_


class IEventListener
{
public:
    virtual             ~IEventListener     (void);
    virtual     void    onEvent             (int                nEvent,
                                             void*              pData1 = NULL,
                                             void*              pData2 = NULL,
                                             void*              pData3 = NULL){ assert(false); };
};

typedef std::vector<IEventListener*> Listeners;
typedef Listeners::iterator ListenersIt;

struct Callback_t
{
    void*           _ensureParam1;
    void*           _pObj;
    SimpleCallback  _cb;
};

typedef std::unordered_multimap<int, Callback_t> MMapSimpleCallbacks;

class CEventEmmiter
{
private:
    Listeners            _vListeners;
    MMapSimpleCallbacks  _mmapCallbacks;
    void*                _currentCbParam = nullptr;
public:
    inline  void    addListener             (IEventListener*    pListener)  { _vListeners.push_back(pListener); }
            void    addSimpleCallback       (void*              pObj,
                                             SimpleCallback     cb,
                                             int                nEvent,
                                             void*              ensureParam1 = (void*)(uintptr_t)0xFFFFFFFF);
    inline  void*   getCallbackParam        (void) { return _currentCbParam;}
            void    removeSimpleCallback    (int                nEvent,
                                             void*              ensureParam1,
                                             void*              pObj);
            void    removeAllCallbacksFor   (void*              pObj);
            bool    removeListener          (IEventListener*    pListener);
            void    emit                    (int                nEvent,
                                             void*              pData1 = NULL,
                                             void*              pData2 = NULL,
                                             void*              pData3 = NULL);
    virtual         ~CEventEmmiter          (void);
};

_G2D_NAMESPACE_END_