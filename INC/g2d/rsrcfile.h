#pragma once

#include "g2d.h"
#include "loader.h"

_G2D_NAMESPACE_BEGIN_

typedef std::function<void(LPCTSTR lpszFileName)> FncOnFileLoaded;
typedef std::function<void(LPCTSTR lpszFileName, float fDone, float fTotal)> FncOnFileProgress;


enum class eRsrcFileGroupState
{
    PENDING,
    RUNNING,
    DONE
};

struct RsrcFileState
{
    int     nLoaded = 0;
};

using MapRsrcFileState = std::unordered_map<std::string, RsrcFileState>;

class RsrcFileGroup
{
public:

    inline void add(LPCTSTR lpszFilePath) 
    { 
        if (_eState == eRsrcFileGroupState::DONE)
               reset();
        assert(_eState == eRsrcFileGroupState::PENDING); 
        if (_eState == eRsrcFileGroupState::PENDING) 
            _setFiles.insert(lpszFilePath); 
    }
    void addSpine(LPCTSTR lpccSpineName, int numOfPngs = 1, bool bWithNormalMap = false);
    void load(SimpleCallback cb, FncOnFileProgress cbOnProgress = {}, FncOnFileLoaded fOnSingleLoaded = {});
    inline bool isLoaded() { return _eState == eRsrcFileGroupState::DONE; }
    inline bool isPending() { return _eState == eRsrcFileGroupState::PENDING; }
    void reset();
    uint64_t countTotalSize();
    void setSizeToLoad(int sz) {_nTotalSizeToLoad = sz;}
    bool empty() { return _setFiles.empty(); }
private:
    
private:
    eRsrcFileGroupState    _eState       = eRsrcFileGroupState::PENDING;
    std::set<std::string>  _setFiles;
    MapRsrcFileState       _map;
    int                    _nFilesLoaded      = 0;
    uint64_t               _nTotalLoadedBytes = 0;
    uint64_t               _nTotalSizeBytes;
    std::optional<int>     _nTotalSizeToLoad;
    SimpleCallback         _cb;
};

_G2D_NAMESPACE_END_