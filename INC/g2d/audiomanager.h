#pragma once

#include "g2d.h"
#include "audioevents.h"
#include "eventemmiter.h"
#include "soundfx.h"

_G2D_NAMESPACE_BEGIN_

typedef std::unordered_map<std::string, SoundSpriteData> MapSoundSprites;

class AudioManager : public CEventEmmiter,
                     public IAudioEventEmitter
{
public:
    AudioManager() = default;
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    static AudioManager& get();

    void                    init                    (void);
    void                    update                  (float                  dt);
    void                    unlockAudioContext      (void);                 
    bool                    isContextUnlocked       (void) const;           
    void                    setMasterVolume         (float                  volume);
    void                    setMusicVolume          (float                  volume);
    void                    setSfxVolume            (float                  volume);
    void                    setMasterMuted          (bool                   bMuted);
    void                    setMusicMuted           (bool                   bMuted);
    void                    setSfxMuted             (bool                   bMuted);
    void                    playMusicPlaylist       (const Strings&         urls, 
                                                     SimpleCallback         onComplete);
    void                    crossfadeMusicPlaylist  (const Strings&         urls, 
                                                     float                  fadeDurationSec);
    void                    pauseMusic              (void);
    void                    resumeMusic             (void);
    void                    stopMusic               (void);
    int                     loadSprite              (LPCTSTR                name, 
                                                     LPCTSTR                url,
                                                     const MapSoundSprites& sounds, 
                                                     SimpleCallback         cb = nullptr);
    bool                    loadSpriteMetadata      (LPCTSTR                name, 
                                                     LPCTSTR                url, 
                                                     LPCTSTR                jsonMetadata, 
                                                     SimpleCallback         cb = nullptr);
    int                     loadSound               (LPCTSTR                name, 
                                                     LPCTSTR                url);
    void                    loadSoundAsync          (LPCTSTR                name, 
                                                     LPCTSTR                url, 
                                                     BoolStateCallback      callback);
    bool                    isSoundLoaded           (LPCTSTR                name) const;
    int                     playSound               (LPCTSTR                name, 
                                                     float                  volume = 1.0f, 
                                                     SoundPlayMode          mode = SoundPlayMode::Unlimited);
    int                     playSoundLooped         (LPCTSTR                name, 
                                                     float                  volume = 1.0f, 
                                                     bool                   loop = true, 
                                                     SoundPlayMode          mode = SoundPlayMode::Unlimited);
    void                    stopSound               (int                    instanceId);
    void                    stopSoundByName         (LPCTSTR                name);
    void                    stopAllSounds           (void);
    void                    setInstanceVolume       (int                    instanceId, 
                                                     float                  volume);
    bool                    isSoundPlaying          (int                    instanceId) const;
    void                    unloadSound             (LPCTSTR                name);
    void                    unloadAllSounds         (void);
    size_t                  getLoadedSoundCount     (void) const;
    size_t                  getActiveInstanceCount  (void) const;
    auto                    getMusicVolume          (void) const { return _musicVolume; }
    auto                    getSfxVolume            (void) const { return _sfxVolume; }
    bool                    isMasterMuted           (void) const { return _masterMuted; }
    bool                    isMusicMuted            (void) const { return _musicMuted; }
    bool                    isSfxMuted              (void) const { return _sfxMuted; }
    auto                    getMasterVolume         (void) const { return _masterVolume; }

    virtual void            emitAudioEvent          (int                    nEvent, 
                                                     void*                  pData1 = nullptr, 
                                                     void*                  pData2 = nullptr, 
                                                     void*                  pData3 = nullptr) override
    {
        emit(nEvent, pData1, pData2, pData3);
    }

private:
    void                    ensureAudioGraph        (void);
    void                    applyVolumes            (void);

    float                   masterGain              (void) const
    {
        if (_masterMuted) return 0.0f;
        return _masterVolume;
    }

    float                   musicGain               (void) const
    {
        if (_musicMuted) return 0.0f;
        return _musicVolume;
    }

    float                   sfxGain                 (void) const
    {
        if (_sfxMuted) return 0.0f;
        return _sfxVolume;
    }

private:
    bool    _initialized     = false;
    bool    _contextUnlocked = false;

    float   _masterVolume    = 1.f;
    float   _musicVolume     = 1.f;
    float   _sfxVolume       = 1.f;
                             
    bool    _masterMuted     = false;
    bool    _musicMuted      = false;
    bool    _sfxMuted        = false;
    void*   _nativeEngine    = nullptr;
};

_G2D_NAMESPACE_END_