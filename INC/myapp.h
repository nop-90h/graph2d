#include "engine.h"
#include "preloader.h"

using namespace g2d;

class ShizGame;
typedef std::shared_ptr<ShizGame> ShizGamePtr;

class MyApp:public App 
{
private:
    PreloaderPtr _ptrPreloader;
    ShizGamePtr  _ptrGame;
public:
    virtual void onFrame(float dt) override;
    virtual void onResize(float cx, float cy) override;
    virtual void init() override;
};