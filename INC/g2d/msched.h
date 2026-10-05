#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

typedef std::function<void (void* pData)> SchedCallback_f;

struct sched_cb_t
{
    SchedCallback_f cb;
    float           fLatter = 0.f;
    void*           pData = nullptr;
    std::string     _strDbg;
    bool            _isCalled = false;
};

typedef std::shared_ptr<sched_cb_t> sched_cb_ptr;
typedef std::vector<sched_cb_ptr>   VCallbacks;
typedef VCallbacks::iterator        VCallbacksIt;

class CSched
{
private:
    static CSched*  _instance;
    VCallbacks      _callbacks;
    VCallbacks      _waitUpdate;
    bool            _durringUpdate = false;
public:
    static CSched*      getInstance     (void);
    void                onFrame         (float              dt);
    void                callLatter      (SchedCallback_f    cb, 
                                         void*              pData = nullptr, 
                                         float              fLatter = 0.f, 
                                         const char*        lpccDebugString = nullptr);
    void                callLatterS     (SimpleCallback     cb, 
                                         float              fLatter = 0.f);
};

_G2D_NAMESPACE_END_