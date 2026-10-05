#include "assetloader.h"
#include "gfx.h" 
#include "engine.h"

_G2D_NAMESPACE_BEGIN_

#ifdef TARGET_EMSCRIPTEN
extern "C" 
{
    EMSCRIPTEN_KEEPALIVE void* _asset_alloc(size_t size) { return malloc(size); }
    EMSCRIPTEN_KEEPALIVE void _asset_free(void* ptr) { free(ptr); }
    EMSCRIPTEN_KEEPALIVE void _process_progress(LPCTSTR url, double loaded, double total) 
    { 
        AssetLoader::instance()._onJsProgress(url, (float)loaded, (float)total); 
    }
    EMSCRIPTEN_KEEPALIVE void _process_success(LPCTSTR url, const uint8_t* data, size_t size) 
    { 
        AssetLoader::instance()._onJsSuccess(url, data, size); 
    }
    EMSCRIPTEN_KEEPALIVE void _process_error(LPCTSTR url, int status) 
    { 
        AssetLoader::instance()._onJsError(url, status); 
    }
}

EM_JS(void, js_fetch_asset, (LPCTSTR url_ptr, LPCTSTR version_ptr), {
    const url = UTF8ToString(url_ptr);
    const version = UTF8ToString(version_ptr);
    const dbName = "rix_assets_db";
    const storeName = "assets_" + version;

    (async () => {
        let data = null;
        let db = null;
        try {
            db = await new Promise((resolve, reject) => {
                const request = indexedDB.open(dbName, 1);
                request.onupgradeneeded = (e) => e.target.result.createObjectStore(storeName);
                request.onsuccess = () => resolve(request.result);
                request.onerror = () => reject();
                setTimeout(() => reject(), 1500);
            });
        } catch (e) {}

        try {
            if (!data) {
                const response = await fetch(url);
                if (!response.ok) throw new Error("HTTP " + response.status);
                const reader = response.body.getReader();
                const total = +response.headers.get('Content-Length') || 0;
                let loaded = 0; let chunks = [];

                const uPtr = __asset_alloc(lengthBytesUTF8(url) + 1);
                stringToUTF8(url, uPtr, lengthBytesUTF8(url) + 1);

                while(true) {
                    const {done, value} = await reader.read();
                    if (done) break;
                    chunks.push(value);
                    loaded += value.length;
                    __process_progress(uPtr, loaded, total);
                }
                __asset_free(uPtr);
                data = new Uint8Array(await new Blob(chunks).arrayBuffer());
            }

            const dPtr = __asset_alloc(data.length);
            const urlFinalPtr = __asset_alloc(lengthBytesUTF8(url) + 1);
            HEAPU8.set(data, dPtr);
            stringToUTF8(url, urlFinalPtr, lengthBytesUTF8(url) + 1);

            __process_success(urlFinalPtr, dPtr, data.length);
            __asset_free(urlFinalPtr);
        } catch (err) {
            const errUrlPtr = __asset_alloc(lengthBytesUTF8(url) + 1);
            stringToUTF8(url, errUrlPtr, lengthBytesUTF8(url) + 1);
            __process_error(errUrlPtr, 0);
            __asset_free(errUrlPtr);
        } finally {  }
    })();
});
#endif

AssetLoader::~AssetLoader()
{
#ifndef TARGET_EMSCRIPTEN
    {
        std::lock_guard<std::mutex> lk(_workerMutex);
        _workerStop = true;
    }
    _workerCv.notify_all();
    if (_worker.joinable())
        _worker.join();
#endif
}

void AssetLoader::addToLoaderQueue(LPCTSTR lpszUrl) 
{
    if (!lpszUrl || lpszUrl[0] == '\0') 
        return;
    std::string url = lpszUrl;
    _filesRequested.push_back(url);
    auto it = _cache.find(url);
    if (it != _cache.end() && it->second->loaded) 
    {
        if (it->second->data.size() != 0)
            return;
        else
            _cache.erase(it);
        it = _cache.end();
    }
    if (std::find(_queue.begin(), _queue.end(), url) != _queue.end()) 
        return;
    _queue.push_back(url);
    if (it == _cache.end()) 
        _cache[url] = std::make_unique<Asset>();
}

void AssetLoader::load(SimpleCallback cbComplete, FOnAssetLoaderProgress cbProgress, FOnAssetLoaderSingleLoaded cbSingle) 
{

    if (cbComplete) _completeCallbacks.push_back(cbComplete);
    if (cbProgress) _progressCallbacks.push_back(cbProgress);
    if (cbSingle) _singleLoadedCallbacks.push_back(cbSingle);

    for (const auto& url : _queue) 
    {
        if (std::find(_filesToWait.begin(), _filesToWait.end(), url) == _filesToWait.end()) 
        {
            _filesToWait.push_back(url);
        }
    }

    if (_filesToWait.empty()) 
    {
        for (auto&it:_filesRequested)
        {
            if (isImage(it))
                CGfx::getInstance()->uploadAsset(it.c_str());
        }
        _filesRequested.clear();
        auto cbs = std::move(_completeCallbacks);
        _completeCallbacks.clear();
        _progressCallbacks.clear();
        _singleLoadedCallbacks.clear();
        for (auto& cb : cbs) 
        {
            if (cb) 
                cb();
        }
        return;
    }

    if (_state == State::IDLE) 
        _state = State::PROCESSING;
}

void AssetLoader::clear() 
{
    if (_state == State::IDLE && _queue.empty()) 
    {
        _filesRequested.clear();
        _cache.clear();
        _filesToWait.clear();
        _completeCallbacks.clear();
        _progressCallbacks.clear();
        _singleLoadedCallbacks.clear();
        _bNeedToClear = false;
    } 
    else 
    {
        _bNeedToClear = true;
    }
}

void AssetLoader::update(float dt) 
{
#ifndef TARGET_EMSCRIPTEN
    // worker thread delivers events; all callbacks fire on the main thread
    drainEvents();
#endif

    if (_state == State::IDLE) 
    {
        if (_bNeedToClear) 
        { 
            _cache.clear(); 
            _bNeedToClear = false; 
        }
        return;
    }
    if (_state == State::RETRY_PAUSE) 
    {
        _retryTimer -= dt;
        if (_retryTimer <= 0) 
        { 
            _state = State::PROCESSING; 
            _currentFileLoading = false; 
        }
        return;
    }
    if (_state == State::PROCESSING && !_currentFileLoading && !_queue.empty()) 
    {
        processNext();
    }
    if (_queue.empty() && !_currentFileLoading && _filesToWait.empty()) 
    {
        _state = State::IDLE;
    }
}

void AssetLoader::_onJsSuccess(LPCTSTR url, const uint8_t* data, size_t size) {
    std::string s_url = url;
    auto it = _cache.find(s_url);
    if (it != _cache.end() && data && size > 0) 
    {
        if (isImage(s_url)) 
        {
            CGfx::getInstance()->uploadAsset(s_url.c_str(), data, size);
            it->second->loaded = true;

            it->second->data.clear();
            it->second->data.shrink_to_fit();
        } 
        else 
        {
            it->second->data.assign(data, data + size);
            it->second->loaded = true;
        }
    }
    _filesToWait.erase(std::remove(_filesToWait.begin(), _filesToWait.end(), s_url), _filesToWait.end());
    if (!_queue.empty() && _queue.front() == s_url) 
        _queue.pop_front();
    _currentFileLoading = false;

    for (auto& cb : _singleLoadedCallbacks) 
    {
        if (cb) 
            cb(url);
    }

    free((void*)data);

    if (_filesToWait.empty()) 
    {
        for (auto&it:_filesRequested)
        {
            if (isImage(it))
                CGfx::getInstance()->uploadAsset(it.c_str(), true);
        }        
        _filesRequested.clear();
        auto cbs = std::move(_completeCallbacks);
        _completeCallbacks.clear();
        _progressCallbacks.clear();
        _singleLoadedCallbacks.clear();
        for (auto& cb : cbs) 
        {
            if (cb) 
                cb();
        }
    }
}

void AssetLoader::_onJsProgress(LPCTSTR url, float loaded, float total) 
{
    for (auto& cb : _progressCallbacks) 
    {
        if (cb) 
            cb(url, loaded, total);
    }
}

void AssetLoader::processNext() 
{
    if (_queue.empty()) 
        return;
    _currentFileLoading = true;
#ifdef TARGET_EMSCRIPTEN
    js_fetch_asset(_queue.front().c_str(), ASSET_VERSION.c_str());
#else
    startWorker();
    {
        std::lock_guard<std::mutex> lk(_workerMutex);
        _workerQueue.push_back(_queue.front());
    }
    _workerCv.notify_one();
#endif
}

#ifndef TARGET_EMSCRIPTEN
void AssetLoader::startWorker()
{
    if (_workerStarted)
        return;
    _workerStarted = true;
    _workerStop = false;
    _worker = std::thread(&AssetLoader::workerLoop, this);
}

void AssetLoader::workerLoop()
{
    const size_t CHUNK = 256 * 1024;

    while (true)
    {
        std::string url;
        {
            std::unique_lock<std::mutex> lk(_workerMutex);
            _workerCv.wait(lk, [&]{ return _workerStop.load() || !_workerQueue.empty(); });
            if (_workerStop && _workerQueue.empty())
                return;
            url = std::move(_workerQueue.front());
            _workerQueue.pop_front();
        }

        std::FILE* file = std::fopen(url.c_str(), "rb");
        if (!file)
        {
            assert(false);
            pushLoadEvent(LoadResult{ url, nullptr, 0, 1 });
            continue;
        }

        std::fseek(file, 0, SEEK_END);
        long long total = std::ftell(file);
        std::rewind(file);
        if (total <= 0)
        {
            assert(false);
            std::fclose(file);
            pushLoadEvent(LoadResult{ url, nullptr, 0, 2 });
            continue;
        }

        uint8_t* buffer = static_cast<uint8_t*>(std::malloc(static_cast<size_t>(total)));
        if (!buffer)
        {
            assert(false);
            std::fclose(file);
            pushLoadEvent(LoadResult{ url, nullptr, 0, 3 });
            continue;
        }

        size_t loaded = 0;
        bool ok = true;
        while (loaded < static_cast<size_t>(total))
        {
            size_t want = std::min(CHUNK, static_cast<size_t>(total) - loaded);
            size_t got  = std::fread(buffer + loaded, 1, want, file);
            loaded += got;
            pushProgressEvent(url, static_cast<float>(loaded), static_cast<float>(total));
            if (got != want) { ok = false; break; }
        }
        std::fclose(file);

        if (ok)
        {
            // ownership of buffer moves to _onJsSuccess (called on main thread), which frees it
            pushLoadEvent(LoadResult{ url, buffer, loaded, 0 });
        }
        else
        {
            std::free(buffer);
            pushLoadEvent(LoadResult{ url, nullptr, 0, 4 });
        }
    }
}

void AssetLoader::pushLoadEvent(LoadResult&& res)
{
    std::lock_guard<std::mutex> lk(_eventsMutex);
    _loadEvents.emplace_back(std::move(res));
}

void AssetLoader::pushProgressEvent(std::string url, float loaded, float total)
{
    std::lock_guard<std::mutex> lk(_eventsMutex);
    _progressEvents.emplace_back(std::move(url), loaded, total);
}

void AssetLoader::drainEvents()
{
    std::vector<LoadResult> loads;
    std::vector<std::tuple<std::string, float, float>> progress;
    {
        std::lock_guard<std::mutex> lk(_eventsMutex);
        loads.swap(_loadEvents);
        progress.swap(_progressEvents);
    }

    for (auto& p : progress)
        _onJsProgress(std::get<0>(p).c_str(), std::get<1>(p), std::get<2>(p));

    for (auto& l : loads)
    {
        if (l.status == 0)
            _onJsSuccess(l.url.c_str(), l.data, l.size);
        else
            _onJsError(l.url.c_str(), l.status);
    }
}
#endif

bool AssetLoader::isLoaded(LPCTSTR lpszURI) 
{
    std::string s;
    auto sPath = std::format("{}/{}", Engine::getCfg().RES_DIR, lpszURI);
    auto it = _cache.find(sPath.c_str());
    return (it != _cache.end() && it->second->loaded);
}

void* AssetLoader::getLoadedFile(LPCTSTR lpszURI, int& nOutSize) 
{
    std::string s;
    auto sPath = std::format("{}/{}", Engine::getCfg().RES_DIR, lpszURI);
    auto it = _cache.find(sPath.c_str());
    if (it != _cache.end() && it->second->loaded) 
    {
        nOutSize = (int)it->second->data.size();
        return it->second->data.data();
    }
    nOutSize = 0; 
    return nullptr;
}

std::string AssetLoader::getLoadedFileStr(LPCTSTR lpszURI)
{
    std::string s;
    auto sPath = std::format("{}/{}", Engine::getCfg().RES_DIR, lpszURI);
    auto it = _cache.find(sPath.c_str());
    assert(it != _cache.end());
    if (it != _cache.end() && it->second->loaded) 
    {
        s.resize(it->second->data.size());
        std::memcpy(s.data(), it->second->data.data(), it->second->data.size());
    }
    return s;
}

bool AssetLoader::isImage(const std::string& url) 
{
    size_t isNoiseTex = url.find( "/noise.png");
    if (isNoiseTex != std::string::npos) 
        return false;

    size_t dot = url.find_last_of('.');
    if (dot == std::string::npos) 
        return false;
    std::string ext = url.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "webp");
}

void AssetLoader::_onJsError(LPCTSTR url, int status) 
{
    _state = State::RETRY_PAUSE;
    _retryTimer = RETRY_DELAY;
}

_G2D_NAMESPACE_END_
