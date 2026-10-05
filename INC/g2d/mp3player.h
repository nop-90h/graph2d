#pragma once

#include "g2d.h"
#include "audioevents.h"

#ifdef G2D_WEB_AUDIO

#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <cassert>

#include <emscripten.h>
#include <emscripten/html5.h>

#include "eventemmiter.h"
#include "inputcontroller.h"

_G2D_NAMESPACE_BEGIN_

extern "C"
{
    EMSCRIPTEN_KEEPALIVE void _audioLoadCallback(void* callbackPtr, int success);
    EMSCRIPTEN_KEEPALIVE void _audioTrackLoadedAndReadyToPlay(int bufferId);
    EMSCRIPTEN_KEEPALIVE void _playNextTrackInPlaylist();
}

class MP3Player : public IEventListener
{
friend void _audioLoadCallback(void* callbackPtr, int success);    
friend void _audioTrackLoadedAndReadyToPlay(int bufferId);
friend void _playNextTrackInPlaylist();
friend class AudioManager;

private:
    MP3Player()
        : current_buffer_id(0)
        , _bCrossfading(false)
        , _crossfadeDuration(0)
        , _crossfadeTimer(0)
    {
        EM_ASM({
            window.GameAudio = window.GameAudio || {};

            if (!window.GameAudio.audioContext)
            {
                window.GameAudio.audioContext =
                    new (window.AudioContext || window.webkitAudioContext)();
            }

            window.GameAudio.buffers = window.GameAudio.buffers || {};
            window.GameAudio.bufferCounter = window.GameAudio.bufferCounter || 1;
            window.GameAudio.activeSource = null;
            window.GameAudio.activeGain = null;
            window.GameAudio.outgoingSource = null;
            window.GameAudio.outgoingGain = null;
            window.GameAudio.playbackTime = 0;
            window.GameAudio.startTime = 0;

            if (!window.GameAudio.musicMasterGain)
            {
                window.GameAudio.musicMasterGain =
                    window.GameAudio.audioContext.createGain();

                window.GameAudio.musicMasterGain.connect(
                    window.GameAudio.audioContext.destination
                );

                window.GameAudio.musicMasterGain.gain.value = 1.0;
            }
        });
    }

    void init();

    virtual void onEvent(
        int nEvent,
        void* pData1,
        void* pData2,
        void* pData3
    ) override;

    void syncPlaybackState();

    void addToPlaylist(const std::string& url);
    void clearPlaylist(bool stopImmediately = true);
    void crossfadeToPlaylist(
        const std::vector<std::string>& newPlaylist,
        float fadeDurationSec
    );

    void playPlaylist(SimpleCallback completeCallback)
    {
        _onPlaylistComplete = completeCallback;
        current_track_index = 0;
        _bPlaylistActive = true;
        _bPendingLoadNextTrack = false;

        emitMusic(MUSIC_EVT_PLAYLIST_STARTED);

        loadNextTrack();
    }

    void loadNextTrack();
    void loadAsync(const std::string& url, void (*callback)(bool success));

    void play();
    void pause();
    void stop();

    void unloadCurrentBuffer();

    void setMuted(bool bMuted);
    void setMasterVolume(float volume);
    float getMasterVolume() const { return _masterVolume; }

    void setBusVolume(float volume);

    static void resumeAudioContext();
    bool isContextUnlocked() { return _bAudioContextUnlocked; }

    void update(float dt);
    void tryStartMusic();

    // ------------------------------------------------------------------
    // Events
    // ------------------------------------------------------------------

    void setEventEmitter(IAudioEventEmitter* pEmitter)
    {
        _audioEvents = pEmitter;
    }

    void emitMusic(
        int nEvent,
        void* pData1 = nullptr,
        void* pData2 = nullptr,
        void* pData3 = nullptr
    )
    {
        if (_audioEvents)
            _audioEvents->emitAudioEvent(nEvent, pData1, pData2, pData3);
    }

private:
    void playMusic();

    bool _bPendingLoadNextTrack = false;

public:
    bool _bPlaylistActive = false;
    int current_buffer_id;

    std::vector<std::string> playlist_urls;
    size_t current_track_index = 0;

    SimpleCallback _onPlaylistComplete;

    bool _bAudioContextUnlocked = false;
    bool _bMuted = false;
    float _masterVolume = 1.f;

    bool _IsAdShowing = false;
    bool _isBlured = false;
    bool _bIsMusicStarted = false;

    bool _bNeedsSync = false;
    float _fSyncTimer = 0;

    bool _bCrossfading = false;
    float _crossfadeDuration = 0;
    float _crossfadeTimer = 0;

protected:
    IAudioEventEmitter* _audioEvents = nullptr;
    float _busVolume = 1.0f;

public:
    inline static MP3Player& get()
    {
        static MP3Player* player = new MP3Player();
        return *player;
    }
};

#else // Native

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

struct ma_engine;
struct ma_sound;

#include "eventemmiter.h"


_G2D_NAMESPACE_BEGIN_


class MP3Player : public IEventListener
{
    friend class AudioManager;
private:
    MP3Player();
    ~MP3Player();

    void init();

    void addToPlaylist(const std::string& url);
    void clearPlaylist(bool stopImmediately = true);
    void crossfadeToPlaylist(
        const std::vector<std::string>& newPlaylist,
        float fadeDurationSec
    );

    void playPlaylist(SimpleCallback completeCallback);

    void loadNextTrack();
    void loadAsync(const std::string& url, void (*callback)(bool success));

    void play();
    void pause();
    void stop();

    void unloadCurrentBuffer();

    void setMuted(bool bMuted);
    void setMasterVolume(float volume);
    float getMasterVolume() const { return _masterVolume; }

    void setBusVolume(float volume);

    static void resumeAudioContext();
    bool isContextUnlocked();

    void update(float dt);
    void tryStartMusic();

    // ------------------------------------------------------------------
    // Events
    // ------------------------------------------------------------------

    void setEventEmitter(IAudioEventEmitter* pEmitter)
    {
        _audioEvents = pEmitter;
    }

    void emitMusic(
        int nEvent,
        void* pData1 = nullptr,
        void* pData2 = nullptr,
        void* pData3 = nullptr
    )
    {
        if (_audioEvents)
            _audioEvents->emitAudioEvent(nEvent, pData1, pData2, pData3);
    }

    // ------------------------------------------------------------------
    // Native engine attachment
    // ------------------------------------------------------------------

    void attachEngine(ma_engine* pEngine);

    // ------------------------------------------------------------------
    // IEventListener
    // ------------------------------------------------------------------

    void onEvent(int, void*, void*, void*) override
    {
        // Native MP3 currently does not listen input/app events.
    }

    int current_buffer_id;

    std::vector<std::string> playlist_urls;
    size_t current_track_index = 0;

    SimpleCallback _onPlaylistComplete;
    bool _bAudioContextUnlocked;
    inline static MP3Player& get()
    {
        static MP3Player* player = new MP3Player();
        return *player;
    }
private:
    void playMusic();
    void syncPlaybackState();
    void applyMusicVolume();

    bool _bPlaylistActive = false;
    bool _bPendingLoadNextTrack = false;

    bool _bMuted = false;
    float _masterVolume = 1.f;

    bool _isPlaying = false;
    bool _isPaused = false;
    bool _bIsMusicStarted = false;

    bool _bCrossfading = false;
    float _crossfadeDuration = 0;
    float _crossfadeTimer = 0;

    uint64_t _pauseFrame = 0;

    std::string _currentUrl;

    ma_engine* _engine = nullptr;
    bool _ownsEngine = false;

    ma_sound* _currentSound = nullptr;
    ma_sound* _outgoingSound = nullptr;

protected:
    IAudioEventEmitter* _audioEvents = nullptr;
    float _busVolume = 1.0f;
};


#endif // G2D_WEB_AUDIO

_G2D_NAMESPACE_END_