#pragma once

#include "g2d.h"
#include "eventdef.h"
#if defined(EMSCRIPTEN) || defined(TARGET_EMSCRIPTEN)
    #ifndef G2D_WEB_AUDIO
        #define G2D_WEB_AUDIO 1
    #endif
#else
    #ifndef G2D_NATIVE_AUDIO
        #define G2D_NATIVE_AUDIO 1
    #endif
#endif

_G2D_NAMESPACE_BEGIN_

enum AudioEvent : int
{
    AUDIO_EVT_NONE = EventsNs::AUDIO_EVT_FIRST,

    AUDIO_EVT_INIT,
    AUDIO_EVT_CONTEXT_UNLOCKED,
    AUDIO_EVT_MASTER_MUTE_CHANGED,

    MUSIC_EVT_FIRST,
    MUSIC_EVT_PLAYLIST_STARTED,
    MUSIC_EVT_TRACK_LOADING,
    MUSIC_EVT_TRACK_LOADED,
    MUSIC_EVT_TRACK_LOAD_FAILED,
    MUSIC_EVT_PLAY,
    MUSIC_EVT_PAUSE,
    MUSIC_EVT_STOP,
    MUSIC_EVT_TRACK_ENDED,
    MUSIC_EVT_PLAYLIST_FINISHED,
    MUSIC_EVT_CROSSFADE_STARTED,
    MUSIC_EVT_CROSSFADE_FINISHED,
    MUSIC_EVT_MUTE_CHANGED,

    SFX_EVT_FIRST,
    SFX_EVT_LOADED,
    SFX_EVT_LOAD_FAILED,
    SFX_EVT_PLAY,
    SFX_EVT_STOP,
    SFX_EVT_STOP_ALL,
    SFX_EVT_ENDED,
    SFX_EVT_MUTE_CHANGED
};

class IAudioEventEmitter
{
public:
    virtual         ~IAudioEventEmitter (void) {}

    virtual void    emitAudioEvent      (int        nEvent, 
                                         void*      pData1 = nullptr,
                                         void*      pData2 = nullptr,
                                         void*      pData3 = nullptr) = 0;
};

template <typename T>
inline void* audioInt(T value)
{
    return reinterpret_cast<void*>(static_cast<std::intptr_t>(value));
}

_G2D_NAMESPACE_END_