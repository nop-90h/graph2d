#include "widgets/widget.h"

_G2D_NAMESPACE_BEGIN_

Widget::Widget()
{
    setPixelSnap(true);
}

Widget::~Widget(void)
{
}

void Widget::update(float dt)
{
    CContainer::update(dt);
    if (_onUpdate)
        _onUpdate();
}

_G2D_NAMESPACE_END_