#pragma once
#include "g2d.h"
#include "sprite.h"
#include "sconts.h"
#include "engine.h"
#include "fontstash.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "easing.h"
struct FONScontext;
_G2D_NAMESPACE_BEGIN_
#ifdef HAS_SPINE
class CSpine;
#endif
class CContainer;
struct HTMLRenderState;

struct HTMLRenderScratch;
enum class eFxStyle
{
    NONE = 0, DUI, WAVE, SHAKE, PULSE, RAINBOW, GLITCH, TYPEWRITER, BOUNCE,
    ROTATE, FIRE, BLINK, BREATHE, SWAY, DRIFT, GLOW, SHIMMER, ISHIMMER,
    CANDLE, MIST, HUE, SOFTFADE, SPARKLE, AURORA, OCEAN, SUNSET, FROST,
    LAVA, MOONLIGHT, STARLIGHT, NEONPULSE, ROSE, MINT, GOLDWAVE, SILVERWAVE,
    PEARL, OPAL, EMERALD, SAPPHIRE, RUBY, AMETHYST, TOPAZ, JADE, NORTHERN,
    TIDE, FIREFLY, SNOWFALL, HEARTBEAT, ORBIT, BLOSSOM, RIPPLE, ZEN, COSMOS,
    RIFT, PHANTOM, MIRROR, WHIRL, TIMEWARP,
    ELECTRIC, SUPERNOVA, HOLY, POISON, DREAD, BLOODLUST,
    SPECTRAL, MAGMA, FROSTBITE, VENOM, METEOR,
    ECLIPSE, TESLA, ROSEGOLD, VOIDWALK, PRISM
};
enum class eAppearStyle
{
    NONE = 0, FADE, RISE, POP, CASCADE, GLITCH_IN, MATRIX, NEON, TYPEWRITER,
    COMET, ASSEMBLE, HOLOGRAM, LIGHTNING, LASERSCAN, VHS, PIXELATE, INK,
    SHATTER, EMBER, GHOST, RUNIC
};
enum class eGridStyle
{
    NONE = 0, ALL, HEADER, VERTICAL, HORIZONTAL, VERTICAL_HORIZONTAL
};
enum class eVAlign
{
    TOP = 0, MIDDLE, BOTTOM
};
#ifdef HAS_SPINE
enum class eHTMLSpineEventType
{
    AnimationComplete = 0,
    SpineEvent
};
#endif
enum class eHTMLImageAnimEventType
{
    AnimationComplete = 0
};
enum class eHTMLFontAnimEventType
{
    None = 0, AppearStart, AppearComplete, FxStart, FxComplete, Stopped
};
struct HTMLPadding
{
    float left = 8.0f;
    float top = 8.0f;
    float right = 8.0f;
    float bottom = 8.0f;
};
using HTMLAttrsMap = FixedSmallMap<std::string, std::string, 30>;

struct HTMLCssClassDecl
{
    std::string name;
    std::string value;
};
struct HTMLCssClassRule
{
    std::vector<HTMLCssClassDecl> decls;

    bool inheritAll = false;
};
struct HTMLNode;
using HTMLDocument = std::vector<HTMLNode>;
struct HTMLTableData
{
    struct Cell
    {
        std::string text;
        float width = 0.f;
        float height = 0.f;
        int align = FONS_ALIGN_LEFT;
        eVAlign vAlign = eVAlign::TOP;
        unsigned int bgColor = 0;
        HTMLPadding padding;
        std::shared_ptr<HTMLDocument> contentDoc;
    };
    struct Row
    {
        std::vector<Cell> cells;
        float height = 0.f;
    };
    std::vector<Row> rows;
    std::vector<float> colWidths;
    std::vector<int> colAligns;
    std::vector<float> colMaxWidths;
    float totalWidth = 0.f;
    float totalHeight = 0.f;
    float maxWidth = 0.f;
    float cellPadding = 25.f;
    float cellSpacing = 0.f;
    HTMLPadding defaultCellPadding;
    int defaultAlign = FONS_ALIGN_LEFT;
    eVAlign defaultVAlign = eVAlign::TOP;
    float border = 0.f;
    unsigned int borderColor = 0xFFFFFFFF;
    eGridStyle gridStyle = eGridStyle::NONE;
    float grid = 0.f;
    unsigned int gridColor = 0x808080FF;
};
struct HTMLNode
{
    enum class Type
    {
        Text, Tag, Table, Break, HR, Image, NineSlice, ThreeSlice, Spine
    };
    Type type = Type::Text;
    std::string text;
    std::string tagName;
    std::string spritePath;
    float imgWidth = 0.f;
    float imgHeight = 0.f;
    bool keepAspect = false;
    bool isClosing = false;
    float sliceA = 0.f;
    float sliceB = 0.f;
    float sliceC = 0.f;
    float sliceD = 0.f;
    bool sliceVertical = false;
    int sliceMirror = 0;
    HTMLPadding padding;
    std::shared_ptr<HTMLDocument> childDoc;
    HTMLAttrsMap attrs;
    std::shared_ptr<HTMLTableData> tableData;
    unsigned long long spineId = 0;
    std::string spinePath;
    std::string spineAnim;
    bool spineLoop = true;
    float spineDelay = 0.f;
    bool spineHasRcBox = false;
    float spineRc[4] = { 0.f, 0.f, 0.f, 0.f };
    CTexturePtr spriteTex;
    float spriteUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float spriteSrcW = 0.f;
    float spriteSrcH = 0.f;
    std::string idleStyle;
    float idleDelay = -1.f;
    float idleDur = -1.f;
    float glareWidth = 40.f;
    float glareSpeed = 150.f;
    float glareTilt = 20.f;
    int glareDir = 0;
    float glareIntensity = 0.7f;
    float pxBlock = 10.f;
    float pxBand = 56.f;
    float pxSpeed = 260.f;
    float pxGlow = 1.f;
};
enum class eSpecialKey
{
    SK_NONE = 0,
    SK_BACKSPACE,
    SK_DELETE,
    SK_LEFT,
    SK_RIGHT,
    SK_HOME,
    SK_END,
    SK_ENTER,
    SK_ESCAPE,
    SK_COPY,
    SK_PASTE,
    SK_CUT,
    SK_SELECT_ALL
};
enum class eHTMLTag
{
    Unknown = 0,
    Document,
    Body,
    BgSpine,
    Text,
    Br,
    Hr,
    Img,
    Nine,
    Three,
    Spine,
    Particles,
    Table,
    Tr,
    Td,
    Font,
    B,
    Strong,
    I,
    Em,
    U,
    S,
    P,
    H1,
    H2,
    H3,
    Center,
    A,
    NineButton,
    Ul,
    Li,
    Checkbox,
    Select,
    Option,
    Slider,
    ProgressBar,
    Span,
    Edit
};
enum class eHTMLIdleStyle
{
    None = 0, Glare, Pixel, Chase, Lens, Echo, Embers, Tear, Lightning,
    Ice, Melt, Dust, Vortex, Confetti
};
enum class eHTMLInputEvent
{
    MOUSE_DOWN,
    MOUSE_UP,
    MOUSE_MOVE
};

enum class eHTMLTweenProp
{
    None = 0,
    OffsetX, OffsetY,
    ScaleX, ScaleY,
    Rotation,
    PivotX, PivotY,
    Alpha,
    Left, Top,
    Width, Height,
    Color,
    BgColor,
    FontSize,
    Progress
};
struct HTMLTweenProps
{
    struct Channel
    {
        eHTMLTweenProp prop = eHTMLTweenProp::None;
        bool isColor = false;
        float valueF = 0.f;
        unsigned int valueC = 0xFFFFFFFF;
    };
    enum { MAX_CHANNELS = 8 };
    Channel ch[MAX_CHANNELS];
    int nCh = 0;
    HTMLTweenProps& add(eHTMLTweenProp prop, float value)
    {
        if (nCh >= MAX_CHANNELS)
            return *this;
        ch[nCh].prop = prop;
        ch[nCh].isColor = false;
        ch[nCh].valueF = value;
        ++nCh;
        return *this;
    }
    HTMLTweenProps& add(eHTMLTweenProp prop, unsigned int color)
    {
        if (nCh >= MAX_CHANNELS)
            return *this;
        ch[nCh].prop = prop;
        ch[nCh].isColor = true;
        ch[nCh].valueC = color;
        ++nCh;
        return *this;
    }
    HTMLTweenProps& offX(float v)           { return add(eHTMLTweenProp::OffsetX, v); }
    HTMLTweenProps& offY(float v)           { return add(eHTMLTweenProp::OffsetY, v); }
    HTMLTweenProps& scaleX(float v)         { return add(eHTMLTweenProp::ScaleX, v); }
    HTMLTweenProps& scaleY(float v)         { return add(eHTMLTweenProp::ScaleY, v); }
    HTMLTweenProps& scale(float v)          { scaleX(v); return scaleY(v); }
    HTMLTweenProps& rotation(float rad)     { return add(eHTMLTweenProp::Rotation, rad); }
    HTMLTweenProps& pivotX(float v)         { return add(eHTMLTweenProp::PivotX, v); }
    HTMLTweenProps& pivotY(float v)         { return add(eHTMLTweenProp::PivotY, v); }
    HTMLTweenProps& pivot(float x, float y) { pivotX(x); return pivotY(y); }
    HTMLTweenProps& alpha(float v)          { return add(eHTMLTweenProp::Alpha, v); }
    HTMLTweenProps& left(float v)           { return add(eHTMLTweenProp::Left, v); }
    HTMLTweenProps& top(float v)            { return add(eHTMLTweenProp::Top, v); }
    HTMLTweenProps& width(float v)          { return add(eHTMLTweenProp::Width, v); }
    HTMLTweenProps& height(float v)         { return add(eHTMLTweenProp::Height, v); }
    HTMLTweenProps& color(unsigned int c)   { return add(eHTMLTweenProp::Color, c); }
    HTMLTweenProps& bgColor(unsigned int c) { return add(eHTMLTweenProp::BgColor, c); }
    HTMLTweenProps& fontSize(float v)       { return add(eHTMLTweenProp::FontSize, v); }
    HTMLTweenProps& progress(float v)       { return add(eHTMLTweenProp::Progress, v); }
};
struct HTMLTweenOptions
{
    float delay = 0.f;
    easingFunction ease = Easing::outQuad;
    easingFunction easeBack = nullptr;
    int repeat = 0;
    bool yoyo = false;
    std::function<void(unsigned long long nodeId)> onComplete;
};
struct HTMLTweenChannel
{
    eHTMLTweenProp prop = eHTMLTweenProp::None;
    bool isColor = false;
    float fromF = 0.f, toF = 0.f;
    unsigned int fromC = 0xFFFFFFFF, toC = 0xFFFFFFFF;
};
struct HTMLTween
{
    unsigned long long tweenId = 0;
    unsigned long long nodeId = 0;
    unsigned long long nodeGen = 0;
    unsigned long long rootId = 0;
    float delay = 0.f;
    float dur = 0.3f;
    float t = 0.f;
    int repeat = 0;
    bool yoyo = false;
    bool reversed = false;
    easingFunction ease = Easing::outQuad;
    easingFunction easeBack = nullptr;
    std::function<void(unsigned long long nodeId)> onComplete;
    HTMLTweenChannel ch[HTMLTweenProps::MAX_CHANNELS];
    int nCh = 0;
};
struct HTMLFontAttrs
{
    bool hasFontName = false;
    std::string fontName;
    bool hasFontSize = false;
    int fontSize = 0;
    bool hasColor = false;
    unsigned int color = 0xFFFFFFFF;
    bool hasFx = false;
    eFxStyle fxStyle = eFxStyle::NONE;
    bool hasShadow = false;
    float shadow = 0.f;
    bool hasShadowColor = false;
    unsigned int shadowColor = 0x000000FF;
    bool hasOutline = false;
    float outline = 0.f;
    bool hasOutlineColor = false;
    unsigned int outlineColor = 0x000000FF;
    bool hasGradient = false;
    unsigned int gradientColor1 = 0xFFFFFFFF;
    unsigned int gradientColor2 = 0xFFFFFFFF;
    bool hasAppear = false;
    eAppearStyle appearStyle = eAppearStyle::NONE;
    bool hasAppearDur = false;
    float appearDur = 0.8f;
    bool hasAppearDelay = false;
    float appearDelay = 0.f;
    bool hasFxDelay = false;
    float fxDelay = 0.f;
    bool hasLineHeight = false;
    float lineHeightMul = 0.f;
};
struct HTMLDomTableConfig
{
    float cellPadding = 25.f;
    float cellSpacing = 0.f;
    float maxWidth = 0.f;
    HTMLPadding defaultCellPadding;
    int defaultAlign = FONS_ALIGN_LEFT;
    eVAlign defaultVAlign = eVAlign::TOP;
    float border = 0.f;
    unsigned int borderColor = 0xFFFFFFFF;
    eGridStyle gridStyle = eGridStyle::NONE;
    float grid = 0.f;
    unsigned int gridColor = 0x808080FF;
    HTMLDomTableConfig()
    {
        defaultCellPadding.left = 0.f;
        defaultCellPadding.top = 0.f;
        defaultCellPadding.right = 0.f;
        defaultCellPadding.bottom = 0.f;
    }
};
struct HTMLDomNode
{
    using ClickHandler =
        std::function<void(unsigned long long nodeId, float x, float y, int button)>;
    unsigned long long id = 0;
    unsigned long long parentId = 0;
    eHTMLTag tag = eHTMLTag::Unknown;
    std::string text;
    HTMLAttrsMap attrs;

    std::vector<std::pair<std::string, std::string>> attrsClass;
    bool attrsClassInheritAll = false;

    HTMLAttrsMap attrsResolved;
    bool attrsResolvedInited = false;
    const std::string* findClassAttr(const char* name) const
    {
        for (const auto& kv : attrsClass)
            if (kv.first == name)
                return &kv.second;
        return nullptr;
    }

    const std::string* findEffectiveAttr(const char* name) const
    {
        if (auto p = attrs.find(name))
            return p;
        return findClassAttr(name);
    }
    std::vector<std::shared_ptr<HTMLDomNode>> children;
    ClickHandler onClick;
    HTMLPadding padding;
    HTMLPadding margin{ 0.f, 0.f, 0.f, 0.f };
    bool positionRelative = false;
    float relX = 0.f;
    float relY = 0.f;
    float hrWidth = -1.f;
    bool imgInline = false;
    eVAlign imgValign = eVAlign::MIDDLE;
    bool spanBold = false;
    bool spanItalic = false;

    Rect editLocalRect;
    std::string editValue;
    int editCursor = 0;
    int editSelAnchor = 0;
    float editScrollX = 0.f;
    int editMaxLen = 0;
    bool editInited = false;
    bool editPassword = false;
    std::string editPlaceholder;
    unsigned int editPlaceholderColor = 0x808080FF;
    float reqWidth = 0.f;
    float reqHeight = 0.f;
    bool keepAspect = false;
    float sliceA = 0.f;
    float sliceB = 0.f;
    float sliceC = 0.f;
    float sliceD = 0.f;
    bool sliceVertical = false;
    int sliceMirror = 0;
    CTexturePtr spriteTex;
    float spriteUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float spriteSrcW = 0.f;
    float spriteSrcH = 0.f;
    eHTMLIdleStyle idleStyle = eHTMLIdleStyle::None;
    float idleDelay = -1.f;
    float idleDur = -1.f;
    float idleTimeOffset = 0.f;
    bool imgAnimCompletedSent = false;
    float glareWidth = 40.f;
    float glareSpeed = 150.f;
    float glareTilt = 20.f;
    int glareDir = 0;
    float glareIntensity = 0.7f;
    float pxBlock = 10.f;
    float pxBand = 56.f;
    float pxSpeed = 260.f;
    float pxGlow = 1.f;
#ifdef HAS_SPINE
    unsigned long long spineId = 0;
    std::string spinePath;
    std::string spineAnim;
    bool spineLoop = true;
    float spineDelay = 0.f;
    bool spineHasRcBox = false;
    float spineRc[4] = { 0.f, 0.f, 0.f, 0.f };
#endif
    float hrThickness = 1.f;
    unsigned int hrColor = 0xFFFFFFFF;
    float hrMargin = -1.f;
    HTMLFontAttrs fontAttrs;
    bool lineHasColor = false;
    unsigned int lineColor = 0xFFFFFFFF;
    float lineThickness = 0.f;
    unsigned int bgColor = 0;

    bool hasBodyBg = false;
    float bodyBgOpacity = 1.f;
    float bodyMinWidth = 0.f;
    float bodyMinHeight = 0.f;
    int bodyBgSize = 0;

    std::string particlesPreset;
    float particlesPrewarm = 0.f;
    bool hasAlign = false;
    int cellAlign = FONS_ALIGN_LEFT;
    bool hasVAlign = false;
    eVAlign cellVAlign = eVAlign::TOP;
    float cellMaxWidth = 0.f;
    bool hasOwnPadding = false;
    HTMLPadding cellPadding;
    HTMLDomTableConfig tableConfig;
    bool clickable = false;
    bool visible = true;
    std::string cursor = "normal";
    bool cursorExplicit = false;
    CTexturePtr hoverTex;
    float hoverUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float hoverSrcW = 0.f;
    float hoverSrcH = 0.f;
    CTexturePtr pressedTex;
    float pressedUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float pressedSrcW = 0.f;
    float pressedSrcH = 0.f;
    bool hasHoverSprite = false;
    bool hasPressedSprite = false;
    bool hasHoverColor = false;
    bool hasPressedColor = false;
    unsigned int hoverColor = 0xFFFFFFFF;
    unsigned int pressedColor = 0xFFFFFFFF;
    unsigned int hoverOverlayColor = 0xFFFFFF33;
    unsigned int pressedOverlayColor = 0x00000055;

    int contentAlignH = FONS_ALIGN_CENTER;
    eVAlign contentAlignV = eVAlign::MIDDLE;
    std::string btnGroup;
    bool checked = false;
    bool hasCheckboxUncheckedSprite = false;
    bool hasCheckboxCheckedSprite = false;
    CTexturePtr checkboxUncheckedTex;
    float checkboxUncheckedUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float checkboxUncheckedSrcW = 0.f;
    float checkboxUncheckedSrcH = 0.f;
    CTexturePtr checkboxCheckedTex;
    float checkboxCheckedUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float checkboxCheckedSrcW = 0.f;
    float checkboxCheckedSrcH = 0.f;

    std::string selectFramePath;
    std::string selectBoxPath;
    std::string selectMarkerPath;
    std::string selectIconPath;
    Rect selectLocalRect;
    float selectLocalItemHeight = 0.f;
    int selectMaxPopupRows = 8;
    float selectItemHeight = 0.f;
    float selectItemPadY = 2.f;
    float selectIconGap = 4.f;
    int selectedIndex = -1;
    CTexturePtr selectBoxTex;
    float selectBoxUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float selectBoxSrcW = 0.f;
    float selectBoxSrcH = 0.f;
    float selectBoxA = 0.f;
    float selectBoxB = 0.f;
    bool selectBoxVertical = false;
    int selectBoxMirror = 0;
    CTexturePtr selectMarkerTex;
    float selectMarkerUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float selectMarkerSrcW = 0.f;
    float selectMarkerSrcH = 0.f;
    float selectMarkerA = 0.f;
    float selectMarkerB = 0.f;
    bool selectMarkerVertical = false;
    int selectMarkerMirror = 0;
    CTexturePtr selectIconTex;
    float selectIconUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float selectIconSrcW = 0.f;
    float selectIconSrcH = 0.f;
    float selectIconW = 0.f;
    float selectIconH = 0.f;
    std::string selectValue;
    bool optionSelected = false;
    bool optionEnabled = true;

    bool disabled = false;
    bool hasDisabledTint = false;
    unsigned int disabledTint = 0xFFFFFFFF;

    mutable bool runtimeFadeCached = false;
    mutable float hoverFadeDurCache = 0.12f;
    mutable float pressedFadeDurCache = 0.08f;
    mutable bool runtimeCheckboxCached = false;
    mutable float checkboxSpacingCache = 4.f;
    mutable float checkboxCheckFadeDurCache = 0.12f;
    mutable bool runtimeListCached = false;
    mutable float ulIndentCache = 16.f;
    mutable std::string liBulletCache;
    std::string innerHTML;

    float visOffX = 0.f;
    float visOffY = 0.f;
    float visScaleX = 1.f;
    float visScaleY = 1.f;
    float visRot = 0.f;
    float visPivotX = 0.f;
    float visPivotY = 0.f;
    bool visPivotSet = false;
    float visAlpha = 1.f;
    Rect visLastRect;
    unsigned long long tweenGen = 0;

    float sliderMin = 0.f;
    float sliderMax = 1.f;
    float sliderValue = 0.f;
    float sliderStep = 0.f;
    bool sliderVertical = false;
    float sliderTrackThickness = 6.f;
    bool sliderFill = true;
    std::string sliderGripPath;
    CTexturePtr sliderGripTex;
    float sliderGripUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float sliderGripSrcW = 0.f;
    float sliderGripSrcH = 0.f;
    float sliderGripW = 16.f;
    float sliderGripH = 16.f;
    bool sliderShowTicks = false;
    bool sliderTicksAuto = false;
    int sliderMajorCount = 10;
    int sliderMinorCount = 4;
    float sliderTickLength = 0.f;
    float sliderMinorTickLength = 0.f;
    float sliderTickThickness = 0.f;
    float sliderTickOffset = 0.f;
    unsigned int sliderTrackColor = 0x808080FF;
    unsigned int sliderFillColor = 0x4DA6FFFF;
    unsigned int sliderGripColor = 0xFFFFFFFF;
    unsigned int sliderTickColor = 0xFFFFFFFF;
    unsigned int sliderMinorTickColor = 0x808080FF;
    Rect sliderLocalRect;
    bool sliderSnapToMajorTicks = false;
    std::string sliderFillPath;
    CTexturePtr sliderFillTex;
    float sliderFillUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float sliderFillSrcW = 0.f;
    float sliderFillSrcH = 0.f;
    float sliderFillA = 0.f;
    float sliderFillB = 0.f;
    float sliderFillC = 0.f;
    float sliderFillD = 0.f;
    bool sliderFillThree = false;
    bool sliderFillVertical = false;
    int sliderFillMirror = 0;

    float progressT = 0.f;
    bool progressVertical = false;
    Rect progressLocalRect;
    std::string progressFramePath;
    CTexturePtr progressFrameTex;
    float progressFrameUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float progressFrameSrcW = 0.f;
    float progressFrameSrcH = 0.f;
    float progressFrameA = 0.f;
    float progressFrameB = 0.f;
    float progressFrameC = 0.f;
    float progressFrameD = 0.f;
    bool progressFrameThree = true;
    bool progressFrameVertical = false;
    int progressFrameMirror = 0;
    std::string progressFillPath;
    CTexturePtr progressFillTex;
    float progressFillUV[4] = { 0.f, 0.f, 0.f, 0.f };
    float progressFillSrcW = 0.f;
    float progressFillSrcH = 0.f;
    float progressFillA = 0.f;
    float progressFillB = 0.f;
    float progressFillC = 0.f;
    float progressFillD = 0.f;
    bool progressFillThree = true;
    bool progressFillVertical = false;
    int progressFillMirror = 0;
    bool progressHasBackColor = false;
    unsigned int progressBackColor = 0x000000AA;
    unsigned int progressFillColor = 0x4DA6FFFF;
    unsigned int progressBorderColor = 0xFFFFFFFF;
    float progressBorder = -1.f;
};
struct HTMLDomOptions
{
    int align = FONS_ALIGN_LEFT | FONS_ALIGN_TOP;
    unsigned int color = 0xFFFFFFFF;
    unsigned int documentColor = 0xFFFFFFFF;
    float appearStart = -1.f;
    unsigned long long animId = 0;
    float maxHeight = 0.f;
};
struct HTMLSelectOptionDesc
{
    std::string label;
    std::string value;
    std::string html;
    std::string fontName;
    int fontSize = 0;
    unsigned int color = 0xFFFFFFFF;
    bool selected = false;
    bool enabled = true;
    float height = 0.f;
    unsigned int disabledTint = 0xFFFFFFFF;
};
struct HTMLSelectStyle
{
    std::string frameSrc;
    float frameA = 0.f;
    float frameB = 0.f;
    float frameC = 0.f;
    float frameD = 0.f;
    std::string markerSrc;
    float markerA = 0.f;
    float markerB = 0.f;
    bool markerVertical = false;
    int markerMirror = 0;
    std::string fontName;
    int fontSize = 0;
    unsigned int color = 0xFFFFFFFF;
    float itemHeight = 0.f;
    float itemPadY = 2.f;
    float contentPad = 4.f;
    int maxPopupRows = 8;
    float minWidth = 0.f;
    std::string popupSelectedSrc;
    std::string popupHoverSrc;
    std::string popupPressedSrc;
    unsigned int popupHoverColor = 0xFFFFFFFF;
    unsigned int popupPressedColor = 0xFFFFFFFF;
    float popupBgTint = 1.f;
    float popupBgAlpha = 1.f;
    float popupPad = 0.f;
    float popupWidth = 0.f;
    float popupRowA = 0.f;
    float popupRowB = 0.f;
    float popupRowC = 0.f;
    float popupRowD = 0.f;
    float itemPad = 2.f;
    bool disabled = false;
    unsigned int disabledTint = 0xFFFFFFFF;
    unsigned int documentColor = 0xFFFFFFFF;
};
struct HTMLSelectView
{
    unsigned long long selectId = 0;
    Rect localRect;
    int selectedIndex = -1;
    std::vector<HTMLSelectOptionDesc> options;
    HTMLSelectStyle style;
};
#ifdef HAS_SPINE
typedef std::function<void(
    unsigned long long rootId,
    unsigned long long nodeId,
    eHTMLSpineEventType type,
    const char* animName,
    const char* eventName)> FOnSpineAnimEvent;
#endif
typedef std::function<void(
    unsigned long long rootId,
    unsigned long long nodeId,
    eHTMLImageAnimEventType type,
    eHTMLIdleStyle idleStyle)> FOnImageAnimEvent;
typedef std::function<void(
    unsigned long long rootId,
    unsigned long long nodeId,
    eHTMLFontAnimEventType type,
    eFxStyle fxStyle,
    eAppearStyle appearStyle)> FOnFontAnimEvent;
class IHTMLDomCallbacks
{
private:
#ifdef HAS_SPINE
    FOnSpineAnimEvent   _cbSpineAnimEvent;
#endif
    FOnImageAnimEvent   _cbImageAnimEvent;
    FOnFontAnimEvent    _cbFontAnimEvent;
public:
#ifdef HAS_SPINE
    void setSpineAnimEventCb(FOnSpineAnimEvent cb) { _cbSpineAnimEvent = cb; }
#endif
    void setImageAnimEventCb(FOnImageAnimEvent cb) { _cbImageAnimEvent = cb; }
    void setFontAnimEventCb(FOnFontAnimEvent cb) { _cbFontAnimEvent = cb; }
    virtual ~IHTMLDomCallbacks() {}
    virtual void onCursorChanged(const char* szNewCursor) {}
    virtual void onHover(unsigned long long rootId, unsigned long long nodeId) {}
    virtual void onLeave(unsigned long long rootId, unsigned long long nodeId) {}
    virtual bool onSelectDropdownRequested(unsigned long long rootId, unsigned long long selectId)
    {
        (void)rootId; (void)selectId;
        return false;
    }
    virtual void onSelectChanged(unsigned long long rootId, unsigned long long selectId,
        unsigned long long optionId, int index, const char* value)
    {
        (void)rootId; (void)selectId; (void)optionId; (void)index; (void)value;
    }
    virtual void onDrag(unsigned long long rootId, unsigned long long nodeId, bool bEnd)
    {
        (void)rootId; (void)nodeId; (void)bEnd;
    }
    virtual void onDragAny(unsigned long long rootId, unsigned long long nodeId, bool bEnd)
    {
        (void)rootId; (void)nodeId; (void)bEnd;
    }
    virtual void onSliderChanged(unsigned long long rootId, unsigned long long nodeId, float value)
    {
        (void)rootId; (void)nodeId; (void)value;
    }
    virtual void onEditFocusChanged(unsigned long long rootId, unsigned long long editId, bool bFocused)
    {
        (void)rootId; (void)editId; (void)bFocused;
    }
    virtual void onEditChanged(unsigned long long rootId, unsigned long long editId, const char* value)
    {
        (void)rootId; (void)editId; (void)value;
    }
    virtual void onEditSubmit(unsigned long long rootId, unsigned long long editId, const char* value)
    {
        (void)rootId; (void)editId; (void)value;
    }
    virtual float getDocumentScale(unsigned long long rootId)
    {
        (void)rootId;
        return 1.f;
    }
#ifdef HAS_SPINE
    virtual void onSpineAnimEvent(unsigned long long rootId, unsigned long long nodeId,
        eHTMLSpineEventType type, const char* animName, const char* eventName)
    {
        if (_cbSpineAnimEvent)
            _cbSpineAnimEvent(rootId, nodeId, type, animName, eventName);
    }
#endif
    virtual void onImageAnimEvent(unsigned long long rootId, unsigned long long nodeId,
        eHTMLImageAnimEventType type, eHTMLIdleStyle idleStyle)
    {
        if (_cbImageAnimEvent)
            _cbImageAnimEvent(rootId, nodeId, type, idleStyle);
    }
    virtual void onFontAnimEvent(unsigned long long rootId, unsigned long long nodeId,
        eHTMLFontAnimEventType type, eFxStyle fxStyle, eAppearStyle appearStyle)
    {
        if (_cbFontAnimEvent)
            _cbFontAnimEvent(rootId, nodeId, type, fxStyle, appearStyle);
    }
};
class HTMLDom
{
public:
    using NodePtr = std::shared_ptr<HTMLDomNode>;
    static HTMLDom* getInstance(void);
    unsigned long long parse(const char* lpszHTML);
    void setRootOptions(unsigned long long rootId, const HTMLDomOptions& options);
    HTMLDomOptions getRootOptions(unsigned long long rootId) const;
    HTMLDomOptions* getRootOptionsPtr(unsigned long long rootId);
    void setDocumentCallbacks(unsigned long long rootId, IHTMLDomCallbacks* callbacks);
    Rect render(unsigned long long rootId, float maxWidth);
    Rect measure(unsigned long long rootId, float maxWidth);
    NodePtr getNode(unsigned long long id);
    unsigned long long getParent(unsigned long long id) const;
    std::vector<unsigned long long> getChildren(unsigned long long id) const;
    unsigned long long createNode(eHTMLTag tag, unsigned long long parentId, int index = -1);
    std::vector<unsigned long long> insertHTML(unsigned long long parentId, const char* html, int index = -1);
    bool removeNode(unsigned long long id);
    bool moveNode(unsigned long long id, unsigned long long newParentId, int index = -1);
    bool setTextContent(unsigned long long id, const char* text);
    bool setInnerHTML(unsigned long long id, const char* html);
    bool setAttr(unsigned long long id, const char* name, const char* value);
    const char* getAttr(unsigned long long id, const char* name);
    bool removeAttr(unsigned long long id, const char* name);
    NodePtr findNodeByAttr(unsigned long long rootId, const char* attrName, const char* value);
    unsigned long long findNodeIdByAttr(unsigned long long rootId, const char* attrName, const char* value);
    NodePtr getElementById(unsigned long long rootId, const char* id);
    unsigned long long getElementIdById(unsigned long long rootId, const char* id);
    bool setOnClick(unsigned long long id, HTMLDomNode::ClickHandler handler);
    void handleInput(eHTMLInputEvent evt, float x, float y, int button = 1);
    void handleInput(unsigned long long rootId, eHTMLInputEvent evt, float x, float y, int button = 1);
    void handleMouseLeave(unsigned long long rootId = 0);
    void clearInputState();
    unsigned long long getHoverTargetId() const { return _hoverTargetId; }
    bool isMouseButtonDown() const { return _mouseButtonDown; }
    int getPressedMouseButton() const { return _pressedButton; }
    unsigned long long hitTest(float x, float y) const;
    unsigned long long hitTest(unsigned long long rootId, float x, float y) const;
    void restartAppear(unsigned long long id);
    void removeAppearState(unsigned long long id);
    void pauseAnimsForHTML(unsigned long long id, bool bIsPaused = true);
    void clearAppearStates(void);
    void setPaused(bool isPaused = true) { _isPaused = isPaused; }
    void update(float dt)
    {
        if (_isPaused)
            return;
        _fAnimTime += dt;
        for (auto& kv : _particlesSystems)
        {
            auto node = lockNode(kv.first);
            if (node && node->visible && kv.second)
                kv.second->update(dt);
        }
        updateTweens(dt);
    }
    float getAnimTime(void) const { return _fAnimTime; }
    bool isVisible(unsigned long long nodeId) const;
    bool setVisible(unsigned long long nodeId, bool visible);
    bool isDisabled(unsigned long long nodeId) const;
    bool setDisabled(unsigned long long nodeId, bool disabled);
    unsigned int getDisabledTint(unsigned long long nodeId) const;
    bool setDisabledTint(unsigned long long nodeId, unsigned int tint);
    bool isChecked(unsigned long long nodeId) const;
    bool setChecked(unsigned long long nodeId, bool checked);
    bool toggleChecked(unsigned long long nodeId);
#ifdef HAS_SPINE
    bool spineSetAnimation(unsigned long long nodeId, size_t trackIndex, const char* animName, bool loop);
    bool spineAddAnimation(unsigned long long nodeId, size_t trackIndex, const char* animName, bool loop, float delay = 0.f);
    bool spineSetTimeScale(unsigned long long nodeId, float timeScale);
    bool spineStopAnimation(unsigned long long nodeId);
    bool spineSetPaused(unsigned long long nodeId, bool paused = true);
    bool spineRestartAnimation(unsigned long long nodeId);
#endif
    bool setImageAnimation(unsigned long long nodeId, eHTMLIdleStyle style);
    bool stopImageAnimation(unsigned long long nodeId);
    bool restartImageAnimation(unsigned long long nodeId);
    bool setFontAppearAnimation(unsigned long long nodeId, eAppearStyle style);
    bool setFontFxAnimation(unsigned long long nodeId, eFxStyle style);
    bool stopFontAnimation(unsigned long long nodeId);
    bool restartFontAnimation(unsigned long long nodeId);

    void parseCSSFromFile(const char* lpszCSSFileName);
    void parseCSS(const char* lpszCSS);
    void clearCSS();

    bool addCssClass(const char* lpszName, const char* lpszDecls);
    bool removeCssClass(const char* lpszName);
    bool hasCssClass(const char* lpszName) const;

    void setHostContainer(unsigned long long rootId, std::weak_ptr<CContainer> host);
    std::weak_ptr<CContainer> getHostContainer(unsigned long long rootId) const;
    bool getSelectView(unsigned long long selectId, HTMLSelectView& out) const;
    bool getSelectScreenRect(unsigned long long selectId, Rect& outScreen) const;
    bool selectOptionByIndex(unsigned long long selectId, int index, bool fireCallback);
    float getDocumentScale(unsigned long long rootId) const;
    unsigned long long getRootIdForNode(unsigned long long nodeId) const;

    float getSliderValue(unsigned long long nodeId) const;
    bool setSliderValue(unsigned long long nodeId, float value, bool fireCallback = false);

    float getProgressValue(unsigned long long nodeId) const;
    bool setProgressValue(unsigned long long nodeId, float value01);
    float getProgressPercent(unsigned long long nodeId) const;
    bool setProgressPercent(unsigned long long nodeId, float percent);

    typedef std::function<std::shared_ptr<CContainer>(void)> FParticlesPresetFactory;
    void registerParticlesPreset(const char* lpszName, FParticlesPresetFactory factory);
    void unregisterParticlesPreset(const char* lpszName);
    bool hasParticlesPreset(const char* lpszName) const;
    std::shared_ptr<CContainer> getParticlesSystem(unsigned long long nodeId);

    unsigned long long tweenTo(unsigned long long nodeId, const HTMLTweenProps& props, float dur, const HTMLTweenOptions& opts = {});
    unsigned long long tweenFrom(unsigned long long nodeId, const HTMLTweenProps& props, float dur, const HTMLTweenOptions& opts = {});
    bool killTween(unsigned long long tweenId);
    bool killTweensOf(unsigned long long nodeId);
    void killAllTweens();
    void setTweensPaused(bool paused);
    void pauseTweensForRoot(unsigned long long rootId, bool paused = true);
    int getActiveTweenCount() const { return (int)_tweens.size(); }

    void handleKeyboardInput(const char* szTextAdd, eSpecialKey eKey, int modifiers = 0);
    bool setEditValue(unsigned long long nodeId, const char* value, bool fireCallback = false);
    const char* getEditValue(unsigned long long nodeId) const;
    bool focusEdit(unsigned long long nodeId, bool focused = true);
    bool isEditFocused(unsigned long long nodeId) const;
    unsigned long long getFocusedEditId() const { return _focusedEditId; }
    bool setEditCursor(unsigned long long nodeId, int bytePos);
    void blurFocusedEdit();
private:
    ~HTMLDom();
    struct LayoutParams;
    struct RenderContext;
    friend struct RenderContext;
    struct HitEntry
    {
        unsigned long long nodeId = 0;
        Rect rc;
    };
    struct FxAnimState
    {
        float fBlockStart = 0.f;
        float fAnimTime = 0.f;
        bool bIsPaused = false;
        bool bIsInitiated = false;
        FxAnimState() = default;
        FxAnimState(float blockStart, float animTime, bool paused, bool initiated)
            : fBlockStart(blockStart), fAnimTime(animTime), bIsPaused(paused), bIsInitiated(initiated) {}
    };
#ifdef HAS_SPINE
    struct SpineRuntimeState
    {
        std::shared_ptr<CSpine> p;
        unsigned long long blockKey = 0;
        unsigned long long nodeId = 0;
        unsigned long long rootId = 0;
        bool animSet = false;
        bool callbacksSet = false;
        float startTime = 0.f;
        float lastTime = 0.f;
        std::string lastAnim;
        bool lastLoop = true;
    };
#endif
    struct FontAnimEventFlags
    {
        unsigned long long rootId = 0;
        bool appearStart = false;
        bool appearComplete = false;
        bool fxStart = false;
        bool fxComplete = false;
        bool stopped = false;
    };

    HTMLRenderScratch* ensureScratch();
    int internRenderState(const HTMLRenderState& st);
    unsigned long long newId(void);
    NodePtr lockNode(unsigned long long id) const;
    void registerNodeRecursive(const NodePtr& node);
    void unregisterNodeRecursive(const NodePtr& node);
    void attachNode(const NodePtr& node, const NodePtr& parent, int index);
    void detachNode(const NodePtr& node);
    NodePtr parseFragmentInternal(const char* lpszHTML, unsigned long long parentId);
    void refreshNode(HTMLDomNode& node);
    void clearHitData(unsigned long long rootId);
    void registerHitRect(unsigned long long rootId, unsigned long long nodeId, float x, float y, float w, float h);
    void registerHitRectAt(unsigned long long rootId, unsigned long long nodeId, float x, float y, float w, float h, size_t insertIndex);
    unsigned long long hitTestInternal(unsigned long long rootId, float x, float y) const;
    unsigned long long findClickTarget(unsigned long long rootId, float x, float y) const;
    void handleMouseEvent(unsigned long long rootId, eHTMLInputEvent evt, float x, float y, int button);
    IHTMLDomCallbacks* getDocumentCallbacks(unsigned long long rootId) const;
    unsigned long long findCursorTarget(unsigned long long rootId, float x, float y) const;
    bool isCursorTrackedNode(const NodePtr& node) const;
    std::string getNodeCursorName(unsigned long long nodeId) const;
    void updateCursorHover(unsigned long long rootId, unsigned long long newCursorTargetId);
    void refreshCurrentCursor();
#ifdef HAS_SPINE
    static unsigned long long makeSpineKey(unsigned long long blockKey, unsigned long long spineId);
    void restartSpinesForBlock(unsigned long long blockKey);
    void removeSpinesForBlock(unsigned long long blockKey);
#endif
    Rect renderInternal(unsigned long long rootId, float maxWidth, bool bMeasureOnly);

    void renderSequence(const std::vector<NodePtr>& children, HTMLRenderState& state,
        const LayoutParams& params, Rect* rcOut, bool bMeasureOnly, unsigned long long rootId);
    void renderChildren(const NodePtr& parent, HTMLRenderState& state, RenderContext& ctx);
    void renderNode(const NodePtr& node, HTMLRenderState& state, RenderContext& ctx);
    void applyFontAttrs(HTMLRenderState& st, const HTMLFontAttrs& attrs);
    NodePtr findNodeByAttrRecursive(const NodePtr& node, const char* attrName, const char* value) const;
    bool isInteractiveNode(const NodePtr& node) const;
#ifdef HAS_SPINE
    SpineRuntimeState* findSpineRuntimeState(unsigned long long nodeId);
#endif
    unsigned long long findRootIdForNode(const NodePtr& node) const;
    void resolveOptionFontAttrsInto(const HTMLDomNode* opt, unsigned long long selectId, HTMLFontAttrs& out) const;
    void updateSliderFromPointer(const NodePtr& node, float x, float y, bool fireChanged);
    void endSliderDrag(bool fireCallback);
    void beginEditDrag(unsigned long long rootId, unsigned long long nodeId);
    void stopEditDrag(bool fireCallback);

    void updateTweens(float dt);
    static void tweenReadProp(HTMLDomNode& node, eHTMLTweenProp prop, float& f, unsigned int& c);
    static void tweenApplyProp(HTMLDomNode& node, const HTMLTweenChannel& ch, float k);

    struct VisMat
    {
        float m[6] = { 1.f, 0.f, 0.f, 0.f, 1.f, 0.f };
    };
    void applyVisMatToRect(float& x, float& y, float& w, float& h) const;
    void ensureRawNodeCache(const NodePtr& node);

    std::unordered_map<std::string, HTMLCssClassRule> _cssClasses;
    std::vector<std::string> _cssClassOrder;
    void resolveClassAttrsRecursive(const NodePtr& node);
    void resolveNodeClassAttrs(HTMLDomNode& node);
    void rebuildAllCssAttrs();
    static bool cssPropInherits(const std::string& name);
    static bool cssClassListContains(const std::string& list, const std::string& name);
private:
    struct DomButtonFxState
    {
        float hover = 0.f;
        float pressed = 0.f;
        float checked = 0.f;
        float lastTime = -1.f;
    };
    unsigned long long _nextNodeId = 1;
    unsigned long long _lastRootId = 0;
    float _fAnimTime = 0.f;
    bool _isPaused = false;
    RenderContext* _currentRenderContext = nullptr;
    std::vector<HTMLDomNode*> _nodeRawById;
    int _btnGroupCount = 0;

    HTMLRenderScratch* _scratch = nullptr;
    std::unordered_map<unsigned long long, NodePtr> _roots;
    std::unordered_map<unsigned long long, std::weak_ptr<HTMLDomNode>> _nodes;
    std::unordered_map<unsigned long long, HTMLDomOptions> _rootOptions;
    std::unordered_map<unsigned long long, std::vector<HitEntry>> _hitData;
    std::unordered_map<unsigned long long, FxAnimState> _appearStarts;
#ifdef HAS_SPINE
    std::unordered_map<unsigned long long, SpineRuntimeState> _spineStates;
#endif
    std::unordered_map<unsigned long long, IHTMLDomCallbacks*> _rootCallbacks;
#ifdef HAS_SPINE
    std::unordered_map<unsigned long long, unsigned long long> _spineKeyByNode;
#endif
    std::unordered_map<unsigned long long, FontAnimEventFlags> _fontAnimEventFlags;
    std::unordered_map<unsigned long long, std::weak_ptr<CContainer>> _hostContainers;
    std::unordered_map<unsigned long long, DomButtonFxState> _domButtonFxMap;
    std::unordered_map<unsigned long long, DomButtonFxState> _domFgFxMap;
    std::unordered_map<std::string, FParticlesPresetFactory> _particlesPresets;
    std::unordered_map<unsigned long long, std::shared_ptr<CContainer>> _particlesSystems;
    unsigned long long _pressedRootId = 0;
    unsigned long long _pressedTargetId = 0;
    unsigned long long _hoverTargetId = 0;
    unsigned long long _hoverRootId = 0;
    unsigned long long _cursorRootId = 0;
    unsigned long long _cursorTargetId = 0;
    std::string _currentCursor = "normal";
    int _pressedButton = 0;
    bool _mouseButtonDown = false;
    float _lastMouseX = 0.f;
    float _lastMouseY = 0.f;
    unsigned long long _fgHoverTargetId = 0;
    unsigned long long _fgPressedTargetId = 0;
    unsigned long long _dragSliderId = 0;
    unsigned long long _dragRootId = 0;
    unsigned long long _focusedEditId = 0;
    unsigned long long _focusedEditRootId = 0;
    unsigned long long _editDragId = 0;
    unsigned long long _editDragRootId = 0;
    unsigned long long _nextTweenId = 1;
    std::vector<HTMLTween> _tweens;
    bool _tweensPaused = false;
    std::unordered_set<unsigned long long> _pausedTweenRoots;
    std::vector<VisMat> _visMatStack;
    std::vector<float> _extraAlphaStack;
    bool _fgFadeActive = false;
    float _fgFadeHover = 0.f;
    float _fgFadePressed = 0.f;
    unsigned int _fgFadeHoverColor = 0xFFFFFFFF;
    unsigned int _fgFadePressedColor = 0xFFFFFFFF;
};
_G2D_NAMESPACE_END_
