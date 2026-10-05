#include "audiomanager.h"

#include "mp3player.h"
#include "soundfx.h"

#include <algorithm>
#include <cmath>

#ifdef G2D_WEB_AUDIO
    #include <emscripten.h>
#else
    #include "miniaudio.h"
#endif

_G2D_NAMESPACE_BEGIN_

AudioManager& AudioManager::get()
{
    static AudioManager* instance = new AudioManager();
    return *instance;
}

void AudioManager::init()
{
    if (_initialized)
    {
        return;
    }

#ifndef G2D_WEB_AUDIO
    if (!_nativeEngine)
    {
        ma_engine* pEngine = new ma_engine();

        if (ma_engine_init(nullptr, pEngine) == MA_SUCCESS)
        {
            _nativeEngine = pEngine;
        }
        else
        {
            delete pEngine;
            pEngine = nullptr;
        }
    }

    if (_nativeEngine)
    {
        MP3Player::get().attachEngine(static_cast<ma_engine*>(_nativeEngine));
        SoundFXPlayer::get().attachEngine(_nativeEngine);
    }
#endif

    MP3Player::get().init();
    SoundFXPlayer::get().init();

    MP3Player::get().setEventEmitter(this);
    SoundFXPlayer::get().setEventEmitter(this);

    ensureAudioGraph();
    applyVolumes();

    _initialized = true;

    emit(AUDIO_EVT_INIT);
}

void AudioManager::update(float dt)
{
    MP3Player::get().update(dt);
    SoundFXPlayer::get().update(dt);
}

void AudioManager::unlockAudioContext()
{
    MP3Player::resumeAudioContext();

    bool bUnlocked = MP3Player::get().isContextUnlocked();

    if (bUnlocked && !_contextUnlocked)
    {
        _contextUnlocked = true;
        emit(AUDIO_EVT_CONTEXT_UNLOCKED);
    }

    MP3Player::get().tryStartMusic();
}

bool AudioManager::isContextUnlocked() const
{
    return MP3Player::get().isContextUnlocked();
}

void AudioManager::setMasterVolume(float volume)
{
    volume = std::clamp(volume, 0.f, 1.f);

    _masterVolume = volume;
    applyVolumes();
}

void AudioManager::setMusicVolume(float volume)
{
    volume = std::clamp(volume, 0.f, 1.f);
    _musicVolume = volume;
    applyVolumes();
}

void AudioManager::setSfxVolume(float volume)
{
    volume = std::clamp(volume, 0.f, 1.f);

    _sfxVolume = volume;
    applyVolumes();
}

void AudioManager::setMasterMuted(bool bMuted)
{
    if (_masterMuted == bMuted)
    {
        return;
    }

    _masterMuted = bMuted;
    applyVolumes();

    emit(AUDIO_EVT_MASTER_MUTE_CHANGED, audioInt(bMuted ? 1 : 0));
}

void AudioManager::setMusicMuted(bool bMuted)
{
    if (_musicMuted == bMuted)
    {
        return;
    }

    _musicMuted = bMuted;
    applyVolumes();

    emit(MUSIC_EVT_MUTE_CHANGED, audioInt(bMuted ? 1 : 0));
}

void AudioManager::setSfxMuted(bool bMuted)
{
    if (_sfxMuted == bMuted)
    {
        return;
    }

    _sfxMuted = bMuted;
    applyVolumes();

    emit(SFX_EVT_MUTE_CHANGED, audioInt(bMuted ? 1 : 0));
}

void AudioManager::playMusicPlaylist(const Strings& urls, SimpleCallback onComplete)
{
    auto& music = MP3Player::get();

    music.clearPlaylist(false);

    for (const auto& url : urls)
    {
        music.addToPlaylist(url);
    }

    music.playPlaylist(onComplete);
}

void AudioManager::crossfadeMusicPlaylist(const Strings& urls, float fadeDurationSec)
{
    MP3Player::get().crossfadeToPlaylist(urls, fadeDurationSec);
}

void AudioManager::pauseMusic()
{
    MP3Player::get().pause();
}

void AudioManager::resumeMusic()
{
    MP3Player::get().play();
}

void AudioManager::stopMusic()
{
    MP3Player::get().stop();
}

int AudioManager::loadSprite(LPCTSTR name, LPCTSTR url, const MapSoundSprites& sounds, SimpleCallback cb)
{
    return SoundFXPlayer::get().loadSprite(name, url, sounds, cb);
}

bool AudioManager::loadSpriteMetadata(LPCTSTR name, LPCTSTR url, LPCTSTR jsonMetadata,SimpleCallback cb
)
{
    return SoundFXPlayer::get().loadSpriteMetadata(name, url, jsonMetadata, cb);
}

int AudioManager::loadSound(LPCTSTR name, LPCTSTR url)
{
    return SoundFXPlayer::get().loadSound(name, url);
}

void AudioManager::loadSoundAsync(LPCTSTR name, LPCTSTR url, BoolStateCallback callback)
{
    SoundFXPlayer::get().loadSoundAsync(name, url, callback);
}

bool AudioManager::isSoundLoaded(LPCTSTR name) const
{
    return SoundFXPlayer::get().isSoundLoaded(name);
}

int AudioManager::playSound(LPCTSTR name, float volume, SoundPlayMode mode)
{
    return SoundFXPlayer::get().playSound(name, volume, mode);
}

int AudioManager::playSoundLooped(LPCTSTR name, float volume, bool loop, SoundPlayMode mode)
{
    return SoundFXPlayer::get().playSoundLooped(name, volume, loop, mode);
}

void AudioManager::stopSound(int instanceId)
{
    SoundFXPlayer::get().stopSound(instanceId);
}

void AudioManager::stopSoundByName(LPCTSTR name)
{
    SoundFXPlayer::get().stopSoundByName(name);
}

void AudioManager::stopAllSounds()
{
    SoundFXPlayer::get().stopAllSounds();
}

void AudioManager::setInstanceVolume(int instanceId, float volume)
{
    SoundFXPlayer::get().setInstanceVolume(instanceId, volume);
}

bool AudioManager::isSoundPlaying(int instanceId) const
{
    return SoundFXPlayer::get().isSoundPlaying(instanceId);
}

void AudioManager::unloadSound(LPCTSTR name)
{
    SoundFXPlayer::get().unloadSound(name);
}

void AudioManager::unloadAllSounds()
{
    SoundFXPlayer::get().unloadAllSounds();
}

size_t AudioManager::getLoadedSoundCount() const
{
    return SoundFXPlayer::get().getLoadedSoundCount();
}

size_t AudioManager::getActiveInstanceCount() const
{
    return SoundFXPlayer::get().getActiveInstanceCount();
}

void AudioManager::ensureAudioGraph()
{
#ifdef G2D_WEB_AUDIO
    EM_ASM({
        try
        {
            window.GameAudio = window.GameAudio || {};

            if (!window.GameAudio.audioContext)
            {
                window.GameAudio.audioContext =
                    new (window.AudioContext || window.webkitAudioContext)();
            }

            var ctx = window.GameAudio.audioContext;

            if (!window.GameAudio.masterGain)
            {
                window.GameAudio.masterGain = ctx.createGain();
                window.GameAudio.masterGain.connect(ctx.destination);
                window.GameAudio.masterGain.gain.value = 1.0;
            }

            if (!window.GameAudio.musicMasterGain)
            {
                window.GameAudio.musicMasterGain = ctx.createGain();
            }

            try
            {
                window.GameAudio.musicMasterGain.disconnect();
            }
            catch (e) {}

            window.GameAudio.musicMasterGain.connect(window.GameAudio.masterGain);

            if (!window.GameAudio.sfxMasterGain)
            {
                window.GameAudio.sfxMasterGain = ctx.createGain();
            }

            try
            {
                window.GameAudio.sfxMasterGain.disconnect();
            }
            catch (e) {}

            window.GameAudio.sfxMasterGain.connect(window.GameAudio.masterGain);

            window.GameAudio.buffers = window.GameAudio.buffers || {};
            window.GameAudio.bufferCounter = window.GameAudio.bufferCounter || 1;
            window.GameAudio.sfxBuffers = window.GameAudio.sfxBuffers || {};
            window.GameAudio.sfxBufferCounter = window.GameAudio.sfxBufferCounter || 1000;
            window.GameAudio.sfxInstances = window.GameAudio.sfxInstances || [];
        }
        catch (e)
        {
            console.error("AudioManager.ensureAudioGraph failed:", e);
        }
    });
#endif
}

// ---------------------------------------------------------------------------
// Применение громкостей
// ---------------------------------------------------------------------------

void AudioManager::applyVolumes()
{
    float master = masterGain();
    float music = musicGain();
    float sfx = sfxGain();

#ifdef G2D_WEB_AUDIO
    EM_ASM({
        try
        {
            if (!window.GameAudio)
            {
                return;
            }

            if (window.GameAudio.masterGain)
            {
                window.GameAudio.masterGain.gain.value = $0;
            }

            if (window.GameAudio.musicMasterGain)
            {
                window.GameAudio.musicMasterGain.gain.value = $1;
            }

            if (window.GameAudio.sfxMasterGain)
            {
                window.GameAudio.sfxMasterGain.gain.value = $2;
            }
        }
        catch (e)
        {
            console.error("AudioManager.applyVolumes failed:", e);
        }
    }, master, music, sfx);
#else
    if (_nativeEngine)
    {
        ma_engine_set_volume(
            static_cast<ma_engine*>(_nativeEngine),
            master
        );

        MP3Player::get().setBusVolume(music);
        SoundFXPlayer::get().setBusVolume(sfx);
    }
    else
    {
        MP3Player::get().setBusVolume(master * music);
        SoundFXPlayer::get().setBusVolume(master * sfx);
    }
#endif
}

_G2D_NAMESPACE_END_