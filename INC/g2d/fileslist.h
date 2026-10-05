#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

typedef std::unordered_map<std::string, uint64_t> FilesListMap;

class FilesList
{
    inline static FilesListMap _map;

public:
    inline static void addFile(LPCTSTR lpszPath, int64_t size)
    {
        auto res = _map.insert_or_assign(lpszPath, size);
        assert(res.second && "Already had that file");
    }
    inline static void moveInit(FilesListMap && m) { _map = std::move(m); }
    inline static auto& get() { return _map; }
};

_G2D_NAMESPACE_END_