#include "soundfx.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "jsondec.h"
#include "audiomanager.h"
#include "engine.h"

_G2D_NAMESPACE_BEGIN_

#ifdef G2D_WEB_AUDIO

// ---------------------------------------------------------------------------
// Web implementation
// ---------------------------------------------------------------------------

static std::function<void(bool success, int bufferId)> s_pendingSfxCallback;

EMSCRIPTEN_KEEPALIVE void _sfxLoadCallback(void* callbackPtr, int success, int bufferId)
{
    void (*callback)(bool, int) = reinterpret_cast<void (*)(bool, int)>(callbackPtr);

    if (callback)
    {
        callback(static_cast<bool>(success), bufferId);
    }

    if (s_pendingSfxCallback)
    {
        s_pendingSfxCallback(static_cast<bool>(success), bufferId);
        s_pendingSfxCallback = nullptr;
    }
}

EMSCRIPTEN_KEEPALIVE void _sfxSimpleCallback(void* callbackPtr)
{
    std::function<void()>* callback =
        reinterpret_cast<std::function<void()>*>(callbackPtr);

    if (callback)
    {
        (*callback)();
        delete callback;
    }
}

EMSCRIPTEN_KEEPALIVE void _sfxDeleteSimpleCallback(void* callbackPtr)
{
    std::function<void()>* callback =
        reinterpret_cast<std::function<void()>*>(callbackPtr);

    if (callback)
    {
        delete callback;
    }
}

EMSCRIPTEN_KEEPALIVE void _sfxSoundEnded(int instanceId)
{
    auto& sfx = SoundFXPlayer::get();

    auto it = std::find(
        sfx._activeInstances.begin(),
        sfx._activeInstances.end(),
        instanceId
    );

    if (it != sfx._activeInstances.end())
    {
        sfx._activeInstances.erase(it);
    }

    auto modeIt = sfx._instanceModes.find(instanceId);

    if (modeIt != sfx._instanceModes.end())
    {
        sfx._instanceModes.erase(modeIt);
    }

    sfx.emitSfx(SFX_EVT_ENDED, audioInt(instanceId));
}

EMSCRIPTEN_KEEPALIVE void _sfxSpriteLoaded(int bufferId)
{
    auto& sfx = SoundFXPlayer::get();

    sfx.emitSfx(
        SFX_EVT_LOADED,
        audioInt(bufferId),
        const_cast<char*>(sfx.spriteNameByBufferId(bufferId))
    );
}

EMSCRIPTEN_KEEPALIVE void _sfxSpriteLoadFailed(int bufferId)
{
    auto& sfx = SoundFXPlayer::get();

    sfx.emitSfx(
        SFX_EVT_LOAD_FAILED,
        audioInt(bufferId),
        const_cast<char*>(sfx.spriteNameByBufferId(bufferId))
    );

    sfx.removeSpriteByBufferId(bufferId);
}

void SoundFXPlayer::setModeLimit(SoundPlayMode mode, int limit)
{
    int idx = static_cast<int>(mode);

    if (idx >= 0 && idx < 3)
    {
        _modeLimits[idx] = std::max(0, limit);
    }
}

int SoundFXPlayer::getModeLimit(SoundPlayMode mode) const
{
    int idx = static_cast<int>(mode);

    if (idx >= 0 && idx < 3)
    {
        return _modeLimits[idx];
    }

    return 0;
}

int SoundFXPlayer::getModeActiveCount(SoundPlayMode mode) const
{
    int count = 0;

    for (int id : _activeInstances)
    {
        auto it = _instanceModes.find(id);

        if (it != _instanceModes.end() && it->second == mode)
        {
            ++count;
        }
    }

    return count;
}

void SoundFXPlayer::init()
{
    EM_ASM({
        try
        {
            window.GameAudio = window.GameAudio || {};

            if (!window.GameAudio.audioContext)
            {
                window.GameAudio.audioContext =
                    new (window.AudioContext || window.webkitAudioContext)();
            }

            window.GameAudio.sfxBuffers = window.GameAudio.sfxBuffers || {};
            window.GameAudio.sfxBufferCounter = window.GameAudio.sfxBufferCounter || 1000;
            window.GameAudio.sfxInstances = window.GameAudio.sfxInstances || [];

            if (!window.GameAudio.sfxMasterGain)
            {
                window.GameAudio.sfxMasterGain =
                    window.GameAudio.audioContext.createGain();

                window.GameAudio.sfxMasterGain.connect(
                    window.GameAudio.audioContext.destination
                );

                window.GameAudio.sfxMasterGain.gain.value = 1.0;
            }

            console.log("SoundFX system initialized");
        }
        catch (e)
        {
            console.error("SoundFX: init failed:", e);
        }
    });
}

void SoundFXPlayer::setMuted(bool bMuted)
{
    if (_bMuted != bMuted)
    {
        _bMuted = bMuted;

        if (bMuted)
        {
            stopAllSounds();
        }

        float vol = _bMuted ? 0.0f : (_masterVolume / 100.0f);
        setBusVolume(vol);

        emitSfx(SFX_EVT_MUTE_CHANGED, audioInt(bMuted ? 1 : 0));
    }
}

void SoundFXPlayer::setMasterVolume(float volume)
{
    _masterVolume = std::clamp(volume, 0.f, 1.f);
    float vol = _bMuted ? 0.0f : _masterVolume;
    setBusVolume(vol);
}

void SoundFXPlayer::setBusVolume(float volume)
{
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    _busVolume = volume;

    EM_ASM({
        try
        {
            if (!window.GameAudio.sfxMasterGain)
            {
                var context = window.GameAudio.audioContext;

                if (context)
                {
                    window.GameAudio.sfxMasterGain = context.createGain();
                    window.GameAudio.sfxMasterGain.connect(context.destination);
                }
            }

            if (window.GameAudio.sfxMasterGain)
            {
                window.GameAudio.sfxMasterGain.gain.value = $0;
            }
        }
        catch (e)
        {
            console.error("SoundFX: setBusVolume failed:", e);
        }
    }, volume);
}

int SoundFXPlayer::loadSprite(
    const char* name,
    const char* url,
    const std::unordered_map<std::string, SoundSpriteData>& sounds,
    SimpleCallback cb
)
{
    std::string nameStr(name ? name : "");
    std::string urlStr = std::format("{}/{}", Engine::getCfg().RES_DIR, url);

    if (nameStr.empty())
    {
        std::cerr << "SoundFX: loadSprite - empty sound name" << std::endl;
        return 0;
    }

    if (urlStr.empty())
    {
        std::cerr << "SoundFX: loadSprite - empty URL for sound '"
                  << nameStr << "'" << std::endl;
        return 0;
    }

    auto it = _loadedSprites.find(nameStr);

    if (it != _loadedSprites.end())
    {
        std::cout << "Sprite '" << nameStr
                  << "' already loaded (buffer ID: "
                  << it->second.bufferId << ")" << std::endl;

        return it->second.bufferId;
    }

    int bufferId = EM_ASM_INT({
        try
        {
            return window.GameAudio.sfxBufferCounter++;
        }
        catch (e)
        {
            console.error("SoundFX: loadSprite - failed to generate buffer ID:", e);
            return 0;
        }
    });

    if (bufferId == 0)
    {
        std::cerr << "SoundFX: loadSprite - failed to generate buffer ID" << std::endl;
        return 0;
    }

    SoundSprite sprite;
    sprite.url = urlStr;
    sprite.bufferId = bufferId;
    sprite.sounds = sounds;

    _loadedSprites[nameStr] = sprite;

    for (const auto& sound : sounds)
    {
        if (!sound.first.empty())
        {
            _soundToSprite[sound.first] = nameStr;
        }
    }

    EM_ASM({
        try
        {
            var name = UTF8ToString($0);
            var url = UTF8ToString($1);
            var bufferId = $2;
            var callbackPtr = $3;

            var buf = {};
            buf.url = url;
            buf.bufferId = bufferId;
            buf.data = null;

            window.GameAudio.sfxBuffers[name] = buf;

            fetch(url)
                .then(response => response.arrayBuffer())
                .then(arrayBuffer => {
                    if (!window.GameAudio.audioContext)
                    {
                        throw new Error("AudioContext not available");
                    }

                    return window.GameAudio.audioContext.decodeAudioData(arrayBuffer);
                })
                .then(audioBuffer => {
                    window.GameAudio.sfxBuffers[name].data = audioBuffer;

                    console.log(
                        "Sound sprite '" + name +
                        "' loaded successfully (buffer ID: " + bufferId + ")"
                    );

                    if (callbackPtr)
                    {
                        __sfxSimpleCallback(callbackPtr);
                    }

                    __sfxSpriteLoaded(bufferId);
                })
                .catch(error => {
                    console.error(
                        "Error loading sound sprite '" + name + "':",
                        error
                    );

                    if (window.GameAudio.sfxBuffers[name])
                    {
                        delete window.GameAudio.sfxBuffers[name];
                    }

                    if (callbackPtr)
                    {
                        __sfxDeleteSimpleCallback(callbackPtr);
                    }

                    __sfxSpriteLoadFailed(bufferId);
                });
        }
        catch (e)
        {
            console.error("SoundFX: loadSprite - EM_ASM failed:", e);
        }
    },
    nameStr.c_str(),
    urlStr.c_str(),
    bufferId,
    cb ? reinterpret_cast<void*>(new std::function<void()>(cb)) : 0);

    std::cout << "Loading sound sprite '" << nameStr
              << "' from " << urlStr
              << " with " << sounds.size()
              << " sounds (buffer ID: " << bufferId << ")"
              << std::endl;

    return bufferId;
}

bool SoundFXPlayer::loadSpriteMetadata(
    const char* name,
    const char* url,
    const char* jsonMetadata,
    SimpleCallback cb
)
{
    if (!name || !jsonMetadata)
    {
        std::cerr << "SoundFX: loadSpriteMetadata - null parameters" << std::endl;
        return false;
    }

    CJson json(jsonMetadata);

    if (!json._child)
    {
        std::cerr << "SoundFX: loadSpriteMetadata - invalid JSON" << std::endl;
        return false;
    }

    CJson* resources = CJson::getItem(&json, "resources");
    std::string audioUrl = url;

    if (resources && resources->_child && resources->_child->_valueString)
    {
        audioUrl = resources->_child->_valueString;
    }

    CJson* spritemap = CJson::getItem(&json, "spritemap");

    if (!spritemap)
    {
        std::cerr << "SoundFX: loadSpriteMetadata - no spritemap found" << std::endl;
        return false;
    }

    std::unordered_map<std::string, SoundSpriteData> sounds;

    for (CJson* child = spritemap->_child; child; child = child->_next)
    {
        if (child->_name)
        {
            SoundSpriteData data;

            data.start = CJson::getFloat(child, "start", 0.0f);

            float end = CJson::getFloat(child, "end", 0.0f);
            data.duration = end - data.start;
            data.loop = CJson::getBoolean(child, "loop", false);

            sounds[child->_name] = data;
        }
    }

    if (sounds.empty())
    {
        std::cerr << "SoundFX: loadSpriteMetadata - no sounds found in spritemap"
                  << std::endl;
        return false;
    }

    return loadSprite(name, audioUrl.c_str(), sounds, cb) != 0;
}

int SoundFXPlayer::loadSound(const char* name, const char* url)
{
    std::string nameStr(name ? name : "");
    std::string urlStr = std::format("{}/{}", Engine::getCfg().RES_DIR, url);

    if (nameStr.empty())
    {
        std::cerr << "SoundFX: loadSound - empty sound name" << std::endl;
        return 0;
    }

    if (urlStr.empty())
    {
        std::cerr << "SoundFX: loadSound - empty URL for sound '"
                  << nameStr << "'" << std::endl;
        return 0;
    }

    SoundSpriteData data;
    data.start = 0.0f;
    data.duration = 0.0f;
    data.loop = false;

    std::unordered_map<std::string, SoundSpriteData> sounds;
    sounds[nameStr] = data;

    return loadSprite(nameStr.c_str(), urlStr.c_str(), sounds);
}

void SoundFXPlayer::loadSoundAsync(
    const char* name,
    const char* url,
    std::function<void(bool success)> callback
)
{
    std::string nameStr(name ? name : "");
    std::string urlStr = std::format("{}/{}", Engine::getCfg().RES_DIR, url);
    url = urlStr.c_str();

    if (nameStr.empty() || urlStr.empty())
    {
        if (callback)
        {
            callback(false);
        }

        return;
    }

    s_pendingSfxCallback = [callback](bool success, int) {
        if (callback)
        {
            callback(success);
        }
    };

    EM_ASM({
        try
        {
            var name = UTF8ToString($0);
            var url = UTF8ToString($1);
            var bufferId = window.GameAudio.sfxBufferCounter++;

            var buf = {};
            buf.url = url;
            buf.bufferId = bufferId;
            buf.data = null;

            window.GameAudio.sfxBuffers[name] = buf;

            fetch(url)
                .then(response => response.arrayBuffer())
                .then(arrayBuffer => {
                    if (!window.GameAudio.audioContext)
                    {
                        throw new Error("AudioContext not available");
                    }

                    return window.GameAudio.audioContext.decodeAudioData(arrayBuffer);
                })
                .then(audioBuffer => {
                    window.GameAudio.sfxBuffers[name].data = audioBuffer;
                    __sfxLoadCallback(0, 1, bufferId);
                })
                .catch(error => {
                    console.error("Error loading sound '" + name + "':", error);

                    if (window.GameAudio.sfxBuffers[name])
                    {
                        delete window.GameAudio.sfxBuffers[name];
                    }

                    __sfxLoadCallback(0, 0, 0);
                });
        }
        catch (e)
        {
            console.error("SoundFX: loadSoundAsync failed:", e);
            __sfxLoadCallback(0, 0, 0);
        }
    }, nameStr.c_str(), urlStr.c_str());
}

bool SoundFXPlayer::isSoundLoaded(const char* name) const
{
    std::string nameStr(name ? name : "");

    if (nameStr.empty())
    {
        return false;
    }

    auto spriteIt = _loadedSprites.find(nameStr);

    if (spriteIt != _loadedSprites.end())
    {
        bool isDecoded = EM_ASM_INT({
            try
            {
                var name = UTF8ToString($0);
                var soundData = window.GameAudio.sfxBuffers[name];

                return (soundData && soundData.data) ? 1 : 0;
            }
            catch (e)
            {
                console.error("SoundFX: isSoundLoaded failed:", e);
                return 0;
            }
        }, nameStr.c_str());

        return isDecoded != 0;
    }

    auto soundIt = _soundToSprite.find(nameStr);

    if (soundIt != _soundToSprite.end())
    {
        const std::string& spriteName = soundIt->second;

        bool isDecoded = EM_ASM_INT({
            try
            {
                var name = UTF8ToString($0);
                var soundData = window.GameAudio.sfxBuffers[name];

                return (soundData && soundData.data) ? 1 : 0;
            }
            catch (e)
            {
                console.error("SoundFX: isSoundLoaded failed:", e);
                return 0;
            }
        }, spriteName.c_str());

        return isDecoded != 0;
    }

    return false;
}

int SoundFXPlayer::playSound(
    const char* name,
    float volume,
    SoundPlayMode mode
)
{
    return playSoundLooped(name, volume, false, mode);
}

int SoundFXPlayer::playSoundLooped(
    const char* name,
    float volume,
    bool loop,
    SoundPlayMode mode
)
{
    std::string nameStr(name ? name : "");

    if (nameStr.empty())
    {
        std::cerr << "SoundFX: playSoundLooped - empty sound name" << std::endl;
        return 0;
    }

    if (std::isnan(volume) || std::isinf(volume))
    {
        std::cerr << "SoundFX: playSoundLooped - invalid volume value" << std::endl;
        return 0;
    }

    volume = std::max(0.0f, std::min(1.0f, volume));

    if (_bMuted)
    {
        return 0;
    }

    if (!isSoundLoaded(nameStr.c_str()))
    {
        std::cerr << "Sound '" << nameStr << "' not loaded or not decoded yet"
                  << std::endl;
        return 0;
    }

    int modeIdx = static_cast<int>(mode);

    if (modeIdx == 1 || modeIdx == 2)
    {
        int limit = _modeLimits[modeIdx];

        if (limit > 0 && getModeActiveCount(mode) >= limit)
        {
            std::cout << "Sound '" << nameStr
                      << "' not started: mode " << modeIdx
                      << " limit reached (" << limit << ")"
                      << std::endl;

            return 0;
        }
    }

    std::string spriteName;
    SoundSpriteData soundData;

    auto spriteIt = _loadedSprites.find(nameStr);

    if (spriteIt != _loadedSprites.end())
    {
        spriteName = nameStr;
        soundData.start = 0.0f;
        soundData.duration = 0.0f;
    }
    else
    {
        auto soundIt = _soundToSprite.find(nameStr);

        if (soundIt == _soundToSprite.end())
        {
            std::cerr << "Sound '" << nameStr << "' not found in any sprite"
                      << std::endl;
            return 0;
        }

        spriteName = soundIt->second;
        soundData = _loadedSprites[spriteName].sounds[nameStr];
    }

    if (std::isnan(soundData.start) || std::isinf(soundData.start))
    {
        soundData.start = 0.0f;
    }

    if (std::isnan(soundData.duration) || std::isinf(soundData.duration))
    {
        soundData.duration = 0.0f;
    }

    if (soundData.start < 0.0f)
    {
        soundData.start = 0.0f;
    }

    if (soundData.duration < 0.0f)
    {
        soundData.duration = 0.0f;
    }

    int instanceId = ++_instanceCounter;

    EM_ASM({
        try
        {
            var spriteName = UTF8ToString($0);
            var soundName = UTF8ToString($1);
            var startTime = $2;
            var duration = $3;
            var volume = $4;
            var loop = $5;
            var instanceId = $6;
            var muted = $7;

            var context = window.GameAudio.audioContext;
            var soundData = window.GameAudio.sfxBuffers[spriteName];

            if (context && soundData && soundData.data)
            {
                var source = context.createBufferSource();
                source.buffer = soundData.data;
                source.loop = loop;

                if (loop && duration > 0)
                {
                    source.loopStart = startTime;
                    source.loopEnd = startTime + duration;
                }

                var gainNode = context.createGain();
                gainNode.gain.value = muted ? 0 : volume;

                source.connect(gainNode);
                gainNode.connect(window.GameAudio.sfxMasterGain || context.destination);

                if (loop && duration > 0)
                {
                    source.start(0, startTime, duration);
                }
                else if (loop)
                {
                    source.start(0, startTime);
                }
                else
                {
                    if (duration > 0)
                    {
                        source.start(0, startTime, duration);
                    }
                    else
                    {
                        source.start(0, startTime);
                    }
                }

                var instance = {};
                instance.id = instanceId;
                instance.source = source;
                instance.gainNode = gainNode;
                instance.name = soundName;
                instance.spriteName = spriteName;
                instance.loop = loop;

                if (!window.GameAudio.sfxInstances)
                {
                    window.GameAudio.sfxInstances = [];
                }

                window.GameAudio.sfxInstances.push(instance);

                source.onended = function() {
                    var instances = window.GameAudio.sfxInstances;

                    for (var i = 0; i < instances.length; i++)
                    {
                        if (instances[i].id === instanceId)
                        {
                            instances.splice(i, 1);
                            break;
                        }
                    }

                    __sfxSoundEnded(instanceId);
                };
            }
            else
            {
                console.error(
                    "Cannot play sound '" + soundName +
                    "': audio data not available"
                );
            }
        }
        catch (e)
        {
            console.error("SoundFX: playSoundLooped failed:", e);
        }
    },
    spriteName.c_str(),
    nameStr.c_str(),
    soundData.start,
    soundData.duration,
    volume,
    loop ? 1 : 0,
    instanceId,
    _bMuted ? 1 : 0);

    _activeInstances.push_back(instanceId);
    _instanceModes[instanceId] = mode;

    emitSfx(
        SFX_EVT_PLAY,
        audioInt(instanceId),
        const_cast<char*>(nameStr.c_str())
    );

    return instanceId;
}

void SoundFXPlayer::stopSound(int instanceId)
{
    EM_ASM({
        try
        {
            var instanceId = $0;
            var instances = window.GameAudio.sfxInstances;

            if (instances)
            {
                for (var i = 0; i < instances.length; i++)
                {
                    if (instances[i].id === instanceId)
                    {
                        try
                        {
                            instances[i].source.stop();
                        }
                        catch (e) {}

                        instances.splice(i, 1);
                        break;
                    }
                }
            }
        }
        catch (e)
        {
            console.error("SoundFX: stopSound failed:", e);
        }
    }, instanceId);

    auto it = std::find(
        _activeInstances.begin(),
        _activeInstances.end(),
        instanceId
    );

    if (it != _activeInstances.end())
    {
        _activeInstances.erase(it);
    }

    auto modeIt = _instanceModes.find(instanceId);

    if (modeIt != _instanceModes.end())
    {
        _instanceModes.erase(modeIt);
    }

    emitSfx(SFX_EVT_STOP, audioInt(instanceId));
}

void SoundFXPlayer::stopSoundByName(const char* name)
{
    std::string nameStr(name ? name : "");

    if (nameStr.empty())
    {
        std::cerr << "SoundFX: stopSoundByName - empty sound name" << std::endl;
        return;
    }

    EM_ASM({
        try
        {
            var name = UTF8ToString($0);
            var instances = window.GameAudio.sfxInstances;

            if (instances)
            {
                for (var i = instances.length - 1; i >= 0; i--)
                {
                    if (instances[i].name === name)
                    {
                        try
                        {
                            instances[i].source.stop();
                        }
                        catch (e) {}

                        instances.splice(i, 1);
                    }
                }
            }
        }
        catch (e)
        {
            console.error("SoundFX: stopSoundByName failed:", e);
        }
    }, nameStr.c_str());

    _activeInstances.erase(
        std::remove_if(
            _activeInstances.begin(),
            _activeInstances.end(),
            [this](int id) {
                return !isSoundPlaying(id);
            }
        ),
        _activeInstances.end()
    );

    for (auto it = _instanceModes.begin(); it != _instanceModes.end(); )
    {
        if (!isSoundPlaying(it->first))
        {
            it = _instanceModes.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void SoundFXPlayer::stopAllSounds()
{
    EM_ASM({
        try
        {
            var instances = window.GameAudio.sfxInstances;

            if (instances)
            {
                for (var i = instances.length - 1; i >= 0; i--)
                {
                    try
                    {
                        instances[i].source.stop();
                    }
                    catch (e) {}
                }

                window.GameAudio.sfxInstances = [];
            }
        }
        catch (e)
        {
            console.error("SoundFX: stopAllSounds failed:", e);
        }
    });

    _activeInstances.clear();
    _instanceModes.clear();

    emitSfx(SFX_EVT_STOP_ALL);
}

void SoundFXPlayer::setInstanceVolume(int instanceId, float volume)
{
    if (std::isnan(volume) || std::isinf(volume))
    {
        std::cerr << "SoundFX: setInstanceVolume - invalid volume value" << std::endl;
        return;
    }

    volume = std::max(0.0f, std::min(1.0f, volume));

    EM_ASM({
        try
        {
            var instanceId = $0;
            var volume = $1;
            var instances = window.GameAudio.sfxInstances;

            if (instances)
            {
                for (var i = 0; i < instances.length; i++)
                {
                    if (instances[i].id === instanceId)
                    {
                        instances[i].gainNode.gain.value = volume;
                        break;
                    }
                }
            }
        }
        catch (e)
        {
            console.error("SoundFX: setInstanceVolume failed:", e);
        }
    }, instanceId, volume);
}

bool SoundFXPlayer::isSoundPlaying(int instanceId) const
{
    return EM_ASM_INT({
        try
        {
            var instanceId = $0;
            var instances = window.GameAudio.sfxInstances;

            if (instances)
            {
                for (var i = 0; i < instances.length; i++)
                {
                    if (instances[i].id === instanceId)
                    {
                        return 1;
                    }
                }
            }

            return 0;
        }
        catch (e)
        {
            console.error("SoundFX: isSoundPlaying failed:", e);
            return 0;
        }
    }, instanceId);
}

void SoundFXPlayer::unloadSound(const char* name)
{
    std::string nameStr(name ? name : "");

    if (nameStr.empty())
    {
        std::cerr << "SoundFX: unloadSound - empty sound name" << std::endl;
        return;
    }

    auto soundIt = _soundToSprite.find(nameStr);

    if (soundIt != _soundToSprite.end())
    {
        std::string spriteName = soundIt->second;
        _soundToSprite.erase(soundIt);

        auto spriteIt = _loadedSprites.find(spriteName);

        if (spriteIt != _loadedSprites.end())
        {
            spriteIt->second.sounds.erase(nameStr);

            if (spriteIt->second.sounds.empty())
            {
                EM_ASM({
                    try
                    {
                        var name = UTF8ToString($0);

                        if (window.GameAudio.sfxBuffers[name])
                        {
                            delete window.GameAudio.sfxBuffers[name];
                        }
                    }
                    catch (e)
                    {
                        console.error("SoundFX: unloadSound failed:", e);
                    }
                }, spriteName.c_str());

                _loadedSprites.erase(spriteIt);
            }
        }

        return;
    }

    auto spriteIt = _loadedSprites.find(nameStr);

    if (spriteIt != _loadedSprites.end())
    {
        for (const auto& sound : spriteIt->second.sounds)
        {
            _soundToSprite.erase(sound.first);
        }

        EM_ASM({
            try
            {
                var name = UTF8ToString($0);

                if (window.GameAudio.sfxBuffers[name])
                {
                    delete window.GameAudio.sfxBuffers[name];
                }
            }
            catch (e)
            {
                console.error("SoundFX: unloadSound failed:", e);
            }
        }, nameStr.c_str());

        _loadedSprites.erase(spriteIt);
    }
}

void SoundFXPlayer::unloadAllSounds()
{
    EM_ASM({
        try
        {
            window.GameAudio.sfxBuffers = {};
        }
        catch (e)
        {
            console.error("SoundFX: unloadAllSounds failed:", e);
        }
    });

    _loadedSprites.clear();
    _soundToSprite.clear();
}

size_t SoundFXPlayer::getLoadedSoundCount() const
{
    size_t count = 0;

    for (const auto& sprite : _loadedSprites)
    {
        count += sprite.second.sounds.size();
    }

    return count;
}

size_t SoundFXPlayer::getActiveInstanceCount() const
{
    return EM_ASM_INT({
        try
        {
            return window.GameAudio.sfxInstances
                ? window.GameAudio.sfxInstances.length
                : 0;
        }
        catch (e)
        {
            console.error("SoundFX: getActiveInstanceCount failed:", e);
            return 0;
        }
    });
}

#else // Native

// ---------------------------------------------------------------------------
// Windows / miniaudio implementation
// ---------------------------------------------------------------------------

#include "miniaudio.h"

struct SoundInstanceInternal
{
    int id = 0;

    ma_sound sound;
    ma_audio_buffer_ref bufferRef;

    std::string name;
    std::string spriteName;

    bool loop = false;
    SoundPlayMode mode = SoundPlayMode::Unlimited;

    float baseVolume = 1.0f;
};

SoundFXPlayer::~SoundFXPlayer()
{
    stopAllSounds();

    if (_ownsEngine && _engine)
    {
        ma_engine_uninit(static_cast<ma_engine*>(_engine));
        delete static_cast<ma_engine*>(_engine);
        _engine = nullptr;
    }
}

void SoundFXPlayer::attachEngine(void* pEngine)
{
    if (_ownsEngine && _engine && _engine != pEngine)
    {
        stopAllSounds();

        ma_engine_uninit(static_cast<ma_engine*>(_engine));
        delete static_cast<ma_engine*>(_engine);
        _engine = nullptr;
    }

    _engine = pEngine;
    _ownsEngine = false;
}

void SoundFXPlayer::setModeLimit(SoundPlayMode mode, int limit)
{
    int idx = static_cast<int>(mode);

    if (idx >= 0 && idx < 3)
    {
        _modeLimits[idx] = std::max(0, limit);
    }
}

int SoundFXPlayer::getModeLimit(SoundPlayMode mode) const
{
    int idx = static_cast<int>(mode);

    if (idx >= 0 && idx < 3)
    {
        return _modeLimits[idx];
    }

    return 0;
}

int SoundFXPlayer::getModeActiveCount(SoundPlayMode mode) const
{
    int count = 0;

    for (void* p : _soundInstances)
    {
        auto* inst = static_cast<SoundInstanceInternal*>(p);

        if (inst && inst->mode == mode)
        {
            ++count;
        }
    }

    return count;
}

void SoundFXPlayer::init()
{
    if (_engine)
    {
        return;
    }

    ma_engine_config engineConfig = ma_engine_config_init();
    engineConfig.sampleRate = 44100;

    ma_engine* engine = new ma_engine();

    if (ma_engine_init(&engineConfig, engine) != MA_SUCCESS)
    {
        std::cerr << "SoundFX: failed to init audio engine" << std::endl;

        delete engine;
        _engine = nullptr;

        return;
    }

    _engine = engine;
    _ownsEngine = true;

    // Master ����� ����������� ����� AudioManager, ���� �� ���������.
    ma_engine_set_volume(engine, 1.0f);
}

void SoundFXPlayer::setMuted(bool bMuted)
{
    if (_bMuted != bMuted)
    {
        _bMuted = bMuted;

        if (_bMuted)
        {
            stopAllSounds();
            setBusVolume(0.0f);
        }
        else
        {
            setBusVolume(_masterVolume / 100.0f);
        }

        emitSfx(SFX_EVT_MUTE_CHANGED, audioInt(bMuted ? 1 : 0));
    }
}

void SoundFXPlayer::setMasterVolume(float volume)
{
    _masterVolume = std::clamp(volume, 0.f, 1.f);
    float vol = _bMuted ? 0.0f : _masterVolume;
    setBusVolume(vol);
}

void SoundFXPlayer::setBusVolume(float volume)
{
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    _busVolume = volume;

    for (void* p : _soundInstances)
    {
        auto* inst = static_cast<SoundInstanceInternal*>(p);

        if (inst)
        {
            ma_sound_set_volume(
                &inst->sound,
                inst->baseVolume * _busVolume
            );
        }
    }
}

int SoundFXPlayer::loadSprite(
    const char* name,
    const char* url,
    const std::unordered_map<std::string, SoundSpriteData>& sounds,
    SimpleCallback cb
)
{
    std::string nameStr(name ? name : "");
    std::string urlStr(url ? url : "");

    if (nameStr.empty() || urlStr.empty())
    {
        std::cerr << "SoundFX: loadSprite - invalid parameters" << std::endl;

        emitSfx(SFX_EVT_LOAD_FAILED, audioInt(0));

        if (cb)
        {
            cb();
        }

        return 0;
    }

    auto it = _loadedSprites.find(nameStr);

    if (it != _loadedSprites.end())
    {
        if (cb)
        {
            cb();
        }

        return it->second.bufferId;
    }

    if (!_engine)
    {
        init();
    }

    if (!_engine)
    {
        std::cerr << "SoundFX: engine not initialized" << std::endl;

        emitSfx(SFX_EVT_LOAD_FAILED, audioInt(0));

        if (cb)
        {
            cb();
        }

        return 0;
    }

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    ma_decoder decoder;

    if (ma_decoder_init_file(urlStr.c_str(), &config, &decoder) != MA_SUCCESS)
    {
        std::cerr << "SoundFX: failed to open file: " << urlStr << std::endl;

        emitSfx(
            SFX_EVT_LOAD_FAILED,
            audioInt(0),
            const_cast<char*>(nameStr.c_str())
        );

        if (cb)
        {
            cb();
        }

        return 0;
    }

    ma_uint64 estimatedFrames = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &estimatedFrames);

    std::vector<float> pcmData;

    if (estimatedFrames > 0)
    {
        pcmData.reserve(
            static_cast<size_t>(estimatedFrames * decoder.outputChannels)
        );
    }

    float tempBuffer[4096 * 8];
    ma_uint64 totalFrames = 0;

    while (true)
    {
        ma_uint64 framesRead = 0;

        ma_result result = ma_decoder_read_pcm_frames(
            &decoder,
            tempBuffer,
            4096,
            &framesRead
        );

        if (framesRead == 0)
        {
            break;
        }

        pcmData.insert(
            pcmData.end(),
            tempBuffer,
            tempBuffer + static_cast<size_t>(framesRead * decoder.outputChannels)
        );

        totalFrames += framesRead;

        if (result != MA_SUCCESS)
        {
            break;
        }
    }

    ma_decoder_uninit(&decoder);

    if (pcmData.empty() || totalFrames == 0)
    {
        std::cerr << "SoundFX: no audio data decoded: " << urlStr << std::endl;

        emitSfx(
            SFX_EVT_LOAD_FAILED,
            audioInt(0),
            const_cast<char*>(nameStr.c_str())
        );

        if (cb)
        {
            cb();
        }

        return 0;
    }

    SoundSprite sprite;

    sprite.url = urlStr;
    sprite.bufferId = _bufferCounter++;
    sprite.sounds = sounds;
    sprite.pcmData = std::move(pcmData);
    sprite.channels = decoder.outputChannels;
    sprite.sampleRate = decoder.outputSampleRate;
    sprite.totalFrames = totalFrames;

    int bufferId = sprite.bufferId;

    _loadedSprites[nameStr] = std::move(sprite);

    for (const auto& sound : sounds)
    {
        if (!sound.first.empty())
        {
            _soundToSprite[sound.first] = nameStr;
        }
    }

    emitSfx(
        SFX_EVT_LOADED,
        audioInt(bufferId),
        const_cast<char*>(nameStr.c_str())
    );

    if (cb)
    {
        cb();
    }

    return bufferId;
}

bool SoundFXPlayer::loadSpriteMetadata(
    const char* name,
    const char* url,
    const char* jsonMetadata,
    SimpleCallback cb
)
{
    if (!name || !jsonMetadata)
    {
        std::cerr << "SoundFX: loadSpriteMetadata - null parameters" << std::endl;

        if (cb)
        {
            cb();
        }

        return false;
    }

    CJson json(jsonMetadata);

    if (!json._child)
    {
        std::cerr << "SoundFX: loadSpriteMetadata - invalid JSON" << std::endl;

        if (cb)
        {
            cb();
        }

        return false;
    }

    CJson* resources = CJson::getItem(&json, "resources");
    std::string audioUrl = url ? url : "";

    if (resources && resources->_child && resources->_child->_valueString)
    {
        audioUrl = resources->_child->_valueString;
    }

    CJson* spritemap = CJson::getItem(&json, "spritemap");

    if (!spritemap)
    {
        std::cerr << "SoundFX: loadSpriteMetadata - no spritemap found" << std::endl;

        if (cb)
        {
            cb();
        }

        return false;
    }

    std::unordered_map<std::string, SoundSpriteData> sounds;

    for (CJson* child = spritemap->_child; child; child = child->_next)
    {
        if (child->_name)
        {
            SoundSpriteData data;

            data.start = CJson::getFloat(child, "start", 0.0f);

            float end = CJson::getFloat(child, "end", 0.0f);
            data.duration = end - data.start;
            data.loop = CJson::getBoolean(child, "loop", false);

            sounds[child->_name] = data;
        }
    }

    if (sounds.empty())
    {
        std::cerr << "SoundFX: loadSpriteMetadata - no sounds found in spritemap"
                  << std::endl;

        if (cb)
        {
            cb();
        }

        return false;
    }
    //auto s = "EMBED/" + audioUrl;
    auto s = audioUrl;
    return loadSprite(name, s.c_str(), sounds, cb) != 0;
}

int SoundFXPlayer::loadSound(const char* name, const char* url)
{
    std::string nameStr(name ? name : "");
    std::string urlStr(url ? url : "");

    if (nameStr.empty() || urlStr.empty())
    {
        std::cerr << "SoundFX: loadSound - invalid parameters" << std::endl;
        return 0;
    }

    SoundSpriteData data;
    data.start = 0.0f;
    data.duration = 0.0f;
    data.loop = false;

    std::unordered_map<std::string, SoundSpriteData> sounds;
    sounds[nameStr] = data;

    return loadSprite(nameStr.c_str(), urlStr.c_str(), sounds);
}

void SoundFXPlayer::loadSoundAsync(
    const char* name,
    const char* url,
    std::function<void(bool success)> callback
)
{
    int result = loadSound(name, url);

    if (callback)
    {
        callback(result != 0);
    }
}

bool SoundFXPlayer::isSoundLoaded(const char* name) const
{
    std::string nameStr(name ? name : "");

    if (nameStr.empty())
    {
        return false;
    }

    auto spriteIt = _loadedSprites.find(nameStr);

    if (spriteIt != _loadedSprites.end())
    {
        return !spriteIt->second.pcmData.empty();
    }

    auto soundIt = _soundToSprite.find(nameStr);

    if (soundIt != _soundToSprite.end())
    {
        auto spIt = _loadedSprites.find(soundIt->second);

        if (spIt != _loadedSprites.end())
        {
            return !spIt->second.pcmData.empty();
        }
    }

    return false;
}

void SoundFXPlayer::update(float dt)
{
    for (auto it = _soundInstances.begin(); it != _soundInstances.end(); )
    {
        auto* inst = static_cast<SoundInstanceInternal*>(*it);

        if (inst && ma_sound_at_end(&inst->sound))
        {
            int instanceId = inst->id;

            emitSfx(
                SFX_EVT_ENDED,
                audioInt(instanceId),
                const_cast<char*>(inst->name.c_str())
            );

            ma_sound_uninit(&inst->sound);
            ma_audio_buffer_ref_uninit(&inst->bufferRef);

            delete inst;

            it = _soundInstances.erase(it);

            auto ait = std::find(
                _activeInstances.begin(),
                _activeInstances.end(),
                instanceId
            );

            if (ait != _activeInstances.end())
            {
                _activeInstances.erase(ait);
            }

            auto modeIt = _instanceModes.find(instanceId);

            if (modeIt != _instanceModes.end())
            {
                _instanceModes.erase(modeIt);
            }
        }
        else
        {
            ++it;
        }
    }
}

int SoundFXPlayer::playSound(
    const char* name,
    float volume,
    SoundPlayMode mode
)
{
    return playSoundLooped(name, volume, false, mode);
}

int SoundFXPlayer::playSoundLooped(
    const char* name,
    float volume,
    bool loop,
    SoundPlayMode mode
)
{
    std::string nameStr(name ? name : "");

    if (nameStr.empty())
    {
        std::cerr << "SoundFX: playSoundLooped - empty sound name" << std::endl;
        return 0;
    }

    if (std::isnan(volume) || std::isinf(volume))
    {
        std::cerr << "SoundFX: playSoundLooped - invalid volume value" << std::endl;
        return 0;
    }

    volume = std::max(0.0f, std::min(1.0f, volume));

    if (_bMuted)
    {
        return 0;
    }

    if (!_engine)
    {
        return 0;
    }

    int modeIdx = static_cast<int>(mode);

    if (modeIdx == 1 || modeIdx == 2)
    {
        int limit = _modeLimits[modeIdx];

        if (limit > 0 && getModeActiveCount(mode) >= limit)
        {
            std::cout << "Sound '" << nameStr
                      << "' not started: mode " << modeIdx
                      << " limit reached (" << limit << ")"
                      << std::endl;

            return 0;
        }
    }

    std::string spriteName;
    SoundSpriteData soundData;

    auto spriteIt = _loadedSprites.find(nameStr);

    if (spriteIt != _loadedSprites.end())
    {
        spriteName = nameStr;

        soundData.start = 0.0f;
        soundData.duration = 0.0f;
    }
    else
    {
        auto soundIt = _soundToSprite.find(nameStr);

        if (soundIt == _soundToSprite.end())
        {
            std::cerr << "Sound '" << nameStr << "' not found in any sprite"
                      << std::endl;
            return 0;
        }

        spriteName = soundIt->second;

        auto& sprite = _loadedSprites[spriteName];
        auto sIt = sprite.sounds.find(nameStr);

        if (sIt == sprite.sounds.end())
        {
            std::cerr << "Sound '" << nameStr
                      << "' not found in sprite '"
                      << spriteName << "'"
                      << std::endl;
            return 0;
        }

        soundData = sIt->second;
    }

    auto& sprite = _loadedSprites[spriteName];

    if (sprite.pcmData.empty() || sprite.totalFrames == 0)
    {
        std::cerr << "SoundFX: sprite '" << spriteName << "' has no audio data"
                  << std::endl;
        return 0;
    }

    if (std::isnan(soundData.start) || std::isinf(soundData.start))
    {
        soundData.start = 0.0f;
    }

    if (std::isnan(soundData.duration) || std::isinf(soundData.duration))
    {
        soundData.duration = 0.0f;
    }

    if (soundData.start < 0.0f)
    {
        soundData.start = 0.0f;
    }

    if (soundData.duration < 0.0f)
    {
        soundData.duration = 0.0f;
    }

    ma_uint64 startFrame =
        static_cast<ma_uint64>(soundData.start * sprite.sampleRate);

    ma_uint64 durationFrame = 0;

    if (soundData.duration > 0.0f)
    {
        durationFrame =
            static_cast<ma_uint64>(soundData.duration * sprite.sampleRate);
    }
    else
    {
        durationFrame = sprite.totalFrames - startFrame;
    }

    if (startFrame >= sprite.totalFrames)
    {
        startFrame = 0;
    }

    if (startFrame + durationFrame > sprite.totalFrames)
    {
        durationFrame = sprite.totalFrames - startFrame;
    }

    float* subData =
        sprite.pcmData.data() + (startFrame * sprite.channels);

    ma_uint64 subFrames = durationFrame;

    int instanceId = ++_instanceCounter;

    auto* instance = new SoundInstanceInternal();

    instance->id = instanceId;
    instance->name = nameStr;
    instance->spriteName = spriteName;
    instance->loop = loop;
    instance->mode = mode;
    instance->baseVolume = volume;

    ma_result result = ma_audio_buffer_ref_init(
        ma_format_f32,
        sprite.channels,
        subData,
        subFrames,
        &instance->bufferRef
    );

    if (result != MA_SUCCESS)
    {
        std::cerr << "SoundFX: failed to init audio buffer ref" << std::endl;

        delete instance;
        return 0;
    }

    result = ma_sound_init_from_data_source(
        static_cast<ma_engine*>(_engine),
        &instance->bufferRef,
        0,
        nullptr,
        &instance->sound
    );

    if (result != MA_SUCCESS)
    {
        std::cerr << "SoundFX: failed to init sound from data source" << std::endl;

        ma_audio_buffer_ref_uninit(&instance->bufferRef);
        delete instance;

        return 0;
    }

    ma_sound_set_volume(
        &instance->sound,
        instance->baseVolume * _busVolume
    );

    ma_sound_set_looping(
        &instance->sound,
        loop ? MA_TRUE : MA_FALSE
    );

    result = ma_sound_start(&instance->sound);

    if (result != MA_SUCCESS)
    {
        std::cerr << "SoundFX: failed to start sound" << std::endl;

        ma_sound_uninit(&instance->sound);
        ma_audio_buffer_ref_uninit(&instance->bufferRef);
        delete instance;

        return 0;
    }

    _activeInstances.push_back(instanceId);
    _instanceModes[instanceId] = mode;
    _soundInstances.push_back(instance);

    emitSfx(
        SFX_EVT_PLAY,
        audioInt(instanceId),
        const_cast<char*>(nameStr.c_str())
    );

    return instanceId;
}

void SoundFXPlayer::stopSound(int instanceId)
{
    for (auto it = _soundInstances.begin(); it != _soundInstances.end(); ++it)
    {
        auto* inst = static_cast<SoundInstanceInternal*>(*it);

        if (inst && inst->id == instanceId)
        {
            ma_sound_stop(&inst->sound);
            ma_sound_uninit(&inst->sound);
            ma_audio_buffer_ref_uninit(&inst->bufferRef);

            delete inst;

            _soundInstances.erase(it);
            break;
        }
    }

    auto ait = std::find(
        _activeInstances.begin(),
        _activeInstances.end(),
        instanceId
    );

    if (ait != _activeInstances.end())
    {
        _activeInstances.erase(ait);
    }

    auto modeIt = _instanceModes.find(instanceId);

    if (modeIt != _instanceModes.end())
    {
        _instanceModes.erase(modeIt);
    }

    emitSfx(SFX_EVT_STOP, audioInt(instanceId));
}

void SoundFXPlayer::stopSoundByName(const char* name)
{
    std::string nameStr(name ? name : "");

    if (nameStr.empty())
    {
        std::cerr << "SoundFX: stopSoundByName - empty sound name" << std::endl;
        return;
    }

    std::vector<int> stoppedIds;

    for (auto it = _soundInstances.begin(); it != _soundInstances.end(); )
    {
        auto* inst = static_cast<SoundInstanceInternal*>(*it);

        if (inst && inst->name == nameStr)
        {
            stoppedIds.push_back(inst->id);

            ma_sound_stop(&inst->sound);
            ma_sound_uninit(&inst->sound);
            ma_audio_buffer_ref_uninit(&inst->bufferRef);

            delete inst;

            it = _soundInstances.erase(it);
        }
        else
        {
            ++it;
        }
    }

    _activeInstances.erase(
        std::remove_if(
            _activeInstances.begin(),
            _activeInstances.end(),
            [&stoppedIds](int id) {
                return std::find(
                    stoppedIds.begin(),
                    stoppedIds.end(),
                    id
                ) != stoppedIds.end();
            }
        ),
        _activeInstances.end()
    );

    for (int id : stoppedIds)
    {
        auto modeIt = _instanceModes.find(id);

        if (modeIt != _instanceModes.end())
        {
            _instanceModes.erase(modeIt);
        }
    }
}

void SoundFXPlayer::stopAllSounds()
{
    for (void* p : _soundInstances)
    {
        auto* inst = static_cast<SoundInstanceInternal*>(p);

        if (inst)
        {
            ma_sound_stop(&inst->sound);
            ma_sound_uninit(&inst->sound);
            ma_audio_buffer_ref_uninit(&inst->bufferRef);

            delete inst;
        }
    }

    _soundInstances.clear();
    _activeInstances.clear();
    _instanceModes.clear();

    emitSfx(SFX_EVT_STOP_ALL);
}

void SoundFXPlayer::setInstanceVolume(int instanceId, float volume)
{
    if (std::isnan(volume) || std::isinf(volume))
    {
        std::cerr << "SoundFX: setInstanceVolume - invalid volume value" << std::endl;
        return;
    }

    volume = std::max(0.0f, std::min(1.0f, volume));

    for (void* p : _soundInstances)
    {
        auto* inst = static_cast<SoundInstanceInternal*>(p);

        if (inst && inst->id == instanceId)
        {
            inst->baseVolume = volume;

            ma_sound_set_volume(
                &inst->sound,
                inst->baseVolume * _busVolume
            );

            return;
        }
    }
}

bool SoundFXPlayer::isSoundPlaying(int instanceId) const
{
    for (void* p : _soundInstances)
    {
        auto* inst = static_cast<SoundInstanceInternal*>(p);

        if (inst && inst->id == instanceId)
        {
            return ma_sound_is_playing(&inst->sound) == MA_TRUE;
        }
    }

    return false;
}

void SoundFXPlayer::unloadSound(const char* name)
{
    std::string nameStr(name ? name : "");

    if (nameStr.empty())
    {
        std::cerr << "SoundFX: unloadSound - empty sound name" << std::endl;
        return;
    }

    auto spriteIt = _loadedSprites.find(nameStr);

    if (spriteIt != _loadedSprites.end())
    {
        for (auto it = _soundInstances.begin(); it != _soundInstances.end(); )
        {
            auto* inst = static_cast<SoundInstanceInternal*>(*it);

            if (inst && inst->spriteName == nameStr)
            {
                ma_sound_stop(&inst->sound);
                ma_sound_uninit(&inst->sound);
                ma_audio_buffer_ref_uninit(&inst->bufferRef);

                delete inst;

                it = _soundInstances.erase(it);
            }
            else
            {
                ++it;
            }
        }

        for (const auto& sound : spriteIt->second.sounds)
        {
            _soundToSprite.erase(sound.first);
        }

        _loadedSprites.erase(spriteIt);
        return;
    }

    auto soundIt = _soundToSprite.find(nameStr);

    if (soundIt != _soundToSprite.end())
    {
        std::string spriteName = soundIt->second;
        _soundToSprite.erase(soundIt);

        auto spIt = _loadedSprites.find(spriteName);

        if (spIt != _loadedSprites.end())
        {
            spIt->second.sounds.erase(nameStr);

            if (spIt->second.sounds.empty())
            {
                for (auto it = _soundInstances.begin(); it != _soundInstances.end(); )
                {
                    auto* inst = static_cast<SoundInstanceInternal*>(*it);

                    if (inst && inst->spriteName == spriteName)
                    {
                        ma_sound_stop(&inst->sound);
                        ma_sound_uninit(&inst->sound);
                        ma_audio_buffer_ref_uninit(&inst->bufferRef);

                        delete inst;

                        it = _soundInstances.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }

                _loadedSprites.erase(spIt);
            }
        }
    }
}

void SoundFXPlayer::unloadAllSounds()
{
    stopAllSounds();

    _loadedSprites.clear();
    _soundToSprite.clear();
}

size_t SoundFXPlayer::getLoadedSoundCount() const
{
    size_t count = 0;

    for (const auto& sprite : _loadedSprites)
    {
        count += sprite.second.sounds.size();
    }

    return count;
}

size_t SoundFXPlayer::getActiveInstanceCount() const
{
    return _soundInstances.size();
}

#endif // G2D_WEB_AUDIO

_G2D_NAMESPACE_END_