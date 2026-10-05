#include "spriteloader.h"

#include "assetloader.h"
#include "engine.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

_G2D_NAMESPACE_BEGIN_

namespace
{
    struct SAtlasPage
    {
        std::string _sImage;
        int         _iWidth   = 0;
        int         _iHeight  = 0;
        bool        _bPMA     = false;
        CTexturePtr _pTexture;
    };

    struct SAtlasRegion
    {
        std::string _sName;
        size_t      _nPage    = 0;
        int         _iX       = 0;
        int         _iY       = 0;
        int         _iWidth   = 0;
        int         _iHeight  = 0;
        int         _iOffsetX = 0;
        int         _iOffsetY = 0;
        int         _iDegrees = 0;
        struct 
        {
            float u0 = 0;
            float u1 = 0;
            float v0 = 0;
            float v1 = 0;
        } uvs;
    };

    std::string trim(const std::string& str)
    {
        const char* ws = " \t\r\n";
        const std::string::size_type beg = str.find_first_not_of(ws);
        if (beg == std::string::npos)
            return std::string();

        const std::string::size_type end = str.find_last_not_of(ws);
        return str.substr(beg, end - beg + 1);
    }

    bool isBlank(const std::string& str)
    {
        return trim(str).empty();
    }

    bool isPropertyLine(const std::string& str)
    {
        return str.find(':') != std::string::npos;
    }

    bool splitKeyValue(const std::string& strLine,
                       std::string&       strKey,
                       std::string&       strValue)
    {
        const std::string::size_type pos = strLine.find(':');
        if (pos == std::string::npos)
            return false;

        strKey   = trim(strLine.substr(0, pos));
        strValue = trim(strLine.substr(pos + 1));
        return !strKey.empty();
    }

    std::vector<std::string> splitCommaList(const std::string& strValue)
    {
        std::vector<std::string> vRes;
        std::string::size_type posBeg = 0;

        for (;;)
        {
            const std::string::size_type posComma = strValue.find(',', posBeg);
            if (posComma == std::string::npos)
            {
                vRes.push_back(trim(strValue.substr(posBeg)));
                break;
            }

            vRes.push_back(trim(strValue.substr(posBeg, posComma - posBeg)));
            posBeg = posComma + 1;
        }

        return vRes;
    }

    bool parseInt(const std::string& strValue, int& iRes)
    {
        if (strValue.empty())
            return false;

        char* pEnd = NULL;
        const long iVal = std::strtol(strValue.c_str(), &pEnd, 10);
        if (pEnd == strValue.c_str() || *pEnd != '\0')
            return false;

        iRes = static_cast<int>(iVal);
        return true;
    }

    bool parseBool(const std::string& strValue, bool& bRes)
    {
        if (strValue == "true")
        {
            bRes = true;
            return true;
        }

        if (strValue == "false")
        {
            bRes = false;
            return true;
        }

        return false;
    }

    bool readTextLines(const std::string& strFilePath,
                       std::vector<std::string>& vLines)
    {
        const std::string strData =
            AssetLoader::instance().getLoadedFileStr(strFilePath.c_str());

        std::string::size_type posBeg = 0;
        while (posBeg <= strData.size())
        {
            const std::string::size_type posEnd = strData.find('\n', posBeg);
            vLines.push_back(trim(strData.substr(
                posBeg,
                posEnd == std::string::npos ? std::string::npos : posEnd - posBeg)));

            if (posEnd == std::string::npos)
                break;

            posBeg = posEnd + 1;
        }

        return true;
    }

    size_t skipBlankLines(const std::vector<std::string>& vLines, size_t nLine)
    {
        while (nLine < vLines.size() && isBlank(vLines[nLine]))
            ++nLine;

        return nLine;
    }

    std::string getDirectory(const std::string& strPath)
    {
        const std::string::size_type pos = strPath.find_last_of("/\\");
        if (pos == std::string::npos)
            return std::string();

        return strPath.substr(0, pos + 1);
    }

    bool parseAtlasFile(const std::string&         strFilePath,
                        std::vector<SAtlasPage>&   vPages,
                        std::vector<SAtlasRegion>& vRegions)
    {
        std::vector<std::string> vLines;
        if (!readTextLines(strFilePath, vLines))
            return false;

        size_t nLine = 0;
        while ((nLine = skipBlankLines(vLines, nLine)) < vLines.size())
        {
            SAtlasPage page;
            page._sImage = vLines[nLine++];

            while (nLine < vLines.size() &&
                   !isBlank(vLines[nLine]) &&
                   isPropertyLine(vLines[nLine]))
            {
                std::string strKey, strValue;
                if (!splitKeyValue(vLines[nLine], strKey, strValue))
                    return false;

                if (strKey == "size")
                {
                    const std::vector<std::string> v = splitCommaList(strValue);
                    if (v.size() != 2 ||
                        !parseInt(v[0], page._iWidth) ||
                        !parseInt(v[1], page._iHeight))
                        return false;
                }
                else if (strKey == "pma")
                {
                    if (!parseBool(strValue, page._bPMA))
                        return false;
                }

                ++nLine;
            }

            if (page._sImage.empty() ||
                page._iWidth <= 0 ||
                page._iHeight <= 0)
                return false;

            const size_t nPage = vPages.size();
            vPages.push_back(page);

            while (nLine < vLines.size() && !isBlank(vLines[nLine]))
            {
                if (isPropertyLine(vLines[nLine]))
                    return false;

                SAtlasRegion region;
                region._sName = vLines[nLine++];
                region._nPage = nPage;

                while (nLine < vLines.size() &&
                       !isBlank(vLines[nLine]) &&
                       isPropertyLine(vLines[nLine]))
                {
                    std::string strKey, strValue;
                    if (!splitKeyValue(vLines[nLine], strKey, strValue))
                        return false;

                    if (strKey == "bounds")
                    {
                        const std::vector<std::string> v = splitCommaList(strValue);
                        if (v.size() != 4 ||
                            !parseInt(v[0], region._iX) ||
                            !parseInt(v[1], region._iY) ||
                            !parseInt(v[2], region._iWidth) ||
                            !parseInt(v[3], region._iHeight))
                            return false;
                    }
                    else if (strKey == "xy")
                    {
                        const std::vector<std::string> v = splitCommaList(strValue);
                        if (v.size() != 2 ||
                            !parseInt(v[0], region._iX) ||
                            !parseInt(v[1], region._iY))
                            return false;
                    }
                    else if (strKey == "size")
                    {
                        const std::vector<std::string> v = splitCommaList(strValue);
                        if (v.size() != 2 ||
                            !parseInt(v[0], region._iWidth) ||
                            !parseInt(v[1], region._iHeight))
                            return false;
                    }
                    else if (strKey == "offset")
                    {
                        const std::vector<std::string> v = splitCommaList(strValue);
                        if (v.size() != 2 ||
                            !parseInt(v[0], region._iOffsetX) ||
                            !parseInt(v[1], region._iOffsetY))
                            return false;
                    }
                    else if (strKey == "offsets")
                    {
                        const std::vector<std::string> v = splitCommaList(strValue);
                        int iOrigWidth  = 0;
                        int iOrigHeight = 0;

                        if (v.size() != 4 ||
                            !parseInt(v[0], region._iOffsetX) ||
                            !parseInt(v[1], region._iOffsetY) ||
                            !parseInt(v[2], iOrigWidth) ||
                            !parseInt(v[3], iOrigHeight))
                            return false;
                    }
                    else if (strKey == "rotate")
                    {
                        if (strValue == "true")
                        {
                            region._iDegrees = 90;
                        }
                        else if (strValue == "false")
                        {
                            region._iDegrees = 0;
                        }
                        else
                        {
                            int iDegrees = 0;
                            if (!parseInt(strValue, iDegrees))
                                return false;

                            if (iDegrees != 0 && iDegrees != 90)
                                return false;

                            region._iDegrees = iDegrees;
                        }
                        if (region._iDegrees > 0)
                        {
                            assert(false && "Rotation is not supported");
                            return false;
                        }
                    }

                    ++nLine;
                }

                if (region._sName.empty() ||
                    region._iWidth <= 0 ||
                    region._iHeight <= 0)
                    return false;

				float fx      = region._iX;
				float fy      = region._iY;
				float fWidth  = region._iWidth;
				float fHeight = region._iHeight;
                auto&page     = vPages[region._nPage];
                region.uvs.u0 = (fx + 0.5f) / (float)page._iWidth;
                region.uvs.v0 = (fy + 0.5f) / (float)page._iHeight;
                if (region._iDegrees == 90)
                {
                    region.uvs.u1 = (fx + fHeight - 0.5f) / (float)page._iHeight;
                    region.uvs.v1 = (fy + fWidth  - 0.5f) / (float)page._iWidth;
                }
                else
                {
                    region.uvs.u1 = (fx + fWidth  - 0.5f) / (float)page._iWidth;
                    region.uvs.v1 = (fy + fHeight - 0.5f) / (float)page._iHeight;
                }

				
                vRegions.push_back(region);
            }
        }

        return !vPages.empty() && !vRegions.empty();
    }
    void fillSprite(const SAtlasPage&   page,
                    const SAtlasRegion& region,
                    CSprite*            pSpr)
    {
        pSpr->setTexture(page._pTexture);

        assert(region._iOffsetX == 0);
        assert(region._iOffsetY == 0);

        const int iImageWidth = region._iWidth;

        const int iImageHeight = region._iHeight;

        const float fLocalX  = static_cast<float>(region._iOffsetX);
        const float fLocalY  = static_cast<float>(region._iOffsetY);
        const float fLocalX2 = fLocalX + static_cast<float>(iImageWidth);
        const float fLocalY2 = fLocalY + static_cast<float>(iImageHeight);

        pSpr->_verts[CSprite::VERT_BLX] = fLocalX;
        pSpr->_verts[CSprite::VERT_BLY] = fLocalY2;

        pSpr->_verts[CSprite::VERT_ULX] = fLocalX;
        pSpr->_verts[CSprite::VERT_ULY] = fLocalY;

        pSpr->_verts[CSprite::VERT_URX] = fLocalX2;
        pSpr->_verts[CSprite::VERT_URY] = fLocalY;

        pSpr->_verts[CSprite::VERT_BRX] = fLocalX2;
        pSpr->_verts[CSprite::VERT_BRY] = fLocalY2;


        pSpr->_uvs[CSprite::VERT_BLX] = region.uvs.u0;
        pSpr->_uvs[CSprite::VERT_BLY] = region.uvs.v1;

        pSpr->_uvs[CSprite::VERT_ULX] = region.uvs.u0;
        pSpr->_uvs[CSprite::VERT_ULY] = region.uvs.v0;

        pSpr->_uvs[CSprite::VERT_URX] = region.uvs.u1;
        pSpr->_uvs[CSprite::VERT_URY] = region.uvs.v0;

        pSpr->_uvs[CSprite::VERT_BRX] = region.uvs.u1;
        pSpr->_uvs[CSprite::VERT_BRY] = region.uvs.v1;

        pSpr->_bIsLoaded = true;
    }
}

SpriteLoader* SpriteLoader::_instance = NULL;

SpriteLoader* SpriteLoader::getInstance()
{
    if (!_instance)
        _instance = new SpriteLoader();

    return _instance;
}

bool SpriteLoader::addAtlas(const char* lpccAtlas)
{
    assert(lpccAtlas);
    if (!lpccAtlas)
        return false;

    auto& cfg = Engine::getCfg();

    std::string strAtlasPath = cfg.DATA_DIR;
    strAtlasPath.append(lpccAtlas);
    strAtlasPath.append(".atlas");

    if (_atlasesLoaded.find(strAtlasPath) != _atlasesLoaded.end())
        return true;

    if (!AssetLoader::instance().isLoaded(strAtlasPath.c_str()))
    {
        assert(false);
        return false;
    }

    std::vector<SAtlasPage>   vPages;
    std::vector<SAtlasRegion> vRegions;

    if (!parseAtlasFile(strAtlasPath, vPages, vRegions))
        return false;

    const std::string strAtlasDir = getDirectory(strAtlasPath);

    for (size_t i = 0; i < vPages.size(); ++i)
    {
        const std::string strTexturePath =
            strAtlasDir + vPages[i]._sImage;

        vPages[i]._pTexture =
            CGfx::getInstance()->getTextureById(strTexturePath.c_str());

        if (!vPages[i]._pTexture)
        {
            assert(false);
            return false;
        }
    }

    for (size_t i = 0; i < vRegions.size(); ++i)
    {
        const SAtlasRegion& region = vRegions[i];
        const SAtlasPage&   page   = vPages[region._nPage];

        CSpritePtr pSpr = std::make_shared<CSprite>();
        fillSprite(page, region, pSpr.get());

        std::string strSprPath = region._sName;
        const auto insertRes =
            _map.insert(std::make_pair(std::move(strSprPath), pSpr));

        assert(insertRes.second);
        if (!insertRes.second)
            return false;
    }

    _atlasesLoaded.insert(strAtlasPath);
    return true;
}

CSpritePtr SpriteLoader::getSprite(const char* sprName)
{
    CSpritePtr pRes;
    assert(sprName);

    if (sprName)
    {
        const auto it = _map.find(sprName);
        assert(it != _map.end());

        if (it != _map.end())
        {
            pRes = it->second->cloneInitial();
            pRes->_sDebug = sprName;
        }
    }

    return pRes;
}

CSpritePtrConst SpriteLoader::getProtoSprite(const char* sprName)
{
    CSpritePtr pRes;
    assert(sprName);

    if (sprName)
    {
        const auto it = _map.find(sprName);
        assert(it != _map.end());

        if (it != _map.end())
        {
            pRes = it->second;
            pRes->_sDebug = sprName;
        }
    }

    return pRes;
}

_G2D_NAMESPACE_END_