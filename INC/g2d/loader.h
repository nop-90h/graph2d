#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_


class CLoader;

typedef std::shared_ptr<CLoader> CXMLReqPtr;

enum eResourceType : uint8_t
{
    UNKNOWN      = 0,
    AUTO,
    XML_REQUEST,
    IMAGE_REQUEST
};

enum eXMLFailReason : uint8_t
{
    NOT_FOUND           = 0,
    BUFFER_TOO_SMALL,
    HTTP_RETURN_FAIL
};

#if defined(LOAD_FROM_WHOLE_FILE) || defined(TARGET_WIN)
struct ImageInfo_t
{
    unsigned char* pImageBytes = nullptr;
    int cx = 0;
    int cy = 0;
};
#endif //LOAD_FROM_WHOLE_FILE

class CLoader;
typedef std::shared_ptr<CLoader> CLoaderPtr;

class IXMLReqCallback
{
public:
    virtual         ~IXMLReqCallback    (void);
    virtual void    onLoaded            (CLoaderPtr         pReq, 
                                         void*              pData, 
                                         int                nSize,
                                         uint32_t           nImgCx,
                                         uint32_t           nImgCy) = 0;
    virtual void    onFailed            (CLoaderPtr         pReq, 
                                         eXMLFailReason     eReason) = 0;
    virtual void    onProgress          (CLoaderPtr         pReq,
                                         float              fDone,
                                         float              fTotal) = 0;
};

enum class eLoaderState : int
{
    NOT_STARTED,
    LOADING,
    LOADED,
    FAILED
};

class CLoader: public std::enable_shared_from_this<CLoader>
{
public:

#if defined(LOAD_FROM_WHOLE_FILE) || defined(TARGET_WIN)
    ImageInfo_t         _imgInfo;
#endif //LOAD_FROM_WHOLE_FILE

private:
    std::string         _strURL;
    IXMLReqCallback*    _pCallback;
    bool                _isBusy;
    void*               _pBuff;
    size_t              _nBuffSz;
    std::uintptr_t      _loadedId;
    eResourceType       _eResourceType;
    bool                _isAllocated;
    uint32_t            _cx;
    uint32_t            _cy;
    eLoaderState        _eState  =  eLoaderState::NOT_STARTED;

private:
            void            xmlRequest          (void);
            void            imgRequest          (void);


public: //Callback!!
            void            private_cb_Loaded   (std::uintptr_t     id,
                                                 uint32_t           cx,
                                                 uint32_t           cy);
            void            private_cb_Failed   (eXMLFailReason     eReason);
            void            private_cb_Progress (float              fDone,
                                                 float              fTotal);

public:
    inline CLoaderPtr       getPtr              (void) {return shared_from_this(); }
                            CLoader             (const char*        lpccURL,
                                                 eResourceType      eType,
                                                 void*              pBuff,
                                                 size_t             nBuffSz,
                                                 IXMLReqCallback*   pCallback);
                           ~CLoader             (void);
            void            run                 (void);
            void*           getData             (void);
            void            setData             (void*              pBuff,
                                                 size_t             nSz);
            size_t          getDataSize         (void);
            bool            allocBuff           (size_t             sz);
            const char*     getStr              (void);
    inline  eResourceType   getResourceType     (void)                  { return _eResourceType; }
    inline  std::string&    getResourceURL      (void)                  { return _strURL;        }
    inline  uintptr_t       getResourceHandle   (void)                  { return _loadedId;      }
    inline  uint32_t        getImgWidth         (void)                  { return _cx;            }
    inline  uint32_t        getImgHeight        (void)                  { return _cy;            }
    inline  void            setResourceURL      (const char* lpccURL)   { _strURL = lpccURL;     }
    inline  eLoaderState    getState            (void)                  { return _eState;        }
};

typedef std::vector<CLoaderPtr> CLoaders;
typedef CLoaders::iterator      CLoadersIt;

_G2D_NAMESPACE_END_