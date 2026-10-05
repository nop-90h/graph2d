#pragma once

#include "g2d.h"
#include "widget.h"
#include "sprite.h"
#include "staticlabel.h"
#include "framedlabel.h"

_G2D_NAMESPACE_BEGIN_

class Button : public Widget
{
friend class BaseDialog;
public:
    enum eButtonState
    {
        E_BS_NORMAL,
        E_BS_HOVER,
        E_BS_PRESSED,
        E_BS_DISABLED
    };
private:
        CSpritePtr      _ptrNormal;
        CSpritePtr      _ptrPressed;
        CSpritePtr      _ptrHover;
        CSpritePtr      _ptrDisabled;
        CSpritePtr      _ptrIconLock;
        CContainerPtr   _ptrBgFrame;
        CContainerPtr   _ptrFgFrame;
        StaticLabelPtr  _ptrText;
        FramedLabelPtr  _ptrFramedLabel;
        eButtonState    _eState = E_BS_NORMAL;
        CContainerPtr   _root;  
        TweenHandle     _twHandle;
        bool            _bHoverAnim     = true;
        bool            _bShowHoverAnim = true;
public:
                                Button()
                                {
                                    setInteractive(true);
                                }
            virtual             ~Button                 (void);
            void                create                  (CSpritePtr     ptrNormal);
            void                create                  (CSpritePtr     ptrNormal,
                                                         CSpritePtr     ptrPressed,
                                                         CSpritePtr     ptrHover,
                                                         CSpritePtr     ptrDisabled,
                                                         bool           bClone = true);
            void                setFramedText           (LPCTSTR        lpszText);
            void                create                  (LPCCTEXT       lpccNormal,
                                                         LPCCTEXT       lpccPressed,
                                                         LPCCTEXT       lpccHover,
                                                         LPCCTEXT       lpccDisabled);
            void                create                  (const char*    lpccNamePrefix);
            void                setIcons                (CSpritePtr     ptrNormal,
                                                         CSpritePtr     ptrPressed,                                   
                                                         CSpritePtr     ptrHover,                                    
                                                         CSpritePtr     ptrDisabled);
            void                setIconGrayScale        (bool           bGrayScale = true,
                                                         float          fAlpha = 1.f);
            void                setText                 (LPCCTEXT       lpccText);
            void                setTextRgba             (uint32_t       rgbaHex);
            void                setTextScale            (float          fTextScale);
            void                setTextFont             (LPCTSTR        lpszFontName,
                                                         float          fontSize);
            void                setTextOffset           (float          fX,
                                                         float          fY);
            void                setTextOffsetY          (float          fY);
            void                setFgFrame              (LPCCTEXT       lpccFramePath);
            void                setBgFrame              (LPCCTEXT       lpccFramePath);
            void                setIconsPos             (float          fX, 
                                                         float          fY);
            void                setBgFrame              (CSpritePtr     ptrSpr,
                                                         float          fA,
                                                         float          fB,
                                                         float          fC,
                                                         float          fD);
            void                setBgFrameVisible       (bool           bIsVisible);
            void                setState                (eButtonState   eNewState);
            void                setEnabled              (bool           bEnabled = true);
    inline  void                setHoverAnim            (bool           bAnimate = true) { _bHoverAnim = bAnimate; }
    inline  bool                isEnabled               (void)              { return _eState != E_BS_DISABLED; }
    virtual bool                getNotTransBounds       (Rect*          p);
    virtual eMouseCursorType    getMouseCursorType      (void);
            void                setRgbas                (uint32_t       rgbNormal   = 0xE5E5E5FF,
                                                         uint32_t       rgbPressed  = 0xCCCCCCFF,
                                                         uint32_t       rgbHover    = 0xFFFFFFFF,
                                                         uint32_t       rgbDisabled = 0x808080FF);
            auto                getFramedLabel          (void) { return _ptrFramedLabel; }
            void                setShowHoverAnim        (bool           bShow = false) { _bShowHoverAnim = bShow; }
            void                showLockIcon            (bool           bShow);
            auto                getRoot                 (void) { return _root; }

private:
            void                updateState             (void);
protected:                  
    virtual void                onHover                 (void) override;
    virtual void                onLeave                 (void) override;
    virtual void                onPointerDown           (void) override;
    virtual void                onPointerUp             (void) override;
    virtual void                onClick                 (void) override;
public:
    inline static auto          makeInst                (CSpritePtr     ptrNormal,
                                                         CSpritePtr     ptrPressed = nullptr,
                                                         CSpritePtr     ptrHover = nullptr,
                                                         CSpritePtr     ptrDisabled = nullptr,
                                                         uint32_t       nSysId = 0)
    {
        auto ptr = std::make_shared<Button>();
        ptr->create(ptrNormal, ptrPressed, ptrHover, ptrDisabled);
        if (nSysId)
        {
            ptr->setSysId(nSysId);
        }

        return ptr;
    }
};      

typedef std::shared_ptr<Button> ButtonPtr;
typedef std::vector<ButtonPtr> ButtonPtrs;

_G2D_NAMESPACE_END_