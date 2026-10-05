#pragma once
#include "g2d.h"
#include "container.h"
#include "panelhtml.h"
#include "htmldom.h"
#include <vector>
_G2D_NAMESPACE_BEGIN_
class HTMLSelectPopup;
typedef std::shared_ptr<HTMLSelectPopup> HTMLSelectPopupPtr;
class HTMLSelectPopup : public PanelHTML
{
public:
    static HTMLSelectPopupPtr show(unsigned long long selectId);
    static void closeCurrent();
    static bool isOpen();
    virtual bool close(void) override;
protected:
    bool initPopup(
        unsigned long long selectId,
        const HTMLSelectView& view,
        const Rect& anchor);
    void scrollToSelected();
    void resetEdgeScrollState();
    void updateEdgeScroll(float dt);
    void ensureHoveredRowVisible(float localX, float localY);
    virtual void update(float dt) override;
    virtual void onEvent(int nEvent, void* pData1, void* pData2, void* pData3) override;
    virtual bool onPointerDown(int32_t x, int32_t y) override;
    virtual bool onPointerMove(int32_t x, int32_t y) override;
    virtual void createScrollShadow(Rect* pRcScroll) override {}
private:
    unsigned long long _selectId = 0;
    int _selectedIndex = -1;
    float _itemH = 32.f;
    float _pad = 4.f;
    float _viewH = 0.f;
    std::vector<float> _rowH;
    std::vector<float> _rowY;
    Rect _scrollBoxLocal;
    bool _hasPointerPos = false;
    float _lastPointerX = 0.f;
    float _lastPointerY = 0.f;
    bool _popupHovered = false;
    bool _topEdgeVisited = false;
    bool _bottomEdgeVisited = false;
    bool _edgeScrolling = false;
    int _activeEdge = 0;
    bool _edgeDelayActive = false;
    int _pendingEdge = 0;
    float _edgeDelayTime = 0.f;
    bool _bWasDrag = false;
    bool _freeEdgePass = false;
    float _rawPointerX = 0.f;
    float _rawPointerY = 0.f;
    float _popupScreenX = 0.f;
    float _popupScreenY = 0.f;
    float _popupScaleX = 1.f;
    float _popupScaleY = 1.f;
    int _ptrCoordMode = -1;
    void resolvePointerLocal(float& outX, float& outY);
    void setupEdgeArrows();
    void updateEdgeArrows(float dt);
    struct EdgeArrowPart
    {
        CSpritePtr outline;
        CSpritePtr core;
    };
    EdgeArrowPart _arrUp, _arrDn;
    float _arrowTime = 0.f;
    float _lastArrowScroll = 0.f;
    int _lastHoveredRow = -1;
    static HTMLSelectPopupPtr _current;
};
void AttachHTMLSelectPopups(
    unsigned long long rootId,
    std::weak_ptr<CContainer> host);
_G2D_NAMESPACE_END_
