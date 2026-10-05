#pragma once
#include "g2d.h"
#include "dialogs/panelhtml.h"
#include "widgets/staticlabel.h"
#include "gfx.h"
#include <unordered_map>
#include <string>
#include <memory>

struct PanelHTMLDemoVolumeCallbacks;

class PanelHTMLDemo : public PanelHTML
{
    enum class eSpineGirlState
    {
        IDLE,
        CHARGING,
        IDLE_CHARGED,
    };

    struct PPBind
    {
        const char* name  = nullptr;
        const char* group = nullptr;
        unsigned long long id      = 0;
        unsigned long long valId   = 0;
        unsigned long long labelId = 0;
        float min = 0.f, max = 1.f;
        int dec = 2;
        float PostProcessSettings::* pf = nullptr;
        float BloomSettings::*       bf = nullptr;
    };

    struct PPCheckBind
    {
        const char* name  = nullptr;
        const char* group = nullptr;
        unsigned long long id = 0;
        bool PostProcessSettings::* pb = nullptr;
        bool BloomSettings::*       bb = nullptr;
    };

    struct VolumeBind
    {
        const char* name = nullptr;
        unsigned long long id      = 0;
        unsigned long long valId   = 0;
        unsigned long long labelId = 0;
        float min = 0.f;
        float max = 1.f;
        int   dec = 2;
    };

private:
    eSpineGirlState    _eAlianna    = eSpineGirlState::IDLE;
    eSpineGirlState    _eAmarantha  = eSpineGirlState::IDLE;

    // --- Данные панели пост-процесса ---
    PostProcessSettings _pp;
    bool                _bIsMusicPlaying   = false;

    unsigned long long  _ppMaskSelect      = 0;
    unsigned long long  _ppMaskSelectLabel = 0;
    unsigned long long  _ppResetBtn        = 0;
    unsigned long long  _ppMasterCheck     = 0;
    unsigned long long  _ppBloomCheck      = 0;

    std::unordered_map<unsigned long long, PPBind>      _ppSliders;
    std::unordered_map<unsigned long long, PPCheckBind> _ppChecks;

    // --- Данные панели громкости ---
    unsigned long long _volRootId = 0;

    std::unordered_map<unsigned long long, VolumeBind>            _volSliders;
    std::unordered_map<unsigned long long, unsigned long long>    _volButtonLabels;

    float _volMaster = 1.f;
    float _volMusic  = 1.f;
    float _volSounds = 1.f;


    void initPPPage();
    void applyPP();
    void pushPPToDom();
    void refreshPPEnabled();
    void updateSliderLabel(const PPBind& b, float v);

    void initVolumePage();
    void onVolumeHover(unsigned long long nodeId);
    void onVolumeLeave(unsigned long long nodeId);
    void onVolumeSliderChanged(unsigned long long nodeId, float value);

    friend struct PanelHTMLDemoVolumeCallbacks;

public:
    ~PanelHTMLDemo();

    void init(int nTextIdx,
              float cx,
              float cy,
              CContainerPtr ptrParent);
};

using PanelHtmlDemoPtr = std::shared_ptr<PanelHTMLDemo>;