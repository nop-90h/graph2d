#include "loader.h"
#include "wholefile.h"

_G2D_NAMESPACE_BEGIN_

#include "utilfuncs.h"
#include "wholefile.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#if !defined(LOAD_FROM_WHOLE_FILE) && defined(TARGET_EMSCRIPTEN)
extern "C"
{
    EM_JS(void, send_get_request, (std::uintptr_t slot_id, const char* path_cstr, void* buf_ptr, uint32_t buf_size),
    {
        const path_str   = UTF8ToString(path_cstr);
        const req        = new XMLHttpRequest();
        req.open('GET', path_str);
        req.responseType = 'arraybuffer';

        req.onprogress = function(event) 
        {
            if (event.lengthComputable) 
            {
                __progress_http_callback(slot_id, event.loaded, event.total);
            }
        };        

        req.onreadystatechange = function() 
        {
            if (req.readyState == XMLHttpRequest.DONE) 
            {
                if ((req.status == 206) || ((req.status == 200))) 
                {
                    const u8_array = new Uint8Array(\x2F\x2A\x2A @type {!ArrayBuffer} \x2A\x2F (req.response));
                    const content_fetched_size = u8_array.length;
                    if (buf_size == 0)
                    {
                        buf_size = __alloc_buff(slot_id, content_fetched_size);
                        if (buf_size)
                            buf_ptr = __get_buf_ptr(slot_id);
                    }
                    if (content_fetched_size <= buf_size) 
                    {
                        HEAPU8.set(u8_array, buf_ptr);
                        __get_response(slot_id, content_fetched_size, 0, 0);
                    }
                    else 
                    {
                        __failed_buffer_too_small(slot_id);
                    }
                }
                else 
                {
                    __failed_http_status(slot_id, req.status);
                }
            }
        };
        req.send();
    });

    EM_JS(void, send_img_request, (std::uintptr_t slot_id, const char* path_cstr),
    {
        const path_str = UTF8ToString(path_cstr);
        const xhr = new XMLHttpRequest();
        xhr.open('GET', path_str, true);
        xhr.responseType = 'arraybuffer'; // Загружаем как бинарные данные
        // Отслеживание прогресса загрузки
        xhr.onprogress = function(event) 
        {
            if (event.lengthComputable) 
            {
                __progress_http_callback(slot_id, event.loaded, event.total);
            }
        };

        xhr.onreadystatechange = function()
        {
            if (xhr.readyState == XMLHttpRequest.DONE) 
            {
                if ((xhr.status == 206) || ((xhr.status == 200))) 
                {
                    // Создаем Blob из полученного ArrayBuffer
                    const blob = new Blob([xhr.response], { type: 'image/jpeg' }); // Укажите правильный MIME-тип, если знаете
                    const imageUrl = URL.createObjectURL(blob); // Создаем URL-объект

                    const imge     = new Image();
                    imge.src       = imageUrl;
                    imge.onerror   = function()
                    {
                        __failed_http_status(slot_id, 0);
                    };
                    imge.onload    = function()
                    {
                        if (!window.gfx_imgs)
                            window.gfx_imgs = [];
                            window.gfx_imgs.push(imge);
                        console.log(path_str + " loaded id = " + (window.gfx_imgs.length - 1).toString());
                        __get_response(slot_id, window.gfx_imgs.length - 1, imge.width, imge.height);
                    }
                }
                else
                {
                    __failed_http_status(slot_id, xhr.status);                
                }
            }
        };
        xhr.send();
    });

    EMSCRIPTEN_KEEPALIVE uint32_t _alloc_buff(uint32_t slot_id, uint32_t size)
    {
        CLoader* p = (CLoader*)slot_id;
        return p->allocBuff(size) ? size:0;
    }

    EMSCRIPTEN_KEEPALIVE uint32_t _get_buf_ptr(uint32_t slot_id)
    {
        CLoader* p = (CLoader*)slot_id;
        return (uint32_t)p->getData();
    }

    EMSCRIPTEN_KEEPALIVE void _failed_buffer_too_small(uint32_t slot_id)
    {
        CLoader* pReq = (CLoader*)slot_id;
        pReq->private_cb_Failed(eXMLFailReason::BUFFER_TOO_SMALL);
    }

    EMSCRIPTEN_KEEPALIVE void _failed_http_status(uint32_t slot_id, uint32_t status)
    {
        CLoader* pReq = (CLoader*)slot_id;
        pReq->private_cb_Failed(eXMLFailReason::HTTP_RETURN_FAIL);
    }

    EMSCRIPTEN_KEEPALIVE void _progress_http_callback(uint32_t slot_id, float fLoaded, float fTotal)
    {
        CLoader* pReq = (CLoader*)slot_id;
        pReq->private_cb_Progress(fLoaded, fTotal);
    }

    EMSCRIPTEN_KEEPALIVE void _get_response(uint32_t slot_id, uint32_t content_fetched_size, uint32_t cx, uint32_t cy)
    {
        CLoader* pReq = (CLoader*)slot_id;
        pReq->private_cb_Loaded(content_fetched_size, cx, cy);
    }
};

#else 

void send_get_request (std::uintptr_t slot_id, const char* path_cstr, void* buf_ptr, uint32_t buf_size)
{
    size_t bytesRead = 0;
    if (buf_size > 0)
    {
        assert(false);
        std::quick_exit(0x1489);
        //if (RAIIFile::readFileToBuff(path_cstr, buf_ptr, buf_size, bytesRead))
        //{
        //    CLoader* pReq = (CLoader*)slot_id;
        //    pReq->private_cb_Loaded(bytesRead, 0, 0);
        //}
        //else
        //{
        //    CLoader* pReq = (CLoader*)slot_id;
        //    pReq->private_cb_Failed(eXMLFailReason::NOT_FOUND);
        //}

    }
    else
    { 
        CLoader* pReq = (CLoader*)slot_id;
        WholeFile wf(path_cstr);
        if (wf.isValid())
        {
            auto pBuff = wf.release();
            pReq->setData(pBuff, bytesRead);
            pReq->private_cb_Loaded(bytesRead, 0, 0);
        }
        else
        {
            pReq->private_cb_Failed(eXMLFailReason::NOT_FOUND);
        }
    }
}

void send_img_request(std::uintptr_t slot_id, const char* path_cstr)
{
    WholeFile wf(path_cstr);
    CLoader* pReq = (CLoader*)slot_id;
    if (wf.isValid())
    {
        int nchans = 0;        
        
        pReq->_imgInfo.pImageBytes = stbi_load_from_memory((stbi_uc*)wf.buffer(), static_cast<int>(wf.size()), &pReq->_imgInfo.cx, &pReq->_imgInfo.cy, &nchans, STBI_rgb_alpha);
        assert(pReq->_imgInfo.pImageBytes);
        wf.reset();
        pReq->private_cb_Loaded((std::uintptr_t)pReq->_imgInfo.pImageBytes, pReq->_imgInfo.cx, pReq->_imgInfo.cy);
    }
    else
    {
        pReq->private_cb_Failed(eXMLFailReason::NOT_FOUND);
    }
}
#endif //TARGET_WIN

IXMLReqCallback::~IXMLReqCallback(){}

CLoader::CLoader(const char* lpccURL, eResourceType eType, void* pBuff, size_t nBuffSz, IXMLReqCallback* pCallback)
{
    _isBusy        = false;
    _eResourceType = eType;
    _isAllocated   = false;
    _strURL        = lpccURL;
    _pBuff         = pBuff;
    _nBuffSz       = nBuffSz;
    _pCallback     = pCallback;
}

CLoader::~CLoader()
{
//    assert(false);
    if (_isAllocated)
        free(_pBuff);
}

bool CLoader::allocBuff(size_t sz)
{
#ifndef TARGET_EMSCRIPTEN
    assert(false);
#endif //TARGET_EMSCRIPTEN

    bool bRes = false;
    assert(_nBuffSz == 0 && _pBuff == NULL);
    if (_nBuffSz == 0 && _pBuff == NULL)
    {
        _nBuffSz      = sz;
        _pBuff        = malloc(sz + 1);
        _isAllocated  = true;
        bRes = true;
    }
    return bRes;
}

void CLoader::setData(void* pBuff, size_t nSz)
{
    if (_pBuff)
    {
        free(_pBuff);
    }
    _pBuff        = pBuff;
    _nBuffSz      = nSz;
    _isAllocated  = pBuff != nullptr;
}

void* CLoader::getData()
{
    assert(_eResourceType == eResourceType::XML_REQUEST);
    return _pBuff;
}

size_t CLoader::getDataSize()
{
    assert(_eResourceType == eResourceType::XML_REQUEST);
    return _nBuffSz;
}

void CLoader::private_cb_Loaded(std::uintptr_t id, uint32_t cx, uint32_t cy)
{
    assert(_pCallback);
    _eState = eLoaderState::LOADED;
    if (_pCallback)
    {
        switch (_eResourceType)
        {
            case eResourceType::XML_REQUEST:
            {
                _pCallback->onLoaded(getPtr(), _pBuff, _nBuffSz, 0, 0);
            }
            break;

            case eResourceType::IMAGE_REQUEST:
            {
                _loadedId = id;
                _cx       = cx;
                _cy       = cy;
                _pCallback->onLoaded(getPtr(), (void*)id, 0, cx, cy);
            }
            break;

            default:
            {
                assert(false);
            }
            break;
        }
    }
    _isBusy = false;
}

void CLoader::private_cb_Progress(float fDone, float fTotal)
{
    assert(_pCallback);
    if (_pCallback)
    {
        _pCallback->onProgress(getPtr(), fDone, fTotal);
    }
}

void CLoader::private_cb_Failed(eXMLFailReason eReason)
{
    _eState = eLoaderState::FAILED;
    if (_pCallback)
    {
        _pCallback->onFailed(getPtr(), eReason);
    }
    _isBusy = false;
}

void CLoader::xmlRequest()
{
    assert(!_isBusy);
    assert(_pCallback);
    if (_pCallback && !_isBusy)
    {
        if (_pBuff == NULL && _nBuffSz > 0)
        {
            _pBuff        = malloc(_nBuffSz + 1);
            _isAllocated  = true;
        }
        _isBusy           = true;
        send_get_request((std::uintptr_t)this, _strURL.c_str(), _pBuff, _nBuffSz);
    }
}

void CLoader::imgRequest()
{
    assert(!_isBusy);
    assert(_pCallback);
    if (_pCallback && !_isBusy)
    {
        send_img_request((std::uintptr_t)this, _strURL.c_str());
    }
}

const char* CLoader::getStr()
{
    const char* pRes = NULL;

    if (_pBuff && _nBuffSz)
    {
        ((char*)_pBuff)[_nBuffSz] = 0; // assume allocated + 1;
        pRes = (const char*)_pBuff;
    }
    return pRes;
}


void CLoader::run()
{
    assert(!_isBusy);
    assert(_pCallback);

    if (_eResourceType == eResourceType::AUTO)
    {
        auto posPng = _strURL.rfind(".png");
        auto posJpg = _strURL.rfind(".jpg");
        if (posPng == _strURL.length() - 4 || posJpg == _strURL.length() - 4)
            _eResourceType = eResourceType::IMAGE_REQUEST;
        else
            _eResourceType = eResourceType::XML_REQUEST;
    }

    if (_pCallback && !_isBusy)
    {
        switch (_eResourceType)
        {
            case eResourceType::XML_REQUEST:
            {
                _eState = eLoaderState::LOADING;
                xmlRequest();
            }
            break;

            case eResourceType::IMAGE_REQUEST:
            {
                _eState = eLoaderState::LOADING;
                imgRequest();
            }
            break;

            default:
            {
                assert(false);
            }
            break;
        }
    }
}

_G2D_NAMESPACE_END_