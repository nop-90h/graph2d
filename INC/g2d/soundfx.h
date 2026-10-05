#pragma once

#include "g2d.h"
#include "audioevents.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

#ifdef G2D_WEB_AUDIO

#include <iostream>

#include <emscripten.h>
#include <emscripten/html5.h>

_G2D_NAMESPACE_BEGIN_

enum class SoundPlayMode
{
    Unlimited = 0,
    LimitedShared = 1,
    LimitedSeparate = 2
};

struct SoundSpriteData
{
    float start = 0.0f;
    float duration = 0.0f;
    bool loop = false;
};

struct SoundSprite
{
    std::string url;
    int bufferId = 0;
    std::unordered_map<std::string, SoundSpriteData> sounds;
};

extern "C"
{
    EMSCRIPTEN_KEEPALIVE void _sfxLoadCallback(void* callbackPtr, int success, int bufferId);
    EMSCRIPTEN_KEEPALIVE void _sfxSimpleCallback(void* callbackPtr);
    EMSCRIPTEN_KEEPALIVE void _sfxDeleteSimpleCallback(void* callbackPtr);
    EMSCRIPTEN_KEEPALIVE void _sfxSoundEnded(int instanceId);
    EMSCRIPTEN_KEEPALIVE void _sfxSpriteLoaded(int bufferId);
    EMSCRIPTEN_KEEPALIVE void _sfxSpriteLoadFailed(int bufferId);
}

class SoundFXPlayer
{
    friend class AudioManager;
    friend void _sfxLoadCallback(void* callbackPtr, int success, int bufferId);
    friend void _sfxDeleteSimpleCallback(void* callbackPtr);
    friend void _sfxSoundEnded(int instanceId);
    friend void _sfxSpriteLoaded(int bufferId);
    friend void _sfxSpriteLoadFailed(int bufferId);
private:
    static auto& get() { static SoundFXPlayer*  instance = new SoundFXPlayer(); return *instance; }
    SoundFXPlayer()
    {
        EM_ASM({
            window.GameAudio = window.GameAudio || {};

            if (!window.GameAudio.audioContext)
            {
                window.GameAudio.audioContext =
                    new (window.AudioContext || window.webkitAudioContext)();
            }

            window.GameAudio.sfxBuffers = window.GameAudio.sfxBuffers || {};
            window.GameAudio.sfxBufferCounter = window.GameAudio.sfxBufferCounter || 1000;
            window.GameAudio.sfxInstances = window.GameAudio.sfxInstances || [];
        });
    }

    void init();

    void setMuted(bool bMuted);
    bool isMuted() const { return _bMuted; }

    void setMasterVolume(float volume);
    float getMasterVolume() const { return _masterVolume; }

    void setBusVolume(float volume);

    void setModeLimit(SoundPlayMode mode, int limit);
    int getModeLimit(SoundPlayMode mode) const;

    int loadSprite(
        const char* name,
        const char* url,
        const std::unordered_map<std::string, SoundSpriteData>& sounds,
        SimpleCallback cb = nullptr
    );

    bool loadSpriteMetadata(
        const char* name,
        const char* url,
        const char* jsonMetadata,
        SimpleCallback cb = nullptr
    );

    int loadSound(const char* name, const char* url);

    void loadSoundAsync(
        const char* name,
        const char* url,
        std::function<void(bool success)> callback
    );

    bool isSoundLoaded(const char* name) const;

    int playSound(
        const char* name,
        float volume = 1.0f,
        SoundPlayMode mode = SoundPlayMode::Unlimited
    );

    int playSoundLooped(
        const char* name,
        float volume = 1.0f,
        bool loop = true,
        SoundPlayMode mode = SoundPlayMode::Unlimited
    );

    void stopSound(int instanceId);
    void stopSoundByName(const char* name);
    void stopAllSounds();

    void setInstanceVolume(int instanceId, float volume);
    bool isSoundPlaying(int instanceId) const;

    void unloadSound(const char* name);
    void unloadAllSounds();

    size_t getLoadedSoundCount() const;
    size_t getActiveInstanceCount() const;
    int getModeActiveCount(SoundPlayMode mode) const;

    void update(float) {}

    // ------------------------------------------------------------------
    // Events
    // ------------------------------------------------------------------

    void setEventEmitter(IAudioEventEmitter* pEmitter)
    {
        _audioEvents = pEmitter;
    }

    void emitSfx(
        int nEvent,
        void* pData1 = nullptr,
        void* pData2 = nullptr,
        void* pData3 = nullptr
    )
    {
        if (_audioEvents)
            _audioEvents->emitAudioEvent(nEvent, pData1, pData2, pData3);
    }

    const char* spriteNameByBufferId(int bufferId) const
    {
        for (const auto& pair : _loadedSprites)
        {
            if (pair.second.bufferId == bufferId)
            {
                return pair.first.c_str();
            }
        }

        return "";
    }

    void removeSpriteByBufferId(int bufferId)
    {
        for (auto it = _loadedSprites.begin(); it != _loadedSprites.end(); ++it)
        {
            if (it->second.bufferId == bufferId)
            {
                for (const auto& sound : it->second.sounds)
                {
                    _soundToSprite.erase(sound.first);
                }

                _loadedSprites.erase(it);
                return;
            }
        }
    }

public:
    std::vector<int> _activeInstances;

    bool _bMuted = false;
    float _masterVolume = 1.f;

    std::unordered_map<std::string, SoundSprite> _loadedSprites;
    std::unordered_map<std::string, std::string> _soundToSprite;

    int _instanceCounter = 0;

    std::unordered_map<int, SoundPlayMode> _instanceModes;
    int _modeLimits[3] = { 0, 8, 8 };

protected:
    IAudioEventEmitter* _audioEvents = nullptr;
    float _busVolume = 1.0f;
};


#else // Native

_G2D_NAMESPACE_BEGIN_

enum class SoundPlayMode
{
    Unlimited = 0,
    LimitedShared = 1,
    LimitedSeparate = 2
};

struct SoundSpriteData
{
    float start = 0.0f;
    float duration = 0.0f;
    bool loop = false;
};

struct SoundSprite
{
    std::string url;
    int bufferId = 0;

    std::unordered_map<std::string, SoundSpriteData> sounds;

    std::vector<float> pcmData;
    uint32_t channels = 0;
    uint32_t sampleRate = 0;
    uint64_t totalFrames = 0;
};

class SoundFXPlayer
{
    friend class AudioManager;
private:
    SoundFXPlayer() = default;
    static auto& get() { static SoundFXPlayer*  instance = new SoundFXPlayer(); return *instance; }
//public:
    ~SoundFXPlayer();

    void init();

    void setMuted(bool bMuted);
    bool isMuted() const { return _bMuted; }

    void setMasterVolume(float volume);
    float getMasterVolume() const { return _masterVolume; }

    void setBusVolume(float volume);

    void setModeLimit(SoundPlayMode mode, int limit);
    int getModeLimit(SoundPlayMode mode) const;

    int loadSprite(
        const char* name,
        const char* url,
        const std::unordered_map<std::string, SoundSpriteData>& sounds,
        SimpleCallback cb = nullptr
    );

    bool loadSpriteMetadata(
        const char* name,
        const char* url,
        const char* jsonMetadata,
        SimpleCallback cb = nullptr
    );

    int loadSound(const char* name, const char* url);

    void loadSoundAsync(
        const char* name,
        const char* url,
        std::function<void(bool success)> callback
    );

    bool isSoundLoaded(const char* name) const;

    int playSound(
        const char* name,
        float volume = 1.0f,
        SoundPlayMode mode = SoundPlayMode::Unlimited
    );

    int playSoundLooped(
        const char* name,
        float volume = 1.0f,
        bool loop = true,
        SoundPlayMode mode = SoundPlayMode::Unlimited
    );

    void stopSound(int instanceId);
    void stopSoundByName(const char* name);
    void stopAllSounds();

    void setInstanceVolume(int instanceId, float volume);
    bool isSoundPlaying(int instanceId) const;

    void unloadSound(const char* name);
    void unloadAllSounds();

    size_t getLoadedSoundCount() const;
    size_t getActiveInstanceCount() const;
    int getModeActiveCount(SoundPlayMode mode) const;

    void update(float dt);

    // ------------------------------------------------------------------
    // Events
    // ------------------------------------------------------------------

    void setEventEmitter(IAudioEventEmitter* pEmitter)
    {
        _audioEvents = pEmitter;
    }

    void emitSfx(
        int nEvent,
        void* pData1 = nullptr,
        void* pData2 = nullptr,
        void* pData3 = nullptr
    )
    {
        _audioEvents->emitAudioEvent(nEvent, pData1, pData2, pData3);
    }

    const char* spriteNameByBufferId(int bufferId) const
    {
        for (const auto& pair : _loadedSprites)
        {
            if (pair.second.bufferId == bufferId)
            {
                return pair.first.c_str();
            }
        }

        return "";
    }

    void removeSpriteByBufferId(int bufferId)
    {
        for (auto it = _loadedSprites.begin(); it != _loadedSprites.end(); ++it)
        {
            if (it->second.bufferId == bufferId)
            {
                for (const auto& sound : it->second.sounds)
                {
                    _soundToSprite.erase(sound.first);
                }

                _loadedSprites.erase(it);
                return;
            }
        }
    }

    // ------------------------------------------------------------------
    // Native engine attachment
    // ------------------------------------------------------------------

    void attachEngine(void* pEngine);

//public:
    std::vector<int> _activeInstances;

private:
    bool _bMuted = false;
    float _masterVolume = 1.f;

    std::unordered_map<std::string, SoundSprite> _loadedSprites;
    std::unordered_map<std::string, std::string> _soundToSprite;

    int _instanceCounter = 0;
    int _bufferCounter = 1000;

    std::unordered_map<int, SoundPlayMode> _instanceModes;
    int _modeLimits[3] = { 0, 8, 8 };

    void* _engine = nullptr;
    bool _ownsEngine = false;

    std::vector<void*> _soundInstances;

//protected:
    IAudioEventEmitter* _audioEvents = nullptr;
    float _busVolume = 1.0f;
};

#endif // G2D_WEB_AUDIO

_G2D_NAMESPACE_END_