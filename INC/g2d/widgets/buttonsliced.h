#pragma once

#include "g2d.h"
#include "widgets/widget.h"
#include "widgets/nineslice.h"
#include "widgets/staticlabel.h"
#include "staticstr.h"

_G2D_NAMESPACE_BEGIN_

enum class eButtonSlicedIcon
{
    NONE,
    ONOFF,
    ICON
};

class ButtonSliced;
typedef std::shared_ptr<ButtonSliced> ButtonSlicedPtr;
typedef std::vector<ButtonSlicedPtr> ButtonSlicedPtrs;

class ButtonSliced : public Widget
{
public:
    enum eButtonState
    {
        E_BS_UNINIT,
        E_BS_NORMAL,
        E_BS_HOVER,
        E_BS_PRESSED,
        E_BS_DISABLED
    };
private:
        NineSlicePtr                _bgNormal;
        NineSlicePtr                _bgHL;
        NineSlicePtr                _bgGS;
        StaticLabelPtr              _ptrText;
        eButtonState                _eState = E_BS_UNINIT;
        CContainerPtr               _root;  
        CSpritePtr                  _ptrSprOn;
        CSpritePtr                  _ptrSprOff;
        CContainerPtr               _ptrOnOffCont;
        int                         _nBlinkCounter = 0;
        bool                        _bIsOn = false;
        eButtonSlicedIcon           _eIcon = eButtonSlicedIcon::NONE;
        TweenHandle                 _twHandle;
        std::function<void(bool)>   _doBlink;
public:
                                ButtonSliced            (void){ }
            void                create                  (const char*    lpccNamePrefix,
                                                         float          fA,
                                                         float          fB,
                                                         float          fC,
                                                         float          fD,
                                                         bool           bOnOff = false,
                                                         bool           bIsOn  = false,
                                                         CContainerPtr  ptrIcon = nullptr);
            void                setOnOffState           (bool           bIsOn);
            bool                getOnOffState           (void);
            void                build                   (float          cx,
                                                         float          cy);
            void                setText                 (LPCCTEXT       lpccText,
                                                         float          fAddX = 0,
                                                         float          fAddY = 0);
            void                setHtmlText             (LPCCTEXT       lpccText);
            void                setTextRgba             (uint32_t       rgbaHex);
            void                setState                (eButtonState   eNewState);
            void                setEnabled              (bool           bEnabled = true);
    inline  bool                isEnabled               (void)              { return _eState != E_BS_DISABLED; }
            void                blink                   (void);
            void                cancelBlink             (void);
            auto                getLabel                (void) { return _ptrText; }
    static  void                makeSameWidth           (ButtonSlicedPtrs &v,
                                                         bool             bSetSameX = true);
    static  void                makeSameWidth           (std::span<ButtonSlicedPtr> spn, 
                                                         bool                       bSetSameX);

    virtual bool                getNotTransBounds       (Rect*          p) override;
    //virtual bool                getTransBounds          (Rect*          p) override;
    virtual eMouseCursorType    getMouseCursorType      (void) override;

private:
            void                setTextPos              (void);
            void                updateState             (void);
            void                doBlink                 (void);
protected:                  
    virtual void                onHover                 (void) override;
    virtual void                onLeave                 (void) override;
    virtual void                onPointerDown           (void) override;
    virtual void                onPointerUp             (void) override;
    virtual void                onClick                 (void) override;
};      


template<StaticStr spr>
class ButtonSlicedTmpl : public ButtonSliced
{
public:
    ButtonSlicedTmpl(float cx, float cy, LPCTSTR lpszText, CContainerPtr ptrIcon = nullptr, bool bIsHtml = false)
    {
        setInteractive(true);
        //RenderTracker track;
        create(static_cast<std::string_view>(spr).data(), 20, 20, 20, 20, false, false, ptrIcon);
        build(cx, cy);
        if (bIsHtml)
            setHtmlText(lpszText);
        else
            setText(lpszText);
    }

    inline static auto makeInst(float cx, float cy, LPCTSTR lpszText, SimpleCallback cbOnClick, CContainerPtr ptrIcon = nullptr)
    {
        auto ptr = std::make_shared<ButtonSlicedTmpl>(cx, cy, lpszText, ptrIcon);
        ptr->setOnClick(cbOnClick);
        return ptr;
    }

    inline static auto makeInst(float cy, LPCTSTR lpszText, SimpleCallback cbOnClick, CContainerPtr ptrIcon = nullptr, bool bIsHtml = false)
    {
        constexpr float minButtonCx = 120.f;
        
        Rect rcText;
        
        if (bIsHtml)
            StaticLabel::measureHtml(lpszText, &rcText);
        else
            StaticLabel::measure(&rcText, lpszText);

        if (ptrIcon)
        {
            Rect rcIcon;
            ptrIcon->calcNotTransBounds(&rcIcon);
            rcIcon.scale(ptrIcon->getScaleX(), ptrIcon->getScaleY());
            rcText.cx += rcIcon.cx + 30.f;

        }
        auto cx = std::max(rcText.cx + 40.f, minButtonCx);
        auto ptr = std::make_shared<ButtonSlicedTmpl>(cx, cy, lpszText, ptrIcon, bIsHtml);
        ptr->setOnClick(cbOnClick);
        return ptr;
    }
};

typedef ButtonSlicedTmpl<"UI/slicedOrangeBtn"> OrangeSlicedButton;
typedef ButtonSlicedTmpl<"UI/slicedGreenBtn">  GreenSlicedButton;
typedef ButtonSlicedTmpl<"UI/slicedCyanBtn">   CyanSlicedButton;

typedef std::shared_ptr<GreenSlicedButton>  GreenSlicedButtonPtr;
typedef std::shared_ptr<OrangeSlicedButton> OrangeSlicedButtonPtr;
typedef std::shared_ptr<CyanSlicedButton>   CyanSlicedButtonPtr;

_G2D_NAMESPACE_END_