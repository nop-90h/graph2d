#include "sprite.h"
#include "gfx.h"
#ifdef HAS_SPINE
#include "spinecont.h"
#endif
#include "container.h"
#include "particle.h"
#ifndef HTMLDOM_EXTERNAL_FONTSTASH
#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"
#undef FONTSTASH_IMPLEMENTATION
#else
#include "fontstash.h"
#endif
#include "htmldom.h"
#include "textrender.h"
#include "textrender_helpers.h"
#include "spriteloader.h"
#include "inputcontroller.h"
#include "l10n.h"

_G2D_NAMESPACE_BEGIN_

struct HTMLDom::LayoutParams
{
    float x = 0.f;
    float y = 0.f;
    float maxWidth = 0.f;
    float maxHeight = 0.f;
    int align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
    unsigned int color = 0xFFFFFFFF;
    float appearStart = -1.f;
    unsigned long long animKey = 0;
};

struct HTMLDom::RenderContext
{
    HTMLDom* dom = nullptr;
    FONScontext* fs = nullptr;
    unsigned long long rootId = 0;
    LayoutParams params;
    bool measureOnly = false;
    unsigned long long animKey = 0;
    float blockStart = 0.f;
    float animTime = 0.f;
    float currentX = 0.f;
    float currentY = 0.f;

    float extraAlpha = 1.f;

    Rect* visRecordRc = nullptr;
    bool* visRecordGot = nullptr;
    Rect* rcOut = nullptr;
    bool hasBounds = false;
    float maxContentWidth = 0.f;

    int baseChunk = 0;
    float totalWidth = 0.f;
    float lineHeight = 0.f;

    void renderLine(float x, float y, float w, float h, unsigned int color);
    void updateBounds(float bx, float by, float bw, float bh);
    float renderChar(
        const char* ch,
        float charX,
        float charY,
        int charSize,
        unsigned int charColor,
        unsigned int charTint,
        const HTMLRenderState& state,
        int charIndex,
        int totalChars,
        unsigned long long fontAnimId
    );
    void pushChunk(const char* text, int len, float width, const HTMLRenderState& state);
    void pushImageChunk(const HTMLDomNode& node, float w, float h, float marginL, float marginR,
        unsigned int color, const HTMLRenderState& state);
    void addWord(const char* word, int len, float wordWidth, const HTMLRenderState& state);
    void flushLine(const HTMLRenderState& state, bool bAdvanceIfEmpty);

    unsigned long long currentLinkId = 0;
    unsigned long long currentCursorId = 0;
    unsigned long long currentFontAnimId = 0;
    unsigned int disabledTint = 0xFFFFFFFF;
    bool grayScale = false;
    unsigned int documentColor = 0xFFFFFFFF;

    unsigned int inheritedDisabledTint = 0xFFFFFFFF;
    bool inheritedDisabled = false;
};

struct HTMLChunk
{
    const char* text = nullptr;
    int len = 0;
    int stateIndex = 0;
    unsigned int colorMul = 0xFFFFFFFF;
    bool isImage = false;
    CTexturePtr imgTex;
    float imgUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float imgW = 0.f, imgH = 0.f;
    int imgValign = 1;
    float imgMarginL = 0.f;
    unsigned int imgColor = 0xFFFFFFFF;
    unsigned long long imgNodeId = 0;
};

struct HTMLTableCell
{
    HTMLDomNode* tdNode = nullptr;
    int styleStart = 0;
    int styleEnd = 0;
    float width = 0.f;
    float height = 0.f;
    int align = FONS_ALIGN_LEFT;
    eVAlign vAlign = eVAlign::TOP;
    unsigned int bgColor = 0;
    HTMLPadding padding;
    float maxWidth = 0.f;
};

struct HTMLGroupWidth
{
    const std::string* name = nullptr;
    float w = 0.f;
};

struct HTMLRenderScratch
{

    std::vector<HTMLRenderState> statePool;
    int stateCount = 0;
    int lastStateIndex = -1;

    std::vector<HTMLChunk> chunks;
    std::vector<unsigned long long> chunkLinkIds;
    std::vector<unsigned long long> chunkFontAnimIds;
    int chunkCount = 0;

    std::vector<std::string> snapFont;
    int snapDepth = 0;

    std::string textBuf;
    std::string textBuf2;

    std::vector<HTMLDomNode*> trNodes;
    std::vector<HTMLDomNode*> stylePath;
    std::vector<HTMLDomNode*> cellStylePool;
    std::vector<HTMLTableCell> cells;
    std::vector<int> rowCellStart;
    std::vector<float> rowHeights;
    std::vector<float> colWidths;
    std::vector<float> colNatWidths;
    std::vector<float> colMinWidths;
    std::vector<float> colWant;
    std::vector<int> colAligns;
    std::vector<float> colMaxWidths;
    std::vector<size_t> order;
    std::vector<float> rowYBounds;
    std::vector<float> colXBounds;

    std::vector<HTMLDomNode*> options;
    HTMLFontAttrs optionFontAttrs;

    std::vector<HTMLGroupWidth> groups;

    HTMLRenderState rootState;
    bool rootStateInited = false;

    void beginFrame()
    {
        stateCount = 0;
        lastStateIndex = -1;
        chunkCount = 0;
        snapDepth = 0;
        groups.clear();
    }
};

namespace
{
    inline void snapPointToBuffer(float& x, float& y)
    {
        auto m = CGfx::getInstance()->getMatrixStack()->top();
        float a = m[0], b = m[1];
        float c = m[3], d = m[4];
        float tx = m[6], ty = m[7];
        float det = a * d - b * c;
        if (std::fabs(det) < 1e-6f)
            return;
        float w = (float)CSceneResize::getInstance()->getScreenWidth();
        float h = (float)CSceneResize::getInstance()->getScreenHeight();
        if (w <= 0.f || h <= 0.f)
            return;
        float ox = a * x + b * y + tx;
        float oy = c * x + d * y + ty;
        float px = std::floor((ox * 0.5f + 0.5f) * w + 0.5f);
        float py = std::floor((oy * 0.5f + 0.5f) * h + 0.5f);
        float nx = (px / w) * 2.f - 1.f;
        float ny = (py / h) * 2.f - 1.f;
        float ex = nx - tx;
        float ey = ny - ty;
        x = (d * ex - b * ey) / det;
        y = (-c * ex + a * ey) / det;
    }

    bool domIsBlockTag(eHTMLTag t)
    {
        switch (t)
        {
        case eHTMLTag::Body:
        case eHTMLTag::BgSpine:
        case eHTMLTag::Particles:
        case eHTMLTag::Hr: case eHTMLTag::Img: case eHTMLTag::Nine: case eHTMLTag::Three:
        case eHTMLTag::Spine: case eHTMLTag::Table: case eHTMLTag::P: case eHTMLTag::H1:
        case eHTMLTag::H2: case eHTMLTag::H3: case eHTMLTag::Center: case eHTMLTag::NineButton:
        case eHTMLTag::Ul: case eHTMLTag::Li: case eHTMLTag::Checkbox: case eHTMLTag::Select:
        case eHTMLTag::Slider: case eHTMLTag::ProgressBar:
        case eHTMLTag::Edit:
            return true;
        default:
            return false;
        }
    }

    bool domNodeContainsId(const HTMLDom::NodePtr& node, unsigned long long id)
    {
        if (!node)
            return false;
        if (node->id == id)
            return true;
        for (const auto& child : node->children)
            if (domNodeContainsId(child, id))
                return true;
        return false;
    }

    void domSyncVirtualKeyboard(const HTMLDom::NodePtr& editNode, bool bFocused)
    {
        if (!editNode)
            return;
        if (bFocused)
        {
            auto pw = editNode->findEffectiveAttr("password");
            InputController::getInstance()->showVirtualKeyboard(editNode->editValue.c_str(), pw != nullptr);
        }
        else
        {
            InputController::getInstance()->hideVirtualKeyboard();
        }
    }

    unsigned int domGrayColor(unsigned int c)
    {
        unsigned int r = (c >> 24) & 0xFF;
        unsigned int g = (c >> 16) & 0xFF;
        unsigned int b = (c >> 8) & 0xFF;
        unsigned int a = c & 0xFF;
        unsigned int y = (r * 77u + g * 150u + b * 29u) >> 8;
        return (y << 24) | (y << 16) | (y << 8) | a;
    }

    FONScontext* domFs(void)
    {
        return TextRender::getInstance()->getFonsContext();
    }

    std::string domToLower(const std::string& s)
    {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        return out;
    }

    unsigned int domUtf8DecodeAt(const std::string& s, size_t pos, size_t& len)
    {
        len = 1;
        if (pos >= s.size())
            return 0;
        unsigned char b0 = (unsigned char)s[pos];
        if (b0 < 0x80)
            return b0;
        if ((b0 & 0xE0) == 0xC0)
        {
            if (pos + 1 < s.size() && (((unsigned char)s[pos + 1] & 0xC0) == 0x80))
            {
                len = 2;
                return ((unsigned int)(b0 & 0x1F) << 6) | ((unsigned int)((unsigned char)s[pos + 1] & 0x3F));
            }
            return b0;
        }
        if ((b0 & 0xF0) == 0xE0)
        {
            if (pos + 2 < s.size() &&
                (((unsigned char)s[pos + 1] & 0xC0) == 0x80) &&
                (((unsigned char)s[pos + 2] & 0xC0) == 0x80))
            {
                len = 3;
                return ((unsigned int)(b0 & 0x0F) << 12) |
                        ((unsigned int)((unsigned char)s[pos + 1] & 0x3F) << 6) |
                        ((unsigned int)((unsigned char)s[pos + 2] & 0x3F));
            }
            return b0;
        }
        if ((b0 & 0xF8) == 0xF0)
        {
            if (pos + 3 < s.size() &&
                (((unsigned char)s[pos + 1] & 0xC0) == 0x80) &&
                (((unsigned char)s[pos + 2] & 0xC0) == 0x80) &&
                (((unsigned char)s[pos + 3] & 0xC0) == 0x80))
            {
                len = 4;
                return ((unsigned int)(b0 & 0x07) << 18) |
                        ((unsigned int)((unsigned char)s[pos + 1] & 0x3F) << 12) |
                        ((unsigned int)((unsigned char)s[pos + 2] & 0x3F) << 6) |
                        ((unsigned int)((unsigned char)s[pos + 3] & 0x3F));
            }
            return b0;
        }
        return b0;
    }

    bool domIsAsciiSpaceChar(unsigned char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
    }

    bool domIsNbspCodePoint(unsigned int cp)
    {
        return cp == 0x00A0;
    }

    size_t domPrevUtf8Start(const std::string& s, size_t endExclusive)
    {
        if (endExclusive == 0 || endExclusive > s.size())
            return std::string::npos;
        size_t pos = endExclusive - 1;
        while (pos > 0 && (((unsigned char)s[pos] & 0xC0) == 0x80))
            --pos;
        return pos;
    }

    bool domStringIsRemovableWhitespace(const std::string& s)
    {
        size_t i = 0;
        while (i < s.size())
        {
            unsigned char c = (unsigned char)s[i];
            if (domIsAsciiSpaceChar(c))
            {
                ++i;
                continue;
            }
            size_t len = 1;
            unsigned int cp = domUtf8DecodeAt(s, i, len);
            if (domIsNbspCodePoint(cp))
                return false;
            return false;
        }
        return true;
    }

    bool domHasNonRemovableWhitespace(const std::string& s)
    {
        return !domStringIsRemovableWhitespace(s);
    }

    void domCollapseAsciiWhiteSpacePreserveNbsp(std::string& s)
    {
        std::string out;
        out.reserve(s.size());
        bool inSpace = false;
        size_t i = 0;
        while (i < s.size())
        {
            unsigned char c = (unsigned char)s[i];
            if (domIsAsciiSpaceChar(c))
            {
                if (!inSpace)
                {
                    out.push_back(' ');
                    inSpace = true;
                }
                ++i;
                continue;
            }
            size_t len = 1;
            domUtf8DecodeAt(s, i, len);
            out.append(s, i, len);
            inSpace = false;
            i += len;
        }
        s.swap(out);
    }

    void domTrimLeftPreserveNbsp(std::string& s)
    {
        size_t i = 0;
        while (i < s.size())
        {
            unsigned char c = (unsigned char)s[i];
            if (domIsAsciiSpaceChar(c))
            {
                ++i;
                continue;
            }
            break;
        }
        if (i)
            s.erase(0, i);
    }

    void domTrimRightPreserveNbsp(std::string& s)
    {
        while (!s.empty())
        {
            size_t start = domPrevUtf8Start(s, s.size());
            if (start == std::string::npos)
                break;
            size_t len = 1;
            unsigned int cp = domUtf8DecodeAt(s, start, len);
            if (domIsNbspCodePoint(cp))
                break;
            if (len == 1 && domIsAsciiSpaceChar((unsigned char)s[start]))
                s.erase(start);
            else
                break;
        }
    }

    void domReplaceAll(std::string& s, const std::string& from, const std::string& to)
    {
        if (from.empty())
            return;
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos)
        {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
    }

    void domSubstituteLocalization(std::string& s)
    {
        L10N& l10n = L10N::getInstance();
        size_t pos = 0;
        while ((pos = s.find("##", pos)) != std::string::npos)
        {
            size_t end = s.find("##", pos + 2);
            if (end == std::string::npos)
                break;

            bool bOk = end > pos + 2;
            for (size_t i = pos + 2; bOk && i < end; ++i)
            {
                char c = s[i];
                bOk = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
            }
            if (!bOk)
            {
                pos += 2;
                continue;
            }
            std::string key = s.substr(pos + 2, end - (pos + 2));
            const char* val = l10n.tr(key.c_str());
            s.replace(pos, end + 2 - pos, val);
            pos += strlen(val);
        }
    }

    std::string domDecodeTextPreserveNbsp(const std::string& raw)
    {
        std::string s = raw;
        const std::string AMP_PLACEHOLDER = "\xEE\x80\x80";
        const std::string NBSP_PLACEHOLDER = "\xEE\x80\x81";
        const std::string NBSP_UTF8 = "\xC2\xA0";
        domReplaceAll(s, "&amp;", AMP_PLACEHOLDER);
        domReplaceAll(s, "&AMP;", AMP_PLACEHOLDER);
        domReplaceAll(s, "&nbsp;", NBSP_PLACEHOLDER);
        domReplaceAll(s, "&NBSP;", NBSP_PLACEHOLDER);
        domReplaceAll(s, "&#160;", NBSP_PLACEHOLDER);
        domReplaceAll(s, "&#xA0;", NBSP_PLACEHOLDER);
        domReplaceAll(s, "&#xa0;", NBSP_PLACEHOLDER);
        std::string decoded = decodeHTMLEntities(s);
        domReplaceAll(decoded, NBSP_PLACEHOLDER, NBSP_UTF8);
        domReplaceAll(decoded, AMP_PLACEHOLDER, "&");
        return decoded;
    }

    bool parseDomTag(const std::string& tagStr, std::string& name, HTMLAttrsMap& attrs, bool& isClosing, bool& selfClosing)
    {
        if (tagStr.empty() || tagStr[0] != '<')
            return false;
        size_t pos = 1;
        isClosing = (pos < tagStr.size() && tagStr[pos] == '/');
        if (isClosing)
            pos++;
        size_t nameStart = pos;
        while (pos < tagStr.size() && !isAsciiSpace((unsigned char)tagStr[pos]) && tagStr[pos] != '>' && tagStr[pos] != '/')
            pos++;
        name = domToLower(tagStr.substr(nameStart, pos - nameStart));
        selfClosing = false;
        while (pos < tagStr.size() && tagStr[pos] != '>' && tagStr[pos] != '/')
        {
            while (pos < tagStr.size() && isAsciiSpace((unsigned char)tagStr[pos]))
                pos++;
            if (pos >= tagStr.size() || tagStr[pos] == '>' || tagStr[pos] == '/')
                break;
            size_t attrNameStart = pos;
            while (pos < tagStr.size() && tagStr[pos] != '=' && !isAsciiSpace((unsigned char)tagStr[pos]) && tagStr[pos] != '>' && tagStr[pos] != '/')
                pos++;
            std::string attrName = tagStr.substr(attrNameStart, pos - attrNameStart);
            attrName = domToLower(attrName);
            while (pos < tagStr.size() && isAsciiSpace((unsigned char)tagStr[pos]))
                pos++;
            std::string attrVal;
            if (pos < tagStr.size() && tagStr[pos] == '=')
            {
                pos++;
                if (pos < tagStr.size() && (tagStr[pos] == '"' || tagStr[pos] == '\''))
                {
                    char quote = tagStr[pos++];
                    size_t valStart = pos;
                    while (pos < tagStr.size() && tagStr[pos] != quote)
                        pos++;
                    attrVal = tagStr.substr(valStart, pos - valStart);
                    if (pos < tagStr.size())
                        pos++;
                }
                else
                {
                    size_t valStart = pos;
                    while (pos < tagStr.size() && !isAsciiSpace((unsigned char)tagStr[pos]) && tagStr[pos] != '>')
                        pos++;
                    attrVal = tagStr.substr(valStart, pos - valStart);
                }
            }
            if (!attrName.empty())
                attrs.insert(attrName, domDecodeTextPreserveNbsp(attrVal));
        }
        if (pos < tagStr.size() && tagStr[pos] == '/')
            selfClosing = true;
        return true;
    }

    float domGetAttrFloat(const HTMLAttrsMap& attrs, const char* name, float def)
    {
        auto p = attrs.find(name);
        if (!p)
            return def;
        return std::stof(*p);
    }

    std::string domGetAttrStr(const HTMLAttrsMap& attrs, const char* name, const std::string& def = "")
    {
        auto p = attrs.find(name);
        return p ? *p : def;
    }

    void domRebuildResolvedAttrs(HTMLDomNode& node)
    {
        node.attrsResolved = node.attrs;
        for (const auto& kv : node.attrsClass)
        {
            if (!node.attrsResolved.find(kv.first.c_str()))
                node.attrsResolved.insert(kv.first, kv.second);
        }
    }

    float domGetNodeAttrFloat(const HTMLDomNode& node, const char* name, float def)
    {
        auto p = node.findEffectiveAttr(name);
        if (!p || p->empty())
            return def;
        return std::stof(*p);
    }

    std::string domGetNodeAttrStr(const HTMLDomNode& node, const char* name, const std::string& def = std::string())
    {
        auto p = node.findEffectiveAttr(name);
        return p ? *p : def;
    }

    struct HTMLCssTextParser
    {
        const char* p;
        explicit HTMLCssTextParser(const char* s) : p(s ? s : "") {}
        void skipWs()
        {
            for (;;)
            {
                while (*p && isAsciiSpace((unsigned char)*p))
                    ++p;
                if (p[0] == '/' && p[1] == '*')
                {
                    const char* e = strstr(p + 2, "*/");
                    if (!e)
                    {
                        p += strlen(p);
                        return;
                    }
                    p = e + 2;
                    continue;
                }
                break;
            }
        }
        static bool identChar(char c)
        {
            unsigned char uc = (unsigned char)c;
            return std::isalnum(uc) || c == '-' || c == '_';
        }
    };

    void domParseCssDeclarations(HTMLCssTextParser& tp, HTMLCssClassRule& rule)
    {
        for (;;)
        {
            tp.skipWs();
            if (!*tp.p || *tp.p == '}')
            {
                if (*tp.p == '}')
                    ++tp.p;
                return;
            }
            std::string prop;
            if (HTMLCssTextParser::identChar(*tp.p))
            {
                const char* s = tp.p;
                while (*tp.p && HTMLCssTextParser::identChar(*tp.p))
                    ++tp.p;
                prop.assign(s, tp.p - s);
            }
            tp.skipWs();
            if (prop.empty() || *tp.p != ':')
            {

                while (*tp.p && *tp.p != ';' && *tp.p != '}')
                    ++tp.p;
                if (*tp.p == ';')
                    ++tp.p;
                continue;
            }
            ++tp.p;
            tp.skipWs();
            std::string val;
            if (*tp.p == '"' || *tp.p == '\'')
            {
                char q = *tp.p++;
                const char* s = tp.p;
                while (*tp.p && *tp.p != q)
                    ++tp.p;
                val.assign(s, tp.p - s);
                if (*tp.p)
                    ++tp.p;
            }
            else
            {
                const char* s = tp.p;
                while (*tp.p && *tp.p != ';' && *tp.p != '}')
                    ++tp.p;
                const char* e = tp.p;
                while (e > s && isAsciiSpace((unsigned char)e[-1]))
                    --e;
                val.assign(s, e - s);
            }
            if (!val.empty())
            {
                std::string pLow = domToLower(prop);
                if (pLow == "inherit")
                {

                    std::string vLow = domToLower(val);
                    if (vLow.find("all") != std::string::npos)
                        rule.inheritAll = true;
                }
                else
                {

                    bool found = false;
                    for (auto& d : rule.decls)
                    {
                        if (d.name == pLow)
                        {
                            d.value = val;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        rule.decls.push_back(HTMLCssClassDecl());
                        rule.decls.back().name = std::move(pLow);
                        rule.decls.back().value = std::move(val);
                    }
                }
            }
            tp.skipWs();
            if (*tp.p == ';')
                ++tp.p;
        }
    }

    void cacheRuntimeAttrs(HTMLDomNode& node)
    {

        float fade = domGetAttrFloat(node.attrsResolved, "fade", 0.12f);
        fade = domGetAttrFloat(node.attrsResolved, "fadedur", fade);
        node.hoverFadeDurCache = domGetAttrFloat(node.attrsResolved, "hoverfade", fade);
        node.hoverFadeDurCache = domGetAttrFloat(node.attrsResolved, "hoverfadedur", node.hoverFadeDurCache);
        node.pressedFadeDurCache = domGetAttrFloat(node.attrsResolved, "pressedfade", node.hoverFadeDurCache);
        node.pressedFadeDurCache = domGetAttrFloat(node.attrsResolved, "pressedfadedur", node.pressedFadeDurCache);
        node.runtimeFadeCached = true;

        node.checkboxSpacingCache = domGetAttrFloat(node.attrsResolved, "spacing", 4.f);
        node.checkboxCheckFadeDurCache = domGetAttrFloat(node.attrsResolved, "checkfade", node.hoverFadeDurCache);
        node.runtimeCheckboxCached = true;

        node.ulIndentCache = domGetAttrFloat(node.attrsResolved, "indent", 16.f);
        node.liBulletCache = domGetAttrStr(node.attrsResolved, "bullet", "");
        node.runtimeListCached = true;
    }

    bool domHasAttr(const HTMLAttrsMap& attrs, const char* name)
    {
        return attrs.find(name) != nullptr;
    }

    int domGetAttrInt(const HTMLAttrsMap& attrs, const char* name, int def)
    {
        auto p = attrs.find(name);
        if (!p || p->empty())
            return def;
        return std::stoi(*p);
    }

    bool domGetAttrBool(const HTMLAttrsMap& attrs, const char* name, bool def)
    {
        auto p = attrs.find(name);
        if (!p)
            return def;
        return (*p == "true" || *p == "1" || *p == "yes");
    }

    eHTMLTag parseTagEnum(const std::string& name)
    {
        if (name == "br") return eHTMLTag::Br;
        if (name == "hr") return eHTMLTag::Hr;
        if (name == "img") return eHTMLTag::Img;
        if (name == "nine") return eHTMLTag::Nine;
        if (name == "three") return eHTMLTag::Three;
#ifdef HAS_SPINE
        if (name == "spine") return eHTMLTag::Spine;
#endif
        if (name == "table") return eHTMLTag::Table;
        if (name == "tr") return eHTMLTag::Tr;
        if (name == "td") return eHTMLTag::Td;
        if (name == "font") return eHTMLTag::Font;
        if (name == "b") return eHTMLTag::B;
        if (name == "strong") return eHTMLTag::Strong;
        if (name == "i") return eHTMLTag::I;
        if (name == "em") return eHTMLTag::Em;
        if (name == "u") return eHTMLTag::U;
        if (name == "s") return eHTMLTag::S;
        if (name == "p") return eHTMLTag::P;
        if (name == "h1") return eHTMLTag::H1;
        if (name == "h2") return eHTMLTag::H2;
        if (name == "h3") return eHTMLTag::H3;
        if (name == "center") return eHTMLTag::Center;
        if (name == "a") return eHTMLTag::A;
        if (name == "ninebutton") return eHTMLTag::NineButton;
        if (name == "ul") return eHTMLTag::Ul;
        if (name == "li") return eHTMLTag::Li;
        if (name == "checkbox") return eHTMLTag::Checkbox;
        if (name == "select") return eHTMLTag::Select;
        if (name == "option") return eHTMLTag::Option;
        if (name == "slider") return eHTMLTag::Slider;
        if (name == "progressbar") return eHTMLTag::ProgressBar;
        if (name == "progress") return eHTMLTag::ProgressBar;
        if (name == "span") return eHTMLTag::Span;
        if (name == "edit") return eHTMLTag::Edit;
        if (name == "body") return eHTMLTag::Body;
        if (name == "bg") return eHTMLTag::Body;
#ifdef HAS_SPINE
        if (name == "bgspine") return eHTMLTag::BgSpine;
#endif
        if (name == "particles") return eHTMLTag::Particles;
        return eHTMLTag::Unknown;
    }

    eHTMLIdleStyle parseIdleEnum(const std::string& s)
    {
        if (s == "glare") return eHTMLIdleStyle::Glare;
        if (s == "pixel") return eHTMLIdleStyle::Pixel;
        if (s == "chase") return eHTMLIdleStyle::Chase;
        if (s == "lens") return eHTMLIdleStyle::Lens;
        if (s == "echo") return eHTMLIdleStyle::Echo;
        if (s == "embers") return eHTMLIdleStyle::Embers;
        if (s == "tear") return eHTMLIdleStyle::Tear;
        if (s == "lightning") return eHTMLIdleStyle::Lightning;
        if (s == "ice") return eHTMLIdleStyle::Ice;
        if (s == "melt") return eHTMLIdleStyle::Melt;
        if (s == "dust") return eHTMLIdleStyle::Dust;
        if (s == "vortex") return eHTMLIdleStyle::Vortex;
        if (s == "confetti") return eHTMLIdleStyle::Confetti;
        return eHTMLIdleStyle::None;
    }

    int parseAlignValue(const std::string& s, int def)
    {
        if (s == "left") return FONS_ALIGN_LEFT;
        if (s == "center") return FONS_ALIGN_CENTER;
        if (s == "right") return FONS_ALIGN_RIGHT;
        return def;
    }

    eGridStyle parseGridStyleValue(const std::string& s)
    {
        if (s == "all" || s == "full") return eGridStyle::ALL;
        if (s == "header") return eGridStyle::HEADER;
        if (s == "v" || s == "vertical") return eGridStyle::VERTICAL;
        if (s == "h" || s == "horizontal") return eGridStyle::HORIZONTAL;
        if (s == "vh" || s == "hv") return eGridStyle::VERTICAL_HORIZONTAL;
        return eGridStyle::NONE;
    }

    bool loadTextureInfo(CTexturePtr& tex, float uv[4], float& w, float& h, const std::string& path)
    {
        tex = CTexturePtr();
        uv[0] = 0.f; uv[1] = 0.f; uv[2] = 0.f; uv[3] = 0.f;
        w = 0.f; h = 0.f;
        if (path.empty())
            return false;
        auto pSpr = SpriteLoader::getInstance()->getProtoSprite(path.c_str());
        if (!pSpr)
            return false;
        tex = pSpr->getTexture();
        if (!tex)
            return false;
        uv[0] = pSpr->_uvs[CSprite::VERT_ULX];
        uv[1] = pSpr->_uvs[CSprite::VERT_ULY];
        uv[2] = pSpr->_uvs[CSprite::VERT_URX];
        uv[3] = pSpr->_uvs[CSprite::VERT_BLY];
        w = pSpr->_verts[CSprite::VERT_BRX] - pSpr->_verts[CSprite::VERT_ULX];
        h = pSpr->_verts[CSprite::VERT_BRY] - pSpr->_verts[CSprite::VERT_ULY];

        if ((w <= 0.f || h <= 0.f) && tex)
        {
            const float tw = (float)tex->getCx();
            const float th = (float)tex->getCy();
            if (w <= 0.f)
                w = (uv[2] - uv[0]) * tw;
            if (h <= 0.f)
                h = (uv[3] - uv[1]) * th;
        }
        return true;
    }

    void loadSpriteInfo(HTMLDomNode& node, const std::string& path)
    {
        loadTextureInfo(node.spriteTex, node.spriteUV, node.spriteSrcW, node.spriteSrcH, path);
    }

    void parseFontAttrs(const HTMLAttrsMap& attrs, HTMLFontAttrs& out)
    {
        if (auto p = attrs.find("name")) { out.hasFontName = true; out.fontName = *p; }
        if (auto p = attrs.find("size")) { out.hasFontSize = true; out.fontSize = std::stoi(*p); }
        if (auto p = attrs.find("color")) { out.hasColor = true; out.color = parseHTMLColor(*p); }
        if (auto p = attrs.find("fx")) { out.hasFx = true; out.fxStyle = parseFxStyle(*p); }
        if (auto p = attrs.find("shadow")) { out.hasShadow = true; out.shadow = std::stof(*p); }
        if (auto p = attrs.find("shadowcolor")) { out.hasShadowColor = true; out.shadowColor = parseHTMLColor(*p); }
        if (auto p = attrs.find("outline")) { out.hasOutline = true; out.outline = std::stof(*p); }
        if (auto p = attrs.find("outlinecolor")) { out.hasOutlineColor = true; out.outlineColor = parseHTMLColor(*p); }
        if (auto p = attrs.find("gradient"))
        {
            out.hasGradient = true;
            size_t comma = p->find(',');
            if (comma != std::string::npos)
            {
                out.gradientColor1 = parseHTMLColor(p->substr(0, comma));
                out.gradientColor2 = parseHTMLColor(p->substr(comma + 1));
            }
            else
            {
                out.gradientColor1 = parseHTMLColor(*p);
                out.gradientColor2 = out.gradientColor1;
            }
        }
        if (auto p = attrs.find("appear")) { out.hasAppear = true; out.appearStyle = parseAppearStyle(*p); }
        if (auto p = attrs.find("lineheight")) { out.hasLineHeight = true; out.lineHeightMul = std::stof(*p); }
        else if (auto p = attrs.find("line-height")) { out.hasLineHeight = true; out.lineHeightMul = std::stof(*p); }
        auto ptrDur = attrs.find("dur");
        if (!ptrDur) ptrDur = attrs.find("duration");
        if (!ptrDur) ptrDur = attrs.find("time");
        if (!ptrDur) ptrDur = attrs.find("appearduration");
        if (ptrDur) { out.hasAppearDur = true; out.appearDur = std::stof(*ptrDur); }
        auto ptrAppearDelay = attrs.find("appeardelay");
        auto ptrDelay = attrs.find("delay");
        if (ptrAppearDelay) { out.hasAppearDelay = true; out.appearDelay = std::stof(*ptrAppearDelay); }
        else if (ptrDelay) { out.hasAppearDelay = true; out.appearDelay = std::stof(*ptrDelay); }
        auto ptrFxDelay = attrs.find("fxdelay");
        if (!ptrFxDelay) ptrFxDelay = attrs.find("repeatdelay");
        if (!ptrFxDelay) ptrFxDelay = attrs.find("delay");
        if (ptrFxDelay) { out.hasFxDelay = true; out.fxDelay = std::stof(*ptrFxDelay); }
    }

    unsigned int domMulColor(unsigned int base, unsigned int tint)
    {
        if (tint == 0xFFFFFFFF)
            return base;
        if (base == 0xFFFFFFFF)
            return tint;
        unsigned int br = (base >> 24) & 0xFF, bg = (base >> 16) & 0xFF, bb = (base >> 8) & 0xFF, ba = base & 0xFF;
        unsigned int tr = (tint >> 24) & 0xFF, tg = (tint >> 16) & 0xFF, tb = (tint >> 8) & 0xFF, ta = tint & 0xFF;
        unsigned int r = (br * tr) / 255, g = (bg * tg) / 255, b = (bb * tb) / 255, a = (ba * ta) / 255;
        return (r << 24) | (g << 16) | (b << 8) | a;
    }

    unsigned int domNodeDisabledTint(const HTMLDomNode& node)
    {
        if (!node.disabled)
            return 0xFFFFFFFF;
        if (node.hasDisabledTint)
            return node.disabledTint;
        return 0x808080FF;
    }

    float domSnap(float v)
    {
        return std::floor(v + 0.5f);
    }

    float domFakeBoldOffset(int fontSize)
    {
        return std::max(1.f, std::floor((float)fontSize * 0.05f + 0.5f));
    }

    inline unsigned int domAlphaColor(float a)
    {
        a = std::clamp(a, 0.f, 1.f);
        return 0xFFFFFF00u | (unsigned int)(a * 255.f + 0.5f);
    }

    struct Aff2
    {
        float m[6];
    };

    inline void affSet(Aff2& r, float px, float py, float rot, float sx, float sy, float ox, float oy)
    {
        float cs = std::cos(rot);
        float sn = std::sin(rot);
        r.m[0] = cs * sx;
        r.m[1] = -sn * sy;
        r.m[2] = (px + ox) - (r.m[0] * px + r.m[1] * py);
        r.m[3] = sn * sx;
        r.m[4] = cs * sy;
        r.m[5] = (py + oy) - (r.m[3] * px + r.m[4] * py);
    }

    inline void affMul(Aff2& r, const Aff2& a, const Aff2& b)
    {
        r.m[0] = a.m[0] * b.m[0] + a.m[1] * b.m[3];
        r.m[1] = a.m[0] * b.m[1] + a.m[1] * b.m[4];
        r.m[2] = a.m[0] * b.m[2] + a.m[1] * b.m[5] + a.m[2];
        r.m[3] = a.m[3] * b.m[0] + a.m[4] * b.m[3];
        r.m[4] = a.m[3] * b.m[1] + a.m[4] * b.m[4];
        r.m[5] = a.m[3] * b.m[2] + a.m[4] * b.m[5] + a.m[5];
    }

    void domVisPivot(const HTMLDomNode& node, float originX, float originY, float& px, float& py)
    {
        if (node.visPivotSet)
        {
            px = originX + node.visPivotX;
            py = originY + node.visPivotY;
            return;
        }
        if (node.visLastRect.cx > 0.f && node.visLastRect.cy > 0.f)
        {
            px = originX + node.visLastRect.x + node.visLastRect.cx * 0.5f;
            py = originY + node.visLastRect.y + node.visLastRect.cy * 0.5f;
            return;
        }
        float w = node.reqWidth > 0.f ? node.reqWidth : node.spriteSrcW;
        float h = node.reqHeight > 0.f ? node.reqHeight : node.spriteSrcH;
        px = originX + w * 0.5f;
        py = originY + h * 0.5f;
    }

    bool domSliceMinWH(const HTMLDomNode& node, bool isNine, float& minW, float& minH)
    {
        minW = 0.f;
        minH = 0.f;
        if (!node.spriteTex)
            return false;
        if (isNine)
        {
            minW = node.sliceA + node.sliceB;
            minH = node.sliceC + node.sliceD;
            return true;
        }
        if (node.sliceVertical)
            minH = node.sliceC + node.sliceD;
        else
            minW = node.sliceA + node.sliceB;
        return true;
    }

    void parseDisabledAttrs(HTMLDomNode& node)
    {
        if (node.tag == eHTMLTag::Option)
        {
            node.disabled = !node.optionEnabled;
        }
        else if (auto p = node.findEffectiveAttr("disabled"))
        {
            node.disabled = p->empty() || *p == "true" || *p == "1" || *p == "yes";
        }
        if (auto p = node.findEffectiveAttr("disabledtint"))
        {
            if (p->empty())
            {
                node.hasDisabledTint = false;
                node.disabledTint = 0xFFFFFFFF;
            }
            else
            {
                node.hasDisabledTint = true;
                node.disabledTint = parseHTMLColor(*p);
            }
        }
        else if (auto p = node.findEffectiveAttr("disabledcolor"))
        {
            if (p->empty())
            {
                node.hasDisabledTint = false;
                node.disabledTint = 0xFFFFFFFF;
            }
            else
            {
                node.hasDisabledTint = true;
                node.disabledTint = parseHTMLColor(*p);
            }
        }
    }

    void updateEffectiveCursor(HTMLDomNode& node)
    {
        if (node.disabled)
        {
            node.cursorExplicit = false;
            node.cursor = "normal";
            return;
        }
        auto p = node.findEffectiveAttr("cursor");
        if (p)
        {
            node.cursorExplicit = true;
            node.cursor = *p;
            if (node.cursor.empty())
                node.cursor = "normal";
        }
        else
        {
            node.cursorExplicit = false;
            if (node.onClick || node.clickable)
                node.cursor = "hand";
            else
                node.cursor = "normal";
        }
    }

    float clampSliderVal(const HTMLDomNode& node, float v)
    {
        float mn = std::min(node.sliderMin, node.sliderMax);
        float mx = std::max(node.sliderMin, node.sliderMax);
        if (mx <= mn)
            return mn;
        v = std::clamp(v, mn, mx);
        if (node.sliderStep > 0.f)
        {
            float k = std::floor((v - mn) / node.sliderStep + 0.5f);
            v = mn + k * node.sliderStep;
            v = std::clamp(v, mn, mx);
        }
        return v;
    }

    float sliderNorm(const HTMLDomNode& node)
    {
        float mn = std::min(node.sliderMin, node.sliderMax);
        float mx = std::max(node.sliderMin, node.sliderMax);
        if (mx <= mn)
            return 0.f;
        return std::clamp((node.sliderValue - mn) / (mx - mn), 0.f, 1.f);
    }

    void sliderTrackInsets(const HTMLDomNode& node, float& inset0, float& inset1)
    {
        inset0 = 0.f;
        inset1 = 0.f;
        if (!node.spriteTex)
            return;
        if (node.sliderVertical)
        {
            inset0 = std::max(0.f, node.sliceC);
            inset1 = std::max(0.f, node.sliceD);
        }
        else
        {
            inset0 = std::max(0.f, node.sliceA);
            inset1 = std::max(0.f, node.sliceB);
        }
    }

    void sliderGripTravel(const HTMLDomNode& node, float& outMin, float& outMax)
    {
        const Rect& rc = node.sliderLocalRect;
        float gripW = std::max(1.f, node.sliderGripW);
        float gripH = std::max(1.f, node.sliderGripH);
        float i0 = 0.f, i1 = 0.f;
        sliderTrackInsets(node, i0, i1);
        if (node.sliderVertical)
        {
            float trackY = rc.y + gripH * 0.5f;
            float trackH = rc.cy - gripH;
            if (trackH <= 0.f)
            {
                trackY = rc.y;
                trackH = rc.cy;
            }
            float cTop = trackY + i0 + gripH * 0.5f;
            float cBottom = trackY + trackH - i1 - gripH * 0.5f;
            if (cBottom < cTop)
            {
                float m = (cTop + cBottom) * 0.5f;
                cTop = m;
                cBottom = m;
            }
            outMin = cBottom;
            outMax = cTop;
        }
        else
        {
            float trackX = rc.x + gripW * 0.5f;
            float trackW = rc.cx - gripW;
            if (trackW <= 0.f)
            {
                trackX = rc.x;
                trackW = rc.cx;
            }
            float cLeft = trackX + i0 + gripW * 0.5f;
            float cRight = trackX + trackW - i1 - gripW * 0.5f;
            if (cRight < cLeft)
            {
                float m = (cLeft + cRight) * 0.5f;
                cLeft = m;
                cRight = m;
            }
            outMin = cLeft;
            outMax = cRight;
        }
    }

    float clampProgress01(float v)
    {
        if (v < 0.f)
            return 0.f;
        if (v > 1.f)
            return 1.f;
        return v;
    }

    bool parseProgressVertical(const std::string& s)
    {
        return s == "v" || s == "vert" || s == "vertical";
    }

    int parseProgressMirror(const std::string& s)
    {
        if (s == "right" || s == "bottom" || s == "second" || s == "2")
            return 1;
        if (s == "left" || s == "top" || s == "first" || s == "1")
            return 2;
        return 0;
    }

    float parseProgressNormalized(const HTMLAttrsMap& attrs)
    {
        if (domHasAttr(attrs, "percent"))
            return clampProgress01(domGetAttrFloat(attrs, "percent", 0.f) / 100.f);
        float min = domGetAttrFloat(attrs, "min", 0.f);
        float max = 1.f;
        bool hasMax = domHasAttr(attrs, "max");
        if (hasMax)
            max = domGetAttrFloat(attrs, "max", 1.f);
        if (min > max)
            std::swap(min, max);
        float value = min;
        bool hasValue = false;
        if (domHasAttr(attrs, "value"))
        {
            value = domGetAttrFloat(attrs, "value", min);
            hasValue = true;
        }
        else if (domHasAttr(attrs, "progress"))
        {
            value = domGetAttrFloat(attrs, "progress", min);
            hasValue = true;
        }
        if (!hasValue)
            return 0.f;
        if (hasMax)
        {
            float span = max - min;
            if (span <= 0.f)
                return 0.f;
            return clampProgress01((value - min) / span);
        }
        if (value >= 0.f && value <= 1.f)
            return clampProgress01(value);
        return clampProgress01(value / 100.f);
    }

    void progressInnerRect(const HTMLDomNode& node, float x, float y, float w, float h,
        float& outX, float& outY, float& outW, float& outH)
    {
        float l = std::max(0.f, node.padding.left);
        float t = std::max(0.f, node.padding.top);
        float r = std::max(0.f, node.padding.right);
        float b = std::max(0.f, node.padding.bottom);
        outX = x + l;
        outY = y + t;
        outW = std::max(0.f, w - l - r);
        outH = std::max(0.f, h - t - b);
    }

    void parseNodeAttrs(HTMLDomNode& node)
    {
        if (!node.attrsResolvedInited)
        {
            domRebuildResolvedAttrs(node);
            node.attrsResolvedInited = true;
        }
        const HTMLAttrsMap& attrs = node.attrsResolved;

        switch (node.tag)
        {
        case eHTMLTag::Particles:
        {
            node.particlesPreset = domGetAttrStr(attrs, "preset", "");
            if (node.particlesPreset.empty())
                node.particlesPreset = domGetAttrStr(attrs, "src", "");
            node.reqWidth = std::max(0.f, domGetAttrFloat(attrs, "width", 100.f));
            node.reqHeight = std::max(0.f, domGetAttrFloat(attrs, "height", 100.f));
            node.particlesPrewarm = std::max(0.f, domGetAttrFloat(attrs, "prewarm", 0.f));
            break;
        }
        case eHTMLTag::Body:
        {
            std::string bgPath = domGetAttrStr(attrs, "src", "");
            if (bgPath.empty())
                bgPath = domGetAttrStr(attrs, "background", "");
            if (bgPath.empty())
                bgPath = domGetAttrStr(attrs, "backgroundimage", "");
            if (bgPath.empty())
                bgPath = domGetAttrStr(attrs, "bgimage", "");
            if (!bgPath.empty())
            {
                loadSpriteInfo(node, bgPath);
                node.hasBodyBg = node.spriteTex ? true : false;
            }
            node.bodyBgOpacity = std::clamp(domGetAttrFloat(attrs, "bgopacity", 1.f), 0.f, 1.f);
            node.bodyMinWidth = std::max(0.f, domGetAttrFloat(attrs, "minwidth", 0.f));
            node.bodyMinHeight = std::max(0.f, domGetAttrFloat(attrs, "minheight", 0.f));
            {
                std::string bgSize = domToLower(domGetAttrStr(attrs, "bgsize", ""));
                if (bgSize == "stretch" || bgSize == "fill")
                    node.bodyBgSize = 2;
                else if (bgSize == "contain")
                    node.bodyBgSize = 1;
                else
                    node.bodyBgSize = 0;
            }
            if (auto p = attrs.find("bgcolor"))
                node.bgColor = parseHTMLColor(*p);
            break;
        }
#ifdef HAS_SPINE
        case eHTMLTag::BgSpine:
        {

            node.spinePath = domGetAttrStr(attrs, "src", "");
            if (node.spinePath.empty())
                node.spinePath = domGetAttrStr(attrs, "path", "");
            node.spineAnim = domGetAttrStr(attrs, "animation", "");
            if (node.spineAnim.empty())
                node.spineAnim = domGetAttrStr(attrs, "anim", "");
            node.spineLoop = domGetAttrBool(attrs, "loop", true);
            node.reqWidth = domGetAttrFloat(attrs, "width", 0.f);
            node.reqHeight = domGetAttrFloat(attrs, "height", 0.f);
            node.bodyMinWidth = std::max(0.f, domGetAttrFloat(attrs, "minwidth", 0.f));
            node.bodyMinHeight = std::max(0.f, domGetAttrFloat(attrs, "minheight", 0.f));
            {
                std::string bgSize = domToLower(domGetAttrStr(attrs, "bgsize", ""));
                if (bgSize == "stretch" || bgSize == "fill")
                    node.bodyBgSize = 2;
                else if (bgSize == "contain")
                    node.bodyBgSize = 1;
                else
                    node.bodyBgSize = 0;
            }
            node.bodyBgOpacity = std::clamp(domGetAttrFloat(attrs, "bgopacity", 1.f), 0.f, 1.f);
            std::string rcbox = domGetAttrStr(attrs, "rcbox", "");
            node.spineHasRcBox = false;
            if (!rcbox.empty())
                node.spineHasRcBox = parseRcBox(rcbox, node.spineRc);
            break;
        }
#endif
        case eHTMLTag::Hr:
            node.hrThickness = domGetAttrFloat(attrs, "thickness", 1.f);
            node.hrColor = parseHTMLColor(domGetAttrStr(attrs, "color", "#FFFFFFFF"));
            node.hrMargin = domGetAttrFloat(attrs, "margin", -1.f);
            node.hrWidth = domGetAttrFloat(attrs, "width", -1.f);
            break;
        case eHTMLTag::Img:
        {
            std::string path = domGetAttrStr(attrs, "src", "");
            node.reqWidth = domGetAttrFloat(attrs, "width", 0.f);
            node.reqHeight = domGetAttrFloat(attrs, "height", 0.f);
            node.keepAspect = domGetAttrBool(attrs, "keepaspect", false);
            node.imgInline = domGetAttrBool(attrs, "inline", false);
            node.imgValign = parseVAlign(domToLower(domGetAttrStr(attrs, "valign", "middle")), eVAlign::MIDDLE);
            node.idleStyle = parseIdleEnum(domGetAttrStr(attrs, "idle", ""));
            node.idleDelay = domGetAttrFloat(attrs, "idledelay", -1.f);
            node.idleDur = domGetAttrFloat(attrs, "idledur", -1.f);
            node.glareWidth = domGetAttrFloat(attrs, "glarewidth", 40.f);
            node.glareSpeed = domGetAttrFloat(attrs, "glarespeed", 150.f);
            node.glareTilt = domGetAttrFloat(attrs, "glaretilt", 20.f);
            node.glareIntensity = domGetAttrFloat(attrs, "glareintensity", 0.7f);
            std::string dir = domGetAttrStr(attrs, "glaredir", "lr");
            if (dir == "rl") node.glareDir = 1;
            else if (dir == "tb") node.glareDir = 2;
            else if (dir == "bt") node.glareDir = 3;
            else node.glareDir = 0;
            node.pxBlock = domGetAttrFloat(attrs, "pxblock", 10.f);
            node.pxBand = domGetAttrFloat(attrs, "pxband", 56.f);
            node.pxSpeed = domGetAttrFloat(attrs, "pxspeed", 260.f);
            node.pxGlow = domGetAttrFloat(attrs, "pxglow", 1.f);
            loadSpriteInfo(node, path);
            break;
        }
        case eHTMLTag::Nine:
        case eHTMLTag::Three:
        {
            std::string path = domGetAttrStr(attrs, "src", "");
            node.reqWidth = domGetAttrFloat(attrs, "width", 0.f);
            node.reqHeight = domGetAttrFloat(attrs, "height", 0.f);
            node.sliceA = domGetAttrFloat(attrs, "a", 0.f);
            node.sliceB = domGetAttrFloat(attrs, "b", 0.f);
            node.sliceC = domGetAttrFloat(attrs, "c", 0.f);
            node.sliceD = domGetAttrFloat(attrs, "d", 0.f);
            node.padding = parsePaddingAttr(attrs, 8.f, 8.f);
            if (node.tag == eHTMLTag::Three)
            {
                std::string orient = domGetAttrStr(attrs, "orient", "");
                node.sliceVertical = (orient == "vert" || orient == "v");
                std::string mirror = domGetAttrStr(attrs, "mirror", "");
                if (mirror == "right" || mirror == "bottom" || mirror == "second" || mirror == "2")
                    node.sliceMirror = 1;
                else if (mirror == "left" || mirror == "top" || mirror == "first" || mirror == "1")
                    node.sliceMirror = 2;
                else
                    node.sliceMirror = 0;
            }
            loadSpriteInfo(node, path);
            break;
        }
        case eHTMLTag::NineButton:
        {
            std::string path = domGetAttrStr(attrs, "src", "");
            node.reqWidth = domGetAttrFloat(attrs, "width", 0.f);
            node.reqHeight = domGetAttrFloat(attrs, "height", 0.f);
            node.sliceA = domGetAttrFloat(attrs, "a", 0.f);
            node.sliceB = domGetAttrFloat(attrs, "b", 0.f);
            node.sliceC = domGetAttrFloat(attrs, "c", 0.f);
            node.sliceD = domGetAttrFloat(attrs, "d", 0.f);
            node.padding = parsePaddingAttr(attrs, 8.f, 8.f);
            loadSpriteInfo(node, path);
            std::string hoverPath = domGetAttrStr(attrs, "hover", "");
            if (hoverPath.empty())
                hoverPath = domGetAttrStr(attrs, "hovsrc", "");
            if (!hoverPath.empty())
                node.hasHoverSprite = loadTextureInfo(node.hoverTex, node.hoverUV, node.hoverSrcW, node.hoverSrcH, hoverPath);
            std::string pressedPath = domGetAttrStr(attrs, "pressed", "");
            if (pressedPath.empty())
                pressedPath = domGetAttrStr(attrs, "presssrc", "");
            if (pressedPath.empty())
                pressedPath = domGetAttrStr(attrs, "down", "");
            if (!pressedPath.empty())
                node.hasPressedSprite = loadTextureInfo(node.pressedTex, node.pressedUV, node.pressedSrcW, node.pressedSrcH, pressedPath);
            node.clickable = true;
            node.btnGroup = domGetAttrStr(attrs, "btngroup", "");
            {
                std::string contentAlign = domToLower(domGetAttrStr(attrs, "contentalign", ""));
                if (!contentAlign.empty())
                {
                    size_t p = 0;
                    while (p < contentAlign.size())
                    {
                        while (p < contentAlign.size() && (isAsciiSpace((unsigned char)contentAlign[p]) || contentAlign[p] == ','))
                            ++p;
                        size_t s = p;
                        while (p < contentAlign.size() && !isAsciiSpace((unsigned char)contentAlign[p]) && contentAlign[p] != ',')
                            ++p;
                        std::string tok = contentAlign.substr(s, p - s);
                        if (tok == "left")
                            node.contentAlignH = FONS_ALIGN_LEFT;
                        else if (tok == "right")
                            node.contentAlignH = FONS_ALIGN_RIGHT;
                        else if (tok == "center")
                        {
                            node.contentAlignH = FONS_ALIGN_CENTER;
                            node.contentAlignV = eVAlign::MIDDLE;
                        }
                        else if (tok == "middle")
                            node.contentAlignV = eVAlign::MIDDLE;
                        else if (tok == "top")
                            node.contentAlignV = eVAlign::TOP;
                        else if (tok == "bottom")
                            node.contentAlignV = eVAlign::BOTTOM;
                    }
                }
            }
            break;
        }
        case eHTMLTag::Ul:
        case eHTMLTag::Li:
            break;
        case eHTMLTag::Checkbox:
        {
            node.reqWidth = domGetAttrFloat(attrs, "width", 0.f);
            node.reqHeight = domGetAttrFloat(attrs, "height", 0.f);
            node.checked = domGetAttrBool(attrs, "checked", false);
            node.padding = parsePaddingAttr(attrs, 2.f, 2.f);
            std::string uncheckedPath = domGetAttrStr(attrs, "srcu", "");
            if (!uncheckedPath.empty())
                node.hasCheckboxUncheckedSprite = loadTextureInfo(node.checkboxUncheckedTex, node.checkboxUncheckedUV,
                    node.checkboxUncheckedSrcW, node.checkboxUncheckedSrcH, uncheckedPath);
            std::string checkedPath = domGetAttrStr(attrs, "srcc", "");
            if (!checkedPath.empty())
                node.hasCheckboxCheckedSprite = loadTextureInfo(node.checkboxCheckedTex, node.checkboxCheckedUV,
                    node.checkboxCheckedSrcW, node.checkboxCheckedSrcH, checkedPath);
            node.clickable = true;
            break;
        }
        case eHTMLTag::Select:
        {
            node.clickable = true;
            node.reqWidth = domGetAttrFloat(attrs, "width", 0.f);
            node.reqHeight = domGetAttrFloat(attrs, "height", 0.f);
            node.padding = parsePaddingAttr(attrs, 4.f, 4.f);
            node.selectItemHeight = domGetAttrFloat(attrs, "itemheight", 0.f);
            node.selectItemPadY = domGetAttrFloat(attrs, "itempady", 2.f);
            node.selectIconGap = domGetAttrFloat(attrs, "icongap", 4.f);
            int dropdownRows = domGetAttrInt(attrs, "dropdownrows", 0);
            if (dropdownRows <= 0)
                dropdownRows = domGetAttrInt(attrs, "size", 0);
            if (dropdownRows <= 0)
                dropdownRows = 8;
            node.selectMaxPopupRows = dropdownRows;
            node.selectedIndex = domGetAttrInt(attrs, "selected", -1);
            node.selectFramePath = domGetAttrStr(attrs, "src", "");
            loadSpriteInfo(node, node.selectFramePath);
            node.sliceA = domGetAttrFloat(attrs, "a", 0.f);
            node.sliceB = domGetAttrFloat(attrs, "b", 0.f);
            node.sliceC = domGetAttrFloat(attrs, "c", 0.f);
            node.sliceD = domGetAttrFloat(attrs, "d", 0.f);
            node.selectBoxPath = domGetAttrStr(attrs, "boxsrc", "");
            if (!node.selectBoxPath.empty())
                loadTextureInfo(node.selectBoxTex, node.selectBoxUV, node.selectBoxSrcW, node.selectBoxSrcH, node.selectBoxPath);
            node.selectBoxA = domGetAttrFloat(attrs, "boxa", 0.f);
            node.selectBoxB = domGetAttrFloat(attrs, "boxb", 0.f);
            std::string boxOrient = domToLower(domGetAttrStr(attrs, "boxorient", "h"));
            node.selectBoxVertical = (boxOrient == "vert" || boxOrient == "v");
            std::string boxMirror = domToLower(domGetAttrStr(attrs, "boxmirror", ""));
            if (boxMirror == "right" || boxMirror == "bottom" || boxMirror == "second" || boxMirror == "2")
                node.selectBoxMirror = 1;
            else if (boxMirror == "left" || boxMirror == "top" || boxMirror == "first" || boxMirror == "1")
                node.selectBoxMirror = 2;
            else
                node.selectBoxMirror = 0;
            node.selectMarkerPath = domGetAttrStr(attrs, "markersrc", "");
            if (!node.selectMarkerPath.empty())
                loadTextureInfo(node.selectMarkerTex, node.selectMarkerUV, node.selectMarkerSrcW, node.selectMarkerSrcH, node.selectMarkerPath);
            node.selectMarkerA = domGetAttrFloat(attrs, "markera", 0.f);
            node.selectMarkerB = domGetAttrFloat(attrs, "markerb", 0.f);
            std::string markerOrient = domToLower(domGetAttrStr(attrs, "markerorient", "h"));
            node.selectMarkerVertical = (markerOrient == "vert" || markerOrient == "v");
            std::string markerMirror = domToLower(domGetAttrStr(attrs, "markermirror", ""));
            if (markerMirror == "right" || markerMirror == "bottom" || markerMirror == "second" || markerMirror == "2")
                node.selectMarkerMirror = 1;
            else if (markerMirror == "left" || markerMirror == "top" || markerMirror == "first" || markerMirror == "1")
                node.selectMarkerMirror = 2;
            else
                node.selectMarkerMirror = 0;
            node.selectIconPath = domGetAttrStr(attrs, "iconsrc", "");
            if (!node.selectIconPath.empty())
                loadTextureInfo(node.selectIconTex, node.selectIconUV, node.selectIconSrcW, node.selectIconSrcH, node.selectIconPath);
            node.selectIconW = domGetAttrFloat(attrs, "iconwidth", node.selectIconSrcW);
            node.selectIconH = domGetAttrFloat(attrs, "iconheight", node.selectIconSrcH);
            if (auto p = attrs.find("font"))
            {
                node.fontAttrs.hasFontName = true;
                node.fontAttrs.fontName = *p;
            }
            else if (auto p2 = attrs.find("name"))
            {
                node.fontAttrs.hasFontName = true;
                node.fontAttrs.fontName = *p2;
            }
            int fsz = domGetAttrInt(attrs, "fontsize", 0);
            if (fsz > 0)
            {
                node.fontAttrs.hasFontSize = true;
                node.fontAttrs.fontSize = fsz;
            }
            if (auto p = attrs.find("color"))
            {
                node.fontAttrs.hasColor = true;
                node.fontAttrs.color = parseHTMLColor(*p);
            }
            std::string hoverPath = domGetAttrStr(attrs, "hoversrc", "");
            if (hoverPath.empty())
                hoverPath = domGetAttrStr(attrs, "hover", "");
            if (!hoverPath.empty())
                node.hasHoverSprite = loadTextureInfo(node.hoverTex, node.hoverUV, node.hoverSrcW, node.hoverSrcH, hoverPath);
            std::string pressedPath = domGetAttrStr(attrs, "pressedsrc", "");
            if (pressedPath.empty())
                pressedPath = domGetAttrStr(attrs, "pressed", "");
            if (pressedPath.empty())
                pressedPath = domGetAttrStr(attrs, "down", "");
            if (!pressedPath.empty())
                node.hasPressedSprite = loadTextureInfo(node.pressedTex, node.pressedUV, node.pressedSrcW, node.pressedSrcH, pressedPath);
            break;
        }
        case eHTMLTag::Option:
        {
            node.clickable = true;
            node.selectValue = domGetAttrStr(attrs, "value", "");
            auto pSel = attrs.find("selected");
            if (pSel)
                node.optionSelected = pSel->empty() || (*pSel == "true" || *pSel == "1" || *pSel == "yes");
            else
                node.optionSelected = false;
            {
                bool dis = false;
                if (auto p = attrs.find("disabled"))
                    dis = p->empty() || *p == "true" || *p == "1" || *p == "yes";
                if (!dis)
                {
                    if (auto p = attrs.find("separator"))
                        dis = p->empty() || *p == "true" || *p == "1" || *p == "yes";
                }
                if (!dis)
                {
                    if (auto p = attrs.find("selectable"))
                        dis = (*p == "false" || *p == "0" || *p == "no");
                }
                node.optionEnabled = !dis;
            }
            if (auto p = attrs.find("font"))
            {
                node.fontAttrs.hasFontName = true;
                node.fontAttrs.fontName = *p;
            }
            else if (auto p2 = attrs.find("name"))
            {
                node.fontAttrs.hasFontName = true;
                node.fontAttrs.fontName = *p2;
            }
            int fsz = domGetAttrInt(attrs, "fontsize", 0);
            if (fsz > 0)
            {
                node.fontAttrs.hasFontSize = true;
                node.fontAttrs.fontSize = fsz;
            }
            if (auto p = attrs.find("color"))
            {
                node.fontAttrs.hasColor = true;
                node.fontAttrs.color = parseHTMLColor(*p);
            }
            break;
        }
        case eHTMLTag::Slider:
        {
            node.clickable = true;
            node.reqWidth = domGetAttrFloat(attrs, "width", 0.f);
            node.reqHeight = domGetAttrFloat(attrs, "height", 0.f);
            node.sliderMin = domGetAttrFloat(attrs, "min", 0.f);
            node.sliderMax = domGetAttrFloat(attrs, "max", 1.f);
            if (node.sliderMin > node.sliderMax)
                std::swap(node.sliderMin, node.sliderMax);
            node.sliderValue = domGetAttrFloat(attrs, "value", node.sliderMin);
            node.sliderStep = std::max(0.f, domGetAttrFloat(attrs, "step", 0.f));
            std::string orient = domToLower(domGetAttrStr(attrs, "orient", "h"));
            node.sliderVertical = (orient == "v" || orient == "vert" || orient == "vertical");
            std::string trackPath = domGetAttrStr(attrs, "tracksrc", "");
            if (trackPath.empty())
                trackPath = domGetAttrStr(attrs, "src", "");
            loadSpriteInfo(node, trackPath);
            node.sliceA = domGetAttrFloat(attrs, "tracka", domGetAttrFloat(attrs, "a", 0.f));
            node.sliceB = domGetAttrFloat(attrs, "trackb", domGetAttrFloat(attrs, "b", 0.f));
            node.sliceC = node.sliceA;
            node.sliceD = node.sliceB;
            std::string trackOrient = domToLower(domGetAttrStr(attrs, "trackorient", orient));
            node.sliceVertical = (trackOrient == "v" || trackOrient == "vert" || trackOrient == "vertical");
            std::string trackMirror = domToLower(domGetAttrStr(attrs, "trackmirror", ""));
            if (trackMirror == "right" || trackMirror == "bottom" || trackMirror == "second" || trackMirror == "2")
                node.sliceMirror = 1;
            else if (trackMirror == "left" || trackMirror == "top" || trackMirror == "first" || trackMirror == "1")
                node.sliceMirror = 2;
            else
                node.sliceMirror = 0;
            node.sliderGripPath = domGetAttrStr(attrs, "gripsrc", "");
            if (node.sliderGripPath.empty())
                node.sliderGripPath = domGetAttrStr(attrs, "grip", "");
            if (!node.sliderGripPath.empty())
                loadTextureInfo(node.sliderGripTex, node.sliderGripUV, node.sliderGripSrcW, node.sliderGripSrcH, node.sliderGripPath);
            node.sliderGripW = domGetAttrFloat(attrs, "gripwidth", node.sliderGripSrcW);
            node.sliderGripH = domGetAttrFloat(attrs, "gripheight", node.sliderGripSrcH);
            if (node.sliderGripW <= 0.f)
                node.sliderGripW = 16.f;
            if (node.sliderGripH <= 0.f)
                node.sliderGripH = 16.f;
            float trackSize = -1.f;
            if (node.sliderVertical && domHasAttr(attrs, "trackwidth"))
                trackSize = domGetAttrFloat(attrs, "trackwidth", -1.f);
            if (trackSize < 0.f && domHasAttr(attrs, "trackheight"))
                trackSize = domGetAttrFloat(attrs, "trackheight", -1.f);
            if (trackSize < 0.f && domHasAttr(attrs, "trackthickness"))
                trackSize = domGetAttrFloat(attrs, "trackthickness", -1.f);
            if (trackSize < 0.f && domHasAttr(attrs, "barheight"))
                trackSize = domGetAttrFloat(attrs, "barheight", -1.f);
            if (trackSize < 0.f && domHasAttr(attrs, "thickness"))
                trackSize = domGetAttrFloat(attrs, "thickness", -1.f);
            if (trackSize < 0.f)
            {
                if (node.sliderVertical)
                {
                    if (node.reqWidth > 0.f)
                        trackSize = node.reqWidth;
                }
                else
                {
                    if (node.reqHeight > 0.f)
                        trackSize = node.reqHeight;
                }
            }
            if (trackSize < 0.f && node.spriteTex)
                trackSize = node.sliderVertical ? node.spriteSrcW : node.spriteSrcH;
            if (trackSize <= 0.f)
                trackSize = 6.f;
            node.sliderTrackThickness = std::max(1.f, trackSize);
            std::string ticks = domToLower(domGetAttrStr(attrs, "ticks", ""));
            if (ticks == "none" || ticks == "0" || ticks == "false" || ticks == "off")
            {
                node.sliderShowTicks = false;
                node.sliderTicksAuto = false;
            }
            else if (!ticks.empty())
            {
                node.sliderShowTicks = true;
                node.sliderTicksAuto = (ticks == "auto");
            }
            int major = domGetAttrInt(attrs, "majorcount", 0);
            if (major <= 0)
                major = domGetAttrInt(attrs, "major", 0);
            if (major <= 0)
                major = domGetAttrInt(attrs, "majors", 0);
            if (major > 0)
            {
                node.sliderShowTicks = true;
                node.sliderTicksAuto = false;
                node.sliderMajorCount = std::max(1, major);
            }
            int minor = domGetAttrInt(attrs, "minorcount", -1);
            if (minor < 0)
                minor = domGetAttrInt(attrs, "minor", -1);
            if (minor < 0)
                minor = domGetAttrInt(attrs, "minors", -1);
            if (minor >= 0)
            {
                node.sliderMinorCount = minor;
                if (minor > 0)
                    node.sliderShowTicks = true;
            }
            if (node.sliderMajorCount < 1)
                node.sliderMajorCount = 10;
            if (node.sliderMinorCount < 0)
                node.sliderMinorCount = 4;
            std::string snap = domToLower(domGetAttrStr(attrs, "snaptoticks", ""));
            if (snap.empty())
                snap = domToLower(domGetAttrStr(attrs, "snap", ""));
            if (snap.empty())
                node.sliderSnapToMajorTicks = node.sliderShowTicks;
            else
                node.sliderSnapToMajorTicks = !(snap == "none" || snap == "off" || snap == "false" || snap == "0");
            bool hasTrackTex = node.spriteTex ? true : false;
            if (auto p = attrs.find("fill"))
                node.sliderFill = (*p == "true" || *p == "1" || *p == "yes" || *p == "on");
            else
                node.sliderFill = !hasTrackTex;
            node.sliderTrackColor = parseHTMLColor(domGetAttrStr(attrs, "trackcolor", "#808080FF"));
            node.sliderFillColor = parseHTMLColor(domGetAttrStr(attrs, "fillcolor", "#4DA6FFFF"));
            node.sliderGripColor = parseHTMLColor(domGetAttrStr(attrs, "gripcolor", "#FFFFFFFF"));
            node.sliderTickColor = parseHTMLColor(domGetAttrStr(attrs, "tickcolor", "#FFFFFFFF"));
            node.sliderMinorTickColor = parseHTMLColor(domGetAttrStr(attrs, "minortickcolor", "#808080FF"));
            node.sliderTickLength = std::max(0.f, domGetAttrFloat(attrs, "ticklength", 0.f));
            node.sliderMinorTickLength = std::max(0.f, domGetAttrFloat(attrs, "minorticklength", 0.f));
            node.sliderTickThickness = std::max(0.f, domGetAttrFloat(attrs, "tickthickness", 0.f));
            node.sliderTickOffset = domGetAttrFloat(attrs, "tickoffset", 0.f);
            node.sliderFillPath = domGetAttrStr(attrs, "fillsrc", "");
            if (!node.sliderFillPath.empty())
                loadTextureInfo(node.sliderFillTex, node.sliderFillUV, node.sliderFillSrcW, node.sliderFillSrcH, node.sliderFillPath);
            else
                node.sliderFillTex = CTexturePtr();
            std::string fillType = domToLower(domGetAttrStr(attrs, "filltype", ""));
            if (fillType.empty())
                fillType = domToLower(domGetAttrStr(attrs, "fillslices", ""));
            node.sliderFillThree = (fillType == "three" || fillType == "3" || fillType == "threeslice");
            node.sliderFillA = domGetAttrFloat(attrs, "filla", node.sliceA);
            node.sliderFillB = domGetAttrFloat(attrs, "fillb", node.sliceB);
            node.sliderFillC = domGetAttrFloat(attrs, "fillc", node.sliceC);
            node.sliderFillD = domGetAttrFloat(attrs, "filld", node.sliceD);
            std::string fillOrient = domToLower(domGetAttrStr(attrs, "fillorient", orient));
            node.sliderFillVertical = (fillOrient == "v" || fillOrient == "vert" || fillOrient == "vertical");
            std::string fillMirror = domToLower(domGetAttrStr(attrs, "fillmirror", ""));
            if (fillMirror == "right" || fillMirror == "bottom" || fillMirror == "second" || fillMirror == "2")
                node.sliderFillMirror = 1;
            else if (fillMirror == "left" || fillMirror == "top" || fillMirror == "first" || fillMirror == "1")
                node.sliderFillMirror = 2;
            else
                node.sliderFillMirror = 0;
            node.sliderValue = clampSliderVal(node, node.sliderValue);
            break;
        }
        case eHTMLTag::ProgressBar:
        {
            node.clickable = false;
            node.reqWidth = domGetAttrFloat(attrs, "width", 0.f);
            node.reqHeight = domGetAttrFloat(attrs, "height", 0.f);
            node.padding = parsePaddingAttr(attrs, 2.f, 2.f);
            std::string orient = domToLower(domGetAttrStr(attrs, "orient", ""));
            if (orient.empty())
                orient = domToLower(domGetAttrStr(attrs, "orientation", "h"));
            node.progressVertical = parseProgressVertical(orient);
            node.progressT = parseProgressNormalized(attrs);
            node.progressFramePath = domGetAttrStr(attrs, "framesrc", "");
            if (node.progressFramePath.empty())
                node.progressFramePath = domGetAttrStr(attrs, "src", "");
            loadTextureInfo(node.progressFrameTex, node.progressFrameUV, node.progressFrameSrcW, node.progressFrameSrcH, node.progressFramePath);
            std::string frameType = domToLower(domGetAttrStr(attrs, "frametype", "three"));
            node.progressFrameThree = !(frameType == "nine" || frameType == "9" || frameType == "nineslice");
            node.progressFrameA = domGetAttrFloat(attrs, "framea", domGetAttrFloat(attrs, "a", 0.f));
            node.progressFrameB = domGetAttrFloat(attrs, "frameb", domGetAttrFloat(attrs, "b", 0.f));
            node.progressFrameC = domGetAttrFloat(attrs, "framec", domGetAttrFloat(attrs, "c", 0.f));
            node.progressFrameD = domGetAttrFloat(attrs, "framed", domGetAttrFloat(attrs, "d", 0.f));
            node.progressFrameVertical = parseProgressVertical(domToLower(domGetAttrStr(attrs, "frameorient", orient)));
            node.progressFrameMirror = parseProgressMirror(domToLower(domGetAttrStr(attrs, "framemirror", "")));
            node.progressFillPath = domGetAttrStr(attrs, "fillsrc", "");
            if (node.progressFillPath.empty())
                node.progressFillPath = domGetAttrStr(attrs, "barsrc", "");
            loadTextureInfo(node.progressFillTex, node.progressFillUV, node.progressFillSrcW, node.progressFillSrcH, node.progressFillPath);
            std::string fillType = domToLower(domGetAttrStr(attrs, "filltype", "three"));
            node.progressFillThree = !(fillType == "nine" || fillType == "9" || fillType == "nineslice");
            node.progressFillA = domGetAttrFloat(attrs, "filla", domGetAttrFloat(attrs, "bara", node.progressFrameA));
            node.progressFillB = domGetAttrFloat(attrs, "fillb", domGetAttrFloat(attrs, "barb", node.progressFrameB));
            node.progressFillC = domGetAttrFloat(attrs, "fillc", domGetAttrFloat(attrs, "barc", node.progressFrameC));
            node.progressFillD = domGetAttrFloat(attrs, "filld", domGetAttrFloat(attrs, "bard", node.progressFrameD));
            node.progressFillVertical = parseProgressVertical(domToLower(domGetAttrStr(attrs, "fillorient", orient)));
            node.progressFillMirror = parseProgressMirror(domToLower(domGetAttrStr(attrs, "fillmirror", "")));
            if (auto p = attrs.find("backcolor"))
            {
                node.progressHasBackColor = true;
                node.progressBackColor = parseHTMLColor(*p);
            }
            else if (auto p = attrs.find("bgcolor"))
            {
                node.progressHasBackColor = true;
                node.progressBackColor = parseHTMLColor(*p);
            }
            else
            {
                node.progressHasBackColor = false;
                node.progressBackColor = 0x000000AA;
            }
            if (auto p = attrs.find("fillcolor"))
                node.progressFillColor = parseHTMLColor(*p);
            else if (auto p = attrs.find("progresscolor"))
                node.progressFillColor = parseHTMLColor(*p);
            else if (auto p = attrs.find("barcolor"))
                node.progressFillColor = parseHTMLColor(*p);
            else if (auto p = attrs.find("color"))
                node.progressFillColor = parseHTMLColor(*p);
            else
                node.progressFillColor = 0x4DA6FFFF;
            if (auto p = attrs.find("bordercolor"))
                node.progressBorderColor = parseHTMLColor(*p);
            else if (auto p = attrs.find("framecolor"))
                node.progressBorderColor = parseHTMLColor(*p);
            else
                node.progressBorderColor = 0xFFFFFFFF;
            if (domHasAttr(attrs, "border"))
                node.progressBorder = std::max(0.f, domGetAttrFloat(attrs, "border", 2.f));
            else if (domHasAttr(attrs, "borderwidth"))
                node.progressBorder = std::max(0.f, domGetAttrFloat(attrs, "borderwidth", 2.f));
            else
                node.progressBorder = -1.f;
            break;
        }
#ifdef HAS_SPINE
        case eHTMLTag::Spine:
        {
            node.spinePath = domGetAttrStr(attrs, "src", "");
            if (node.spinePath.empty())
                node.spinePath = domGetAttrStr(attrs, "path", "");
            node.spineAnim = domGetAttrStr(attrs, "anim", "");
            if (node.spineAnim.empty())
                node.spineAnim = domGetAttrStr(attrs, "animation", "");
            node.spineLoop = domGetAttrBool(attrs, "loop", true);
            node.spineDelay = domGetAttrFloat(attrs, "delay", 0.f);
            node.reqWidth = domGetAttrFloat(attrs, "width", 0.f);
            node.reqHeight = domGetAttrFloat(attrs, "height", 0.f);
            node.keepAspect = domGetAttrBool(attrs, "keepaspect", false);
            node.padding = parsePaddingAttr(attrs, 0.f, 0.f);
            std::string rcbox = domGetAttrStr(attrs, "rcbox", "");
            node.spineHasRcBox = false;
            if (!rcbox.empty())
                node.spineHasRcBox = parseRcBox(rcbox, node.spineRc);
            if (!node.spineHasRcBox && node.reqWidth > 0.f && node.reqHeight > 0.f)
            {
                node.spineRc[0] = 0.f;
                node.spineRc[1] = 0.f;
                node.spineRc[2] = node.reqWidth;
                node.spineRc[3] = node.reqHeight;
                node.spineHasRcBox = true;
            }
            if (node.spineId == 0 && node.id != 0)
                node.spineId = node.id;
            break;
        }
#endif
        case eHTMLTag::Table:
        {
            HTMLDomTableConfig cfg;
            if (auto p = attrs.find("padding"))
                cfg.cellPadding = std::stof(*p);
            else
                cfg.cellPadding = 25.f;
            cfg.defaultCellPadding = parsePaddingAttr(attrs, 0.f, 0.f);
            if (auto p = attrs.find("cellspacing"))
                cfg.cellSpacing = std::stof(*p);
            if (auto p = attrs.find("align"))
                cfg.defaultAlign = parseAlignValue(*p, FONS_ALIGN_LEFT);
            if (auto p = attrs.find("valign"))
                cfg.defaultVAlign = parseVAlign(*p, eVAlign::TOP);
            if (auto p = attrs.find("border"))
                cfg.border = std::stof(*p);
            if (auto p = attrs.find("bordercolor"))
                cfg.borderColor = parseHTMLColor(*p);
            if (auto p = attrs.find("grid"))
                cfg.gridStyle = parseGridStyleValue(*p);
            if (auto p = attrs.find("gridwidth"))
                cfg.grid = std::stof(*p);
            if (auto p = attrs.find("gridcolor"))
                cfg.gridColor = parseHTMLColor(*p);
            if (auto p = attrs.find("maxwidth"))
                cfg.maxWidth = std::stof(*p);
            node.tableConfig = cfg;
            break;
        }
        case eHTMLTag::Td:
        {
            if (auto p = attrs.find("bgcolor"))
                node.bgColor = parseHTMLColor(*p);
            if (auto p = attrs.find("align"))
            {
                node.hasAlign = true;
                node.cellAlign = parseAlignValue(*p, FONS_ALIGN_LEFT);
            }
            if (auto p = attrs.find("valign"))
            {
                node.hasVAlign = true;
                node.cellVAlign = parseVAlign(*p, eVAlign::TOP);
            }
            if (auto p = attrs.find("maxwidth"))
            {
                node.cellMaxWidth = std::stof(*p);
                if (node.cellMaxWidth < 0.f)
                    node.cellMaxWidth = 0.f;
            }
            node.hasOwnPadding =
                domHasAttr(attrs, "padding") ||
                domHasAttr(attrs, "padding-left") ||
                domHasAttr(attrs, "padding-top") ||
                domHasAttr(attrs, "padding-right") ||
                domHasAttr(attrs, "padding-bottom");
            if (node.hasOwnPadding)
                node.cellPadding = parsePaddingAttr(attrs, 0.f, 0.f);
            break;
        }
        case eHTMLTag::Edit:
        {
            node.clickable = true;
            node.reqWidth = domGetAttrFloat(attrs, "width", 0.f);
            node.reqHeight = domGetAttrFloat(attrs, "height", 0.f);
            node.padding = parsePaddingAttr(attrs, 6.f, 4.f);
            node.sliceA = domGetAttrFloat(attrs, "a", 0.f);
            node.sliceB = domGetAttrFloat(attrs, "b", 0.f);
            node.sliceC = domGetAttrFloat(attrs, "c", 0.f);
            node.sliceD = domGetAttrFloat(attrs, "d", 0.f);
            loadSpriteInfo(node, domGetAttrStr(attrs, "src", ""));
            node.editPassword = domGetAttrBool(attrs, "password", false);
            node.editMaxLen = domGetAttrInt(attrs, "maxlength", 0);
            node.editPlaceholder = domGetAttrStr(attrs, "placeholder", "");
            node.editPlaceholderColor = parseHTMLColor(domGetAttrStr(attrs, "placeholdercolor", "#808080FF"));
            if (auto p = attrs.find("font"))
            {
                node.fontAttrs.hasFontName = true;
                node.fontAttrs.fontName = *p;
            }
            else if (auto p2 = attrs.find("name"))
            {
                node.fontAttrs.hasFontName = true;
                node.fontAttrs.fontName = *p2;
            }
            int fsz = domGetAttrInt(attrs, "fontsize", 0);
            if (fsz <= 0)
                fsz = domGetAttrInt(attrs, "size", 0);
            if (fsz > 0)
            {
                node.fontAttrs.hasFontSize = true;
                node.fontAttrs.fontSize = fsz;
            }
            if (auto p = attrs.find("color"))
            {
                node.fontAttrs.hasColor = true;
                node.fontAttrs.color = parseHTMLColor(*p);
            }
            break;
        }
        case eHTMLTag::Span:
        {
            if (auto p = attrs.find("color"))
            {
                node.fontAttrs.hasColor = true;
                node.fontAttrs.color = parseHTMLColor(*p);
            }
            if (auto p = attrs.find("bgcolor"))
                node.bgColor = parseHTMLColor(*p);
            if (auto p = attrs.find("size"))
            {
                node.fontAttrs.hasFontSize = true;
                node.fontAttrs.fontSize = std::stoi(*p);
            }
            if (auto p = attrs.find("name"))
            {
                node.fontAttrs.hasFontName = true;
                node.fontAttrs.fontName = *p;
            }
            node.spanBold = domGetAttrBool(attrs, "bold", false);
            node.spanItalic = domGetAttrBool(attrs, "italic", false);
            break;
        }
        case eHTMLTag::Font:
            parseFontAttrs(attrs, node.fontAttrs);
            break;
        case eHTMLTag::U:
        case eHTMLTag::S:
        {
            if (auto p = attrs.find("color"))
            {
                node.lineHasColor = true;
                node.lineColor = parseHTMLColor(*p);
            }
            if (auto p = attrs.find("thickness"))
                node.lineThickness = std::stof(*p);
            break;
        }
        case eHTMLTag::A:
            node.clickable = true;
            break;
        default:
            break;
        }
        node.margin = parseMarginAttr(attrs);
        node.positionRelative = domGetAttrStr(attrs, "position", "") == "relative";
        node.relX = domGetAttrFloat(attrs, "left", 0.f);
        node.relY = domGetAttrFloat(attrs, "top", 0.f);
        if (auto p = attrs.find("hovercolor"))
        {
            node.hasHoverColor = true;
            node.hoverColor = parseHTMLColor(*p);
        }
        else
        {
            node.hasHoverColor = false;
            node.hoverColor = 0xFFFFFFFF;
        }
        if (auto p = attrs.find("pressedcolor"))
        {
            node.hasPressedColor = true;
            node.pressedColor = parseHTMLColor(*p);
        }
        else
        {
            node.hasPressedColor = false;
            node.pressedColor = 0xFFFFFFFF;
        }
        node.visible = domGetAttrBool(attrs, "visible", true);
        if (auto p = attrs.find("display"))
        {
            if (*p == "none")
                node.visible = false;
        }
        parseDisabledAttrs(node);
        updateEffectiveCursor(node);
    }

    int domFindFirstContent(const std::vector<HTMLDom::NodePtr>& children)
    {
        for (int i = 0; i < (int)children.size(); ++i)
        {
            const auto& n = children[i];
            if (!n)
                continue;
            if (n->tag == eHTMLTag::Text)
            {
                if (domHasNonRemovableWhitespace(n->text))
                    return i;
            }
            else
            {
                return i;
            }
        }
        return -1;
    }

    int domFindLastContent(const std::vector<HTMLDom::NodePtr>& children)
    {
        for (int i = (int)children.size() - 1; i >= 0; --i)
        {
            const auto& n = children[i];
            if (!n)
                continue;
            if (n->tag == eHTMLTag::Text)
            {
                if (domHasNonRemovableWhitespace(n->text))
                    return i;
            }
            else
            {
                return i;
            }
        }
        return -1;
    }

    void domTrimNode(HTMLDomNode* node)
    {
        if (!node)
            return;
        if (node->tag == eHTMLTag::Text)
            domCollapseAsciiWhiteSpacePreserveNbsp(node->text);
        for (auto& child : node->children)
            domTrimNode(child.get());
        for (int i = (int)node->children.size() - 1; i >= 0; --i)
        {
            auto& child = node->children[i];
            if (child && child->tag == eHTMLTag::Text && child->text.empty())
                node->children.erase(node->children.begin() + i);
        }
        bool changed = true;
        while (changed)
        {
            changed = false;
            int first = domFindFirstContent(node->children);
            if (first < 0)
            {
                for (int i = (int)node->children.size() - 1; i >= 0; --i)
                {
                    auto& child = node->children[i];
                    if (child && child->tag == eHTMLTag::Text && domStringIsRemovableWhitespace(child->text))
                    {
                        node->children.erase(node->children.begin() + i);
                        changed = true;
                    }
                }
                break;
            }
            for (int i = first - 1; i >= 0; --i)
            {
                auto& child = node->children[i];
                if (child && child->tag == eHTMLTag::Text && domStringIsRemovableWhitespace(child->text))
                {
                    node->children.erase(node->children.begin() + i);
                    changed = true;
                }
            }
            first = domFindFirstContent(node->children);
            if (first >= 0 && node->children[first]->tag == eHTMLTag::Text)
            {
                auto& t = node->children[first]->text;
                size_t oldSize = t.size();
                domTrimLeftPreserveNbsp(t);
                if (t.size() != oldSize)
                    changed = true;
                if (domStringIsRemovableWhitespace(t))
                {
                    node->children.erase(node->children.begin() + first);
                    changed = true;
                }
            }
        }
        changed = true;
        while (changed)
        {
            changed = false;
            int last = domFindLastContent(node->children);
            if (last < 0)
            {
                for (int i = (int)node->children.size() - 1; i >= 0; --i)
                {
                    auto& child = node->children[i];
                    if (child && child->tag == eHTMLTag::Text && domStringIsRemovableWhitespace(child->text))
                    {
                        node->children.erase(node->children.begin() + i);
                        changed = true;
                    }
                }
                break;
            }
            for (int i = (int)node->children.size() - 1; i > last; --i)
            {
                auto& child = node->children[i];
                if (child && child->tag == eHTMLTag::Text && domStringIsRemovableWhitespace(child->text))
                {
                    node->children.erase(node->children.begin() + i);
                    changed = true;
                }
            }
            last = domFindLastContent(node->children);
            if (last >= 0 && node->children[last]->tag == eHTMLTag::Text)
            {
                auto& t = node->children[last]->text;
                size_t oldSize = t.size();
                domTrimRightPreserveNbsp(t);
                if (t.size() != oldSize)
                    changed = true;
                if (domStringIsRemovableWhitespace(t))
                {
                    node->children.erase(node->children.begin() + last);
                    changed = true;
                }
            }
        }
    }

    void domRefreshFontState(HTMLRenderState& st)
    {
        auto tr = TextRender::getInstance();
        FONScontext* fs = domFs();
        int base = (st.baseFontHandle != -1) ? st.baseFontHandle : st.fontHandle;
        int boldHandle = -1;
        int italicHandle = -1;
        if (!st.fontName.empty())
        {
            if (st.bold)
                boldHandle = tr->findBoldFontHandle(st.fontName.c_str());
            if (st.italic)
                italicHandle = tr->findItalicFontHandle(st.fontName.c_str());
        }
        int selected = base;
        if (st.bold && boldHandle != -1)
            selected = boldHandle;
        else if (st.italic && italicHandle != -1)
            selected = italicHandle;
        else if (st.bold && italicHandle != -1)
            selected = italicHandle;
        else if (st.italic && boldHandle != -1)
            selected = boldHandle;
        if (selected < 0)
            selected = 0;
        bool realBoldUsed = (st.bold && boldHandle != -1 && selected == boldHandle);
        bool realItalicUsed = (st.italic && italicHandle != -1 && selected == italicHandle);
        st.fakeBold = st.bold && !realBoldUsed;
        st.fakeItalic = st.italic && !realItalicUsed;
        st.fontHandle = selected;
        fonsSetFont(fs, st.fontHandle);
        fonsSetSize(fs, st.fontSize);
        fonsSetColor(fs, st.color);
        fonsSetAlign(fs, st.align);
    }

    float domGetLineHeightForState(const HTMLRenderState& cst)
    {
        HTMLRenderState& st = const_cast<HTMLRenderState&>(cst);
        domRefreshFontState(st);
        float lh = TextRender::getInstance()->getLineHeight();
        if (st.lineHeightMul > 0.f)
            lh *= st.lineHeightMul;
        return lh;
    }

    HTMLRenderState domMakeDefaultState(unsigned int color, int align)
    {
        auto tr = TextRender::getInstance();
        HTMLRenderState st;
        st.fontName = Engine::getCfg().DEFAULT_FONT_NAME;
        st.baseFontHandle = tr->getFontHandle(st.fontName.c_str());
        st.fontHandle = st.baseFontHandle;
        st.fontSize = Engine::getCfg().DEFAULT_FONT_SIZE;
        st.color = color;
        st.align = align | FONS_ALIGN_TOP;
        st.fxStyle = eFxStyle::NONE;
        st.shadow = 0.f;
        st.shadowColor = 0x000000FF;
        st.outline = 0.f;
        st.outlineColor = 0x000000FF;
        st.underline = false;
        st.strikethrough = false;
        st.lineColor = 0xFFFFFFFF;
        st.lineThickness = 0.f;
        st.hasGradient = false;
        st.gradientColor1 = 0xFFFFFFFF;
        st.gradientColor2 = 0xFFFFFFFF;
        st.appearStyle = eAppearStyle::NONE;
        st.appearDur = 0.8f;
        st.appearDelay = 0.f;
        st.fxDelay = 0.f;
        st.bold = false;
        st.italic = false;
        st.fakeBold = false;
        st.fakeItalic = false;
        domRefreshFontState(st);
        return st;
    }

    float domMeasureStyledTextRange(const char* str, int len, const HTMLRenderState& st)
    {
        if (!str || len <= 0)
            return 0.f;
        FONScontext* fs = domFs();
        fonsSetFont(fs, st.fontHandle);
        fonsSetSize(fs, st.fontSize);
        fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
        float w = fonsTextBounds(fs, 0.f, 0.f, str, str + len, nullptr);
        if (st.fakeBold)
        {
            int chars = 0;
            const char* p = str;
            const char* end = str + len;
            while (p < end)
            {
                p += getUtf8CharLen(p);
                chars++;
            }
            w += domFakeBoldOffset(st.fontSize) * (float)chars;
        }
        return w;
    }

    float domMeasureStyledText(const std::string& s, const HTMLRenderState& st)
    {
        return domMeasureStyledTextRange(s.c_str(), (int)s.size(), st);
    }

    void applyFontAttrsToState(HTMLRenderState& st, const HTMLFontAttrs& a)
    {
        auto tr = TextRender::getInstance();
        if (a.hasFontName)
        {
            st.fontName = a.fontName;
            st.baseFontHandle = tr->getFontHandle(a.fontName.c_str());
        }
        if (a.hasFontSize)
            st.fontSize = a.fontSize;
        if (a.hasColor)
            st.color = a.color;
        if (a.hasFx)
            st.fxStyle = a.fxStyle;
        if (a.hasShadow)
            st.shadow = a.shadow;
        if (a.hasShadowColor)
            st.shadowColor = a.shadowColor;
        if (a.hasOutline)
            st.outline = a.outline;
        if (a.hasOutlineColor)
            st.outlineColor = a.outlineColor;
        if (a.hasGradient)
        {
            st.hasGradient = true;
            st.gradientColor1 = a.gradientColor1;
            st.gradientColor2 = a.gradientColor2;
        }
        if (a.hasAppear)
            st.appearStyle = a.appearStyle;
        if (a.hasAppearDur)
            st.appearDur = a.appearDur;
        if (a.hasAppearDelay)
            st.appearDelay = a.appearDelay;
        if (a.hasFxDelay)
            st.fxDelay = a.fxDelay;
        if (a.hasLineHeight)
            st.lineHeightMul = a.lineHeightMul;
    }

    void applyInlineNodeToState(const HTMLDomNode* node, HTMLRenderState& st)
    {
        if (!node)
            return;
        switch (node->tag)
        {
        case eHTMLTag::Font:
            applyFontAttrsToState(st, node->fontAttrs);
            break;
        case eHTMLTag::B:
        case eHTMLTag::Strong:
            st.bold = true;
            break;
        case eHTMLTag::I:
        case eHTMLTag::Em:
            st.italic = true;
            break;
        case eHTMLTag::U:
            st.underline = true;
            if (node->lineHasColor)
                st.lineColor = node->lineColor;
            if (node->lineThickness > 0.f)
                st.lineThickness = node->lineThickness;
            break;
        case eHTMLTag::S:
            st.strikethrough = true;
            if (node->lineHasColor)
                st.lineColor = node->lineColor;
            if (node->lineThickness > 0.f)
                st.lineThickness = node->lineThickness;
            break;
        case eHTMLTag::Span:
            applyFontAttrsToState(st, node->fontAttrs);
            if (node->spanBold)
                st.bold = true;
            if (node->spanItalic)
                st.italic = true;
            if (node->bgColor != 0)
                st.bgColor = node->bgColor;
            break;
        case eHTMLTag::Center:
            st.align = FONS_ALIGN_CENTER | FONS_ALIGN_TOP;
            break;
        default:
            break;
        }
        domRefreshFontState(st);
    }

    struct HTMLStateScope
    {
        HTMLRenderState& st;
        HTMLRenderScratch& sc;
        int depth;
        int baseFontHandle, fontHandle, fontSize, align;
        unsigned int color, shadowColor, outlineColor, lineColor, gradientColor1, gradientColor2, bgColor;
        float shadow, outline, lineThickness, appearDur, appearDelay, fxDelay, lineHeightMul;
        eFxStyle fxStyle;
        eAppearStyle appearStyle;
        bool underline, strikethrough, hasGradient, bold, italic, fakeBold, fakeItalic;

        HTMLStateScope(HTMLRenderState& s, HTMLRenderScratch& scratch)
            : st(s), sc(scratch)
        {
            depth = sc.snapDepth++;
            if ((int)sc.snapFont.size() <= depth)
                sc.snapFont.resize(depth + 1);
            sc.snapFont[depth] = st.fontName;
            baseFontHandle = st.baseFontHandle;
            fontHandle = st.fontHandle;
            fontSize = st.fontSize;
            align = st.align;
            color = st.color;
            shadowColor = st.shadowColor;
            outlineColor = st.outlineColor;
            lineColor = st.lineColor;
            gradientColor1 = st.gradientColor1;
            gradientColor2 = st.gradientColor2;
            bgColor = st.bgColor;
            shadow = st.shadow;
            outline = st.outline;
            lineThickness = st.lineThickness;
            appearDur = st.appearDur;
            appearDelay = st.appearDelay;
            fxDelay = st.fxDelay;
            lineHeightMul = st.lineHeightMul;
            fxStyle = st.fxStyle;
            appearStyle = st.appearStyle;
            underline = st.underline;
            strikethrough = st.strikethrough;
            hasGradient = st.hasGradient;
            bold = st.bold;
            italic = st.italic;
            fakeBold = st.fakeBold;
            fakeItalic = st.fakeItalic;
        }

        ~HTMLStateScope()
        {
            st.fontName = sc.snapFont[depth];
            st.baseFontHandle = baseFontHandle;
            st.fontHandle = fontHandle;
            st.fontSize = fontSize;
            st.align = align;
            st.color = color;
            st.shadowColor = shadowColor;
            st.outlineColor = outlineColor;
            st.lineColor = lineColor;
            st.gradientColor1 = gradientColor1;
            st.gradientColor2 = gradientColor2;
            st.bgColor = bgColor;
            st.shadow = shadow;
            st.outline = outline;
            st.lineThickness = lineThickness;
            st.appearDur = appearDur;
            st.appearDelay = appearDelay;
            st.fxDelay = fxDelay;
            st.lineHeightMul = lineHeightMul;
            st.fxStyle = fxStyle;
            st.appearStyle = appearStyle;
            st.underline = underline;
            st.strikethrough = strikethrough;
            st.hasGradient = hasGradient;
            st.bold = bold;
            st.italic = italic;
            st.fakeBold = fakeBold;
            st.fakeItalic = fakeItalic;
            sc.snapDepth = depth;
            domRefreshFontState(st);
        }
    };

    void domGetEditSelectionRange(const HTMLDomNode& node, int& start, int& end)
    {
        int a = std::clamp(node.editSelAnchor, 0, (int)node.editValue.size());
        int c = std::clamp(node.editCursor, 0, (int)node.editValue.size());
        start = std::min(a, c);
        end = std::max(a, c);
    }

    bool domHasEditSelection(const HTMLDomNode& node)
    {
        int s, e;
        domGetEditSelectionRange(node, s, e);
        return s < e;
    }

    float domMeasureEditPrefix(const HTMLDomNode& node, const HTMLRenderState& st, int bytePos)
    {
        bytePos = std::clamp(bytePos, 0, (int)node.editValue.size());
        if (bytePos <= 0)
            return 0.f;
        if (node.editPassword)
        {
            int chars = 0;
            const char* p = node.editValue.c_str();
            int i = 0;
            while (i < bytePos)
            {
                i += getUtf8CharLen(p + i);
                ++chars;
            }
            return domMeasureStyledTextRange("*", 1, st) * (float)chars;
        }
        return domMeasureStyledTextRange(node.editValue.c_str(), bytePos, st);
    }

    HTMLRenderState domMakeEditMeasureState(HTMLDom* dom, const HTMLDom::NodePtr& node)
    {
        HTMLRenderState st = domMakeDefaultState(0xFFFFFFFF, FONS_ALIGN_LEFT);
        if (!dom || !node)
            return st;
        std::vector<HTMLDom::NodePtr> chain;
        unsigned long long pid = node->parentId;
        while (pid != 0)
        {
            auto p = dom->getNode(pid);
            if (!p)
                break;
            chain.push_back(p);
            pid = p->parentId;
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it)
            applyInlineNodeToState(it->get(), st);
        applyFontAttrsToState(st, node->fontAttrs);
        st.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
        domRefreshFontState(st);
        return st;
    }

    int domEditPosFromX(const std::string& v, const HTMLRenderState& st, float clickX)
    {
        float acc = 0.f;
        int bestPos = 0;
        float bestDist = std::fabs(clickX);
        int i = 0;
        while (i < (int)v.size())
        {
            int cl = getUtf8CharLen(v.c_str() + i);
            float cw = domMeasureStyledTextRange(v.c_str() + i, cl, st);
            acc += cw;
            float d = std::fabs(acc - clickX);
            if (d < bestDist)
            {
                bestDist = d;
                bestPos = i + cl;
            }
            i += cl;
        }
        return bestPos;
    }

    int domEditPosFromPointer(const HTMLDom::NodePtr& node, float globalX, const HTMLRenderState& st)
    {
        if (!node)
            return 0;
        const Rect& rc = node->editLocalRect;
        const std::string& v = node->editValue;
        const float clickX = globalX - rc.x - node->padding.left + node->editScrollX;
        float starW = 0.f;
        if (node->editPassword)
            starW = domMeasureStyledTextRange("*", 1, st);
        float acc = 0.f;
        int bestPos = 0;
        float bestDist = std::fabs(clickX);
        int i = 0;
        while (i < (int)v.size())
        {
            int cl = getUtf8CharLen(v.c_str() + i);
            float cw = node->editPassword ? starW : domMeasureStyledTextRange(v.c_str() + i, cl, st);
            acc += cw;
            float d = std::fabs(acc - clickX);
            if (d < bestDist)
            {
                bestDist = d;
                bestPos = i + cl;
            }
            i += cl;
        }
        return bestPos;
    }

    HTMLNode makeHTMLNodeForIdle(const HTMLDomNode& node)
    {
        HTMLNode hn;
        hn.spriteTex = node.spriteTex;
        hn.spriteUV[0] = node.spriteUV[0];
        hn.spriteUV[1] = node.spriteUV[1];
        hn.spriteUV[2] = node.spriteUV[2];
        hn.spriteUV[3] = node.spriteUV[3];
        hn.spriteSrcW = node.spriteSrcW;
        hn.spriteSrcH = node.spriteSrcH;
        hn.idleDelay = node.idleDelay;
        hn.idleDur = node.idleDur;
        hn.glareWidth = node.glareWidth;
        hn.glareSpeed = node.glareSpeed;
        hn.glareTilt = node.glareTilt;
        hn.glareDir = node.glareDir;
        hn.glareIntensity = node.glareIntensity;
        hn.pxBlock = node.pxBlock;
        hn.pxBand = node.pxBand;
        hn.pxSpeed = node.pxSpeed;
        hn.pxGlow = node.pxGlow;
        return hn;
    }

    bool rectContains(const Rect& rc, float x, float y)
    {
        return x >= rc.x && y >= rc.y && x < rc.x + rc.cx && y < rc.y + rc.cy;
    }

    float buttonApproach(float cur, float target, float dur, float dt)
    {
        if (dur <= 0.f)
            return target;
        if (dt <= 0.f)
            return cur;
        float step = dt / dur;
        if (cur < target)
            return std::min(target, cur + step);
        return std::max(target, cur - step);
    }

    void drawSpriteQuadAlpha(const CTexturePtr& tex, float x, float y, float w, float h,
        float u0, float v0, float u1, float v1, float alpha, bool bGrayScale = false, bool bSnap = true)
    {
        if (!tex || alpha <= 0.f || w <= 0.f || h <= 0.f)
            return;
        if (bSnap)
            snapPointToBuffer(x, y);
        alpha = std::clamp(alpha, 0.f, 1.f);
        float verts[8];
        verts[CSprite::VERT_ULX] = x;
        verts[CSprite::VERT_ULY] = y;
        verts[CSprite::VERT_URX] = x + w;
        verts[CSprite::VERT_URY] = y;
        verts[CSprite::VERT_BLX] = x;
        verts[CSprite::VERT_BLY] = y + h;
        verts[CSprite::VERT_BRX] = x + w;
        verts[CSprite::VERT_BRY] = y + h;
        float uvs[8];
        uvs[CSprite::VERT_ULX] = u0;
        uvs[CSprite::VERT_ULY] = v0;
        uvs[CSprite::VERT_URX] = u1;
        uvs[CSprite::VERT_URY] = v0;
        uvs[CSprite::VERT_BLX] = u0;
        uvs[CSprite::VERT_BLY] = v1;
        uvs[CSprite::VERT_BRX] = u1;
        uvs[CSprite::VERT_BRY] = v1;
        float rgba[4] = { 1.f, 1.f, 1.f, alpha };
        CSprite::renderVerts(tex, verts, uvs, rgba, eSpriteBlendMode::NORMAL, bGrayScale);
    }

    void drawSpriteQuadAlphaClipped(const CTexturePtr& tex, float x, float y, float w, float h,
        float u0, float v0, float u1, float v1, float alpha, const Rect* pClip, bool bGrayScale = false, bool bSnap = true)
    {
        if (!tex || alpha <= 0.f || w <= 0.f || h <= 0.f)
            return;
        if (!pClip)
        {
            drawSpriteQuadAlpha(tex, x, y, w, h, u0, v0, u1, v1, alpha, bGrayScale, bSnap);
            return;
        }
        float cx0 = std::max(x, pClip->x);
        float cy0 = std::max(y, pClip->y);
        float cx1 = std::min(x + w, pClip->x + pClip->cx);
        float cy1 = std::min(y + h, pClip->y + pClip->cy);
        if (cx1 <= cx0 || cy1 <= cy0)
            return;
        float du = u1 - u0, dv = v1 - v0;
        float tu0 = u0 + du * ((cx0 - x) / w);
        float tu1 = u0 + du * ((cx1 - x) / w);
        float tv0 = v0 + dv * ((cy0 - y) / h);
        float tv1 = v0 + dv * ((cy1 - y) / h);
        drawSpriteQuadAlpha(tex, cx0, cy0, cx1 - cx0, cy1 - cy0, tu0, tv0, tu1, tv1, alpha, bGrayScale, bSnap);
    }

    void drawSpriteQuadColorAlpha(const CTexturePtr& tex, float x, float y, float w, float h,
        float u0, float v0, float u1, float v1, unsigned int color, float alpha, bool bGrayScale = false, bool bSnap = true)
    {
        if (!tex || alpha <= 0.f || w <= 0.f || h <= 0.f)
            return;
        if (bSnap)
            snapPointToBuffer(x, y);
        alpha = std::clamp(alpha, 0.f, 1.f);
        float verts[8];
        verts[CSprite::VERT_ULX] = x;
        verts[CSprite::VERT_ULY] = y;
        verts[CSprite::VERT_URX] = x + w;
        verts[CSprite::VERT_URY] = y;
        verts[CSprite::VERT_BLX] = x;
        verts[CSprite::VERT_BLY] = y + h;
        verts[CSprite::VERT_BRX] = x + w;
        verts[CSprite::VERT_BRY] = y + h;
        float uvs[8];
        uvs[CSprite::VERT_ULX] = u0;
        uvs[CSprite::VERT_ULY] = v0;
        uvs[CSprite::VERT_URX] = u1;
        uvs[CSprite::VERT_URY] = v0;
        uvs[CSprite::VERT_BLX] = u0;
        uvs[CSprite::VERT_BLY] = v1;
        uvs[CSprite::VERT_BRX] = u1;
        uvs[CSprite::VERT_BRY] = v1;
        float colorAlpha = (float)(color & 0xFF) / 255.f;
        float rgba[4];
        rgba[0] = (float)((color >> 24) & 0xFF) / 255.f;
        rgba[1] = (float)((color >> 16) & 0xFF) / 255.f;
        rgba[2] = (float)((color >> 8) & 0xFF) / 255.f;
        rgba[3] = alpha * colorAlpha;
        CSprite::renderVerts(tex, verts, uvs, rgba, eSpriteBlendMode::NORMAL, bGrayScale);
    }

    void drawSpriteQuadColorAlphaClipped(const CTexturePtr& tex, float x, float y, float w, float h,
        float u0, float v0, float u1, float v1, unsigned int color, float alpha, const Rect* pClip, bool bGrayScale = false, bool bSnap = true)
    {
        if (!tex || alpha <= 0.f || w <= 0.f || h <= 0.f)
            return;
        if (!pClip)
        {
            drawSpriteQuadColorAlpha(tex, x, y, w, h, u0, v0, u1, v1, color, alpha, bGrayScale, bSnap);
            return;
        }
        float cx0 = std::max(x, pClip->x);
        float cy0 = std::max(y, pClip->y);
        float cx1 = std::min(x + w, pClip->x + pClip->cx);
        float cy1 = std::min(y + h, pClip->y + pClip->cy);
        if (cx1 <= cx0 || cy1 <= cy0)
            return;
        float du = u1 - u0, dv = v1 - v0;
        float tu0 = u0 + du * ((cx0 - x) / w);
        float tu1 = u0 + du * ((cx1 - x) / w);
        float tv0 = v0 + dv * ((cy0 - y) / h);
        float tv1 = v0 + dv * ((cy1 - y) / h);
        drawSpriteQuadColorAlpha(tex, cx0, cy0, cx1 - cx0, cy1 - cy0, tu0, tv0, tu1, tv1, color, alpha, bGrayScale, bSnap);
    }

    void renderNineSliceAlpha(const CTexturePtr& tex, float u0, float u1, float v0, float v1,
        float srcW, float srcH, float x, float y, float w, float h,
        float a, float b, float c, float d, float alpha, const Rect* pClip = nullptr, bool bGrayScale = false)
    {
        if (!tex || alpha <= 0.f || w <= 0.f || h <= 0.f)
            return;
        alpha = std::clamp(alpha, 0.f, 1.f);
        if (srcW <= 0.f || srcH <= 0.f)
        {
            drawSpriteQuadAlphaClipped(tex, x, y, w, h, u0, v0, u1, v1, alpha, pClip, bGrayScale);
            return;
        }
        if (alpha >= 1.f && !pClip)
        {
            renderSliceSprite(tex, u0, u1, v0, v1, srcW, srcH, x, y, w, h, a, b, c, d, bGrayScale);
            return;
        }
        float sa = std::max(0.f, a), sb = std::max(0.f, b), sc = std::max(0.f, c), sd = std::max(0.f, d);
        if (sa + sc > srcW && sa + sc > 0.f)
        {
            float k = srcW / (sa + sc);
            sa *= k;
            sc *= k;
        }
        if (sb + sd > srcH && sb + sd > 0.f)
        {
            float k = srcH / (sb + sd);
            sb *= k;
            sd *= k;
        }
        float da = sa, db = sb, dc = sc, dd = sd;
        if (da + dc > w && da + dc > 0.f)
        {
            float k = w / (da + dc);
            da *= k;
            dc *= k;
        }
        if (db + dd > h && db + dd > 0.f)
        {
            float k = h / (db + dd);
            db *= k;
            dd *= k;
        }
        float midW = std::max(0.f, w - da - dc);
        float midH = std::max(0.f, h - db - dd);
        float su0 = u0, su1 = u0 + (u1 - u0) * (sa / srcW), su2 = u1 - (u1 - u0) * (sc / srcW), su3 = u1;
        float sv0 = v0, sv1 = v0 + (v1 - v0) * (sb / srcH), sv2 = v1 - (v1 - v0) * (sd / srcH), sv3 = v1;
        if (su1 > su2)
        {
            float m = (su1 + su2) * 0.5f;
            su1 = su2 = m;
        }
        if (sv1 > sv2)
        {
            float m = (sv1 + sv2) * 0.5f;
            sv1 = sv2 = m;
        }
        auto cell = [&](float dx, float dy, float dw, float dh, float cu0, float cv0, float cu1, float cv1)
        {
            if (dw <= 0.f || dh <= 0.f)
                return;
            if (cu1 <= cu0 || cv1 <= cv0)
                return;
            drawSpriteQuadAlphaClipped(tex, dx, dy, dw, dh, cu0, cv0, cu1, cv1, alpha, pClip, bGrayScale);
        };
        float x0 = x, x1 = x + da, x2 = x + da + midW;
        float y0 = y, y1 = y + db, y2 = y + db + midH;
        cell(x0, y0, da, db, su0, sv0, su1, sv1);
        cell(x1, y0, midW, db, su1, sv0, su2, sv1);
        cell(x2, y0, dc, db, su2, sv0, su3, sv1);
        cell(x0, y1, da, midH, su0, sv1, su1, sv2);
        cell(x1, y1, midW, midH, su1, sv1, su2, sv2);
        cell(x2, y1, dc, midH, su2, sv1, su3, sv2);
        cell(x0, y2, da, dd, su0, sv2, su1, sv3);
        cell(x1, y2, midW, dd, su1, sv2, su2, sv3);
        cell(x2, y2, dc, dd, su2, sv2, su3, sv3);
    }

    void renderNineSliceColorAlpha(const CTexturePtr& tex, float u0, float u1, float v0, float v1,
        float srcW, float srcH, float x, float y, float w, float h,
        float a, float b, float c, float d, unsigned int color, float alpha, const Rect* pClip = nullptr, bool bGrayScale = false)
    {
        if (!tex || alpha <= 0.f || w <= 0.f || h <= 0.f)
            return;
        snapPointToBuffer(x, y);
        alpha = std::clamp(alpha, 0.f, 1.f);
        if (srcW <= 0.f || srcH <= 0.f)
        {
            drawSpriteQuadColorAlphaClipped(tex, x, y, w, h, u0, v0, u1, v1, color, alpha, pClip, bGrayScale, false);
            return;
        }
        if (color == 0xFFFFFFFF && alpha >= 1.f && !pClip)
        {
            renderSliceSprite(tex, u0, u1, v0, v1, srcW, srcH, x, y, w, h, a, b, c, d, bGrayScale);
            return;
        }
        float sa = std::max(0.f, a), sb = std::max(0.f, b), sc = std::max(0.f, c), sd = std::max(0.f, d);
        if (sa + sc > srcW && sa + sc > 0.f)
        {
            float k = srcW / (sa + sc);
            sa *= k;
            sc *= k;
        }
        if (sb + sd > srcH && sb + sd > 0.f)
        {
            float k = srcH / (sb + sd);
            sb *= k;
            sd *= k;
        }
        float da = sa, db = sb, dc = sc, dd = sd;
        if (da + dc > w && da + dc > 0.f)
        {
            float k = w / (da + dc);
            da *= k;
            dc *= k;
        }
        if (db + dd > h && db + dd > 0.f)
        {
            float k = h / (db + dd);
            db *= k;
            dd *= k;
        }
        float midW = std::max(0.f, w - da - dc);
        float midH = std::max(0.f, h - db - dd);
        float su0 = u0, su1 = u0 + (u1 - u0) * (sa / srcW), su2 = u1 - (u1 - u0) * (sc / srcW), su3 = u1;
        float sv0 = v0, sv1 = v0 + (v1 - v0) * (sb / srcH), sv2 = v1 - (v1 - v0) * (sd / srcH), sv3 = v1;
        if (su1 > su2)
        {
            float m = (su1 + su2) * 0.5f;
            su1 = su2 = m;
        }
        if (sv1 > sv2)
        {
            float m = (sv1 + sv2) * 0.5f;
            sv1 = sv2 = m;
        }
        auto cell = [&](float dx, float dy, float dw, float dh, float cu0, float cv0, float cu1, float cv1)
        {
            if (dw <= 0.f || dh <= 0.f)
                return;
            if (cu1 <= cu0 || cv1 <= cv0)
                return;
            drawSpriteQuadColorAlphaClipped(tex, dx, dy, dw, dh, cu0, cv0, cu1, cv1, color, alpha, pClip, bGrayScale, false);
        };
        float x0 = x, x1 = x + da, x2 = x + da + midW;
        float y0 = y, y1 = y + db, y2 = y + db + midH;
        cell(x0, y0, da, db, su0, sv0, su1, sv1);
        cell(x1, y0, midW, db, su1, sv0, su2, sv1);
        cell(x2, y0, dc, db, su2, sv0, su3, sv1);
        cell(x0, y1, da, midH, su0, sv1, su1, sv2);
        cell(x1, y1, midW, midH, su1, sv1, su2, sv2);
        cell(x2, y1, dc, midH, su2, sv1, su3, sv2);
        cell(x0, y2, da, dd, su0, sv2, su1, sv3);
        cell(x1, y2, midW, dd, su1, sv2, su2, sv3);
        cell(x2, y2, dc, dd, su2, sv2, su3, sv3);
    }

    void renderThreeSliceAlphaClipped(const CTexturePtr& tex, float u0, float u1, float v0, float v1,
        float srcW, float srcH, float x, float y, float w, float h,
        float a, float b, bool vertical, int mirror, float alpha, const Rect* pClip, bool bGrayScale = false)
    {
        if (!tex || alpha <= 0.f || w <= 0.f || h <= 0.f)
            return;
        snapPointToBuffer(x, y);
        alpha = std::clamp(alpha, 0.f, 1.f);
        if (srcW <= 0.f || srcH <= 0.f)
        {
            drawSpriteQuadAlphaClipped(tex, x, y, w, h, u0, v0, u1, v1, alpha, pClip, bGrayScale, false);
            return;
        }
        if (alpha >= 1.f && !pClip)
        {
            renderThreeSliceSprite(tex, u0, u1, v0, v1, srcW, srcH, x, y, w, h, a, b, vertical, mirror, bGrayScale);
            return;
        }
        const float srcLen = vertical ? srcH : srcW;
        const float dstLen = vertical ? h : w;
        float sa = std::max(0.f, a), sb = std::max(0.f, b);
        if (sa + sb > srcLen && sa + sb > 0.f)
        {
            float k = srcLen / (sa + sb);
            sa *= k;
            sb *= k;
        }
        float da = sa, db = sb;
        if (mirror == 1)
            da = sb;
        else if (mirror == 2)
            db = sa;
        if (da + db > dstLen && da + db > 0.f)
        {
            float k = dstLen / (da + db);
            da *= k;
            db *= k;
        }
        const float midDst = std::max(0.f, dstLen - da - db);
        const float midSrc0 = sa;
        const float midSrc1 = srcLen - sb;
        auto axisUV = [&](float p) -> float
        {
            float t = (srcLen > 0.f) ? (p / srcLen) : 0.f;
            return vertical ? (v0 + (v1 - v0) * t) : (u0 + (u1 - u0) * t);
        };
        float la0, la1, ra0, ra1;
        bool lRev = false, rRev = false;
        if (mirror == 1)
        {
            la0 = srcLen - sb;
            la1 = srcLen;
            lRev = true;
        }
        else
        {
            la0 = 0.f;
            la1 = sa;
        }
        if (mirror == 2)
        {
            ra0 = 0.f;
            ra1 = sa;
            rRev = true;
        }
        else
        {
            ra0 = srcLen - sb;
            ra1 = srcLen;
        }
        auto drawCell = [&](float d0, float dLen, float s0, float s1, bool rev)
        {
            if (dLen <= 0.f)
                return;
            if (s1 <= s0)
                return;
            float q0 = axisUV(s0), q1 = axisUV(s1);
            if (rev)
                std::swap(q0, q1);
            if (!vertical)
                drawSpriteQuadAlphaClipped(tex, x + d0, y, dLen, h, q0, v0, q1, v1, alpha, pClip, bGrayScale, false);
            else
                drawSpriteQuadAlphaClipped(tex, x, y + d0, w, dLen, u0, q0, u1, q1, alpha, pClip, bGrayScale, false);
        };
        drawCell(0.f, da, la0, la1, lRev);
        drawCell(da, midDst, midSrc0, midSrc1, false);
        drawCell(da + midDst, db, ra0, ra1, rRev);
    }

    void renderThreeSliceColorAlphaClipped(const CTexturePtr& tex, float u0, float u1, float v0, float v1,
        float srcW, float srcH, float x, float y, float w, float h,
        float a, float b, bool vertical, int mirror, unsigned int color, float alpha, const Rect* pClip, bool bGrayScale = false)
    {
        if (!tex || alpha <= 0.f || w <= 0.f || h <= 0.f)
            return;
        snapPointToBuffer(x, y);
        alpha = std::clamp(alpha, 0.f, 1.f);
        if (srcW <= 0.f || srcH <= 0.f)
        {
            drawSpriteQuadColorAlphaClipped(tex, x, y, w, h, u0, v0, u1, v1, color, alpha, pClip, bGrayScale, false);
            return;
        }
        if (color == 0xFFFFFFFF && alpha >= 1.f && !pClip)
        {
            renderThreeSliceSprite(tex, u0, u1, v0, v1, srcW, srcH, x, y, w, h, a, b, vertical, mirror, bGrayScale);
            return;
        }
        const float srcLen = vertical ? srcH : srcW;
        const float dstLen = vertical ? h : w;
        float sa = std::max(0.f, a), sb = std::max(0.f, b);
        if (sa + sb > srcLen && sa + sb > 0.f)
        {
            float k = srcLen / (sa + sb);
            sa *= k;
            sb *= k;
        }
        float da = sa, db = sb;
        if (mirror == 1)
            da = sb;
        else if (mirror == 2)
            db = sa;
        if (da + db > dstLen && da + db > 0.f)
        {
            float k = dstLen / (da + db);
            da *= k;
            db *= k;
        }
        const float midDst = std::max(0.f, dstLen - da - db);
        const float midSrc0 = sa;
        const float midSrc1 = srcLen - sb;
        auto axisUV = [&](float p) -> float
        {
            float t = (srcLen > 0.f) ? (p / srcLen) : 0.f;
            return vertical ? (v0 + (v1 - v0) * t) : (u0 + (u1 - u0) * t);
        };
        float la0, la1, ra0, ra1;
        bool lRev = false, rRev = false;
        if (mirror == 1)
        {
            la0 = srcLen - sb;
            la1 = srcLen;
            lRev = true;
        }
        else
        {
            la0 = 0.f;
            la1 = sa;
        }
        if (mirror == 2)
        {
            ra0 = 0.f;
            ra1 = sa;
            rRev = true;
        }
        else
        {
            ra0 = srcLen - sb;
            ra1 = srcLen;
        }
        auto drawCell = [&](float d0, float dLen, float s0, float s1, bool rev)
        {
            if (dLen <= 0.f)
                return;
            if (s1 <= s0)
                return;
            float q0 = axisUV(s0), q1 = axisUV(s1);
            if (rev)
                std::swap(q0, q1);
            if (!vertical)
                drawSpriteQuadColorAlphaClipped(tex, x + d0, y, dLen, h, q0, v0, q1, v1, color, alpha, pClip, bGrayScale, false);
            else
                drawSpriteQuadColorAlphaClipped(tex, x, y + d0, w, dLen, u0, q0, u1, q1, color, alpha, pClip, bGrayScale, false);
        };
        drawCell(0.f, da, la0, la1, lRev);
        drawCell(da, midDst, midSrc0, midSrc1, false);
        drawCell(da + midDst, db, ra0, ra1, rRev);
    }

    void drawThreeSliceSafe(const CTexturePtr& tex, const float uv[4], float srcW, float srcH,
        float x, float y, float w, float h, float a, float b, bool vertical, int mirror,
        const Rect* pClip = nullptr, bool bGrayScale = false)
    {
        if (!tex || w <= 0.f || h <= 0.f)
            return;
        renderThreeSliceAlphaClipped(tex, uv[0], uv[2], uv[1], uv[3], srcW, srcH,
            x, y, w, h, a, b, vertical, mirror, 1.f, pClip, bGrayScale);
    }

    void drawThreeSliceSafeColor(const CTexturePtr& tex, const float uv[4], float srcW, float srcH,
        float x, float y, float w, float h, float a, float b, bool vertical, int mirror,
        unsigned int color, const Rect* pClip = nullptr, bool bGrayScale = false)
    {
        if (!tex || w <= 0.f || h <= 0.f)
            return;
        renderThreeSliceColorAlphaClipped(tex, uv[0], uv[2], uv[1], uv[3], srcW, srcH,
            x, y, w, h, a, b, vertical, mirror, color, 1.f, pClip, bGrayScale);
    }

    void getButtonFadeDurations(const HTMLDomNode& node, float& hoverDur, float& pressedDur)
    {
        if (!node.runtimeFadeCached)
            cacheRuntimeAttrs(const_cast<HTMLDomNode&>(node));
        hoverDur = node.hoverFadeDurCache;
        pressedDur = node.pressedFadeDurCache;
    }

    bool nodeHasIdAttr(const std::shared_ptr<HTMLDomNode>& node)
    {
        if (!node)
            return false;
        return node->attrs.find("id") != nullptr;
    }

    void collectTrNodesRaw(const HTMLDomNode* node, std::vector<HTMLDomNode*>& out)
    {
        if (!node)
            return;
        if (node->tag == eHTMLTag::Tr)
        {
            out.push_back(const_cast<HTMLDomNode*>(node));
            return;
        }
        if (node->tag == eHTMLTag::Table)
            return;
        for (const auto& child : node->children)
            collectTrNodesRaw(child.get(), out);
    }

    void collectTdCellsRaw(const HTMLDomNode* node, std::vector<HTMLDomNode*>& stylePath, HTMLRenderScratch& sc)
    {
        if (!node)
            return;
        if (node->tag == eHTMLTag::Td)
        {
            int idx = (int)sc.cells.size();
            sc.cells.push_back(HTMLTableCell());
            sc.cells[idx].tdNode = const_cast<HTMLDomNode*>(node);
            sc.cells[idx].styleStart = (int)sc.cellStylePool.size();
            for (HTMLDomNode* s : stylePath)
                sc.cellStylePool.push_back(s);
            sc.cells[idx].styleEnd = (int)sc.cellStylePool.size();
            return;
        }
        if (node->tag == eHTMLTag::Table)
            return;
        bool isStyle =
            node->tag == eHTMLTag::Font || node->tag == eHTMLTag::Span || node->tag == eHTMLTag::B || node->tag == eHTMLTag::Strong ||
            node->tag == eHTMLTag::I || node->tag == eHTMLTag::Em || node->tag == eHTMLTag::U ||
            node->tag == eHTMLTag::S || node->tag == eHTMLTag::Center;
        if (isStyle)
            stylePath.push_back(const_cast<HTMLDomNode*>(node));
        for (const auto& child : node->children)
            collectTdCellsRaw(child.get(), stylePath, sc);
        if (isStyle)
            stylePath.pop_back();
    }

    void collectSelectOptionsRaw(const HTMLDomNode* node, std::vector<HTMLDomNode*>& out)
    {
        if (!node)
            return;
        for (const auto& child : node->children)
        {
            if (!child)
                continue;
            if (!child->visible)
                continue;
            if (child->tag == eHTMLTag::Option)
            {
                out.push_back(child.get());
                continue;
            }
            if (child->tag == eHTMLTag::Select)
                continue;
            collectSelectOptionsRaw(child.get(), out);
        }
    }

    void collectSelectOptions(const HTMLDom::NodePtr& node, std::vector<HTMLDom::NodePtr>& out)
    {
        if (!node)
            return;
        for (const auto& child : node->children)
        {
            if (!child)
                continue;
            if (!child->visible)
                continue;
            if (child->tag == eHTMLTag::Option)
            {
                out.push_back(child);
                continue;
            }
            if (child->tag == eHTMLTag::Select)
                continue;
            collectSelectOptions(child, out);
        }
    }

    void collectPlainTextInto(const HTMLDomNode* node, std::string& out)
    {
        if (!node)
            return;
        if (node->tag == eHTMLTag::Text)
        {
            out += node->text;
            return;
        }
        if (node->tag == eHTMLTag::Br)
        {
            out += ' ';
            return;
        }
        for (const auto& child : node->children)
            collectPlainTextInto(child.get(), out);
    }

    std::string collectPlainText(const HTMLDom::NodePtr& node)
    {
        std::string res;
        collectPlainTextInto(node.get(), res);
        return res;
    }

    void domCollectBtnGroupWidths(HTMLDom* dom, const HTMLDom::NodePtr& node, HTMLRenderState& st, HTMLRenderScratch& sc)
    {
        if (!node || !node->visible)
            return;
        HTMLStateScope scope(st, sc);
        applyInlineNodeToState(node.get(), st);
        if (node->tag == eHTMLTag::NineButton && !node->btnGroup.empty())
        {
            sc.textBuf.clear();
            collectPlainTextInto(node.get(), sc.textBuf);
            domCollapseAsciiWhiteSpacePreserveNbsp(sc.textBuf);
            domTrimLeftPreserveNbsp(sc.textBuf);
            domTrimRightPreserveNbsp(sc.textBuf);
            float naturalW = sc.textBuf.empty() ? 0.f : domMeasureStyledText(sc.textBuf, st);

            float w = naturalW
                + std::max(node->padding.left, node->sliceA)
                + std::max(node->padding.right, node->sliceB)
                + 2.f;
            HTMLGroupWidth* g = nullptr;
            for (auto& gw : sc.groups)
            {
                if (gw.name && *gw.name == node->btnGroup)
                {
                    g = &gw;
                    break;
                }
            }
            if (!g)
            {
                sc.groups.push_back(HTMLGroupWidth());
                g = &sc.groups.back();
                g->name = &node->btnGroup;
                g->w = 0.f;
            }
            if (w > g->w)
                g->w = w;
        }
        for (const auto& child : node->children)
            domCollectBtnGroupWidths(dom, child, st, sc);
    }

    void domEnsureEditInited(const HTMLDom::NodePtr& node)
    {
        if (!node || node->editInited)
            return;
        node->editInited = true;
        std::string v;
        collectPlainTextInto(node.get(), v);
        domCollapseAsciiWhiteSpacePreserveNbsp(v);
        domTrimLeftPreserveNbsp(v);
        domTrimRightPreserveNbsp(v);
        node->editValue = std::move(v);
        node->editCursor = (int)node->editValue.size();
        node->editSelAnchor = node->editCursor;
    }

    void getSelectOptionLabelInto(const HTMLDomNode* option, std::string& out)
    {
        out.clear();
        if (!option)
            return;
        collectPlainTextInto(option, out);
        domCollapseAsciiWhiteSpacePreserveNbsp(out);
        domTrimLeftPreserveNbsp(out);
        domTrimRightPreserveNbsp(out);
        if (out.empty())
            out = option->selectValue;
    }

    std::string getSelectOptionLabel(const HTMLDom::NodePtr& option)
    {
        std::string s;
        getSelectOptionLabelInto(option.get(), s);
        return s;
    }

    int computeSelectIndex(const HTMLDomNode* node, const std::vector<HTMLDomNode*>& options)
    {
        if (!node || options.empty())
            return -1;
        if (node->selectedIndex >= 0 && node->selectedIndex < (int)options.size() &&
            options[node->selectedIndex]->optionEnabled)
            return node->selectedIndex;
        for (int i = 0; i < (int)options.size(); ++i)
            if (options[i] && options[i]->optionSelected && options[i]->optionEnabled)
                return i;
        for (int i = 0; i < (int)options.size(); ++i)
            if (options[i] && options[i]->optionEnabled)
                return i;
        return 0;
    }

    int computeSelectIndex(const HTMLDomNode* node, const std::vector<HTMLDom::NodePtr>& options)
    {
        if (!node || options.empty())
            return -1;
        if (node->selectedIndex >= 0 && node->selectedIndex < (int)options.size() &&
            options[node->selectedIndex]->optionEnabled)
            return node->selectedIndex;
        for (int i = 0; i < (int)options.size(); ++i)
            if (options[i] && options[i]->optionSelected && options[i]->optionEnabled)
                return i;
        for (int i = 0; i < (int)options.size(); ++i)
            if (options[i] && options[i]->optionEnabled)
                return i;
        return 0;
    }

    void fitSelectLabel(const std::string& text, const HTMLRenderState& st, float maxW, std::string& out)
    {
        out.clear();
        if (text.empty())
            return;
        if (maxW <= 0.f)
            return;
        if (domMeasureStyledText(text, st) <= maxW)
        {
            out = text;
            return;
        }
        const char* ellipsis = "...";
        float ellW = domMeasureStyledTextRange(ellipsis, 3, st);
        if (ellW >= maxW)
        {
            out.assign(ellipsis);
            return;
        }
        int len = (int)text.size();
        int i = 0;
        int keepEnd = 0;
        float acc = 0.f;
        while (i < len)
        {
            int cl = getUtf8CharLen(text.c_str() + i);
            float cw = domMeasureStyledTextRange(text.c_str() + i, cl, st);
            if (acc + cw + ellW > maxW)
                break;
            acc += cw;
            i += cl;
            keepEnd = i;
        }
        out.assign(text, 0, keepEnd);
        out.append(ellipsis);
    }

    void drawTextRange(FONScontext* fs, const char* text, int len, float x, float y, const HTMLRenderState& cst)
    {
        if (!text || len <= 0)
            return;
        HTMLRenderState& st = const_cast<HTMLRenderState&>(cst);
        domRefreshFontState(st);
        fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
        float dx = domSnap(x);
        float dy = domSnap(y);
        if (st.fakeBold)
        {
            float ox = domFakeBoldOffset(st.fontSize);
            fonsDrawText(fs, dx, dy, text, text + len);
            fonsDrawText(fs, dx + ox, dy, text, text + len);
        }
        else
        {
            fonsDrawText(fs, dx, dy, text, text + len);
        }
    }

    Rect localRectToScreen(std::weak_ptr<CContainer> whost, const Rect& localRect)
    {
        auto host = whost.lock();
        if (!host)
            return localRect;

        float x = localRect.x, y = localRect.y, sx = 1.f, sy = 1.f;
        CContainer* cur = host.get();

        while (cur)
        {
            switch (cur->getScrollType())
            {
            case E_ST_HOR:
                x += cur->getScroll();
                break;
            case E_ST_VERT:
                y += cur->getScroll();
                break;
            default:
                break;
            }

            x = cur->getX() + x * cur->getScaleX();
            y = cur->getY() + y * cur->getScaleY();

            sx *= std::fabs(cur->getScaleX());
            sy *= std::fabs(cur->getScaleY());

            cur = cur->getParent();
        }

        Rect out;
        out.x = x;
        out.y = y;
        out.cx = localRect.cx * sx;
        out.cy = localRect.cy * sy;
        return out;
    }
}

void HTMLDom::RenderContext::renderLine(float x, float y, float w, float h, unsigned int color)
{
    if (measureOnly || w <= 0.f || h <= 0.f)
        return;
    snapPointToBuffer(x, y);
    auto ptrWhiteBox = Engine::getCfg().wb;
    if (!ptrWhiteBox)
        return;
    float verts[8];
    verts[CSprite::VERT_ULX] = x;
    verts[CSprite::VERT_ULY] = y;
    verts[CSprite::VERT_URX] = x + w;
    verts[CSprite::VERT_URY] = y;
    verts[CSprite::VERT_BLX] = x;
    verts[CSprite::VERT_BLY] = y + h;
    verts[CSprite::VERT_BRX] = x + w;
    verts[CSprite::VERT_BRY] = y + h;
    float rgba[4];
    rgba[0] = ((color >> 24) & 0xFF) / 255.f;
    rgba[1] = ((color >> 16) & 0xFF) / 255.f;
    rgba[2] = ((color >> 8) & 0xFF) / 255.f;
    rgba[3] = ((color >> 0) & 0xFF) / 255.f;
    CSprite::renderVerts(
        ptrWhiteBox->getTexture(),
        verts,
        ptrWhiteBox->_uvs,
        rgba,
        eSpriteBlendMode::NORMAL,
        grayScale
    );
}

void HTMLDom::RenderContext::updateBounds(float bx, float by, float bw, float bh)
{
    if (bw <= 0.f || bh <= 0.f)
        return;
    if (visRecordGot && !*visRecordGot)
    {
        visRecordRc->set(bx, by, bw, bh);
        *visRecordGot = true;
    }
    if (!hasBounds)
    {
        rcOut->set(bx, by, bx + bw, by + bh);
        hasBounds = true;
    }
    else
    {
        Rect r;
        r.set(bx, by, bx + bw, by + bh);
        rcOut->unite(&r);
    }
}

float HTMLDom::RenderContext::renderChar(
    const char* ch,
    float charX,
    float charY,
    int charSize,
    unsigned int charColor,
    unsigned int charTint,
    const HTMLRenderState& state,
    int charIndex,
    int totalChars,
    unsigned long long fontAnimId)
{
    fonsSetFont(fs, state.fontHandle);
    fonsSetSize(fs, charSize);
    fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
    float baseW = fonsTextBounds(fs, 0.f, 0.f, ch, nullptr, nullptr);
    float styleAdvance = 0.f;
    if (state.fakeBold)
        styleAdvance += domFakeBoldOffset(charSize);
    float totalW = baseW + styleAdvance;
    if (measureOnly)
        return totalW;

    float localBlockStart = blockStart;
    float localAnimTime = animTime;
    if (fontAnimId != 0 &&
        (state.appearStyle != eAppearStyle::NONE || state.fxStyle != eFxStyle::NONE))
    {
        auto itAnim = dom->_appearStarts.find(fontAnimId);
        if (itAnim == dom->_appearStarts.end())
        {
            itAnim = dom->_appearStarts.emplace(
                fontAnimId,
                FxAnimState{ blockStart, animTime, false, true }
            ).first;
        }
        else if (!itAnim->second.bIsInitiated)
        {
            itAnim->second.bIsInitiated = true;
            itAnim->second.fBlockStart = blockStart;
            itAnim->second.fAnimTime = animTime;
        }
        if (!itAnim->second.bIsPaused)
            itAnim->second.fAnimTime = animTime;
        localBlockStart = itAnim->second.fBlockStart;
        localAnimTime = itAnim->second.fAnimTime;

        auto& flags = dom->_fontAnimEventFlags[fontAnimId];
        flags.rootId = rootId;
        bool wantAppearStart = false;
        bool wantAppearComplete = false;
        bool wantFxStart = false;
        if (state.appearStyle != eAppearStyle::NONE)
        {
            float appearStart = localBlockStart + state.appearDelay;
            float appearEnd = appearStart + state.appearDur;
            if (!flags.appearStart && localAnimTime >= localBlockStart)
                wantAppearStart = true;
            if (!flags.appearComplete && localAnimTime >= appearEnd)
                wantAppearComplete = true;
        }
        if (state.fxStyle != eFxStyle::NONE)
        {
            float fxStart = localBlockStart + state.fxDelay;
            if (state.appearStyle != eAppearStyle::NONE)
            {
                float appearEnd = localBlockStart + state.appearDelay + state.appearDur;
                fxStart = appearEnd + state.fxDelay;
            }
            if (!flags.fxStart && localAnimTime >= fxStart)
                wantFxStart = true;
        }
        if (wantAppearStart)
            flags.appearStart = true;
        if (wantAppearComplete)
            flags.appearComplete = true;
        if (wantFxStart)
            flags.fxStart = true;
        if (wantAppearStart || wantAppearComplete || wantFxStart)
        {
            auto fontNode = dom->lockNode(fontAnimId);
            if (nodeHasIdAttr(fontNode))
            {
                if (auto cb = dom->getDocumentCallbacks(rootId))
                {
                    if (wantAppearStart)
                    {
                        cb->onFontAnimEvent(
                            rootId,
                            fontAnimId,
                            eHTMLFontAnimEventType::AppearStart,
                            state.fxStyle,
                            state.appearStyle
                        );
                    }
                    if (wantAppearComplete)
                    {
                        cb->onFontAnimEvent(
                            rootId,
                            fontAnimId,
                            eHTMLFontAnimEventType::AppearComplete,
                            state.fxStyle,
                            state.appearStyle
                        );
                    }
                    if (wantFxStart)
                    {
                        cb->onFontAnimEvent(
                            rootId,
                            fontAnimId,
                            eHTMLFontAnimEventType::FxStart,
                            state.fxStyle,
                            state.appearStyle
                        );
                    }
                }
            }
        }
    }

    float renderX = charX;
    float renderY = charY;
    int renderSize = charSize;
    unsigned int renderColor = charColor;
    float mtxDX = 0.f;
    float mtxDY = 0.f;
    float mtxScale = 1.f;
    float fxRot = 0.f;
    float fxChroma = 0.f;
    float fxEcho = 0.f;
    float fxMirror = 0.f;
    float fxGlow = 0.f;
    unsigned int fxGlowColor = 0xFFFFFFFFu;

    if (state.hasGradient && totalChars > 1)
    {
        float t = (float)charIndex / (float)(totalChars - 1);
        renderColor = lerpColor(domMulColor(state.gradientColor1, charTint),
                                domMulColor(state.gradientColor2, charTint), t);
    }

    if (state.appearStyle != eAppearStyle::NONE)
    {
        AppearFx ap = evalAppearEffect(
            state.appearStyle,
            state.appearDur,
            state.appearDelay,
            localBlockStart,
            localAnimTime,
            charIndex,
            totalChars,
            charSize,
            renderColor
        );
        if (!ap.visible)
            return totalW;
        renderX += ap.dx;
        renderY += ap.dy;
        renderColor = ap.color;
        if (ap.size != charSize && charSize > 0)
            mtxScale *= (float)ap.size / (float)charSize;
    }

    if (state.fxStyle != eFxStyle::NONE)
    {
        bool fxOn = true;
        float fxTime = localAnimTime;
        if (state.appearStyle != eAppearStyle::NONE)
        {
            float appearEnd = localBlockStart + state.appearDelay + state.appearDur;
            if (localAnimTime < appearEnd)
                fxOn = false;
            else
                fxTime = localAnimTime - appearEnd;
        }
        if (fxOn)
        {
            applyFxEffect(
                fs,
                renderColor,
                charIndex,
                fxTime,
                state.fxStyle,
                state.fxDelay,
                mtxDX,
                mtxDY,
                mtxScale,
                fxGlow,
                fxGlowColor,
                fxRot,
                fxChroma,
                fxEcho,
                fxMirror
            );
        }
    }

    if (documentColor != 0xFFFFFFFF)
        renderColor = domMulColor(renderColor, documentColor);
    if (documentColor != 0xFFFFFFFF)
        fxGlowColor = domMulColor(fxGlowColor, documentColor);

    float masterAlpha = (float)(renderColor & 0xFFu) / 255.f;
    if (masterAlpha <= 0.f)
        return totalW;

    bool fxMatrix = (mtxDX != 0.f || mtxDY != 0.f || mtxScale != 1.f || fxRot != 0.f);
    auto pFxMS = fxMatrix ? CGfx::getInstance()->getMatrixStack() : nullptr;
    if (pFxMS)
    {
        pFxMS->save();
        if (mtxDX != 0.f || mtxDY != 0.f)
            pFxMS->translate(mtxDX, mtxDY);
        if (mtxScale != 1.f)
        {
            pFxMS->translate(renderX, renderY);
            pFxMS->scale(mtxScale, mtxScale);
            pFxMS->translate(-renderX, -renderY);
        }
        if (fxRot != 0.f)
        {
            const float rcx = renderX + totalW * 0.5f;
            const float rcy = renderY + (float)charSize * 0.5f;
            pFxMS->translate(rcx, rcy);
            pFxMS->rotate(fxRot);
            pFxMS->translate(-rcx, -rcy);
        }
    }

    auto applyFakeItalicMatrix = [&](float x, float y) -> bool
    {
        if (!state.fakeItalic)
            return false;
        auto pMS = CGfx::getInstance()->getMatrixStack();
        if (!pMS)
            return false;
        float ascent = 0.f;
        float descent = 0.f;
        float lineh = 0.f;
        fonsVertMetrics(fs, &ascent, &descent, &lineh);
        pMS->save();
        pMS->shearXAt(HTML_FAKE_ITALIC_SKEW, y + ascent);
        return true;
    };
    auto clearFakeItalicMatrix = [&](bool active)
    {
        if (!active)
            return;
        auto pMS = CGfx::getInstance()->getMatrixStack();
        if (pMS)
            pMS->restore();
    };

    auto drawStyled = [&](float x, float y, const char* s, unsigned int col)
    {
        if (grayScale)
            col = domGrayColor(col);
        fonsSetFont(fs, state.fontHandle);
        fonsSetSize(fs, renderSize);
        fonsSetColor(fs, col);
        fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
        float dx = domSnap(x);
        float dy = domSnap(y);
        bool italicMatrix = applyFakeItalicMatrix(dx, dy);
        if (state.fakeBold)
        {
            float ox = domFakeBoldOffset(renderSize);
            fonsDrawText(fs, dx, dy, s, nullptr);
            fonsDrawText(fs, dx + ox, dy, s, nullptr);
        }
        else
        {
            fonsDrawText(fs, dx, dy, s, nullptr);
        }
        clearFakeItalicMatrix(italicMatrix);
    };

    if (state.shadow > 0.f)
    {
        unsigned int shColor = domMulColor(mulAlpha(domMulColor(state.shadowColor, charTint), masterAlpha), documentColor);
        drawStyled(renderX + state.shadow, renderY + state.shadow, ch, shColor);
    }
    if (state.outline > 0.f)
    {
        unsigned int outColor = domMulColor(mulAlpha(domMulColor(state.outlineColor, charTint), masterAlpha), documentColor);
        float ox = state.outline;
        drawStyled(renderX - ox, renderY, ch, outColor);
        drawStyled(renderX + ox, renderY, ch, outColor);
        drawStyled(renderX, renderY - ox, ch, outColor);
        drawStyled(renderX, renderY + ox, ch, outColor);
        drawStyled(renderX - ox, renderY - ox, ch, outColor);
        drawStyled(renderX + ox, renderY - ox, ch, outColor);
        drawStyled(renderX - ox, renderY + ox, ch, outColor);
        drawStyled(renderX + ox, renderY + ox, ch, outColor);
    }
    if (fxGlow > 0.f)
    {
        unsigned int gc = mulAlpha(fxGlowColor, std::clamp(fxGlow, 0.f, 1.f) * masterAlpha);
        const float o = 1.5f;
        drawStyled(renderX - o, renderY, ch, gc);
        drawStyled(renderX + o, renderY, ch, gc);
        drawStyled(renderX, renderY - o, ch, gc);
        drawStyled(renderX, renderY + o, ch, gc);
        drawStyled(renderX - o, renderY - o, ch, gc);
        drawStyled(renderX + o, renderY - o, ch, gc);
        drawStyled(renderX - o, renderY + o, ch, gc);
        drawStyled(renderX + o, renderY + o, ch, gc);
    }
    if (fxChroma > 0.05f)
    {

        drawStyled(renderX - fxChroma, renderY, ch, mulAlpha(0xFF2A2AFF, masterAlpha));
        drawStyled(renderX + fxChroma, renderY, ch, mulAlpha(0x40E8FFFF, masterAlpha));
    }

    drawStyled(renderX, renderY, ch, renderColor);

    if (std::fabs(fxEcho) > 0.05f)
    {

        for (int i = 1; i <= 2; ++i)
        {
            const float k = (float)i / 3.0f;
            unsigned int ec = lerpColor(renderColor, fxGlowColor, 0.65f);
            ec = mulAlpha(ec, (1.0f - k) * 0.55f * masterAlpha);
            drawStyled(renderX + fxEcho * (float)i, renderY, ch, ec);
        }
    }
    if (fxMirror > 0.01f)
    {

        float mAsc = 0.f;
        float mDesc = 0.f;
        float mLh = 0.f;
        fonsVertMetrics(fs, &mAsc, &mDesc, &mLh);
        const float mirrorY = renderY + mAsc * 1.05f;
        auto pMSMirror = CGfx::getInstance()->getMatrixStack();
        if (pMSMirror)
        {
            pMSMirror->save();
            pMSMirror->translate(0.f, mirrorY);
            pMSMirror->scale(1.f, -1.f);
            pMSMirror->translate(0.f, -mirrorY);
            drawStyled(renderX, renderY, ch, mulAlpha(renderColor, fxMirror * masterAlpha));
            pMSMirror->restore();
        }
    }

    if (pFxMS)
        pFxMS->restore();

    return totalW;
}

void HTMLDom::RenderContext::pushChunk(const char* text, int len, float width, const HTMLRenderState& state)
{
    if (len <= 0)
        return;
    HTMLRenderScratch& sc = *dom->_scratch;
    int idx = sc.chunkCount;
    if ((int)sc.chunks.size() <= idx)
    {
        sc.chunks.push_back(HTMLChunk());
        sc.chunkLinkIds.push_back(0);
        sc.chunkFontAnimIds.push_back(0);
    }
    HTMLChunk& c = sc.chunks[idx];
    c.text = text;
    c.len = len;
    c.stateIndex = dom->internRenderState(state);
    c.colorMul = domMulColor(disabledTint, domAlphaColor(extraAlpha));
    c.isImage = false;
    c.imgTex = CTexturePtr();
    sc.chunkLinkIds[idx] = currentLinkId;
    sc.chunkFontAnimIds[idx] = currentFontAnimId;
    ++sc.chunkCount;
    totalWidth += width;
}

void HTMLDom::RenderContext::pushImageChunk(const HTMLDomNode& node, float w, float h, float marginL, float marginR,
    unsigned int color, const HTMLRenderState& state)
{
    HTMLRenderScratch& sc = *dom->_scratch;
    int idx = sc.chunkCount;
    if ((int)sc.chunks.size() <= idx)
    {
        sc.chunks.push_back(HTMLChunk());
        sc.chunkLinkIds.push_back(0);
        sc.chunkFontAnimIds.push_back(0);
    }
    HTMLChunk& c = sc.chunks[idx];
    c.text = nullptr;
    c.len = 0;
    c.stateIndex = dom->internRenderState(state);
    c.colorMul = 0xFFFFFFFF;
    c.isImage = true;
    c.imgTex = node.spriteTex;
    c.imgUV[0] = node.spriteUV[0];
    c.imgUV[1] = node.spriteUV[1];
    c.imgUV[2] = node.spriteUV[2];
    c.imgUV[3] = node.spriteUV[3];
    c.imgW = w;
    c.imgH = h;
    c.imgValign = (int)node.imgValign;
    c.imgMarginL = marginL;
    c.imgColor = color;
    c.imgNodeId = node.id;
    sc.chunkLinkIds[idx] = currentLinkId;
    sc.chunkFontAnimIds[idx] = 0;
    ++sc.chunkCount;
    totalWidth += w + marginL + marginR;
}

void HTMLDom::RenderContext::addWord(const char* word, int len, float wordWidth, const HTMLRenderState& state)
{
    HTMLRenderScratch& sc = *dom->_scratch;

    auto trimLeadingIfLineEmpty = [&]()
    {
        if (sc.chunkCount > baseChunk)
            return;
        int p = 0;
        while (p < len && isAsciiSpace((unsigned char)word[p]))
            ++p;
        if (p)
        {
            word += p;
            len -= p;
            wordWidth = domMeasureStyledTextRange(word, len, state);
        }
    };

    trimLeadingIfLineEmpty();
    if (len <= 0)
        return;

    int probeLen = len;
    while (probeLen > 0 && isAsciiSpace((unsigned char)word[probeLen - 1]))
        --probeLen;
    const float wrapWidth = (probeLen > 0) ? domMeasureStyledTextRange(word, probeLen, state) : 0.f;

    if (params.maxWidth > 0.f && wrapWidth > params.maxWidth)
    {
        if (sc.chunkCount > baseChunk)
            flushLine(state, true);
        trimLeadingIfLineEmpty();
        if (len <= 0)
            return;
        int pieceStart = 0;
        int pieceLen = 0;
        float pieceW = 0.f;
        int bi = 0;
        while (bi < len)
        {
            int cl = getUtf8CharLen(word + bi);
            int candLen = pieceLen + cl;
            float candW = domMeasureStyledTextRange(word + pieceStart, candLen, state);
            if (candW > params.maxWidth && pieceLen > 0)
            {
                pushChunk(word + pieceStart, pieceLen, pieceW, state);
                flushLine(state, true);
                pieceStart = bi;
                pieceLen = cl;
                pieceW = domMeasureStyledTextRange(word + pieceStart, pieceLen, state);
            }
            else
            {
                pieceLen = candLen;
                pieceW = candW;
            }
            bi += cl;
        }
        if (pieceLen > 0)
            pushChunk(word + pieceStart, pieceLen, pieceW, state);
        return;
    }

    if (params.maxWidth > 0.f &&
        totalWidth + wrapWidth > params.maxWidth &&
        sc.chunkCount > baseChunk)
    {
        flushLine(state, true);
        trimLeadingIfLineEmpty();
        if (len <= 0)
            return;
    }

    pushChunk(word, len, wordWidth, state);
}

void HTMLDom::RenderContext::flushLine(const HTMLRenderState& state, bool bAdvanceIfEmpty)
{
    HTMLRenderScratch& sc = *dom->_scratch;

    if (sc.chunkCount > baseChunk)
    {
        HTMLChunk& last = sc.chunks[sc.chunkCount - 1];
        if (!last.isImage && last.len > 0)
        {
            int q = last.len;
            while (q > 0 && isAsciiSpace((unsigned char)last.text[q - 1]))
                --q;
            if (q != last.len)
            {
                const HTMLRenderState& lst = sc.statePool[last.stateIndex];
                const float oldW = domMeasureStyledTextRange(last.text, last.len, lst);
                const float newW = (q > 0) ? domMeasureStyledTextRange(last.text, q, lst) : 0.f;
                totalWidth = std::max(0.f, totalWidth - (oldW - newW));
                last.len = q;
                if (q == 0)
                    --sc.chunkCount;
            }
        }
    }

    if (sc.chunkCount <= baseChunk)
    {
        sc.chunkCount = baseChunk;
        totalWidth = 0.f;
        if (bAdvanceIfEmpty)
            currentY += lineHeight;
        currentX = params.x;
        return;
    }

    const int firstChunk = baseChunk;
    const int chunkEnd = sc.chunkCount;

    float maxAscent = 0.f;
    float maxDescent = 0.f;
    float maxLineH = 0.f;
    for (int ci = firstChunk; ci < chunkEnd; ++ci)
    {
        const HTMLChunk& chunk = sc.chunks[ci];
        const HTMLRenderState& cst = sc.statePool[chunk.stateIndex];
        if (chunk.isImage)
        {
            float ia = 0.f;
            float idesc = 0.f;
            if (chunk.imgValign == 0)
            {
                ia = chunk.imgH;
            }
            else if (chunk.imgValign == 1)
            {
                ia = chunk.imgH * 0.5f;
                idesc = chunk.imgH * 0.5f;
            }
            else
            {
                idesc = chunk.imgH;
            }
            maxAscent = std::max(maxAscent, ia);
            maxDescent = std::max(maxDescent, idesc);
            continue;
        }
        int fh = (cst.fontHandle != -1) ? cst.fontHandle : state.fontHandle;
        fonsSetFont(fs, fh);
        fonsSetSize(fs, cst.fontSize);
        float a = 0.f;
        float d = 0.f;
        float lh = 0.f;
        fonsVertMetrics(fs, &a, &d, &lh);
        float mul = cst.lineHeightMul > 0.f ? cst.lineHeightMul : 1.f;
        maxAscent = std::max(maxAscent, a * mul);
        maxDescent = std::max(maxDescent, -d * mul);
        maxLineH = std::max(maxLineH, lh * mul);
    }
    maxLineH = std::max(maxLineH, maxAscent + maxDescent);
    if (maxLineH <= 0.f)
        maxLineH = lineHeight;

    float baselineY = currentY + maxAscent;
    float startX = params.x;
    int hAlign = sc.statePool[sc.chunks[firstChunk].stateIndex].align &
        (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT);
    if (hAlign == FONS_ALIGN_CENTER)
    {
        if (params.maxWidth > 0.f)
            startX = params.x + (params.maxWidth - totalWidth) * 0.5f;
    }
    else if (hAlign == FONS_ALIGN_RIGHT && params.maxWidth > 0.f)
    {
        startX = params.x + params.maxWidth - totalWidth;
    }

    float lineTop = currentY;
    float chunkX = startX;

    int totalCharsInLine = 0;
    for (int ci = firstChunk; ci < chunkEnd; ++ci)
    {
        const HTMLChunk& chunk = sc.chunks[ci];
        if (chunk.isImage)
            continue;
        const char* t = chunk.text;
        int tb = 0;
        while (tb < chunk.len)
        {
            tb += getUtf8CharLen(t + tb);
            totalCharsInLine++;
        }
    }

    int globalCharIndex = 0;
    float lastOverhang = 0.f;
    for (int ci = firstChunk; ci < chunkEnd; ++ci)
    {
        const HTMLChunk& chunk = sc.chunks[ci];
        const HTMLRenderState& cst = sc.statePool[chunk.stateIndex];
        unsigned long long chunkLinkId = sc.chunkLinkIds[ci];
        unsigned long long chunkFontAnimId = sc.chunkFontAnimIds[ci];
        const unsigned int chunkTint = chunk.colorMul;

        if (chunk.isImage)
        {
            float iy;
            if (chunk.imgValign == 0)
            {
                iy = lineTop;
            }
            else if (chunk.imgValign == 1)
            {
                int fhx = (cst.fontHandle != -1) ? cst.fontHandle : state.fontHandle;
                float b[4];
                fonsSetFont(fs, fhx);
                fonsSetSize(fs, cst.fontSize);
                fonsTextBounds(fs, 0.f, 0.f, "x", nullptr, b);
                float xh = -b[1];
                if (xh <= 0.f)
                    xh = (float)cst.fontSize * 0.5f;
                iy = baselineY - xh * 0.5f - chunk.imgH * 0.5f;
            }
            else
            {
                iy = lineTop + (maxAscent + maxDescent) - chunk.imgH;
            }
            const float ix = chunkX + chunk.imgMarginL;
            if (!measureOnly && chunk.imgTex)
            {
                auto imgNode = dom->lockNode(chunk.imgNodeId);
                if (imgNode && imgNode->idleStyle != eHTMLIdleStyle::None)
                {
                    bool destructiveIdle =
                        imgNode->idleStyle == eHTMLIdleStyle::Tear ||
                        imgNode->idleStyle == eHTMLIdleStyle::Lightning ||
                        imgNode->idleStyle == eHTMLIdleStyle::Ice ||
                        imgNode->idleStyle == eHTMLIdleStyle::Melt ||
                        imgNode->idleStyle == eHTMLIdleStyle::Dust ||
                        imgNode->idleStyle == eHTMLIdleStyle::Vortex ||
                        imgNode->idleStyle == eHTMLIdleStyle::Confetti ||
                        imgNode->idleStyle == eHTMLIdleStyle::Echo;
                    if (!destructiveIdle)
                    {
                        drawSpriteQuadColorAlpha(
                            chunk.imgTex,
                            ix,
                            iy,
                            chunk.imgW,
                            chunk.imgH,
                            chunk.imgUV[0],
                            chunk.imgUV[1],
                            chunk.imgUV[2],
                            chunk.imgUV[3],
                            chunk.imgColor,
                            1.f,
                            grayScale
                        );
                    }
                    HTMLNode hn = makeHTMLNodeForIdle(*imgNode);
                    const float idleNow = idleRestartTime(
                        animTime - imgNode->idleTimeOffset,
                        imgNode->idleDelay
                    );
                    switch (imgNode->idleStyle)
                    {
                    case eHTMLIdleStyle::Glare:
                        renderImgGlare(hn, ix, iy, chunk.imgW, chunk.imgH, idleNow);
                        break;
                    case eHTMLIdleStyle::Pixel:
                        renderImgPixel(hn, ix, iy, chunk.imgW, chunk.imgH, idleNow);
                        break;
                    case eHTMLIdleStyle::Chase:
                        renderImgChase(hn, ix, iy, chunk.imgW, chunk.imgH, idleNow);
                        break;
                    case eHTMLIdleStyle::Embers:
                        renderImgEmbers(hn, ix, iy, chunk.imgW, chunk.imgH, idleNow);
                        break;
                    case eHTMLIdleStyle::Lens:
                        renderImgLens(hn, ix, iy, chunk.imgW, chunk.imgH, animTime);
                        break;
                    case eHTMLIdleStyle::Echo:
                        renderImgEcho(hn, ix, iy, chunk.imgW, chunk.imgH, animTime);
                        break;
                    case eHTMLIdleStyle::Tear:
                        renderImgTear(hn, ix, iy, chunk.imgW, chunk.imgH, animTime);
                        break;
                    case eHTMLIdleStyle::Lightning:
                        renderImgLightning(hn, ix, iy, chunk.imgW, chunk.imgH, animTime);
                        break;
                    case eHTMLIdleStyle::Ice:
                        renderImgIce(hn, ix, iy, chunk.imgW, chunk.imgH, animTime);
                        break;
                    case eHTMLIdleStyle::Melt:
                        renderImgMelt(hn, ix, iy, chunk.imgW, chunk.imgH, animTime);
                        break;
                    case eHTMLIdleStyle::Dust:
                        renderImgDust(hn, ix, iy, chunk.imgW, chunk.imgH, animTime);
                        break;
                    case eHTMLIdleStyle::Vortex:
                        renderImgVortex(hn, ix, iy, chunk.imgW, chunk.imgH, animTime);
                        break;
                    case eHTMLIdleStyle::Confetti:
                        renderImgConfetti(hn, ix, iy, chunk.imgW, chunk.imgH, animTime);
                        break;
                    default:
                        break;
                    }
                    if (imgNode->idleDur > 0.f && !imgNode->imgAnimCompletedSent)
                    {
                        float imgLocal = animTime - imgNode->idleTimeOffset;
                        if (imgNode->idleDelay > 0.f)
                            imgLocal -= imgNode->idleDelay;
                        if (imgLocal >= imgNode->idleDur)
                        {
                            imgNode->imgAnimCompletedSent = true;
                            if (nodeHasIdAttr(imgNode))
                            {
                                if (auto cb = dom->getDocumentCallbacks(rootId))
                                {
                                    cb->onImageAnimEvent(
                                        rootId,
                                        imgNode->id,
                                        eHTMLImageAnimEventType::AnimationComplete,
                                        imgNode->idleStyle
                                    );
                                }
                            }
                        }
                    }
                }
                else
                {
                    drawSpriteQuadColorAlpha(
                        chunk.imgTex,
                        ix,
                        iy,
                        chunk.imgW,
                        chunk.imgH,
                        chunk.imgUV[0],
                        chunk.imgUV[1],
                        chunk.imgUV[2],
                        chunk.imgUV[3],
                        chunk.imgColor,
                        1.f,
                        grayScale
                    );
                }
            }
            updateBounds(ix, iy, chunk.imgW, chunk.imgH);
            unsigned long long imgHitId = chunkLinkId;
            if (imgHitId == 0)
                imgHitId = currentCursorId;
            if (!measureOnly && imgHitId != 0)
            {
                dom->registerHitRect(
                    rootId,
                    imgHitId,
                    chunkX,
                    lineTop,
                    chunk.imgW + chunk.imgMarginL,
                    maxLineH
                );
            }
            chunkX += chunk.imgW + chunk.imgMarginL;
            continue;
        }

        if (cst.bgColor != 0 && !measureOnly)
        {
            float bgW = domMeasureStyledTextRange(chunk.text, chunk.len, cst);
            if (bgW > 0.f)
            {
                unsigned int bg = domMulColor(cst.bgColor, chunkTint);
                if (grayScale)
                    bg = domGrayColor(bg);
                renderLine(chunkX, lineTop, bgW, maxLineH, domMulColor(bg, documentColor));
            }
        }

        int fh = (cst.fontHandle != -1) ? cst.fontHandle : state.fontHandle;
        fonsSetFont(fs, fh);
        fonsSetSize(fs, cst.fontSize);
        fonsSetColor(fs, domMulColor(cst.color, chunkTint));
        float a = 0.f;
        float d = 0.f;
        float lh = 0.f;
        fonsVertMetrics(fs, &a, &d, &lh);
        float chunkTop = baselineY - a;
        const char* text = chunk.text;
        float charX = chunkX;
        float chunkStartX = chunkX;
        int byteIdx = 0;
        const unsigned int chunkTextColor = domMulColor(cst.color, chunkTint);
        while (byteIdx < chunk.len)
        {
            int charLen = getUtf8CharLen(text + byteIdx);
            char ch[5] = { 0 };
            memcpy(ch, text + byteIdx, charLen);
            float w = renderChar(
                ch,
                charX,
                chunkTop,
                cst.fontSize,
                chunkTextColor,
                chunkTint,
                cst,
                globalCharIndex,
                totalCharsInLine,
                chunkFontAnimId
            );
            float overhang = cst.fakeItalic
                ? htmlFakeItalicExtraAdvance(cst.fontSize)
                : 0.f;
            lastOverhang = overhang;
            updateBounds(charX, lineTop, w + overhang, maxLineH);
            charX += w;
            byteIdx += charLen;
            globalCharIndex++;
        }

        if (!measureOnly &&
            (cst.underline || cst.strikethrough) &&
            charX > chunkStartX)
        {
            float thickness = cst.lineThickness > 0.f
                ? cst.lineThickness
                : (float)cst.fontSize / 12.f;
            thickness = std::max(thickness, 1.f);
            unsigned int lineCol = domMulColor(
                mulAlpha(domMulColor(cst.lineColor, chunkTint), (float)(chunkTextColor & 0xFFu) / 255.f),
                documentColor
            );
            if (grayScale)
                lineCol = domGrayColor(lineCol);
            if (cst.underline)
            {
                renderLine(
                    chunkStartX,
                    domSnap(chunkTop + lh * 0.9f),
                    charX - chunkStartX,
                    thickness,
                    lineCol
                );
            }
            if (cst.strikethrough)
            {
                renderLine(
                    chunkStartX,
                    domSnap(chunkTop + lh * 0.5f),
                    charX - chunkStartX,
                    thickness,
                    lineCol
                );
            }
        }

        unsigned long long chunkHitId = chunkLinkId;
        if (chunkHitId == 0)
            chunkHitId = currentCursorId;
        if (!measureOnly && chunkHitId != 0 && charX > chunkStartX)
        {
            dom->registerHitRect(
                rootId,
                chunkHitId,
                chunkStartX,
                lineTop,
                charX - chunkStartX,
                maxLineH
            );
        }
        chunkX = charX;
    }

    float lineW = totalWidth + lastOverhang;
    if (lineW > maxContentWidth)
        maxContentWidth = lineW;

    currentY += maxLineH;
    sc.chunkCount = baseChunk;
    totalWidth = 0.f;
    currentX = params.x;
    lineHeight = domGetLineHeightForState(state);
}

HTMLDom::~HTMLDom()
{
    delete _scratch;
}

HTMLRenderScratch* HTMLDom::ensureScratch()
{
    if (!_scratch)
        _scratch = new HTMLRenderScratch();
    return _scratch;
}

static bool htmlStateEqual(const HTMLRenderState& a, const HTMLRenderState& b)
{
    if (a.fontSize != b.fontSize) return false;
    if (a.color != b.color) return false;
    if (a.fontHandle != b.fontHandle) return false;
    if (a.align != b.align) return false;
    if (a.bold != b.bold) return false;
    if (a.italic != b.italic) return false;
    if (a.fakeBold != b.fakeBold) return false;
    if (a.fakeItalic != b.fakeItalic) return false;
    if (a.underline != b.underline) return false;
    if (a.strikethrough != b.strikethrough) return false;
    if (a.hasGradient != b.hasGradient) return false;
    if (a.fxStyle != b.fxStyle) return false;
    if (a.appearStyle != b.appearStyle) return false;
    if (a.baseFontHandle != b.baseFontHandle) return false;
    if (a.shadow != b.shadow) return false;
    if (a.shadowColor != b.shadowColor) return false;
    if (a.outline != b.outline) return false;
    if (a.outlineColor != b.outlineColor) return false;
    if (a.lineColor != b.lineColor) return false;
    if (a.lineThickness != b.lineThickness) return false;
    if (a.gradientColor1 != b.gradientColor1) return false;
    if (a.gradientColor2 != b.gradientColor2) return false;
    if (a.appearDur != b.appearDur) return false;
    if (a.appearDelay != b.appearDelay) return false;
    if (a.fxDelay != b.fxDelay) return false;
    if (a.lineHeightMul != b.lineHeightMul) return false;
    if (a.bgColor != b.bgColor) return false;
    return a.fontName == b.fontName;
}

int HTMLDom::internRenderState(const HTMLRenderState& st)
{
    HTMLRenderScratch& s = *_scratch;
    if (s.lastStateIndex >= 0 && htmlStateEqual(s.statePool[s.lastStateIndex], st))
        return s.lastStateIndex;
    int idx = s.stateCount;
    if (idx < (int)s.statePool.size())
    {
        if (htmlStateEqual(s.statePool[idx], st))
        {
            s.lastStateIndex = idx;
            ++s.stateCount;
            return idx;
        }
        s.statePool[idx] = st;
    }
    else
    {
        s.statePool.push_back(st);
    }
    s.lastStateIndex = idx;
    ++s.stateCount;
    return idx;
}

HTMLDom* HTMLDom::getInstance(void)
{
    static HTMLDom* res = new HTMLDom();
    return res;
}

void HTMLDom::ensureRawNodeCache(const NodePtr& node)
{
    if (!node)
        return;
    if (node->id == 0)
        return;
    if (node->id >= (unsigned long long)_nodeRawById.size())
        _nodeRawById.resize((size_t)node->id + 1, nullptr);
    _nodeRawById[(size_t)node->id] = node.get();
}

unsigned long long HTMLDom::newId(void)
{
    return _nextNodeId++;
}

HTMLDom::NodePtr HTMLDom::lockNode(unsigned long long id) const
{
    auto it = _nodes.find(id);
    if (it == _nodes.end())
        return nullptr;
    return it->second.lock();
}

HTMLDom::NodePtr HTMLDom::getNode(unsigned long long id)
{
    auto it = _nodes.find(id);
    if (it == _nodes.end())
        return nullptr;
    auto p = it->second.lock();
    if (!p)
    {
        _nodes.erase(it);
        return nullptr;
    }
    return p;
}

unsigned long long HTMLDom::getParent(unsigned long long id) const
{
    auto node = lockNode(id);
    if (!node)
        return 0;
    return node->parentId;
}

std::vector<unsigned long long> HTMLDom::getChildren(unsigned long long id) const
{
    std::vector<unsigned long long> res;
    auto node = lockNode(id);
    if (!node)
        return res;
    res.reserve(node->children.size());
    for (const auto& child : node->children)
        if (child)
            res.push_back(child->id);
    return res;
}

void HTMLDom::registerNodeRecursive(const NodePtr& node)
{
    if (!node)
        return;
    if (node->id == 0)
        node->id = newId();
    _nodes[node->id] = node;
    ensureRawNodeCache(node);
    if (node->tag == eHTMLTag::NineButton && !node->btnGroup.empty())
        ++_btnGroupCount;
    resolveNodeClassAttrs(*node);
    cacheRuntimeAttrs(*node);
    for (auto& child : node->children)
    {
        if (!child)
            continue;
        child->parentId = node->id;
        registerNodeRecursive(child);
    }
}

void HTMLDom::unregisterNodeRecursive(const NodePtr& node)
{
    if (!node)
        return;

    node->tweenGen++;
    if (node->tag == eHTMLTag::NineButton && !node->btnGroup.empty())
        --_btnGroupCount;
    _particlesSystems.erase(node->id);
    _domButtonFxMap.erase(node->id);
    _domFgFxMap.erase(node->id);
#ifdef HAS_SPINE
    _spineKeyByNode.erase(node->id);
#endif
    _fontAnimEventFlags.erase(node->id);
    if (node->id < (unsigned long long)_nodeRawById.size())
        _nodeRawById[(size_t)node->id] = nullptr;
    _nodes.erase(node->id);
    for (auto& child : node->children)
        unregisterNodeRecursive(child);
}

void HTMLDom::attachNode(const NodePtr& node, const NodePtr& parent, int index)
{
    if (!node || !parent)
        return;
    if (node->parentId != 0 && node->parentId != parent->id)
        detachNode(node);
    node->parentId = parent->id;
    if (index < 0 || index >= (int)parent->children.size())
        parent->children.push_back(node);
    else
        parent->children.insert(parent->children.begin() + index, node);
}

void HTMLDom::detachNode(const NodePtr& node)
{
    if (!node)
        return;
    auto parent = lockNode(node->parentId);
    if (!parent)
        return;
    auto& children = parent->children;
    for (auto it = children.begin(); it != children.end(); ++it)
    {
        if (*it && (*it)->id == node->id)
        {
            children.erase(it);
            break;
        }
    }
    node->parentId = 0;
}

HTMLDom::NodePtr HTMLDom::parseFragmentInternal(const char* lpszHTML, unsigned long long parentId)
{
    if (!lpszHTML)
        return nullptr;

    std::string localizedBuf = lpszHTML;
    domSubstituteLocalization(localizedBuf);
    lpszHTML = localizedBuf.c_str();
    auto root = std::make_shared<HTMLDomNode>();
    root->tag = eHTMLTag::Document;
    root->id = newId();
    root->parentId = parentId;
    struct StackEntry
    {
        HTMLDomNode* p = nullptr;
        size_t contentStart = 0;
    };
    std::vector<StackEntry> stack;
    stack.push_back({ root.get(), 0 });
    size_t len = strlen(lpszHTML);
    size_t i = 0;
    std::string currentText;
    auto flushText = [&]()
    {
        if (currentText.empty())
            return;
        std::string decoded = domDecodeTextPreserveNbsp(currentText);
        if (!decoded.empty())
        {
            auto textNode = std::make_shared<HTMLDomNode>();
            textNode->tag = eHTMLTag::Text;
            textNode->text = decoded;
            textNode->id = newId();
            HTMLDomNode* parent = stack.back().p;
            textNode->parentId = parent->id;
            parent->children.push_back(textNode);
        }
        currentText.clear();
    };
    while (i < len)
    {
        if (lpszHTML[i] == '<')
        {

            if (i + 3 < len &&
                lpszHTML[i + 1] == '!' &&
                lpszHTML[i + 2] == '-' &&
                lpszHTML[i + 3] == '-')
            {
                const char* pCommentEnd = strstr(lpszHTML + i + 4, "-->");
                if (!pCommentEnd)
                    break;
                i = (size_t)(pCommentEnd - lpszHTML) + 3;
                continue;
            }
            flushText();
            const char* tagEndPtr = strchr(lpszHTML + i, '>');
            if (!tagEndPtr)
                break;
            size_t tagEnd = (size_t)(tagEndPtr - lpszHTML);
            std::string tagStr(lpszHTML + i, tagEnd - i + 1);
            std::string tagName;
            HTMLAttrsMap attrs;
            bool isClosing = false, selfClosing = false;
            if (parseDomTag(tagStr, tagName, attrs, isClosing, selfClosing))
            {
                eHTMLTag tag = parseTagEnum(tagName);
                if (tag != eHTMLTag::Unknown)
                {
                    if (isClosing)
                    {
                        for (int si = (int)stack.size() - 1; si >= 1; --si)
                        {
                            if (stack[si].p->tag == tag)
                            {
                                if (tag == eHTMLTag::Option && stack[si].p->innerHTML.empty())
                                    stack[si].p->innerHTML.assign(lpszHTML + stack[si].contentStart, i - stack[si].contentStart);
                                stack.resize((size_t)si);
                                break;
                            }
                        }
                    }
                    else
                    {
                        auto node = std::make_shared<HTMLDomNode>();
                        node->tag = tag;
                        node->id = newId();
                        node->attrs = attrs;
                        parseNodeAttrs(*node.get());
                        HTMLDomNode* parent = stack.back().p;
                        node->parentId = parent->id;
                        parent->children.push_back(node);
                        bool voidTag =
                            tag == eHTMLTag::Br || tag == eHTMLTag::Hr ||
                            tag == eHTMLTag::Img || tag == eHTMLTag::Slider ||
                            tag == eHTMLTag::ProgressBar;
                        if (!voidTag && !selfClosing)
                            stack.push_back({ parent->children.back().get(), tagEnd + 1 });
                    }
                }
            }
            i = tagEnd + 1;
        }
        else
        {
            currentText += lpszHTML[i];
            i++;
        }
    }
    flushText();
    domTrimNode(root.get());
    registerNodeRecursive(root);
    return root;
}

unsigned long long HTMLDom::parse(const char* lpszHTML)
{
    NodePtr root = parseFragmentInternal(lpszHTML, 0);
    if (!root)
        return 0;
    root->parentId = 0;
    _roots[root->id] = root;
    _rootOptions[root->id] = HTMLDomOptions{};
    _lastRootId = root->id;
    return root->id;
}

void HTMLDom::refreshNode(HTMLDomNode& node)
{
    domRebuildResolvedAttrs(node);
    node.attrsResolvedInited = true;
    bool wasBtnGroup = (node.tag == eHTMLTag::NineButton && !node.btnGroup.empty());
    parseNodeAttrs(node);
    bool nowBtnGroup = (node.tag == eHTMLTag::NineButton && !node.btnGroup.empty());
    if (wasBtnGroup != nowBtnGroup)
        _btnGroupCount += nowBtnGroup ? 1 : -1;
    cacheRuntimeAttrs(node);
}

void HTMLDom::parseCSSFromFile(const char* lpszCSSFileName)
{

    auto s = std::filesystem::current_path();

    std::string out;
    FILE* f = std::fopen(lpszCSSFileName, "rb");
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
    parseCSS(out.c_str());
}
void HTMLDom::parseCSS(const char* lpszCSS)
{
    _cssClasses.clear();
    _cssClassOrder.clear();
    if (!lpszCSS || !*lpszCSS)
    {
        rebuildAllCssAttrs();
        return;
    }
    std::string text = lpszCSS;
    domSubstituteLocalization(text);
    HTMLCssTextParser tp(text.c_str());
    for (;;)
    {
        tp.skipWs();
        if (!*tp.p)
            break;
        if (*tp.p != '.')
        {

            ++tp.p;
            continue;
        }
        ++tp.p;
        std::string name;
        if (HTMLCssTextParser::identChar(*tp.p))
        {
            const char* s = tp.p;
            while (*tp.p && HTMLCssTextParser::identChar(*tp.p))
                ++tp.p;
            name.assign(s, tp.p - s);
        }
        tp.skipWs();
        if (*tp.p != '{')
        {
            ++tp.p;
            continue;
        }
        ++tp.p;
        HTMLCssClassRule rule;
        domParseCssDeclarations(tp, rule);
        if (name.empty())
            continue;
        auto it = _cssClasses.find(name);
        if (it == _cssClasses.end())
        {
            _cssClasses.emplace(name, std::move(rule));
            _cssClassOrder.push_back(name);
        }
        else
        {

            it->second = std::move(rule);
        }
    }
    rebuildAllCssAttrs();
}

void HTMLDom::clearCSS()
{
    _cssClasses.clear();
    _cssClassOrder.clear();
    rebuildAllCssAttrs();
}

bool HTMLDom::addCssClass(const char* lpszName, const char* lpszDecls)
{
    if (!lpszName || !*lpszName || !lpszDecls)
        return false;
    std::string name = lpszName[0] == '.' ? lpszName + 1 : lpszName;
    std::string text = lpszDecls;
    domSubstituteLocalization(text);
    HTMLCssTextParser tp(text.c_str());
    tp.skipWs();
    if (*tp.p == '{')
        ++tp.p;
    HTMLCssClassRule rule;
    domParseCssDeclarations(tp, rule);
    auto it = _cssClasses.find(name);
    if (it == _cssClasses.end())
    {
        _cssClasses.emplace(name, std::move(rule));
        _cssClassOrder.push_back(name);
    }
    else
    {
        it->second = std::move(rule);
    }
    rebuildAllCssAttrs();
    return true;
}

bool HTMLDom::removeCssClass(const char* lpszName)
{
    if (!lpszName || !*lpszName)
        return false;
    std::string name = lpszName[0] == '.' ? lpszName + 1 : lpszName;
    bool removed = _cssClasses.erase(name) != 0;
    if (removed)
    {
        _cssClassOrder.erase(
            std::remove(_cssClassOrder.begin(), _cssClassOrder.end(), name),
            _cssClassOrder.end());
        rebuildAllCssAttrs();
    }
    return removed;
}

bool HTMLDom::hasCssClass(const char* lpszName) const
{
    if (!lpszName || !*lpszName)
        return false;
    std::string name = lpszName[0] == '.' ? lpszName + 1 : lpszName;
    return _cssClasses.find(name) != _cssClasses.end();
}

void HTMLDom::rebuildAllCssAttrs()
{
    for (auto& kv : _roots)
        resolveClassAttrsRecursive(kv.second);
}

void HTMLDom::resolveClassAttrsRecursive(const NodePtr& node)
{
    if (!node)
        return;
    resolveNodeClassAttrs(*node);
    for (const auto& child : node->children)
        resolveClassAttrsRecursive(child);
}

void HTMLDom::resolveNodeClassAttrs(HTMLDomNode& node)
{
    const bool bHadClassAttrs = !node.attrsClass.empty();
    node.attrsClass.clear();
    node.attrsClassInheritAll = false;
    if (!_cssClasses.empty())
    {
        const std::string* pClass = node.attrs.find("class");
        if (pClass && !pClass->empty())
        {

            for (const auto& ruleName : _cssClassOrder)
            {
                if (!cssClassListContains(*pClass, ruleName))
                    continue;
                auto it = _cssClasses.find(ruleName);
                if (it == _cssClasses.end())
                    continue;
                for (const auto& d : it->second.decls)
                {
                    bool found = false;
                    for (auto& kv : node.attrsClass)
                    {
                        if (kv.first == d.name)
                        {
                            kv.second = d.value;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        node.attrsClass.emplace_back(d.name, d.value);
                }
                if (it->second.inheritAll)
                    node.attrsClassInheritAll = true;
            }
        }

        unsigned long long pid = node.parentId;
        while (pid != 0)
        {
            auto p = lockNode(pid);
            if (!p)
                break;
            if (!p->attrsClass.empty())
            {
                for (const auto& kv : p->attrsClass)
                {
                    if (!p->attrsClassInheritAll && !cssPropInherits(kv.first))
                        continue;
                    if (node.attrs.find(kv.first.c_str()))
                        continue;
                    if (node.findClassAttr(kv.first.c_str()))
                        continue;
                    node.attrsClass.emplace_back(kv.first, kv.second);
                }
            }
            pid = p->parentId;
        }
    }
    domRebuildResolvedAttrs(node);
    node.attrsResolvedInited = true;

    if (!node.attrsClass.empty() || bHadClassAttrs)
    {
        parseNodeAttrs(node);
        cacheRuntimeAttrs(node);
    }
}

bool HTMLDom::cssPropInherits(const std::string& name)
{

    static const std::unordered_set<std::string> props = {
        "color",
        "font", "name", "fontsize", "size",
        "bold", "italic",
        "lineheight", "line-height",
        "shadow", "shadowcolor", "outline", "outlinecolor", "gradient",
        "fx", "appear",
        "dur", "duration", "time", "appearduration",
        "delay", "appeardelay", "fxdelay", "repeatdelay",
        "align", "cursor", "visible",
    };
    return props.count(name) != 0;
}

bool HTMLDom::cssClassListContains(const std::string& list, const std::string& name)
{
    size_t i = 0;
    while (i < list.size())
    {
        while (i < list.size() && isAsciiSpace((unsigned char)list[i]))
            ++i;
        size_t s = i;
        while (i < list.size() && !isAsciiSpace((unsigned char)list[i]))
            ++i;
        if (i > s && list.compare(s, i - s, name) == 0)
            return true;
    }
    return false;
}

void HTMLDom::setRootOptions(unsigned long long rootId, const HTMLDomOptions& options)
{
    if (rootId == 0)
        return;
    _rootOptions[rootId] = options;
}

HTMLDomOptions HTMLDom::getRootOptions(unsigned long long rootId) const
{
    auto it = _rootOptions.find(rootId);
    if (it != _rootOptions.end())
        return it->second;
    return HTMLDomOptions{};
}

HTMLDomOptions* HTMLDom::getRootOptionsPtr(unsigned long long rootId)
{
    if (rootId == 0)
        return nullptr;
    auto it = _rootOptions.find(rootId);
    return (it != _rootOptions.end()) ? &it->second : nullptr;
}

unsigned long long HTMLDom::createNode(eHTMLTag tag, unsigned long long parentId, int index)
{
    auto parent = getNode(parentId);
    if (!parent)
        return 0;
    auto node = std::make_shared<HTMLDomNode>();
    node->tag = tag;
    node->id = newId();
    if (tag == eHTMLTag::A || tag == eHTMLTag::NineButton || tag == eHTMLTag::Checkbox ||
        tag == eHTMLTag::Select || tag == eHTMLTag::Option || tag == eHTMLTag::Slider ||
        tag == eHTMLTag::Edit)
    {
        node->clickable = true;
    }
    updateEffectiveCursor(*node);
    attachNode(node, parent, index);
    _nodes[node->id] = node;
    ensureRawNodeCache(node);
    resolveNodeClassAttrs(*node);
    cacheRuntimeAttrs(*node);
    return node->id;
}

std::vector<unsigned long long> HTMLDom::insertHTML(unsigned long long parentId, const char* html, int index)
{
    std::vector<unsigned long long> inserted;
    auto parent = getNode(parentId);
    if (!parent || !html)
        return inserted;
    NodePtr fragment = parseFragmentInternal(html, parentId);
    if (!fragment)
        return inserted;
    int insertIndex = index;
    for (auto& child : fragment->children)
    {
        if (!child)
            continue;
        child->parentId = parentId;
        if (insertIndex < 0 || insertIndex >= (int)parent->children.size())
        {
            parent->children.push_back(child);
            insertIndex = (int)parent->children.size();
        }
        else
        {
            parent->children.insert(parent->children.begin() + insertIndex, child);
            insertIndex++;
        }

        resolveClassAttrsRecursive(child);
        inserted.push_back(child->id);
    }
    fragment->children.clear();
    return inserted;
}

bool HTMLDom::removeNode(unsigned long long id)
{
    auto node = getNode(id);
    if (!node)
        return false;
    if (_dragSliderId != 0 && (id == _dragSliderId || id == _dragRootId))
        endSliderDrag(true);
    if (_focusedEditId != 0 && domNodeContainsId(node, _focusedEditId))
        blurFocusedEdit();
    if (_editDragId != 0 && domNodeContainsId(node, _editDragId))
        stopEditDrag(true);
    if (_cursorTargetId == id)
    {
        unsigned long long oldRoot = _cursorRootId;
        if (auto cb = getDocumentCallbacks(oldRoot))
        {
            cb->onLeave(oldRoot, id);
            if (_currentCursor != "normal")
                cb->onCursorChanged("normal");
        }
        _cursorRootId = 0;
        _cursorTargetId = 0;
        _currentCursor = "normal";
    }
    if (_roots.find(id) != _roots.end())
    {
        if (_cursorRootId == id)
        {
            if (_cursorTargetId != 0 && _cursorTargetId != id)
            {
                if (auto cb = getDocumentCallbacks(id))
                {
                    cb->onLeave(id, _cursorTargetId);
                    if (_currentCursor != "normal")
                        cb->onCursorChanged("normal");
                }
            }
            _cursorRootId = 0;
            _cursorTargetId = 0;
            _currentCursor = "normal";
        }
        _roots.erase(id);
        _rootOptions.erase(id);
        _hitData.erase(id);
        _rootCallbacks.erase(id);
        _hostContainers.erase(id);
        _pausedTweenRoots.erase(id);
        removeAppearState(id);
        if (_lastRootId == id)
            _lastRootId = 0;
        if (_pressedRootId == id)
            clearInputState();
    }
    else
    {
        detachNode(node);
    }
    unregisterNodeRecursive(node);
    return true;
}

bool HTMLDom::moveNode(unsigned long long id, unsigned long long newParentId, int index)
{
    auto node = getNode(id);
    auto newParent = getNode(newParentId);
    if (!node || !newParent)
        return false;
    unsigned long long checkId = newParentId;
    while (checkId != 0)
    {
        if (checkId == id)
            return false;
        auto checkNode = getNode(checkId);
        if (!checkNode)
            break;
        checkId = checkNode->parentId;
    }
    detachNode(node);
    attachNode(node, newParent, index);
    resolveClassAttrsRecursive(node);
    return true;
}

bool HTMLDom::setTextContent(unsigned long long id, const char* text)
{
    auto node = getNode(id);
    if (!node)
        return false;
    if (node->tag == eHTMLTag::Text)
    {
        node->text = text ? text : " ";
        return true;
    }
    for (auto& child : node->children)
        unregisterNodeRecursive(child);
    node->children.clear();
    auto textNode = std::make_shared<HTMLDomNode>();
    textNode->tag = eHTMLTag::Text;
    textNode->id = newId();
    textNode->parentId = node->id;
    textNode->text = text ? text : " ";
    node->children.push_back(textNode);
    _nodes[textNode->id] = textNode;
    ensureRawNodeCache(textNode);
    return true;
}

bool HTMLDom::setInnerHTML(unsigned long long id, const char* html)
{
    auto node = getNode(id);
    if (!node)
        return false;
    for (auto& child : node->children)
    {
        if (_dragSliderId != 0 && domNodeContainsId(child, _dragSliderId))
            endSliderDrag(true);
        if (_editDragId != 0 && domNodeContainsId(child, _editDragId))
            stopEditDrag(true);
        if (_cursorTargetId != 0 && domNodeContainsId(child, _cursorTargetId))
        {
            if (auto cb = getDocumentCallbacks(_cursorRootId))
            {
                cb->onLeave(_cursorRootId, _cursorTargetId);
                if (_currentCursor != "normal")
                    cb->onCursorChanged("normal");
            }
            _cursorTargetId = 0;
            _cursorRootId = 0;
            _currentCursor = "normal";
        }
        if (_hoverTargetId != 0 && domNodeContainsId(child, _hoverTargetId))
        {
            _hoverTargetId = 0;
            _hoverRootId = 0;
        }
        if (_fgHoverTargetId != 0 && domNodeContainsId(child, _fgHoverTargetId))
            _fgHoverTargetId = 0;
        if (_fgPressedTargetId != 0 && domNodeContainsId(child, _fgPressedTargetId))
            _fgPressedTargetId = 0;
        if (_pressedTargetId != 0 && domNodeContainsId(child, _pressedTargetId))
        {
            _pressedTargetId = 0;
            _pressedRootId = 0;
            _mouseButtonDown = false;
            _pressedButton = 0;
        }
        unregisterNodeRecursive(child);
    }
    node->children.clear();
    if (html && *html)
        insertHTML(id, html, -1);
    return true;
}

bool HTMLDom::setAttr(unsigned long long id, const char* name, const char* value)
{
    auto node = getNode(id);
    if (!node || !name || !value)
        return false;
    auto p = node->attrs.find(name);
    if (p)
        *p = value;
    else
        node->attrs.insert(name, value);

    resolveClassAttrsRecursive(node);
    refreshNode(*node);
    refreshCurrentCursor();
    return true;
}

const char* HTMLDom::getAttr(unsigned long long id, const char* name)
{
    auto node = getNode(id);
    if (!node || !name)
        return nullptr;

    auto p = node->findEffectiveAttr(name);
    return p ? p->c_str() : nullptr;
}

bool HTMLDom::removeAttr(unsigned long long id, const char* name)
{
    auto node = getNode(id);
    if (!node || !name)
        return false;
    std::string lower = domToLower(name);
    node->attrs.erase(lower);
    if (lower != name)
        node->attrs.erase(name);
    if (lower == "disabled")
        node->disabled = false;
    if (lower == "disabledtint" || lower == "disabledcolor")
    {
        node->hasDisabledTint = false;
        node->disabledTint = 0xFFFFFFFF;
    }
    resolveClassAttrsRecursive(node);
    refreshNode(*node);
    refreshCurrentCursor();
    return true;
}

HTMLDom::NodePtr HTMLDom::findNodeByAttrRecursive(const NodePtr& node, const char* attrName, const char* value) const
{
    if (!node || !attrName || !value)
        return nullptr;
    if (auto p = node->attrs.find(attrName))
        if (*p == value)
            return node;
    for (const auto& child : node->children)
    {
        auto found = findNodeByAttrRecursive(child, attrName, value);
        if (found)
            return found;
    }
    return nullptr;
}

HTMLDom::NodePtr HTMLDom::findNodeByAttr(unsigned long long rootId, const char* attrName, const char* value)
{
    if (!attrName || !value)
        return nullptr;
    if (rootId != 0)
    {
        auto root = getNode(rootId);
        return findNodeByAttrRecursive(root, attrName, value);
    }
    for (auto& kv : _roots)
    {
        auto found = findNodeByAttrRecursive(kv.second, attrName, value);
        if (found)
            return found;
    }
    return nullptr;
}

unsigned long long HTMLDom::findNodeIdByAttr(unsigned long long rootId, const char* attrName, const char* value)
{
    auto node = findNodeByAttr(rootId, attrName, value);
    return node ? node->id : 0;
}

HTMLDom::NodePtr HTMLDom::getElementById(unsigned long long rootId, const char* id)
{
    return findNodeByAttr(rootId, "id", id);
}

unsigned long long HTMLDom::getElementIdById(unsigned long long rootId, const char* id)
{
    auto node = getElementById(rootId, id);
    return node ? node->id : 0;
}

bool HTMLDom::setOnClick(unsigned long long id, HTMLDomNode::ClickHandler handler)
{
    auto node = getNode(id);
    if (!node)
        return false;
    node->onClick = handler;
    node->clickable = true;
    updateEffectiveCursor(*node);
    refreshCurrentCursor();
    return true;
}

bool HTMLDom::isVisible(unsigned long long nodeId) const
{
    auto node = lockNode(nodeId);
    if (!node)
        return false;
    return node->visible;
}

bool HTMLDom::setVisible(unsigned long long nodeId, bool visible)
{
    auto node = getNode(nodeId);
    if (!node)
        return false;
    node->visible = visible;
    if (!visible && _dragSliderId == nodeId)
        endSliderDrag(true);
    if (!visible && _focusedEditId == nodeId)
        blurFocusedEdit();
    if (!visible && _editDragId != 0 && domNodeContainsId(node, _editDragId))
        stopEditDrag(true);
    if (!visible && _cursorTargetId == nodeId)
    {
        unsigned long long oldRoot = _cursorRootId;
        if (auto cb = getDocumentCallbacks(oldRoot))
        {
            cb->onLeave(oldRoot, nodeId);
            if (_currentCursor != "normal")
                cb->onCursorChanged("normal");
        }
        _cursorTargetId = 0;
        _cursorRootId = 0;
        _currentCursor = "normal";
    }
    if (!visible && _hoverTargetId == nodeId)
    {
        _hoverTargetId = 0;
        _hoverRootId = 0;
    }
    if (!visible && _fgHoverTargetId == nodeId)
        _fgHoverTargetId = 0;
    if (!visible && _fgPressedTargetId == nodeId)
        _fgPressedTargetId = 0;
    if (!visible && _pressedTargetId == nodeId)
    {
        _pressedTargetId = 0;
        _pressedRootId = 0;
        _mouseButtonDown = false;
        _pressedButton = 0;
    }
    return true;
}

bool HTMLDom::isDisabled(unsigned long long nodeId) const
{
    auto node = lockNode(nodeId);
    if (!node)
        return false;
    return node->disabled;
}

bool HTMLDom::setDisabled(unsigned long long nodeId, bool disabled)
{
    auto node = getNode(nodeId);
    if (!node)
        return false;
    if (node->disabled == disabled)
        return true;
    node->disabled = disabled;
    updateEffectiveCursor(*node);
    if (disabled)
    {
        if (_dragSliderId == nodeId)
            endSliderDrag(true);
        if (_focusedEditId == nodeId)
            blurFocusedEdit();
        if (_editDragId != 0 && domNodeContainsId(node, _editDragId))
            stopEditDrag(true);
        if (_hoverTargetId == nodeId)
        {
            _hoverTargetId = 0;
            _hoverRootId = 0;
        }
        if (_fgHoverTargetId == nodeId)
            _fgHoverTargetId = 0;
        if (_fgPressedTargetId == nodeId)
            _fgPressedTargetId = 0;
        if (_pressedTargetId == nodeId)
        {
            _pressedTargetId = 0;
            _pressedRootId = 0;
            _mouseButtonDown = false;
            _pressedButton = 0;
        }
        if (_cursorTargetId == nodeId)
        {
            unsigned long long oldRoot = _cursorRootId;
            if (auto cb = getDocumentCallbacks(oldRoot))
            {
                cb->onLeave(oldRoot, nodeId);
                if (_currentCursor != "normal")
                    cb->onCursorChanged("normal");
            }
            _cursorTargetId = 0;
            _cursorRootId = 0;
            _currentCursor = "normal";
        }
    }
    unsigned long long rootId = findRootIdForNode(node);
    if (rootId != 0)
        updateCursorHover(rootId, findCursorTarget(rootId, _lastMouseX, _lastMouseY));
    else
        refreshCurrentCursor();
    return true;
}

unsigned int HTMLDom::getDisabledTint(unsigned long long nodeId) const
{
    auto node = lockNode(nodeId);
    if (!node)
        return 0xFFFFFFFF;
    return domNodeDisabledTint(*node);
}

bool HTMLDom::setDisabledTint(unsigned long long nodeId, unsigned int tint)
{
    auto node = getNode(nodeId);
    if (!node)
        return false;
    node->hasDisabledTint = true;
    node->disabledTint = tint;
    return true;
}

bool HTMLDom::isChecked(unsigned long long nodeId) const
{
    auto node = lockNode(nodeId);
    if (!node)
        return false;
    if (node->tag != eHTMLTag::Checkbox)
        return false;
    return node->checked;
}

bool HTMLDom::setChecked(unsigned long long nodeId, bool checked)
{
    auto node = getNode(nodeId);
    if (!node)
        return false;
    if (node->tag != eHTMLTag::Checkbox)
        return false;
    node->checked = checked;
    return true;
}

bool HTMLDom::toggleChecked(unsigned long long nodeId)
{
    auto node = getNode(nodeId);
    if (!node)
        return false;
    if (node->tag != eHTMLTag::Checkbox)
        return false;
    node->checked = !node->checked;
    return true;
}

void HTMLDom::setHostContainer(unsigned long long rootId, std::weak_ptr<CContainer> host)
{
    if (rootId == 0)
        return;
    if (host.lock())
        _hostContainers[rootId] = host;
    else
        _hostContainers.erase(rootId);
}

std::weak_ptr<CContainer> HTMLDom::getHostContainer(unsigned long long rootId) const
{
    auto it = _hostContainers.find(rootId);
    if (it == _hostContainers.end())
        return {};
    return it->second;
}

unsigned long long HTMLDom::findRootIdForNode(const NodePtr& node) const
{
    if (!node)
        return 0;
    unsigned long long id = node->id;
    while (id != 0)
    {
        if (_roots.find(id) != _roots.end())
            return id;
        auto n = lockNode(id);
        if (!n)
            break;
        id = n->parentId;
    }
    return 0;
}

void HTMLDom::resolveOptionFontAttrsInto(const HTMLDomNode* opt, unsigned long long selectId, HTMLFontAttrs& res) const
{
    res = opt->fontAttrs;
    unsigned long long pid = opt->parentId;
    while (pid != 0 && pid != selectId)
    {
        auto pn = lockNode(pid);
        if (!pn)
            break;
        if (pn->tag == eHTMLTag::Font)
        {
            if (!res.hasFontName && pn->fontAttrs.hasFontName)
            {
                res.hasFontName = true;
                res.fontName = pn->fontAttrs.fontName;
            }
            if (!res.hasFontSize && pn->fontAttrs.hasFontSize)
            {
                res.hasFontSize = true;
                res.fontSize = pn->fontAttrs.fontSize;
            }
            if (!res.hasColor && pn->fontAttrs.hasColor)
            {
                res.hasColor = true;
                res.color = pn->fontAttrs.color;
            }
        }
        pid = pn->parentId;
    }
}

bool HTMLDom::getSelectView(unsigned long long selectId, HTMLSelectView& out) const
{
    auto node = lockNode(selectId);
    if (!node)
        return false;
    if (node->tag != eHTMLTag::Select)
        return false;
    if (!node->visible)
        return false;
    if (node->selectLocalRect.cx <= 0.f || node->selectLocalRect.cy <= 0.f)
        return false;
    std::vector<NodePtr> options;
    collectSelectOptions(node, options);
    out.selectId = selectId;
    out.localRect = node->selectLocalRect;
    out.selectedIndex = computeSelectIndex(node.get(), options);
    out.style.frameSrc = node->selectFramePath;
    out.style.frameA = node->sliceA;
    out.style.frameB = node->sliceB;
    out.style.frameC = node->sliceC;
    out.style.frameD = node->sliceD;
    out.style.markerSrc = node->selectMarkerPath;
    out.style.markerA = node->selectMarkerA;
    out.style.markerB = node->selectMarkerB;
    out.style.markerVertical = node->selectMarkerVertical;
    out.style.markerMirror = node->selectMarkerMirror;
    out.style.fontName = node->fontAttrs.fontName;
    out.style.fontSize = node->fontAttrs.fontSize;
    out.style.color = node->fontAttrs.hasColor ? node->fontAttrs.color : 0xFFFFFFFF;
    out.style.itemHeight = node->selectLocalItemHeight > 0.f ? node->selectLocalItemHeight : node->selectItemHeight;
    out.style.itemPadY = node->selectItemPadY;
    out.style.contentPad = std::max(node->padding.left, node->padding.top);
    out.style.maxPopupRows = node->selectMaxPopupRows;
    out.style.minWidth = node->selectLocalRect.cx;
    out.style.popupWidth = domGetNodeAttrFloat(*node, "popupwidth", 0.f);
    if (out.style.popupWidth <= 0.f)
        out.style.popupWidth = domGetNodeAttrFloat(*node, "dropdownwidth", 0.f);
    if (out.style.popupWidth <= 0.f)
        out.style.popupWidth = domGetNodeAttrFloat(*node, "listwidth", 0.f);
    out.style.popupSelectedSrc = domGetNodeAttrStr(*node, "popupselsrc", "");
    out.style.popupHoverSrc = domGetNodeAttrStr(*node, "popuphoversrc", "");
    out.style.popupPressedSrc = domGetNodeAttrStr(*node, "popuppressedsrc", "");
    out.style.popupHoverColor = parseHTMLColor(domGetNodeAttrStr(*node, "popuphovercolor", "#FFFFFFFF"));
    out.style.popupPressedColor = parseHTMLColor(domGetNodeAttrStr(*node, "popuppressedcolor", "#FFFFFFFF"));
    out.style.popupBgTint = domGetNodeAttrFloat(*node, "popuptint", 1.f);
    out.style.popupBgAlpha = domGetNodeAttrFloat(*node, "popupalpha", 1.f);
    out.style.popupPad = domGetNodeAttrFloat(*node, "popuppad", 0.f);
    out.style.popupRowA = domGetNodeAttrFloat(*node, "popuprowa", 0.f);
    out.style.popupRowB = domGetNodeAttrFloat(*node, "popuprowb", 0.f);
    out.style.popupRowC = domGetNodeAttrFloat(*node, "popuprowc", 0.f);
    out.style.popupRowD = domGetNodeAttrFloat(*node, "popuprowd", 0.f);
    out.style.itemPad = domGetNodeAttrFloat(*node, "itempad", 2.f);
    out.style.disabled = node->disabled;
    out.style.disabledTint = domNodeDisabledTint(*node);
    out.style.documentColor = getRootOptions(findRootIdForNode(node)).documentColor;
    out.options.clear();
    out.options.reserve(options.size());
    for (const auto& opt : options)
    {
        HTMLSelectOptionDesc desc;
        HTMLFontAttrs fa = opt->fontAttrs;
        {

            unsigned long long pid = opt->parentId;
            while (pid != 0 && pid != selectId)
            {
                auto pn = lockNode(pid);
                if (!pn)
                    break;
                if (pn->tag == eHTMLTag::Font)
                {
                    if (!fa.hasFontName && pn->fontAttrs.hasFontName)
                    {
                        fa.hasFontName = true;
                        fa.fontName = pn->fontAttrs.fontName;
                    }
                    if (!fa.hasFontSize && pn->fontAttrs.hasFontSize)
                    {
                        fa.hasFontSize = true;
                        fa.fontSize = pn->fontAttrs.fontSize;
                    }
                    if (!fa.hasColor && pn->fontAttrs.hasColor)
                    {
                        fa.hasColor = true;
                        fa.color = pn->fontAttrs.color;
                    }
                }
                pid = pn->parentId;
            }
        }
        desc.label = getSelectOptionLabel(opt);
        desc.value = opt->selectValue;
        desc.html = opt->innerHTML;
        desc.fontName = fa.hasFontName ? fa.fontName : std::string();
        desc.fontSize = fa.hasFontSize ? fa.fontSize : 0;
        desc.selected = opt->optionSelected;
        desc.enabled = opt->optionEnabled;
        desc.height = domGetNodeAttrFloat(*opt, "height", 0.f);
        desc.color = fa.hasColor ? fa.color : out.style.color;
        desc.disabledTint = domNodeDisabledTint(*opt);
        if (!desc.enabled)
            desc.color = domMulColor(desc.color, desc.disabledTint);
        out.options.push_back(desc);
    }
    return true;
}

bool HTMLDom::getSelectScreenRect(unsigned long long selectId, Rect& outScreen) const
{
    auto node = lockNode(selectId);
    if (!node)
        return false;
    if (node->tag != eHTMLTag::Select)
        return false;
    if (!node->visible)
        return false;
    if (node->selectLocalRect.cx <= 0.f || node->selectLocalRect.cy <= 0.f)
        return false;
    unsigned long long rootId = findRootIdForNode(node);
    auto host = getHostContainer(rootId);
    outScreen = localRectToScreen(host, node->selectLocalRect);
    return true;
}

bool HTMLDom::selectOptionByIndex(unsigned long long selectId, int index, bool fireCallback)
{
    auto node = getNode(selectId);
    if (!node)
        return false;
    if (node->tag != eHTMLTag::Select)
        return false;
    if (node->disabled)
        return false;
    std::vector<NodePtr> options;
    collectSelectOptions(node, options);
    if (index < 0 || index >= (int)options.size())
        return false;
    if (!options[index]->optionEnabled)
        return false;
    node->selectedIndex = index;
    if (!fireCallback)
        return true;
    unsigned long long rootId = findRootIdForNode(node);
    const auto& opt = options[index];
    std::string value = opt->selectValue;
    if (value.empty())
        value = getSelectOptionLabel(opt);
    if (auto cb = getDocumentCallbacks(rootId))
        cb->onSelectChanged(rootId, selectId, opt->id, index, value.c_str());
    return true;
}

float HTMLDom::getDocumentScale(unsigned long long rootId) const
{
    if (rootId == 0)
        return 1.f;
    auto cb = getDocumentCallbacks(rootId);
    if (!cb)
        return 1.f;
    float s = cb->getDocumentScale(rootId);
    if (s <= 0.f)
        s = 1.f;
    return s;
}

unsigned long long HTMLDom::getRootIdForNode(unsigned long long nodeId) const
{
    auto node = lockNode(nodeId);
    if (!node)
        return 0;
    return findRootIdForNode(node);
}

float HTMLDom::getSliderValue(unsigned long long nodeId) const
{
    auto node = lockNode(nodeId);
    if (!node || node->tag != eHTMLTag::Slider)
        return 0.f;
    return node->sliderValue;
}

bool HTMLDom::setSliderValue(unsigned long long nodeId, float value, bool fireCallback)
{
    auto node = getNode(nodeId);
    if (!node || node->tag != eHTMLTag::Slider)
        return false;
    float val = clampSliderVal(*node, value);
    if (val == node->sliderValue)
        return true;
    node->sliderValue = val;
    if (fireCallback)
    {
        unsigned long long rootId = findRootIdForNode(node);
        if (auto cb = getDocumentCallbacks(rootId))
            cb->onSliderChanged(rootId, nodeId, val);
    }
    return true;
}

float HTMLDom::getProgressValue(unsigned long long nodeId) const
{
    auto node = lockNode(nodeId);
    if (!node || node->tag != eHTMLTag::ProgressBar)
        return 0.f;
    return clampProgress01(node->progressT);
}

bool HTMLDom::setProgressValue(unsigned long long nodeId, float value01)
{
    auto node = getNode(nodeId);
    if (!node || node->tag != eHTMLTag::ProgressBar)
        return false;
    float v = clampProgress01(value01);
    if (v == node->progressT)
        return true;
    node->progressT = v;
    return true;
}

float HTMLDom::getProgressPercent(unsigned long long nodeId) const
{
    return getProgressValue(nodeId) * 100.f;
}

bool HTMLDom::setProgressPercent(unsigned long long nodeId, float percent)
{
    return setProgressValue(nodeId, percent / 100.f);
}

void HTMLDom::registerParticlesPreset(const char* lpszName, FParticlesPresetFactory factory)
{
    if (!lpszName || !*lpszName || !factory)
        return;
    _particlesPresets[domToLower(lpszName)] = std::move(factory);
}

void HTMLDom::unregisterParticlesPreset(const char* lpszName)
{
    if (!lpszName || !*lpszName)
        return;
    _particlesPresets.erase(domToLower(lpszName));
}

bool HTMLDom::hasParticlesPreset(const char* lpszName) const
{
    if (!lpszName || !*lpszName)
        return false;
    return _particlesPresets.find(domToLower(lpszName)) != _particlesPresets.end();
}

std::shared_ptr<CContainer> HTMLDom::getParticlesSystem(unsigned long long nodeId)
{
    auto it = _particlesSystems.find(nodeId);
    return (it != _particlesSystems.end()) ? it->second : nullptr;
}

void HTMLDom::blurFocusedEdit()
{
    if (_focusedEditId == 0)
        return;
    const unsigned long long editId = _focusedEditId;
    const unsigned long long rootId = _focusedEditRootId;
    auto node = lockNode(editId);
    _focusedEditId = 0;
    _focusedEditRootId = 0;
    stopEditDrag(true);
    if (node)
        node->editSelAnchor = node->editCursor;
    if (auto cb = getDocumentCallbacks(rootId))
        cb->onEditFocusChanged(rootId, editId, false);
    domSyncVirtualKeyboard(node, false);
}

bool HTMLDom::isEditFocused(unsigned long long nodeId) const
{
    return _focusedEditId == nodeId && nodeId != 0;
}

bool HTMLDom::focusEdit(unsigned long long nodeId, bool focused)
{
    auto node = getNode(nodeId);
    if (!node || node->tag != eHTMLTag::Edit)
        return false;
    if (!focused)
    {
        if (_focusedEditId == nodeId)
            blurFocusedEdit();
        return true;
    }
    if (node->disabled || !node->visible)
        return false;
    if (_focusedEditId == nodeId)
        return true;
    blurFocusedEdit();
    _focusedEditId = nodeId;
    _focusedEditRootId = findRootIdForNode(node);
    node->editSelAnchor = node->editCursor;
    stopEditDrag(true);
    if (auto cb = getDocumentCallbacks(_focusedEditRootId))
        cb->onEditFocusChanged(_focusedEditRootId, nodeId, true);
    domSyncVirtualKeyboard(node, true);
    return true;
}

bool HTMLDom::setEditCursor(unsigned long long nodeId, int bytePos)
{
    auto node = getNode(nodeId);
    if (!node || node->tag != eHTMLTag::Edit)
        return false;
    bytePos = std::clamp(bytePos, 0, (int)node->editValue.size());
    node->editCursor = bytePos;
    node->editSelAnchor = bytePos;
    return true;
}

const char* HTMLDom::getEditValue(unsigned long long nodeId) const
{
    auto node = lockNode(nodeId);
    if (!node || node->tag != eHTMLTag::Edit)
        return nullptr;
    return node->editValue.c_str();
}

bool HTMLDom::setEditValue(unsigned long long nodeId, const char* value, bool fireCallback)
{
    auto node = getNode(nodeId);
    if (!node || node->tag != eHTMLTag::Edit)
        return false;
    node->editInited = true;
    node->editValue = value ? value : "";
    node->editCursor = (int)node->editValue.size();
    node->editSelAnchor = node->editCursor;
    if (fireCallback)
    {
        unsigned long long rootId = findRootIdForNode(node);
        if (auto cb = getDocumentCallbacks(rootId))
            cb->onEditChanged(rootId, nodeId, node->editValue.c_str());
    }
    return true;
}
void HTMLDom::handleKeyboardInput(const char* szTextAdd, eSpecialKey eKey, int modifiers)
{
    if (_focusedEditId == 0)
        return;
    auto node = lockNode(_focusedEditId);
    if (!node || node->tag != eHTMLTag::Edit)
    {
        blurFocusedEdit();
        return;
    }
    {
        unsigned long long id = node->id;
        while (id != 0)
        {
            auto n = lockNode(id);
            if (!n)
                return;
            if (!n->visible || n->disabled)
                return;
            id = n->parentId;
        }
    }
    domEnsureEditInited(node);
    std::string& v = node->editValue;
    int& cur = node->editCursor;
    bool changed = false;
    auto clampPos = [&](int p) -> int
    {
        return std::clamp(p, 0, (int)v.size());
    };
    node->editSelAnchor = clampPos(node->editSelAnchor);
    cur = clampPos(cur);
    auto getSelection = [&](int& start, int& end)
    {
        start = clampPos(std::min(node->editSelAnchor, cur));
        end = clampPos(std::max(node->editSelAnchor, cur));
    };
    auto hasSelection = [&]() -> bool
    {
        int s, e;
        getSelection(s, e);
        return s < e;
    };
    auto makeEditState = [&]() -> HTMLRenderState
    {
        return domMakeEditMeasureState(this, node);
    };
    auto adjustScrollLeft = [&](int oldCursor, int newCursor)
    {
        if (oldCursor <= newCursor || node->editScrollX <= 0.f)
            return;
        HTMLRenderState st = makeEditState();
        float oldX = domMeasureEditPrefix(*node, st, oldCursor);
        float newX = domMeasureEditPrefix(*node, st, newCursor);
        float dx = oldX - newX;
        if (dx > 0.f)
            node->editScrollX = std::max(0.f, node->editScrollX - dx);
    };
    auto deleteSelection = [&](bool adjustScroll) -> bool
    {
        int s, e;
        getSelection(s, e);
        if (s >= e)
            return false;
        if (adjustScroll)
            adjustScrollLeft(cur, s);
        v.erase((size_t)s, (size_t)(e - s));
        cur = s;
        node->editSelAnchor = cur;
        changed = true;
        return true;
    };
    auto countUtf8Chars = [](const std::string& s) -> int
    {
        int n = 0;
        for (const char* p = s.c_str(); *p; p += getUtf8CharLen(p))
            ++n;
        return n;
    };
    auto filterInput = [&](const char* text) -> std::string
    {
        std::string clean;
        if (!text)
            return clean;
        for (const char* p = text; *p; )
        {
            int cl = getUtf8CharLen(p);
            unsigned char c0 = (unsigned char)p[0];
            if (cl == 1 && (c0 < 0x20 || c0 == 0x7F))
            {
                p += cl;
                continue;
            }
            clean.append(p, cl);
            p += cl;
        }
        return clean;
    };
    auto insertText = [&](const char* text)
    {
        std::string clean = filterInput(text);
        if (clean.empty())
            return;
        deleteSelection(true);
        if (node->editMaxLen > 0)
        {
            int currentChars = countUtf8Chars(v);
            int room = node->editMaxLen - currentChars;
            if (room <= 0)
            {
                clean.clear();
            }
            else
            {
                std::string truncated;
                int added = 0;
                for (const char* p = clean.c_str(); *p && added < room; )
                {
                    int cl = getUtf8CharLen(p);
                    truncated.append(p, cl);
                    p += cl;
                    ++added;
                }
                clean.swap(truncated);
            }
        }
        if (clean.empty())
            return;
        v.insert((size_t)cur, clean);
        cur += (int)clean.size();
        node->editSelAnchor = cur;
        changed = true;
    };
    auto fireChanged = [&]()
    {
        unsigned long long rootId = findRootIdForNode(node);
        if (auto cb = getDocumentCallbacks(rootId))
            cb->onEditChanged(rootId, node->id, v.c_str());
    };
    const bool shift = (modifiers & 1) != 0;
    switch (eKey)
    {
    case eSpecialKey::SK_NONE:
    {
        if (szTextAdd && *szTextAdd)
            insertText(szTextAdd);
        break;
    }
    case eSpecialKey::SK_BACKSPACE:
    {
        if (deleteSelection(true))
            break;
        if (cur > 0)
        {
            size_t start = domPrevUtf8Start(v, (size_t)cur);
            adjustScrollLeft(cur, (int)start);
            v.erase(start, (size_t)cur - start);
            cur = (int)start;
            node->editSelAnchor = cur;
            changed = true;
        }
        break;
    }
    case eSpecialKey::SK_DELETE:
    {
        if (deleteSelection(true))
            break;
        if (cur < (int)v.size())
        {
            size_t len = 1;
            domUtf8DecodeAt(v, (size_t)cur, len);
            v.erase((size_t)cur, len);
            node->editSelAnchor = cur;
            changed = true;
        }
        break;
    }
    case eSpecialKey::SK_LEFT:
    {
        if (shift)
        {
            if (!hasSelection())
                node->editSelAnchor = cur;
            if (cur > 0)
                cur = (int)domPrevUtf8Start(v, (size_t)cur);
        }
        else
        {
            int s, e;
            getSelection(s, e);
            if (s < e)
                cur = s;
            else if (cur > 0)
                cur = (int)domPrevUtf8Start(v, (size_t)cur);
            node->editSelAnchor = cur;
        }
        break;
    }
    case eSpecialKey::SK_RIGHT:
    {
        if (shift)
        {
            if (!hasSelection())
                node->editSelAnchor = cur;
            if (cur < (int)v.size())
            {
                size_t len = 1;
                domUtf8DecodeAt(v, (size_t)cur, len);
                cur += (int)len;
            }
        }
        else
        {
            int s, e;
            getSelection(s, e);
            if (s < e)
                cur = e;
            else if (cur < (int)v.size())
            {
                size_t len = 1;
                domUtf8DecodeAt(v, (size_t)cur, len);
                cur += (int)len;
            }
            node->editSelAnchor = cur;
        }
        break;
    }
    case eSpecialKey::SK_HOME:
    {
        if (shift)
        {
            if (!hasSelection())
                node->editSelAnchor = cur;
            cur = 0;
        }
        else
        {
            cur = 0;
            node->editSelAnchor = cur;
        }
        break;
    }
    case eSpecialKey::SK_END:
    {
        if (shift)
        {
            if (!hasSelection())
                node->editSelAnchor = cur;
            cur = (int)v.size();
        }
        else
        {
            cur = (int)v.size();
            node->editSelAnchor = cur;
        }
        break;
    }
    case eSpecialKey::SK_COPY:
    {
        int s, e;
        getSelection(s, e);
        if (s < e && !node->editPassword)
        {
            std::string selected = v.substr((size_t)s, (size_t)(e - s));
            InputController::getInstance()->setClipboardText(selected.c_str());
        }
        break;
    }
    case eSpecialKey::SK_CUT:
    {
        int s, e;
        getSelection(s, e);
        if (s < e && !node->editPassword)
        {
            std::string selected = v.substr((size_t)s, (size_t)(e - s));
            InputController::getInstance()->setClipboardText(selected.c_str());
            deleteSelection(true);
        }
        break;
    }
    case eSpecialKey::SK_PASTE:
    {
        std::string clip = InputController::getInstance()->getClipboardText();
        if (!clip.empty())
            insertText(clip.c_str());
        break;
    }
    case eSpecialKey::SK_SELECT_ALL:
    {
        node->editSelAnchor = 0;
        cur = (int)v.size();
        break;
    }
    case eSpecialKey::SK_ENTER:
    {
        unsigned long long rootId = findRootIdForNode(node);
        if (auto cb = getDocumentCallbacks(rootId))
            cb->onEditSubmit(rootId, node->id, v.c_str());
        break;
    }
    case eSpecialKey::SK_ESCAPE:
    {
        blurFocusedEdit();
        break;
    }
    default:
        break;
    }
    if (changed)
        fireChanged();
}

void HTMLDom::tweenReadProp(HTMLDomNode& node, eHTMLTweenProp prop, float& f, unsigned int& c)
{
    switch (prop)
    {
    case eHTMLTweenProp::OffsetX:  f = node.visOffX; break;
    case eHTMLTweenProp::OffsetY:  f = node.visOffY; break;
    case eHTMLTweenProp::ScaleX:   f = node.visScaleX; break;
    case eHTMLTweenProp::ScaleY:   f = node.visScaleY; break;
    case eHTMLTweenProp::Rotation: f = node.visRot; break;
    case eHTMLTweenProp::PivotX:   f = node.visPivotX; break;
    case eHTMLTweenProp::PivotY:   f = node.visPivotY; break;
    case eHTMLTweenProp::Alpha:    f = node.visAlpha; break;
    case eHTMLTweenProp::Left:     f = node.relX; break;
    case eHTMLTweenProp::Top:      f = node.relY; break;
    case eHTMLTweenProp::Width:    f = node.reqWidth; break;
    case eHTMLTweenProp::Height:   f = node.reqHeight; break;
    case eHTMLTweenProp::Color:    c = node.fontAttrs.color; break;
    case eHTMLTweenProp::BgColor:  c = node.bgColor; break;
    case eHTMLTweenProp::FontSize: f = (float)node.fontAttrs.fontSize; break;
    case eHTMLTweenProp::Progress: f = node.progressT; break;
    default: break;
    }
}

void HTMLDom::tweenApplyProp(HTMLDomNode& node, const HTMLTweenChannel& ch, float k)
{
    if (ch.isColor)
    {
        unsigned int v = lerpColor(ch.fromC, ch.toC, k);
        switch (ch.prop)
        {
        case eHTMLTweenProp::Color:
            node.fontAttrs.hasColor = true;
            node.fontAttrs.color = v;
            break;
        case eHTMLTweenProp::BgColor:
            node.bgColor = v;
            break;
        default:
            break;
        }
        return;
    }
    float v = ch.fromF + (ch.toF - ch.fromF) * k;
    switch (ch.prop)
    {
    case eHTMLTweenProp::OffsetX:  node.visOffX = v; break;
    case eHTMLTweenProp::OffsetY:  node.visOffY = v; break;
    case eHTMLTweenProp::ScaleX:   node.visScaleX = v; break;
    case eHTMLTweenProp::ScaleY:   node.visScaleY = v; break;
    case eHTMLTweenProp::Rotation: node.visRot = v; break;
    case eHTMLTweenProp::PivotX:   node.visPivotX = v; node.visPivotSet = true; break;
    case eHTMLTweenProp::PivotY:   node.visPivotY = v; node.visPivotSet = true; break;
    case eHTMLTweenProp::Alpha:    node.visAlpha = std::clamp(v, 0.f, 1.f); break;
    case eHTMLTweenProp::Left:     node.relX = v; node.positionRelative = true; break;
    case eHTMLTweenProp::Top:      node.relY = v; node.positionRelative = true; break;
    case eHTMLTweenProp::Width:    node.reqWidth = std::max(0.f, v); break;
    case eHTMLTweenProp::Height:   node.reqHeight = std::max(0.f, v); break;
    case eHTMLTweenProp::FontSize:
        node.fontAttrs.hasFontSize = true;
        node.fontAttrs.fontSize = std::max(1, (int)(v + 0.5f));
        break;
    case eHTMLTweenProp::Progress:
        node.progressT = std::clamp(v, 0.f, 1.f);
        break;
    default:
        break;
    }
}

unsigned long long HTMLDom::tweenTo(unsigned long long nodeId, const HTMLTweenProps& props, float dur, const HTMLTweenOptions& opts)
{
    auto node = getNode(nodeId);
    if (!node || props.nCh <= 0 || dur < 0.f)
        return 0;
    HTMLTween tw;
    tw.tweenId = _nextTweenId++;
    tw.nodeId = nodeId;
    tw.nodeGen = node->tweenGen;
    tw.rootId = findRootIdForNode(node);
    tw.delay = std::max(0.f, opts.delay);
    tw.dur = std::max(dur, 1e-5f);
    tw.repeat = opts.repeat;
    tw.yoyo = opts.yoyo;
    tw.ease = opts.ease ? opts.ease : Easing::linear;
    tw.easeBack = opts.easeBack;
    tw.onComplete = opts.onComplete;
    tw.nCh = 0;
    for (int i = 0; i < props.nCh && tw.nCh < HTMLTweenProps::MAX_CHANNELS; ++i)
    {
        HTMLTweenChannel ch;
        ch.prop = props.ch[i].prop;
        ch.isColor = props.ch[i].isColor;
        tweenReadProp(*node, ch.prop, ch.fromF, ch.fromC);
        if (ch.isColor)
            ch.toC = props.ch[i].valueC;
        else
            ch.toF = props.ch[i].valueF;
        tw.ch[tw.nCh++] = ch;
    }
    if (tw.nCh == 0)
        return 0;
    _tweens.push_back(std::move(tw));
    return _tweens.back().tweenId;
}

unsigned long long HTMLDom::tweenFrom(unsigned long long nodeId, const HTMLTweenProps& props, float dur, const HTMLTweenOptions& opts)
{
    auto node = getNode(nodeId);
    if (!node || props.nCh <= 0 || dur < 0.f)
        return 0;
    HTMLTween tw;
    tw.tweenId = _nextTweenId++;
    tw.nodeId = nodeId;
    tw.nodeGen = node->tweenGen;
    tw.rootId = findRootIdForNode(node);
    tw.delay = std::max(0.f, opts.delay);
    tw.dur = std::max(dur, 1e-5f);
    tw.repeat = opts.repeat;
    tw.yoyo = opts.yoyo;
    tw.ease = opts.ease ? opts.ease : Easing::linear;
    tw.easeBack = opts.easeBack;
    tw.onComplete = opts.onComplete;
    tw.nCh = 0;
    for (int i = 0; i < props.nCh && tw.nCh < HTMLTweenProps::MAX_CHANNELS; ++i)
    {
        HTMLTweenChannel ch;
        ch.prop = props.ch[i].prop;
        ch.isColor = props.ch[i].isColor;

        tweenReadProp(*node, ch.prop, ch.toF, ch.toC);
        if (ch.isColor)
            ch.fromC = props.ch[i].valueC;
        else
            ch.fromF = props.ch[i].valueF;
        tw.ch[tw.nCh] = ch;

        tweenApplyProp(*node, tw.ch[tw.nCh], 0.f);
        ++tw.nCh;
    }
    if (tw.nCh == 0)
        return 0;
    _tweens.push_back(std::move(tw));
    return _tweens.back().tweenId;
}

void HTMLDom::updateTweens(float dt)
{
    if (_tweens.empty())
        return;
    for (size_t i = _tweens.size(); i-- > 0; )
    {
        auto& tw = _tweens[i];
        if (_tweensPaused ||
            _pausedTweenRoots.find(tw.rootId) != _pausedTweenRoots.end())
            continue;
        auto node = lockNode(tw.nodeId);
        if (!node || node->tweenGen != tw.nodeGen)
        {
            _tweens[i] = _tweens.back();
            _tweens.pop_back();
            continue;
        }
        float step = dt;
        if (tw.delay > 0.f)
        {
            if (tw.delay >= step)
            {
                tw.delay -= step;
                continue;
            }
            step -= tw.delay;
            tw.delay = 0.f;
        }
        tw.t += step / tw.dur;
        const bool finished = tw.t >= 1.f;
        const float tt = finished ? 1.f : tw.t;
        float k;
        if (tw.reversed)
        {
            easingFunction eb = tw.easeBack ? tw.easeBack : tw.ease;
            k = eb(1.f - tt);
        }
        else
        {
            k = tw.ease(tt);
        }
        for (int c = 0; c < tw.nCh; ++c)
            tweenApplyProp(*node, tw.ch[c], k);
        if (!finished)
            continue;
        if (tw.repeat != 0)
        {
            if (tw.repeat > 0)
                --tw.repeat;
            tw.t = 0.f;
            if (tw.yoyo)
                tw.reversed = !tw.reversed;
            continue;
        }

        const unsigned long long doneId = tw.tweenId;
        const unsigned long long doneNode = tw.nodeId;
        auto cb = std::move(tw.onComplete);
        if (cb)
            cb(doneNode);
        for (size_t j = 0; j < _tweens.size(); ++j)
        {
            if (_tweens[j].tweenId == doneId)
            {
                _tweens[j] = _tweens.back();
                _tweens.pop_back();
                break;
            }
        }
    }
}

bool HTMLDom::killTween(unsigned long long tweenId)
{
    for (size_t i = 0; i < _tweens.size(); ++i)
    {
        if (_tweens[i].tweenId == tweenId)
        {
            _tweens[i] = _tweens.back();
            _tweens.pop_back();
            return true;
        }
    }
    return false;
}

bool HTMLDom::killTweensOf(unsigned long long nodeId)
{
    bool removed = false;
    for (size_t i = _tweens.size(); i-- > 0; )
    {
        if (_tweens[i].nodeId == nodeId)
        {
            _tweens[i] = _tweens.back();
            _tweens.pop_back();
            removed = true;
        }
    }
    return removed;
}

void HTMLDom::killAllTweens()
{
    _tweens.clear();
}

void HTMLDom::setTweensPaused(bool paused)
{
    _tweensPaused = paused;
}

void HTMLDom::pauseTweensForRoot(unsigned long long rootId, bool paused)
{
    if (paused)
        _pausedTweenRoots.insert(rootId);
    else
        _pausedTweenRoots.erase(rootId);
}

void HTMLDom::applyVisMatToRect(float& x, float& y, float& w, float& h) const
{
    if (_visMatStack.empty())
        return;
    const float* m = _visMatStack.back().m;
    auto pt = [&](float px, float py, float& ox, float& oy)
    {
        ox = m[0] * px + m[1] * py + m[2];
        oy = m[3] * px + m[4] * py + m[5];
    };
    float x0, y0, x1, y1, x2, y2, x3, y3;
    pt(x, y, x0, y0);
    pt(x + w, y, x1, y1);
    pt(x, y + h, x2, y2);
    pt(x + w, y + h, x3, y3);
    float nx = std::min(std::min(x0, x1), std::min(x2, x3));
    float ny = std::min(std::min(y0, y1), std::min(y2, y3));
    float mx = std::max(std::max(x0, x1), std::max(x2, x3));
    float my = std::max(std::max(y0, y1), std::max(y2, y3));
    x = nx;
    y = ny;
    w = mx - nx;
    h = my - ny;
}

void HTMLDom::endSliderDrag(bool fireCallback)
{
    if (_dragSliderId == 0)
        return;
    unsigned long long rootId = _dragRootId;
    unsigned long long nodeId = _dragSliderId;
    _dragSliderId = 0;
    _dragRootId = 0;
    if (fireCallback)
        if (auto cb = getDocumentCallbacks(rootId))
            cb->onDrag(rootId, nodeId, true);
}

void HTMLDom::beginEditDrag(unsigned long long rootId, unsigned long long nodeId)
{
    if (nodeId == 0)
        return;
    if (_editDragId == nodeId && _editDragRootId == rootId)
        return;
    stopEditDrag(true);
    _editDragId = nodeId;
    _editDragRootId = rootId;
    if (auto cb = getDocumentCallbacks(rootId))
        cb->onDragAny(rootId, nodeId, false);
}

void HTMLDom::stopEditDrag(bool fireCallback)
{
    if (_editDragId == 0)
        return;
    unsigned long long rootId = _editDragRootId;
    unsigned long long nodeId = _editDragId;
    _editDragId = 0;
    _editDragRootId = 0;
    if (fireCallback)
        if (auto cb = getDocumentCallbacks(rootId))
            cb->onDragAny(rootId, nodeId, true);
}

int sliderEffectiveMajorCount(const HTMLDomNode& node)
{
    int major = node.sliderMajorCount;
    if (!node.sliderTicksAuto && major > 0)
        return major;
    const Rect& rc = node.sliderLocalRect;
    float gripW = std::max(1.f, node.sliderGripW);
    float gripH = std::max(1.f, node.sliderGripH);
    float len;
    if (!node.sliderVertical)
    {
        float tw = rc.cx - gripW;
        len = (tw <= 0.f) ? rc.cx : tw;
    }
    else
    {
        float th = rc.cy - gripH;
        len = (th <= 0.f) ? rc.cy : th;
    }
    int autoMajor = (int)(len / 80.f);
    return std::clamp(autoMajor, 2, 20);
}

void HTMLDom::updateSliderFromPointer(const NodePtr& node, float x, float y, bool fireChanged)
{
    if (!node || node->tag != eHTMLTag::Slider)
        return;
    const Rect& rc = node->sliderLocalRect;
    if (rc.cx <= 0.f || rc.cy <= 0.f)
        return;
    float mn = std::min(node->sliderMin, node->sliderMax);
    float mx = std::max(node->sliderMin, node->sliderMax);
    if (mx <= mn)
        return;
    float cMin = 0.f, cMax = 0.f;
    sliderGripTravel(*node, cMin, cMax);
    float pos = node->sliderVertical ? y : x;
    float span = cMax - cMin;
    float t = (span <= 0.f) ? 0.f : std::clamp((pos - cMin) / span, 0.f, 1.f);
    float val;
    if (node->sliderSnapToMajorTicks)
    {
        int major = sliderEffectiveMajorCount(*node);
        if (major > 0)
        {
            float fMajor = (float)major;
            float idx = std::floor(t * fMajor + 0.5f);
            t = std::clamp(idx / fMajor, 0.f, 1.f);
        }
        val = std::clamp(mn + t * (mx - mn), mn, mx);
    }
    else
    {
        val = clampSliderVal(*node, mn + t * (mx - mn));
    }
    if (val != node->sliderValue)
    {
        node->sliderValue = val;
        if (fireChanged)
        {
            unsigned long long rootId = findRootIdForNode(node);
            if (auto cb = getDocumentCallbacks(rootId))
                cb->onSliderChanged(rootId, node->id, val);
        }
    }
}

#ifdef HAS_SPINE
HTMLDom::SpineRuntimeState* HTMLDom::findSpineRuntimeState(unsigned long long nodeId)
{
    auto itKey = _spineKeyByNode.find(nodeId);
    if (itKey == _spineKeyByNode.end())
        return nullptr;
    auto itState = _spineStates.find(itKey->second);
    if (itState == _spineStates.end())
        return nullptr;
    return &itState->second;
}

bool HTMLDom::spineSetAnimation(unsigned long long nodeId, size_t trackIndex, const char* animName, bool loop)
{
    auto node = getNode(nodeId);
    if (!node || node->tag != eHTMLTag::Spine || !animName)
        return false;
    auto st = findSpineRuntimeState(nodeId);
    if (!st || !st->p)
        return false;
    st->p->setAnimation(trackIndex, animName, loop);
    st->lastAnim = animName;
    st->lastLoop = loop;
    return true;
}

bool HTMLDom::spineAddAnimation(unsigned long long nodeId, size_t trackIndex, const char* animName, bool loop, float delay)
{
    auto node = getNode(nodeId);
    if (!node || node->tag != eHTMLTag::Spine || !animName)
        return false;
    auto st = findSpineRuntimeState(nodeId);
    if (!st || !st->p)
        return false;
    st->p->addAnimation(trackIndex, animName, loop, delay);
    st->lastAnim = animName;
    st->lastLoop = loop;
    return true;
}

bool HTMLDom::spineSetTimeScale(unsigned long long nodeId, float timeScale)
{
    auto node = getNode(nodeId);
    if (!node || node->tag != eHTMLTag::Spine)
        return false;
    auto st = findSpineRuntimeState(nodeId);
    if (!st || !st->p)
        return false;
    st->p->setTimeScale(timeScale);
    return true;
}

bool HTMLDom::spineStopAnimation(unsigned long long nodeId)
{
    auto node = getNode(nodeId);
    if (!node || node->tag != eHTMLTag::Spine)
        return false;
    auto st = findSpineRuntimeState(nodeId);
    if (!st || !st->p)
        return false;
    st->p->clearTracks();
    st->p->setToSetupPose();
    st->lastAnim.clear();
    return true;
}

bool HTMLDom::spineSetPaused(unsigned long long nodeId, bool paused)
{
    auto node = getNode(nodeId);
    if (!node || node->tag != eHTMLTag::Spine)
        return false;
    auto st = findSpineRuntimeState(nodeId);
    if (!st || !st->p)
        return false;
    st->p->setPaused(paused);
    return true;
}

bool HTMLDom::spineRestartAnimation(unsigned long long nodeId)
{
    auto node = getNode(nodeId);
    if (!node || node->tag != eHTMLTag::Spine)
        return false;
    auto st = findSpineRuntimeState(nodeId);
    if (!st || !st->p)
        return false;
    st->p->clearTracks();
    st->p->setToSetupPose();
    std::string anim = st->lastAnim;
    bool loop = st->lastLoop;
    if (anim.empty())
    {
        anim = node->spineAnim;
        loop = node->spineLoop;
    }
    if (!anim.empty() && st->p->hasAnimation(anim.c_str()))
    {
        st->p->setAnimation(0, anim.c_str(), loop);
        st->lastAnim = anim;
        st->lastLoop = loop;
    }
    return true;
}

#endif

bool HTMLDom::setImageAnimation(unsigned long long nodeId, eHTMLIdleStyle style)
{
    auto node = getNode(nodeId);
    if (!node)
        return false;
    node->idleStyle = style;
    node->imgAnimCompletedSent = false;
    return true;
}

bool HTMLDom::stopImageAnimation(unsigned long long nodeId)
{
    auto node = getNode(nodeId);
    if (!node)
        return false;
    node->idleStyle = eHTMLIdleStyle::None;
    node->imgAnimCompletedSent = false;
    return true;
}

bool HTMLDom::restartImageAnimation(unsigned long long nodeId)
{
    auto node = getNode(nodeId);
    if (!node)
        return false;
    node->idleTimeOffset = _fAnimTime;
    node->imgAnimCompletedSent = false;
    return true;
}

bool HTMLDom::setFontAppearAnimation(unsigned long long nodeId, eAppearStyle style)
{
    auto node = getNode(nodeId);
    if (!node)
        return false;
    if (style == eAppearStyle::NONE)
    {
        node->fontAttrs.hasAppear = false;
        node->fontAttrs.appearStyle = eAppearStyle::NONE;
    }
    else
    {
        node->fontAttrs.hasAppear = true;
        node->fontAttrs.appearStyle = style;
    }
    _fontAnimEventFlags.erase(nodeId);
    restartAppear(nodeId);
    return true;
}

bool HTMLDom::setFontFxAnimation(unsigned long long nodeId, eFxStyle style)
{
    auto node = getNode(nodeId);
    if (!node)
        return false;
    if (style == eFxStyle::NONE)
    {
        node->fontAttrs.hasFx = false;
        node->fontAttrs.fxStyle = eFxStyle::NONE;
    }
    else
    {
        node->fontAttrs.hasFx = true;
        node->fontAttrs.fxStyle = style;
    }
    _fontAnimEventFlags.erase(nodeId);
    restartAppear(nodeId);
    return true;
}

bool HTMLDom::stopFontAnimation(unsigned long long nodeId)
{
    auto node = getNode(nodeId);
    if (!node)
        return false;
    node->fontAttrs.hasAppear = false;
    node->fontAttrs.appearStyle = eAppearStyle::NONE;
    node->fontAttrs.hasFx = false;
    node->fontAttrs.fxStyle = eFxStyle::NONE;
    auto itFlags = _fontAnimEventFlags.find(nodeId);
    if (itFlags != _fontAnimEventFlags.end())
    {
        if (nodeHasIdAttr(node))
        {
            if (auto cb = getDocumentCallbacks(itFlags->second.rootId))
            {
                if (!itFlags->second.stopped)
                    cb->onFontAnimEvent(itFlags->second.rootId, nodeId, eHTMLFontAnimEventType::Stopped, node->fontAttrs.fxStyle, node->fontAttrs.appearStyle);
                if (itFlags->second.fxStart && !itFlags->second.fxComplete)
                    cb->onFontAnimEvent(itFlags->second.rootId, nodeId, eHTMLFontAnimEventType::FxComplete, node->fontAttrs.fxStyle, node->fontAttrs.appearStyle);
            }
        }
        _fontAnimEventFlags.erase(itFlags);
    }
    removeAppearState(nodeId);
    return true;
}

bool HTMLDom::restartFontAnimation(unsigned long long nodeId)
{
    auto node = getNode(nodeId);
    if (!node)
        return false;
    _fontAnimEventFlags.erase(nodeId);
    restartAppear(nodeId);
    return true;
}

void HTMLDom::clearHitData(unsigned long long rootId)
{
    if (rootId == 0)
        return;
    _hitData[rootId].clear();
}

void HTMLDom::registerHitRect(
    unsigned long long rootId,
    unsigned long long nodeId,
    float x,
    float y,
    float w,
    float h)
{
    if (rootId == 0 || nodeId == 0 || w <= 0.f || h <= 0.f)
        return;
    auto isInteractiveRaw = [](const HTMLDomNode* node) -> bool
    {
        if (!node)
            return false;
        if (node->clickable)
            return true;
        if (node->onClick)
            return true;
        if (node->tag == eHTMLTag::A)
            return true;
        if (node->tag == eHTMLTag::NineButton)
            return true;
        if (node->tag == eHTMLTag::Checkbox)
            return true;
        if (node->tag == eHTMLTag::Select)
            return true;
        if (node->tag == eHTMLTag::Option)
            return true;
        if (node->tag == eHTMLTag::Slider)
            return true;
        return false;
    };
    auto isCursorTrackedRaw = [](const HTMLDomNode* node) -> bool
    {
        if (!node)
            return false;
        if (node->cursorExplicit)
            return true;
        if (node->onClick)
            return true;
        if (node->clickable)
            return true;
        return false;
    };
    NodePtr fallback;
    auto allowHitForNode = [&](unsigned long long checkedNodeId) -> bool
    {
        const HTMLDomNode* node = nullptr;
        if (checkedNodeId < (unsigned long long)_nodeRawById.size())
            node = _nodeRawById[(size_t)checkedNodeId];
        if (!node)
        {
            fallback = lockNode(checkedNodeId);
            node = fallback.get();
        }
        if (!node)
            return false;
        if (!node->visible)
            return false;
        if (node->hasHoverColor || node->hasPressedColor)
            return true;
        if (node->tag == eHTMLTag::Checkbox)
            return true;
        switch (node->tag)
        {
        case eHTMLTag::P:
        case eHTMLTag::H1:
        case eHTMLTag::H2:
        case eHTMLTag::H3:
        case eHTMLTag::Center:
        case eHTMLTag::Table:
        case eHTMLTag::Tr:
        case eHTMLTag::Td:
            return isInteractiveRaw(node) || isCursorTrackedRaw(node);
        case eHTMLTag::Nine:
        case eHTMLTag::Three:
            if (node->spriteTex || node->spriteSrcW > 0.f || node->spriteSrcH > 0.f)
                return true;
            return isInteractiveRaw(node) || isCursorTrackedRaw(node);
        case eHTMLTag::Spine:
            if (!node->spinePath.empty())
                return true;
            return isInteractiveRaw(node) || isCursorTrackedRaw(node);
        default:
            return true;
        }
    };
    if (!allowHitForNode(nodeId))
        return;
    applyVisMatToRect(x, y, w, h);
    HitEntry e;
    e.nodeId = nodeId;
    e.rc.x = x;
    e.rc.y = y;
    e.rc.cx = w;
    e.rc.cy = h;
    _hitData[rootId].push_back(e);
}

void HTMLDom::registerHitRectAt(
    unsigned long long rootId,
    unsigned long long nodeId,
    float x,
    float y,
    float w,
    float h,
    size_t insertIndex)
{
    if (rootId == 0 || nodeId == 0 || w <= 0.f || h <= 0.f)
        return;
    auto isInteractiveRaw = [](const HTMLDomNode* node) -> bool
    {
        if (!node)
            return false;
        if (node->clickable)
            return true;
        if (node->onClick)
            return true;
        if (node->tag == eHTMLTag::A)
            return true;
        if (node->tag == eHTMLTag::NineButton)
            return true;
        if (node->tag == eHTMLTag::Checkbox)
            return true;
        if (node->tag == eHTMLTag::Select)
            return true;
        if (node->tag == eHTMLTag::Option)
            return true;
        if (node->tag == eHTMLTag::Slider)
            return true;
        return false;
    };
    auto isCursorTrackedRaw = [](const HTMLDomNode* node) -> bool
    {
        if (!node)
            return false;
        if (node->cursorExplicit)
            return true;
        if (node->onClick)
            return true;
        if (node->clickable)
            return true;
        return false;
    };
    NodePtr fallback;
    auto allowHitForNode = [&](unsigned long long checkedNodeId) -> bool
    {
        const HTMLDomNode* node = nullptr;
        if (checkedNodeId < (unsigned long long)_nodeRawById.size())
            node = _nodeRawById[(size_t)checkedNodeId];
        if (!node)
        {
            fallback = lockNode(checkedNodeId);
            node = fallback.get();
        }
        if (!node)
            return false;
        if (!node->visible)
            return false;
        if (node->hasHoverColor || node->hasPressedColor)
            return true;
        if (node->tag == eHTMLTag::Checkbox)
            return true;
        switch (node->tag)
        {
        case eHTMLTag::P:
        case eHTMLTag::H1:
        case eHTMLTag::H2:
        case eHTMLTag::H3:
        case eHTMLTag::Center:
        case eHTMLTag::Table:
        case eHTMLTag::Tr:
        case eHTMLTag::Td:
            return isInteractiveRaw(node) || isCursorTrackedRaw(node);
        case eHTMLTag::Nine:
        case eHTMLTag::Three:
            if (node->spriteTex || node->spriteSrcW > 0.f || node->spriteSrcH > 0.f)
                return true;
            return isInteractiveRaw(node) || isCursorTrackedRaw(node);
        case eHTMLTag::Spine:
            if (!node->spinePath.empty())
                return true;
            return isInteractiveRaw(node) || isCursorTrackedRaw(node);
        default:
            return true;
        }
    };
    if (!allowHitForNode(nodeId))
        return;
    applyVisMatToRect(x, y, w, h);
    HitEntry e;
    e.nodeId = nodeId;
    e.rc.x = x;
    e.rc.y = y;
    e.rc.cx = w;
    e.rc.cy = h;
    auto& entries = _hitData[rootId];
    if (insertIndex > entries.size())
        insertIndex = entries.size();
    entries.insert(entries.begin() + insertIndex, e);
}

unsigned long long HTMLDom::hitTestInternal(unsigned long long rootId, float x, float y) const
{
    auto it = _hitData.find(rootId);
    if (it == _hitData.end())
        return 0;
    const auto& entries = it->second;
    for (int i = (int)entries.size() - 1; i >= 0; --i)
        if (rectContains(entries[i].rc, x, y))
            return entries[i].nodeId;
    return 0;
}

unsigned long long HTMLDom::hitTest(float x, float y) const
{
    return hitTestInternal(_lastRootId, x, y);
}

unsigned long long HTMLDom::hitTest(unsigned long long rootId, float x, float y) const
{
    return hitTestInternal(rootId, x, y);
}

bool HTMLDom::isInteractiveNode(const NodePtr& node) const
{
    if (!node)
        return false;
    if (node->clickable)
        return true;
    if (node->onClick)
        return true;
    if (node->tag == eHTMLTag::A)
        return true;
    if (node->tag == eHTMLTag::NineButton)
        return true;
    if (node->tag == eHTMLTag::Checkbox)
        return true;
    if (node->tag == eHTMLTag::Select)
        return true;
    if (node->tag == eHTMLTag::Option)
        return true;
    if (node->tag == eHTMLTag::Slider)
        return true;
    return false;
}

unsigned long long HTMLDom::findClickTarget(unsigned long long rootId, float x, float y) const
{
    unsigned long long id = hitTestInternal(rootId, x, y);
    while (id != 0)
    {
        auto node = lockNode(id);
        if (!node)
            return 0;
        if (node->disabled)
            return 0;
        if (isInteractiveNode(node))
            return id;
        id = node->parentId;
    }
    return 0;
}

void HTMLDom::setDocumentCallbacks(unsigned long long rootId, IHTMLDomCallbacks* callbacks)
{
    if (rootId == 0)
        return;
    if (callbacks)
        _rootCallbacks[rootId] = callbacks;
    else
        _rootCallbacks.erase(rootId);
}

IHTMLDomCallbacks* HTMLDom::getDocumentCallbacks(unsigned long long rootId) const
{
    auto it = _rootCallbacks.find(rootId);
    if (it == _rootCallbacks.end())
        return nullptr;
    return it->second;
}

bool HTMLDom::isCursorTrackedNode(const NodePtr& node) const
{
    if (!node)
        return false;
    if (node->cursorExplicit)
        return true;
    if (node->onClick)
        return true;
    if (node->clickable)
        return true;
    return false;
}

unsigned long long HTMLDom::findCursorTarget(unsigned long long rootId, float x, float y) const
{
    unsigned long long id = hitTestInternal(rootId, x, y);
    while (id != 0)
    {
        auto node = lockNode(id);
        if (!node)
            return 0;
        if (node->disabled)
            return 0;
        if (isCursorTrackedNode(node))
            return id;
        id = node->parentId;
    }
    return 0;
}

std::string HTMLDom::getNodeCursorName(unsigned long long nodeId) const
{
    auto node = lockNode(nodeId);
    if (!node)
        return "normal";
    if (node->cursor.empty())
        return "normal";
    return node->cursor;
}

void HTMLDom::updateCursorHover(unsigned long long rootId, unsigned long long newCursorTargetId)
{
    unsigned long long oldRoot = _cursorRootId;
    unsigned long long oldTarget = _cursorTargetId;
    if (oldTarget == newCursorTargetId && oldRoot == rootId)
        return;
    if (oldTarget != 0)
        if (auto cb = getDocumentCallbacks(oldRoot))
            cb->onLeave(oldRoot, oldTarget);
    std::string newCursor = "normal";
    if (newCursorTargetId != 0)
    {
        newCursor = getNodeCursorName(newCursorTargetId);
        if (newCursor.empty())
            newCursor = "normal";
        if (auto cb = getDocumentCallbacks(rootId))
            cb->onHover(rootId, newCursorTargetId);
    }
    _cursorRootId = (newCursorTargetId != 0) ? rootId : 0;
    _cursorTargetId = newCursorTargetId;
    if (newCursor != _currentCursor)
    {
        _currentCursor = newCursor;
        unsigned long long callbackRootId = (newCursorTargetId != 0) ? rootId : oldRoot;
        if (auto cb = getDocumentCallbacks(callbackRootId))
            cb->onCursorChanged(newCursor.c_str());
    }
}

void HTMLDom::refreshCurrentCursor()
{
    if (_cursorTargetId == 0)
        return;
    auto node = lockNode(_cursorTargetId);
    auto isDisabledChain = [&](unsigned long long id) -> bool
    {
        while (id != 0)
        {
            auto n = lockNode(id);
            if (!n)
                return true;
            if (n->disabled)
                return true;
            id = n->parentId;
        }
        return false;
    };
    if (!node || !isCursorTrackedNode(node) || isDisabledChain(_cursorTargetId))
    {
        unsigned long long oldRoot = _cursorRootId;
        unsigned long long oldTarget = _cursorTargetId;
        if (auto cb = getDocumentCallbacks(oldRoot))
        {
            cb->onLeave(oldRoot, oldTarget);
            if (_currentCursor != "normal")
                cb->onCursorChanged("normal");
        }
        _cursorRootId = 0;
        _cursorTargetId = 0;
        _currentCursor = "normal";
        return;
    }
    std::string cursorName = getNodeCursorName(_cursorTargetId);
    if (cursorName.empty())
        cursorName = "normal";
    if (cursorName != _currentCursor)
    {
        _currentCursor = cursorName;
        if (auto cb = getDocumentCallbacks(_cursorRootId))
            cb->onCursorChanged(cursorName.c_str());
    }
}

void HTMLDom::clearInputState()
{
    endSliderDrag(true);
    _pressedRootId = 0;
    _pressedTargetId = 0;
    _pressedButton = 0;
    _mouseButtonDown = false;
    _fgPressedTargetId = 0;
    stopEditDrag(true);
}

void HTMLDom::handleInput(eHTMLInputEvent evt, float x, float y, int button)
{
    handleInput(_lastRootId, evt, x, y, button);
}

void HTMLDom::handleInput(unsigned long long rootId, eHTMLInputEvent evt, float x, float y, int button)
{
    handleMouseEvent(rootId, evt, x, y, button);
}

void HTMLDom::handleMouseEvent(unsigned long long rootId, eHTMLInputEvent evt, float x, float y, int button)
{
    if (rootId == 0)
        return;
    _lastMouseX = x;
    _lastMouseY = y;
    auto isDisabledChain = [&](unsigned long long id) -> bool
    {
        while (id != 0)
        {
            auto n = lockNode(id);
            if (!n)
                return true;
            if (n->disabled)
                return true;
            id = n->parentId;
        }
        return false;
    };
    if (evt == eHTMLInputEvent::MOUSE_MOVE && _dragSliderId != 0 && _mouseButtonDown)
    {
        auto slider = lockNode(_dragSliderId);
        if (slider && slider->visible && slider->tag == eHTMLTag::Slider && !isDisabledChain(_dragSliderId))
            updateSliderFromPointer(slider, x, y, true);
        else
            endSliderDrag(true);
    }
    if (evt == eHTMLInputEvent::MOUSE_MOVE && _editDragId != 0 && _mouseButtonDown && _dragSliderId == 0)
    {
        auto editNode = lockNode(_editDragId);
        if (editNode && editNode->visible && editNode->tag == eHTMLTag::Edit &&
            !isDisabledChain(_editDragId) && _editDragRootId == rootId)
        {
            HTMLRenderState st = domMakeEditMeasureState(this, editNode);
            int pos = domEditPosFromPointer(editNode, x, st);
            if (editNode->editCursor != pos)
                editNode->editCursor = pos;
        }
        else
        {
            stopEditDrag(true);
        }
    }
    unsigned long long target = findClickTarget(rootId, x, y);
    if (evt == eHTMLInputEvent::MOUSE_DOWN)
    {
        unsigned long long editId = 0;
        unsigned long long id = target;
        while (id != 0)
        {
            auto n = lockNode(id);
            if (!n)
                break;
            if (n->tag == eHTMLTag::Edit)
            {
                editId = id;
                break;
            }
            id = n->parentId;
        }
        if (editId != 0)
        {
            auto n = lockNode(editId);
            if (n && !n->disabled)
            {
                if (_focusedEditId != editId)
                {
                    blurFocusedEdit();
                    _focusedEditId = editId;
                    _focusedEditRootId = rootId;
                    if (auto cb = getDocumentCallbacks(rootId))
                        cb->onEditFocusChanged(rootId, editId, true);
                    domEnsureEditInited(n);
                    domSyncVirtualKeyboard(n, true);
                }
                domEnsureEditInited(n);
                HTMLRenderState textState = domMakeEditMeasureState(this, n);
                int pos = domEditPosFromPointer(n, x, textState);
                n->editCursor = pos;
                n->editSelAnchor = pos;
                beginEditDrag(rootId, editId);
            }
        }
        else if (_focusedEditId != 0)
        {
            blurFocusedEdit();
        }
    }
    _hoverTargetId = target;
    _hoverRootId = rootId;
    auto findForegroundTarget = [&](unsigned long long startId) -> unsigned long long
    {
        unsigned long long id = startId;
        while (id != 0)
        {
            auto node = lockNode(id);
            if (!node)
                break;
            if (node->disabled)
                return 0;
            if (node->hasHoverColor || node->hasPressedColor)
                return id;
            id = node->parentId;
        }
        return 0;
    };
    unsigned long long rawFgTarget = hitTestInternal(rootId, x, y);
    _fgHoverTargetId = findForegroundTarget(rawFgTarget);
    unsigned long long cursorTarget = findCursorTarget(rootId, x, y);
    updateCursorHover(rootId, cursorTarget);
    switch (evt)
    {
    case eHTMLInputEvent::MOUSE_DOWN:
    {
        _mouseButtonDown = true;
        _pressedButton = button;
        _pressedRootId = rootId;
        _pressedTargetId = target;
        _fgPressedTargetId = _fgHoverTargetId;
        if (target != 0)
        {
            auto node = lockNode(target);
            if (node && node->tag == eHTMLTag::Slider && !node->disabled)
            {
                _dragSliderId = target;
                _dragRootId = rootId;
                updateSliderFromPointer(node, x, y, true);
                if (auto cb = getDocumentCallbacks(rootId))
                    cb->onDrag(rootId, target, false);
            }
        }
        break;
    }
    case eHTMLInputEvent::MOUSE_UP:
    {
        if (_mouseButtonDown && _pressedButton == button)
        {
            unsigned long long endedSliderId = 0;
            if (_dragSliderId != 0)
            {
                auto slider = lockNode(_dragSliderId);
                if (slider && slider->visible && !isDisabledChain(_dragSliderId))
                    updateSliderFromPointer(slider, x, y, true);
                endedSliderId = _dragSliderId;
                endSliderDrag(true);
            }
            stopEditDrag(true);
            if (_pressedTargetId != 0 && rootId == _pressedRootId && target == _pressedTargetId)
            {
                auto node = lockNode(_pressedTargetId);
                if (node && !node->disabled)
                {
                    bool skipClick = (endedSliderId != 0 && node->id == endedSliderId);
                    if (!skipClick)
                    {
                        if (node->tag == eHTMLTag::Checkbox)
                            node->checked = !node->checked;
                        if (node->tag == eHTMLTag::Select)
                            if (auto cb = getDocumentCallbacks(rootId))
                                cb->onSelectDropdownRequested(rootId, node->id);
                        if (node->onClick)
                            node->onClick(node->id, x, y, button);
                    }
                }
            }
            _mouseButtonDown = false;
            _pressedButton = 0;
            _pressedRootId = 0;
            _pressedTargetId = 0;
            _fgPressedTargetId = 0;
        }
        break;
    }
    case eHTMLInputEvent::MOUSE_MOVE:
    default:
        break;
    }
}

void HTMLDom::restartAppear(unsigned long long id)
{
    auto it = _appearStarts.find(id);
    if (it == _appearStarts.end())
        _appearStarts.emplace(id, FxAnimState{ _fAnimTime, _fAnimTime, false, true });
    else
    {
        it->second.fBlockStart = _fAnimTime;
        it->second.fAnimTime = _fAnimTime;
        it->second.bIsPaused = false;
        it->second.bIsInitiated = true;
    }
#ifdef HAS_SPINE
    restartSpinesForBlock(id);
#endif
}

void HTMLDom::removeAppearState(unsigned long long id)
{
    _appearStarts.erase(id);
    _fontAnimEventFlags.erase(id);
#ifdef HAS_SPINE
    removeSpinesForBlock(id);
#endif
}

void HTMLDom::pauseAnimsForHTML(unsigned long long id, bool bIsPaused)
{
    auto it = _appearStarts.find(id);
    if (it != _appearStarts.end())
        it->second.bIsPaused = bIsPaused;
    else
        _appearStarts.emplace(id, FxAnimState{ _fAnimTime, _fAnimTime, bIsPaused, true });
}

void HTMLDom::clearAppearStates(void)
{
    _appearStarts.clear();
#ifdef HAS_SPINE
    _spineStates.clear();
    _spineKeyByNode.clear();
#endif
    _fontAnimEventFlags.clear();
}

#ifdef HAS_SPINE
unsigned long long HTMLDom::makeSpineKey(unsigned long long blockKey, unsigned long long spineId)
{
    unsigned long long h = blockKey;
    h ^= spineId + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    return h;
}

void HTMLDom::restartSpinesForBlock(unsigned long long blockKey)
{
    for (auto& kv : _spineStates)
    {
        if (kv.second.blockKey != blockKey)
            continue;
        kv.second.animSet = false;
        kv.second.startTime = _fAnimTime;
        kv.second.lastTime = _fAnimTime;
        if (kv.second.p)
        {
            kv.second.p->clearTracks();
            kv.second.p->setToSetupPose();
        }
    }
}

void HTMLDom::removeSpinesForBlock(unsigned long long blockKey)
{
    for (auto it = _spineStates.begin(); it != _spineStates.end(); )
    {
        if (it->second.blockKey == blockKey)
            it = _spineStates.erase(it);
        else
            ++it;
    }
}

#endif

Rect HTMLDom::render(unsigned long long rootId, float maxWidth)
{
    return renderInternal(rootId, maxWidth, false);
}

Rect HTMLDom::measure(unsigned long long rootId, float maxWidth)
{
    return renderInternal(rootId, maxWidth, true);
}

Rect HTMLDom::renderInternal(unsigned long long rootId, float maxWidth, bool bMeasureOnly)
{
    auto root = getNode(rootId);
    if (!root)
        return Rect{};
    HTMLRenderScratch* sc = ensureScratch();
    if (!root->visible)
    {
        _lastRootId = rootId;
        if (!bMeasureOnly)
            clearHitData(rootId);
        return Rect{};
    }
    _lastRootId = rootId;
    if (!bMeasureOnly)
        clearHitData(rootId);
    HTMLDomOptions options = getRootOptions(rootId);
    LayoutParams rootParams;
    rootParams.x = 0.f;
    rootParams.y = 0.f;
    rootParams.maxWidth = maxWidth;
    rootParams.maxHeight = options.maxHeight;
    rootParams.align = options.align;
    rootParams.color = options.color;
    rootParams.appearStart = options.appearStart;
    rootParams.animKey = (options.animId != 0) ? options.animId : rootId;

    if (!sc->rootStateInited)
    {
        sc->rootState = domMakeDefaultState(0xFFFFFFFF, FONS_ALIGN_LEFT);
        sc->rootStateInited = true;
    }
    sc->rootState.color = options.color;
    sc->rootState.align = options.align | FONS_ALIGN_TOP;
    sc->beginFrame();
    domRefreshFontState(sc->rootState);
    if (_btnGroupCount > 0)
        domCollectBtnGroupWidths(this, root, sc->rootState, *sc);
    Rect rc;
    renderSequence(
        root->children,
        sc->rootState,
        rootParams,
        &rc,
        bMeasureOnly,
        rootId
    );
    return rc;
}

void HTMLDom::renderSequence(
    const std::vector<NodePtr>& children,
    HTMLRenderState& state,
    const LayoutParams& params,
    Rect* rcOut,
    bool bMeasureOnly,
    unsigned long long rootId)
{
    if (!rcOut)
        return;
    HTMLRenderScratch& sc = *ensureScratch();
    RenderContext ctx;
    ctx.dom = this;
    ctx.fs = domFs();
    RenderContext* prevRenderCtx = _currentRenderContext;
    if (prevRenderCtx)
    {
        ctx.inheritedDisabledTint = prevRenderCtx->inheritedDisabledTint;
        ctx.inheritedDisabled = prevRenderCtx->inheritedDisabled;
    }
    _currentRenderContext = &ctx;
    struct PrevCtxGuard
    {
        HTMLDom* dom;
        RenderContext* prev;
        ~PrevCtxGuard()
        {
            dom->_currentRenderContext = prev;
        }
    } prevCtxGuard{ this, prevRenderCtx };
    ctx.rootId = rootId;
    ctx.params = params;
    ctx.measureOnly = bMeasureOnly;
    ctx.rcOut = rcOut;
    ctx.currentX = params.x;
    ctx.currentY = params.y;
    ctx.hasBounds = false;
    ctx.maxContentWidth = 0.f;
    ctx.currentCursorId = 0;
    ctx.currentFontAnimId = 0;
    ctx.documentColor = getRootOptions(rootId).documentColor;
    ctx.extraAlpha = _extraAlphaStack.empty() ? 1.f : _extraAlphaStack.back();

    ctx.baseChunk = sc.chunkCount;
    ctx.totalWidth = 0.f;
    rcOut->set(params.x, params.y, params.x, params.y);
    unsigned long long blockKey = params.animKey;
    if (blockKey == 0)
        blockKey = rootId;
    float blockStart = params.appearStart;
    float fAnimTime = 0.f;
    if (blockStart < 0.f)
    {
        if (bMeasureOnly)
        {
            blockStart = 0.f;
        }
        else
        {
            auto it = _appearStarts.find(blockKey);
            if (it == _appearStarts.end())
            {
                it = _appearStarts.emplace(
                    blockKey,
                    FxAnimState{ _fAnimTime, _fAnimTime, false, true }
                ).first;
            }
            else if (!it->second.bIsInitiated)
            {
                it->second.bIsInitiated = true;
                it->second.fAnimTime = _fAnimTime;
                it->second.fBlockStart = _fAnimTime;
            }
            blockStart = it->second.fBlockStart;
            if (!it->second.bIsPaused)
                it->second.fAnimTime = _fAnimTime;
            fAnimTime = it->second.fAnimTime;
        }
    }
    if (!bMeasureOnly && params.appearStart >= 0.f)
        fAnimTime = _fAnimTime;
    ctx.animKey = blockKey;
    ctx.blockStart = blockStart;
    ctx.animTime = fAnimTime;
    domRefreshFontState(state);
    ctx.lineHeight = domGetLineHeightForState(state);
    for (const auto& child : children)
        renderNode(child, state, ctx);
    ctx.flushLine(state, false);

    sc.chunkCount = ctx.baseChunk;
    float effW = std::max(0.f, ctx.maxContentWidth);
    float effH = std::max(0.f, ctx.currentY - params.y);
    rcOut->set(params.x, params.y, params.x + effW, params.y + effH);
}

void HTMLDom::renderChildren(const NodePtr& parent, HTMLRenderState& state, RenderContext& ctx)
{
    if (!parent)
        return;
    for (const auto& child : parent->children)
        renderNode(child, state, ctx);
}

void HTMLDom::applyFontAttrs(HTMLRenderState& st, const HTMLFontAttrs& attrs)
{
    applyFontAttrsToState(st, attrs);
}
void HTMLDom::renderNode(const NodePtr& node, HTMLRenderState& state, RenderContext& ctx)
{
    if (!node)
        return;
    if (!node->visible)
        return;
    unsigned long long prevCursorId = ctx.currentCursorId;
    if (isCursorTrackedNode(node))
        ctx.currentCursorId = node->id;
    struct CursorIdGuard
    {
        RenderContext& ctx;
        unsigned long long prev;
        ~CursorIdGuard()
        {
            ctx.currentCursorId = prev;
        }
    } cursorGuard{ ctx, prevCursorId };
    const unsigned int savedInheritTint = ctx.inheritedDisabledTint;
    const bool savedInheritDisabled = ctx.inheritedDisabled;
    unsigned int chainTint = ctx.inheritedDisabledTint;
    bool chainDisabled = ctx.inheritedDisabled;
    if (node->disabled)
    {
        chainDisabled = true;
        chainTint = domMulColor(chainTint, domNodeDisabledTint(*node));
    }
    ctx.inheritedDisabledTint = chainTint;
    ctx.inheritedDisabled = chainDisabled;
    ctx.disabledTint = chainTint;
    ctx.grayScale = chainDisabled;
    struct InheritGuard
    {
        RenderContext& ctx;
        unsigned int savedTint;
        bool savedDisabled;
        ~InheritGuard()
        {
            ctx.inheritedDisabledTint = savedTint;
            ctx.inheritedDisabled = savedDisabled;
        }
    } inheritGuard{ ctx, savedInheritTint, savedInheritDisabled };
    const float savedExtraAlpha = ctx.extraAlpha;
    ctx.extraAlpha *= node->visAlpha;
    if (node->visAlpha != 1.f)
        _extraAlphaStack.push_back(ctx.extraAlpha);
    struct ExtraAlphaGuard
    {
        RenderContext& ctx;
        std::vector<float>& stack;
        bool pushed;
        float prev;
        ~ExtraAlphaGuard()
        {
            if (pushed)
                stack.pop_back();
            ctx.extraAlpha = prev;
        }
    } extraAlphaGuard{ ctx, _extraAlphaStack, node->visAlpha != 1.f, savedExtraAlpha };
    const unsigned int combinedTint = domMulColor(ctx.documentColor,
        domMulColor(ctx.disabledTint, domAlphaColor(ctx.extraAlpha)));
    auto tint = [combinedTint](unsigned int c) -> unsigned int
    {
        return domMulColor(c, combinedTint);
    };
    const bool interactionBlocked = chainDisabled;

    unsigned int savedFgColor = state.color;
    bool fgOverride = false;
    const bool savedFadeActive = _fgFadeActive;
    const float savedFadeHover = _fgFadeHover;
    const float savedFadePressed = _fgFadePressed;
    const unsigned int savedFadeHoverColor = _fgFadeHoverColor;
    const unsigned int savedFadePressedColor = _fgFadePressedColor;
    if (!ctx.measureOnly &&
        !interactionBlocked &&
        (node->hasHoverColor || node->hasPressedColor))
    {
        bool fgHovered = (_fgHoverTargetId == node->id);
        bool fgPressed =
            _mouseButtonDown &&
            (_fgPressedTargetId == node->id) &&
            fgHovered;
        float hoverDur = 0.12f;
        float pressedDur = 0.08f;
        getButtonFadeDurations(*node, hoverDur, pressedDur);
        auto& fx = _domFgFxMap[node->id];
        float now = _fAnimTime;
        if (fx.lastTime < 0.f)
            fx.lastTime = now;
        float dt = std::clamp(now - fx.lastTime, 0.f, 0.25f);
        fx.lastTime = now;
        fx.hover = buttonApproach(fx.hover, fgHovered ? 1.f : 0.f, hoverDur, dt);
        fx.pressed = buttonApproach(fx.pressed, fgPressed ? 1.f : 0.f, pressedDur, dt);
        unsigned int c = savedFgColor;
        if (node->hasHoverColor && fx.hover > 0.001f)
            c = lerpColor(c, node->hoverColor, fx.hover);
        if (node->hasPressedColor && fx.pressed > 0.001f)
            c = lerpColor(c, node->pressedColor, fx.pressed);
        state.color = c;
        fgOverride = true;
        _fgFadeActive = true;
        _fgFadeHover = fx.hover;
        _fgFadePressed = fx.pressed;
        _fgFadeHoverColor = node->hoverColor;
        _fgFadePressedColor = node->pressedColor;
    }
    struct ColorGuard
    {
        HTMLRenderState& st;
        unsigned int oldColor;
        bool active;
        ~ColorGuard()
        {
            if (active)
                st.color = oldColor;
        }
    } colorGuard{ state, savedFgColor, fgOverride };
    const bool bBlockTag = domIsBlockTag(node->tag) && !(node->tag == eHTMLTag::Img && node->imgInline);
    const bool bHasMargin = bBlockTag &&
        (node->margin.top != 0.f || node->margin.bottom != 0.f ||
         node->margin.left != 0.f || node->margin.right != 0.f);
    LayoutParams savedParams = ctx.params;
    if (bHasMargin)
    {
        ctx.flushLine(state, false);
        ctx.params.x += node->margin.left;
        if (ctx.params.maxWidth > 0.f)
            ctx.params.maxWidth = std::max(0.f, ctx.params.maxWidth - node->margin.left - node->margin.right);
        ctx.currentY += node->margin.top;
        ctx.currentX = ctx.params.x;
    }
    if (bBlockTag && node->positionRelative && !ctx.measureOnly &&
        (node->relX != 0.f || node->relY != 0.f))
    {
        ctx.flushLine(state, false);
    }
    const float relDX = (bBlockTag && node->positionRelative && !ctx.measureOnly) ? node->relX : 0.f;
    const float relDY = (bBlockTag && node->positionRelative && !ctx.measureOnly) ? node->relY : 0.f;
    const bool bRelShift = (relDX != 0.f || relDY != 0.f);
    if (bRelShift)
    {
        ctx.currentX += relDX;
        ctx.currentY += relDY;
        ctx.params.x += relDX;
    }
    const float visEntryX = ctx.currentX;
    const float visEntryY = ctx.currentY;
    const bool bVisTransform =
        !ctx.measureOnly &&
        (node->visOffX != 0.f || node->visOffY != 0.f ||
         node->visScaleX != 1.f || node->visScaleY != 1.f ||
         node->visRot != 0.f);
    Rect visSelfRc;
    bool visGotSelf = false;
    if (bVisTransform)
    {
        ctx.visRecordRc = &visSelfRc;
        ctx.visRecordGot = &visGotSelf;
        float ppx = 0.f, ppy = 0.f;
        domVisPivot(*node, visEntryX, visEntryY, ppx, ppy);
        Aff2 local;
        affSet(local, ppx, ppy, node->visRot, node->visScaleX, node->visScaleY, node->visOffX, node->visOffY);
        VisMat vm;
        if (_visMatStack.empty())
        {
            memcpy(vm.m, local.m, sizeof(vm.m));
        }
        else
        {
            Aff2 parent;
            memcpy(parent.m, _visMatStack.back().m, sizeof(parent.m));
            Aff2 composed;
            affMul(composed, parent, local);
            memcpy(vm.m, composed.m, sizeof(vm.m));
        }
        _visMatStack.push_back(vm);
        if (auto pMS = CGfx::getInstance()->getMatrixStack())
        {
            pMS->save();
            pMS->translate(ppx + node->visOffX, ppy + node->visOffY);
            if (node->visRot != 0.f)
                pMS->rotate(node->visRot);
            if (node->visScaleX != 1.f || node->visScaleY != 1.f)
                pMS->scale(node->visScaleX, node->visScaleY);
            pMS->translate(-ppx, -ppy);
        }
    }
    switch (node->tag)
    {
    case eHTMLTag::Document:
        renderChildren(node, state, ctx);
        break;
    case eHTMLTag::Body:
    {
        ctx.flushLine(state, false);
        const bool hasChild = !node->children.empty();
        size_t hitsBeforeChildren = 0;
        if (!ctx.measureOnly)
            hitsBeforeChildren = _hitData[ctx.rootId].size();
        LayoutParams childParams;
        childParams.x = ctx.currentX;
        childParams.y = ctx.currentY;
        childParams.maxWidth = ctx.params.maxWidth;
        childParams.align = (state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT)) | FONS_ALIGN_TOP;
        childParams.color = state.color;
        childParams.appearStart = ctx.blockStart;
        childParams.animKey = ctx.animKey;
        float bodyX = 0.f, bodyY = 0.f, bodyW = 0.f, bodyH = 0.f;
        {
            HTMLStateScope bodyScope(state, *_scratch);
            state.align = childParams.align;
            domRefreshFontState(state);
            Rect childRc;
            if (hasChild)
                renderSequence(node->children, state, childParams, &childRc, true, ctx.rootId);
            else
                childRc.set(childParams.x, childParams.y, childParams.x, childParams.y);
            bodyX = childRc.x;
            bodyY = childRc.y;
            bodyW = std::max(childRc.cx, ctx.params.maxWidth > 0.f ? ctx.params.maxWidth : 0.f);
            bodyH = childRc.cy;
            bodyW = std::max(bodyW, node->bodyMinWidth);
            bodyH = std::max(bodyH, node->bodyMinHeight);
            if (bodyW <= 0.f)
                bodyW = 1.f;
            if (bodyH <= 0.f)
                bodyH = 1.f;
            if (!ctx.measureOnly)
            {
                if (node->bgColor != 0)
                    ctx.renderLine(bodyX, bodyY, bodyW, bodyH, tint(node->bgColor));
                if (node->hasBodyBg && node->spriteTex)
                {
                    float imgW = bodyW, imgH = bodyH;
                    float imgX = bodyX, imgY = bodyY;
                    if (node->bodyBgSize != 2 && node->spriteSrcW > 0.f && node->spriteSrcH > 0.f)
                    {
                        float s;
                        if (node->bodyBgSize == 1)
                            s = std::min(bodyW / node->spriteSrcW, bodyH / node->spriteSrcH);
                        else
                            s = std::max(bodyW / node->spriteSrcW, bodyH / node->spriteSrcH);
                        imgW = node->spriteSrcW * s;
                        imgH = node->spriteSrcH * s;
                        imgX = bodyX + (bodyW - imgW) * 0.5f;
                        imgY = bodyY + (bodyH - imgH) * 0.5f;
                    }
                    drawSpriteQuadColorAlpha(node->spriteTex, imgX, imgY, imgW, imgH,
                        node->spriteUV[0], node->spriteUV[1],
                        node->spriteUV[2], node->spriteUV[3],
                        tint(0xFFFFFFFF), node->bodyBgOpacity, ctx.grayScale);
                }
            }
            ctx.updateBounds(bodyX, bodyY, bodyW, bodyH);
            if (bodyW > ctx.maxContentWidth)
                ctx.maxContentWidth = bodyW;
            if (!ctx.measureOnly)
                registerHitRectAt(ctx.rootId, node->id, bodyX, bodyY, bodyW, bodyH, hitsBeforeChildren);
            if (hasChild)
            {
                Rect renderRc;
                renderSequence(node->children, state, childParams, &renderRc, ctx.measureOnly, ctx.rootId);
                if (!ctx.measureOnly)
                    bodyH = renderRc.cy;
            }
        }
        ctx.currentY = bodyY + bodyH;
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
#ifdef HAS_SPINE
    case eHTMLTag::BgSpine:
    {

        ctx.flushLine(state, false);
        const bool hasChild = !node->children.empty();
        size_t hitsBeforeChildren = 0;
        if (!ctx.measureOnly)
            hitsBeforeChildren = _hitData[ctx.rootId].size();

        HTMLRenderScratch& scBg = *_scratch;
        const std::vector<NodePtr>* pFlow = &node->children;
        std::vector<NodePtr> flowChildren;
        bool bHasInl = false;
        for (const auto& ch : node->children)
        {
            if (ch && ch->findEffectiveAttr("spine-inl"))
            {
                bHasInl = true;
                break;
            }
        }
        if (bHasInl)
        {
            flowChildren.reserve(node->children.size());
            for (const auto& ch : node->children)
            {
                if (ch && !ch->attrs.find("spine-inl"))
                    flowChildren.push_back(ch);
            }
            pFlow = &flowChildren;
        }
        const bool hasFlowChild = !pFlow->empty();

        LayoutParams childParams;
        childParams.x = ctx.currentX;
        childParams.y = ctx.currentY;
        childParams.maxWidth = ctx.params.maxWidth;
        childParams.align = (state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT)) | FONS_ALIGN_TOP;
        childParams.color = state.color;
        childParams.appearStart = ctx.blockStart;
        childParams.animKey = ctx.animKey;
        float bgX = 0.f, bgY = 0.f, bgW = 0.f, bgH = 0.f;
        {
            HTMLStateScope bodyScope(state, scBg);
            state.align = childParams.align;
            domRefreshFontState(state);
            Rect childRc;
            if (hasFlowChild)
                renderSequence(*pFlow, state, childParams, &childRc, ctx.measureOnly, ctx.rootId);
            else
                childRc.set(childParams.x, childParams.y, childParams.x, childParams.y);
            bgX = childRc.x;
            bgY = childRc.y;
            bgW = std::max(childRc.cx, ctx.params.maxWidth > 0.f ? ctx.params.maxWidth : 0.f);
            bgH = childRc.cy;
            bgW = std::max(bgW, node->bodyMinWidth);
            bgH = std::max(bgH, node->bodyMinHeight);
            if (bgW <= 0.f)
                bgW = 1.f;
            if (bgH <= 0.f)
                bgH = 1.f;
            ctx.updateBounds(bgX, bgY, bgW, bgH);
            if (bgW > ctx.maxContentWidth)
                ctx.maxContentWidth = bgW;
            if (!ctx.measureOnly)
                registerHitRectAt(ctx.rootId, node->id, bgX, bgY, bgW, bgH, hitsBeforeChildren);
        }

        if (!ctx.measureOnly && !node->spinePath.empty())
        {
            unsigned long long spineKey = makeSpineKey(ctx.animKey, node->id);
            auto it = _spineStates.find(spineKey);
            if (it == _spineStates.end())
            {
                SpineRuntimeState st;
                st.blockKey = ctx.animKey;
                st.startTime = ctx.blockStart;
                st.lastTime = ctx.animTime;
                it = _spineStates.emplace(spineKey, std::move(st)).first;
            }
            auto& st = it->second;
            st.nodeId = node->id;
            st.rootId = ctx.rootId;
            _spineKeyByNode[node->id] = spineKey;
            float dt = ctx.animTime - st.lastTime;
            if (dt < 0.f)
                dt = 0.f;
            if (dt > 0.25f)
                dt = 0.25f;
            st.lastTime = ctx.animTime;
            if (!st.p)
                st.p = CSpineManager::getInstance()->getNewSpine(node->spinePath.c_str());
            if (st.p)
            {
                st.p->setSkipLight(true);
                st.p->setAlpha(node->bodyBgOpacity);
                if (!st.animSet)
                {
                    if (!node->spineAnim.empty() && st.p->hasAnimation(node->spineAnim.c_str()))
                    {
                        st.p->setAnimation(0, node->spineAnim.c_str(), node->spineLoop);
                        st.p->update(0.f);
                    }
                    st.animSet = true;
                }
                else
                {
                    st.p->update(dt);
                }

                float l = 0.f, tp = 0.f, boxW = 0.f, boxH = 0.f;
                if (node->spineHasRcBox)
                {
                    l = std::min(node->spineRc[0], node->spineRc[2]);
                    tp = std::min(node->spineRc[1], node->spineRc[3]);
                    boxW = std::max(node->spineRc[0], node->spineRc[2]) - l;
                    boxH = std::max(node->spineRc[1], node->spineRc[3]) - tp;
                }
                else if (node->reqWidth > 0.f && node->reqHeight > 0.f)
                {
                    boxW = node->reqWidth;
                    boxH = node->reqHeight;
                }
                else
                {
                    Rect rc;
                    st.p->getRect(&rc);
                    l = rc.x;
                    tp = rc.y;
                    boxW = rc.cx;
                    boxH = rc.cy;
                }
                if (boxW <= 0.f || boxH <= 0.f)
                    boxW = boxH = 1.f;

                float sx, sy, offX, offY;
                if (node->bodyBgSize == 2)
                {
                    sx = bgW / boxW;
                    sy = bgH / boxH;
                    offX = bgX;
                    offY = bgY;
                }
                else
                {
                    float s;
                    if (node->bodyBgSize == 1)
                        s = std::min(bgW / boxW, bgH / boxH);
                    else
                        s = std::max(bgW / boxW, bgH / boxH);
                    sx = sy = s;
                    offX = bgX + (bgW - boxW * s) * 0.5f;
                    offY = bgY + (bgH - boxH * s) * 0.5f;
                }

                unsigned int spineColor = combinedTint;
                if (ctx.grayScale)
                    spineColor = domGrayColor(spineColor);
                float docTint[4] =
                {
                    ((spineColor >> 24) & 0xFF) / 255.f,
                    ((spineColor >> 16) & 0xFF) / 255.f,
                    ((spineColor >> 8) & 0xFF) / 255.f,
                    ((spineColor >> 0) & 0xFF) / 255.f
                };
                auto pMS = CGfx::getInstance()->getMatrixStack();
                pMS->save();
                pMS->translate(offX - l * sx, offY - tp * sy);
                if (sx != 1.f || sy != 1.f)
                    pMS->scale(sx, sy);
                st.p->renderSelf(dt, docTint);
                pMS->restore();

                if (bHasInl)
                {
                    for (const auto& ch : node->children)
                    {
                        if (!ch)
                            continue;
                        auto pBB = ch->findEffectiveAttr("spine-inl");
                        if (!pBB || pBB->empty())
                            continue;
                        Rect rcBB;
                        bool bFound = st.p->getBoundingBoxRect(pBB->c_str(), { 0.f, 0.f }, rcBB);
                        LayoutParams cp;
                        cp.color = state.color;
                        cp.appearStart = ctx.blockStart;
                        cp.animKey = ctx.animKey;
                        if (bFound)
                        {
                            cp.x = offX + (rcBB.x - l) * sx;
                            cp.y = offY + (rcBB.y - tp) * sy;
                            cp.maxWidth = std::max(1.f, rcBB.cx * sx);
                            cp.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
                        }
                        else
                        {

                            assert(false && "bgspine: spine-inl BB not found");
                            cp.x = bgX;
                            cp.y = bgY;
                            cp.maxWidth = std::max(1.f, bgW);
                            cp.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
                        }

                        flowChildren.clear();
                        flowChildren.push_back(ch);
                        Rect childRc;
                        HTMLStateScope cs(state, scBg);
                        state.align = cp.align;
                        domRefreshFontState(state);
                        renderSequence(flowChildren, state, cp, &childRc, false, ctx.rootId);
                    }
                }
            }
        }
        ctx.currentY = bgY + bgH;
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
#endif
    case eHTMLTag::Text:
    {

        const char* tp = node->text.c_str();
        const int n = (int)node->text.size();
        int i = 0;
        while (i < n)
        {
            const int wordStart = i;
            while (i < n && domIsAsciiSpaceChar((unsigned char)tp[i]))
                ++i;
            while (i < n && !domIsAsciiSpaceChar((unsigned char)tp[i]))
                i += getUtf8CharLen(tp + i);
            const int wordLen = i - wordStart;
            if (wordLen > 0)
            {
                const float w = domMeasureStyledTextRange(tp + wordStart, wordLen, state);
                ctx.addWord(tp + wordStart, wordLen, w, state);
            }
        }
        break;
    }
    case eHTMLTag::Br:
        ctx.flushLine(state, true);
        break;
    case eHTMLTag::Hr:
    {
        ctx.flushLine(state, false);
        float margin = (node->hrMargin >= 0.f) ? node->hrMargin : ctx.lineHeight * 0.5f;
        float hrWidth = node->hrWidth;
        if (hrWidth <= 0.f)
            hrWidth = (ctx.params.maxWidth > 0.f) ? ctx.params.maxWidth : 200.f;
        if (ctx.params.maxWidth > 0.f && hrWidth > ctx.params.maxWidth)
            hrWidth = ctx.params.maxWidth;
        float hrX = ctx.params.x;
        int hAlign = state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT);
        if (ctx.params.maxWidth > 0.f)
        {
            if (hAlign == FONS_ALIGN_CENTER)
                hrX = ctx.params.x + (ctx.params.maxWidth - hrWidth) * 0.5f;
            else if (hAlign == FONS_ALIGN_RIGHT)
                hrX = ctx.params.x + ctx.params.maxWidth - hrWidth;
        }
        ctx.renderLine(hrX, ctx.currentY + margin, hrWidth, node->hrThickness, tint(node->hrColor));
        ctx.updateBounds(hrX, ctx.currentY + margin, hrWidth, node->hrThickness);
        if (hrWidth > ctx.maxContentWidth)
            ctx.maxContentWidth = hrWidth;
        if (!ctx.measureOnly)
            registerHitRect(ctx.rootId, node->id, hrX, ctx.currentY + margin, hrWidth, node->hrThickness);
        ctx.currentY += node->hrThickness + margin * 2.f;
        ctx.currentX = ctx.params.x;
        break;
    }
    case eHTMLTag::Img:
    {
        if (node->imgInline)
        {
            float imgWidth = 0.f, imgHeight = 0.f;
            computeImgSize(node->spriteSrcW, node->spriteSrcH, node->reqWidth, node->reqHeight, node->keepAspect, imgWidth, imgHeight);
            if (ctx.params.maxWidth > 0.f && imgWidth > ctx.params.maxWidth)
            {
                float scale = ctx.params.maxWidth / imgWidth;
                imgWidth = ctx.params.maxWidth;
                if (node->keepAspect || node->reqHeight <= 0.f)
                    imgHeight *= scale;
            }
            if (imgWidth <= 0.f || imgHeight <= 0.f)
                break;
            const float mL = std::max(0.f, node->margin.left);
            const float mR = std::max(0.f, node->margin.right);
            const float wTot = imgWidth + mL + mR;
            HTMLRenderScratch& scImg = *_scratch;
            if (ctx.params.maxWidth > 0.f &&
                ctx.totalWidth + wTot > ctx.params.maxWidth &&
                scImg.chunkCount > ctx.baseChunk)
            {
                ctx.flushLine(state, true);
            }
            unsigned int imgColor = tint(0xFFFFFFFF);
            if (ctx.grayScale)
                imgColor = domGrayColor(imgColor);
            ctx.pushImageChunk(*node, imgWidth, imgHeight, mL, mR, imgColor, state);
            break;
        }
        ctx.flushLine(state, false);
        float imgWidth = 0.f, imgHeight = 0.f;
        computeImgSize(node->spriteSrcW, node->spriteSrcH, node->reqWidth, node->reqHeight, node->keepAspect, imgWidth, imgHeight);
        if (imgWidth <= 0.f && imgHeight <= 0.f)
            break;
        if (ctx.params.maxWidth > 0.f && imgWidth > ctx.params.maxWidth)
        {
            if (imgWidth > 0.f)
            {
                float scale = ctx.params.maxWidth / imgWidth;
                imgWidth = ctx.params.maxWidth;
                if (node->keepAspect || node->reqHeight <= 0.f)
                    imgHeight *= scale;
            }
            else
            {
                imgWidth = ctx.params.maxWidth;
            }
        }
        if (imgWidth <= 0.f)
            imgWidth = 1.f;
        if (imgHeight <= 0.f)
            imgHeight = ctx.lineHeight;
        float imgX = ctx.currentX;
        int hAlign = state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT);
        if (hAlign == FONS_ALIGN_CENTER)
        {
            if (ctx.params.maxWidth > 0.f)
                imgX = ctx.params.x + (ctx.params.maxWidth - imgWidth) * 0.5f;
        }
        else if (hAlign == FONS_ALIGN_RIGHT && ctx.params.maxWidth > 0.f)
        {
            imgX = ctx.params.x + ctx.params.maxWidth - imgWidth;
        }
        float imgY = ctx.currentY;
        if (node->spriteTex && !ctx.measureOnly)
        {
            bool destructiveIdle =
                node->idleStyle == eHTMLIdleStyle::Tear ||
                node->idleStyle == eHTMLIdleStyle::Lightning ||
                node->idleStyle == eHTMLIdleStyle::Ice ||
                node->idleStyle == eHTMLIdleStyle::Melt ||
                node->idleStyle == eHTMLIdleStyle::Dust ||
                node->idleStyle == eHTMLIdleStyle::Vortex ||
                node->idleStyle == eHTMLIdleStyle::Confetti ||
                node->idleStyle == eHTMLIdleStyle::Echo;
            if (!destructiveIdle)
            {
                drawSpriteQuadColorAlpha(
                    node->spriteTex,
                    imgX, imgY, imgWidth, imgHeight,
                    node->spriteUV[0], node->spriteUV[1],
                    node->spriteUV[2], node->spriteUV[3],
                    tint(0xFFFFFFFF),
                    1.f,
                    ctx.grayScale
                );
            }
            HTMLNode hn = makeHTMLNodeForIdle(*node);
            const float idleNow = idleRestartTime(ctx.animTime - node->idleTimeOffset, node->idleDelay);
            switch (node->idleStyle)
            {
            case eHTMLIdleStyle::Glare: renderImgGlare(hn, imgX, imgY, imgWidth, imgHeight, idleNow); break;
            case eHTMLIdleStyle::Pixel: renderImgPixel(hn, imgX, imgY, imgWidth, imgHeight, idleNow); break;
            case eHTMLIdleStyle::Chase: renderImgChase(hn, imgX, imgY, imgWidth, imgHeight, idleNow); break;
            case eHTMLIdleStyle::Embers: renderImgEmbers(hn, imgX, imgY, imgWidth, imgHeight, idleNow); break;
            case eHTMLIdleStyle::Lens: renderImgLens(hn, imgX, imgY, imgWidth, imgHeight, ctx.animTime); break;
            case eHTMLIdleStyle::Echo: renderImgEcho(hn, imgX, imgY, imgWidth, imgHeight, ctx.animTime); break;
            case eHTMLIdleStyle::Tear: renderImgTear(hn, imgX, imgY, imgWidth, imgHeight, ctx.animTime); break;
            case eHTMLIdleStyle::Lightning: renderImgLightning(hn, imgX, imgY, imgWidth, imgHeight, ctx.animTime); break;
            case eHTMLIdleStyle::Ice: renderImgIce(hn, imgX, imgY, imgWidth, imgHeight, ctx.animTime); break;
            case eHTMLIdleStyle::Melt: renderImgMelt(hn, imgX, imgY, imgWidth, imgHeight, ctx.animTime); break;
            case eHTMLIdleStyle::Dust: renderImgDust(hn, imgX, imgY, imgWidth, imgHeight, ctx.animTime); break;
            case eHTMLIdleStyle::Vortex: renderImgVortex(hn, imgX, imgY, imgWidth, imgHeight, ctx.animTime); break;
            case eHTMLIdleStyle::Confetti: renderImgConfetti(hn, imgX, imgY, imgWidth, imgHeight, ctx.animTime); break;
            default: break;
            }
            if (node->idleStyle != eHTMLIdleStyle::None && node->idleDur > 0.f && !node->imgAnimCompletedSent)
            {
                float imgLocal = ctx.animTime - node->idleTimeOffset;
                if (node->idleDelay > 0.f)
                    imgLocal -= node->idleDelay;
                if (imgLocal >= node->idleDur)
                {
                    node->imgAnimCompletedSent = true;
                    if (nodeHasIdAttr(node))
                    {
                        if (auto cb = getDocumentCallbacks(ctx.rootId))
                            cb->onImageAnimEvent(ctx.rootId, node->id, eHTMLImageAnimEventType::AnimationComplete, node->idleStyle);
                    }
                }
            }
        }
        ctx.updateBounds(imgX, imgY, imgWidth, imgHeight);
        if (imgWidth > ctx.maxContentWidth)
            ctx.maxContentWidth = imgWidth;
        if (!ctx.measureOnly)
            registerHitRect(ctx.rootId, node->id, imgX, imgY, imgWidth, imgHeight);
        ctx.currentY += std::max(ctx.lineHeight, imgHeight);
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
    case eHTMLTag::Nine:
    case eHTMLTag::Three:
    {
        ctx.flushLine(state, false);
        const bool isNine = (node->tag == eHTMLTag::Nine);
        const bool hasChild = !node->children.empty();
        const float padL = isNine ? std::max(node->padding.left,  node->sliceA) : node->padding.left;
        const float padT = isNine ? std::max(node->padding.top,    node->sliceC) : node->padding.top;
        const float padR = isNine ? std::max(node->padding.right, node->sliceB) : node->padding.right;
        const float padB = isNine ? std::max(node->padding.bottom, node->sliceD) : node->padding.bottom;
        float availW = 0.f;
        if (node->reqWidth > 0.f)
            availW = node->reqWidth - padL - padR;
        else if (ctx.params.maxWidth > 0.f)
            availW = ctx.params.maxWidth - padL - padR;
        float contentW = 0.f, contentH = 0.f;
        if (hasChild)
        {
            LayoutParams childParams;
            childParams.x = 0.f;
            childParams.y = 0.f;
            childParams.maxWidth = std::max(0.f, availW);
            childParams.align = FONS_ALIGN_LEFT;
            childParams.color = state.color;
            childParams.appearStart = ctx.blockStart;
            childParams.animKey = ctx.animKey;
            HTMLStateScope ns(state, *_scratch);
            state.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
            domRefreshFontState(state);
            Rect childRc;
            renderSequence(node->children, state, childParams, &childRc, true, ctx.rootId);
            contentW = childRc.cx;
            contentH = childRc.cy;
        }
        const float origW = node->spriteSrcW, origH = node->spriteSrcH;
        float w = 0.f, h = 0.f;
        if (isNine)
        {
            w = node->reqWidth > 0.f ? node->reqWidth : origW;
            h = node->reqHeight > 0.f ? node->reqHeight : origH;
        }
        else if (node->sliceVertical)
        {
            w = origW;
            h = node->reqHeight > 0.f ? node->reqHeight : origH;
        }
        else
        {
            w = node->reqWidth > 0.f ? node->reqWidth : origW;
            h = origH;
        }
        w = std::max(w, contentW + padL + padR);
        h = std::max(h, contentH + padT + padB);
        if (w <= 0.f)
            w = 1.f;
        if (h <= 0.f)
            h = 1.f;
        if (ctx.params.maxWidth > 0.f && w > ctx.params.maxWidth)
            w = ctx.params.maxWidth;
        float sliceMinW = 0.f, sliceMinH = 0.f;
        if (domSliceMinWH(*node, isNine, sliceMinW, sliceMinH))
        {
            w = std::max(w, sliceMinW);
            h = std::max(h, sliceMinH);
        }
        if (isNine && hasChild)
        {
            LayoutParams childParams;
            childParams.x = 0.f;
            childParams.y = 0.f;
            childParams.maxWidth = std::max(0.f, w - padL - padR);
            childParams.align = FONS_ALIGN_LEFT;
            childParams.color = state.color;
            childParams.appearStart = ctx.blockStart;
            childParams.animKey = ctx.animKey;
            HTMLStateScope ns(state, *_scratch);
            state.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
            domRefreshFontState(state);
            Rect childRc;
            renderSequence(node->children, state, childParams, &childRc, true, ctx.rootId);
            contentH = childRc.cy;
            h = std::max(h, contentH + padT + padB);
        }
        if (domSliceMinWH(*node, isNine, sliceMinW, sliceMinH))
            h = std::max(h, sliceMinH);
        float x = ctx.currentX;
        int hAlign = state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT);
        if (hAlign == FONS_ALIGN_CENTER)
        {
            if (ctx.params.maxWidth > 0.f)
                x = ctx.params.x + (ctx.params.maxWidth - w) * 0.5f;
        }
        else if (hAlign == FONS_ALIGN_RIGHT && ctx.params.maxWidth > 0.f)
        {
            x = ctx.params.x + ctx.params.maxWidth - w;
        }
        float y = ctx.currentY;
        if (node->spriteTex && !ctx.measureOnly)
        {
            if (isNine)
            {
                renderNineSliceColorAlpha(node->spriteTex,
                    node->spriteUV[0], node->spriteUV[2], node->spriteUV[1], node->spriteUV[3],
                    origW, origH, x, y, w, h,
                    node->sliceA, node->sliceB, node->sliceC, node->sliceD,
                    tint(0xFFFFFFFF), 1.f, nullptr, ctx.grayScale);
            }
            else if (node->sliceVertical)
            {
                drawThreeSliceSafeColor(node->spriteTex, node->spriteUV,
                    origW, origH,
                    x, y, w, h,
                    node->sliceC, node->sliceD,
                    true, node->sliceMirror,
                    tint(0xFFFFFFFF), nullptr, ctx.grayScale);
            }
            else
            {
                drawThreeSliceSafeColor(node->spriteTex, node->spriteUV,
                    origW, origH,
                    x, y, w, h,
                    node->sliceA, node->sliceB,
                    false, node->sliceMirror,
                    tint(0xFFFFFFFF), nullptr, ctx.grayScale);
            }
        }
        size_t hitsBeforeChildren = 0;
        if (hasChild && !ctx.measureOnly)
            hitsBeforeChildren = _hitData[ctx.rootId].size();
        if (hasChild)
        {
            LayoutParams childParams;
            childParams.x = x + padL;
            childParams.y = y + padT;
            childParams.maxWidth = std::max(0.f, w - padL - padR);
            childParams.align = (state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT)) | FONS_ALIGN_TOP;
            childParams.color = state.color;
            childParams.appearStart = ctx.blockStart;
            childParams.animKey = ctx.animKey;
            HTMLStateScope cs(state, *_scratch);
            state.align = childParams.align;
            domRefreshFontState(state);
            Rect childRc;
            renderSequence(node->children, state, childParams, &childRc, ctx.measureOnly, ctx.rootId);
        }
        ctx.updateBounds(x, y, w, h);
        if (w > ctx.maxContentWidth)
            ctx.maxContentWidth = w;
        if (!ctx.measureOnly)
        {
            if (hasChild)
                registerHitRectAt(ctx.rootId, node->id, x, y, w, h, hitsBeforeChildren);
            else
                registerHitRect(ctx.rootId, node->id, x, y, w, h);
        }
        ctx.currentY += h;
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
    case eHTMLTag::NineButton:
    {
        ctx.flushLine(state, false);
        const bool hasChild = !node->children.empty();
        const float padL = std::max(node->padding.left,   node->sliceA);
        const float padT = std::max(node->padding.top,     node->sliceC);
        const float padR = std::max(node->padding.right,  node->sliceB);
        const float padB = std::max(node->padding.bottom, node->sliceD);
        float availW = 0.f;
        if (node->reqWidth > 0.f)
            availW = node->reqWidth - padL - padR;
        else if (ctx.params.maxWidth > 0.f)
            availW = ctx.params.maxWidth - padL - padR;
        float contentW = 0.f, contentH = 0.f;
        if (hasChild)
        {
            LayoutParams childParams;
            childParams.x = 0.f;
            childParams.y = 0.f;
            childParams.maxWidth = std::max(0.f, availW);
            childParams.align = FONS_ALIGN_LEFT;
            childParams.color = state.color;
            childParams.appearStart = ctx.blockStart;
            childParams.animKey = ctx.animKey;
            HTMLStateScope ms(state, *_scratch);
            state.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
            domRefreshFontState(state);
            Rect childRc;
            renderSequence(node->children, state, childParams, &childRc, true, ctx.rootId);
            contentW = childRc.cx;
            contentH = childRc.cy;
        }
        float origW = node->spriteSrcW, origH = node->spriteSrcH;
        if (origW <= 0.f && node->hasHoverSprite)
        {
            origW = node->hoverSrcW;
            origH = node->hoverSrcH;
        }
        if (origW <= 0.f && node->hasPressedSprite)
        {
            origW = node->pressedSrcW;
            origH = node->pressedSrcH;
        }
        float w = node->reqWidth > 0.f ? node->reqWidth : origW;
        float h = node->reqHeight > 0.f ? node->reqHeight : origH;
        w = std::max(w, contentW + padL + padR);
        if (w <= 0.f)
            w = 1.f;
        if (ctx.params.maxWidth > 0.f && w > ctx.params.maxWidth)
            w = ctx.params.maxWidth;
        if (hasChild && node->reqWidth <= 0.f)
        {
            LayoutParams naturalParams;
            naturalParams.x = 0.f;
            naturalParams.y = 0.f;
            naturalParams.maxWidth = 0.f;
            naturalParams.align = FONS_ALIGN_LEFT;
            naturalParams.color = state.color;
            naturalParams.appearStart = ctx.blockStart;
            naturalParams.animKey = ctx.animKey;
            HTMLStateScope natScope(state, *_scratch);
            state.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
            domRefreshFontState(state);
            Rect naturalRc;
            renderSequence(node->children, state, naturalParams, &naturalRc, true, ctx.rootId);
            float wantW = std::max(origW, naturalRc.cx + padL + padR + 2.f);
            if (ctx.params.maxWidth > 0.f && wantW > ctx.params.maxWidth)
                wantW = ctx.params.maxWidth;
            if (wantW > w)
                w = wantW;
        }

        if (!node->btnGroup.empty())
        {
            float gw = 0.f;
            for (const auto& grp : _scratch->groups)
            {
                if (grp.name && *grp.name == node->btnGroup)
                {
                    gw = grp.w;
                    break;
                }
            }
            if (gw > w)
                w = gw;
            if (ctx.params.maxWidth > 0.f && w > ctx.params.maxWidth)
                w = ctx.params.maxWidth;
        }
        if (hasChild)
        {
            LayoutParams childParams;
            childParams.x = 0.f;
            childParams.y = 0.f;
            childParams.maxWidth = std::max(0.f, w - padL - padR);
            childParams.align = FONS_ALIGN_LEFT;
            childParams.color = state.color;
            childParams.appearStart = ctx.blockStart;
            childParams.animKey = ctx.animKey;
            HTMLStateScope fs2(state, *_scratch);
            state.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
            domRefreshFontState(state);
            Rect childRc;
            renderSequence(node->children, state, childParams, &childRc, true, ctx.rootId);
            contentW = childRc.cx;
            contentH = childRc.cy;
        }
        h = std::max(h, contentH + padT + padB);
        w = std::max(w, node->sliceA + node->sliceB);
        h = std::max(h, node->sliceC + node->sliceD);
        if (h <= 0.f)
            h = 1.f;
        float x = ctx.currentX;
        int hAlign = state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT);
        if (hAlign == FONS_ALIGN_CENTER)
        {
            if (ctx.params.maxWidth > 0.f)
                x = ctx.params.x + (ctx.params.maxWidth - w) * 0.5f;
        }
        else if (hAlign == FONS_ALIGN_RIGHT && ctx.params.maxWidth > 0.f)
        {
            x = ctx.params.x + ctx.params.maxWidth - w;
        }
        float y = ctx.currentY;
        bool hovered = !interactionBlocked && (_hoverTargetId == node->id);
        bool pressed = !interactionBlocked &&
            _mouseButtonDown &&
            _pressedRootId == ctx.rootId &&
            _pressedTargetId == node->id &&
            hovered;
        float hoverFadeDur = 0.12f, pressedFadeDur = 0.08f;
        getButtonFadeDurations(*node, hoverFadeDur, pressedFadeDur);
        float hoverAlpha = 0.f, pressedAlpha = 0.f;
        if (!ctx.measureOnly)
        {
            auto& fx = _domButtonFxMap[node->id];
            float now = _fAnimTime;
            if (fx.lastTime < 0.f)
                fx.lastTime = now;
            float dt = std::clamp(now - fx.lastTime, 0.f, 0.25f);
            fx.lastTime = now;
            const bool hoverOn = hovered && !pressed;
            fx.hover = buttonApproach(fx.hover, hoverOn ? 1.f : 0.f, hoverFadeDur, dt);
            fx.pressed = buttonApproach(fx.pressed, pressed ? 1.f : 0.f, pressedFadeDur, dt);
            hoverAlpha = fx.hover;
            pressedAlpha = fx.pressed;
        }
        else
        {
            auto itFx = _domButtonFxMap.find(node->id);
            if (itFx != _domButtonFxMap.end())
            {
                hoverAlpha = itFx->second.hover;
                pressedAlpha = itFx->second.pressed;
            }
        }
        if (!ctx.measureOnly)
        {
            auto drawButtonSkin = [&](const CTexturePtr& tex, const float* uv, float srcW, float srcH, float alpha)
            {
                if (!tex || alpha <= 0.001f)
                    return;
                if (srcW <= 0.f)
                    srcW = origW;
                if (srcH <= 0.f)
                    srcH = origH;
                renderNineSliceColorAlpha(tex, uv[0], uv[2], uv[1], uv[3], srcW, srcH, x, y, w, h,
                    node->sliceA, node->sliceB, node->sliceC, node->sliceD,
                    tint(0xFFFFFFFF), alpha, nullptr, ctx.grayScale);
            };
            drawButtonSkin(node->spriteTex, node->spriteUV, node->spriteSrcW, node->spriteSrcH, 1.f);
            if (hoverAlpha > 0.001f && node->hasHoverSprite)
                drawButtonSkin(node->hoverTex, node->hoverUV, node->hoverSrcW, node->hoverSrcH, hoverAlpha);
            if (pressedAlpha > 0.001f && node->hasPressedSprite)
                drawButtonSkin(node->pressedTex, node->pressedUV, node->pressedSrcW, node->pressedSrcH, pressedAlpha);
        }
        if (hasChild)
        {
            const float contentAreaW = std::max(0.f, w - padL - padR);
            const float contentAreaH = std::max(0.f, h - padT - padB);
            float contentOffY = 0.f;
            if (node->contentAlignV == eVAlign::MIDDLE)
                contentOffY = std::max(0.f, (contentAreaH - contentH) * 0.5f);
            else if (node->contentAlignV == eVAlign::BOTTOM)
                contentOffY = std::max(0.f, contentAreaH - contentH);
            LayoutParams childParams;
            childParams.x = x + padL;
            childParams.y = y + padT + contentOffY;
            childParams.maxWidth = contentAreaW;
            childParams.align = node->contentAlignH | FONS_ALIGN_TOP;
            childParams.color = state.color;
            childParams.appearStart = ctx.blockStart;
            childParams.animKey = ctx.animKey;
            HTMLStateScope cs3(state, *_scratch);
            state.align = childParams.align;
            domRefreshFontState(state);
            Rect childRc;
            renderSequence(node->children, state, childParams, &childRc, ctx.measureOnly, ctx.rootId);
        }
        ctx.updateBounds(x, y, w, h);
        if (w > ctx.maxContentWidth)
            ctx.maxContentWidth = w;
        if (!ctx.measureOnly)
            registerHitRect(ctx.rootId, node->id, x, y, w, h);
        ctx.currentY += h;
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
    case eHTMLTag::Ul:
    {
        ctx.flushLine(state, false);
        float startY = ctx.currentY;
        size_t hitsBeforeChildren = 0;
        if (!ctx.measureOnly)
            hitsBeforeChildren = _hitData[ctx.rootId].size();
        float oldX = ctx.params.x;
        float indent = node->ulIndentCache;
        ctx.params.x += indent;
        ctx.currentX = ctx.params.x;
        renderChildren(node, state, ctx);
        ctx.flushLine(state, false);
        float endY = ctx.currentY;
        ctx.params.x = oldX;
        ctx.currentX = ctx.params.x;
        float blockW = ctx.params.maxWidth;
        if (!ctx.measureOnly && blockW > 0.f)
            registerHitRectAt(ctx.rootId, node->id, oldX, startY, blockW, endY - startY, hitsBeforeChildren);
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
    case eHTMLTag::Li:
    {
        ctx.flushLine(state, false);
        float startY = ctx.currentY;
        size_t hitsBeforeChildren = 0;
        if (!ctx.measureOnly)
            hitsBeforeChildren = _hitData[ctx.rootId].size();
        const std::string& bullet = node->liBulletCache;
        if (!bullet.empty())
        {
            const int bulletLen = (int)bullet.size();
            float bulletW = domMeasureStyledTextRange(bullet.c_str(), bulletLen, state);
            ctx.pushChunk(bullet.c_str(), bulletLen, bulletW, state);
        }
        renderChildren(node, state, ctx);
        ctx.flushLine(state, false);
        float endY = ctx.currentY;
        float blockW = (ctx.params.maxWidth > 0.f) ? ctx.params.maxWidth : ctx.maxContentWidth;
        if (!ctx.measureOnly && blockW > 0.f)
            registerHitRectAt(ctx.rootId, node->id, ctx.params.x, startY, blockW, endY - startY, hitsBeforeChildren);
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
    case eHTMLTag::Checkbox:
    {
        ctx.flushLine(state, false);
        const bool hasChild = !node->children.empty();
        const float padL = node->padding.left, padT = node->padding.top, padR = node->padding.right, padB = node->padding.bottom;
        float spacing = node->checkboxSpacingCache;
        float iconW = node->reqWidth, iconH = node->reqHeight;
        if (iconW <= 0.f || iconH <= 0.f)
        {
            float srcW = 0.f, srcH = 0.f;
            if (node->hasCheckboxUncheckedSprite)
            {
                srcW = node->checkboxUncheckedSrcW;
                srcH = node->checkboxUncheckedSrcH;
            }
            else if (node->hasCheckboxCheckedSprite)
            {
                srcW = node->checkboxCheckedSrcW;
                srcH = node->checkboxCheckedSrcH;
            }
            if (iconW <= 0.f)
                iconW = srcW;
            if (iconH <= 0.f)
                iconH = srcH;
        }
        if (iconW <= 0.f || iconH <= 0.f)
        {
            iconW = 16.f;
            iconH = 16.f;
        }
        float textGap = hasChild ? spacing : 0.f;
        float availW = 0.f;
        if (ctx.params.maxWidth > 0.f)
            availW = ctx.params.maxWidth - padL - padR - iconW - textGap;
        float contentW = 0.f, contentH = 0.f;
        if (hasChild)
        {
            LayoutParams childParams;
            childParams.x = 0.f;
            childParams.y = 0.f;
            childParams.maxWidth = (ctx.params.maxWidth > 0.f) ? std::max(1.f, availW) : 0.f;
            childParams.align = FONS_ALIGN_LEFT;
            childParams.color = state.color;
            childParams.appearStart = ctx.blockStart;
            childParams.animKey = ctx.animKey;
            HTMLStateScope mScope(state, *_scratch);
            state.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
            domRefreshFontState(state);
            Rect childRc;
            renderSequence(node->children, state, childParams, &childRc, true, ctx.rootId);
            contentW = childRc.cx;
            contentH = childRc.cy;
        }
        float w = iconW + textGap + contentW + padL + padR;
        float h = std::max(iconH, contentH) + padT + padB;
        if (w <= 0.f)
            w = 1.f;
        if (h <= 0.f)
            h = 1.f;
        if (ctx.params.maxWidth > 0.f && w > ctx.params.maxWidth)
            w = ctx.params.maxWidth;
        float x = ctx.currentX;
        int hAlign = state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT);
        if (hAlign == FONS_ALIGN_CENTER)
        {
            if (ctx.params.maxWidth > 0.f)
                x = ctx.params.x + (ctx.params.maxWidth - w) * 0.5f;
        }
        else if (hAlign == FONS_ALIGN_RIGHT && ctx.params.maxWidth > 0.f)
        {
            x = ctx.params.x + ctx.params.maxWidth - w;
        }
        float y = ctx.currentY;
        float hoverFadeDur = 0.12f, pressedFadeDur = 0.08f;
        getButtonFadeDurations(*node, hoverFadeDur, pressedFadeDur);
        float checkFadeDur = node->checkboxCheckFadeDurCache;
        float checkedAlpha = 0.f;
        if (!ctx.measureOnly)
        {
            auto& fx = _domButtonFxMap[node->id];
            float now = _fAnimTime;
            if (fx.lastTime < 0.f)
                fx.lastTime = now;
            float dt = std::clamp(now - fx.lastTime, 0.f, 0.25f);
            fx.lastTime = now;
            fx.checked = buttonApproach(fx.checked, node->checked ? 1.f : 0.f, checkFadeDur, dt);
            checkedAlpha = fx.checked;
        }
        else
        {
            auto itFx = _domButtonFxMap.find(node->id);
            if (itFx != _domButtonFxMap.end())
                checkedAlpha = itFx->second.checked;
        }
        if (!ctx.measureOnly)
        {
            float contentAreaH = h - padT - padB;
            float iconX = x + padL;
            float iconY = y + padT + (contentAreaH - iconH) * 0.5f;
            unsigned int iconColor = tint(state.color);
            if (node->hasCheckboxUncheckedSprite)
            {
                drawSpriteQuadColorAlpha(node->checkboxUncheckedTex, iconX, iconY, iconW, iconH,
                    node->checkboxUncheckedUV[0], node->checkboxUncheckedUV[1],
                    node->checkboxUncheckedUV[2], node->checkboxUncheckedUV[3], iconColor, 1.f, ctx.grayScale);
            }
            if (node->hasCheckboxCheckedSprite && checkedAlpha > 0.001f)
            {
                drawSpriteQuadColorAlpha(node->checkboxCheckedTex, iconX, iconY, iconW, iconH,
                    node->checkboxCheckedUV[0], node->checkboxCheckedUV[1],
                    node->checkboxCheckedUV[2], node->checkboxCheckedUV[3], iconColor, checkedAlpha, ctx.grayScale);
            }
        }
        if (hasChild)
        {
            LayoutParams childParams;
            childParams.x = x + padL + iconW + textGap;
            childParams.y = y + padT + std::max(0.f, (h - padT - padB - contentH) * 0.5f);
            childParams.maxWidth = std::max(1.f, w - padL - padR - iconW - textGap);
            childParams.align = (state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT)) | FONS_ALIGN_TOP;
            childParams.color = state.color;
            childParams.appearStart = ctx.blockStart;
            childParams.animKey = ctx.animKey;
            HTMLStateScope rScope(state, *_scratch);
            state.align = childParams.align;
            domRefreshFontState(state);
            Rect childRc;
            renderSequence(node->children, state, childParams, &childRc, ctx.measureOnly, ctx.rootId);
        }
        ctx.updateBounds(x, y, w, h);
        if (w > ctx.maxContentWidth)
            ctx.maxContentWidth = w;
        if (!ctx.measureOnly)
            registerHitRect(ctx.rootId, node->id, x, y, w, h);
        ctx.currentY += h;
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
    case eHTMLTag::Select:
    {
        ctx.flushLine(state, false);
        HTMLRenderScratch& scSel = *_scratch;
        scSel.options.clear();
        collectSelectOptionsRaw(node.get(), scSel.options);
        node->selectedIndex = computeSelectIndex(node.get(), scSel.options);
        const float padL = std::max(node->padding.left,   node->sliceA);
        const float padT = std::max(node->padding.top,     node->sliceC);
        const float padR = std::max(node->padding.right,  node->sliceB);
        const float padB = std::max(node->padding.bottom, node->sliceD);
        const float capL = node->spriteTex ? std::max(0.f, node->sliceA) : 0.f;
        const float capR = node->spriteTex ? std::max(0.f, node->sliceB) : 0.f;
        float maxLabelW = 0.f;
        float lineH = 0.f;
        {
            HTMLStateScope uiScope(state, scSel);
            applyFontAttrsToState(state, node->fontAttrs);
            state.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
            domRefreshFontState(state);
            lineH = TextRender::getInstance()->getLineHeight();
            for (HTMLDomNode* opt : scSel.options)
            {
                HTMLStateScope os(state, scSel);
                resolveOptionFontAttrsInto(opt, node->id, scSel.optionFontAttrs);
                applyFontAttrsToState(state, scSel.optionFontAttrs);
                domRefreshFontState(state);
                scSel.textBuf.clear();
                getSelectOptionLabelInto(opt, scSel.textBuf);
                maxLabelW = std::max(maxLabelW, domMeasureStyledText(scSel.textBuf, state));
            }
        }
        float itemH = node->selectItemHeight;
        if (itemH <= 0.f)
            itemH = lineH + node->selectItemPadY * 2.f;
        if (itemH <= 0.f)
            itemH = 1.f;
        float iconW = 0.f, iconH = 0.f;
        if (node->selectIconTex)
        {
            iconW = node->selectIconW;
            iconH = node->selectIconH;
        }
        float autoW = capL + capR + maxLabelW + 4.f;
        if (iconW > 0.f)
            autoW += node->selectIconGap + iconW;
        float w = node->reqWidth > 0.f ? node->reqWidth : autoW;
        if (ctx.params.maxWidth > 0.f && w > ctx.params.maxWidth)
            w = ctx.params.maxWidth;
        w = std::max(w, capL + capR);
        if (w <= 0.f)
            w = 1.f;
        float h = node->reqHeight > 0.f ? node->reqHeight : padT + padB + lineH;
        if (node->spriteTex)
        {
            w = std::max(w, node->sliceA + node->sliceB);
            h = std::max(h, node->sliceC + node->sliceD);
        }
        if (node->selectBoxTex)
        {

            if (node->selectBoxVertical)
                h = std::max(h, node->selectBoxA + node->selectBoxB);
            else
                w = std::max(w, node->selectBoxA + node->selectBoxB);
        }
        if (h <= 0.f)
            h = 1.f;
        float x = ctx.currentX;
        int hAlign = state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT);
        if (hAlign == FONS_ALIGN_CENTER)
        {
            if (ctx.params.maxWidth > 0.f)
                x = ctx.params.x + (ctx.params.maxWidth - w) * 0.5f;
        }
        else if (hAlign == FONS_ALIGN_RIGHT && ctx.params.maxWidth > 0.f)
        {
            x = ctx.params.x + ctx.params.maxWidth - w;
        }
        float y = ctx.currentY;
        node->selectLocalRect.set(x, y, w, h);
        node->selectLocalItemHeight = itemH;
        if (!ctx.measureOnly)
        {
            if (node->selectBoxTex)
            {
                drawThreeSliceSafeColor(node->selectBoxTex, node->selectBoxUV,
                    node->selectBoxSrcW, node->selectBoxSrcH,
                    x, y, w, h,
                    node->selectBoxA, node->selectBoxB,
                    node->selectBoxVertical, node->selectBoxMirror,
                    tint(0xFFFFFFFF), nullptr, ctx.grayScale);
            }
            else if (node->spriteTex)
            {
                renderNineSliceColorAlpha(node->spriteTex,
                    node->spriteUV[0], node->spriteUV[2], node->spriteUV[1], node->spriteUV[3],
                    node->spriteSrcW, node->spriteSrcH,
                    x, y, w, h,
                    node->sliceA, node->sliceB, node->sliceC, node->sliceD,
                    tint(0xFFFFFFFF), 1.f, nullptr, ctx.grayScale);
            }
            if (node->hasHoverSprite || node->hasPressedSprite)
            {
                bool hovered = !interactionBlocked && (_hoverTargetId == node->id);
                bool pressed = !interactionBlocked &&
                    _mouseButtonDown &&
                    _pressedRootId == ctx.rootId &&
                    _pressedTargetId == node->id &&
                    hovered;
                float hoverFadeDur = 0.12f;
                float pressedFadeDur = 0.08f;
                getButtonFadeDurations(*node, hoverFadeDur, pressedFadeDur);
                auto& fx = _domButtonFxMap[node->id];
                float now = _fAnimTime;
                if (fx.lastTime < 0.f)
                    fx.lastTime = now;
                float dt = std::clamp(now - fx.lastTime, 0.f, 0.25f);
                fx.lastTime = now;
                const bool hoverOn = hovered && !pressed;
                fx.hover = buttonApproach(fx.hover, hoverOn ? 1.f : 0.f, hoverFadeDur, dt);
                fx.pressed = buttonApproach(fx.pressed, pressed ? 1.f : 0.f, pressedFadeDur, dt);
                auto drawSelectSkin = [&](const CTexturePtr& tex, const float* uv, float srcW, float srcH, float alpha)
                {
                    if (!tex || alpha <= 0.001f)
                        return;
                    renderNineSliceColorAlpha(tex, uv[0], uv[2], uv[1], uv[3], srcW, srcH, x, y, w, h,
                        node->sliceA, node->sliceB, node->sliceC, node->sliceD,
                        tint(0xFFFFFFFF), alpha, nullptr, ctx.grayScale);
                };
                if (fx.hover > 0.001f && node->hasHoverSprite)
                    drawSelectSkin(node->hoverTex, node->hoverUV, node->hoverSrcW, node->hoverSrcH, fx.hover);
                if (fx.pressed > 0.001f && node->hasPressedSprite)
                    drawSelectSkin(node->pressedTex, node->pressedUV, node->pressedSrcW, node->pressedSrcH, fx.pressed);
            }

            if (node->selectedIndex >= 0 && node->selectedIndex < (int)scSel.options.size())
            {
                HTMLStateScope ls(state, scSel);
                resolveOptionFontAttrsInto(scSel.options[node->selectedIndex], node->id, scSel.optionFontAttrs);
                applyFontAttrsToState(state, scSel.optionFontAttrs);
                domRefreshFontState(state);
                state.color = tint(state.color);
                scSel.textBuf.clear();
                getSelectOptionLabelInto(scSel.options[node->selectedIndex], scSel.textBuf);
                float iconSpace = (iconW > 0.f) ? iconW + node->selectIconGap : 0.f;
                float innerW = std::max(0.f, w - capL - capR - iconSpace);
                fitSelectLabel(scSel.textBuf, state, innerW - 2.f, scSel.textBuf2);
                float labW = domMeasureStyledText(scSel.textBuf2, state);
                float tx = x + capL + std::max(0.f, (innerW - labW) * 0.5f);
                float ty = y + std::max(0.f, (h - lineH) * 0.5f);
                if (ctx.grayScale)
                    state.color = domGrayColor(state.color);
                drawTextRange(domFs(), scSel.textBuf2.c_str(), (int)scSel.textBuf2.size(), tx, ty, state);
            }
            if (node->selectIconTex && iconW > 0.f && iconH > 0.f)
            {
                float ix = x + w - capR - iconW;
                float iy = y + std::max(0.f, (h - iconH) * 0.5f);
                drawSpriteQuadColorAlpha(node->selectIconTex, ix, iy, iconW, iconH,
                    node->selectIconUV[0], node->selectIconUV[1],
                    node->selectIconUV[2], node->selectIconUV[3],
                    tint(0xFFFFFFFF), 1.f, ctx.grayScale);
            }
            registerHitRect(ctx.rootId, node->id, x, y, w, h);
        }
        ctx.updateBounds(x, y, w, h);
        if (w > ctx.maxContentWidth)
            ctx.maxContentWidth = w;
        ctx.currentY += h;
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
    case eHTMLTag::Option:
        break;
    case eHTMLTag::Edit:
    {
        ctx.flushLine(state, false);
        domEnsureEditInited(node);
        HTMLRenderScratch& scEd = *_scratch;
        const float padL = std::max(node->padding.left,   node->sliceA);
        const float padT = std::max(node->padding.top,     node->sliceC);
        const float padR = std::max(node->padding.right,  node->sliceB);
        const float padB = std::max(node->padding.bottom, node->sliceD);

        const int hAlign = state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT);
        const unsigned int baseTextColor = tint(state.color);
        HTMLStateScope escope(state, scEd);
        state.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
        applyFontAttrsToState(state, node->fontAttrs);
        domRefreshFontState(state);
        float lineH = TextRender::getInstance()->getLineHeight();
        float w = node->reqWidth > 0.f ? node->reqWidth : 200.f;
        float h = node->reqHeight > 0.f ? node->reqHeight : lineH + padT + padB;
        if (node->spriteTex)
        {
            w = std::max(w, node->sliceA + node->sliceB);
            h = std::max(h, node->sliceC + node->sliceD);
        }
        if (ctx.params.maxWidth > 0.f && w > ctx.params.maxWidth)
            w = ctx.params.maxWidth;
        if (w <= 0.f)
            w = 1.f;
        if (h <= 0.f)
            h = 1.f;
        float x = ctx.currentX;

        if (hAlign == FONS_ALIGN_CENTER)
        {
            if (ctx.params.maxWidth > 0.f)
                x = ctx.params.x + (ctx.params.maxWidth - w) * 0.5f;
        }
        else if (hAlign == FONS_ALIGN_RIGHT && ctx.params.maxWidth > 0.f)
        {
            x = ctx.params.x + ctx.params.maxWidth - w;
        }
        float y = ctx.currentY;
        node->editLocalRect.set(x, y, w, h);
        const bool bFocused = (_focusedEditId == node->id);
        const float innerW = std::max(0.f, w - padL - padR);
        if (bFocused)
        {
            const float caretX = domMeasureEditPrefix(*node, state, node->editCursor);
            const float rightLimit = std::max(4.f, innerW - 4.f);
            const float leftMargin = std::min(12.f, innerW * 0.3f);
            if (caretX - node->editScrollX > rightLimit)
                node->editScrollX = caretX - rightLimit;
            if (caretX < node->editScrollX + leftMargin)
                node->editScrollX = std::max(0.f, caretX - leftMargin);
            if (node->editScrollX < 0.f)
                node->editScrollX = 0.f;
        }
        else
        {
            node->editScrollX = 0.f;
        }
        if (!ctx.measureOnly)
        {
            if (node->spriteTex)
            {
                renderNineSliceColorAlpha(node->spriteTex,
                    node->spriteUV[0], node->spriteUV[2], node->spriteUV[1], node->spriteUV[3],
                    node->spriteSrcW, node->spriteSrcH, x, y, w, h,
                    node->sliceA, node->sliceB, node->sliceC, node->sliceD,
                    tint(0xFFFFFFFF), 1.f, nullptr, ctx.grayScale);
            }
            else
            {
                ctx.renderLine(x, y, w, h, tint(0x00000080));
                ctx.renderLine(x, y, w, 2.f, tint(bFocused ? 0x4DA6FFFF : 0x808080FF));
            }
            if (bFocused)
            {
                int selStart = 0;
                int selEnd = 0;
                domGetEditSelectionRange(*node, selStart, selEnd);
                if (selStart < selEnd)
                {
                    float sx1 = domMeasureEditPrefix(*node, state, selStart);
                    float sx2 = domMeasureEditPrefix(*node, state, selEnd);
                    float x1 = x + padL + sx1 - node->editScrollX;
                    float x2 = x + padL + sx2 - node->editScrollX;
                    float left = std::max(x1, x + padL);
                    float right = std::min(x2, x + padL + innerW);
                    if (right > left)
                    {
                        ctx.renderLine(left, y + padT, right - left, std::max(1.f, h - padT - padB), tint(0x4DA6FF66));
                    }
                }
            }
            const float textY = y + padT + std::max(0.f, (h - padT - padB - lineH) * 0.5f);
            const std::string& v = node->editValue;
            const bool bPass = node->editPassword;
            if (v.empty() && !bFocused && !node->editPlaceholder.empty())
            {
                unsigned int pc = tint(node->editPlaceholderColor);
                if (ctx.grayScale)
                    pc = domGrayColor(pc);
                state.color = pc;
                drawTextRange(domFs(), node->editPlaceholder.c_str(), (int)node->editPlaceholder.size(), x + padL, textY, state);
            }
            else if (!v.empty())
            {
                unsigned int tc = baseTextColor;
                if (ctx.grayScale)
                    tc = domGrayColor(tc);
                state.color = tc;
                float starW = 0.f;
                if (bPass)
                    starW = domMeasureStyledTextRange("*", 1, state);
                float acc = 0.f;
                int sb = 0;
                while (sb < (int)v.size())
                {
                    int cl = getUtf8CharLen(v.c_str() + sb);
                    float cw = bPass ? starW : domMeasureStyledTextRange(v.c_str() + sb, cl, state);
                    if (acc + cw > node->editScrollX)
                        break;
                    acc += cw;
                    sb += cl;
                }
                int vi = sb;
                float visW = 0.f;
                const float leftOff = acc - node->editScrollX;
                while (vi < (int)v.size())
                {
                    int cl = getUtf8CharLen(v.c_str() + vi);
                    float cw = bPass ? starW : domMeasureStyledTextRange(v.c_str() + vi, cl, state);
                    if (leftOff + visW + cw > innerW + 1.f)
                        break;
                    visW += cw;
                    vi += cl;
                }
                if (vi > sb)
                {
                    if (bPass)
                    {
                        int nStars = 0;
                        for (int k = sb; k < vi; )
                        {
                            k += getUtf8CharLen(v.c_str() + k);
                            ++nStars;
                        }
                        scEd.textBuf2.assign((size_t)nStars, '*');
                        drawTextRange(domFs(), scEd.textBuf2.c_str(), (int)scEd.textBuf2.size(), x + padL + leftOff, textY, state);
                    }
                    else
                    {
                        drawTextRange(domFs(), v.c_str() + sb, vi - sb, x + padL + leftOff, textY, state);
                    }
                }
            }
            if (bFocused)
            {
                const float blink = std::fmod(_fAnimTime * 1.75f, 1.6f);
                if (blink < 1.1f)
                {
                    const float caretX = x + padL +
                        domMeasureEditPrefix(*node, state, node->editCursor) - node->editScrollX;
                    ctx.renderLine(caretX, textY + 2.f, 2.f, std::max(1.f, lineH - 4.f), baseTextColor);
                }
            }
            registerHitRect(ctx.rootId, node->id, x, y, w, h);
        }
        ctx.updateBounds(x, y, w, h);
        if (w > ctx.maxContentWidth)
            ctx.maxContentWidth = w;
        ctx.currentY += h;
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
    case eHTMLTag::Slider:
    {
        ctx.flushLine(state, false);
        float gripW = std::max(1.f, node->sliderGripW);
        float gripH = std::max(1.f, node->sliderGripH);
        float trackThickness = std::max(1.f, node->sliderTrackThickness);
        float tickThick = node->sliderTickThickness > 0.f ? node->sliderTickThickness : std::max(1.f, trackThickness * 0.15f);
        float tickMajor = node->sliderTickLength > 0.f ? node->sliderTickLength : trackThickness * 1.0f;
        float tickMinor = node->sliderMinorTickLength > 0.f ? node->sliderMinorTickLength : tickMajor * 0.5f;
        float tickGap = std::max(1.f, trackThickness * 0.25f);
        float w = 0.f, h = 0.f;
        if (!node->sliderVertical)
        {
            float autoH = std::max(trackThickness, gripH);
            if (node->sliderShowTicks)
                autoH = std::max(autoH, trackThickness + tickGap + tickMajor + 2.f);
            w = node->reqWidth > 0.f ? node->reqWidth : ((ctx.params.maxWidth > 0.f) ? ctx.params.maxWidth : 200.f);
            h = std::max(autoH, node->reqHeight);
        }
        else
        {
            float autoW = std::max(trackThickness, gripW);
            if (node->sliderShowTicks)
                autoW = std::max(autoW, trackThickness + tickGap + tickMajor + 2.f);
            h = node->reqHeight > 0.f ? node->reqHeight : 200.f;
            w = std::max(autoW, node->reqWidth);
        }
        if (w <= 0.f)
            w = 1.f;
        if (h <= 0.f)
            h = 1.f;
        if (ctx.params.maxWidth > 0.f && w > ctx.params.maxWidth)
            w = ctx.params.maxWidth;
        float x = ctx.currentX;
        int hAlign = state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT);
        if (hAlign == FONS_ALIGN_CENTER)
        {
            if (ctx.params.maxWidth > 0.f)
                x = ctx.params.x + (ctx.params.maxWidth - w) * 0.5f;
        }
        else if (hAlign == FONS_ALIGN_RIGHT && ctx.params.maxWidth > 0.f)
        {
            x = ctx.params.x + ctx.params.maxWidth - w;
        }
        if (node->spriteTex)
        {

            if (!node->sliderVertical)
                w = std::max(w, gripW + node->sliceA + node->sliceB);
            else
                h = std::max(h, gripH + node->sliceC + node->sliceD);
        }
        float y = ctx.currentY;
        node->sliderLocalRect.set(x, y, w, h);
        if (!ctx.measureOnly)
        {
            bool hovered = !interactionBlocked && (_hoverTargetId == node->id);
            bool pressed = !interactionBlocked &&
                _mouseButtonDown &&
                ((_pressedTargetId == node->id && hovered) || _dragSliderId == node->id);
            float hoverFadeDur = 0.12f, pressedFadeDur = 0.08f;
            getButtonFadeDurations(*node, hoverFadeDur, pressedFadeDur);
            auto& fx = _domButtonFxMap[node->id];
            float now = _fAnimTime;
            if (fx.lastTime < 0.f)
                fx.lastTime = now;
            float dt = std::clamp(now - fx.lastTime, 0.f, 0.25f);
            fx.lastTime = now;
            fx.hover = buttonApproach(fx.hover, hovered ? 1.f : 0.f, hoverFadeDur, dt);
            fx.pressed = buttonApproach(fx.pressed, pressed ? 1.f : 0.f, pressedFadeDur, dt);
            float t = sliderNorm(*node);
            float i0 = 0.f, i1 = 0.f;
            sliderTrackInsets(*node, i0, i1);
            float cMin = 0.f, cMax = 0.f;
            sliderGripTravel(*node, cMin, cMax);
            float trackX = x, trackY = y, trackW = w, trackH = trackThickness;
            if (!node->sliderVertical)
            {
                trackX = x + gripW * 0.5f;
                trackW = w - gripW;
                if (trackW <= 0.f)
                {
                    trackX = x;
                    trackW = w;
                }
                trackY = y + (h - trackH) * 0.5f;
            }
            else
            {
                trackY = y + gripH * 0.5f;
                trackH = h - gripH;
                if (trackH <= 0.f)
                {
                    trackY = y;
                    trackH = h;
                }
                trackW = trackThickness;
                trackX = x + (w - trackW) * 0.5f;
            }
            if (node->spriteTex)
            {
                drawThreeSliceSafeColor(node->spriteTex, node->spriteUV, node->spriteSrcW, node->spriteSrcH,
                    trackX, trackY, trackW, trackH, node->sliceA, node->sliceB, node->sliceVertical, node->sliceMirror,
                    tint(0xFFFFFFFF), nullptr, ctx.grayScale);
            }
            else
            {
                ctx.renderLine(trackX, trackY, trackW, trackH, tint(node->sliderTrackColor));
            }
            float c = cMin + t * (cMax - cMin);
            if (node->sliderFill)
            {
                Rect clipRc;
                bool hasClip = false;
                if (t > 0.f)
                {
                    if (!node->sliderVertical)
                    {
                        float front = c + gripW * 0.5f;
                        if (t >= 1.f)
                            front = trackX + trackW;
                        else
                            front = std::max(front, trackX + i0);
                        float fillW = std::clamp(front - trackX, 0.f, trackW);
                        if (fillW > 0.f)
                        {
                            clipRc.set(trackX, trackY, fillW, trackH);
                            hasClip = true;
                        }
                    }
                    else
                    {
                        float frontTop = c - gripH * 0.5f;
                        if (t >= 1.f)
                            frontTop = trackY;
                        else
                            frontTop = std::min(frontTop, trackY + trackH - i1);
                        float fillH = std::clamp((trackY + trackH) - frontTop, 0.f, trackH);
                        if (fillH > 0.f)
                        {
                            clipRc.set(trackX, frontTop, trackW, fillH);
                            hasClip = true;
                        }
                    }
                }
                if (hasClip)
                {
                    if (node->sliderFillTex)
                    {
                        if (node->sliderFillThree)
                        {
                            drawThreeSliceSafeColor(node->sliderFillTex, node->sliderFillUV,
                                node->sliderFillSrcW, node->sliderFillSrcH,
                                trackX, trackY, trackW, trackH,
                                node->sliderFillA, node->sliderFillB,
                                node->sliderFillVertical, node->sliderFillMirror,
                                tint(0xFFFFFFFF), &clipRc, ctx.grayScale);
                        }
                        else
                        {
                            renderNineSliceColorAlpha(node->sliderFillTex,
                                node->sliderFillUV[0], node->sliderFillUV[2],
                                node->sliderFillUV[1], node->sliderFillUV[3],
                                node->sliderFillSrcW, node->sliderFillSrcH,
                                trackX, trackY, trackW, trackH,
                                node->sliderFillA, node->sliderFillB,
                                node->sliderFillC, node->sliderFillD,
                                tint(0xFFFFFFFF), 1.f, &clipRc, ctx.grayScale);
                        }
                    }
                    else
                    {
                        ctx.renderLine(clipRc.x, clipRc.y, clipRc.cx, clipRc.cy, tint(node->sliderFillColor));
                    }
                }
            }
            if (node->sliderShowTicks)
            {
                float len = node->sliderVertical ? trackH : trackW;
                int major = node->sliderMajorCount;
                int minor = node->sliderMinorCount;
                if (node->sliderTicksAuto || major <= 0)
                {
                    int autoMajor = (int)(len / 80.f);
                    major = std::clamp(autoMajor, 2, 20);
                    int autoMinor = (int)((len / (float)std::max(1, major)) / 20.f);
                    minor = std::clamp(autoMinor, 1, 8);
                }
                if (major < 1)
                    major = 1;
                if (minor < 0)
                    minor = 0;
                auto drawTick = [&](float tt, bool isMajor)
                {
                    float thick = tickThick;
                    float tickLen = isMajor ? tickMajor : tickMinor;
                    unsigned int col = isMajor ? tint(node->sliderTickColor) : tint(node->sliderMinorTickColor);
                    float pos = cMin + tt * (cMax - cMin);
                    if (!node->sliderVertical)
                    {
                        float ty = trackY + trackH + tickGap + node->sliderTickOffset;
                        ctx.renderLine(pos - thick * 0.5f, ty, thick, tickLen, col);
                    }
                    else
                    {
                        float tx = trackX + trackW + tickGap + node->sliderTickOffset;
                        ctx.renderLine(tx, pos - thick * 0.5f, tickLen, thick, col);
                    }
                };
                for (int i = 0; i <= major; ++i)
                {
                    float tt = (major > 0) ? ((float)i / (float)major) : 0.f;
                    drawTick(tt, true);
                    if (i < major && minor > 0)
                    {
                        for (int j = 1; j <= minor; ++j)
                        {
                            float mt = ((float)i + ((float)j / ((float)minor + 1.f))) / (float)major;
                            drawTick(mt, false);
                        }
                    }
                }
            }
            float gx = x, gy = y;
            if (!node->sliderVertical)
            {
                gx = c - gripW * 0.5f;
                gy = y + (h - gripH) * 0.5f;
            }
            else
            {
                gy = c - gripH * 0.5f;
                gx = x + (w - gripW) * 0.5f;
            }
            unsigned int gripColor = tint(node->sliderGripColor);
            if (node->hasHoverColor && fx.hover > 0.001f)
                gripColor = lerpColor(gripColor, tint(node->hoverColor), fx.hover);
            if (node->hasPressedColor && fx.pressed > 0.001f)
                gripColor = lerpColor(gripColor, tint(node->pressedColor), fx.pressed);
            if (node->sliderGripTex)
            {
                drawSpriteQuadColorAlpha(node->sliderGripTex, gx, gy, gripW, gripH,
                    node->sliderGripUV[0], node->sliderGripUV[1], node->sliderGripUV[2], node->sliderGripUV[3],
                    gripColor, 1.f, ctx.grayScale);
            }
            else
            {
                ctx.renderLine(gx, gy, gripW, gripH, gripColor);
            }
            registerHitRect(ctx.rootId, node->id, x, y, w, h);
        }
        ctx.updateBounds(x, y, w, h);
        if (w > ctx.maxContentWidth)
            ctx.maxContentWidth = w;
        ctx.currentY += h;
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
    case eHTMLTag::ProgressBar:
    {
        ctx.flushLine(state, false);
        float w = 0.f, h = 0.f;
        if (!node->progressVertical)
        {
            w = node->reqWidth > 0.f ? node->reqWidth : ((ctx.params.maxWidth > 0.f) ? ctx.params.maxWidth : 200.f);
            if (node->reqHeight > 0.f)
                h = node->reqHeight;
            else if (node->progressFrameTex && node->progressFrameSrcH > 0.f)
                h = node->progressFrameSrcH;
            else
                h = 20.f;
        }
        else
        {
            h = node->reqHeight > 0.f ? node->reqHeight : 200.f;
            if (node->reqWidth > 0.f)
                w = node->reqWidth;
            else if (node->progressFrameTex && node->progressFrameSrcW > 0.f)
                w = node->progressFrameSrcW;
            else
                w = 20.f;
        }
        if (ctx.params.maxWidth > 0.f && w > ctx.params.maxWidth)
            w = ctx.params.maxWidth;
        if (w <= 0.f)
            w = 1.f;
        if (h <= 0.f)
            h = 1.f;
        float x = ctx.currentX;
        int hAlign = state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT);
        if (hAlign == FONS_ALIGN_CENTER)
        {
            if (ctx.params.maxWidth > 0.f)
                x = ctx.params.x + (ctx.params.maxWidth - w) * 0.5f;
        }
        else if (hAlign == FONS_ALIGN_RIGHT && ctx.params.maxWidth > 0.f)
        {
            x = ctx.params.x + ctx.params.maxWidth - w;
        }
        if (node->progressFrameTex)
        {
            if (node->progressFrameThree)
            {

                if (!node->progressFrameVertical)
                    w = std::max(w, node->progressFrameA + node->progressFrameB);
                else
                    h = std::max(h, node->progressFrameA + node->progressFrameB);
            }
            else
            {
                w = std::max(w, node->progressFrameA + node->progressFrameB);
                h = std::max(h, node->progressFrameC + node->progressFrameD);
            }
        }
        float y = ctx.currentY;
        node->progressLocalRect.set(x, y, w, h);
        if (!ctx.measureOnly)
        {
            float t = clampProgress01(node->progressT);
            auto drawWhiteBoxRect = [&](float rx, float ry, float rw, float rh, unsigned int color)
            {
                if (rw <= 0.f || rh <= 0.f)
                    return;
                float alpha = (float)(color & 0xFF) / 255.f;
                if (alpha <= 0.f)
                    return;
                ctx.renderLine(rx, ry, rw, rh, color);
            };
            auto drawWhiteBoxBorder = [&](float rx, float ry, float rw, float rh, float thickness, unsigned int color)
            {
                if (rw <= 0.f || rh <= 0.f || thickness <= 0.f)
                    return;
                float alpha = (float)(color & 0xFF) / 255.f;
                if (alpha <= 0.f)
                    return;
                thickness = std::min(thickness, std::min(rw * 0.5f, rh * 0.5f));
                if (thickness <= 0.f)
                    return;
                ctx.renderLine(rx, ry, rw, thickness, color);
                ctx.renderLine(rx, ry + rh - thickness, rw, thickness, color);
                ctx.renderLine(rx, ry + thickness, thickness, rh - thickness * 2.f, color);
                ctx.renderLine(rx + rw - thickness, ry + thickness, thickness, rh - thickness * 2.f, color);
            };
            const bool hasFrameSkin = node->progressFrameTex ? true : false;
            if (node->progressHasBackColor || !hasFrameSkin)
                drawWhiteBoxRect(x, y, w, h, tint(node->progressBackColor));
            if (hasFrameSkin)
            {
                if (node->progressFrameThree)
                {
                    drawThreeSliceSafeColor(node->progressFrameTex, node->progressFrameUV,
                        node->progressFrameSrcW, node->progressFrameSrcH,
                        x, y, w, h,
                        node->progressFrameA, node->progressFrameB,
                        node->progressFrameVertical, node->progressFrameMirror,
                        tint(0xFFFFFFFF), nullptr, ctx.grayScale);
                }
                else
                {
                    renderNineSliceColorAlpha(node->progressFrameTex,
                        node->progressFrameUV[0], node->progressFrameUV[2],
                        node->progressFrameUV[1], node->progressFrameUV[3],
                        node->progressFrameSrcW, node->progressFrameSrcH,
                        x, y, w, h,
                        node->progressFrameA, node->progressFrameB,
                        node->progressFrameC, node->progressFrameD,
                        tint(0xFFFFFFFF), 1.f, nullptr, ctx.grayScale);
                }
            }
            Rect clipRc;
            bool hasClip = false;
            if (node->progressVertical)
            {
                float fillH = h * t;
                if (fillH > 0.01f)
                {
                    clipRc.set(x, y + h - fillH, w, fillH);
                    hasClip = true;
                }
            }
            else
            {
                float fillW = w * t;
                if (fillW > 0.01f)
                {
                    clipRc.set(x, y, fillW, h);
                    hasClip = true;
                }
            }
            if (hasClip)
            {
                if (node->progressFillTex)
                {
                    if (node->progressFillThree)
                    {
                        drawThreeSliceSafeColor(node->progressFillTex, node->progressFillUV,
                            node->progressFillSrcW, node->progressFillSrcH,
                            x, y, w, h,
                            node->progressFillA, node->progressFillB,
                            node->progressFillVertical, node->progressFillMirror,
                            tint(0xFFFFFFFF), &clipRc, ctx.grayScale);
                    }
                    else
                    {
                        renderNineSliceColorAlpha(node->progressFillTex,
                            node->progressFillUV[0], node->progressFillUV[2],
                            node->progressFillUV[1], node->progressFillUV[3],
                            node->progressFillSrcW, node->progressFillSrcH,
                            x, y, w, h,
                            node->progressFillA, node->progressFillB,
                            node->progressFillC, node->progressFillD,
                            tint(0xFFFFFFFF), 1.f, &clipRc, ctx.grayScale);
                    }
                }
                else
                {
                    drawWhiteBoxRect(clipRc.x, clipRc.y, clipRc.cx, clipRc.cy, tint(node->progressFillColor));
                }
            }
            float border = node->progressBorder >= 0.f ? node->progressBorder : (hasFrameSkin ? 0.f : 2.f);
            if (border > 0.f)
                drawWhiteBoxBorder(x, y, w, h, border, tint(node->progressBorderColor));
            registerHitRect(ctx.rootId, node->id, x, y, w, h);
        }
        ctx.updateBounds(x, y, w, h);
        if (w > ctx.maxContentWidth)
            ctx.maxContentWidth = w;
        ctx.currentY += h;
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
    case eHTMLTag::Particles:
    {
        ctx.flushLine(state, false);
        float w = node->reqWidth > 0.f ? node->reqWidth : 100.f;
        float h = node->reqHeight > 0.f ? node->reqHeight : 100.f;
        if (ctx.params.maxWidth > 0.f && w > ctx.params.maxWidth)
            w = ctx.params.maxWidth;
        if (w <= 0.f)
            w = 1.f;
        if (h <= 0.f)
            h = 1.f;
        float x = ctx.currentX;
        int hAlign = state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT);
        if (hAlign == FONS_ALIGN_CENTER)
        {
            if (ctx.params.maxWidth > 0.f)
                x = ctx.params.x + (ctx.params.maxWidth - w) * 0.5f;
        }
        else if (hAlign == FONS_ALIGN_RIGHT && ctx.params.maxWidth > 0.f)
        {
            x = ctx.params.x + ctx.params.maxWidth - w;
        }
        float y = ctx.currentY;
        if (!ctx.measureOnly)
        {
            auto itSys = _particlesSystems.find(node->id);
            if (itSys == _particlesSystems.end())
            {
                auto itFactory = _particlesPresets.find(domToLower(node->particlesPreset));
                if (itFactory != _particlesPresets.end())
                {
                    auto ps = itFactory->second();
                    if (ps)
                    {
                        itSys = _particlesSystems.emplace(node->id, ps).first;
                        if (node->particlesPrewarm > 0.f)
                            if (auto* pps = dynamic_cast<CParticleSystem<>*>(ps.get()))
                                pps->prewarm(node->particlesPrewarm);
                    }
                }
            }
            if (itSys != _particlesSystems.end() && itSys->second)
            {
                unsigned int pc = combinedTint;
                if (ctx.grayScale)
                    pc = domGrayColor(pc);
                float prgba[4] =
                {
                    ((pc >> 24) & 0xFF) / 255.f,
                    ((pc >> 16) & 0xFF) / 255.f,
                    ((pc >> 8) & 0xFF) / 255.f,
                    ((pc >> 0) & 0xFF) / 255.f
                };
                auto pMS = CGfx::getInstance()->getMatrixStack();
                pMS->save();
                pMS->translate(x + w * 0.5f, y + h * 0.5f);
                itSys->second->renderSelf(0.f, prgba);
                pMS->restore();
            }
            registerHitRect(ctx.rootId, node->id, x, y, w, h);
        }
        ctx.updateBounds(x, y, w, h);
        if (w > ctx.maxContentWidth)
            ctx.maxContentWidth = w;
        ctx.currentY += h;
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
#ifdef HAS_SPINE
    case eHTMLTag::Spine:
    {
        ctx.flushLine(state, false);
        if (!node->spineHasRcBox)
        {
            ctx.currentX = ctx.params.x;
            break;
        }
        float l = std::min(node->spineRc[0], node->spineRc[2]);
        float t = std::min(node->spineRc[1], node->spineRc[3]);
        float r = std::max(node->spineRc[0], node->spineRc[2]);
        float b = std::max(node->spineRc[1], node->spineRc[3]);
        float boxW = r - l, boxH = b - t;
        if (boxW <= 0.f || boxH <= 0.f)
        {
            ctx.currentX = ctx.params.x;
            break;
        }
        float layoutW = boxW, layoutH = boxH;
        computeImgSize(boxW, boxH, node->reqWidth, node->reqHeight, node->keepAspect, layoutW, layoutH);
        if (ctx.params.maxWidth > 0.f && layoutW > ctx.params.maxWidth)
        {
            float k = ctx.params.maxWidth / layoutW;
            layoutW = ctx.params.maxWidth;
            if (node->keepAspect || node->reqHeight <= 0.f)
                layoutH *= k;
        }
        if (layoutW <= 0.f)
            layoutW = 1.f;
        if (layoutH <= 0.f)
            layoutH = 1.f;
        const float padL = node->padding.left, padT = node->padding.top, padR = node->padding.right, padB = node->padding.bottom;
        float childMaxW = std::max(0.f, layoutW - padL - padR);
        float contentW = 0.f, contentH = 0.f;
        bool hasChild = !node->children.empty();
        if (hasChild)
        {
            LayoutParams childParams;
            childParams.x = 0.f;
            childParams.y = 0.f;
            childParams.maxWidth = childMaxW;
            childParams.align = FONS_ALIGN_LEFT;
            childParams.color = state.color;
            childParams.appearStart = ctx.blockStart;
            childParams.animKey = ctx.animKey;
            HTMLStateScope spM(state, *_scratch);
            state.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
            domRefreshFontState(state);
            Rect childRc;
            renderSequence(node->children, state, childParams, &childRc, true, ctx.rootId);
            contentW = std::max(0.f, childRc.cx);
            contentH = std::max(0.f, childRc.cy);
        }
        float w = layoutW, h = layoutH;
        if (node->reqWidth <= 0.f)
            w = std::max(w, contentW + padL + padR);
        if (node->reqHeight <= 0.f)
            h = std::max(h, contentH + padT + padB);
        if (w <= 0.f)
            w = 1.f;
        if (h <= 0.f)
            h = 1.f;
        if (ctx.params.maxWidth > 0.f && w > ctx.params.maxWidth)
            w = ctx.params.maxWidth;
        float x = ctx.currentX;
        int hAlign = state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT);
        if (hAlign == FONS_ALIGN_CENTER)
        {
            if (ctx.params.maxWidth > 0.f)
                x = ctx.params.x + (ctx.params.maxWidth - w) * 0.5f;
        }
        else if (hAlign == FONS_ALIGN_RIGHT && ctx.params.maxWidth > 0.f)
        {
            x = ctx.params.x + ctx.params.maxWidth - w;
        }
        float y = ctx.currentY;
        if (!ctx.measureOnly && !node->spinePath.empty())
        {
            unsigned long long spineId = node->spineId ? node->spineId : node->id;
            unsigned long long spineKey = makeSpineKey(ctx.animKey, spineId);
            auto it = _spineStates.find(spineKey);
            if (it == _spineStates.end())
            {
                SpineRuntimeState st;
                st.blockKey = ctx.animKey;
                st.startTime = ctx.blockStart;
                st.lastTime = ctx.animTime;
                it = _spineStates.emplace(spineKey, std::move(st)).first;
            }
            auto& st = it->second;
            st.nodeId = node->id;
            st.rootId = ctx.rootId;
            _spineKeyByNode[node->id] = spineKey;
            if (st.lastAnim.empty())
            {
                st.lastAnim = node->spineAnim;
                st.lastLoop = node->spineLoop;
            }
            float dt = ctx.animTime - st.lastTime;
            if (dt < 0.f)
                dt = 0.f;
            if (dt > 0.25f)
                dt = 0.25f;
            st.lastTime = ctx.animTime;
            float local = ctx.animTime - st.startTime - node->spineDelay;
            if (local >= 0.f)
            {
                if (!st.p)
                    st.p = CSpineManager::getInstance()->getNewSpine(node->spinePath.c_str());
                if (st.p)
                {
                    st.p->setSkipLight(true);
                    if (!st.animSet)
                    {
                        if (!node->spineAnim.empty() && st.p->hasAnimation(node->spineAnim.c_str()))
                        {
                            st.p->clearTracks();
                            st.p->setAnimation(0, node->spineAnim.c_str(), node->spineLoop);
                            st.p->update(0.f);
                            st.lastAnim = node->spineAnim;
                            st.lastLoop = node->spineLoop;
                        }
                        st.animSet = true;
                    }
                    else
                    {
                        st.p->update(dt);
                    }
                    if (!st.callbacksSet || st.rootId != ctx.rootId || st.nodeId != node->id)
                    {
                        unsigned long long cbRoot = ctx.rootId;
                        unsigned long long cbNode = node->id;
                        unsigned long long cbKey = spineKey;
                        st.p->onComplete([this, cbRoot, cbNode, cbKey](CSpine* owner, const char* animName)
                        {
                            auto cbNodePtr = getNode(cbNode);
                            if (!nodeHasIdAttr(cbNodePtr))
                                return;
                            auto cb = getDocumentCallbacks(cbRoot);
                            if (!cb)
                                return;
                            auto itState = _spineStates.find(cbKey);
                            if (itState != _spineStates.end() && animName)
                                itState->second.lastAnim = animName;
                            cb->onSpineAnimEvent(cbRoot, cbNode, eHTMLSpineEventType::AnimationComplete, animName ? animName : "", "");
                        });
                        st.p->onEvent([this, cbRoot, cbNode, cbKey](CSpine* owner, const char* eventName)
                        {
                            auto cbNodePtr = getNode(cbNode);
                            if (!nodeHasIdAttr(cbNodePtr))
                                return;
                            auto cb = getDocumentCallbacks(cbRoot);
                            if (!cb)
                                return;
                            std::string animName;
                            auto itState = _spineStates.find(cbKey);
                            if (itState != _spineStates.end())
                                animName = itState->second.lastAnim;
                            cb->onSpineAnimEvent(cbRoot, cbNode, eHTMLSpineEventType::SpineEvent, animName.c_str(), eventName ? eventName : "");
                        });
                        st.callbacksSet = true;
                        st.rootId = ctx.rootId;
                        st.nodeId = node->id;
                    }
                    unsigned int spineColor = combinedTint;
                    if (ctx.grayScale)
                        spineColor = domGrayColor(spineColor);
                    float docTint[4] =
                    {
                        ((spineColor >> 24) & 0xFF) / 255.f,
                        ((spineColor >> 16) & 0xFF) / 255.f,
                        ((spineColor >> 8) & 0xFF) / 255.f,
                        ((spineColor >> 0) & 0xFF) / 255.f
                    };
                    auto pMS = CGfx::getInstance()->getMatrixStack();
                    pMS->save();
                    if (node->keepAspect)
                    {
                        float scale = std::min(w / boxW, h / boxH);
                        float drawW = boxW * scale, drawH = boxH * scale;
                        float offX = x + (w - drawW) * 0.5f, offY = y + (h - drawH) * 0.5f;
                        pMS->translate(offX - l * scale, offY - t * scale);
                        if (scale != 1.f)
                            pMS->scale(scale, scale);
                    }
                    else
                    {
                        float sx = w / boxW, sy = h / boxH;
                        pMS->translate(x - l * sx, y - t * sy);
                        if (sx != 1.f || sy != 1.f)
                            pMS->scale(sx, sy);
                    }
                    st.p->renderSelf(dt, docTint);
                    pMS->restore();
                }
            }
        }
        size_t hitsBeforeChildren = 0;
        if (hasChild && !ctx.measureOnly)
            hitsBeforeChildren = _hitData[ctx.rootId].size();
        if (hasChild && !ctx.measureOnly)
        {
            LayoutParams childParams;
            childParams.x = x + padL;
            childParams.y = y + padT;
            childParams.maxWidth = std::max(0.f, w - padL - padR);
            childParams.align = (state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT)) | FONS_ALIGN_TOP;
            childParams.color = state.color;
            childParams.appearStart = ctx.blockStart;
            childParams.animKey = ctx.animKey;
            HTMLStateScope spR(state, *_scratch);
            state.align = childParams.align;
            domRefreshFontState(state);
            Rect childRc;
            renderSequence(node->children, state, childParams, &childRc, false, ctx.rootId);
        }
        ctx.updateBounds(x, y, w, h);
        if (w > ctx.maxContentWidth)
            ctx.maxContentWidth = w;
        if (!ctx.measureOnly)
        {
            if (hasChild)
                registerHitRectAt(ctx.rootId, node->id, x, y, w, h, hitsBeforeChildren);
            else
                registerHitRect(ctx.rootId, node->id, x, y, w, h);
        }
        ctx.currentY += h;
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
#endif
 case eHTMLTag::Table:
 {
     ctx.flushLine(state, false);
     size_t tableHitsBefore = 0;
     if (!ctx.measureOnly)
         tableHitsBefore = _hitData[ctx.rootId].size();
     const auto& cfg = node->tableConfig;
     HTMLRenderScratch& scTab = *_scratch;

     scTab.trNodes.clear();
     for (const auto& child : node->children)
         collectTrNodesRaw(child.get(), scTab.trNodes);
     scTab.cells.clear();
     scTab.rowCellStart.clear();
     scTab.rowCellStart.push_back(0);
     scTab.cellStylePool.clear();
     scTab.colAligns.clear();
     scTab.colMaxWidths.clear();
     int rowIdx = 0;
     for (HTMLDomNode* trNode : scTab.trNodes)
     {
         if (!trNode)
             continue;
         const int cellBase = (int)scTab.cells.size();
         for (const auto& trChild : trNode->children)
         {
             scTab.stylePath.clear();
             collectTdCellsRaw(trChild.get(), scTab.stylePath, scTab);
         }
         int colIdx = 0;
         for (int ci = cellBase; ci < (int)scTab.cells.size(); ++ci)
         {
             HTMLTableCell& cell = scTab.cells[ci];
             HTMLDomNode* tdNode = cell.tdNode;
             cell.bgColor = tdNode->bgColor;
             int cellAlign = tdNode->hasAlign ? tdNode->cellAlign : cfg.defaultAlign;
             if (rowIdx == 0)
             {
                 while (scTab.colAligns.size() <= (size_t)colIdx)
                     scTab.colAligns.push_back(cfg.defaultAlign);
                 scTab.colAligns[colIdx] = cellAlign;
             }
             cell.align = (colIdx < (int)scTab.colAligns.size()) ? scTab.colAligns[colIdx] : cellAlign;
             cell.vAlign = tdNode->hasVAlign ? tdNode->cellVAlign : cfg.defaultVAlign;
             cell.padding = tdNode->hasOwnPadding ? tdNode->cellPadding : cfg.defaultCellPadding;
             cell.maxWidth = tdNode->cellMaxWidth;
             if (scTab.colMaxWidths.size() <= (size_t)colIdx)
                 scTab.colMaxWidths.push_back(cell.maxWidth);
             ++colIdx;
         }
         if ((int)scTab.cells.size() > cellBase)
         {
             scTab.rowCellStart.push_back((int)scTab.cells.size());
             ++rowIdx;
         }
     }
     const int rowCount = (int)scTab.rowCellStart.size() - 1;
     if (rowCount <= 0)
     {
         ctx.currentX = ctx.params.x;
         break;
     }
     const size_t nCols = scTab.colMaxWidths.size();
     scTab.colWidths.assign(nCols, 0.f);
     scTab.colNatWidths.assign(nCols, 0.f);
     scTab.colMinWidths.assign(nCols, 0.f);
     scTab.colWant.assign(nCols, 0.f);
     scTab.rowHeights.assign((size_t)rowCount, 0.f);

     const int hAlign = state.align & (FONS_ALIGN_LEFT | FONS_ALIGN_CENTER | FONS_ALIGN_RIGHT);
     HTMLStateScope tableScope(state, scTab);
     state.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
     domRefreshFontState(state);
     float cellHeight = TextRender::getInstance()->getLineHeight();
     auto getColMaxWidth = [&](size_t i) -> float
     {
         if (i >= scTab.colMaxWidths.size())
             return 0.f;
         return std::max(0.f, scTab.colMaxWidths[i]);
     };
     auto measureCell = [&](HTMLTableCell& cell, float maxWidth, Rect& crc)
     {
         const float padL = cell.padding.left, padR = cell.padding.right, padT = cell.padding.top, padB = cell.padding.bottom;
         LayoutParams cp;
         cp.x = 0.f;
         cp.y = 0.f;
         cp.maxWidth = (maxWidth > 0.f) ? std::max(1.f, maxWidth - padL - padR) : 0.f;
         cp.align = FONS_ALIGN_LEFT;
         cp.color = state.color;
         cp.appearStart = 0.f;
         cp.animKey = ctx.animKey;
         HTMLStateScope cs(state, scTab);
         for (int si = cell.styleStart; si < cell.styleEnd; ++si)
             applyInlineNodeToState(scTab.cellStylePool[si], state);
         state.align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
         domRefreshFontState(state);
         if (cell.tdNode)
             renderSequence(cell.tdNode->children, state, cp, &crc, true, ctx.rootId);
         else
             crc.set(0.f, 0.f, 0.f, 0.f);
         crc.cx += padL + padR;
         crc.cy += padT + padB;
     };

     for (int r = 0; r < rowCount; ++r)
     {
         for (int ci = scTab.rowCellStart[r]; ci < scTab.rowCellStart[r + 1]; ++ci)
         {
             HTMLTableCell& cell = scTab.cells[ci];
             const size_t i = (size_t)(ci - scTab.rowCellStart[r]);
             const float colMaxW = getColMaxWidth(i);
             if (cell.tdNode && !cell.tdNode->children.empty())
             {
                 Rect crc;
                 measureCell(cell, colMaxW > 0.f ? colMaxW : 0.f, crc);
                 cell.width = crc.cx;
                 cell.height = crc.cy > 0.f ? crc.cy : cellHeight;
             }
             else
             {
                 cell.width = cell.padding.left + cell.padding.right;
                 cell.height = cellHeight + cell.padding.top + cell.padding.bottom;
             }
             if (colMaxW > 0.f)
                 cell.width = std::min(cell.width, colMaxW);
             scTab.colWidths[i] = std::max(scTab.colWidths[i], cell.width);
         }
     }
     for (size_t i = 0; i < nCols; ++i)
     {
         const float colMaxW = getColMaxWidth(i);
         if (colMaxW > 0.f)
             scTab.colWidths[i] = std::min(scTab.colWidths[i], colMaxW);
     }
     scTab.colNatWidths = scTab.colWidths;
     float tableAvail = 0.f;
     if (ctx.params.maxWidth > 0.f)
         tableAvail = std::max(0.f, ctx.params.maxWidth - 2.f * cfg.cellSpacing);
     if (cfg.maxWidth > 0.f)
     {
         float cap = std::max(0.f, cfg.maxWidth - 2.f * cfg.cellSpacing);
         tableAvail = (tableAvail > 0.f) ? std::min(tableAvail, cap) : cap;
     }
     if (tableAvail > 0.f && nCols > 0)
     {
         float padTotal = cfg.cellPadding * (float)(nCols > 0 ? nCols - 1 : 0);
         float sumNat = 0.f;
         for (float w : scTab.colWidths)
             sumNat += w;
         if (sumNat + padTotal > tableAvail)
         {
             scTab.colMinWidths.assign(nCols, 0.f);
             for (int r = 0; r < rowCount; ++r)
             {
                 for (int ci = scTab.rowCellStart[r]; ci < scTab.rowCellStart[r + 1]; ++ci)
                 {
                     HTMLTableCell& cell = scTab.cells[ci];
                     const size_t i = (size_t)(ci - scTab.rowCellStart[r]);
                     if (cell.tdNode && !cell.tdNode->children.empty())
                     {
                         const float colMaxW = getColMaxWidth(i);
                         Rect crc;
                         measureCell(cell, colMaxW > 0.f ? std::min(1.f, colMaxW) : 1.f, crc);
                         scTab.colMinWidths[i] = std::max(scTab.colMinWidths[i], crc.cx);
                     }
                 }
             }
             for (size_t i = 0; i < nCols; ++i)
             {
                 const float colMaxW = getColMaxWidth(i);
                 if (colMaxW > 0.f)
                     scTab.colMinWidths[i] = std::min(scTab.colMinWidths[i], colMaxW);
             }
             float sumMin = 0.f;
             for (float w : scTab.colMinWidths)
                 sumMin += w;
             float availContent = tableAvail - padTotal;
             if (availContent <= sumMin)
                 scTab.colWidths = scTab.colMinWidths;
             else if (sumNat > sumMin)
             {
                 float k = (availContent - sumMin) / (sumNat - sumMin);
                 for (size_t i = 0; i < nCols; ++i)
                     scTab.colWidths[i] = scTab.colMinWidths[i] + (scTab.colWidths[i] - scTab.colMinWidths[i]) * k;
             }
             for (size_t i = 0; i < nCols; ++i)
             {
                 const float colMaxW = getColMaxWidth(i);
                 if (colMaxW > 0.f)
                     scTab.colWidths[i] = std::min(scTab.colWidths[i], colMaxW);
             }
         }
     }
     auto measureRowHeights = [&]()
     {
         for (int r = 0; r < rowCount; ++r)
         {
             float maxH = 0.f;
             for (int ci = scTab.rowCellStart[r]; ci < scTab.rowCellStart[r + 1]; ++ci)
             {
                 HTMLTableCell& cell = scTab.cells[ci];
                 const size_t i = (size_t)(ci - scTab.rowCellStart[r]);
                 if (cell.tdNode && !cell.tdNode->children.empty())
                 {
                     const float colMaxW = getColMaxWidth(i);
                     float measureWidth = scTab.colWidths[i] + 1.f;
                     if (colMaxW > 0.f)
                         measureWidth = std::min(measureWidth, colMaxW);
                     Rect crc;
                     measureCell(cell, measureWidth, crc);
                     cell.width = crc.cx;
                     if (colMaxW > 0.f)
                         cell.width = std::min(cell.width, colMaxW);
                     cell.height = crc.cy > 0.f ? crc.cy : cellHeight;
                 }
                 maxH = std::max(maxH, cell.height);
             }
             scTab.rowHeights[r] = maxH;
         }
     };
     measureRowHeights();

     if (nCols > 0)
     {
         float freed = 0.f;
         for (size_t i = 0; i < nCols; ++i)
         {
             float used = 0.f;
             for (int r = 0; r < rowCount; ++r)
             {
                 int ci = scTab.rowCellStart[r] + (int)i;
                 if (ci < scTab.rowCellStart[r + 1])
                     used = std::max(used, scTab.cells[ci].width);
             }
             used = std::min(used, scTab.colWidths[i]);
             float used2 = used;
             for (int r = 0; r < rowCount; ++r)
             {
                 int ci = scTab.rowCellStart[r] + (int)i;
                 if (ci >= scTab.rowCellStart[r + 1])
                     continue;
                 HTMLTableCell& cell = scTab.cells[ci];
                 if (cell.tdNode && !cell.tdNode->children.empty())
                 {
                     const float colMaxW = getColMaxWidth(i);
                     float measureWidth = used + 1.f;
                     if (colMaxW > 0.f)
                         measureWidth = std::min(measureWidth, colMaxW);
                     Rect crc;
                     measureCell(cell, measureWidth, crc);
                     used2 = std::max(used2, std::min(crc.cx, scTab.colWidths[i]));
                 }
             }
             used2 = std::min(used2, scTab.colWidths[i]);
             freed += scTab.colWidths[i] - used2;
             scTab.colWidths[i] = used2;
             scTab.colWant[i] = std::max(0.f, scTab.colNatWidths[i] - used2);
             const float colMaxW = getColMaxWidth(i);
             if (colMaxW > 0.f)
             {
                 scTab.colWidths[i] = std::min(scTab.colWidths[i], colMaxW);
                 scTab.colWant[i] = std::max(0.f, std::min(scTab.colNatWidths[i], colMaxW) - scTab.colWidths[i]);
             }
         }
         if (freed > 0.5f)
         {
             scTab.order.resize(nCols);
             for (size_t i = 0; i < nCols; ++i)
                 scTab.order[i] = i;
             std::sort(scTab.order.begin(), scTab.order.end(), [&scTab](size_t a, size_t b) { return scTab.colWant[a] < scTab.colWant[b]; });
             for (size_t idx : scTab.order)
             {
                 if (freed <= 0.5f || scTab.colWant[idx] <= 0.5f)
                     continue;
                 if (scTab.colWant[idx] <= freed)
                 {
                     scTab.colWidths[idx] += scTab.colWant[idx];
                     freed -= scTab.colWant[idx];
                     scTab.colWant[idx] = 0.f;
                 }
             }
         }
         measureRowHeights();
     }
     float tableTotalWidth = 0.f;
     for (size_t i = 0; i < nCols; ++i)
     {
         tableTotalWidth += scTab.colWidths[i];
         if (i + 1 < nCols)
             tableTotalWidth += cfg.cellPadding;
     }
     float tableTotalHeight = 0.f;
     for (int r = 0; r < rowCount; ++r)
         tableTotalHeight += scTab.rowHeights[r];
     float tableX = ctx.currentX + cfg.cellSpacing;

     float outerW = tableTotalWidth + cfg.cellSpacing * 2.f;
     float outerH = tableTotalHeight + cfg.cellSpacing * 2.f;
     if (outerW > ctx.maxContentWidth)
         ctx.maxContentWidth = outerW;
     if (hAlign == FONS_ALIGN_CENTER && ctx.params.maxWidth > 0.f)
         tableX = ctx.params.x + (ctx.params.maxWidth - outerW) * 0.5f + cfg.cellSpacing;
     else if (hAlign == FONS_ALIGN_RIGHT && ctx.params.maxWidth > 0.f)
         tableX = ctx.params.x + ctx.params.maxWidth - outerW + cfg.cellSpacing;
     float tableY = ctx.currentY + cfg.cellSpacing;
     float rowY = tableY;
     scTab.rowYBounds.clear();
     scTab.rowYBounds.push_back(tableY);
     for (int r = 0; r < rowCount; ++r)
     {
         float colX = tableX;
         float rowTop = rowY;
         scTab.colXBounds.clear();
         scTab.colXBounds.push_back(colX);
         const int cellStart = scTab.rowCellStart[r];
         const int cellEnd = scTab.rowCellStart[r + 1];
         const float rowH = scTab.rowHeights[r];
         for (int ci = cellStart; ci < cellEnd; ++ci)
         {
             const HTMLTableCell& cell = scTab.cells[ci];
             const size_t c = (size_t)(ci - cellStart);
             float cellWidth = (c < nCols) ? scTab.colWidths[c] : 0.f;
             const float cellMaxWidth = getColMaxWidth(c);
             if (cellMaxWidth > 0.f)
                 cellWidth = std::min(cellWidth, cellMaxWidth);
             const float padL = cell.padding.left, padR = cell.padding.right, padT = cell.padding.top, padB = cell.padding.bottom;
             if (cell.bgColor != 0)
                 ctx.renderLine(colX, rowY, cellWidth, rowH, tint(cell.bgColor));
             size_t cellHitsBefore = 0;
             bool cellContentRendered = false;
             if (cell.tdNode && !cell.tdNode->children.empty() && !ctx.measureOnly)
             {
                cellHitsBefore = _hitData[ctx.rootId].size();
                const float contentW = std::max(0.f, cellWidth - padL - padR);

                const float cellContentH = std::max(0.f, cell.height - padT - padB);
                const float availH = std::max(0.f, rowH - padT - padB);
                float offY = padT;
                if (cell.vAlign == eVAlign::MIDDLE)
                    offY = padT + std::max(0.f, (availH - cellContentH) * 0.5f);
                else if (cell.vAlign == eVAlign::BOTTOM)
                    offY = padT + std::max(0.f, availH - cellContentH);
                LayoutParams cp;
                 cp.x = colX + padL;
                 cp.y = rowY + offY;
                 float contentMaxWidth = contentW + 1.f;
                 if (cellMaxWidth > 0.f)
                     contentMaxWidth = std::min(contentMaxWidth, std::max(0.f, cellMaxWidth - padL - padR));
                 cp.maxWidth = contentMaxWidth;
                 cp.align = cell.align;
                 cp.color = state.color;
                 cp.appearStart = ctx.blockStart;
                 cp.animKey = ctx.animKey;
                 HTMLStateScope cs2(state, scTab);
                 state.align = cell.align | FONS_ALIGN_TOP;
                 for (int si = cell.styleStart; si < cell.styleEnd; ++si)
                     applyInlineNodeToState(scTab.cellStylePool[si], state);
                 Rect crc;
                 renderSequence(cell.tdNode->children, state, cp, &crc, false, ctx.rootId);
                 cellContentRendered = true;
             }
             ctx.updateBounds(colX, rowY, cellWidth, rowH);
             if (!ctx.measureOnly && cell.tdNode)
             {
                 if (cellContentRendered)
                     registerHitRectAt(ctx.rootId, cell.tdNode->id, colX, rowY, cellWidth, rowH, cellHitsBefore);
                 else
                     registerHitRect(ctx.rootId, cell.tdNode->id, colX, rowY, cellWidth, rowH);
             }
             colX += cellWidth + cfg.cellPadding;
             scTab.colXBounds.push_back(colX);
         }
         rowY += rowH;
         scTab.rowYBounds.push_back(rowY);
         if (cfg.grid > 0.f)
         {
             bool drawVertical = false;
             if (cfg.gridStyle == eGridStyle::ALL)
                 drawVertical = true;
             else if (cfg.gridStyle == eGridStyle::HEADER && r == 0)
                 drawVertical = true;
             else if (cfg.gridStyle == eGridStyle::VERTICAL)
                 drawVertical = true;
             else if (cfg.gridStyle == eGridStyle::VERTICAL_HORIZONTAL && r > 0)
                 drawVertical = true;
             if (drawVertical)
             {
                 for (size_t c2 = 1; c2 + 1 < scTab.colXBounds.size(); ++c2)
                     ctx.renderLine(scTab.colXBounds[c2] - cfg.grid * 0.5f, rowTop, cfg.grid, rowH, tint(cfg.gridColor));
             }
         }
     }
     if (cfg.grid > 0.f)
     {
         for (size_t rb = 1; rb < scTab.rowYBounds.size(); ++rb)
         {
             bool drawHorizontal = false;
             if (cfg.gridStyle == eGridStyle::ALL)
                 drawHorizontal = true;
             else if (cfg.gridStyle == eGridStyle::HEADER && rb == 1)
                 drawHorizontal = true;
             else if (cfg.gridStyle == eGridStyle::HORIZONTAL)
                 drawHorizontal = true;
             else if (cfg.gridStyle == eGridStyle::VERTICAL_HORIZONTAL && rb >= 1)
                 drawHorizontal = true;
             if (drawHorizontal)
                 ctx.renderLine(tableX, scTab.rowYBounds[rb] - cfg.grid * 0.5f, tableTotalWidth, cfg.grid, tint(cfg.gridColor));
         }
         if (cfg.gridStyle == eGridStyle::HEADER)
             ctx.renderLine(tableX, scTab.rowYBounds[0] - cfg.grid * 0.5f, tableTotalWidth, cfg.grid, tint(cfg.gridColor));
     }
     if (cfg.border > 0.f)
     {
         float bX = tableX - cfg.cellSpacing, bY = tableY - cfg.cellSpacing;
         ctx.renderLine(bX, bY, outerW, cfg.border, tint(cfg.borderColor));
         ctx.renderLine(bX, bY + outerH - cfg.border, outerW, cfg.border, tint(cfg.borderColor));
         ctx.renderLine(bX, bY, cfg.border, outerH, tint(cfg.borderColor));
         ctx.renderLine(bX + outerW - cfg.border, bY, cfg.border, outerH, tint(cfg.borderColor));
     }
     ctx.updateBounds(tableX - cfg.cellSpacing, tableY - cfg.cellSpacing, outerW, outerH);
     if (!ctx.measureOnly)
         registerHitRectAt(ctx.rootId, node->id, tableX - cfg.cellSpacing, tableY - cfg.cellSpacing, outerW, outerH, tableHitsBefore);
     ctx.currentY = rowY + cfg.cellSpacing;
     ctx.currentX = ctx.params.x;
     ctx.lineHeight = domGetLineHeightForState(state);
     break;
 }

    case eHTMLTag::Font:
    {
        float oldLineHeight = ctx.lineHeight;
        {
            HTMLStateScope fScope(state, *_scratch);
            applyFontAttrs(state, node->fontAttrs);
            if (node->fontAttrs.hasColor && _fgFadeActive && !interactionBlocked)
            {
                if (_fgFadeHover > 0.001f)
                    state.color = lerpColor(state.color, _fgFadeHoverColor, _fgFadeHover);
                if (_fgFadePressed > 0.001f)
                    state.color = lerpColor(state.color, _fgFadePressedColor, _fgFadePressed);
            }
            domRefreshFontState(state);
            ctx.lineHeight = domGetLineHeightForState(state);
            unsigned long long prevFontAnimId = ctx.currentFontAnimId;
            if (node->fontAttrs.hasAppear || node->fontAttrs.hasFx)
                ctx.currentFontAnimId = node->id;
            renderChildren(node, state, ctx);
            ctx.currentFontAnimId = prevFontAnimId;
        }
        ctx.lineHeight = oldLineHeight;
        break;
    }
    case eHTMLTag::B:
    case eHTMLTag::Strong:
    {
        float oldLineHeight = ctx.lineHeight;
        {
            HTMLStateScope bScope(state, *_scratch);
            state.bold = true;
            domRefreshFontState(state);
            ctx.lineHeight = domGetLineHeightForState(state);
            renderChildren(node, state, ctx);
        }
        ctx.lineHeight = oldLineHeight;
        break;
    }
    case eHTMLTag::I:
    case eHTMLTag::Em:
    {
        float oldLineHeight = ctx.lineHeight;
        {
            HTMLStateScope iScope(state, *_scratch);
            state.italic = true;
            domRefreshFontState(state);
            ctx.lineHeight = domGetLineHeightForState(state);
            renderChildren(node, state, ctx);
        }
        ctx.lineHeight = oldLineHeight;
        break;
    }
    case eHTMLTag::U:
    {
        HTMLStateScope uScope(state, *_scratch);
        state.underline = true;
        if (node->lineHasColor)
            state.lineColor = node->lineColor;
        if (node->lineThickness > 0.f)
            state.lineThickness = node->lineThickness;
        renderChildren(node, state, ctx);
        break;
    }
    case eHTMLTag::S:
    {
        HTMLStateScope sScope(state, *_scratch);
        state.strikethrough = true;
        if (node->lineHasColor)
            state.lineColor = node->lineColor;
        if (node->lineThickness > 0.f)
            state.lineThickness = node->lineThickness;
        renderChildren(node, state, ctx);
        break;
    }
    case eHTMLTag::P:
    case eHTMLTag::H1:
    case eHTMLTag::H2:
    case eHTMLTag::H3:
    {
        ctx.flushLine(state, false);
        float startY = ctx.currentY;
        size_t hitsBeforeChildren = 0;
        if (!ctx.measureOnly)
            hitsBeforeChildren = _hitData[ctx.rootId].size();
        ctx.currentY += ctx.lineHeight * 0.5f;
        ctx.currentX = ctx.params.x;
        {
            HTMLStateScope pScope(state, *_scratch);
            if (node->tag == eHTMLTag::H1)
                state.fontSize = Engine::getCfg().HTMLCfg.fH1FontSize;
            else if (node->tag == eHTMLTag::H2)
                state.fontSize = Engine::getCfg().HTMLCfg.fH2FontSize;
            else if (node->tag == eHTMLTag::H3)
                state.fontSize = Engine::getCfg().HTMLCfg.fH3FontSize;
            domRefreshFontState(state);
            ctx.lineHeight = domGetLineHeightForState(state);
            renderChildren(node, state, ctx);
            ctx.flushLine(state, false);
        }
        ctx.currentY += ctx.lineHeight * 0.5f;
        float endY = ctx.currentY;
        float blockW = ctx.params.maxWidth;
        if (!ctx.measureOnly && blockW > 0.f)
            registerHitRectAt(ctx.rootId, node->id, ctx.params.x, startY, blockW, endY - startY, hitsBeforeChildren);
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
    case eHTMLTag::Center:
    {
        ctx.flushLine(state, false);
        float startY = ctx.currentY;
        size_t hitsBeforeChildren = 0;
        if (!ctx.measureOnly)
            hitsBeforeChildren = _hitData[ctx.rootId].size();
        {
            HTMLStateScope cScope(state, *_scratch);
            state.align = FONS_ALIGN_CENTER | FONS_ALIGN_TOP;
            domRefreshFontState(state);
            renderChildren(node, state, ctx);
            ctx.flushLine(state, false);
        }
        float endY = ctx.currentY;
        float blockW = ctx.params.maxWidth;
        if (!ctx.measureOnly && blockW > 0.f)
            registerHitRectAt(ctx.rootId, node->id, ctx.params.x, startY, blockW, endY - startY, hitsBeforeChildren);
        ctx.currentX = ctx.params.x;
        ctx.lineHeight = domGetLineHeightForState(state);
        break;
    }
    case eHTMLTag::A:
    {
        unsigned long long oldLinkId = ctx.currentLinkId;
        ctx.currentLinkId = node->id;
        renderChildren(node, state, ctx);
        ctx.currentLinkId = oldLinkId;
        break;
    }
    default:
        renderChildren(node, state, ctx);
        break;
    }
    if (bVisTransform)
    {
        if (visGotSelf)
        {
            node->visLastRect.set(
                visSelfRc.x - visEntryX,
                visSelfRc.y - visEntryY,
                visSelfRc.cx,
                visSelfRc.cy);
        }
        ctx.visRecordRc = nullptr;
        ctx.visRecordGot = nullptr;
        _visMatStack.pop_back();
        if (auto pMS = CGfx::getInstance()->getMatrixStack())
            pMS->restore();
    }
    if (bRelShift)
    {
        ctx.currentX -= relDX;
        ctx.currentY -= relDY;
        ctx.params.x -= relDX;
    }
    if (bHasMargin)
    {
        ctx.currentY += node->margin.bottom;
        ctx.params = savedParams;
        ctx.currentX = ctx.params.x;
    }
    _fgFadeActive = savedFadeActive;
    _fgFadeHover = savedFadeHover;
    _fgFadePressed = savedFadePressed;
    _fgFadeHoverColor = savedFadeHoverColor;
    _fgFadePressedColor = savedFadePressedColor;
}

void HTMLDom::handleMouseLeave(unsigned long long rootId)
{
    if (rootId != 0)
    {
        if (_hoverRootId != rootId && _pressedRootId != rootId &&
            _cursorRootId != rootId && _dragRootId != rootId &&
            _editDragRootId != rootId)
            return;
    }
    endSliderDrag(true);
    stopEditDrag(true);
    if (_cursorTargetId != 0)
    {
        unsigned long long oldRoot = _cursorRootId;
        if (auto cb = getDocumentCallbacks(oldRoot))
        {
            cb->onLeave(oldRoot, _cursorTargetId);
            if (_currentCursor != "normal")
                cb->onCursorChanged("normal");
        }
    }
    _cursorTargetId = 0;
    _cursorRootId = 0;
    _currentCursor = "normal";
    _hoverTargetId = 0;
    _hoverRootId = 0;
    _fgHoverTargetId = 0;
    _fgPressedTargetId = 0;
    clearInputState();
}

_G2D_NAMESPACE_END_
