#include "pch.h"
#include "engine.h"
#include "myapp.h"


int main(int argc, char* argv[])
{
    static MyApp app;
    EngineCfg cfg;
    cfg.setResolution(1672, 941);

    cfg.FONT_TEXTURE_DIM = 2048;

    cfg.addFont("greengoth",     "EMBED/FONTS/greengoth.ttf");
    cfg.addFont("creepster",     "EMBED/FONTS/Creepster.ttf");
    cfg.addFont("trigramlight",  "EMBED/FONTS/TrigramLight.ttf");
    cfg.addFont("edugot",        "EMBED/FONTS/edugot.ttf");
    cfg.addFont("secretorigins", "EMBED/FONTS/YanoneKaffeesatz-Bold.ttf");
    cfg.addFont("marmelad",      "EMBED/FONTS/Marmelad-Regular.ttf");
    
    cfg.addFont("condence",      "EMBED/FONTS/FiraSansCondensed-Medium.ttf");
    
    cfg.DATA_DIR             = "BBB/";
    cfg.DEFAULT_FONT_NAME    = "condence"; 
    cfg.NOVEL_FONT           = "condence";
    cfg.NOVEL_FONT_TEXT_SIZE = 52;

    Engine::getInstance().initAndRun(&app, &cfg);
    return 0;
}