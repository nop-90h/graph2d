#pragma once

#include "g2d.h"
#include "widgets/widget.h"
#include "widgets/staticlabel.h"

_G2D_NAMESPACE_BEGIN_

class IconedLabel : public Widget
{
public:
                        IconedLabel             (CContainerPtr              ptrIcon,
                                                 LPCTSTR                    lpszText,
                                                 std::optional<float>       fIconScale = std::nullopt,
                                                 std::optional<float>       fFontSize  = std::nullopt,
                                                 std::optional<std::string> sFont      = std::nullopt);
            void        setText                 (LPCTSTR                    lpszText);
            void        setTextBox              (bool                       bSet,
                                                 float                      fBoxCx = 0.f);
            void        setIcon                 (CContainerPtr              ptrIcon);
            void        setTextColor            (float                      r,
                                                 float                      g,
                                                 float                      b,
                                                 float                      a = 1.f)
            {
                _ptrLabel->setTint(r, g, b);
                _ptrLabel->setAlpha(a);
            }

public: //CContainer

    virtual bool        getNotTransBounds       (Rect*        p) override;


protected:
            void        updatePositions         (void);
private:
    StaticLabelPtr          _ptrLabel;
    CContainerPtr           _ptrIcon;
    std::optional<float>    _fIconScale;
};

typedef std::shared_ptr<IconedLabel> IconedLabelPtr;

_G2D_NAMESPACE_END_