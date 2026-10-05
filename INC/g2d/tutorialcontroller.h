#pragma once

#include "g2d.h"
#include "container.h"
#include "sprite.h"
#include "eventemmiter.h"

_G2D_NAMESPACE_BEGIN_

class TutorialController;

class TutorialLayer : public CContainer
{
public:
    virtual void                update                  (float              dt)override;
};

class TutorialController: public CEventEmmiter
{
    friend class TutorialLayer;
    friend class CContainer;
public:
    enum Events
    {
        EVT_ON_START_EVENT = EventsNs::TUTORIALCONTROLLER_EVT_FIRST,
        EVT_ON_COMPLETE_EVENT,
        EVT_ON_CANCEL_EVENT,
    };

    inline static TutorialController* getInstance()
    {
        static TutorialController res;
        return &res;
    }
                    TutorialController      (void);
    void            addElements             (std::span<int>           elements);
    void            addElement              (int                      nTutorialId);
    void            releaseElement          (bool                     bSilent = false);
    bool            isElementActive         (CContainer*              ptrElement);
    bool            isElementActive         (CContainerPtr            ptrElement) { return isElementActive(ptrElement.get()); }
    void            onElementClick          (CContainerPtr            ptrElement);
    void            onElementClickHandled   (CContainerPtr            ptrElement);
    bool            isRunning               (void) { return !_q.empty(); }
    int             getEvent                (void) { return _nCurrenEvent; }
    void            init                    (void);
    void            beginScenario           (int                      nScenario) { assert(_nScenario < 0); _nScenario = nScenario; }
    void            endScenario             (void) {assert(_nScenario > -1); _nScenario = -1; }
    auto            getScenario             (void) {return _nScenario;}
    void            cancelAll               (void);
public:
   
private:
    void            animatePointer          (void);
    void            showFront               (void);
    void            updatePos               (void);
    void            onElementRendered       (CContainer*                ptrRender);

private:
    std::queue<int>            _q;
    CSpritePtr                 _ptrSprPointer;
    CContainerPtr              _ptrLayer;
    CContainerPtr              _ptrTutorialHost;
    bool                       _frontShown    = false;
    float                      _calcedX       = 0.f;
    float                      _calcedY       = 0.f;
    float                      _dx            = 0;
    float                      _dy            = 0;
    int                        _nCurrenEvent  = -1;
    int                        _nScenario     = -1;
   
};

_G2D_NAMESPACE_END_