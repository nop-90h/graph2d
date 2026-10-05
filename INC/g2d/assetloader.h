#pragma once

#include "g2d.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <deque>
#include <tuple>

_G2D_NAMESPACE_BEGIN_

typedef std::function<void(LPCTSTR, float, float)>              FOnAssetLoaderProgress;
typedef std::function<void(LPCTSTR)>                            FOnAssetLoaderSingleLoaded;

namespace
{
    struct Asset 
    {
        std::vector<uint8_t>    data;
        bool                    loaded = false;
    };

    typedef std::unordered_map<std::string, std::unique_ptr<Asset>> AssetCache;
}

class AssetLoader {
public:
                            ~AssetLoader        (void);
            void            addToLoaderQueue    (LPCTSTR                    lpszUrl);    
            void            load                (SimpleCallback             cbOnComplete = nullptr,
                                                 FOnAssetLoaderProgress     cbOnProgress = nullptr,
                                                 FOnAssetLoaderSingleLoaded cbOnSingleLoaded = nullptr);
            void            clear               (void);
            void            update              (float                      dt);
            bool            isLoaded            (LPCTSTR                    lpszURI);
            void*           getLoadedFile       (LPCTSTR                    lpszURI, 
                                                 int&                       nOutSize);
            std::string     getLoadedFileStr    (LPCTSTR                    lpszURI);
            void            _onJsSuccess        (LPCTSTR                    url, 
                                                 const uint8_t*             data, 
                                                 size_t                     size);
            void            _onJsProgress       (LPCTSTR                    url, 
                                                 float                      loaded, 
                                                 float                      total);
            void            _onJsError          (LPCTSTR                    url, 
                                                 int                        status);
    static  AssetLoader&    instance            (void) 
    { 
        static AssetLoader inst; 
        return inst; 
    }

private:
                            AssetLoader         (void) = default;
            void            processNext         (void);
            bool            isImage             (const std::string&         url);

#ifndef TARGET_EMSCRIPTEN
    // --- native threaded loader ---
    struct LoadResult {
        std::string         url;
        uint8_t*            data    = nullptr;  // malloc'ed in worker, freed by _onJsSuccess
        size_t              size    = 0;
        int                 status  = 0;        // 0 = ok, otherwise error code
    };

            void            startWorker         (void);
            void            workerLoop          (void);
            void            pushLoadEvent       (LoadResult&&               res);
            void            pushProgressEvent   (std::string                url,
                                                 float                      loaded,
                                                 float                      total);
            void            drainEvents         (void);
#endif

private:
    enum class State { IDLE, PROCESSING, RETRY_PAUSE };

private:
    State                                   _state = State::IDLE;
    std::deque<std::string>                 _queue;
    AssetCache                              _cache;

    Strings                                 _filesToWait;
    Strings                                 _filesRequested;

    std::vector<SimpleCallback>             _completeCallbacks;
    std::vector<FOnAssetLoaderProgress>     _progressCallbacks;
    std::vector<FOnAssetLoaderSingleLoaded> _singleLoadedCallbacks;
    bool                                    _currentFileLoading        = false;
    bool                                    _bNeedToClear              = false;
    float                                   _retryTimer                = 0.0f;

#ifndef TARGET_EMSCRIPTEN
    std::thread                             _worker;
    std::mutex                              _workerMutex;
    std::condition_variable                 _workerCv;
    std::deque<std::string>                 _workerQueue;
    std::atomic<bool>                       _workerStop                { false };
    bool                                    _workerStarted             = false;

    std::mutex                              _eventsMutex;
    std::vector<LoadResult>                 _loadEvents;
    std::vector<std::tuple<std::string, float, float>> _progressEvents;
#endif

private:
    const float                             RETRY_DELAY                = 3.0f;
    inline static const std::string         ASSET_VERSION              = "v4";
};

_G2D_NAMESPACE_END_
