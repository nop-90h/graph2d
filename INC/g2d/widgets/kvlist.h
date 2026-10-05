#pragma once

#include "g2d.h"
#include "widgets/widget.h"
#include "sprite.h"
#include "widgets/staticlabel.h"
#include "widgets/iconedlabel.h"

_G2D_NAMESPACE_BEGIN_

struct KVListItem_t
{
    CContainerPtr   ptrIcon;
    CContainerPtr   ptrKey;
    CContainerPtr   ptrVal;

    KVListItem_t(CContainerPtr icon, CContainerPtr key, CContainerPtr val):ptrIcon(icon), ptrKey(key), ptrVal(val){}
};

struct KVListParams_t
{
    //float fMarginVert   = 5.f;
    float fColumnMargin = 10.f;
    float fLineHeight   = 65.f;
    float fIconHeight   = 60.f;
};

class KVList : public Widget
{
public:
    KVList();
    template <typename... Args>
    void addItemF(CContainerPtr icon, LPCTSTR lpszKey, std::format_string<Args...> fmt, Args&&... args)
    {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        StaticLabelPtr ptrIL = std::make_shared<StaticLabel>();
        ptrIL->setText(lpszKey);    
        StaticLabelPtr ptrSL = std::make_shared<StaticLabel>();
        ptrSL->setText(s.c_str());
        addItem(icon, ptrIL, ptrSL);
    }
    void addItem(CContainerPtr icon, LPCTSTR lpszKey, LPCTSTR lpszVal);
    void addItem(CContainerPtr icon, CContainerPtr ptrKey, CContainerPtr ptrVal) { _v.emplace_back(icon, ptrKey, ptrVal); }
    void build();
    void clear();
    auto& getParams(){return _params;}
protected:

private:
    KVListParams_t              _params;
    CContainerPtr               _ptrKeys;
    CContainerPtr               _ptrVals;
    CContainerPtr               _ptrIcons;
    
    std::vector<KVListItem_t>   _v;
};

typedef std::shared_ptr<KVList> KVListPtr;

_G2D_NAMESPACE_END_