#pragma once

#include "g2d.h"
#include "assetloader.h"


_G2D_NAMESPACE_BEGIN_

// ------------------------------------------------------------------
// Ограничения на один батч.
// Если при записи батч получается больше, он автоматически режется
// на несколько батчей.
// ------------------------------------------------------------------
static constexpr uint32_t PACKED_MAX_BATCH_VERTS  = 16384;
static constexpr uint32_t PACKED_MAX_UNIQUE_VERTS = 16384;

#pragma pack(push, 1)

struct PackedAnimHeader
{
    char     magic[4];        // "PAN2"
    uint16_t version;
    uint16_t flags;
    float    fps;
    uint32_t frameCount;
    float    scaleX;
    float    scaleY;
    uint32_t maxBatchVerts;
    uint32_t maxUniqueVerts;
};

struct PackedFrameHeader
{
    uint32_t byteSize;        // размер тела кадра после этого заголовка
    uint16_t batchCount;
    uint16_t reserved;
};

struct PackedBatchHeader
{
    uint8_t  blend;           // 0 additive, 1 normal, 2 multiply
    uint8_t  alpha;           // 0..255
    uint8_t  mode;            // 0 = raw, 1 = indexed
    uint8_t  reserved;
    uint32_t vertCount;       // raw: expanded verts; indexed: unique verts
    uint32_t indexCount;      // raw: 0; indexed: expanded indices count
};

struct PackedPosUV
{
    int16_t  x, y;
    uint16_t u, v;
};

#pragma pack(pop)

static_assert(sizeof(PackedPosUV) == 8, "PackedPosUV must be 8 bytes");
static_assert(sizeof(PackedBatchHeader) == 12, "PackedBatchHeader must be 12 bytes");
static_assert(sizeof(PackedFrameHeader) == 8, "PackedFrameHeader must be 8 bytes");

static inline void packedAppendRaw(std::vector<uint8_t>& out,
                                   const void* p,
                                   size_t n)
{
    const uint8_t* b = (const uint8_t*)p;
    out.insert(out.end(), b, b + n);
}

static inline void packedPadTo4(std::vector<uint8_t>& out)
{
    while (out.size() & 3)
        out.push_back(0);
}

// ------------------------------------------------------------------
// Runtime resource.
//
// Хранит упакованные кадры.
// Специально сделан владеющим буфером, чтобы не зависеть от того,
// держит ли AssetLoader файл в памяти.
//
// Если у тебя AssetLoader гарантированно держит ресурс вечно,
// можно переделать на zero-copy через initFromMemory(..., false).
// ------------------------------------------------------------------
class PackedAnim
{
public:
    PackedAnim() = default;
    ~PackedAnim() = default;

    PackedAnim(const PackedAnim&) = delete;
    PackedAnim& operator=(const PackedAnim&) = delete;

    bool initFromMemory(const void* data, int size, bool copy = true)
    {
        if (!data || size < (int)sizeof(PackedAnimHeader))
            return false;

        if (copy)
        {
            _storage.resize(size);
            memcpy(_storage.data(), data, size);
            _data = _storage.data();
        }
        else
        {
            _storage.clear();
            _data = (const uint8_t*)data;
        }

        _size = size;

        memcpy(&_header, _data, sizeof(_header));

        if (memcmp(_header.magic, "PAN2", 4) != 0)
            return false;

        if (_header.version != 1)
            return false;

        if (_header.maxBatchVerts > PACKED_MAX_BATCH_VERTS ||
            _header.maxUniqueVerts > PACKED_MAX_UNIQUE_VERTS)
        {
            return false;
        }

        const int tableSize = (int)(_header.frameCount * sizeof(uint32_t));
        if (size < (int)sizeof(PackedAnimHeader) + tableSize)
            return false;

        _frameTable = _data + sizeof(PackedAnimHeader);

        return true;
    }

    bool load(const char* filename)
    {
        int sz = 0;
        const void* p = AssetLoader::instance().getLoadedFile(filename, sz);
        if (!p || sz <= 0)
            return false;

        return initFromMemory(p, sz, false);
    }

    uint32_t frameCount() const
    {
        return _header.frameCount;
    }

    float fps() const
    {
        return _header.fps;
    }

    float scaleX() const
    {
        return _header.scaleX;
    }

    float scaleY() const
    {
        return _header.scaleY;
    }

    const uint8_t* framePtr(uint32_t idx) const
    {
        if (idx >= _header.frameCount)
            return nullptr;

        uint32_t off = 0;
        memcpy(&off, _frameTable + idx * sizeof(uint32_t), sizeof(off));

        return _data + off;
    }

private:
    std::vector<uint8_t> _storage;

    const uint8_t*   _data = nullptr;
    int              _size = 0;
    const uint8_t*   _frameTable = nullptr;
    PackedAnimHeader _header{};
};

using PackedAnimPtr = std::shared_ptr<const PackedAnim>;

// ------------------------------------------------------------------
// Общий кэш упакованных анимаций.
//
// Использование:
//
// auto anim = PackedAnimCache::get("intro.panm");
//
// Несколько спинов могут использовать один и тот же anim.
// ------------------------------------------------------------------
class PackedAnimCache
{
public:
    static PackedAnimPtr get(const char* filename)
    {
        if (!filename || !filename[0])
            return nullptr;

        auto& m = getMap();
        auto it = m.find(filename);
        if (it != m.end())
            return it->second;

        auto p = std::make_shared<PackedAnim>();
        if (!p->load(filename))
            return nullptr;

        PackedAnimPtr shared = p;
        m.emplace(filename, shared);

        return shared;
    }

    static void preload(const char* filename)
    {
        get(filename);
    }

private:
    static std::unordered_map<std::string, PackedAnimPtr>& getMap()
    {
        static std::unordered_map<std::string, PackedAnimPtr> map;
        return map;
    }
};

// ------------------------------------------------------------------
// Writer.
// Используется только при записи/запекании.
// ------------------------------------------------------------------
class PackedAnimWriter
{
public:
    void begin(float fps, float scaleX, float scaleY)
    {
        _fps = fps;
        _scaleX = scaleX;
        _scaleY = scaleY;

        _body.clear();
        _offsets.clear();
        _frameCount = 0;

        _frameHeaderPos = 0;
        _payloadStart = 0;
        _batchCount = 0;

        _batch.clear();
    }

    uint32_t frameCount() const
    {
        return _frameCount;
    }

    int16_t quantX(float x) const
    {
        float v = (x / _scaleX) * 32767.0f;
        v = std::clamp(v, -32767.0f, 32767.0f);
        return (int16_t)std::lround(v);
    }

    int16_t quantY(float y) const
    {
        float v = (y / _scaleY) * 32767.0f;
        v = std::clamp(v, -32767.0f, 32767.0f);
        return (int16_t)std::lround(v);
    }

    uint16_t quantUV(float t) const
    {
        t = std::clamp(t, 0.0f, 1.0f);
        return (uint16_t)std::lround(t * 65535.0f);
    }

    uint8_t quantAlpha(float a) const
    {
        a = std::clamp(a, 0.0f, 1.0f);
        return (uint8_t)std::lround(a * 255.0f);
    }

    void beginFrame()
    {
        _batch.clear();

        _batchCount = 0;
        _frameHeaderPos = _body.size();

        PackedFrameHeader fh{};
        packedAppendRaw(_body, &fh, sizeof(fh));

        _payloadStart = _body.size();
    }

    void addTriangle(const PackedPosUV tri[3], uint8_t blend, uint8_t alpha)
    {
        if (!_batch.active ||
            _batch.blend != blend ||
            _batch.alpha != alpha)
        {
            flushBatch();
            _batch.begin(blend, alpha);
        }

        if (_batch.needFlushForTriangle(PACKED_MAX_BATCH_VERTS,
                                        PACKED_MAX_UNIQUE_VERTS))
        {
            flushBatch();
            _batch.begin(blend, alpha);
        }

        _batch.addVertex(tri[0]);
        _batch.addVertex(tri[1]);
        _batch.addVertex(tri[2]);
    }

    void endFrame()
    {
        flushBatch();

        PackedFrameHeader fh{};
        fh.byteSize = (uint32_t)(_body.size() - _payloadStart);
        fh.batchCount = _batchCount;
        fh.reserved = 0;

        memcpy(&_body[_frameHeaderPos], &fh, sizeof(fh));

        _offsets.push_back((uint32_t)_frameHeaderPos);
        ++_frameCount;
    }

    bool save(const char* filename) const
    {
        if (!filename || !filename[0] || _frameCount == 0)
            return false;

        const uint32_t tableBytes = (uint32_t)(_offsets.size() * sizeof(uint32_t));
        const uint32_t base = (uint32_t)(sizeof(PackedAnimHeader) + tableBytes);

        std::vector<uint8_t> out;
        out.resize(sizeof(PackedAnimHeader) + tableBytes);

        PackedAnimHeader hdr{};
        memcpy(hdr.magic, "PAN2", 4);
        hdr.version = 1;
        hdr.flags = 0;
        hdr.fps = _fps;
        hdr.frameCount = _frameCount;
        hdr.scaleX = _scaleX;
        hdr.scaleY = _scaleY;
        hdr.maxBatchVerts = PACKED_MAX_BATCH_VERTS;
        hdr.maxUniqueVerts = PACKED_MAX_UNIQUE_VERTS;

        memcpy(&out[0], &hdr, sizeof(hdr));

        for (size_t i = 0; i < _offsets.size(); ++i)
        {
            uint32_t realOff = base + _offsets[i];
            memcpy(&out[sizeof(PackedAnimHeader) + i * sizeof(uint32_t)],
                   &realOff,
                   sizeof(realOff));
        }

        out.insert(out.end(), _body.begin(), _body.end());

        FILE* f = fopen(filename, "wb");
        if (!f)
            return false;

        fwrite(out.data(), 1, out.size(), f);
        fclose(f);

        return true;
    }

private:
    struct PackedBatchBuilder
    {
        bool active = false;
        uint8_t blend = 0;
        uint8_t alpha = 0;

        std::vector<PackedPosUV> unique;
        std::vector<uint16_t> indices;
        std::unordered_map<uint64_t, uint16_t> map;

        void begin(uint8_t b, uint8_t a)
        {
            active = true;
            blend = b;
            alpha = a;

            unique.clear();
            indices.clear();
            map.clear();

            if (map.bucket_count() < 4096)
                map.reserve(4096);
        }

        void clear()
        {
            active = false;
            unique.clear();
            indices.clear();
            map.clear();
        }

        bool needFlushForTriangle(uint32_t maxBatchVerts,
                                  uint32_t maxUniqueVerts) const
        {
            if (!active)
                return false;

            return (indices.size() + 3 > maxBatchVerts) ||
                   (unique.size() + 3 > maxUniqueVerts);
        }

        static uint64_t makeKey(const PackedPosUV& v)
        {
            return ((uint64_t)(uint16_t)v.x << 48) |
                   ((uint64_t)(uint16_t)v.y << 32) |
                   ((uint64_t)v.u << 16) |
                   ((uint64_t)v.v);
        }

        void addVertex(const PackedPosUV& v)
        {
            uint64_t key = makeKey(v);
            auto it = map.find(key);

            uint16_t idx = 0;
            if (it == map.end())
            {
                idx = (uint16_t)unique.size();
                unique.push_back(v);
                map.emplace(key, idx);
            }
            else
            {
                idx = it->second;
            }

            indices.push_back(idx);
        }

        void flush(std::vector<uint8_t>& out, uint16_t& batchCount)
        {
            if (!active || indices.empty())
            {
                clear();
                return;
            }

            const size_t rawBytes =
                indices.size() * sizeof(PackedPosUV);

            const size_t indexedBytes =
                unique.size() * sizeof(PackedPosUV) +
                indices.size() * sizeof(uint16_t);

            bool useIndexed = indexedBytes < rawBytes;

            PackedBatchHeader bh{};
            bh.blend = blend;
            bh.alpha = alpha;

            if (useIndexed)
            {
                bh.mode = 1;
                bh.vertCount = (uint32_t)unique.size();
                bh.indexCount = (uint32_t)indices.size();

                packedAppendRaw(out, &bh, sizeof(bh));
                packedAppendRaw(out,
                                unique.data(),
                                unique.size() * sizeof(PackedPosUV));
                packedAppendRaw(out,
                                indices.data(),
                                indices.size() * sizeof(uint16_t));
                packedPadTo4(out);
            }
            else
            {
                bh.mode = 0;
                bh.vertCount = (uint32_t)indices.size();
                bh.indexCount = 0;

                packedAppendRaw(out, &bh, sizeof(bh));

                for (uint16_t idx : indices)
                {
                    packedAppendRaw(out,
                                    &unique[idx],
                                    sizeof(PackedPosUV));
                }
            }

            ++batchCount;
            clear();
        }
    };

    void flushBatch()
    {
        _batch.flush(_body, _batchCount);
    }

private:
    std::vector<uint8_t> _body;
    std::vector<uint32_t> _offsets;

    float _fps = 30.0f;
    float _scaleX = 1.0f;
    float _scaleY = 1.0f;

    uint32_t _frameCount = 0;

    size_t _frameHeaderPos = 0;
    size_t _payloadStart = 0;
    uint16_t _batchCount = 0;

    PackedBatchBuilder _batch;
};

_G2D_NAMESPACE_END_