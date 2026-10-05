#pragma once

#include "g2d.h"
#include "engine.h"

_G2D_NAMESPACE_BEGIN_

class L10N
{
public:

    static L10N& getInstance();

    // грузит ini заданного языка и делает его активным
    bool load(eLang lang);

    // смена активного языка (перечитает ini; повторный вызов с тем же
    // языком — без перечитывания)
    void setLanguage(eLang lang);

    eLang getLanguage() const { return _lang; }
    const char* getLangCode() const;

    // ключ -> строка активного языка; неизвестный ключ возвращается
    // как есть (удобно заметить опечатку прямо на экране/в логе)
    const char* tr(const char* key) const;
    bool has(const char* key) const;

private:
    L10N() = default;

    eLang _lang = eLang::EN;
    std::unordered_map<std::string, std::string> _map;

    static std::string loadFile(const char* lpszRelPath);
    static void trim(std::string& s);
};
_G2D_NAMESPACE_END_
