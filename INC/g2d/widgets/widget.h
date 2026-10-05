#pragma once

#include "g2d.h"
#include "container.h"
#include "sprite.h"

_G2D_NAMESPACE_BEGIN_

class Widget : public CContainer
{
protected:
private:
    SimpleCallback  _onUpdate;
public:
                      Widget            (void);
        virtual       ~Widget           (void);
virtual void          update            (float          dt) override;
        void          setOnUpdate       (SimpleCallback cb) { _onUpdate = cb; }
private:    
        
};

_G2D_NAMESPACE_END_