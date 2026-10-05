#pragma once

#include "g2d.h"
#include "dialogs/basedialog.h"

_G2D_NAMESPACE_BEGIN_

class PanelContainer : public BaseDialog
{
public:
    bool init(BaseDialog* pParent, float cx, float cy)
    {
        _bDispatchOutOfClientRect = true;
        return BaseDialog::init(cx, cy, E_ST_NONE, nullptr, nullptr, pParent);
    }
};
typedef std::shared_ptr<PanelContainer> PanelContainerPtr;

_G2D_NAMESPACE_END_