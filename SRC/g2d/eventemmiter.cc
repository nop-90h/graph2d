#include "eventemmiter.h"

_G2D_NAMESPACE_BEGIN_

IEventListener::~IEventListener(){}

void CEventEmmiter::addSimpleCallback(void* pObj, SimpleCallback cb, int nEvent, void *ensureParam1)
{
    Callback_t cbv = {ensureParam1, pObj, cb};
    _mmapCallbacks.insert(MMapSimpleCallbacks::value_type(nEvent, cbv));
}

void CEventEmmiter::removeSimpleCallback(int nEvent, void* ensureParam1, void* pObj)
{
    auto range = _mmapCallbacks.equal_range(nEvent);
    for (auto it = range.first; it != range.second;)
    {
        if (it->second._ensureParam1 == ensureParam1 && it->second._pObj == pObj)
            it = _mmapCallbacks.erase(it);
        else
            ++it;
    }
}

void CEventEmmiter::removeAllCallbacksFor(void *pObj)
{
    
    for (auto it = _mmapCallbacks.begin(); it != _mmapCallbacks.end();)
    {
        if (it->second._pObj == pObj)
            it = _mmapCallbacks.erase(it);
        else
            ++it;
    }
}

bool CEventEmmiter::removeListener(IEventListener *pListener)
{
    bool bRes = false;
    ListenersIt it = std::find(_vListeners.begin(), _vListeners.end(), pListener);
    if (it != _vListeners.end())
    {
        _vListeners.erase(it);
        bRes = true;
    }
    return bRes;
}

void CEventEmmiter::emit(int nEvent, void* pData1, void* pData2, void* pData3)
{
    static Listeners v;
    v.clear();

    for (size_t i = 0; i < _vListeners.size(); i++)
    {
        v.push_back(_vListeners[i]);
    }
    for (size_t i = 0; i < v.size(); i++)
    {
        v[i]->onEvent(nEvent, pData1, pData2, pData3);
    }

    static std::vector<std::tuple<SimpleCallback, void*>> v2;
    v2.clear();

    auto range = _mmapCallbacks.equal_range(nEvent);
    
    for (auto it = range.first; it != range.second; it++)
    {
        if (it->second._ensureParam1 == (void*)(uintptr_t)0xFFFFFFFF || it->second._ensureParam1 == pData1)
        {
            v2.push_back({ it->second._cb, pData1 });
        }
    }
    for (auto& it : v2)
    {
        _currentCbParam = std::get<1>(it);
        std::get<0>(it)();
    }
}

CEventEmmiter::~CEventEmmiter()
{    
}

_G2D_NAMESPACE_END_