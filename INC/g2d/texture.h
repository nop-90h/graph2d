#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

class CTexture;
typedef std::shared_ptr<CTexture> CTexturePtr;

class CTexture : public std::enable_shared_from_this<CTexture>
{
    friend class CGfx;
private:
        int             _sampler;
        GLuint          _handle;
        uint32_t        _cx;
        uint32_t        _cy;
        std::string     _path;
        bool            _bIsInternal = false;
        float           _fLastUse    = 0;
        bool            _bIsUploded  = false;

private:
    inline  void            setSampler          (int sampler)           { _sampler = sampler; }
    inline  void            unsetSampler        (void)                  { _sampler = -1;      }
    inline  int             getSampler          (void)                  { return _sampler;    }
    inline  void            setInternal         (bool bIsInternal)      { _bIsInternal = bIsInternal; }
            void            updateLastUse       (void);
    inline  void            setUploaded         (bool bIsUp = true)     { _bIsUploded = bIsUp; }

public:
                            CTexture            (void);
    inline  CTexturePtr     getPtr              (void)                  { return shared_from_this();}
    inline  void            setPath             (const char* lpccPath)  { _path = lpccPath; }
    inline  const char*     getPath             (void)                  { return _path.c_str(); }
    inline  float           getCx               (void)                  { return static_cast<float>(_cx); }
    inline  float           getCy               (void)                  { return static_cast<float>(_cy); }
    inline  bool            isUploaded          (void)                  { return _bIsUploded;  }
    virtual                 ~CTexture           (void);
};

typedef std::vector<CTexturePtr>  CTextures;
typedef CTextures::iterator CTexturesIt;
typedef std::unordered_map<std::string, CTexturePtr> MapTextures;
typedef MapTextures::iterator MapTexturesIt;
typedef std::unordered_set<CTexturePtr> SetTextures;

_G2D_NAMESPACE_END_