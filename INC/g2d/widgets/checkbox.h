#pragma once

#include "g2d.h"
#include "widget.h"
#include "sprite.h"
#include "spriteloader.h"
#include "iconedlabel.h"

_G2D_NAMESPACE_BEGIN_

class CheckBox : public IconedLabel
{
private:
    bool            _isChecked = false;
    CSpritePtr      _ptrUnchecked;
    CSpritePtr      _ptrChecked;
    SimpleCallback  _cbOnClickUser;

private:
    CheckBox    (LPCTSTR                    lpszText, 
                 bool                       bChecked,
                 std::optional<float>       fIconScale,
                 std::optional<float>       fFontSize,
                 std::optional<std::string> sFont,
                 CSpritePtr                 ptrUnchecked, 
                 CSpritePtr                 ptrChecked):IconedLabel(bChecked ? ptrChecked : ptrUnchecked, lpszText, fIconScale, fFontSize, sFont)
                                                       ,_isChecked(bChecked)
                                                       ,_ptrUnchecked(std::move(ptrUnchecked))
                                                       ,_ptrChecked(std::move(ptrChecked))
    {
        setInteractive(true);
    }

    void updateVisualState()
    {
        setIcon(_isChecked ? _ptrChecked : _ptrUnchecked);
    }

    void handleClick()
    {
        _isChecked = !_isChecked;
        updateVisualState();
        if (_cbOnClickUser)
        {
            _cbOnClickUser();
        }
    }

public:
    CheckBox    (LPCTSTR                    lpszUnchecked, 
                 LPCTSTR                    lpszChecked, 
                 LPCTSTR                    lpszText, 
                 bool                       bChecked   = false,
                 std::optional<float>       fIconScale = std::nullopt,
                 std::optional<float>       fFontSize  = std::nullopt,
                 std::optional<std::string> sFont      = std::nullopt):CheckBox(lpszText, 
                                                                       bChecked, 
                                                                       fIconScale,
                                                                       fFontSize, 
                                                                       sFont, 
                                                                       SpriteLoader::getInstance()->getSprite(lpszUnchecked), 
                                                                       SpriteLoader::getInstance()->getSprite(lpszChecked))
    {
        _onClickCb = [this]
        {
            handleClick();
        };
    }
    bool isChecked() { return _isChecked; }
    void setChecked(bool bIsChecked)
    {
        _isChecked = bIsChecked;
        updateVisualState();
    }
    virtual void setOnClick (SimpleCallback cb) override
    {
        _cbOnClickUser = cb;
    }
};

typedef std::shared_ptr<CheckBox> CheckBoxPtr;

_G2D_NAMESPACE_END_
