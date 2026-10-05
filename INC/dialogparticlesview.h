#pragma once

#include "basedialog.h"
#include "spriteloader.h"
#include "gfx.h"
#include "buttonsliced.h"
#include "engine.h"
#include "button.h"
#include "particle.h"
#include "checkbox.h"
#include "particlepresets.h"
#include "l10n.h"

class DialogParticlesView : public BaseDialog
{
    enum class eControlType { NONE, ROTATE, REPEL, ATTRACT };
private:
    eParticlePreset    _eParticlePreset = eParticlePreset::FIRE;
    CParticleSystemPtr _ptrParticles;
    CheckBoxPtr        _ptrCheckBoxRotate;
    CheckBoxPtr        _ptrCheckBoxRepel;
    CheckBoxPtr        _ptrCheckBoxAttract;
    CContainerPtr      _ptrButtonsCont;
    StaticLabelPtr     _ptrHintLabel;
    TouchHandlerPtr    _ptrTouchHandler;
    inline static eControlType _eCT = eControlType::NONE;
    void updateControlType()
    {
        switch (_eCT)
        {
        case eControlType::NONE:
            _ptrHintLabel->setVisible(false);
            _ptrCheckBoxRotate->setVisible(true);
            _ptrCheckBoxRepel->setVisible(true);
            _ptrCheckBoxAttract->setVisible(true);
            _ptrParticles->rotate(0);
            _ptrParticles->disableAttractor();
            _ptrParticles->disableRepeller();
            break;
        case eControlType::ROTATE:
            _ptrHintLabel->setVisible(true);
            _ptrHintLabel->setText(L10N::getInstance().tr("PARTICLES_HINT_ROTATE"));
            _ptrCheckBoxRotate->setVisible(true);
            _ptrCheckBoxRepel->setVisible(false);
            _ptrCheckBoxAttract->setVisible(false);
            _ptrParticles->disableAttractor();
            _ptrParticles->disableRepeller();
            break;
        case eControlType::REPEL:
            _ptrHintLabel->setVisible(true);
            _ptrHintLabel->setText(L10N::getInstance().tr("PARTICLES_HINT_REPEL"));
            _ptrCheckBoxRotate->setVisible(false);
            _ptrCheckBoxRepel->setVisible(true);
            _ptrCheckBoxAttract->setVisible(false);
            _ptrParticles->rotate(0);
            _ptrParticles->disableAttractor();
            break;
        case eControlType::ATTRACT:
            _ptrHintLabel->setVisible(true);
            _ptrHintLabel->setText(L10N::getInstance().tr("PARTICLES_HINT_ATTRACT"));
            _ptrCheckBoxRotate->setVisible(false);
            _ptrCheckBoxRepel->setVisible(false);
            _ptrCheckBoxAttract->setVisible(true);
            _ptrParticles->rotate(0);
            _ptrParticles->disableRepeller();
            break;
        }
        _ptrButtonsCont->alignChildren(eChildrenAlign::HORIZONTAL, 20.f, getDialogCx());
    }
    eControlType getControlTypeByCheckBox(CheckBoxPtr ptr)
    {
        if (_ptrCheckBoxAttract == ptr) return eControlType::ATTRACT;
        if (_ptrCheckBoxRepel == ptr) return eControlType::REPEL;
        if (_ptrCheckBoxRotate == ptr) return eControlType::ROTATE;
        return eControlType::NONE;
    }
    void checkBoxClicked(CheckBoxPtr ptrClicked)
    {
        auto eCT = getControlTypeByCheckBox(ptrClicked);
        bool bActivate = ptrClicked->isChecked();
        if (!bActivate) {
            assert(eCT == _eCT);
            _eCT = eControlType::NONE;
        } else {
            assert(eCT != _eCT);
            _eCT = eCT;
        }
        updateControlType();
    }
public:
    void initAndShow(eParticlePreset ePreset)
    {
        auto& cfg = Engine::getCfg();
        _eParticlePreset = ePreset;
        init(cfg.INIT_SCR_CX, cfg.INIT_SCR_CY, eScrollType::E_ST_NONE, nullptr, nullptr);
        _ptrTouchHandler = std::make_shared<TouchHandler>(getDialogCx(), getDialogCy());
        _ptrTouchHandler->setOnMove([this](bool bisPressed, int x, int y){
            switch (_eCT)
            {
            case eControlType::ROTATE: {
                float dx = x - _ptrParticles->getTransX();
                float dy = y - _ptrParticles->getTransY();
                _ptrParticles->rotate(atan2f(-dy, dx));
            } break;
            case eControlType::REPEL: {
                auto pt = InputController::getInstance()->getMousePos();
                _ptrTouchHandler->toLocal(pt);
                _ptrParticles->repelFrom(pt.x - _ptrParticles->getX(), pt.y - _ptrParticles->getY(), 10000, 200);
            } break;
            case eControlType::ATTRACT: {
                auto pt = InputController::getInstance()->getMousePos();
                _ptrTouchHandler->toLocal(pt);
                _ptrParticles->attractTo(pt.x - _ptrParticles->getX(), pt.y - _ptrParticles->getY(), 1000);
            } break;
            }
        });
        auto& ptrSettings = ParticlePresets::getPreset(_eParticlePreset);
        _ptrParticles = std::make_shared<CParticleSystem<>>(ptrSettings, std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(_eParticlePreset)).c_str());
        _ptrParticles->setPos(getDialogCx() * 0.5f, getDialogCy() * 0.5f);
        _ptrButtonsCont = std::make_shared<CContainer>();
        auto ptrCloseBtn = CyanSlicedButton::makeInst(70, L10N::getInstance().tr("BTN_CLOSE"), [this]{ close(); });
        auto ptrBurstBtn = OrangeSlicedButton::makeInst(70, L10N::getInstance().tr("BTN_BURST"), [this]{ _ptrParticles->burst(500); });
        _ptrButtonsCont->addChild(ptrCloseBtn);
        _ptrButtonsCont->addChild(ptrBurstBtn);
        _ptrCheckBoxRotate = std::make_shared<CheckBox>("UI/checkbox_unchecked","UI/checkbox_checked", L10N::getInstance().tr("CHK_ROTATE"), _eCT == eControlType::ROTATE, 0.5f);
        _ptrCheckBoxRotate->setOnClick([this]{ checkBoxClicked(_ptrCheckBoxRotate); });
        _ptrCheckBoxRepel = std::make_shared<CheckBox>("UI/checkbox_unchecked","UI/checkbox_checked", L10N::getInstance().tr("CHK_REPEL"), _eCT == eControlType::REPEL, 0.5f);
        _ptrCheckBoxRepel->setOnClick([this]{ checkBoxClicked(_ptrCheckBoxRepel); });
        _ptrCheckBoxAttract = std::make_shared<CheckBox>("UI/checkbox_unchecked","UI/checkbox_checked", L10N::getInstance().tr("CHK_ATTRACT"), _eCT == eControlType::ATTRACT, 0.5f);
        _ptrCheckBoxAttract->setOnClick([this]{ checkBoxClicked(_ptrCheckBoxAttract); });
        _ptrHintLabel = std::make_shared<StaticLabel>();
        _ptrHintLabel->setVisible(false);
        _ptrButtonsCont->addChild(_ptrCheckBoxRotate);
        _ptrButtonsCont->addChild(_ptrCheckBoxRepel);
        _ptrButtonsCont->addChild(_ptrCheckBoxAttract);
        _ptrButtonsCont->addChild(_ptrHintLabel);
        _ptrButtonsCont->alignChildren(eChildrenAlign::HORIZONTAL, 20.f, getDialogCx());
        _ptrButtonsCont->setY(getDialogCy() - _ptrButtonsCont->calcNotTransCy());
        auto ptrBg = CGfx::getInstance()->spriteFromTexture("BBB/shiz_bg.png");
        _root->addChild(ptrBg);
        _root->addChild(_ptrParticles);
        ptrBg->setPosCentered(getDialogCx(), getDialogCy());
        _root->addChild(_ptrTouchHandler);
        _root->addChild(_ptrButtonsCont);
        updateControlType();
        open();
    }
};