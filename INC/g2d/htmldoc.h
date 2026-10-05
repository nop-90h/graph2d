#pragma once
#include "htmldom.h"
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

_G2D_NAMESPACE_BEGIN_

class CHTMLDocument;
class CHTMLElement;

class CHTMLDocumentCallbacks : public IHTMLDomCallbacks
{
public:
    using ClickHandler = HTMLDomNode::ClickHandler;

    CHTMLDocumentCallbacks() = default;
    ~CHTMLDocumentCallbacks() override;

    CHTMLDocumentCallbacks(const CHTMLDocumentCallbacks&) = delete;
    CHTMLDocumentCallbacks& operator=(const CHTMLDocumentCallbacks&) = delete;

    struct NodeHandlers
    {
        bool clickBridgeInstalled = false;

        ClickHandler click;

        std::function<void()> enter;
        std::function<void()> leave;
        std::function<void()> dropdown;

        std::function<void(int, const char*)> selectChanged;
        std::function<void(float)> sliderChanged;
        std::function<void(bool)> drag;

#ifdef HAS_SPINE
        std::function<void(eHTMLSpineEventType, const char*, const char*)> spine;
#endif
        std::function<void(eHTMLImageAnimEventType)> image;
        std::function<void(eHTMLFontAnimEventType, eFxStyle, eAppearStyle)> font;

        std::function<void(bool)> editFocus;
        std::function<void(const char*)> editChanged;
        std::function<void(const char*)> editSubmit;
    };

    NodeHandlers* getHandlers(unsigned long long nodeId);

    bool addClickHandler(unsigned long long nodeId, ClickHandler handler);
    bool unbindNode(unsigned long long nodeId);

    bool addEnterHandler(unsigned long long nodeId, std::function<void()> fn);
    bool addLeaveHandler(unsigned long long nodeId, std::function<void()> fn);
    bool addDropdownHandler(unsigned long long nodeId, std::function<void()> fn);

    bool addSelectChangedHandler(unsigned long long nodeId, std::function<void(int, const char*)> fn);
    bool addSliderChangedHandler(unsigned long long nodeId, std::function<void(float)> fn);
    bool addDragHandler(unsigned long long nodeId, std::function<void(bool)> fn);

#ifdef HAS_SPINE
    bool addSpineHandler(unsigned long long nodeId,
        std::function<void(eHTMLSpineEventType, const char*, const char*)> fn);
#endif

    bool addImageHandler(unsigned long long nodeId,
        std::function<void(eHTMLImageAnimEventType)> fn);

    bool addFontHandler(unsigned long long nodeId,
        std::function<void(eHTMLFontAnimEventType, eFxStyle, eAppearStyle)> fn);

    bool addEditFocusHandler(unsigned long long nodeId, std::function<void(bool)> fn);
    bool addEditChangedHandler(unsigned long long nodeId, std::function<void(const char*)> fn);
    bool addEditSubmitHandler(unsigned long long nodeId, std::function<void(const char*)> fn);

    bool addAnyHoverHandler(std::function<void(unsigned long long)> enter,
                            std::function<void(unsigned long long)> leave = {});

    bool addAnySelectChangedHandler(std::function<void(unsigned long long, int, const char*)> fn);
    bool addAnySliderChangedHandler(std::function<void(unsigned long long, float)> fn);
    bool addAnyDragHandler(std::function<void(unsigned long long, bool)> fn);
    bool addAnyDropdownHandler(std::function<void(unsigned long long)> fn);

    bool addAnyEditFocusHandler(std::function<void(unsigned long long, bool)> fn);
    bool addAnyEditChangedHandler(std::function<void(unsigned long long, const char*)> fn);
    bool addAnyEditSubmitHandler(std::function<void(unsigned long long, const char*)> fn);

    void setCursorChangedHandler(std::function<void(const char*)> fn);
    void setDocumentScaleHandler(std::function<float()> fn);

    void clearClickBridges();
    void reset();

    void onCursorChanged(const char* szNewCursor) override;

    void onHover(unsigned long long rootId, unsigned long long nodeId) override;
    void onLeave(unsigned long long rootId, unsigned long long nodeId) override;

    bool onSelectDropdownRequested(unsigned long long rootId, unsigned long long selectId) override;

    void onSelectChanged(unsigned long long rootId,
                         unsigned long long selectId,
                         unsigned long long optionId,
                         int index,
                         const char* value) override;

    void onDrag(unsigned long long rootId, unsigned long long nodeId, bool bEnd) override;

    void onSliderChanged(unsigned long long rootId, unsigned long long nodeId, float value) override;

    void onEditFocusChanged(unsigned long long rootId, unsigned long long editId, bool bFocused) override;
    void onEditChanged(unsigned long long rootId, unsigned long long editId, const char* value) override;
    void onEditSubmit(unsigned long long rootId, unsigned long long editId, const char* value) override;
    void onDragAny(unsigned long long rootId, unsigned long long nodeId, bool bEnd) override;

    float getDocumentScale(unsigned long long rootId) override;

#ifdef HAS_SPINE
    void onSpineAnimEvent(unsigned long long rootId,
                          unsigned long long nodeId,
                          eHTMLSpineEventType type,
                          const char* animName,
                          const char* eventName) override;
#endif

    void onImageAnimEvent(unsigned long long rootId,
                          unsigned long long nodeId,
                          eHTMLImageAnimEventType type,
                          eHTMLIdleStyle idleStyle) override;

    void onFontAnimEvent(unsigned long long rootId,
                         unsigned long long nodeId,
                         eHTMLFontAnimEventType type,
                         eFxStyle fxStyle,
                         eAppearStyle appearStyle) override;

private:
    void dispatchClick(unsigned long long nodeId, float x, float y, int button);
    void installClickBridge(unsigned long long nodeId);

private:
    std::unordered_map<unsigned long long, NodeHandlers> _handlers;
    std::unordered_set<unsigned long long> _clickBridges;

    std::function<void(const char*)> _cursorChanged;

    std::function<void(unsigned long long)> _anyHover;
    std::function<void(unsigned long long)> _anyLeave;
    std::function<void(unsigned long long)> _anyDropdown;

    std::function<void(unsigned long long, int, const char*)> _anySelectChanged;
    std::function<void(unsigned long long, float)> _anySliderChanged;
    std::function<void(unsigned long long, bool)> _anyDrag;

    std::function<void(unsigned long long, bool)> _anyEditFocus;
    std::function<void(unsigned long long, const char*)> _anyEditChanged;
    std::function<void(unsigned long long, const char*)> _anyEditSubmit;

    std::function<float()> _documentScale;
};

class CHTMLElement
{
public:
    CHTMLElement() = default;

    CHTMLElement(CHTMLDocument* doc, unsigned long long nodeId)
        : _doc(doc), _nodeId(nodeId)
    {
    }

    bool isValid() const;
    explicit operator bool() const { return isValid(); }

    unsigned long long getId() const { return _nodeId; }
    unsigned long long getRootId() const;
    eHTMLTag getTag() const;

    bool setVisible(bool visible);
    bool setDisabled(bool disabled);
    bool setDisabledTint(unsigned int tint);

    bool setAttr(const char* name, const char* value);
    bool setAttr(const char* name, const std::string& value);
    bool setAttr(const char* name, int value);
    bool setAttr(const char* name, float value);
    bool setAttr(const char* name, bool value);
    bool setAttr(const char* name, unsigned int value);

    bool removeAttr(const char* name);
    std::string getAttr(const char* name, const char* def = "") const;

    bool setText(const char* text);
    bool setText(const std::string& text);

    template <typename... Args>
    bool setTextFmt(std::format_string<Args...> fmt, Args&&... args)
    {
        return setText(std::format(fmt, std::forward<Args>(args)...));
    }

    bool onClick(std::function<void()> fn);
    bool onClick(std::function<void(float, float, int)> fn);

    bool isChecked() const;
    bool setChecked(bool checked);
    bool toggleChecked();
    bool onCheckedChanged(std::function<void(bool)> fn);

    bool selectOption(int index, bool fireCallback = false);
    int getSelectedIndex() const;
    bool onSelectChanged(std::function<void(int, const char*)> fn);

    float getSliderValue() const;
    bool setSliderValue(float value, bool fireCallback = false);
    bool onSliderChanged(std::function<void(float)> fn);

    float getProgress() const;
    float getProgressPercent() const;
    bool setProgress(float value01);
    bool setProgressPercent(float percent);

    std::string getEditValue() const;

    bool setEditValue(const char* value, bool fireCallback = false);
    bool setEditValue(const std::string& value, bool fireCallback = false);

    bool focusEdit(bool focused = true);
    bool blurEdit();
    bool isEditFocused() const;

    int getEditCursor() const;
    bool setEditCursor(int bytePos);

    bool onEditFocusChanged(std::function<void(bool)> fn);
    bool onEditChanged(std::function<void(const char*)> fn);
    bool onEditSubmit(std::function<void(const char*)> fn);

#ifdef HAS_SPINE
    bool spinePlay(const char* animName, bool loop = true);
    bool spineAdd(const char* animName, bool loop = true);
    bool spineStop();
    bool spinePause(bool bPaused = true);
    bool spineRestart();
#endif

    bool setIdleAnimation(eHTMLIdleStyle style);
    bool stopIdleAnimation();
    bool restartIdleAnimation();

    bool setAppearAnimation(eAppearStyle style);
    bool setFxAnimation(eFxStyle style);
    bool stopFontAnimation();
    bool restartFontAnimation();

    bool restartAppear();

    unsigned long long tweenTo(const HTMLTweenProps& props, float dur, const HTMLTweenOptions& opts = {});
    unsigned long long tweenFrom(const HTMLTweenProps& props, float dur, const HTMLTweenOptions& opts = {});
    bool killTweens();

private:
    HTMLDom::NodePtr node() const;

    CHTMLDocument* _doc = nullptr;
    unsigned long long _nodeId = 0;
};

class CHTMLDocument
{
public:
    using ClickHandler = HTMLDomNode::ClickHandler;

    CHTMLDocument();
    explicit CHTMLDocument(const char* html);
    explicit CHTMLDocument(unsigned long long rootId, bool bOwns = false);
    ~CHTMLDocument();

    CHTMLDocument(const CHTMLDocument&) = delete;
    CHTMLDocument& operator=(const CHTMLDocument&) = delete;

    CHTMLDocument(CHTMLDocument&& other) noexcept;
    CHTMLDocument& operator=(CHTMLDocument&& other) noexcept;

    unsigned long long getRootId() const { return _rootId; }
    bool ownsRoot() const { return _bOwns; }

    bool isValid() const;
    explicit operator bool() const { return isValid(); }

    void reset(unsigned long long newRootId = 0, bool bOwns = false);

    unsigned long long find(const char* id);
    bool exists(const char* id) { return find(id) != 0; }

    CHTMLElement get(const char* id);
    CHTMLElement operator[](const char* id) { return get(id); }

    std::vector<unsigned long long> insertHTML(const char* targetId, const char* html, int index = -1);
    unsigned long long createElement(eHTMLTag tag, const char* parentId, int index = -1);
    bool remove(const char* id);
    bool move(const char* id, const char* newParentId, int index = -1);

    bool setAttr(const char* id, const char* name, const char* value);
    bool setAttr(const char* id, const char* name, const std::string& value);
    bool setAttr(const char* id, const char* name, int value);
    bool setAttr(const char* id, const char* name, float value);
    bool setAttr(const char* id, const char* name, bool value);
    bool setAttr(const char* id, const char* name, unsigned int value);

    bool removeAttr(const char* id, const char* name);
    std::string getAttr(const char* id, const char* name, const char* def = "");

    bool setText(const char* id, const char* text);
    bool setText(const char* id, const std::string& text);

    template <typename... Args>
    bool setTextFmt(const char* id, std::format_string<Args...> fmt, Args&&... args)
    {
        return setText(id, std::format(fmt, std::forward<Args>(args)...));
    }

    bool bind(const char* id, ClickHandler handler);
    bool unbind(const char* id);

    bool onClick(const char* id, std::function<void()> fn);
    bool onClick(const char* id, std::function<void(float, float, int)> fn);

    bool onHover(const char* id, std::function<void()> enter, std::function<void()> leave = {});

    bool onSelectChanged(const char* id, std::function<void(int, const char*)> fn);
    bool onSliderChanged(const char* id, std::function<void(float)> fn);
    bool onDrag(const char* id, std::function<void(bool)> fn);
    bool onSelectDropdownRequested(const char* id, std::function<void()> fn);

#ifdef HAS_SPINE
    bool onSpineAnimEvent(const char* id,
        std::function<void(eHTMLSpineEventType, const char*, const char*)> fn);
#endif

    bool onImageAnimEvent(const char* id,
        std::function<void(eHTMLImageAnimEventType)> fn);

    bool onFontAnimEvent(const char* id,
        std::function<void(eHTMLFontAnimEventType, eFxStyle, eAppearStyle)> fn);

    bool onHoverAny(std::function<void(unsigned long long nodeId)> enter,
                    std::function<void(unsigned long long nodeId)> leave = {});

    bool onSelectChangedAny(std::function<void(unsigned long long selectId, int index, const char* value)> fn);
    bool onSliderChangedAny(std::function<void(unsigned long long nodeId, float value)> fn);
    bool onDragAny(std::function<void(unsigned long long nodeId, bool bEnd)> fn);
    bool onDropdownRequestedAny(std::function<void(unsigned long long selectId)> fn);

    unsigned long long getFocusedEditId() const;
    CHTMLElement getFocusedEdit();
    void blurFocusedEdit();

    bool setEditValue(const char* id, const char* value, bool fireCallback = false);
    bool setEditValue(const char* id, const std::string& value, bool fireCallback = false);

    std::string getEditValue(const char* id, const char* def = "");

    bool focusEdit(const char* id, bool focused = true);
    bool isEditFocused(const char* id);
    bool setEditCursor(const char* id, int bytePos);

    bool onEditFocusChanged(const char* id, std::function<void(bool)> fn);
    bool onEditChanged(const char* id, std::function<void(const char*)> fn);
    bool onEditSubmit(const char* id, std::function<void(const char*)> fn);

    bool onEditFocusChangedAny(std::function<void(unsigned long long editId, bool focused)> fn);
    bool onEditChangedAny(std::function<void(unsigned long long editId, const char* value)> fn);
    bool onEditSubmitAny(std::function<void(unsigned long long editId, const char* value)> fn);

    void setCursorChangedHandler(std::function<void(const char*)> fn);
    void setDocumentScaleHandler(std::function<float()> fn);

    void setDocumentColor(unsigned int color);
    void setHostContainer(std::weak_ptr<CContainer> host);
    void restartAnimations();
    void pauseAnimations(bool bPaused = true);

    unsigned long long tweenTo(const char* id, const HTMLTweenProps& props, float dur, const HTMLTweenOptions& opts = {});
    unsigned long long tweenFrom(const char* id, const HTMLTweenProps& props, float dur, const HTMLTweenOptions& opts = {});
    bool killTweensOf(const char* id);
    void pauseTweens(bool bPaused = true);

private:
    friend class CHTMLElement;

    HTMLDom::NodePtr findNode(const char* id);
    bool bindNode(unsigned long long nodeId, ClickHandler handler);

    void registerSelf();
    void releaseRoot();

private:
    unsigned long long _rootId = 0;
    bool _bOwns = false;

    std::unique_ptr<CHTMLDocumentCallbacks> _callbacks;
};

_G2D_NAMESPACE_END_
