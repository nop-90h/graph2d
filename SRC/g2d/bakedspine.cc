#include "bakedspine.h"
#include "gfx.h"
#include "spinepacked.h"

_G2D_NAMESPACE_BEGIN_

CBakedSpine::CBakedSpine(std::span<const char*> filenames,
                         std::span<const char*> animNames,
                         CTexturePtr tex,
                         bool isLooped)
{
    assert(filenames.size() == animNames.size());
    assert(!filenames.empty());

    _vertsDrawer.resize(filenames.size());

    for (size_t i = 0; i < filenames.size(); ++i)
    {
        bool bOk = _vertsDrawer[i].loadAnimation(filenames[i],
                                                 animNames[i],
                                                 isLooped,
                                                 tex);

        assert(bOk && "Failed to load packed animation");
        (void)bOk;
    }

    _nCurrentAnimIdx = 0;
    _vertsDrawer[0].update(0.0f);
}

void CBakedSpine::update(float dt)
{
    if (_nCurrentAnimIdx > -1)
        _vertsDrawer[_nCurrentAnimIdx].update(dt);

    CContainer::update(dt);
}

void CBakedSpine::renderSelf(float dt, float* rgba)
{
    (void)dt;

    static float my_rgba[4];

    my_rgba[0] = rgba[0] * _RGBA[0];
    my_rgba[1] = rgba[1] * _RGBA[1];
    my_rgba[2] = rgba[2] * _RGBA[2];
    my_rgba[3] = rgba[3] * _RGBA[3];

    if (_nCurrentAnimIdx > -1)
    {
        _vertsDrawer[_nCurrentAnimIdx].renderVerts(my_rgba[0],
                                                   my_rgba[1],
                                                   my_rgba[2],
                                                   my_rgba[3]);
    }
}

bool CBakedSpine::setAnimationByName(const char* lpccAnimName,
                                     bool bLoop,
                                     SimpleCallback cbOnComplete)
{
    int targetIdx = -1;

    for (int i = 0; i < (int)_vertsDrawer.size(); i++)
    {
        if (_vertsDrawer[i].getName() == lpccAnimName)
        {
            targetIdx = i;
            break;
        }
    }

    if (targetIdx == -1)
    {
        assert(false && "Animation name not found!");
        return false;
    }

    _nCurrentAnimIdx = targetIdx;

    _vertsDrawer[_nCurrentAnimIdx].reset();
    _vertsDrawer[_nCurrentAnimIdx].setLooped(bLoop);
    _vertsDrawer[_nCurrentAnimIdx].setOnComplete(cbOnComplete);
    _vertsDrawer[_nCurrentAnimIdx].update(0.0f);

    return true;
}

bool SpineVertsDrawable::loadAnimation(const char* filename,
                                       const char* animName,
                                       bool isLooped,
                                       CTexturePtr ptrTex,
                                       SimpleCallback cbOnComplete,
                                       float fpsOverride,
                                       float fTimeScale,
                                       bool ignoreAdditiveBlend)
{
    return setAnimation(PackedAnimCache::get(filename), animName,
                        isLooped,
                        ptrTex,
                        cbOnComplete,
                        fpsOverride,
                        fTimeScale,
                        ignoreAdditiveBlend);
}

bool SpineVertsDrawable::setAnimation(PackedAnimPtr anim,
                                      const char* animName,
                                      bool isLooped,
                                      CTexturePtr ptrTex,
                                      SimpleCallback cbOnComplete,
                                      float fpsOverride,
                                      float fTimeScale,
                                      bool ignoreAdditiveBlend)
{
    if (!anim || anim->frameCount() == 0)
        return false;

    _anim = anim;

    _name = animName ? animName : "";
    _isLooped = isLooped;
    _tex = ptrTex;
    _onComplete = cbOnComplete;

    float fps = fpsOverride;
    if (fps <= 0.0f)
        fps = _anim->fps();

    if (fps <= 0.0f)
        fps = 30.0f;

    _targetFps = 1.0f / fps;
    _fTimeScale = fTimeScale;
    _ignoreAdditiveBlend = ignoreAdditiveBlend;

    _isInterpolated = true;

    reset();

    return true;
}

void SpineVertsDrawable::update(float dt)
{
    if (!_anim || _anim->frameCount() == 0)
        return;

    if (_isComplete && !_isLooped)
    {
        _interpolationFactor = 0.0f;
        _nextFrame = _frame;
        return;
    }

    if (_targetFps <= 0.0f)
        return;

    _frameTimer += dt * _fTimeScale;

    while (_frameTimer >= _targetFps)
    {
        _frameTimer -= _targetFps;

        uint32_t prevFrame = _frame;

        if (_frame + 1 < _anim->frameCount())
        {
            ++_frame;
        }
        else
        {
            if (!_isLooped)
            {
                _isComplete = true;
                _frame = prevFrame;

                if (_onComplete)
                    _onComplete();

                _frameTimer = 0.0f;
                break;
            }
            else
            {
                if (_onComplete)
                    _onComplete();

                _frame = 0;
            }
        }
    }

    if (_isComplete && !_isLooped)
    {
        _interpolationFactor = 0.0f;
        _nextFrame = _frame;
    }
    else
    {
        if (_targetFps > 0.0f)
            _interpolationFactor = _frameTimer / _targetFps;
        else
            _interpolationFactor = 0.0f;

        if (_frame + 1 < _anim->frameCount())
        {
            _nextFrame = _frame + 1;
        }
        else
        {
            _nextFrame = _isLooped ? 0 : _frame;
        }
    }
}

static bool packedBatchValid(const PackedBatchHeader& bh)
{
    if (bh.mode == 0)
    {
        return bh.vertCount > 0 &&
               bh.vertCount <= PACKED_MAX_BATCH_VERTS;
    }

    if (bh.mode == 1)
    {
        return bh.vertCount > 0 &&
               bh.indexCount > 0 &&
               bh.vertCount <= PACKED_MAX_UNIQUE_VERTS &&
               bh.indexCount <= PACKED_MAX_BATCH_VERTS;
    }

    return false;
}

static uint32_t packedPaddedIndexBytes(uint32_t indexCount)
{
    const uint32_t idxBytes = indexCount * sizeof(uint16_t);
    return (idxBytes + 3u) & ~3u;
}

static uint32_t packedBatchPayloadBytes(const PackedBatchHeader& bh)
{
    if (bh.mode == 0)
    {
        return (uint32_t)(bh.vertCount * sizeof(PackedPosUV));
    }

    return (uint32_t)(bh.vertCount * sizeof(PackedPosUV)) +
           packedPaddedIndexBytes(bh.indexCount);
}

static bool packedFramesCompatible(const uint8_t* c,
                                   const uint8_t* n,
                                   const PackedFrameHeader& fc,
                                   const PackedFrameHeader& fn)
{
    if (fc.batchCount != fn.batchCount)
        return false;

    for (uint32_t i = 0; i < fc.batchCount; ++i)
    {
        PackedBatchHeader bc{};
        PackedBatchHeader bn{};

        memcpy(&bc, c, sizeof(bc));
        c += sizeof(bc);

        memcpy(&bn, n, sizeof(bn));
        n += sizeof(bn);

        if (!packedBatchValid(bc) || !packedBatchValid(bn))
            return false;

        if (bc.mode != bn.mode)
            return false;

        if (bc.blend != bn.blend)
            return false;

        if (bc.mode == 0)
        {
            if (bc.vertCount != bn.vertCount)
                return false;
        }
        else if (bc.mode == 1)
        {
            if (bc.vertCount != bn.vertCount)
                return false;

            if (bc.indexCount != bn.indexCount)
                return false;

            if (bc.indexCount > 0)
            {
                const uint8_t* idxC =
                    c + (bc.vertCount * sizeof(PackedPosUV));

                const uint8_t* idxN =
                    n + (bn.vertCount * sizeof(PackedPosUV));

                if (memcmp(idxC,
                           idxN,
                           bc.indexCount * sizeof(uint16_t)) != 0)
                {
                    return false;
                }
            }
        }
        else
        {
            return false;
        }

        c += packedBatchPayloadBytes(bc);
        n += packedBatchPayloadBytes(bn);
    }

    return true;
}

void SpineVertsDrawable::renderVerts(float r, float g, float b, float a)
{
    if (!_anim ||
        _anim->frameCount() == 0 ||
        _frame >= _anim->frameCount())
    {
        return;
    }

    const uint8_t* curBase = _anim->framePtr(_frame);
    if (!curBase)
        return;

    PackedFrameHeader fhCur{};
    memcpy(&fhCur, curBase, sizeof(fhCur));

    const uint8_t* curData = curBase + sizeof(fhCur);

    bool interpolate =
        _isInterpolated &&
        _interpolationFactor > 0.001f &&
        _nextFrame != _frame &&
        _nextFrame < _anim->frameCount();

    const uint8_t* nextData = nullptr;

    if (interpolate)
    {
        const uint8_t* nextBase = _anim->framePtr(_nextFrame);

        if (!nextBase)
        {
            interpolate = false;
        }
        else
        {
            PackedFrameHeader fhNext{};
            memcpy(&fhNext, nextBase, sizeof(fhNext));

            nextData = nextBase + sizeof(fhNext);

            interpolate = packedFramesCompatible(curData,
                                                 nextData,
                                                 fhCur,
                                                 fhNext);
        }
    }

    static PackedPosUV s_unique[PACKED_MAX_UNIQUE_VERTS];
    static float s_curr[PACKED_MAX_BATCH_VERTS * 6];
    static float s_next[PACKED_MAX_BATCH_VERTS * 6];

    const float sx = _anim->scaleX();
    const float sy = _anim->scaleY();

    auto decodeVertex = [&](float* dst,
                            const PackedPosUV& v,
                            uint8_t blend,
                            uint8_t alpha)
    {
        dst[0] = ((float)v.x / 32767.0f) * sx;
        dst[1] = ((float)v.y / 32767.0f) * sy;
        dst[2] = ((float)v.u / 65535.0f);
        dst[3] = ((float)v.v / 65535.0f);

        float blendF = (float)blend;
        if (_ignoreAdditiveBlend && blend == 0)
            blendF = 1.0f;

        dst[4] = blendF;
        dst[5] = ((float)alpha) / 255.0f;
    };

    auto decodeBatch = [&](const uint8_t*& p,
                           float* out,
                           uint32_t& outCount,
                           PackedBatchHeader& bh) -> bool
    {
        memcpy(&bh, p, sizeof(bh));
        p += sizeof(bh);

        if (!packedBatchValid(bh))
            return false;

        if (bh.mode == 0)
        {
            const uint32_t vertCount = bh.vertCount;

            if (vertCount == 0 || vertCount > PACKED_MAX_BATCH_VERTS)
                return false;

            outCount = vertCount;

            for (uint32_t i = 0; i < vertCount; ++i)
            {
                PackedPosUV v{};
                memcpy(&v, p, sizeof(v));
                p += sizeof(v);

                decodeVertex(&out[i * 6], v, bh.blend, bh.alpha);
            }

            return true;
        }

        if (bh.mode == 1)
        {
            const uint32_t uniqueCount = bh.vertCount;
            const uint32_t indexCount = bh.indexCount;

            if (uniqueCount == 0 ||
                indexCount == 0 ||
                uniqueCount > PACKED_MAX_UNIQUE_VERTS ||
                indexCount > PACKED_MAX_BATCH_VERTS)
            {
                return false;
            }

            for (uint32_t i = 0; i < uniqueCount; ++i)
            {
                memcpy(&s_unique[i], p, sizeof(PackedPosUV));
                p += sizeof(PackedPosUV);
            }

            const uint8_t* idxBase = p;

            const uint32_t idxBytes = indexCount * sizeof(uint16_t);
            const uint32_t idxBytesPadded = (idxBytes + 3u) & ~3u;

            p += idxBytesPadded;

            outCount = indexCount;

            for (uint32_t i = 0; i < indexCount; ++i)
            {
                uint16_t idx = 0;
                memcpy(&idx, idxBase + i * sizeof(uint16_t), sizeof(idx));

                if (idx >= uniqueCount)
                    return false;

                decodeVertex(&out[i * 6],
                             s_unique[idx],
                             bh.blend,
                             bh.alpha);
            }

            return true;
        }

        return false;
    };

    float ndcX = CGfx::getInstance()->getMatrixStack()->top()[6];
    float ndcY = CGfx::getInstance()->getMatrixStack()->top()[7];
    float m3 = CGfx::getInstance()->getMatrixStack()->top()[3];
    float m4 = CGfx::getInstance()->getMatrixStack()->top()[4];

    auto screenWidth = CSceneResize::getInstance()->getGameWidth();
    auto screenHeight = CSceneResize::getInstance()->getScreenHeight();

    float totalYLength = std::sqrt(m3 * m3 + m4 * m4);
    float currentScaleY = totalYLength / (2.0f / screenHeight);

    float screenX = (ndcX + 1.0f) * 0.5f * screenWidth;
    float screenY = (1.0f - ndcY) * 0.5f * screenHeight;

    auto minReflY = screenY + 0.0f * currentScaleY;
    auto maxReflY = screenY + 150.0f * currentScaleY;

    float reflectedMinY = minReflY;
    float reflectedMaxY = maxReflY;

    float ndcMinReflY = 1.0f - (2.0f * reflectedMinY / screenHeight);
    float ndcMaxReflY = 1.0f - (2.0f * reflectedMaxY / screenHeight);

    reflectedMinY = ndcMinReflY;
    reflectedMaxY = ndcMaxReflY;

    const uint8_t* pCur = curData;
    const uint8_t* pNext = nextData;

    for (uint32_t bi = 0; bi < fhCur.batchCount; ++bi)
    {
        PackedBatchHeader bhCur{};
        uint32_t count = 0;

        if (!decodeBatch(pCur, s_curr, count, bhCur))
            break;

        if (count == 0)
            continue;

        if (interpolate)
        {
            PackedBatchHeader bhNext{};
            uint32_t countNext = 0;

            if (!decodeBatch(pNext, s_next, countNext, bhNext) ||
                countNext != count)
            {
                interpolate = false;
            }
            else
            {
                const float t = _interpolationFactor;

                for (uint32_t v = 0; v < count; ++v)
                {
                    const int idx = (int)(v * 6);

                    s_curr[idx + 0] +=
                        (s_next[idx + 0] - s_curr[idx + 0]) * t;

                    s_curr[idx + 1] +=
                        (s_next[idx + 1] - s_curr[idx + 1]) * t;

                    s_curr[idx + 5] +=
                        (s_next[idx + 5] - s_curr[idx + 5]) * t;
                }
            }
        }

        CGfx::getInstance()->preBatchVert((int)count);
        CGfx::getInstance()->batchRawInterleaved(_tex.get(),
                                                 s_curr,
                                                 (int)count,
                                                 r,
                                                 g,
                                                 b,
                                                 a,
                                                 _normalMap,
                                                 _bSkipLight,
                                                 reflectedMinY,
                                                 reflectedMaxY);
    }
}

_G2D_NAMESPACE_END_
