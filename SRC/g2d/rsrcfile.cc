#include "rsrcfile.h"
#include "fileslist.h"
#include "assetloader.h"
#include "engine.h"

_G2D_NAMESPACE_BEGIN_

void RsrcFileGroup::reset()
{
    assert(_eState == eRsrcFileGroupState::PENDING || _eState == eRsrcFileGroupState::DONE);
    if (_eState == eRsrcFileGroupState::PENDING || _eState == eRsrcFileGroupState::DONE)
    {
        _eState       = eRsrcFileGroupState::PENDING;
        _nFilesLoaded = 0;
        _setFiles.clear();
    }
}
void RsrcFileGroup::addSpine(LPCTSTR lpccSpineName, int numOfPngs, bool bWithNormalMap)
{
    auto& cfg = Engine::getCfg();
    
    std::string strFilePath = "";//strFilePath = std::format("{}/", cfg.RES_DIR);
    strFilePath.append(lpccSpineName);
    
    std::string strFileAtlas(strFilePath);
    strFileAtlas.append(".atlas");
    std::string strFileJson(strFilePath);
    strFileJson.append(".json");
    
    for (int i = 1; i <= numOfPngs; i++)
    {
        if (i == 1)
        {
            if (bWithNormalMap)
            {
                std::string strFilePng(strFilePath);
                strFilePng.append("_n.png");
                add(strFilePng.c_str());
            }
            std::string strFilePng(strFilePath);
            strFilePng.append(".png");
            add(strFilePng.c_str());
        }
        else
        {
            assert(!bWithNormalMap && "Not implemented!");
            std::string strFilePng = std::format("{}/{}_{}.png", cfg.RES_DIR, lpccSpineName , i);
            add(strFilePng.c_str());
        }
    }
    add(strFileAtlas.c_str());
    add(strFileJson.c_str());
}

uint64_t  RsrcFileGroup::countTotalSize()
{
    uint64_t res = 0;
    if (_nTotalSizeToLoad.has_value())
    {
        res = *_nTotalSizeToLoad;
    }
    else
    {
        for (auto&it:_setFiles)
        {
            std::filesystem::path path(it);        
            auto infoIt = FilesList::get().find(path.filename().string());
            if (infoIt != FilesList::get().end())
            {
                res += infoIt->second;
            }
        }
    }
    return res;
}

void RsrcFileGroup::load(SimpleCallback cb, FncOnFileProgress cbOnProgress, FncOnFileLoaded fOnSingleLoaded)
{
    assert(cb);
    assert(_eState == eRsrcFileGroupState::PENDING);
    if (_eState == eRsrcFileGroupState::PENDING  && cb)
    {
        _nTotalSizeBytes = countTotalSize();
        _cb = cb;
        _eState = eRsrcFileGroupState::RUNNING;
        for (auto& it : _setFiles)
        {
            AssetLoader::instance().addToLoaderQueue(it.c_str());
        }
        AssetLoader::instance().load([this]{
            _eState = eRsrcFileGroupState::DONE;
            assert(_cb);
            if (_cb)
            {
                _cb();
                //_cb = {};
            }
        }, [this, cbOnProgress](LPCTSTR lpszFileName, float fDone, float fTotal){
            auto it = _map.find(lpszFileName);
            if (it == _map.end())
            {
                RsrcFileState st;
                auto insPair = _map.emplace(lpszFileName, st);
                it = insPair.first;
            }
            auto diff = fDone - it->second.nLoaded;
            _nTotalLoadedBytes += diff;
            it->second.nLoaded = fDone;
            if (cbOnProgress)
            {
                if (_nTotalSizeBytes)
                    cbOnProgress(lpszFileName, _nTotalLoadedBytes, _nTotalSizeBytes);
                else
                    cbOnProgress(lpszFileName, fDone, fTotal);
            }
        }, fOnSingleLoaded);
    }
}

_G2D_NAMESPACE_END_