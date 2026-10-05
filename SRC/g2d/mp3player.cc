#include "mp3player.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#ifndef G2D_WEB_AUDIO
#include "miniaudio.h"
#endif

#include "engine.h"

_G2D_NAMESPACE_BEGIN_

#ifdef G2D_WEB_AUDIO

// ---------------------------------------------------------------------------
// Web implementation
// ---------------------------------------------------------------------------

EMSCRIPTEN_KEEPALIVE void _audioLoadCallback(void* callbackPtr, int success)
{
    void (*callback)(bool) = reinterpret_cast<void (*)(bool)>(callbackPtr);

    if (callback)
    {
        callback(static_cast<bool>(success));
    }
}

EMSCRIPTEN_KEEPALIVE void _audioTrackLoadedAndReadyToPlay(int bufferId)
{
    if (bufferId == MP3Player::get().current_buffer_id)
    {
        MP3Player::get().play();
    }
}

EMSCRIPTEN_KEEPALIVE void _playNextTrackInPlaylist()
{
    auto& player = MP3Player::get();

    if (!player._bPlaylistActive)
    {
        return;
    }

    player.emitMusic(
        MUSIC_EVT_TRACK_ENDED,
        audioInt(static_cast<int>(player.current_track_index))
    );

    player.unloadCurrentBuffer();
    player.current_track_index++;
    player.loadNextTrack();
}

void MP3Player::resumeAudioContext()
{
    if (!MP3Player::get()._bAudioContextUnlocked)
    {
        std::cout << "MP3Player::resumeAudioContext()" << std::endl;

        MP3Player::get()._bAudioContextUnlocked = true;

        EM_ASM({
            if (window.GameAudio && window.GameAudio.audioContext)
            {
                if (window.GameAudio.audioContext.state === 'suspended' ||
                    window.GameAudio.audioContext.state === 'interrupted')
                {
                    window.GameAudio.audioContext.resume().then(() => {
                        console.log("Audio Context resumed successfully on user interaction.");
                    });
                }
            }
        });
    }
}

void MP3Player::update(float dt)
{
    if (_bPendingLoadNextTrack && _bPlaylistActive)
    {
        loadNextTrack();
    }

    if (_bCrossfading)
    {
        _crossfadeTimer += dt;

        if (_crossfadeTimer >= _crossfadeDuration)
        {
            _bCrossfading = false;

            EM_ASM({
                if (window.GameAudio.outgoingSource)
                {
                    try
                    {
                        window.GameAudio.outgoingSource.stop();
                    }
                    catch (e) {}

                    window.GameAudio.outgoingSource = null;
                    window.GameAudio.outgoingGain = null;
                }
            });

            emitMusic(MUSIC_EVT_CROSSFADE_FINISHED);
        }
    }

    if (_bNeedsSync)
    {
        _fSyncTimer -= dt;

        if (_fSyncTimer <= 0)
        {
            _bNeedsSync = false;
            syncPlaybackState();
        }
    }
}

void MP3Player::init()
{
    static bool isInitDone = false;

    assert(!isInitDone);

    if (!isInitDone)
    {
        isInitDone = true;

        //_IsAdShowing = !ExtApi::getInstance().isShowInterAdDone();
        //_isBlured = !InputController::getInstance()->isFocused();

        InputController::getInstance()->addListener(this);
        //ExtApi::getInstance().addListener(this);
        //CApp::getInstance()->addListener(this);
    }
}

void MP3Player::tryStartMusic()
{
    if (MP3Player::get().isContextUnlocked() && !_bIsMusicStarted && !_isBlured && !_IsAdShowing)
    {
        EM_ASM({
            console.log("!!!!!!!!MP3Player::tryStartMusic() started");
        });

        _bIsMusicStarted = true;
        playMusic();
    }
}

void MP3Player::playMusic()
{
    if (isContextUnlocked())
    {
        playPlaylist([this] {
            playMusic();
        });
    }
}

void MP3Player::onEvent(int nEvent, void* pData1, void* pData2, void* pData3)
{
    switch (nEvent)
    {
        case InputController::EVT_ON_BLUR:
        case InputController::EVT_ON_FOCUS:
        {
            _isBlured = !InputController::getInstance()->isFocused();

            if (!_isBlured)
            {
                _bNeedsSync = true;
                _fSyncTimer = 1.0f;
            }
            else
            {
                syncPlaybackState();
            }
        }
        break;

        default:
            break;
    }
}

void MP3Player::syncPlaybackState()
{
    _bNeedsSync = false;

    if (!_bIsMusicStarted)
    {
        tryStartMusic();
    }
    else
    {
        if (_IsAdShowing || _isBlured)
        {
            pause();
        }
        else
        {
            play();
        }
    }
}

void MP3Player::addToPlaylist(const std::string& url)
{
    playlist_urls.push_back(url);

    if (_bPendingLoadNextTrack && _bPlaylistActive)
    {
        loadNextTrack();
    }
}

void MP3Player::clearPlaylist(bool stopImmediately)
{
    _bPlaylistActive = false;
    _bPendingLoadNextTrack = false;

    if (stopImmediately)
    {
        stop();
    }

    playlist_urls.clear();
    current_track_index = 0;
    _onPlaylistComplete = nullptr;
    _bIsMusicStarted = false;
    _bCrossfading = false;
    _crossfadeTimer = 0;
}

void MP3Player::crossfadeToPlaylist(
    const std::vector<std::string>& newPlaylist,
    float fadeDurationSec
)
{
    if (playlist_urls.empty() || current_buffer_id == 0)
    {
        playlist_urls = newPlaylist;
        current_track_index = 0;

        _bIsMusicStarted = true;
        _bPlaylistActive = true;
        _bPendingLoadNextTrack = false;

        emitMusic(MUSIC_EVT_PLAYLIST_STARTED);

        loadNextTrack();
        return;
    }

    _bCrossfading = true;
    _crossfadeDuration = fadeDurationSec;
    _crossfadeTimer = 0;

    EM_ASM({
        var duration = $0;
        var context = window.GameAudio.audioContext;
        var now = context.currentTime;

        if (window.GameAudio.activeGain)
        {
            window.GameAudio.activeGain.gain.cancelScheduledValues(now);
            window.GameAudio.activeGain.gain.setValueAtTime(
                window.GameAudio.activeGain.gain.value,
                now
            );

            window.GameAudio.activeGain.gain.linearRampToValueAtTime(
                0,
                now + duration
            );

            window.GameAudio.outgoingGain = window.GameAudio.activeGain;
            window.GameAudio.outgoingSource = window.GameAudio.activeSource;

            window.GameAudio.activeGain = null;
            window.GameAudio.activeSource = null;
        }

        window.GameAudio.playbackTime = 0;
    }, fadeDurationSec);

    playlist_urls = newPlaylist;
    current_track_index = 0;
    _bPlaylistActive = true;

    emitMusic(
        MUSIC_EVT_CROSSFADE_STARTED,
        audioInt(static_cast<int>(fadeDurationSec * 1000.0f))
    );

    loadNextTrack();
}

void MP3Player::loadNextTrack()
{
    if (!_bPlaylistActive)
    {
        _bPendingLoadNextTrack = false;
        return;
    }

    if (playlist_urls.empty())
    {
        _bPendingLoadNextTrack = true;
        return;
    }

    _bPendingLoadNextTrack = false;

    if (current_track_index >= playlist_urls.size())
    {
        std::cout << "Playlist finished." << std::endl;

        emitMusic(MUSIC_EVT_PLAYLIST_FINISHED);

        if (_onPlaylistComplete)
        {
            _onPlaylistComplete();
        }

        return;
    }

    std::string url = playlist_urls[current_track_index];

    auto sRealUrl = std::format("{}/{}", Engine::getCfg().RES_DIR, url);

    std::cout << "Loading track "
              << (current_track_index + 1) << "/"
              << playlist_urls.size() << ": "
              << url << std::endl;

    emitMusic(
        MUSIC_EVT_TRACK_LOADING,
        audioInt(static_cast<int>(current_track_index)),
        const_cast<char*>(url.c_str())
    );

    loadAsync(sRealUrl, [](bool success) {
        auto& player = MP3Player::get();

        if (player.current_track_index >= player.playlist_urls.size())
        {
            return;
        }

        const std::string& currentUrl =
            player.playlist_urls[player.current_track_index];

        if (success)
        {
            player.emitMusic(
                MUSIC_EVT_TRACK_LOADED,
                audioInt(static_cast<int>(player.current_track_index)),
                const_cast<char*>(currentUrl.c_str())
            );
        }
        else
        {
            player.emitMusic(
                MUSIC_EVT_TRACK_LOAD_FAILED,
                audioInt(static_cast<int>(player.current_track_index)),
                const_cast<char*>(currentUrl.c_str())
            );
        }
    });
}

void MP3Player::loadAsync(const std::string& url, void (*callback)(bool success))
{
    current_buffer_id = EM_ASM_INT({
        var url = UTF8ToString($0);
        var callbackPtr = $1;
        var bufferId = window.GameAudio.bufferCounter++;

        fetch(url)
            .then(response => response.arrayBuffer())
            .then(arrayBuffer => {
                return window.GameAudio.audioContext.decodeAudioData(arrayBuffer);
            })
            .then(audioBuffer => {
                window.GameAudio.buffers[bufferId] = audioBuffer;

                __audioLoadCallback(callbackPtr, 1);
                __audioTrackLoadedAndReadyToPlay(bufferId);
            })
            .catch(error => {
                console.error("Error loading audio:", error);
                __audioLoadCallback(callbackPtr, 0);
            });

        return bufferId;
    }, url.c_str(), callback);
}

void MP3Player::play()
{
    if (_bMuted)
    {
        return;
    }

    if (current_buffer_id == 0)
    {
        std::cerr << "Error: No audio loaded yet." << std::endl;
        return;
    }

    EM_ASM({
        var context = window.GameAudio.audioContext;
        var bufferId = $0;
        var isCrossfade = $1;
        var fadeDuration = $2;

        var audioBuffer = window.GameAudio.buffers[bufferId];

        if (context && audioBuffer)
        {
            if (context.state === 'suspended')
            {
                context.resume();
            }

            if (!isCrossfade && window.GameAudio.activeSource)
            {
                window.GameAudio.activeSource.stop();
                window.GameAudio.activeSource = null;
                window.GameAudio.activeGain = null;
            }

            var source = context.createBufferSource();
            source.buffer = audioBuffer;

            var gainNode = context.createGain();

            if (isCrossfade && fadeDuration > 0)
            {
                gainNode.gain.setValueAtTime(0, context.currentTime);
                gainNode.gain.linearRampToValueAtTime(
                    1.0,
                    context.currentTime + fadeDuration
                );
            }
            else
            {
                gainNode.gain.value = 1.0;
            }

            source.connect(gainNode);
            gainNode.connect(window.GameAudio.musicMasterGain || context.destination);

            var offset = window.GameAudio.playbackTime || 0;
            source.start(0, offset);

            window.GameAudio.activeSource = source;
            window.GameAudio.activeGain = gainNode;
            window.GameAudio.startTime = context.currentTime - offset;

            source.onended = function() {
                if (window.GameAudio.activeSource === source)
                {
                    window.GameAudio.playbackTime = 0;
                    window.GameAudio.activeSource = null;
                    window.GameAudio.activeGain = null;

                    __playNextTrackInPlaylist();
                }
            };
        }
    }, current_buffer_id, _bCrossfading ? 1 : 0, _crossfadeDuration);

    emitMusic(MUSIC_EVT_PLAY, audioInt(current_buffer_id));
}

void MP3Player::pause()
{
    if (current_buffer_id == 0)
    {
        return;
    }

    EM_ASM({
        var context = window.GameAudio.audioContext;
        var source = window.GameAudio.activeSource;

        if (source)
        {
            source.stop();

            window.GameAudio.playbackTime =
                context.currentTime - window.GameAudio.startTime;

            window.GameAudio.activeSource = null;
            window.GameAudio.activeGain = null;

            console.log("Paused at:", window.GameAudio.playbackTime, "seconds");
        }
    });

    emitMusic(MUSIC_EVT_PAUSE);
}

void MP3Player::stop()
{
    bool bWasActive = _bPlaylistActive || current_buffer_id != 0;

    _bPlaylistActive = false;
    _bPendingLoadNextTrack = false;

    if (current_buffer_id == 0)
    {
        return;
    }

    EM_ASM({
        var source = window.GameAudio.activeSource;

        if (source)
        {
            source.stop();
            window.GameAudio.activeSource = null;
        }

        window.GameAudio.activeGain = null;
        window.GameAudio.playbackTime = 0;
        window.GameAudio.startTime = 0;
    });

    if (bWasActive)
    {
        emitMusic(MUSIC_EVT_STOP);
    }
}

void MP3Player::unloadCurrentBuffer()
{
    EM_ASM({
        var bufferId = $0;

        if (window.GameAudio.buffers[bufferId])
        {
            delete window.GameAudio.buffers[bufferId];
            console.log("Buffer ID " + bufferId + " freed from memory.");
        }

        if (window.GameAudio.activeSource)
        {
            try
            {
                window.GameAudio.activeSource.stop();
            }
            catch (e) {}

            window.GameAudio.activeSource = null;
        }

        window.GameAudio.activeGain = null;
    }, current_buffer_id);
}

void MP3Player::setMuted(bool bMuted)
{
    if (_bMuted != bMuted)
    {
        _bMuted = bMuted;

        if (bMuted)
        {
            pause();
        }
        else
        {
            play();
        }

        emitMusic(MUSIC_EVT_MUTE_CHANGED, audioInt(bMuted ? 1 : 0));
    }
}

void MP3Player::setMasterVolume(float volume)
{
    _masterVolume = std::clamp(volume, 0.f, 1.f);
    setBusVolume(_masterVolume);
}

void MP3Player::setBusVolume(float volume)
{
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    _busVolume = volume;

    EM_ASM({
        try
        {
            if (!window.GameAudio.musicMasterGain)
            {
                var context = window.GameAudio.audioContext;

                if (context)
                {
                    window.GameAudio.musicMasterGain = context.createGain();
                    window.GameAudio.musicMasterGain.connect(context.destination);
                }
            }

            if (window.GameAudio.musicMasterGain)
            {
                window.GameAudio.musicMasterGain.gain.value = $0;
            }
        }
        catch (e)
        {
            console.error("MP3Player.setBusVolume failed:", e);
        }
    }, volume);
}

#else // Native

// ---------------------------------------------------------------------------
// Windows / miniaudio implementation
// ---------------------------------------------------------------------------

MP3Player::MP3Player()
    : current_buffer_id(0)
    , _bAudioContextUnlocked(true)
{
}

MP3Player::~MP3Player()
{
    stop();

    if (_ownsEngine && _engine)
    {
        ma_engine_uninit(_engine);
        delete _engine;
        _engine = nullptr;
    }
}

void MP3Player::init()
{
}

void MP3Player::tryStartMusic()
{
    if (isContextUnlocked() && !_bIsMusicStarted)
    {
        _bIsMusicStarted = true;
        playMusic();
    }
}

void MP3Player::playMusic()
{
    if (isContextUnlocked())
    {
        playPlaylist([this] {
            playMusic();
        });
    }
}

void MP3Player::syncPlaybackState()
{
    if (!_bIsMusicStarted)
    {
        tryStartMusic();
    }
}

void MP3Player::addToPlaylist(const std::string& url)
{
    playlist_urls.push_back(url);

    if (_bPendingLoadNextTrack && _bPlaylistActive)
    {
        loadNextTrack();
    }
}

void MP3Player::clearPlaylist(bool stopImmediately)
{
    _bPlaylistActive = false;
    _bPendingLoadNextTrack = false;

    if (stopImmediately)
    {
        stop();
    }

    playlist_urls.clear();
    current_track_index = 0;
    _onPlaylistComplete = nullptr;
    _bIsMusicStarted = false;
    _bCrossfading = false;
    _crossfadeTimer = 0;
}

void MP3Player::crossfadeToPlaylist(
    const std::vector<std::string>& newPlaylist,
    float fadeDurationSec
)
{
    if (playlist_urls.empty() || !_currentSound || !_isPlaying)
    {
        playlist_urls = newPlaylist;
        current_track_index = 0;

        _bIsMusicStarted = true;
        _bPlaylistActive = true;
        _bPendingLoadNextTrack = false;

        playPlaylist([this] {
            playMusic();
        });

        return;
    }

    _bCrossfading = true;
    _crossfadeDuration = fadeDurationSec;
    _crossfadeTimer = 0;

    _outgoingSound = _currentSound;
    _currentSound = nullptr;

    _isPlaying = false;
    _isPaused = false;

    playlist_urls = newPlaylist;
    current_track_index = 0;
    _bPlaylistActive = true;

    emitMusic(
        MUSIC_EVT_CROSSFADE_STARTED,
        audioInt(static_cast<int>(fadeDurationSec * 1000.0f))
    );

    loadNextTrack();
}

void MP3Player::playPlaylist(SimpleCallback completeCallback)
{
    _onPlaylistComplete = completeCallback;
    current_track_index = 0;
    _bPlaylistActive = true;
    _bPendingLoadNextTrack = false;

    emitMusic(MUSIC_EVT_PLAYLIST_STARTED);

    loadNextTrack();
}

void MP3Player::loadNextTrack()
{
    if (!_bPlaylistActive)
    {
        _bPendingLoadNextTrack = false;
        return;
    }

    if (playlist_urls.empty())
    {
        _bPendingLoadNextTrack = true;
        return;
    }

    _bPendingLoadNextTrack = false;

    if (current_track_index >= playlist_urls.size())
    {
        std::cout << "Playlist finished." << std::endl;

        emitMusic(MUSIC_EVT_PLAYLIST_FINISHED);

        if (_onPlaylistComplete)
        {
            _onPlaylistComplete();
        }

        return;
    }

    std::string url = playlist_urls[current_track_index];

    auto sRealUrl = std::format("{}/{}", Engine::getCfg().RES_DIR, url);

    std::cout << "Loading track "
              << (current_track_index + 1) << "/"
              << playlist_urls.size() << ": "
              << url << std::endl;

    emitMusic(
        MUSIC_EVT_TRACK_LOADING,
        audioInt(static_cast<int>(current_track_index)),
        const_cast<char*>(url.c_str())
    );

    loadAsync(sRealUrl, [](bool success) {
        auto& player = get();

        if (player.current_track_index >= player.playlist_urls.size())
        {
            return;
        }

        const std::string& currentUrl =
            player.playlist_urls[player.current_track_index];

        if (success)
        {
            player.emitMusic(
                MUSIC_EVT_TRACK_LOADED,
                audioInt(static_cast<int>(player.current_track_index)),
                const_cast<char*>(currentUrl.c_str())
            );
        }
        else
        {
            player.emitMusic(
                MUSIC_EVT_TRACK_LOAD_FAILED,
                audioInt(static_cast<int>(player.current_track_index)),
                const_cast<char*>(currentUrl.c_str())
            );
        }
    });
}

void MP3Player::loadAsync(const std::string& url, void (*callback)(bool success))
{
    current_buffer_id++;
    _currentUrl = url;

    play();

    if (callback)
    {
        callback(_currentSound != nullptr);
    }
}

void MP3Player::play()
{
    if (_bMuted)
    {
        return;
    }

    if (_isPaused && _currentSound)
    {
        ma_sound_seek_to_pcm_frame(_currentSound, _pauseFrame);
        ma_sound_start(_currentSound);

        _isPaused = false;

        emitMusic(MUSIC_EVT_PLAY, audioInt(current_buffer_id));
        return;
    }

    if (_currentUrl.empty() && current_track_index < playlist_urls.size())
    {
        _currentUrl = playlist_urls[current_track_index];
    }

    if (_currentUrl.empty())
    {
        std::cerr << "MP3Player: no audio loaded" << std::endl;
        return;
    }

    if (_currentSound)
    {
        ma_sound_stop(_currentSound);
        ma_sound_uninit(_currentSound);
        delete _currentSound;
        _currentSound = nullptr;
    }

    if (!_engine)
    {
        _engine = new ma_engine();

        if (ma_engine_init(nullptr, _engine) != MA_SUCCESS)
        {
            std::cerr << "MP3Player: failed to init audio engine" << std::endl;

            delete _engine;
            _engine = nullptr;

            return;
        }

        _ownsEngine = true;
    }

    _currentSound = new ma_sound();

    ma_result result = ma_sound_init_from_file(
        _engine,
        _currentUrl.c_str(),
        MA_SOUND_FLAG_STREAM,
        nullptr,
        nullptr,
        _currentSound
    );

    if (result != MA_SUCCESS)
    {
        std::cerr << "MP3Player: failed to load sound: "
                  << _currentUrl
                  << " (error: " << result << ")"
                  << std::endl;

        delete _currentSound;
        _currentSound = nullptr;

        return;
    }

    float baseVol = _bMuted ? 0.0f : _busVolume;

    if (_bCrossfading)
    {
        ma_sound_set_volume(_currentSound, 0.0f);
    }
    else
    {
        ma_sound_set_volume(_currentSound, baseVol);
    }

    ma_sound_start(_currentSound);

    _isPlaying = true;
    _isPaused = false;
    _pauseFrame = 0;

    emitMusic(MUSIC_EVT_PLAY, audioInt(current_buffer_id));
}

void MP3Player::pause()
{
    if (_currentSound && _isPlaying && !_isPaused)
    {
        ma_sound_get_cursor_in_pcm_frames(_currentSound, &_pauseFrame);
        ma_sound_stop(_currentSound);

        _isPaused = true;

        emitMusic(MUSIC_EVT_PAUSE);
    }
}

void MP3Player::stop()
{
    bool bEmitStop =
        _bPlaylistActive ||
        _currentSound != nullptr ||
        _outgoingSound != nullptr;

    _bPlaylistActive = false;
    _bPendingLoadNextTrack = false;

    unloadCurrentBuffer();

    _bCrossfading = false;
    _crossfadeTimer = 0;

    if (bEmitStop)
    {
        emitMusic(MUSIC_EVT_STOP);
    }
}

void MP3Player::unloadCurrentBuffer()
{
    if (_currentSound)
    {
        ma_sound_stop(_currentSound);
        ma_sound_uninit(_currentSound);
        delete _currentSound;
        _currentSound = nullptr;
    }

    if (_outgoingSound)
    {
        ma_sound_stop(_outgoingSound);
        ma_sound_uninit(_outgoingSound);
        delete _outgoingSound;
        _outgoingSound = nullptr;
    }

    _isPlaying = false;
    _isPaused = false;
    _pauseFrame = 0;
}

void MP3Player::setMuted(bool bMuted)
{
    if (_bMuted != bMuted)
    {
        _bMuted = bMuted;

        applyMusicVolume();

        if (bMuted)
        {
            pause();
        }
        else
        {
            play();
        }

        emitMusic(MUSIC_EVT_MUTE_CHANGED, audioInt(bMuted ? 1 : 0));
    }
}

void MP3Player::setMasterVolume(float volume)
{
    _masterVolume = std::clamp(volume, 0.f, 1.f);
    setBusVolume(_masterVolume);
}

void MP3Player::setBusVolume(float volume)
{
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    _busVolume = volume;
    applyMusicVolume();
}

void MP3Player::applyMusicVolume()
{
    float baseVolume = _bMuted ? 0.0f : _busVolume;

    if (_bCrossfading && _crossfadeDuration > 0.0f)
    {
        float t = _crossfadeTimer / _crossfadeDuration;

        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        if (_currentSound)
        {
            ma_sound_set_volume(_currentSound, baseVolume * t);
        }

        if (_outgoingSound)
        {
            ma_sound_set_volume(_outgoingSound, baseVolume * (1.0f - t));
        }
    }
    else
    {
        if (_currentSound)
        {
            ma_sound_set_volume(_currentSound, baseVolume);
        }

        if (_outgoingSound)
        {
            ma_sound_set_volume(_outgoingSound, 0.0f);
        }
    }
}

void MP3Player::attachEngine(ma_engine* pEngine)
{
    if (_ownsEngine && _engine && _engine != pEngine)
    {
        stop();

        ma_engine_uninit(_engine);
        delete _engine;
        _engine = nullptr;
    }

    _engine = pEngine;
    _ownsEngine = false;
}

void MP3Player::resumeAudioContext()
{
    get()._bAudioContextUnlocked = true;
}

bool MP3Player::isContextUnlocked()
{
    return _bAudioContextUnlocked;
}

void MP3Player::update(float dt)
{
    if (_bPendingLoadNextTrack && _bPlaylistActive)
    {
        loadNextTrack();
    }

    if (_bCrossfading)
    {
        _crossfadeTimer += dt;

        float t = std::min(_crossfadeTimer / _crossfadeDuration, 1.0f);
        float baseVolume = _bMuted ? 0.0f : _busVolume;

        if (_outgoingSound)
        {
            ma_sound_set_volume(_outgoingSound, baseVolume * (1.0f - t));
        }

        if (_currentSound)
        {
            ma_sound_set_volume(_currentSound, baseVolume * t);
        }

        if (t >= 1.0f)
        {
            _bCrossfading = false;

            if (_outgoingSound)
            {
                ma_sound_stop(_outgoingSound);
                ma_sound_uninit(_outgoingSound);
                delete _outgoingSound;
                _outgoingSound = nullptr;
            }

            if (_currentSound)
            {
                ma_sound_set_volume(_currentSound, baseVolume);
            }

            emitMusic(MUSIC_EVT_CROSSFADE_FINISHED);
        }
    }
    else if (_isPlaying && !_isPaused && _currentSound)
    {
        if (ma_sound_is_playing(_currentSound) == MA_FALSE)
        {
            _isPlaying = false;

            emitMusic(
                MUSIC_EVT_TRACK_ENDED,
                audioInt(static_cast<int>(current_track_index))
            );

            unloadCurrentBuffer();

            current_track_index++;
            loadNextTrack();
        }
    }
}

#endif // G2D_WEB_AUDIO

_G2D_NAMESPACE_END_