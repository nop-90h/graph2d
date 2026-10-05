#include "msched.h"

_G2D_NAMESPACE_BEGIN_

CSched* CSched::_instance = NULL;

CSched* CSched::getInstance()
{
    if (!CSched::_instance)
        CSched::_instance = new CSched();
    return CSched::_instance;
}

void CSched::onFrame(float dt)
{
    _durringUpdate = true;
    for (auto it:_callbacks)
    {
        it->fLatter -= dt;
        if (islessequal(it->fLatter, 0.f))
        {
            assert(!it->_isCalled);
            //LOG_TRACE_FMT("CSched calling %s ...", !it->_strDbg.empty() ? it->_strDbg.c_str() : "NO DEBUG INFO");
            it->cb(it->pData);
            it->_isCalled = true;
        }
    }
    _callbacks.erase(std::remove_if(_callbacks.begin(), _callbacks.end(), [](sched_cb_ptr it){
        return it->_isCalled && islessequal(it->fLatter, 0.f);
    }), _callbacks.end());
    _durringUpdate = false;

    for (size_t i = 0; i < _waitUpdate.size(); i++)
        _callbacks.push_back(_waitUpdate[i]);
    _waitUpdate.clear();
}

void CSched::callLatter(SchedCallback_f cb, void* pData, float fLatter, const char* lpccDebugString)
{
    sched_cb_ptr p(new sched_cb_t());
    p->cb           = cb;
    p->fLatter      = fLatter;
    p->pData        = pData;
    p->_isCalled    = false;
    if (lpccDebugString)
        p->_strDbg = lpccDebugString;
    if (_durringUpdate)
    {
        _waitUpdate.push_back(p);
    }
    else
    {
        _callbacks.push_back(p);

    }
}

void CSched::callLatterS(SimpleCallback cb, float fLatter)
{
    callLatter([cb](void*) {cb();}, nullptr, fLatter);
}

_G2D_NAMESPACE_END_
