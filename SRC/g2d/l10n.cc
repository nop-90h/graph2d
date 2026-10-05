#include "pch.h"
#include "l10n.h"

namespace {

void Unescape(std::string& s)
{
    std::string out;
    out.reserve(s.size());

    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];

        if (c == '\\' && i + 1 < s.size())
        {
            char next = s[++i];

            switch (next)
            {
                case 'n':
                    out.push_back('\n');
                    break;

                case 't':
                    out.push_back('\t');
                    break;

                case 'r':
                    break;

                case '"':
                    out.push_back('"');
                    break;

                case '\'':
                    out.push_back('\'');
                    break;

                case '\\':
                    out.push_back('\\');
                    break;

                default:
                    out.push_back('\\');
                    out.push_back(next);
                    break;
            }
        }
        else
        {
            out.push_back(c);
        }
    }

    s.swap(out);
}

} // namespace

_G2D_NAMESPACE_BEGIN_

std::string L10N::loadFile(const char* lpszRelPath)
{
    std::string fullPath = std::string("EMBED/l10n/") + lpszRelPath;
    std::string out;

    FILE* f = std::fopen(fullPath.c_str(), "rb");
    if (!f)
        return out;

    if (std::fseek(f, 0, SEEK_END) == 0)
    {
        long nSize = std::ftell(f);
        if (nSize > 0)
        {
            out.resize((size_t)nSize);
            std::rewind(f);

            size_t nRead = std::fread(&out[0], 1, (size_t)nSize, f);
            if (nRead != (size_t)nSize)
                out.resize(nRead);
        }
    }

    std::fclose(f);
    return out;
}

void L10N::trim(std::string& s)
{
    const char* ws = " \t\r\n";

    size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos)
    {
        s.clear();
        return;
    }

    size_t e = s.find_last_not_of(ws);
    s = s.substr(b, e - b + 1);
}

L10N& L10N::getInstance()
{
    static L10N inst;
    return inst;
}

const char* L10N::getLangCode() const
{
    switch (_lang)
    {
        case eLang::RU: return "ru";
        default:        return "en";
    }
}

bool L10N::load(eLang lang)
{
    _lang = lang;
    _map.clear();

    std::string content = loadFile(
        lang == eLang::RU ? "ru.ini" : "en.ini"
    );

    if (content.empty())
        return false;

    // UTF-8 BOM
    if (content.size() >= 3 &&
        (unsigned char)content[0] == 0xEF &&
        (unsigned char)content[1] == 0xBB &&
        (unsigned char)content[2] == 0xBF)
    {
        content.erase(0, 3);
    }

    size_t pos = 0;
    const size_t n = content.size();

    while (pos < n)
    {
        size_t eol = content.find('\n', pos);

        if (eol == std::string::npos)
            eol = n;

        std::string line;
        line.reserve(eol > pos ? eol - pos : 0);

        // \
        // 
        for (size_t i = pos; i < eol; ++i)
        {
            char c = content[i];
            if (c != '\r')
                line.push_back(c);
        }

        pos = (eol == n) ? n : eol + 1;

        trim(line);

        if (line.empty())
            continue;

        if (line[0] == ';' || line[0] == '#' || line[0] == '[')
            continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        trim(key);
        trim(val);

        if (key.empty())
            continue;

        if (val.size() >= 2 &&
            ((val.front() == '"' && val.back() == '"') ||
             (val.front() == '\'' && val.back() == '\'')))
        {
            val = val.substr(1, val.size() - 2);
        }

        Unescape(val);

        _map[key] = std::move(val);
    }

    return true;
}

void L10N::setLanguage(eLang lang)
{
    if (lang == _lang && !_map.empty())
        return;

    load(lang);
}

const char* L10N::tr(const char* key) const
{
    if (!key)
        return "";

    auto it = _map.find(key);

    assert(it != _map.end());

    return (it != _map.end()) ? it->second.c_str() : key;
}

bool L10N::has(const char* key) const
{
    if (!key)
        return false;

    return _map.find(key) != _map.end();
}

_G2D_NAMESPACE_END_