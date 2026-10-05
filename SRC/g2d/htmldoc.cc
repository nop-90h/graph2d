#include "htmldoc.h"

_G2D_NAMESPACE_BEGIN_

namespace
{

    template <typename R, typename... A>
    void appendHandler(std::function<R(A...)>& slot, std::function<R(A...)> fn)
    {
        if (!fn)
            return;

        if (slot)
        {
            auto prev = std::move(slot);

            slot = [prev = std::move(prev), fn = std::move(fn)](A... args)
            {
                prev(args...);
                fn(args...);
            };
        }
        else
        {
            slot = std::move(fn);
        }
    }
}

CHTMLDocumentCallbacks::~CHTMLDocumentCallbacks()
{
    clearClickBridges();
}

CHTMLDocumentCallbacks::NodeHandlers* CHTMLDocumentCallbacks::getHandlers(unsigned long long nodeId)
{
    auto it = _handlers.find(nodeId);
    return (it != _handlers.end()) ? &it->second : nullptr;
}

bool CHTMLDocumentCallbacks::addClickHandler(unsigned long long nodeId, ClickHandler handler)
{
    if (nodeId == 0 || !handler)
        return false;

    auto& h = _handlers[nodeId];

    appendHandler(h.click, std::move(handler));

    if (!h.clickBridgeInstalled)
        installClickBridge(nodeId);

    return true;
}

bool CHTMLDocumentCallbacks::unbindNode(unsigned long long nodeId)
{
    auto it = _handlers.find(nodeId);

    if (it == _handlers.end())
        return false;

    const bool hadBridge = it->second.clickBridgeInstalled;

    _handlers.erase(it);

    if (hadBridge)
    {
        _clickBridges.erase(nodeId);
        HTMLDom::getInstance()->setOnClick(nodeId, nullptr);
    }

    return true;
}

bool CHTMLDocumentCallbacks::addEnterHandler(unsigned long long nodeId, std::function<void()> fn)
{
    if (nodeId == 0 || !fn)
        return false;

    appendHandler(_handlers[nodeId].enter, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addLeaveHandler(unsigned long long nodeId, std::function<void()> fn)
{
    if (nodeId == 0 || !fn)
        return false;

    appendHandler(_handlers[nodeId].leave, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addDropdownHandler(unsigned long long nodeId, std::function<void()> fn)
{
    if (nodeId == 0 || !fn)
        return false;

    appendHandler(_handlers[nodeId].dropdown, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addSelectChangedHandler(unsigned long long nodeId, std::function<void(int, const char*)> fn)
{
    if (nodeId == 0 || !fn)
        return false;

    appendHandler(_handlers[nodeId].selectChanged, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addSliderChangedHandler(unsigned long long nodeId, std::function<void(float)> fn)
{
    if (nodeId == 0 || !fn)
        return false;

    appendHandler(_handlers[nodeId].sliderChanged, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addDragHandler(unsigned long long nodeId, std::function<void(bool)> fn)
{
    if (nodeId == 0 || !fn)
        return false;

    appendHandler(_handlers[nodeId].drag, std::move(fn));
    return true;
}

#ifdef HAS_SPINE
bool CHTMLDocumentCallbacks::addSpineHandler(unsigned long long nodeId,
    std::function<void(eHTMLSpineEventType, const char*, const char*)> fn)
{
    if (nodeId == 0 || !fn)
        return false;

    appendHandler(_handlers[nodeId].spine, std::move(fn));
    return true;
}
#endif

bool CHTMLDocumentCallbacks::addImageHandler(unsigned long long nodeId,
    std::function<void(eHTMLImageAnimEventType)> fn)
{
    if (nodeId == 0 || !fn)
        return false;

    appendHandler(_handlers[nodeId].image, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addFontHandler(unsigned long long nodeId,
    std::function<void(eHTMLFontAnimEventType, eFxStyle, eAppearStyle)> fn)
{
    if (nodeId == 0 || !fn)
        return false;

    appendHandler(_handlers[nodeId].font, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addEditFocusHandler(unsigned long long nodeId, std::function<void(bool)> fn)
{
    if (nodeId == 0 || !fn)
        return false;

    appendHandler(_handlers[nodeId].editFocus, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addEditChangedHandler(unsigned long long nodeId, std::function<void(const char*)> fn)
{
    if (nodeId == 0 || !fn)
        return false;

    appendHandler(_handlers[nodeId].editChanged, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addEditSubmitHandler(unsigned long long nodeId, std::function<void(const char*)> fn)
{
    if (nodeId == 0 || !fn)
        return false;

    appendHandler(_handlers[nodeId].editSubmit, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addAnyHoverHandler(std::function<void(unsigned long long)> enter,
                                                std::function<void(unsigned long long)> leave)
{
    if (!enter && !leave)
        return false;

    appendHandler(_anyHover, std::move(enter));
    appendHandler(_anyLeave, std::move(leave));

    return true;
}

bool CHTMLDocumentCallbacks::addAnySelectChangedHandler(std::function<void(unsigned long long, int, const char*)> fn)
{
    if (!fn)
        return false;

    appendHandler(_anySelectChanged, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addAnySliderChangedHandler(std::function<void(unsigned long long, float)> fn)
{
    if (!fn)
        return false;

    appendHandler(_anySliderChanged, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addAnyDragHandler(std::function<void(unsigned long long, bool)> fn)
{
    if (!fn)
        return false;

    appendHandler(_anyDrag, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addAnyDropdownHandler(std::function<void(unsigned long long)> fn)
{
    if (!fn)
        return false;

    appendHandler(_anyDropdown, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addAnyEditFocusHandler(std::function<void(unsigned long long, bool)> fn)
{
    if (!fn)
        return false;

    appendHandler(_anyEditFocus, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addAnyEditChangedHandler(std::function<void(unsigned long long, const char*)> fn)
{
    if (!fn)
        return false;

    appendHandler(_anyEditChanged, std::move(fn));
    return true;
}

bool CHTMLDocumentCallbacks::addAnyEditSubmitHandler(std::function<void(unsigned long long, const char*)> fn)
{
    if (!fn)
        return false;

    appendHandler(_anyEditSubmit, std::move(fn));
    return true;
}

void CHTMLDocumentCallbacks::setCursorChangedHandler(std::function<void(const char*)> fn)
{
    appendHandler(_cursorChanged, std::move(fn));
}

void CHTMLDocumentCallbacks::setDocumentScaleHandler(std::function<float()> fn)
{
    _documentScale = std::move(fn);
}

void CHTMLDocumentCallbacks::clearClickBridges()
{
    for (auto id : _clickBridges)
        HTMLDom::getInstance()->setOnClick(id, nullptr);

    _clickBridges.clear();

    for (auto& kv : _handlers)
        kv.second.clickBridgeInstalled = false;
}

void CHTMLDocumentCallbacks::reset()
{
    clearClickBridges();

    _handlers.clear();

    _cursorChanged = nullptr;

    _anyHover = nullptr;
    _anyLeave = nullptr;
    _anyDropdown = nullptr;

    _anySelectChanged = nullptr;
    _anySliderChanged = nullptr;
    _anyDrag = nullptr;

    _anyEditFocus = nullptr;
    _anyEditChanged = nullptr;
    _anyEditSubmit = nullptr;

    _documentScale = nullptr;
}

void CHTMLDocumentCallbacks::installClickBridge(unsigned long long nodeId)
{
    auto h = getHandlers(nodeId);

    if (!h)
        return;

    h->clickBridgeInstalled = true;

    bool ok = HTMLDom::getInstance()->setOnClick(nodeId,
        [this](unsigned long long nid, float x, float y, int button)
        {
            dispatchClick(nid, x, y, button);
        });

    if (ok)
        _clickBridges.insert(nodeId);
    else
        h->clickBridgeInstalled = false;
}

void CHTMLDocumentCallbacks::dispatchClick(unsigned long long nodeId, float x, float y, int button)
{

    if (auto h = getHandlers(nodeId))
        if (h->click)
            h->click(nodeId, x, y, button);
}

void CHTMLDocumentCallbacks::onCursorChanged(const char* szNewCursor)
{
    if (_cursorChanged)
        _cursorChanged(szNewCursor);
}

void CHTMLDocumentCallbacks::onHover(unsigned long long, unsigned long long nodeId)
{
    if (auto h = getHandlers(nodeId))
        if (h->enter)
            h->enter();

    if (_anyHover)
        _anyHover(nodeId);
}

void CHTMLDocumentCallbacks::onLeave(unsigned long long, unsigned long long nodeId)
{
    if (auto h = getHandlers(nodeId))
        if (h->leave)
            h->leave();

    if (_anyLeave)
        _anyLeave(nodeId);
}

bool CHTMLDocumentCallbacks::onSelectDropdownRequested(unsigned long long, unsigned long long selectId)
{
    bool handled = false;

    if (auto h = getHandlers(selectId))
    {
        if (h->dropdown)
        {
            h->dropdown();
            handled = true;
        }
    }

    if (_anyDropdown)
    {
        _anyDropdown(selectId);
        handled = true;
    }

    return handled;
}

void CHTMLDocumentCallbacks::onSelectChanged(unsigned long long,
                                             unsigned long long selectId,
                                             unsigned long long,
                                             int index,
                                             const char* value)
{
    if (auto h = getHandlers(selectId))
        if (h->selectChanged)
            h->selectChanged(index, value);

    if (_anySelectChanged)
        _anySelectChanged(selectId, index, value);
}

void CHTMLDocumentCallbacks::onDrag(unsigned long long, unsigned long long nodeId, bool bEnd)
{
    if (auto h = getHandlers(nodeId))
        if (h->drag)
            h->drag(bEnd);

    if (_anyDrag)
        _anyDrag(nodeId, bEnd);
}

void CHTMLDocumentCallbacks::onSliderChanged(unsigned long long, unsigned long long nodeId, float value)
{
    if (auto h = getHandlers(nodeId))
        if (h->sliderChanged)
            h->sliderChanged(value);

    if (_anySliderChanged)
        _anySliderChanged(nodeId, value);
}

void CHTMLDocumentCallbacks::onEditFocusChanged(unsigned long long, unsigned long long editId, bool bFocused)
{
    if (auto h = getHandlers(editId))
        if (h->editFocus)
            h->editFocus(bFocused);

    if (_anyEditFocus)
        _anyEditFocus(editId, bFocused);
}

void CHTMLDocumentCallbacks::onEditChanged(unsigned long long, unsigned long long editId, const char* value)
{
    if (auto h = getHandlers(editId))
        if (h->editChanged)
            h->editChanged(value);

    if (_anyEditChanged)
        _anyEditChanged(editId, value);
}

void CHTMLDocumentCallbacks::onEditSubmit(unsigned long long, unsigned long long editId, const char* value)
{
    if (auto h = getHandlers(editId))
        if (h->editSubmit)
            h->editSubmit(value);

    if (_anyEditSubmit)
        _anyEditSubmit(editId, value);
}

float CHTMLDocumentCallbacks::getDocumentScale(unsigned long long)
{
    return _documentScale ? _documentScale() : 1.f;
}

#ifdef HAS_SPINE
void CHTMLDocumentCallbacks::onSpineAnimEvent(unsigned long long,
                                              unsigned long long nodeId,
                                              eHTMLSpineEventType type,
                                              const char* animName,
                                              const char* eventName)
{
    if (auto h = getHandlers(nodeId))
        if (h->spine)
            h->spine(type, animName, eventName);
}
#endif

void CHTMLDocumentCallbacks::onImageAnimEvent(unsigned long long,
                                              unsigned long long nodeId,
                                              eHTMLImageAnimEventType type,
                                              eHTMLIdleStyle)
{
    if (auto h = getHandlers(nodeId))
        if (h->image)
            h->image(type);
}

void CHTMLDocumentCallbacks::onFontAnimEvent(unsigned long long,
                                             unsigned long long nodeId,
                                             eHTMLFontAnimEventType type,
                                             eFxStyle fxStyle,
                                             eAppearStyle appearStyle)
{
    if (auto h = getHandlers(nodeId))
        if (h->font)
            h->font(type, fxStyle, appearStyle);
}

void CHTMLDocumentCallbacks::onDragAny(unsigned long long, unsigned long long nodeId, bool bEnd)
{

    if (auto h = getHandlers(nodeId))
        if (h->drag)
            h->drag(bEnd);

    if (_anyDrag)
        _anyDrag(nodeId, bEnd);
}

CHTMLDocument::CHTMLDocument()
    : _rootId(0)
    , _bOwns(false)
    , _callbacks(std::make_unique<CHTMLDocumentCallbacks>())
{
}

CHTMLDocument::CHTMLDocument(const char* html)
    : CHTMLDocument()
{
    _rootId = HTMLDom::getInstance()->parse(html);
    _bOwns = (_rootId != 0);

    registerSelf();
}

CHTMLDocument::CHTMLDocument(unsigned long long rootId, bool bOwns)
    : CHTMLDocument()
{
    if (rootId != 0 && HTMLDom::getInstance()->getNode(rootId))
    {
        _rootId = rootId;
        _bOwns = bOwns;

        registerSelf();
    }
}

CHTMLDocument::~CHTMLDocument()
{
    releaseRoot();
}

CHTMLDocument::CHTMLDocument(CHTMLDocument&& other) noexcept
    : _rootId(other._rootId)
    , _bOwns(other._bOwns)
    , _callbacks(std::move(other._callbacks))
{
    other._rootId = 0;
    other._bOwns = false;

    if (!other._callbacks)
        other._callbacks = std::make_unique<CHTMLDocumentCallbacks>();
}

CHTMLDocument& CHTMLDocument::operator=(CHTMLDocument&& other) noexcept
{
    if (this != &other)
    {
        releaseRoot();

        _rootId = other._rootId;
        _bOwns = other._bOwns;
        _callbacks = std::move(other._callbacks);

        other._rootId = 0;
        other._bOwns = false;

        if (!other._callbacks)
            other._callbacks = std::make_unique<CHTMLDocumentCallbacks>();
    }

    return *this;
}

void CHTMLDocument::registerSelf()
{
    if (_rootId != 0 && _callbacks)
        HTMLDom::getInstance()->setDocumentCallbacks(_rootId, _callbacks.get());
}

bool CHTMLDocument::isValid() const
{
    if (_rootId == 0)
        return false;

    return HTMLDom::getInstance()->getNode(_rootId) != nullptr;
}

void CHTMLDocument::releaseRoot()
{
    if (_rootId != 0)
    {
        auto dom = HTMLDom::getInstance();

        if (_callbacks)
            _callbacks->clearClickBridges();

        dom->pauseTweensForRoot(_rootId, false);

        if (_bOwns)
        {
            dom->removeNode(_rootId);
        }
        else
        {
            dom->setDocumentCallbacks(_rootId, nullptr);
            dom->setHostContainer(_rootId, {});
        }
    }

    _rootId = 0;
    _bOwns = false;

    if (_callbacks)
        _callbacks->reset();
}

void CHTMLDocument::reset(unsigned long long newRootId, bool bOwns)
{
    releaseRoot();

    if (newRootId == 0)
        return;

    if (HTMLDom::getInstance()->getNode(newRootId))
    {
        _rootId = newRootId;
        _bOwns = bOwns;

        registerSelf();
    }
}

HTMLDom::NodePtr CHTMLDocument::findNode(const char* id)
{
    if (_rootId == 0 || !id)
        return nullptr;

    return HTMLDom::getInstance()->getElementById(_rootId, id);
}

unsigned long long CHTMLDocument::find(const char* id)
{
    auto node = findNode(id);
    return node ? node->id : 0;
}

CHTMLElement CHTMLDocument::get(const char* id)
{
    return CHTMLElement(this, find(id));
}

std::vector<unsigned long long> CHTMLDocument::insertHTML(const char* targetId, const char* html, int index)
{
    auto target = findNode(targetId);

    if (!target)
        return {};

    return HTMLDom::getInstance()->insertHTML(target->id, html, index);
}

unsigned long long CHTMLDocument::createElement(eHTMLTag tag, const char* parentId, int index)
{
    auto parent = findNode(parentId);

    if (!parent)
        return 0;

    return HTMLDom::getInstance()->createNode(tag, parent->id, index);
}

bool CHTMLDocument::remove(const char* id)
{
    auto node = findNode(id);

    if (!node)
        return false;

    return HTMLDom::getInstance()->removeNode(node->id);
}

bool CHTMLDocument::move(const char* id, const char* newParentId, int index)
{
    auto node = findNode(id);
    auto newParent = findNode(newParentId);

    if (!node || !newParent)
        return false;

    return HTMLDom::getInstance()->moveNode(node->id, newParent->id, index);
}

bool CHTMLDocument::setAttr(const char* id, const char* name, const char* value)
{
    auto node = findNode(id);

    if (!node)
        return false;

    return HTMLDom::getInstance()->setAttr(node->id, name, value);
}

bool CHTMLDocument::setAttr(const char* id, const char* name, const std::string& value)
{
    return setAttr(id, name, value.c_str());
}

bool CHTMLDocument::setAttr(const char* id, const char* name, int value)
{
    return setAttr(id, name, std::to_string(value));
}

bool CHTMLDocument::setAttr(const char* id, const char* name, float value)
{
    return setAttr(id, name, std::format("{}", value));
}

bool CHTMLDocument::setAttr(const char* id, const char* name, bool value)
{
    return setAttr(id, name, value ? "true" : "false");
}

bool CHTMLDocument::setAttr(const char* id, const char* name, unsigned int value)
{

    return setAttr(id, name, std::format("#{:08X}", value));
}

bool CHTMLDocument::removeAttr(const char* id, const char* name)
{
    auto node = findNode(id);

    if (!node)
        return false;

    return HTMLDom::getInstance()->removeAttr(node->id, name);
}

std::string CHTMLDocument::getAttr(const char* id, const char* name, const char* def)
{
    auto node = findNode(id);

    if (!node)
        return def ? def : "";

    const char* v = HTMLDom::getInstance()->getAttr(node->id, name);

    return v ? v : (def ? def : "");
}

bool CHTMLDocument::setText(const char* id, const char* text)
{
    auto node = findNode(id);

    if (!node)
        return false;

    return HTMLDom::getInstance()->setTextContent(node->id, text);
}

bool CHTMLDocument::setText(const char* id, const std::string& text)
{
    return setText(id, text.c_str());
}

bool CHTMLDocument::bindNode(unsigned long long nodeId, ClickHandler handler)
{
    if (!_callbacks)
        return false;

    return _callbacks->addClickHandler(nodeId, std::move(handler));
}

bool CHTMLDocument::bind(const char* id, ClickHandler handler)
{
    auto node = findNode(id);

    if (!node)
        return false;

    return bindNode(node->id, std::move(handler));
}

bool CHTMLDocument::unbind(const char* id)
{
    auto node = findNode(id);

    if (!node)
        return false;

    if (!_callbacks)
        return false;

    return _callbacks->unbindNode(node->id);
}

bool CHTMLDocument::onClick(const char* id, std::function<void()> fn)
{
    if (!fn)
        return false;

    return bind(id, [fn = std::move(fn)](unsigned long long, float, float, int)
    {
        fn();
    });
}

bool CHTMLDocument::onClick(const char* id, std::function<void(float, float, int)> fn)
{
    if (!fn)
        return false;

    return bind(id, [fn = std::move(fn)](unsigned long long, float x, float y, int button)
    {
        fn(x, y, button);
    });
}

bool CHTMLDocument::onHover(const char* id, std::function<void()> enter, std::function<void()> leave)
{
    auto node = findNode(id);

    if (!node || !_callbacks)
        return false;

    if (enter)
        _callbacks->addEnterHandler(node->id, std::move(enter));

    if (leave)
        _callbacks->addLeaveHandler(node->id, std::move(leave));

    return true;
}

bool CHTMLDocument::onSelectChanged(const char* id, std::function<void(int, const char*)> fn)
{
    auto node = findNode(id);

    if (!node || !fn || !_callbacks)
        return false;

    return _callbacks->addSelectChangedHandler(node->id, std::move(fn));
}

bool CHTMLDocument::onSliderChanged(const char* id, std::function<void(float)> fn)
{
    auto node = findNode(id);

    if (!node || !fn || !_callbacks)
        return false;

    return _callbacks->addSliderChangedHandler(node->id, std::move(fn));
}

bool CHTMLDocument::onDrag(const char* id, std::function<void(bool)> fn)
{
    auto node = findNode(id);

    if (!node || !fn || !_callbacks)
        return false;

    return _callbacks->addDragHandler(node->id, std::move(fn));
}

bool CHTMLDocument::onSelectDropdownRequested(const char* id, std::function<void()> fn)
{
    auto node = findNode(id);

    if (!node || !fn || !_callbacks)
        return false;

    return _callbacks->addDropdownHandler(node->id, std::move(fn));
}

#ifdef HAS_SPINE
bool CHTMLDocument::onSpineAnimEvent(const char* id,
    std::function<void(eHTMLSpineEventType, const char*, const char*)> fn)
{
    auto node = findNode(id);

    if (!node || !fn || !_callbacks)
        return false;

    return _callbacks->addSpineHandler(node->id, std::move(fn));
}
#endif

bool CHTMLDocument::onImageAnimEvent(const char* id, std::function<void(eHTMLImageAnimEventType)> fn)
{
    auto node = findNode(id);

    if (!node || !fn || !_callbacks)
        return false;

    return _callbacks->addImageHandler(node->id, std::move(fn));
}

bool CHTMLDocument::onFontAnimEvent(const char* id,
    std::function<void(eHTMLFontAnimEventType, eFxStyle, eAppearStyle)> fn)
{
    auto node = findNode(id);

    if (!node || !fn || !_callbacks)
        return false;

    return _callbacks->addFontHandler(node->id, std::move(fn));
}

bool CHTMLDocument::onHoverAny(std::function<void(unsigned long long)> enter,
                               std::function<void(unsigned long long)> leave)
{
    if (!_callbacks)
        return false;

    return _callbacks->addAnyHoverHandler(std::move(enter), std::move(leave));
}

bool CHTMLDocument::onSelectChangedAny(std::function<void(unsigned long long, int, const char*)> fn)
{
    if (!fn || !_callbacks)
        return false;

    return _callbacks->addAnySelectChangedHandler(std::move(fn));
}

bool CHTMLDocument::onSliderChangedAny(std::function<void(unsigned long long, float)> fn)
{
    if (!fn || !_callbacks)
        return false;

    return _callbacks->addAnySliderChangedHandler(std::move(fn));
}

bool CHTMLDocument::onDragAny(std::function<void(unsigned long long, bool)> fn)
{
    if (!fn || !_callbacks)
        return false;

    return _callbacks->addAnyDragHandler(std::move(fn));
}

bool CHTMLDocument::onDropdownRequestedAny(std::function<void(unsigned long long)> fn)
{
    if (!fn || !_callbacks)
        return false;

    return _callbacks->addAnyDropdownHandler(std::move(fn));
}

unsigned long long CHTMLDocument::getFocusedEditId() const
{
    unsigned long long id = HTMLDom::getInstance()->getFocusedEditId();

    if (id == 0)
        return 0;

    if (_rootId == 0)
        return 0;

    if (HTMLDom::getInstance()->getRootIdForNode(id) != _rootId)
        return 0;

    return id;
}

CHTMLElement CHTMLDocument::getFocusedEdit()
{
    return CHTMLElement(this, getFocusedEditId());
}

void CHTMLDocument::blurFocusedEdit()
{
    HTMLDom::getInstance()->blurFocusedEdit();
}

bool CHTMLDocument::setEditValue(const char* id, const char* value, bool fireCallback)
{
    auto node = findNode(id);

    if (!node)
        return false;

    return HTMLDom::getInstance()->setEditValue(node->id, value, fireCallback);
}

bool CHTMLDocument::setEditValue(const char* id, const std::string& value, bool fireCallback)
{
    return setEditValue(id, value.c_str(), fireCallback);
}

std::string CHTMLDocument::getEditValue(const char* id, const char* def)
{
    auto node = findNode(id);

    if (!node)
        return def ? def : "";

    const char* v = HTMLDom::getInstance()->getEditValue(node->id);

    return v ? v : (def ? def : "");
}

bool CHTMLDocument::focusEdit(const char* id, bool focused)
{
    auto node = findNode(id);

    if (!node)
        return false;

    return HTMLDom::getInstance()->focusEdit(node->id, focused);
}

bool CHTMLDocument::isEditFocused(const char* id)
{
    auto node = findNode(id);

    if (!node)
        return false;

    return HTMLDom::getInstance()->isEditFocused(node->id);
}

bool CHTMLDocument::setEditCursor(const char* id, int bytePos)
{
    auto node = findNode(id);

    if (!node)
        return false;

    return HTMLDom::getInstance()->setEditCursor(node->id, bytePos);
}

bool CHTMLDocument::onEditFocusChanged(const char* id, std::function<void(bool)> fn)
{
    auto node = findNode(id);

    if (!node || !fn || !_callbacks)
        return false;

    return _callbacks->addEditFocusHandler(node->id, std::move(fn));
}

bool CHTMLDocument::onEditChanged(const char* id, std::function<void(const char*)> fn)
{
    auto node = findNode(id);

    if (!node || !fn || !_callbacks)
        return false;

    return _callbacks->addEditChangedHandler(node->id, std::move(fn));
}

bool CHTMLDocument::onEditSubmit(const char* id, std::function<void(const char*)> fn)
{
    auto node = findNode(id);

    if (!node || !fn || !_callbacks)
        return false;

    return _callbacks->addEditSubmitHandler(node->id, std::move(fn));
}

bool CHTMLDocument::onEditFocusChangedAny(std::function<void(unsigned long long, bool)> fn)
{
    if (!fn || !_callbacks)
        return false;

    return _callbacks->addAnyEditFocusHandler(std::move(fn));
}

bool CHTMLDocument::onEditChangedAny(std::function<void(unsigned long long, const char*)> fn)
{
    if (!fn || !_callbacks)
        return false;

    return _callbacks->addAnyEditChangedHandler(std::move(fn));
}

bool CHTMLDocument::onEditSubmitAny(std::function<void(unsigned long long, const char*)> fn)
{
    if (!fn || !_callbacks)
        return false;

    return _callbacks->addAnyEditSubmitHandler(std::move(fn));
}

void CHTMLDocument::setCursorChangedHandler(std::function<void(const char*)> fn)
{
    if (_callbacks)
        _callbacks->setCursorChangedHandler(std::move(fn));
}

void CHTMLDocument::setDocumentScaleHandler(std::function<float()> fn)
{
    if (_callbacks)
        _callbacks->setDocumentScaleHandler(std::move(fn));
}

void CHTMLDocument::setDocumentColor(unsigned int color)
{
    if (_rootId == 0)
        return;

    if (auto opts = HTMLDom::getInstance()->getRootOptionsPtr(_rootId))
        opts->documentColor = color;
}

void CHTMLDocument::setHostContainer(std::weak_ptr<CContainer> host)
{
    if (_rootId != 0)
        HTMLDom::getInstance()->setHostContainer(_rootId, host);
}

void CHTMLDocument::restartAnimations()
{
    if (_rootId != 0)
        HTMLDom::getInstance()->restartAppear(_rootId);
}

void CHTMLDocument::pauseAnimations(bool bPaused)
{
    if (_rootId != 0)
    {
        HTMLDom::getInstance()->pauseAnimsForHTML(_rootId, bPaused);
        HTMLDom::getInstance()->pauseTweensForRoot(_rootId, bPaused);
    }
}

HTMLDom::NodePtr CHTMLElement::node() const
{
    if (!_doc || _nodeId == 0)
        return nullptr;

    return HTMLDom::getInstance()->getNode(_nodeId);
}

bool CHTMLElement::isValid() const
{
    return node() != nullptr;
}

unsigned long long CHTMLElement::getRootId() const
{
    return _doc ? _doc->getRootId() : 0;
}

eHTMLTag CHTMLElement::getTag() const
{
    auto n = node();
    return n ? n->tag : eHTMLTag::Unknown;
}

bool CHTMLElement::setVisible(bool visible)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setVisible(n->id, visible);
}

bool CHTMLElement::setDisabled(bool disabled)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setDisabled(n->id, disabled);
}

bool CHTMLElement::setDisabledTint(unsigned int tint)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setDisabledTint(n->id, tint);
}

bool CHTMLElement::setAttr(const char* name, const char* value)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setAttr(n->id, name, value);
}

bool CHTMLElement::setAttr(const char* name, const std::string& value)
{
    return setAttr(name, value.c_str());
}

bool CHTMLElement::setAttr(const char* name, int value)
{
    return setAttr(name, std::to_string(value));
}

bool CHTMLElement::setAttr(const char* name, float value)
{
    return setAttr(name, std::format("{}", value));
}

bool CHTMLElement::setAttr(const char* name, bool value)
{
    return setAttr(name, value ? "true" : "false");
}

bool CHTMLElement::setAttr(const char* name, unsigned int value)
{
    return setAttr(name, std::format("#{:08X}", value));
}

bool CHTMLElement::removeAttr(const char* name)
{
    auto n = node();
    return n && HTMLDom::getInstance()->removeAttr(n->id, name);
}

std::string CHTMLElement::getAttr(const char* name, const char* def) const
{
    auto n = node();

    if (!n)
        return def ? def : "";

    const char* v = HTMLDom::getInstance()->getAttr(n->id, name);

    return v ? v : (def ? def : "");
}

bool CHTMLElement::setText(const char* text)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setTextContent(n->id, text);
}

bool CHTMLElement::setText(const std::string& text)
{
    return setText(text.c_str());
}

bool CHTMLElement::onClick(std::function<void()> fn)
{
    auto n = node();

    if (!n || !_doc || !fn)
        return false;

    return _doc->bindNode(n->id,
        [fn = std::move(fn)](unsigned long long, float, float, int)
        {
            fn();
        });
}

bool CHTMLElement::onClick(std::function<void(float, float, int)> fn)
{
    auto n = node();

    if (!n || !_doc || !fn)
        return false;

    return _doc->bindNode(n->id,
        [fn = std::move(fn)](unsigned long long, float x, float y, int button)
        {
            fn(x, y, button);
        });
}

bool CHTMLElement::isChecked() const
{
    auto n = node();
    return n && HTMLDom::getInstance()->isChecked(n->id);
}

bool CHTMLElement::setChecked(bool checked)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setChecked(n->id, checked);
}

bool CHTMLElement::toggleChecked()
{
    auto n = node();
    return n && HTMLDom::getInstance()->toggleChecked(n->id);
}

bool CHTMLElement::onCheckedChanged(std::function<void(bool)> fn)
{
    if (!fn)
        return false;

    CHTMLElement self = *this;

    return onClick([self, fn = std::move(fn)](float, float, int)
    {
        if (self.isValid())
            fn(self.isChecked());
    });
}

bool CHTMLElement::selectOption(int index, bool fireCallback)
{
    auto n = node();
    return n && HTMLDom::getInstance()->selectOptionByIndex(n->id, index, fireCallback);
}

int CHTMLElement::getSelectedIndex() const
{
    auto n = node();

    if (!n)
        return -1;

    return n->selectedIndex;
}

bool CHTMLElement::onSelectChanged(std::function<void(int, const char*)> fn)
{
    auto n = node();

    if (!n || !_doc || !_doc->_callbacks || !fn)
        return false;

    return _doc->_callbacks->addSelectChangedHandler(n->id, std::move(fn));
}

float CHTMLElement::getSliderValue() const
{
    auto n = node();
    return n ? HTMLDom::getInstance()->getSliderValue(n->id) : 0.f;
}

bool CHTMLElement::setSliderValue(float value, bool fireCallback)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setSliderValue(n->id, value, fireCallback);
}

bool CHTMLElement::onSliderChanged(std::function<void(float)> fn)
{
    auto n = node();

    if (!n || !_doc || !_doc->_callbacks || !fn)
        return false;

    return _doc->_callbacks->addSliderChangedHandler(n->id, std::move(fn));
}

float CHTMLElement::getProgress() const
{
    auto n = node();
    return n ? HTMLDom::getInstance()->getProgressValue(n->id) : 0.f;
}

float CHTMLElement::getProgressPercent() const
{
    auto n = node();
    return n ? HTMLDom::getInstance()->getProgressPercent(n->id) : 0.f;
}

bool CHTMLElement::setProgress(float value01)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setProgressValue(n->id, value01);
}

bool CHTMLElement::setProgressPercent(float percent)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setProgressPercent(n->id, percent);
}

std::string CHTMLElement::getEditValue() const
{
    auto n = node();

    if (!n)
        return "";

    const char* v = HTMLDom::getInstance()->getEditValue(n->id);

    return v ? v : "";
}

bool CHTMLElement::setEditValue(const char* value, bool fireCallback)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setEditValue(n->id, value, fireCallback);
}

bool CHTMLElement::setEditValue(const std::string& value, bool fireCallback)
{
    return setEditValue(value.c_str(), fireCallback);
}

bool CHTMLElement::focusEdit(bool focused)
{
    auto n = node();
    return n && HTMLDom::getInstance()->focusEdit(n->id, focused);
}

bool CHTMLElement::blurEdit()
{
    return focusEdit(false);
}

bool CHTMLElement::isEditFocused() const
{
    auto n = node();
    return n && HTMLDom::getInstance()->isEditFocused(n->id);
}

int CHTMLElement::getEditCursor() const
{
    auto n = node();

    if (!n)
        return -1;

    return n->editCursor;
}

bool CHTMLElement::setEditCursor(int bytePos)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setEditCursor(n->id, bytePos);
}

bool CHTMLElement::onEditFocusChanged(std::function<void(bool)> fn)
{
    auto n = node();

    if (!n || !_doc || !_doc->_callbacks || !fn)
        return false;

    return _doc->_callbacks->addEditFocusHandler(n->id, std::move(fn));
}

bool CHTMLElement::onEditChanged(std::function<void(const char*)> fn)
{
    auto n = node();

    if (!n || !_doc || !_doc->_callbacks || !fn)
        return false;

    return _doc->_callbacks->addEditChangedHandler(n->id, std::move(fn));
}

bool CHTMLElement::onEditSubmit(std::function<void(const char*)> fn)
{
    auto n = node();

    if (!n || !_doc || !_doc->_callbacks || !fn)
        return false;

    return _doc->_callbacks->addEditSubmitHandler(n->id, std::move(fn));
}

#ifdef HAS_SPINE
bool CHTMLElement::spinePlay(const char* animName, bool loop)
{
    auto n = node();
    return n && HTMLDom::getInstance()->spineSetAnimation(n->id, 0, animName, loop);
}

bool CHTMLElement::spineAdd(const char* animName, bool loop)
{
    auto n = node();
    return n && HTMLDom::getInstance()->spineAddAnimation(n->id, 0, animName, loop);
}

bool CHTMLElement::spineStop()
{
    auto n = node();
    return n && HTMLDom::getInstance()->spineStopAnimation(n->id);
}

bool CHTMLElement::spinePause(bool bPaused)
{
    auto n = node();
    return n && HTMLDom::getInstance()->spineSetPaused(n->id, bPaused);
}

bool CHTMLElement::spineRestart()
{
    auto n = node();
    return n && HTMLDom::getInstance()->spineRestartAnimation(n->id);
}

#endif

bool CHTMLElement::setIdleAnimation(eHTMLIdleStyle style)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setImageAnimation(n->id, style);
}

bool CHTMLElement::stopIdleAnimation()
{
    auto n = node();
    return n && HTMLDom::getInstance()->stopImageAnimation(n->id);
}

bool CHTMLElement::restartIdleAnimation()
{
    auto n = node();
    return n && HTMLDom::getInstance()->restartImageAnimation(n->id);
}

bool CHTMLElement::setAppearAnimation(eAppearStyle style)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setFontAppearAnimation(n->id, style);
}

bool CHTMLElement::setFxAnimation(eFxStyle style)
{
    auto n = node();
    return n && HTMLDom::getInstance()->setFontFxAnimation(n->id, style);
}

bool CHTMLElement::stopFontAnimation()
{
    auto n = node();
    return n && HTMLDom::getInstance()->stopFontAnimation(n->id);
}

bool CHTMLElement::restartFontAnimation()
{
    auto n = node();
    return n && HTMLDom::getInstance()->restartFontAnimation(n->id);
}

bool CHTMLElement::restartAppear()
{
    auto n = node();

    if (!n)
        return false;

    HTMLDom::getInstance()->restartAppear(n->id);

    return true;
}

unsigned long long CHTMLElement::tweenTo(const HTMLTweenProps& props, float dur, const HTMLTweenOptions& opts)
{
    auto n = node();
    return n ? HTMLDom::getInstance()->tweenTo(n->id, props, dur, opts) : 0;
}

unsigned long long CHTMLElement::tweenFrom(const HTMLTweenProps& props, float dur, const HTMLTweenOptions& opts)
{
    auto n = node();
    return n ? HTMLDom::getInstance()->tweenFrom(n->id, props, dur, opts) : 0;
}

bool CHTMLElement::killTweens()
{
    auto n = node();
    return n && HTMLDom::getInstance()->killTweensOf(n->id);
}

unsigned long long CHTMLDocument::tweenTo(const char* id, const HTMLTweenProps& props, float dur, const HTMLTweenOptions& opts)
{
    auto node = findNode(id);
    return node ? HTMLDom::getInstance()->tweenTo(node->id, props, dur, opts) : 0;
}

unsigned long long CHTMLDocument::tweenFrom(const char* id, const HTMLTweenProps& props, float dur, const HTMLTweenOptions& opts)
{
    auto node = findNode(id);
    return node ? HTMLDom::getInstance()->tweenFrom(node->id, props, dur, opts) : 0;
}

bool CHTMLDocument::killTweensOf(const char* id)
{
    auto node = findNode(id);
    return node && HTMLDom::getInstance()->killTweensOf(node->id);
}

void CHTMLDocument::pauseTweens(bool bPaused)
{
    if (_rootId != 0)
        HTMLDom::getInstance()->pauseTweensForRoot(_rootId, bPaused);
}

_G2D_NAMESPACE_END_
