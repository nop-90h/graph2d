#pragma once

#include "g2d.h"
#include "texture.h"
#include "loader.h"
#include "matrixstack.h"
#include "container.h"
#include "sceneresize.h"
#include "sprite.h"
#include "spine/Vector.h"

#include <cstdint>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>
#include <chrono>

_G2D_NAMESPACE_BEGIN_

#define MAX_VERTICIES       (4096 * 4)
#define VERTEX_SIZE         144

static constexpr size_t VERTEX_FLOATS = VERTEX_SIZE / sizeof(float);
static constexpr size_t MAX_INDICES   = MAX_VERTICIES * 3;

#define SAMPLERS_COUNT      8
#define MAX_LIGHTS          256
#define MAX_UBO_LIGHTS      204
enum class eHitColor
{
    NORMAL,
    PURPLE_NEON
};

typedef struct
{
    GLint a_position;
    GLint a_texCoord;
    GLint a_mat3;
    GLint a_tintColor;
    GLint a_reflectionType;
    GLint a_isNormalBlend;
    GLint a_addRGB;
    GLint a_sparkLife;
    GLint a_effectType;
    GLint a_tutIntensity;
    GLint a_texMuls;
    GLint a_params;
} shader_attribs_loc_t;

struct ParticleQuadData
{
    float verts[8];
    float uvs[8];
    float rgba[4];
};

enum class eRenderLayer
{
    GAME,
    BACKGORUND,
    FOREGROUND,
    FOREGROUND_CONTROLS,
    FOREGROUND_EFFECTS,
};

struct CameraInfo
{
    float zoom;
    float gameX, gameY;
    float bgX, bgY;
};

struct Shockwave
{
    float time      = 2.0f;
    float centerX   = 0.5f;
    float centerY   = 0.5f;
    float duration  = 0.4f;
};

enum class eWorldDarkenState
{
    BASIC,
    DARKENING,
    DARK,
    BRIGHTENING,
    BRIGHT,
};

struct HitEffect
{
    float x, y, radius, intensity;
    bool  active = false;
};

struct BloomSettings
{
    bool  enabled   = true;
    float intensity = 0.7f;
    float threshold = 0.75f;
    float radius    = 1.0f;

    BloomSettings()
        : enabled(true)
        , intensity(0.5f)
        , threshold(0.75f)
        , radius(1.0f)
    {}
};

struct PostProcessSettings
{
    bool  enabled                    = true;
    float vignetteIntensity          = 0.05f;
    float vignetteRadius             = 0.2f;
    float vignetteSmoothness         = 0.9f;
    float saturation                 = 0.85f;
    float contrast                   = 1.15f;
    float brightness                 = -0.08f;
    float grainIntensity             = 0.06f;
    float aberration                 = 0.002f;
    float tintR                      = 0.80f;
    float tintG                      = 0.78f;
    float tintB                      = 0.90f;
    BloomSettings bloom;

    float acesIntensity              = 1.0f;
    float lutIntensity               = 1.0f;
    float aoIntensity                = 0.6f;
    float envSaturation              = 0.45f;
    float chromaticVig               = 0.5f;
    float ditherIntensity            = 0.025f;
    float distortionIntensity        = 0.0f;
    float vignetteBreatheIntensity   = 0.0f;
    float fisheyeIntensity           = 0.0f;
    float fleshLUTIntensity          = 0.0f;
    float ghostTrailIntensity        = 0.0f;
    float ghostTrailSpeed            = 1.0f;
    float fRainIntensity             = 0.0f;
    float fFogIntensity              = 0.0f;
    float fBloodDrops                = 0.0f;
    float fStarTrailIntensity        = 0.0f;
    bool  isNarrativeMode            = false;

    PostProcessSettings()
    {
        enabled                    = true;
        vignetteIntensity          = 0.8f;
        vignetteRadius             = 0.35f;
        vignetteSmoothness         = 0.35f;
        saturation                 = 0.85f;
        contrast                   = 1.18f;
        brightness                 = -0.00f;
        grainIntensity             = 0.1f;
        aberration                 = 0.005f;
        tintR                      = 0.85f;
        tintG                      = 0.82f;
        tintB                      = 0.92f;
        bloom.enabled              = true;
        bloom.intensity            = 3.5f;
        bloom.threshold            = 0.05f;
        bloom.radius               = 1.0f;
        acesIntensity              = .2f;
        lutIntensity               = 0.f;
        aoIntensity                = 0.0f;
        envSaturation              = 0.0f;
        chromaticVig               = 0.0f;
        ditherIntensity            = .1f;
        distortionIntensity        = 0.0f;
        vignetteBreatheIntensity   = 0.0f;
        fisheyeIntensity           = 0.0f;
        fleshLUTIntensity          = 0.0f;
        ghostTrailIntensity        = 0.0f;
        ghostTrailSpeed            = 1.0f;
    }
};

typedef spine::Vector<unsigned short>   SpineIndices;
typedef spine::Vector<float>            SpineVertices;
class CGfx : public IEventListener
{
    friend class CSceneResize;

private:
    static CGfx*                    _instance;

    std::map<CTexture*, GLuint>     _rttFBOs;
    GLint                           _rttPrevFBO = 0;
    GLint                           _rttPrevViewport[4] = {0, 0, 0, 0};

    Shockwave                       _wave;
    Point                           _ptCameraPos;
    CMatrixStack                    _matStack;
    bool                            _isSetupDone;

    GLuint                          _vertexBuff;
    GLuint                          _indexBuff;
    GLuint                          _progId;
    shader_attribs_loc_t            _attribsLocations;

    void*                           _pVerBuffMem;
    size_t                          _nBuffMemSz;

    void*                           _pIdxBuffMem;
    size_t                          _nIdxBuffMemSz;

    size_t                          _verteciesInMemBuff;
    size_t                          _vertexSize;
    size_t                          _vertexOffset;
    size_t                          _indexOffset;

    CTextures                       _tempTextures;
    CTextures                       _activeTextures;
    MapTextures                     _all;
    MapTextures                     _unloaded;
    SetTextures                     _toUnload;
    SetTextures                     _saved;

    SimpleCallback                  _cbOnRenderEnded;
    SimpleCallback                  _onWorldDarkenCb;

    size_t                          _nLastActiveTexture;
    GLint                           _samplersUniforms[SAMPLERS_COUNT];

    GLint                           _shockTimeUniform;
    GLint                           _shockCenterUniform;
    GLint                           _lowHPEffectUniform;
    GLint                           _timeUniform;
    GLint                           _tutParamsUniform;
    GLint                           _resolutionUniform;
    GLint                           _tutRectSizeUniform;

    HitEffect                       _hits[5];
    GLint                           _uHitPos[5];
    GLint                           _uHitIntensity[5];
    GLint                           _uHitColor;

    GLuint                          _ppVAO = 0;

    int                             _numIndicesToDraw;
    int                             _drawCalls;

    CContainerPtr                   _gameRoot;
    CContainerPtr                   _gameIface;
    CContainerPtr                   _bgRoot;
    CContainerPtr                   _fgRoot;
    CContainerPtr                   _fgControlsRoot;

    bool                            _bScissorEnabled = false;
    Rect                            _rcScissor;
    bool                            _bIsScissorsRectEmpty = true;

    float                           _viewPortCx = 0;
    float                           _viewPortCy = 0;
    float                           _fOrigCx    = 1270.f;
    float                           _fOrigCy    = 714.f;

    bool                            _isWebGL2   = true;
    CameraInfo                      _camInfo;

    float                           _shakeIntensity = 0;
    float                           _zoomShakeIntensity = 0;
    float                           _currentHP = 100.f;

    float                           _tutorialBox_x01 = 0.f;
    float                           _tutorialBox_y01 = 0.f;
    float                           _tutorialBoxCorners = 0.f;
    float                           _tutorialBoxIntencity = 0.f;
    float                           _tutorialWait = 0.f;
    Rect                            _tutorialRc;
    bool                            _bIgnoreTutorialBox = false;

    eHitColor                       _eHitColor = eHitColor::NORMAL;
    CContainerPtr                   _ptrTutorElement;
    bool                            _isTutorActive = false;

    float                           _fTextEffectLifeTime = 0.f;
    float                           _fTextEffect = 0.f;

    inline static auto              _startTime = std::chrono::steady_clock::now();

    SimpleCallback                  _cbOnCameraAnimComplete = {};
    eRenderLayer                    _eCurrLayer = eRenderLayer::BACKGORUND;

    float                           _fDarkenWorldTimer = 0;
    eWorldDarkenState               _eDarkenState = eWorldDarkenState::BASIC;
    float                           _fMinEnvDarken = 0.f;
    float                           _fMinGameDarken = 0.f;

    int                             _gpuTier = 3;
    float                           _lastDt = 0;

    CTexturePtr                     _ptrTexGrain;

    // Post-process
    PostProcessSettings             _ppSettings;
    bool                            _ppInitialized = false;
    GLuint                          _ppFBO = 0;
    GLuint                          _ppTexture = 0;
    GLuint                          _ppProgId = 0;
    GLuint                          _ppVBO = 0;
    GLint                           _ppPrevFBO = 0;

    GLint                           _ppUSceneTex = -1;
    GLint                           _ppUGrainTex = -1;
    GLint                           _ppUResolution = -1;
    GLint                           _ppUTime = -1;

    GLint                           _ppUVignetteIntensity = -1;
    GLint                           _ppUVignetteRadius = -1;
    GLint                           _ppUVignetteSmoothness = -1;

    GLint                           _ppUTintColor = -1;
    GLint                           _ppUSaturation = -1;
    GLint                           _ppUContrast = -1;
    GLint                           _ppUBrightness = -1;
    GLint                           _ppUGrain = -1;
    GLint                           _ppUAberration = -1;
    GLint                           _ppUFisheyeIntensity = -1;
    GLint                           _ppUDistortionIntensity = -1;
    GLint                           _ppURainIntensity = -1;
    GLint                           _ppUBloodDrops = -1;
    GLint                           _ppAPos = -1;
    GLint                           _ppAUV = -1;
    
    float                           _ppWidth = 0;
    float                           _ppHeight = 0;

    // Bloom
    bool                            _bloomInitialized = false;
    GLuint                          _bloomFBO[2] = {0, 0};
    GLuint                          _bloomTexture[2] = {0, 0};
    float                           _bloomWidth = 0;
    float                           _bloomHeight = 0;
    GLuint                          _bloomProgId[2] = {0, 0};

    GLint                           _bloomUTex[2] = {-1, -1};
    GLint                           _bloomUResolution[2] = {-1, -1};
    GLint                           _bloomUThreshold = -1;

    GLint                           _bloomAPos[2] = {-1, -1};
    GLint                           _bloomAUV[2] = {-1, -1};

    GLint                           _ppUBloomTex = -1;
    GLint                           _ppUBloomIntensity = -1;

    CTexturePtr                     _pFlatNormalTex;

    // Light buffer
    bool                            _lightBufferInitialized = false;
    GLuint                          _lightFBO = 0;
    GLuint                          _lightTexture = 0;
    GLuint                          _lightVecTexture = 0;
    float                           _lightBufferWidth = 0;
    float                           _lightBufferHeight = 0;
    bool                            _lightBufferHalfFloat = true;

    GLuint                          _lightTex[2] = {0, 0};
    int                             _lightTexIdx = 0;
    GLint                           _uZLayer = -1;
    GLint                           _uItemCullingMask = -1; 
    GLuint                          _lightUBO = 0;
    GLint                           _lightBlockIndex = -1;

    GLuint                          _lightVAO = 0;
    GLuint                          _lightQuadVBO = 0;
    GLuint                          _lightInstanceVBO = 0;
    GLuint                          _lightProgId = 0;
    GLint                           _lightUResolution = -1;

    GLint                           _uLightBuffer = -1;
    GLint                           _uLightVecBuffer = -1;
    GLint                           _uAmbientLight = -1;
    GLint                           _uHasLights = -1;

    bool                            _specularEnabled = true;
    GLint                           _uSpecularEnabled = -1;

    float                           _lightIntensityMul = 1.0f;
    int                             _currentZLayer = 0;
    uint16_t                        _currentItemCullingMask = 0xFFFF;

    float                           _lastFinalZoom = 1.0f;
    float                           _lastOffsetGmX = 0.0f;
    float                           _lastOffsetGmY = 0.0f;

    GPULight                        _lights[MAX_LIGHTS];
    int                             _lightCount = 0;
    
    std::vector<int>                _reservedLightSamplers;
    float                           _timestepAccumulator = 0.f;
    float                           _renderAlpha         = 1.f;


private:
    bool                            isSamplerReserved                   (int                        idx) const;
    void                            reserveSampler                      (int                        idx);
    void                            unreserveSampler                    (int                        idx);
    void                            clearReservedSamplers               (void);
    bool                            hasFreeSamplerForLightTexture       (void);
    bool                            canReserveLightSampler();

    bool                            initLightBuffer                     (void);
    void                            resizeLightBuffer                   (float                      w, 
                                                                         float                      h);
    void                            destroyLightBuffer                  (void);
    void                            renderLightPass                     (float                      finalZoom, 
                                                                         float                      offsetGmX, 
                                                                         float                      offsetGmY);
    bool                            setupLightShaders                   (void);
    void                            lookupLightUniforms                 (void);

#ifdef TARGET_WIN
    void                            printProgramLog                     (GLuint                     program);
    void                            printShaderLog                      (GLuint                     shader);
#endif

    bool                            initOpenGL                          (void);
    bool                            setupShaders                        (void);
    bool                            setupAttributes                     (void);
    bool                            lookupUniforms                      (void);

    void                            allocVertBuffMem                    (void);
    void                            createTempTextures                  (void);
    void                            setActiveTexture                    (CTexture*                  ptr);
    void                            updateActiveTextures                (void);
    void                            updateViewPort                      (void);
    void                            setViewPort                         (float                      cx, 
                                                                         float                      cy);

    bool                            initPostProcess                     (void);
    void                            resizePostProcess                   (float                      w, 
                                                                         float                      h);
    void                            renderPostProcess                   (void);
    bool                            setupPostProcessShaders             (void);
    void                            lookupPostProcessUniforms           (void);

    bool                            initBloom                           (void);
    void                            destroyBloom                        (void);
    void                            resizeBloom                         (float                      w, 
                                                                         float                      h);
    void                            renderBloom                         (void);
    bool                            setupBloomShaders                   (void);
    void                            lookupBloomUniforms                 (void);

    void                            restoreVertexFormat                 (void);

    bool                            ensureSpace                         (size_t                     nVerts, 
                                                                         size_t                     nIndices);
    void                            appendQuadIndices                   (GLuint                     baseVertex, 
                                                                         bool                       particleTopology);
    void                            appendSequentialIndices             (GLuint                     baseVertex, 
                                                                         size_t                     count);
    void                            appendIndexArray                    (GLuint                     baseVertex,  
                                                                         SpineIndices*              indices);

public:
    bool                            flush                               (void);
    float                           getRenderAlpha                      (void) const { return _renderAlpha; }
public:
    CTexturePtr                     createRenderTexture                 (float                      w, 
                                                                         float                      h);
    void                            destroyRenderTexture                (CTexturePtr                tex);
    void                            setRenderTarget                     (CTexturePtr                tex);
    void                            restoreRenderTarget                 (void);
    void                            clearRenderTexture                  (CTexturePtr                tex, 
                                                                         float                      r = 0, 
                                                                         float                      g = 0, 
                                                                         float                      b = 0, 
                                                                         float                      a = 0);
    virtual void                    onEvent                             (int                        nEvent, 
                                                                         void*                      pData1 = nullptr, 
                                                                         void*                      pData2 = nullptr, 
                                                                         void*                      pData3 = nullptr) override;
    float                           getLastDt                           (void) { return _lastDt; }
    CTexturePtr                     getTextureById                      (LPCTSTR                    lpszTexId,
                                                                         bool                       bFullPath = false);
    void                            setOnRenderEnded                    (SimpleCallback cb) { assert(!_cbOnRenderEnded); _cbOnRenderEnded = cb; }

    CSpritePtr                      spriteFromTexture                   (LPCTSTR                    lpszTexName);
    CSpritePtr                      spriteFromTexture                   (CTexturePtr                ptrTex);

    bool                            batchSprite                         (CTexturePtr                pTex,
                                                                         float*                     verts,
                                                                         float*                     uvs,
                                                                         float*                     rgba,
                                                                         bool                       bGrayScale = false,
                                                                         float                      fIsNormalBlend = 1.f,
                                                                         float*                     addRGB = nullptr,
                                                                         float                      effectType = 0.f,
                                                                         float                      lifeTime = 0.f,
                                                                         std::optional<float>       optTutBoxIntencity = std::nullopt,
                                                                         CTexturePtr                pNormalMapTex = nullptr,
                                                                         bool                       skipLight = true);

    bool                            batchParticles                      (CTexturePtr                pTex,
                                                                         const ParticleQuadData*    quads,
                                                                         int                        numQuads,
                                                                         float                      isNormalBlend,
                                                                         bool                       skipLight = true);

    bool                            isPtOnScene                         (float                      fx, 
                                                                         float                      fy)
    {
        return _bScissorEnabled ? _rcScissor.contains(fx, fy)
                                : (fx >= 0 && fx < _viewPortCx && fy >= 0 && fy < _viewPortCy);
    }

    bool                            isRcOnScene                         (float                      left, 
                                                                         float                      top, 
                                                                         float                      right, 
                                                                         float                      bot)
    {
        return (_bScissorEnabled
                ? _rcScissor.checkIntersectByVals(left, top, right, bot,
                                                  _rcScissor.x, _rcScissor.y,
                                                  _rcScissor.right(), _rcScissor.bottom())
                : _rcScissor.checkIntersectByVals(left, top, right, bot,
                                                  0, 0, _viewPortCx, _viewPortCy));
    }

    void                            getViewportRect                     (Rect*                      pRcOut)  { pRcOut->set(0, 0, _viewPortCx, _viewPortCy); }
    bool                            preBatchVert                        (size_t                     nVertCount = 6);

    bool                            batchFontStashVerts                 (CTexturePtr                pTex,
                                                                         const float*               verts,
                                                                         const float*               tcoords,
                                                                         const unsigned int*        colors,
                                                                         int                        nverts,
                                                                         bool                       skipLight = true);

    void                            animateWorldDarken                  (float                      dt);

    bool                            batchRawInterleaved                 (CTexture*                  tex,
                                                                         const float*               data,
                                                                         int                        vertexCount,
                                                                         float                      r, 
                                                                         float                      g, 
                                                                         float                      b, 
                                                                         float                      a,
                                                                        CTexturePtr                 pNormalMapTex = nullptr,
                                                                         bool                       skipLight = true,
                                                                         float                      reflFadeStartY = 0,
                                                                         float                      reflFadeEndY = 0);

    bool                            batchSpineVect                      (CTexture*                  pTex,
                                                                         SpineVertices*             verts,
                                                                         SpineVertices*             uvs,
                                                                         SpineIndices*              indices,
                                                                         float                      r, 
                                                                         float                      g, 
                                                                         float                      b, 
                                                                         float                      a,
                                                                         float                      gsr, 
                                                                         float                      gsg, 
                                                                         float                      gsb, 
                                                                         float                      gsa,
                                                                         float                      fIsBlendNormal,
                                                                         float                      fEffectType,
                                                                         float                      fEffectLifeTime,
                                                                         CTexturePtr                pNormalMapTex,
                                                                         bool                       skipLight,
                                                                         float                      reflFadeStartY,
                                                                         float                      reflFadeEndY);

    bool                            batchVert                           (CTexture*                  pTex,
                                                                         float                      x, 
                                                                         float                      y, 
                                                                         float                      u, 
                                                                         float                      v,
                                                                         float                      r, 
                                                                         float                      g, 
                                                                         float                      b, 
                                                                         float                      a,
                                                                         float                      gsr, 
                                                                         float                      gsg, 
                                                                         float                      gsb, 
                                                                         float                      gsa,
                                                                         float                      isNormalBlendMode = 1.f,
                                                                         CTexturePtr                pNormalMapTex = nullptr,
                                                                         bool                       skipLight = true);

    GLuint                          allocAndBindGPUTexture              (void);
    void                            deleteTexture                       (CTexturePtr                ptrTex);

public:
    GLuint _vao = 0;

public:
    size_t                          getMaxSamplers                      (void) { return SAMPLERS_COUNT; }

                                    CGfx                                (void);
    static CGfx*                    getInstance                         (void);
    bool                            setup                               (void);
    static CameraInfo&              calculateCamera                     (float                      targetX, 
                                                                         float                      targetY, 
                                                                         float                      desiredZoom);
    float                           getElapsedSeconds                   (void);
    void                            setTutorialElips                    (CContainerPtr              ptrVisibleElement);
    float                           getLoopedTime                       (void);
    void                            setShockTimeUniform                 (float                      fValue);
    void                            startHit                            (float                      x, 
                                                                         float                      y);
    void                            update                              (float                      dt);
    void                            initGameMatrix                      (void);
    void                            renderGraph                         (float                      dt);
    void                            begin                               (float                      dt);
    void                            end                                 (void);
    CTexturePtr                     uploadAsset                         (LPCTSTR                    lpszFileName, 
                                                                         const uint8_t*             pBytes, 
                                                                         size_t                     nSize);
    CTexturePtr                     uploadAsset                         (LPCTSTR                    lpszTexId,
                                                                         bool                       bFullPath = false);
    void                            uploadGrainTexture                  (void);
    void                            uploadAssets                        (LPCTSTR                    lpszTexId, 
                                                                         int                        numOfPngs = 1);

    void*                           decodeImage                         (LPCTSTR                    lpszTexId, 
                                                                         int                        bytesSize, 
                                                                         const void*                imgBytes, 
                                                                         int&                       cx, 
                                                                         int&                       cy);
    void                            freeDecodedImage                    (void*                      decodedBytes);

    void                            unloadAsset                         (LPCTSTR                    lpszTexId, 
                                                                         int                        numOfPngs = 1);
    void                            saveLoadedTextures                  (void);
    void                            freeAllNotSavedTextures             (void);
    void                            unloadTexture                       (CTexturePtr                ptr);
    void                            unloadTexture                       (LPCTSTR                    lpszTexId);

    CTexturePtr                     uploadFontStahTexture               (int                        cx, 
                                                                         int                        cy);
    void                            updateFontStashTexture              (CTexturePtr                ptrTex, 
                                                                         int*                       rect, 
                                                                         const unsigned char*       data);

    void                            triggerShockWave                    (float                      x, 
                                                                         float                      y);
    bool                            updateUniforms                      (void);

    void                            enableScissor                       (bool                       bEnabled);
    void                            setScissor                          (Rect*                      pScissor);
    bool                            getScissors                         (Rect*                      rcOut);

    float                           getCurrLayerOffsetX                 (void);
    float                           getCurrLayerOffsetY                 (void);
    float                           getLayerOffsetX                     (eRenderLayer               eLayer);
    float                           getLayerOffsetY                     (eRenderLayer               eLayer);

    void                            resetCamera                         (void);
    auto&                           getCameraInfo                       (void) { return _camInfo; }
    void                            setCenterCamera                     (Point                      ptFocus, 
                                                                         float                      fZoom);

    void                            setTextEffect                       (float                      fEffect = 0.f, 
                                                                         float                      fLifeTime = 0.f)
    {
        _fTextEffect = fEffect;
        _fTextEffectLifeTime = fLifeTime;
    }

    void                            setIgnoreTutorialBox                (bool                       bIgnore) { _bIgnoreTutorialBox = bIgnore; }

    void                            animateCenterCamera                 (CContainerPtr              ptrCont, 
                                                                         float                      zoom, 
                                                                         float                      fDuring, 
                                                                         SimpleCallback             cbOnComplete);
    void                            animateCenterCamera                 (Point                      ptFocus, 
                                                                         float                      zoom, 
                                                                         float                      fDuring, 
                                                                         SimpleCallback             cbOnComplete);
    void                            animateResetCamera                  (float                      fDuring, 
                                                                         SimpleCallback             cbOnComplete);

    void                            setWorldDarken                      (bool                       bDarken = true, 
                                                                         float                      fMinEnvDarken = 0.4f, 
                                                                         float                      fMinGameDarken = 0.2f, 
                                                                         SimpleCallback             cb = {});
    void                            setWorldDarkenNoAnim                (bool                       bDarken = true, 
                                                                         float                      fMinEnvDarken = 0.4f, 
                                                                         float                      fMinGameDarken = 0.2f);

    void                            setHitColor                         (eHitColor                  eColor) { _eHitColor = eColor; }

    bool                            isPotato                            (void) { return _gpuTier <= 1; }
    auto                            getTier                             (void) { return _gpuTier; }

    void                            startShake                          (float                      force) { _shakeIntensity = force; }
    void                            startZoomShake                      (float                      force) { _zoomShakeIntensity = force; }

    void                            setCurrentHP                        (float                      fHP) { _currentHP = std::min(_currentHP, fHP); }
    void                            resetCurrentHP                      (void) { _currentHP = 100; }

    CContainer*                     getGameRoot                         (void) { return _gameRoot.get(); }
    CContainer*                     getGameIface                        (void) { return _gameIface.get(); }
    CContainer*                     getBgRoot                           (void) { return _bgRoot.get(); }
    CContainer*                     getFgRoot                           (void) { return _fgRoot.get(); }
    CContainer*                     getFgControlsRoot                   (void) { return _fgControlsRoot.get(); }
    CMatrixStack*                   getMatrixStack                      (void) { return &(_matStack); }
    bool                            isBloomAvailable                    (void) { return _gpuTier > 1; }
    bool                            isLightAvailable                    (void) { return _gpuTier > 0; }
    bool                            isPostProcessAvailable              (void) { return _gpuTier > 0; }
    bool                            isPostProcessEnebled                (void) { return isPostProcessAvailable() ? _ppSettings.enabled : false; }
    void                            getOrigViewPort                     (float&                     fcx, 
                                                                         float&                     fcy)
    {
        fcx = _fOrigCx;
        fcy = _fOrigCy;
    }

    void                            getScaleFactor                      (float&                     fOutScaleX, 
                                                                         float&                     fOutScaleY)
    {
        fOutScaleX = _viewPortCx / _fOrigCx;
        fOutScaleY = _viewPortCy / _fOrigCy;
    }

    // Post-process API
    void                            setPostProcessEnabled               (bool                       b) { _ppSettings.enabled = b; }
    void                            setPostProcessSettings              (const PostProcessSettings& s) { _ppSettings = s; }
    PostProcessSettings&            getPostProcessSettings              (void) { return _ppSettings; }
    bool                            isPostProcessEnabled                (void) const { return _ppSettings.enabled && _ppInitialized; }

    // Lighting API
    void                            appendLights                        (std::span<GPULight>        lights);
    void                            clearLights                         (void);

    void                            setSpecularEnabled                  (bool                       bEnabled) { _specularEnabled = bEnabled; }
    bool                            isSpecularEnabled                   (void) const { return _specularEnabled; }

    float                           setLightIntensityMul                (float                      intensity)
    {
        if (intensity < 0.0f) intensity = 0.0f;
        else if (intensity > 1.0f) intensity = 1.0f;
        std::swap(_lightIntensityMul, intensity);
        return intensity;
    }

    float                           getLightIntensity                   (void) const
    {
        return _lightIntensityMul;
    }

    void                            setCurrentZLayer                    (int                        z, 
                                                                         uint16_t                   itemCullingMask = 0xFFFF);
    void                            setCurrentItemCullingMask           (uint16_t                   mask);

    int                             getCurrentZLayer                    (void) const { return _currentZLayer; }
    uint16_t                        getCurrentItemCullingMask           (void) const { return _currentItemCullingMask; }
    float                           getDpr                              (void) { return CSceneResize::getInstance()->get_dpr(); }
};

inline bool CGfx::preBatchVert(size_t nVertCount)
{
    if (_vertexOffset * sizeof(float) + (VERTEX_SIZE * nVertCount) > _nBuffMemSz ||
        _indexOffset * sizeof(uint32_t) + (nVertCount * sizeof(uint32_t)) > _nIdxBuffMemSz)
    {
        flush();
    }

    return true;
}


_G2D_NAMESPACE_END_