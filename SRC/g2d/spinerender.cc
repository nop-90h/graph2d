#include "spinerender.h"
#include "spinepacked.h"
#include "gfx.h"
#include "rsrcfile.h"
#include "m3.h"
#include "engine.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

namespace spine
{
using namespace _G2D_NS_;

SkeletonDrawable::SkeletonDrawable(LPCTSTR lpszSpinePath,
                                   SkeletonData* skeletonData,
                                   AnimationStateData* animationStateData)
{
    static MixEntries vMixEntries;

    _quadIndices.add(0);
    _quadIndices.add(1);
    _quadIndices.add(2);
    _quadIndices.add(2);
    _quadIndices.add(3);
    _quadIndices.add(0);

    Bone::setYDown(true);

    skeleton = new (__FILE__, __LINE__) Skeleton(skeletonData);

    ownsAnimationStateData = animationStateData == nullptr;

    if (ownsAnimationStateData)
        animationStateData = new (__FILE__, __LINE__) AnimationStateData(skeletonData);

    animationStateData->setDefaultMix(SpineMixingOptions::getDefaultMixTime());

    SpineMixingOptions::getMixEntries(lpszSpinePath, vMixEntries);

    for (auto& it : vMixEntries)
    {
        if (std::get<0>(it) == "def")
        {
            animationStateData->setDefaultMix(std::get<2>(it));
        }
        else
        {
            String sFromAnim(std::get<0>(it).c_str());
            String sToAnim(std::get<1>(it).c_str());

            animationStateData->setMix(sFromAnim, sToAnim, std::get<2>(it));
        }
    }

    animationState = new (__FILE__, __LINE__) AnimationState(animationStateData);
}

SkeletonDrawable::~SkeletonDrawable()
{
    if (ownsAnimationStateData)
        delete animationState->getData();

    delete animationState;
    delete skeleton;
}

void SkeletonDrawable::updateTweenAnims(float dt)
{
    for (int i = 0; i < SIZE_OF(_ghosts); i++)
        _ghosts[i].update(dt);
}

void SkeletonDrawable::update(float delta, Physics physics)
{
    updateTweenAnims(delta);

    animationState->update(delta);
    animationState->apply(*skeleton);

    skeleton->update(delta);
    skeleton->updateWorldTransform(physics);
}

SkeletonData* SkeletonDrawable::createSpineSkeletonData(const char* lpccSpineName)
{
    SkeletonData* pRes = nullptr;

    spine::String strAtlas(lpccSpineName);
    spine::String strSkel(lpccSpineName);

    strAtlas.append(".atlas");
    strSkel.append(".json");

    Atlas* atlas = new Atlas(strAtlas, GfxTextureLoader::getInstance());

    if (atlas->getPages().size() == 0)
    {
        assert(false);
        delete atlas;
        return nullptr;
    }

    SkeletonJson json(atlas);
    pRes = json.readSkeletonDataFile(strSkel);

    if (!pRes)
    {
        assert(false);
        delete atlas;
        return nullptr;
    }

    AnimationStateData* animationStateData = new AnimationStateData(pRes);

    return pRes;
}

bool SkeletonDrawable::getRenderedRect(Rect* pOut)
{
    pOut->set(_left, _top, _right - _left, _bottom - _top);
    return true;
}

bool SkeletonDrawable::hasAnimation(LPCTSTR lpszAnimName)
{
    return animationState->getData()->getSkeletonData()->findAnimation(lpszAnimName) != nullptr;
}

spine::TweenAnim* spine::SkeletonDrawable::addGhostAnim()
{
    TweenAnim* pRes = nullptr;

    for (int i = 0; i < SIZE_OF(_ghosts); i++)
    {
        if (!_ghosts[i].isActive())
        {
            _ghosts[i].init(eSpineGhostAnimState::MOVING_Y);
            pRes = &(_ghosts[i].tw);
            break;
        }
    }

    return pRes;
}

void spine::SkeletonDrawable::addGhostAnim(float nGhostX)
{
    bool bAnyInactive = false;

    for (int i = 0; i < SIZE_OF(_ghosts); i++)
    {
        if (!_ghosts[i].isActive())
        {
            bAnyInactive = true;

            _ghosts[i].init();

            _ghosts[i].tw.start(0.0f, nGhostX, 0.5f, 0.0f, Easing::outExpo,
                [this, i]
                {
                    _ghosts[i].tw.start(1.0f, 0.0f, 0.15f, 0.0f, Easing::linear, {});
                });

            break;
        }
    }

    assert(bAnyInactive);
}

void SkeletonDrawable::polygonToRect(Polygon* pPoly, Rect* pRcOut)
{
    Vec4 vecBoxBar(0xffff, 0xffff, -0xffff, -0xffff);

    for (size_t j = 0; j < pPoly->_vertices.size() / 2; j++)
    {
        float x = pPoly->_vertices[j * 2];
        float y = pPoly->_vertices[(j * 2) + 1];

        if (x < vecBoxBar.x1)
            vecBoxBar.x1 = x;

        if (x > vecBoxBar.x2)
            vecBoxBar.x2 = x;

        if (y < vecBoxBar.y1)
            vecBoxBar.y1 = y;

        if (y > vecBoxBar.y2)
            vecBoxBar.y2 = y;
    }

    pRcOut->set(vecBoxBar.x1,
                vecBoxBar.y1,
                vecBoxBar.x2 - vecBoxBar.x1,
                vecBoxBar.y2 - vecBoxBar.y1);
}

void SkeletonDrawable::getBBIndexes(SkeletonBounds& sb)
{
    assert(!_bIndexesGot);

    _bIndexesGot = true;

    auto& cfg = Engine::getCfg();

    for (size_t i = 0; i < sb.getBoundingBoxes().size(); i++)
    {
        const String& bbName = sb.getBoundingBoxes()[i]->getName();

        if (bbName == cfg.SPINE_INTERACTIVE_BOUNDING_BOX_NAME.c_str())
        {
            assert(_nBB_BodyIdx == -1);

            _nBB_BodyIdx = i;
            _bHasInteractiveBox = true;
        }
    }
}

bool SkeletonDrawable::getBoundingBoxRect(LPCTSTR lpszBBname, Point ptCenter, Rect& rcOut)
{
    bool bRes = false;

    SkeletonBounds sb;
    sb.update(*skeleton, false);

    for (size_t i = 0; i < sb.getBoundingBoxes().size(); i++)
    {
        if (sb.getBoundingBoxes()[i]->getName() == lpszBBname)
        {
            bRes = true;

            Polygon* pBoxChar = sb.getPolygon(sb.getBoundingBoxes()[i]);

            polygonToRect(pBoxChar, &rcOut);
            rcOut.offset(ptCenter.x, ptCenter.y);

            break;
        }
    }

    return bRes;
}

LPCTSTR SkeletonDrawable::getBoundingBoxAtPoint(Point ptCenter, Point pt)
{
    LPCTSTR lpszRes = nullptr;

    SkeletonBounds sb;
    sb.update(*skeleton, false);

    for (size_t i = 0; i < sb.getBoundingBoxes().size(); i++)
    {
        Rect rc;

        Polygon* pBoxChar = sb.getPolygon(sb.getBoundingBoxes()[i]);

        polygonToRect(pBoxChar, &rc);
        rc.offset(ptCenter.x, ptCenter.y);

        if (rc.contains(pt.x, pt.y))
        {
            const String& bbName = sb.getBoundingBoxes()[i]->getName();
            lpszRes = bbName.buffer();
            break;
        }
    }

    return lpszRes;
}

void SkeletonDrawable::updateBoundingBoxes()
{
    SkeletonBounds sb;
    sb.update(*skeleton, false);

    if (!_bIndexesGot)
    {
        getBBIndexes(sb);
    }

    if (_nBB_BodyIdx > -1)
    {
        assert(sb.getBoundingBoxes().size() > (size_t)_nBB_BodyIdx);

        Polygon* pBoxChar = sb.getPolygon(sb.getBoundingBoxes()[_nBB_BodyIdx]);

        polygonToRect(pBoxChar, &_rcInteractiveBox);
    }
}

bool SkeletonDrawable::recordAnimation(const char* outFilename,
                                       const char* animationName,
                                       float fps,
                                       bool bSkipAlpha)
{
    if (!outFilename || !outFilename[0] || fps <= 0.0f)
        return false;

    spine::Animation* anim = skeleton->getData()->findAnimation(animationName);
    if (!anim)
        return false;

    const float step = 1.0f / fps;
    const float duration = anim->getDuration();

    auto startPass = [&]()
    {
        animationState->clearTracks();
        animationState->setAnimation(0, anim, false);
        animationState->setListener((AnimationStateListenerObject*)nullptr);
        skeleton->setToSetupPose();
    };

    auto blendToUint = [](spine::BlendMode mode) -> uint8_t
    {
        switch (mode)
        {
            case spine::BlendMode_Additive:
                return 0;

            case spine::BlendMode_Multiply:
                return 2;

            default:
                return 1;
        }
    };

    float mx = 0.01f;
    float my = 0.01f;

    auto processFrame = [&](PackedAnimWriter* writer)
    {
        SkeletonClipping& clipper = _clipping;
        clipper.clipEnd();

        for (unsigned i = 0; i < skeleton->getSlots().size(); ++i)
        {
            Slot& slot = *skeleton->getDrawOrder()[i];
            Attachment* attachment = slot.getAttachment();

            if (!attachment)
            {
                clipper.clipEnd(slot);
                continue;
            }

            if ((slot.getColor().a == 0 || !slot.getBone().isActive()) &&
                !attachment->getRTTI().isExactly(ClippingAttachment::rtti))
            {
                clipper.clipEnd(slot);
                continue;
            }

            float slotAlpha = bSkipAlpha ? 1.0f : slot.getColor().a;
            slotAlpha = std::clamp(slotAlpha, 0.0f, 1.0f);

            uint8_t blend8 = blendToUint(slot.getData().getBlendMode());

            spine::Vector<float>* finalVerts = &_worldVertices;
            spine::Vector<float>* finalUVs = nullptr;
            spine::Vector<unsigned short>* finalIndices = nullptr;
            Color* attachmentColor = nullptr;

            if (attachment->getRTTI().isExactly(RegionAttachment::rtti))
            {
                RegionAttachment* region = (RegionAttachment*)attachment;

                attachmentColor = &region->getColor();
                if (attachmentColor->a == 0)
                {
                    clipper.clipEnd(slot);
                    continue;
                }

                _worldVertices.setSize(8, 0);
                region->computeWorldVertices(slot, _worldVertices, 0, 2);

                finalUVs = &region->getUVs();
                finalIndices = &_quadIndices;
            }
            else if (attachment->getRTTI().isExactly(MeshAttachment::rtti))
            {
                MeshAttachment* mesh = (MeshAttachment*)attachment;

                attachmentColor = &mesh->getColor();
                if (attachmentColor->a == 0)
                {
                    clipper.clipEnd(slot);
                    continue;
                }

                _worldVertices.setSize(mesh->getWorldVerticesLength(), 0);
                mesh->computeWorldVertices(slot,
                                           0,
                                           mesh->getWorldVerticesLength(),
                                           _worldVertices.buffer(),
                                           0,
                                           2);

                finalUVs = &mesh->getUVs();
                finalIndices = &mesh->getTriangles();

                assert(finalUVs->size() == finalVerts->size() &&
                       "UV/Verts size mismatch in mesh!");
            }
            else if (attachment->getRTTI().isExactly(ClippingAttachment::rtti))
            {
                clipper.clipStart(slot, (ClippingAttachment*)attachment);
                continue;
            }
            else
            {
                clipper.clipEnd(slot);
                continue;
            }

            if (clipper.isClipping())
            {
                clipper.clipTriangles(*finalVerts, *finalIndices, *finalUVs, 2);

                finalVerts = &clipper.getClippedVertices();
                finalUVs = &clipper.getClippedUVs();
                finalIndices = &clipper.getClippedTriangles();

                assert(finalUVs->size() == finalVerts->size() &&
                       "UV/Verts size mismatch after clipping!");
            }

            if (finalIndices && finalIndices->size() > 0)
            {
                assert(finalIndices->size() % 3 == 0 &&
                       "Packed record expects triangle list!");

                if (!writer)
                {
                    for (int ii = 0; ii < (int)finalIndices->size(); ii++)
                    {
                        unsigned short idx = ((*finalIndices)[ii]);
                        int xIdx = idx * 2;

                        float vx = (*finalVerts)[xIdx];
                        float vy = (*finalVerts)[xIdx + 1];

                        mx = std::max(mx, std::abs(vx));
                        my = std::max(my, std::abs(vy));
                    }
                }
                else
                {
                    uint8_t alpha8 = writer->quantAlpha(slotAlpha);

                    PackedPosUV tri[3];
                    int triCount = 0;

                    for (int ii = 0; ii < (int)finalIndices->size(); ii++)
                    {
                        unsigned short idx = ((*finalIndices)[ii]);
                        int xIdx = idx * 2;

                        float vx = (*finalVerts)[xIdx];
                        float vy = (*finalVerts)[xIdx + 1];
                        float u = (*finalUVs)[xIdx];
                        float v = (*finalUVs)[xIdx + 1];

                        tri[triCount].x = writer->quantX(vx);
                        tri[triCount].y = writer->quantY(vy);
                        tri[triCount].u = writer->quantUV(u);
                        tri[triCount].v = writer->quantUV(v);

                        ++triCount;

                        if (triCount == 3)
                        {
                            writer->addTriangle(tri, blend8, alpha8);
                            triCount = 0;
                        }
                    }

                    assert(triCount == 0 && "Broken triangle list!");
                }
            }

            clipper.clipEnd(slot);
        }

        clipper.clipEnd();
    };

    startPass();

    for (float t = 0.0f; t <= duration; t += step)
    {
        animationState->update(step);
        animationState->apply(*skeleton);
        skeleton->updateWorldTransform(spine::Physics_Update);

        processFrame(nullptr);
    }

    PackedAnimWriter writer;
    writer.begin(fps, mx, my);

    startPass();

    for (float t = 0.0f; t <= duration; t += step)
    {
        animationState->update(step);
        animationState->apply(*skeleton);
        skeleton->updateWorldTransform(spine::Physics_Update);

        writer.beginFrame();
        processFrame(&writer);
        writer.endFrame();
    }

    if (writer.frameCount() == 0)
        return false;

    return writer.save(outFilename);
}

void SkeletonDrawable::render(float dt,
                              int64_t tag,
                              bool bGs,
                              bool skipRender)
{
    static bool bIsInited = false;
    static Vector<unsigned short> quadIndices;

    if (!bIsInited)
    {
        bIsInited = true;

        quadIndices.add(0);
        quadIndices.add(1);
        quadIndices.add(2);
        quadIndices.add(2);
        quadIndices.add(3);
        quadIndices.add(0);
    }

    const bool bCollectSpineLights = (!_lightAttachments.empty() || !_radialLights.empty());

    CTexture* texture = nullptr;
    SkeletonClipping& clipper = _clipping;

    if (bCollectSpineLights)
        _vPrevFrameLights.clear();

    clipper.clipEnd();

    float sparkLife = 0.0f;

    if (_fEffectInitialTime > 0.0f)
    {
        sparkLife = _fEffectLifeTime / _fEffectInitialTime;
        _fEffectLifeTime -= dt;
    }

    if (_fEffectLifeTime <= 0.0f || CGfx::getInstance()->isPotato())
    {
        _fEffectType = 0.0f;
        _fEffectInitialTime = 0.0f;
        sparkLife = 0.0f;

        if (!_effectCbCalled && _cbEffectComplete)
        {
            _effectCbCalled = true;
            _cbEffectComplete();
        }
    }

    if (sparkLife > 1.0f)
        sparkLife = 1.0f;

    if (sparkLife < 0.0f)
        sparkLife = 0.0f;

    bool initDoneCalled = false;

    if (!_bInitialized)
    {
        _left = INFINITY;
        _top = INFINITY;
        _right = -INFINITY;
        _bottom = -INFINITY;
    }

    int renderCount = 0;

    for (unsigned i = 0; i < skeleton->getSlots().size(); ++i)
    {
        Slot& slot = *skeleton->getDrawOrder()[i];
        Attachment* attachment = slot.getAttachment();

        if (!attachment)
        {
            clipper.clipEnd(slot);
            continue;
        }

        if ((slot.getColor().a == 0 || !slot.getBone().isActive()) &&
            !attachment->getRTTI().isExactly(ClippingAttachment::rtti))
        {
            clipper.clipEnd(slot);
            continue;
        }

        Vector<float>* worldVertices = &_worldVertices;
        Vector<unsigned short>* localQuadIndices = &quadIndices;

        int32_t verticesCount = 0;
        Vector<float>* uvs = nullptr;
        Vector<unsigned short>* indices = nullptr;
        int32_t indicesCount = 0;

        Color* attachmentColor = nullptr;

        if (attachment->getRTTI().isExactly(RegionAttachment::rtti))
        {
            RegionAttachment* regionAttachment = (RegionAttachment*)attachment;

            attachmentColor = &regionAttachment->getColor();
            if (attachmentColor->a == 0)
            {
                clipper.clipEnd(slot);
                continue;
            }

            worldVertices->setSize(8, 0);
            regionAttachment->computeWorldVertices(slot, *worldVertices, 0, 2);

            verticesCount = 4;
            uvs = &regionAttachment->getUVs();
            indices = localQuadIndices;
            indicesCount = 6;

            texture = (CTexture*)regionAttachment->getRegion()->rendererObject;
        }
        else if (attachment->getRTTI().isExactly(MeshAttachment::rtti))
        {
            MeshAttachment* mesh = (MeshAttachment*)attachment;

            attachmentColor = &mesh->getColor();
            if (attachmentColor->a == 0)
            {
                clipper.clipEnd(slot);
                continue;
            }

            worldVertices->setSize(mesh->getWorldVerticesLength(), 0);
            mesh->computeWorldVertices(slot,
                                       0,
                                       mesh->getWorldVerticesLength(),
                                       worldVertices->buffer(),
                                       0,
                                       2);

            verticesCount = (int32_t)(mesh->getWorldVerticesLength() >> 1);
            uvs = &mesh->getUVs();
            indices = &mesh->getTriangles();
            indicesCount = (int32_t)indices->size();

            texture = (CTexture*)mesh->getRegion()->rendererObject;
        }
        else if (attachment->getRTTI().isExactly(ClippingAttachment::rtti))
        {
            ClippingAttachment* clip = (ClippingAttachment*)slot.getAttachment();
            clipper.clipStart(slot, clip);
            continue;
        }
        else
        {
            clipper.clipEnd(slot);
            continue;
        }

        assert(!slot.hasDarkColor());

        spine::Color& slotColor = slot.getColor();

        float a = attachmentColor->a * slotColor.a * skeleton->getColor().a * RGBA[3];
        float r = attachmentColor->r * slotColor.r * skeleton->getColor().r * RGBA[0];
        float g = attachmentColor->g * slotColor.g * skeleton->getColor().g * RGBA[1];
        float b = attachmentColor->b * slotColor.b * skeleton->getColor().b * RGBA[2];

        if (!_lightAttachments.empty())
        {
            auto itLight = _lightAttachments.find(slot.getData().getIndex());

            if (itLight != _lightAttachments.end())
            {
                if (bCollectSpineLights &&
                    attachment->getRTTI().isExactly(RegionAttachment::rtti))
                {
                    collectLightAttachment(worldVertices,
                                           uvs,
                                           texture,
                                           a,
                                           r,
                                           g,
                                           b,
                                           itLight->second);
                }

                if (itLight->second.bDontRender)
                {
                    clipper.clipEnd(slot);
                    continue;
                }
            }
        }

        _timesDrawn++;

        if (clipper.isClipping())
        {
            clipper.clipTriangles(*worldVertices, *indices, *uvs, 2);

            worldVertices = &clipper.getClippedVertices();
            verticesCount = (int32_t)(clipper.getClippedVertices().size() >> 1);

            uvs = &clipper.getClippedUVs();
            indices = &clipper.getClippedTriangles();
            indicesCount = (int32_t)(clipper.getClippedTriangles().size());
        }

        if (!_bInitialized)
        {
            for (int ii = 0; ii < (int)indices->size(); ii++)
            {
                unsigned short idx = ((*indices)[ii]);

                int xIdx = idx * 2;
                int yIdx = (idx * 2) + 1;

                _left = std::min((*worldVertices)[xIdx], _left);
                _top = std::min((*worldVertices)[yIdx], _top);
                _right = std::max((*worldVertices)[xIdx], _right);
                _bottom = std::max((*worldVertices)[yIdx], _bottom);
            }
        }

        if (indices != nullptr && indices->size() > 0)
        {
            renderCount++;

            float fIsBlendNormal = 1.0f;

            switch (slot.getData().getBlendMode())
            {
                case BlendMode_Additive:
                    fIsBlendNormal = 0.0f;
                    break;

                case BlendMode_Multiply:
                    fIsBlendNormal = 2.0f;
                    break;

                default:
                    break;
            }

            auto screenWidth = CSceneResize::getInstance()->getGameWidth();
            auto screenHeight = CSceneResize::getInstance()->getScreenHeight();

            float ndcX = CGfx::getInstance()->getMatrixStack()->top()[6];
            float ndcY = CGfx::getInstance()->getMatrixStack()->top()[7];
            float m3 = CGfx::getInstance()->getMatrixStack()->top()[3];
            float m4 = CGfx::getInstance()->getMatrixStack()->top()[4];

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

            if (!CGfx::getInstance()->isPotato())
            {
                auto pMS = CGfx::getInstance()->getMatrixStack();

                for (int jj = 0; jj < SIZE_OF(_ghosts); jj++)
                {
                    if (_ghosts[jj].isActive())
                    {
                        pMS->save();
                        pMS->translate(_ghosts[jj].x, _ghosts[jj].y);

                        auto oldLightLayer = CGfx::getInstance()->getCurrentZLayer();

                        CGfx::getInstance()->preBatchVert(indices->size());
                        CGfx::getInstance()->batchSpineVect((CTexture*)texture,
                                                            worldVertices,
                                                            uvs,
                                                            indices,
                                                            r,
                                                            g,
                                                            b,
                                                            a * _ghosts[jj].fAlpha,
                                                            0.0f,
                                                            0.0f,
                                                            0.0f,
                                                            0.0f,
                                                            fIsBlendNormal,
                                                            3.0f,
                                                            _ghosts[jj].fAlpha,
                                                            _ptrNormalMap,
                                                            _bSkipLight,
                                                            reflectedMinY,
                                                            reflectedMaxY);

                        pMS->restore();
                    }
                }
            }

            if (!skipRender)
            {
                CGfx::getInstance()->preBatchVert(indices->size());

                if (bGs)
                {
                    CGfx::getInstance()->batchSpineVect((CTexture*)texture,
                                                        worldVertices,
                                                        uvs,
                                                        indices,
                                                        r,
                                                        g,
                                                        b,
                                                        a,
                                                        1.0f,
                                                        1.0f,
                                                        1.0f,
                                                        1.0f,
                                                        fIsBlendNormal,
                                                        _fEffectType,
                                                        sparkLife,
                                                        _ptrNormalMap,
                                                        _bSkipLight,
                                                        reflectedMinY,
                                                        reflectedMaxY);
                }
                else
                {
                    CGfx::getInstance()->batchSpineVect((CTexture*)texture,
                                                        worldVertices,
                                                        uvs,
                                                        indices,
                                                        r,
                                                        g,
                                                        b,
                                                        a,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        fIsBlendNormal,
                                                        _fEffectType,
                                                        sparkLife,
                                                        _ptrNormalMap,
                                                        _bSkipLight,
                                                        reflectedMinY,
                                                        reflectedMaxY);
                }
            }
        }

        clipper.clipEnd(slot);
    }

    clipper.clipEnd();

    if (bCollectSpineLights)
    {
        collectRadialLights(_vPrevFrameLights,
                            CGfx::getInstance()->getMatrixStack()->top());
    }

    if (!_bInitialized && _timesDrawn > 0)
    {
        if (isfinite(_left) && isfinite(_top) &&
            isfinite(_right) && isfinite(_bottom))
        {
            _bInitialized = true;
            updateBoundingBoxes();
            initDoneCalled = true;
        }
    }

    if (initDoneCalled && _onInitDone)
        _onInitDone();
}

void SkeletonDrawable::setLightAttachment(const char* lpszAttachmentName,
                                          bool bDontRender)
{
    if (!lpszAttachmentName || !lpszAttachmentName[0])
        return;

    auto pSlot = skeleton->findSlot(lpszAttachmentName);
    assert(pSlot);

    static_assert(std::is_same_v<decltype(pSlot->getData().getIndex()), int>,
                  "Spine run time depended map _lightAttachments shoud be changed to coresponding type");

    auto idx = pSlot->getData().getIndex();

    auto it = _lightAttachments.find(idx);

    if (it == _lightAttachments.end())
    {
        SpineLightAttachmentInfo info;

        info.bDontRender = bDontRender;

        info.lightTemplate.color = Point3(1.0f, 1.0f, 1.0f);
        info.lightTemplate.intensity = 1.0f;
        info.lightTemplate.radius = 0.0f;
        info.lightTemplate.bCanAffectGL = false;
        info.lightTemplate.eLayer = eLightLayer::GAME;
        info.lightTemplate.sprite = nullptr;
        info.lightTemplate.rotate = 0.0f;
        info.lightTemplate.zMin = -4096;
        info.lightTemplate.zMax = 4096;
        info.lightTemplate.itemCullingMask = 0xFFFF;
        info.lightTemplate.bScreenSpace = true;
        info.lightTemplate.spritePivotX = 0.5f;
        info.lightTemplate.spritePivotY = 0.5f;
        info.lightTemplate.spriteScaleX = 0.0f;
        info.lightTemplate.spriteScaleY = 0.0f;
        info.lightTemplate.bExplicitQuad = false;

        _lightAttachments.emplace(idx, info);
    }
    else
    {
        it->second.bDontRender = bDontRender;
    }
}

bool SkeletonDrawable::collectLightAttachment(Vector<float>* worldVertices,
                                              Vector<float>* uvs,
                                              CTexture* texture,
                                              float alpha,
                                              float red,
                                              float green,
                                              float blue,
                                              SpineLightAttachmentInfo& info)
{
    if (!texture || !texture->isUploaded())
        return false;

    if (alpha <= 0.001f)
        return false;

    if (!worldVertices || worldVertices->size() < 8)
        return false;

    if (!uvs || uvs->size() < 8)
        return false;

    float minU = (*uvs)[0];
    float maxU = (*uvs)[0];
    float minV = (*uvs)[1];
    float maxV = (*uvs)[1];

    for (int c = 1; c < 4; ++c)
    {
        float u = (*uvs)[c * 2 + 0];
        float v = (*uvs)[c * 2 + 1];

        if (u < minU) minU = u;
        if (u > maxU) maxU = u;
        if (v < minV) minV = v;
        if (v > maxV) maxV = v;
    }

    if ((maxU - minU) < 1e-5f || (maxV - minV) < 1e-5f)
        return false;

    int i00 = -1;
    int i10 = -1;
    int i01 = -1;
    int i11 = -1;

    const float EPS = 1e-3f;

    for (int c = 0; c < 4; ++c)
    {
        float u = (*uvs)[c * 2 + 0];
        float v = (*uvs)[c * 2 + 1];

        if (fabsf(u - minU) < EPS && fabsf(v - minV) < EPS)
            i00 = c;

        if (fabsf(u - maxU) < EPS && fabsf(v - minV) < EPS)
            i10 = c;

        if (fabsf(u - minU) < EPS && fabsf(v - maxV) < EPS)
            i01 = c;

        if (fabsf(u - maxU) < EPS && fabsf(v - maxV) < EPS)
            i11 = c;
    }

    int idx[4];

    if (i00 >= 0 && i10 >= 0 && i01 >= 0 && i11 >= 0)
    {
        idx[0] = i00;
        idx[1] = i01;
        idx[2] = i11;
        idx[3] = i10;
    }
    else
    {
        idx[0] = 0;
        idx[1] = 1;
        idx[2] = 2;
        idx[3] = 3;
    }

    GPULight l = info.lightTemplate;

    l.bExplicitQuad = true;
    l.explicitTexture = texture->getPtr();
    l.sprite = nullptr;
    l.radius = 0.0f;
    l.bScreenSpace = true;
    l.intensity = info.lightTemplate.intensity * alpha;
    l.color.set(red, green, blue);

    float screenW = (float)CSceneResize::getInstance()->getScreenWidth();
    float screenH = (float)CSceneResize::getInstance()->getScreenHeight();

    float* m = CGfx::getInstance()->getMatrixStack()->top();

    for (int k = 0; k < 4; ++k)
    {
        int c = idx[k];

        float wx = (*worldVertices)[c * 2 + 0];
        float wy = (*worldVertices)[c * 2 + 1];

        float ndcX = m[0] * wx + m[3] * wy + m[6];
        float ndcY = m[1] * wx + m[4] * wy + m[7];

        l.explicitX[k] = (ndcX + 1.0f) * 0.5f * screenW;
        l.explicitY[k] = (1.0f - ndcY) * 0.5f * screenH;
        l.explicitU[k] = (*uvs)[c * 2 + 0];
        l.explicitV[k] = (*uvs)[c * 2 + 1];
    }

    _vPrevFrameLights.push_back(l);

    return true;
}

static void getRadialAttachmentLocalPos(Attachment* att,
                                        float& outX,
                                        float& outY)
{
    outX = 0.0f;
    outY = 0.0f;

    if (!att)
        return;

    if (att->getRTTI().isExactly(RegionAttachment::rtti))
    {
        RegionAttachment* region = static_cast<RegionAttachment*>(att);
        outX = region->getX();
        outY = region->getY();
    }
    else if (att->getRTTI().isExactly(PointAttachment::rtti))
    {
        PointAttachment* point = static_cast<PointAttachment*>(att);
        outX = point->getX();
        outY = point->getY();
    }
}

static void radialXformPoint2(float a,
                              float b,
                              float c,
                              float d,
                              float x,
                              float y,
                              float& outX,
                              float& outY)
{
    outX = a * x + b * y;
    outY = c * x + d * y;
}

static void computeRadialBoneSetupTransform(Bone* bone,
                                            SpineRadialLight& info)
{
    if (!bone)
        return;

    std::vector<Bone*> chain;

    for (Bone* b = bone; b != nullptr; b = b->getParent())
        chain.push_back(b);

    std::reverse(chain.begin(), chain.end());

    struct SavedBone
    {
        float x;
        float y;
        float rot;
        float sx;
        float sy;
        float shx;
        float shy;
    };

    std::vector<SavedBone> saved(chain.size());

    for (size_t i = 0; i < chain.size(); ++i)
    {
        Bone* b = chain[i];

        saved[i] =
        {
            b->getX(),
            b->getY(),
            b->getRotation(),
            b->getScaleX(),
            b->getScaleY(),
            b->getShearX(),
            b->getShearY()
        };
    }

    for (Bone* b : chain)
        b->setToSetupPose();

    for (Bone* b : chain)
        b->updateWorldTransform();

    info.setupA = bone->getA();
    info.setupB = bone->getB();
    info.setupC = bone->getC();
    info.setupD = bone->getD();
    info.setupWorldX = bone->getWorldX();
    info.setupWorldY = bone->getWorldY();
    info.setupParentWorldX = bone->getParent()
                           ? bone->getParent()->getWorldX()
                           : 0.0f;

    for (size_t i = 0; i < chain.size(); ++i)
    {
        Bone* b = chain[i];

        b->setX(saved[i].x);
        b->setY(saved[i].y);
        b->setRotation(saved[i].rot);
        b->setScaleX(saved[i].sx);
        b->setScaleY(saved[i].sy);
        b->setShearX(saved[i].shx);
        b->setShearY(saved[i].shy);
    }

    for (Bone* b : chain)
        b->updateWorldTransform();
}

SpineRadialLight& SkeletonDrawable::addRadialLight(const char* lpszBoneName,
                                                   const char* lpszSlotName,
                                                   float fRadius,
                                                   float fIntensity)
{
    _radialLights.emplace_back();

    SpineRadialLight& res = _radialLights.back();

    if (lpszBoneName)
        res.sBoneName = lpszBoneName;

    if (lpszSlotName)
        res.sSlotName = lpszSlotName;

    res.fRadius = fRadius;
    res.fIntensity = fIntensity;

    res.invalidateCache();

    return res;
}

void SkeletonDrawable::removeRadialLight(SpineRadialLight& light)
{
    for (auto it = _radialLights.begin(); it != _radialLights.end(); ++it)
    {
        if (&(*it) == &light)
        {
            _radialLights.erase(it);
            return;
        }
    }
}

void SkeletonDrawable::clearRadialLights()
{
    _radialLights.clear();
}

void SkeletonDrawable::collectRadialLights(GPULights& out,
                                           const float* spineMat)
{
    if (_radialLights.empty())
        return;

    if (!spineMat)
        return;

    auto* pGfx = CGfx::getInstance();

    float screenW = (float)CSceneResize::getInstance()->getScreenWidth();
    float screenH = (float)CSceneResize::getInstance()->getScreenHeight();

    if (screenW <= 0.0f || screenH <= 0.0f)
        return;

    float lenX = std::sqrt(spineMat[0] * spineMat[0] +
                           spineMat[1] * spineMat[1]) * 0.5f * screenW;

    float lenY = std::sqrt(spineMat[3] * spineMat[3] +
                           spineMat[4] * spineMat[4]) * 0.5f * screenH;

    float spineScale = (lenX + lenY) * 0.5f;

    if (spineScale <= 1e-5f)
        spineScale = 1.0f;

    out.reserve(out.size() + _radialLights.size());

    for (auto& info : _radialLights)
    {
        if (!info.bEnabled)
            continue;

        if (info.fRadius <= 0.0f || info.fIntensity <= 0.0f)
            continue;

        if (info.sBoneName.empty())
            continue;

        if (!info.pBone)
            info.pBone = skeleton->findBone(info.sBoneName.c_str());

        if (!info.pBone)
            continue;

        if (!info.pBone->isActive())
            continue;

        if (!info.pSlot && !info.sSlotName.empty())
            info.pSlot = skeleton->findSlot(info.sSlotName.c_str());

        if (info.pSlot && !info.pSlot->getAttachment())
            continue;

        float alpha = 1.0f;

        if (!info.bIgnoreAlpha)
        {
            alpha = RGBA[3] * skeleton->getColor().a;

            if (info.pSlot)
                alpha *= info.pSlot->getColor().a;
        }

        if (alpha <= info.fAlphaThreshold)
            continue;

        float attLocalX = 0.0f;
        float attLocalY = 0.0f;

        if (info.bUseAttachmentOffset &&
            info.pSlot &&
            info.pSlot->getAttachment())
        {
            getRadialAttachmentLocalPos(info.pSlot->getAttachment(),
                                        attLocalX,
                                        attLocalY);
        }

        float a = info.pBone->getA();
        float b = info.pBone->getB();
        float c = info.pBone->getC();
        float d = info.pBone->getD();

        float attOffX = a * attLocalX + b * attLocalY;
        float attOffY = c * attLocalX + d * attLocalY;

        float localOffX = a * info.ptBoneLocalOffset.x +
                          b * info.ptBoneLocalOffset.y;

        float localOffY = c * info.ptBoneLocalOffset.x +
                          d * info.ptBoneLocalOffset.y;

        float boneX = info.pBone->getWorldX();
        float boneY = info.pBone->getWorldY();

        if (std::fabs(info.fSwayX - 1.0f) > 1e-5f)
        {
            float parentX = info.pBone->getParent()
                          ? info.pBone->getParent()->getWorldX()
                          : 0.0f;

            boneX = parentX + ((boneX - parentX) * info.fSwayX);
        }

        float spineX = boneX +
                       attOffX +
                       localOffX +
                       info.ptWorldOffset.x;

        float spineY = boneY +
                       attOffY +
                       localOffY +
                       info.ptWorldOffset.y;

        float tx = spineMat[0] * spineX +
                   spineMat[3] * spineY +
                   spineMat[6];

        float ty = spineMat[1] * spineX +
                   spineMat[4] * spineY +
                   spineMat[7];

        float screenX = (tx + 1.0f) * 0.5f * screenW;
        float screenY = (1.0f - ty) * 0.5f * screenH;

        if (info.lockAxis != eSpineRadialLockAxis::NONE)
        {
            if (!info.bSetupCached ||
                info.pSetupCacheSkeleton != skeleton ||
                info.pSetupCacheBone != info.pBone)
            {
                computeRadialBoneSetupTransform(info.pBone, info);

                info.bSetupCached = true;
                info.pSetupCacheSkeleton = skeleton;
                info.pSetupCacheBone = info.pBone;
            }

            float sAttOffX = 0.0f;
            float sAttOffY = 0.0f;

            radialXformPoint2(info.setupA,
                              info.setupB,
                              info.setupC,
                              info.setupD,
                              attLocalX,
                              attLocalY,
                              sAttOffX,
                              sAttOffY);

            float sOffX = 0.0f;
            float sOffY = 0.0f;

            radialXformPoint2(info.setupA,
                              info.setupB,
                              info.setupC,
                              info.setupD,
                              info.ptBoneLocalOffset.x,
                              info.ptBoneLocalOffset.y,
                              sOffX,
                              sOffY);

            float setupDeltaX = info.setupWorldX - info.setupParentWorldX;
            float setupModifiedBoneX = info.setupParentWorldX +
                                       (setupDeltaX * info.fSwayX);

            float setupSpaceX = setupModifiedBoneX +
                                sAttOffX +
                                sOffX +
                                info.ptWorldOffset.x;

            float setupSpaceY = info.setupWorldY +
                                sAttOffY +
                                sOffY +
                                info.ptWorldOffset.y;

            float stx = spineMat[0] * setupSpaceX +
                        spineMat[3] * setupSpaceY +
                        spineMat[6];

            float sty = spineMat[1] * setupSpaceX +
                        spineMat[4] * setupSpaceY +
                        spineMat[7];

            float setupLightX = (stx + 1.0f) * 0.5f * screenW;
            float setupLightY = (1.0f - sty) * 0.5f * screenH;

            if (info.lockAxis == eSpineRadialLockAxis::X)
                screenX = setupLightX;
            else
                screenY = setupLightY;
        }

        GPULight l;

        l.position.set(screenX, screenY);
        l.color       = info.color;
        l.intensity   = info.fIntensity * alpha;
        l.radius      = info.fRadius * spineScale;

        if (info.bScaleRadiusByAlpha)
            l.radius *= alpha;

        l.eLayer          = info.eLayer;
        l.bCanAffectGL    = info.bCanAffectGL;
        l.zMin            = info.zMin;
        l.zMax            = info.zMax;
        l.itemCullingMask = info.itemCullingMask;

        l.bScreenSpace = true;

        l.sprite         = nullptr;
        l.rotate         = 0.0f;
        l.spritePivotX   = 0.5f;
        l.spritePivotY   = 0.5f;
        l.spriteScaleX   = 0.0f;
        l.spriteScaleY   = 0.0f;
        l.bExplicitQuad  = false;

        out.push_back(l);
    }
}

GfxTextureLoader* GfxTextureLoader::_instance = nullptr;

GfxTextureLoader* GfxTextureLoader::getInstance()
{
    if (!GfxTextureLoader::_instance)
        GfxTextureLoader::_instance = new GfxTextureLoader();

    return GfxTextureLoader::_instance;
}

void GfxTextureLoader::load(AtlasPage& page, const String& path)
{
    std::cout << "SPINE LOADING: " << path.buffer() << std::endl;

    CTexturePtr pTexture = CGfx::getInstance()->uploadAsset(path.buffer());

    assert(pTexture);

    if (!pTexture)
        return;

    page.texture = pTexture.get();
}

void GfxTextureLoader::unload(void* texture)
{
}

}
