#include "pch.h"
#include <algorithm>
#include <iterator>
#include "myapp.h"
#include "spinecont.h"
#include "gfx.h"
#include "basedialog.h"
#include "novel.h"
#include "button.h"
#include "beheading.h"
#include "dialogdemo.h"
#include "touchhandler.h"
#include "audiomanager.h"
#include "particle.h"
#include "l10n.h"
#include "bakedspine.h"

class MitaDragTutor : public BaseDialog
{
public:
	bool init()
	{
		auto ptrTutorSpr = SpriteLoader::getInstance()->getSprite("SHIZ/drag_tutor");
		float cx = 900;
		float cy = 800;
		Rect chRect(20, 20, cx - 40, cy - 40);
		CSpritePtr pFrameBg = SpriteLoader::getInstance()->getSprite("UI/frameBg");
		bool bRes = BaseDialog::init(cx, cy, eScrollType::E_ST_NONE, NULL, pFrameBg);
		if (bRes)
		{
			auto ptrCont = std::make_shared<CContainer>();
			ptrCont->addChild(ptrTutorSpr);
			auto ptrOk = GreenSlicedButton::makeInst(80, L10N::getInstance().tr("BTN_UNDERSTOOD"), [this] { close(); });
			ptrCont->addChild(ptrOk);
			ptrOk->setY(ptrTutorSpr->getNotTransCy() + 30.f);
			ptrOk->setXPosCentered(ptrTutorSpr->getNotTransCx());
			_root->addChild(ptrCont);
			ptrCont->setPosCentered(getDialogCx(), getDialogCy(), 0, 50);
			createFgLayer(SpriteLoader::getInstance()->getSprite("UI/frameFg"), 60, 60, 46, 46);
			auto ptrSL = SpriteLoader::getInstance();
			createCloseButton(ptrSL->getSprite("UI/btn_close_normal"), ptrSL->getSprite("UI/btn_close_pressed"),
				ptrSL->getSprite("UI/btn_close_hovered"), ptrSL->getSprite("UI/btn_close_disabled"));
			CSpritePtr pTitleFrame = SpriteLoader::getInstance()->getSprite("UI/frameTitle");
			setTitle(L10N::getInstance().tr("MEDBOT_TITLE"), pTitleFrame, 113, 113);
			_titleCont->setY(-20.f);
		}
		return bRes;
	}
};

class CubeClosedDialog : public BaseDialog
{
private:
	CSpinePtr               _ptrCube;
	OrangeSlicedButtonPtr   _ptrBtn;
	StaticLabelPtr          _ptrTitle;
public:
	void init()
	{
		auto& cfg = Engine::getCfg();
		float cx = cfg.INIT_SCR_CX;
		float cy = cfg.INIT_SCR_CY;
		bool bRes = BaseDialog::init(cx, cy, eScrollType::E_ST_NONE, NULL, NULL);
		if (bRes)
		{
			_ptrTitle = std::make_shared<StaticLabel>();
			_ptrTitle->setFontSize(78);
			_ptrTitle->setTint(1, 0, 0);
			_ptrTitle->setBoxMode(eTextRenderType::BOX, getDialogCx() - 100.f);
			_ptrTitle->setY(120.f);
			_ptrTitle->setText(L10N::getInstance().tr("CUBE_CLOSED_DESC"));
			_root->addChild(_ptrTitle);
			_ptrTitle->setXPosCentered(getDialogCx());
			_ptrCube = CSpineManager::getInstance()->getNewSpine("BBB/CUBE_CLOSED");
			_ptrCube->setSkipLight(true);
			_ptrCube->setScale(1.3f, 1.3f);
			_ptrCube->setPos(cx * 0.5f, cy * 0.5f);
			_root->addChild(_ptrCube);
			_ptrCube->setAnimation(0, "idle", true);
			_ptrBtn = OrangeSlicedButton::makeInst(70, L10N::getInstance().tr("BTN_TOUCH_CUBE"), [this] {
				_ptrTitle->addSelfTween(eTweenProp::ALPHA, 1.f, 0.f, 2.f);
				_ptrBtn->setVisible(false);
				_ptrCube->setAnimation(0, "tounge", false);
				_ptrCube->addAnimation(0, "idle_tounge", true);
				_ptrCube->onComplete([this](CSpine* pOwner, const char* lpccAnimName) {
					if (!strcmp("tounge", lpccAnimName))
					{
						setTimeout(2.f, [this] {
							close();
						});
					}
				});
			});
			_ptrBtn->setY(750);
			_ptrBtn->setXPosCentered(getDialogCx());
			_root->addChild(_ptrBtn);
		}
	}
};

class YesNoDialog : public BaseDialog
{
private:
public:
	void init(LPCTSTR lpszText, LPCTSTR lpszYes, LPCTSTR lpszNo, SimpleCallback cbOnYes, SimpleCallback cbOnNo)
	{
		auto& cfg = Engine::getCfg();
		float cx = cfg.INIT_SCR_CX;
		float cy = cfg.INIT_SCR_CY;
		bool bRes = BaseDialog::init(cx, cy, eScrollType::E_ST_NONE, NULL, NULL);
		if (bRes)
		{
			auto ptrCont = std::make_shared<CContainer>();
			auto ptrTitle = std::make_shared<StaticLabel>();
			ptrTitle->setFontSize(78);
			ptrTitle->setTint(1, 0, 0);
			ptrTitle->setBoxMode(eTextRenderType::BOX, getDialogCx() - 100.f);
			ptrTitle->setY(120.f);
			ptrTitle->setText(lpszText);
			ptrCont->addChild(ptrTitle);
			auto ptrBtnCont = std::make_shared<CContainer>();
			auto ptrBtnY = GreenSlicedButton::makeInst(70, lpszYes, [this, cbOnYes] {
				close();
				if (cbOnYes)
					cbOnYes();
			});
			auto ptrBtnN = OrangeSlicedButton::makeInst(70, lpszNo, [this, cbOnNo] {
				close();
				if (cbOnNo)
					cbOnNo();
			});
			ptrBtnCont->addChild(ptrBtnY);
			ptrBtnCont->addChild(ptrBtnN);
			ptrBtnCont->alignChildren(eChildrenAlign::HORIZONTAL, 30.f, getDialogCx());
			ptrCont->addChild(ptrBtnCont);
			ptrCont->alignChildren(eChildrenAlign::VERTICAL_COLUMN, 100.f, getDialogCx());
			ptrCont->setPosCentered(getDialogCx(), getDialogCy());
			_root->addChild(ptrCont);
			ptrCont->setPosCentered(getDialogCx(), getDialogCy());
		}
	}
};

class CubeTpDialog : public BaseDialog
{
private:
	CSpinePtr               _ptrCube;
public:
	void init()
	{
		auto& cfg = Engine::getCfg();
		float cx = cfg.INIT_SCR_CX;
		float cy = cfg.INIT_SCR_CY;
		bool bRes = BaseDialog::init(cx, cy, eScrollType::E_ST_NONE, NULL, NULL);
		if (bRes)
		{
			_ptrCube = CSpineManager::getInstance()->getNewSpine("BBB/CUBE_OPEN");
			_ptrCube->setSkipLight(true);
			_ptrCube->setScale(1.3f, 1.3f);
			_ptrCube->setPos(cx * 0.5f, cy * 0.5f);
			_root->addChild(_ptrCube);
			_ptrCube->setAnimation(0, "animation", false);
			_ptrCube->onComplete([this](CSpine* pOwner, const char* lpccAnimName) {
				if (!strcmp("animation", lpccAnimName))
				{
					close();
				}
			});
		}
	}
};

class MitaDiagnosis : public BaseDialog
{
private:
	StaticLabelPtr _ptrTitle;
	CContainerPtr  _ptrCont;
	CSpritePtr     _ptrCool;
public:
	void showText(int nDiagStep)
	{
		auto ptrLine = std::make_shared<StaticLabel>();
		ptrLine->setFont("trigramlight");
		ptrLine->setFontSize(60);
		ptrLine->setText(nDiagStep == 0 ? L10N::getInstance().tr("DIAG_RESULT_0") : L10N::getInstance().tr("DIAG_RESULT_1"));
		ptrLine->setAlpha(0);
		ptrLine->addSelfTween(eTweenProp::ALPHA, 0, 1.f, 1.f, Easing::linear, 2 + 0.5f);
		_ptrCont->addChild(ptrLine);

		ptrLine = std::make_shared<StaticLabel>();
		ptrLine->setFont("trigramlight");
		ptrLine->setFontSize(44);
		ptrLine->setTint(1, 0, 0);
		ptrLine->setText(nDiagStep == 0 ? L10N::getInstance().tr("DIAG_ANOMALY_0") : L10N::getInstance().tr("DIAG_ANOMALY_1"));
		ptrLine->setAlpha(0);
		ptrLine->addSelfTween(eTweenProp::ALPHA, 0, 1.f, 1.f, Easing::linear, 2 + 1.5f);
		_ptrCont->addChild(ptrLine);

		ptrLine = std::make_shared<StaticLabel>();
		ptrLine->setFont("trigramlight");
		ptrLine->setFontSize(44);
		ptrLine->setText(nDiagStep == 0 ? L10N::getInstance().tr("DIAG_CONCLUSION_0") : L10N::getInstance().tr("DIAG_CONCLUSION_1"));
		ptrLine->setAlpha(0);
		ptrLine->addSelfTween(eTweenProp::ALPHA, 0, 1.f, 1.f, Easing::linear, 2 + 1.5f);
		_ptrCont->addChild(ptrLine);

		ptrLine = std::make_shared<StaticLabel>();
		ptrLine->setFont("trigramlight");
		ptrLine->setFontSize(44);
		ptrLine->setText(nDiagStep == 0 ? L10N::getInstance().tr("DIAG_ACTION_0") : L10N::getInstance().tr("DIAG_ACTION_1"));
		ptrLine->setAlpha(0);
		ptrLine->addSelfTween(eTweenProp::ALPHA, 0, 1.f, 1.f, Easing::linear, 2 + 1.5f, [this, nDiagStep] {
			auto ptrDummy = std::make_shared<TouchHandler>(getDialogCx(), getDialogCy());
			ptrDummy->setOnClick([this, ptrDummy, nDiagStep] {
				if (nDiagStep == 0)
				{
					ptrDummy->removeFromParent();
					_ptrTitle->setText(L10N::getInstance().tr("DIAG_HACKED_BOT"));
					_ptrTitle->setXPosCentered(getDialogCx());
					_ptrTitle->setScale(0.1f, 0.1f);
					_ptrTitle->addSelfTween(eTweenProp::SCALE, 0.1f, 1.f, 1.f, Easing::outBounce, 0.f, [this, ptrDummy] {
						auto ptrLine = std::make_shared<StaticLabel>();
						ptrLine->setFont("trigramlight");
						ptrLine->setFontSize(60);
						ptrLine->setText("VIRUS   VIRUS   VIRUS   VIRUS   VIRUS   VIRUS   VIRUS   VIRUS   VIRUS   VIRUS   VIRUS   VIRUS   VIRUS");
						ptrLine->setTint(1.f, 0.f, 0.f);
						ptrLine->setAlpha(0);
						auto w = ptrLine->weak_from_this();
						ptrLine->addSelfTween(eTweenProp::ALPHA, 0, 1.f, 1.f, Easing::linear, 2 + 0.5f, [this, w] {
							if (auto p = w.lock())
							{
								p->addSelfTween(eTweenProp::X, p->getX(), 1500, 5.f, Easing::linear, 0, {}, nullptr, false);
							}
						});
						_ptrCont->addChild(ptrLine);
						auto ptrL1 = ptrLine;

						ptrLine = std::make_shared<StaticLabel>();
						ptrLine->setFont("trigramlight");
						ptrLine->setFontSize(44);
						ptrLine->setTint(1, 0, 0);
						ptrLine->setText(L10N::getInstance().tr("DIAG_EXTERNAL_CONTROL"));
						ptrLine->setAlpha(0);
						ptrLine->addSelfTween(eTweenProp::ALPHA, 0, 1.f, 1.f, Easing::linear, 2 + 1.5f);
						_ptrCont->addChild(ptrLine);

						ptrLine = std::make_shared<StaticLabel>();
						ptrLine->setFont("trigramlight");
						ptrLine->setFontSize(80);
						ptrLine->setText(L10N::getInstance().tr("DIAG_LIQUIDATE"));
						ptrLine->setAlpha(0);
						ptrLine->addSelfTween(eTweenProp::ALPHA, 0, 1.f, 1.f, Easing::linear, 2 + 1.5f, [this, w = ptrLine->weak_from_this()] {
							if (auto p = w.lock())
							{
								p->addSelfTween(eTweenProp::ALPHA, 1.f, 0.25f, 1.f, Easing::linear, 0.f, {}, nullptr, true);
							}
						});
						_ptrCont->addChild(ptrLine);

						ptrLine = std::make_shared<StaticLabel>();
						ptrLine->setFont("trigramlight");
						ptrLine->setFontSize(44);
						ptrLine->setText(L10N::getInstance().tr("DIAG_TERMINATOR"));
						ptrLine->setAlpha(0);
						ptrLine->addSelfTween(eTweenProp::ALPHA, 0, 1.f, 1.f, Easing::linear, 2 + 1.5f, [this, ptrDummy] {
							_root->addChild(ptrDummy);
							ptrDummy->setOnClick([this] {
								CGfx::getInstance()->getPostProcessSettings().grainIntensity   = 0.1f;
								CGfx::getInstance()->getPostProcessSettings().fisheyeIntensity = 0.f;
								CGfx::getInstance()->getPostProcessSettings().lutIntensity     = 0.f;
								CGfx::getInstance()->getPostProcessSettings().aberration       = 0.002f;
								CGfx::getInstance()->setWorldDarken(false);
								close();
							});
						});
						_ptrCont->addChild(ptrLine);
						_ptrCont->alignChildren(eChildrenAlign::VERTICAL_COLUMN, 20.f, getDialogCx(), true);
						_ptrCont->setPosCentered(getDialogCx(), getDialogCy(), 0, 50);
						ptrL1->setX(-2500);
					});
					_ptrTitle->setTint(1.f, 0.f, 0.f);
					_ptrCont->removeAll();
				}
				else
				{
					close();
				}
			});
			_root->addChild(ptrDummy);
		});
		_ptrCont->addChild(ptrLine);
		_ptrCont->alignChildren(eChildrenAlign::VERTICAL_COLUMN, 20.f, getDialogCx(), true);
		_ptrCont->setPosCentered(getDialogCx(), getDialogCy(), 0, 50);
		_root->addChild(_ptrCont);
	}

	bool init(int nDiagStep = 0)
	{
		auto& cfg = Engine::getCfg();
		float cx = cfg.INIT_SCR_CX;
		float cy = cfg.INIT_SCR_CY;
		bool bRes = BaseDialog::init(cx, cy, eScrollType::E_ST_NONE, NULL, NULL);
		if (bRes)
		{
			_ptrCool = SpriteLoader::getInstance()->getSprite("UI/cool");
			_ptrCool->setPivotCentered();
			_ptrCool->setPosPivotCentered(getDialogCx(), getDialogCy(), 0, 100);
			_root->addChild(_ptrCool);
			_ptrCool->setAlpha(0);
			_ptrCool->setScale(0.1f, 0.1f);
			_ptrCont = std::make_shared<CContainer>();
			_ptrTitle = std::make_shared<StaticLabel>();
			_ptrTitle->setFont("trigramlight");
			_ptrTitle->setFontSize(78);
			_ptrTitle->setText(nDiagStep == 0 ? L10N::getInstance().tr("DIAG_SEX_BOT") : L10N::getInstance().tr("DIAG_HACKED_BOT"));
			if (nDiagStep == 0)
				_ptrTitle->setTint(0.5f, 1.f, .5f);
			else
				_ptrTitle->setTint(1.f, 0.f, 0.f);
			_ptrTitle->setAlpha(0);
			_ptrTitle->addSelfTween(eTweenProp::ALPHA, 0, .7f, 2.5f, Easing::outBounce, 0.f);
			_root->addChild(_ptrTitle);
			_ptrTitle->setXPosCentered(getDialogCx());
			_ptrTitle->setY(200);
			if (nDiagStep == 0)
			{
				showText(nDiagStep);
			}
			else
			{
				_ptrCool->addSelfTween(eTweenProp::ALPHA, _ptrCool->getAlpha(), .7f, 0.3f);
				_ptrCool->addSelfTween(eTweenProp::SCALE, _ptrCool->getScaleX(), .7f, .7f, Easing::outElastic, 0.f, [this, nDiagStep] {
					setTimeout(1.f, [this, nDiagStep] {
						_ptrCool->addSelfTween(eTweenProp::X, _ptrCool->getX(), 100, 0.3f);
						_ptrCool->addSelfTween(eTweenProp::Y, _ptrCool->getY(), 0, 0.3f);
						_ptrCool->addSelfTween(eTweenProp::SCALE, _ptrCool->getScaleX(), 0.4f, 0.3f);
						showText(nDiagStep);
					});
				});
			}
		}
		return bRes;
	}
};

class ShizGame : public BaseDialog
{
	enum class eShizGameState
	{
		INITIAL,
		INTRO,
		DIALING,
		DRIVING
	};
private:
	enum class eLastGameState
	{
		BASIC,
		MOVING_TO_OBJ,
		EXTING,
		WAITING_LAST_TORCH,
		OPEN_ANIM_TORCHES,
		OPEN_ANIM_CUBE,
	};
	eLastGameState                _eLastGameState = eLastGameState::BASIC;
	CContainerPtr                 _ptrGameHolder;
	CSpritePtr                    _ptrShizBgU;
	CSpritePtr                    _ptrShizBgB;
	CContainerPtr                 _ptrShizBgCont;
	CSpritePtr                    _ptrShizBg2;
	CSpritePtr                    _ptrNotebook;
	CSpritePtr                    _ptrNotebookBurned;
	CSpinePtr                     _ptrShiz;
	CSpinePtr                     _ptrLamp;
	CParticleSystemPtr            _ptrFlies;
	CParticleSystemPtr            _ptrFlies2;
	CSpinePtr                     _ptrAmbulance;
	CSpinePtr                     _ptrPhoneCall;
	CSpinePtr                     _ptrDragon;
	CBakedSpinePtr                _ptrIntro;
	CSpinePtr                     _ptrMita;
	CContainerPtr                 _ptrEnterCubeBgCont;
	CSpinePtr                     _ptrEnterCubeBg;
	CSpinePtr                     _ptrSanctuaryBg;
	CSpinePtr                     _ptrCubeClosed;
	CSpinePtr                     _ptrAngel;
	CSpinePtr                     _ptrBarbarian;
	CSpinePtr                     _ptrFires[4];
	CContainerPtr                 _ptrMainMenu;
	NineSlicePtr                  _ptrSubMenuFrame;
	CContainerPtr                 _ptrGameSubMenu;
	TouchHandlerPtr               _ptrTouchHandler;
	TouchHandlerPtr               _ptrCubeHandler;
	CContainerPtr                 _ptrIntroLayer;
	CContainerPtr                 _ptrDialLayer;
	CContainerPtr                 _ptrDriveLayer;
	ButtonPtr                     _ptrBtnWatch;
	ButtonPtr                     _ptrBtnGames;
	CContainerPtr                 _ptrBtnDialCont;
	NineSlicePtr                  _ptrMenuFrame;
	eShizGameState                _eState = eShizGameState::INITIAL;
	bool                          _torchesActivated[4]  = { 0 };
	int                           _torchAtivateOrder[4] = { 0 };
	int                           _nKeyIdx = 0;
	bool                          _bTrackEye = false;
	Rect                          _rcMoveArea;
	bool                          _isBookTaken = false;
	bool                          _bIntoAnimShown = false;
	int                           _nFunGameSelected = 0;

private:
	void setPostProcessing()
	{
		static PostProcessSettings config;
		static bool bInitialized = false;
		if (!bInitialized) {
			bInitialized = true;
			config.enabled = true;
			config.contrast = 1.0f;
			config.brightness = 0.f;
			config.saturation = 1.f;
			config.envSaturation = 0.f;
			config.acesIntensity = 0.f;
			config.tintR = 1.f; config.tintG = 1.f; config.tintB = 1.00f;
			config.lutIntensity = 0.f;
			config.aoIntensity = 0.f;
			config.bloom.enabled = true;
			config.bloom.threshold = 0.90f;
			config.bloom.intensity = 0.15f;
			config.bloom.radius = 0.6f;
			config.grainIntensity = 0.1f;
			config.aberration = 0.002f;
			config.chromaticVig = 0;
			config.vignetteRadius = 0.45f;
			config.vignetteSmoothness = 0.55f;
			config.vignetteIntensity = 0.50f;
			config.ditherIntensity = 0.02f;
		}
		CGfx::getInstance()->getPostProcessSettings() = config;
	}
public:
	~ShizGame()
	{
		int a = 0;
	}

	virtual void update(float dt) override
	{
		BaseDialog::update(dt);
	}

	void initLights()
	{
		LightLayerArray arrLamp;
		arrLamp[0].bActive = true;
		arrLamp[0].nLayer  = 1;
		arrLamp[1].bActive = true;
		arrLamp[1].nLayer  = 0;
		arrLamp[1].fMul    = 0.05f;
		arrLamp[2].bActive = true;
		arrLamp[2].nLayer  = 2;
		arrLamp[2].fMul    = 1.f;

		LightLayerArray arrAmbulance;
		arrAmbulance[0].bActive = true;
		arrAmbulance[0].nLayer  = 0;
		arrAmbulance[1].bActive = true;
		arrAmbulance[1].nLayer  = 1;
		arrAmbulance[1].fMul    = 0.1f;

		_ptrLamp->setLightAttachment("light",            true);
		_ptrLamp->setLightAttachment("lightspot",        true);
		_ptrLamp->setLightAttachment("lightspot2",       true);
		_ptrLamp->setLightEmmiterSettings(arrLamp);

		_ptrAmbulance->setLightAttachment("bluelight",   true);
		_ptrAmbulance->setLightAttachment("lightspot2",  true);
		_ptrAmbulance->setLightAttachment("headlight",   true);
		_ptrAmbulance->setLightAttachment("lightspot",   true);
		_ptrAmbulance->setLightAttachment("lightspot_headlights", true);
		_ptrAmbulance->setLightAttachment("STOP_1",      true);
		_ptrAmbulance->setLightAttachment("STOP_2",      true);
		_ptrAmbulance->setLightEmmiterSettings(arrAmbulance);

		_ptrDragon->setLightAttachment("body_light",     true);
		_ptrDragon->setLightAttachment("flame_throw",    true);
		_ptrDragon->setLightAttachment("lightspot",      true);
		_ptrDragon->setLightAttachment("effect_rays",    true);

		_ptrEnterCubeBg->addRadialLight(200.f, "LIGNING1l");
		_ptrEnterCubeBg->addRadialLight(500.f, "LIGHTNING2l").fIntensity = 2.f;
		_ptrEnterCubeBg->addRadialLight(3500.f, "TAIL_LIGHT", "TAIL_LIGHT").fIntensity = 1.2f;

		LightLayerArray arr;
		arr[0].bActive = true;
		arr[0].fMul = 1;
		arr[1].bActive = true;
		arr[1].nLayer = 1;
		arr[1].fMul = 0.3f;
		_ptrEnterCubeBg->setLightEmmiterSettings(arr);

		for (int i = 0; i < 6; i++)
		{
			if (!i)
			{
				_ptrEnterCubeBg->addRadialLight(500.f, "CIRCLE", "CIRCLE").fIntensity = 1.f;
			}
			else
			{
				auto s = std::format("CIRCLE{}", i + 1);
				_ptrEnterCubeBg->addRadialLight(500.f, s.c_str(), s.c_str()).fIntensity = 1.f;
			}
		}
		for (int i = 0; i < 28; i++)
		{
			if (i > 0)
			{
				auto s = std::format("dot{}", i + 1);
				_ptrEnterCubeBg->addRadialLight(150.f, s.c_str(), s.c_str()).fIntensity = 1.f;
			}
			else
			{
				_ptrEnterCubeBg->addRadialLight(150.f, "dot", "dot").fIntensity = 1.f;
			}
		}
		for (int i = 0; i < SIZE_OF(_ptrFires); i++)
		{
			_ptrFires[i]->addRadialLight(300, "fire").color.set(1.f, 0.7f, 0.1f);
			_ptrFires[i]->addRadialLight(1000, "fire", nullptr, 0.5f).color.set(1.f, 0.7f, 0.1f);
		}
	}

	void playDial(SimpleCallback cbOnComplete)
	{
		assert(_eState == eShizGameState::INTRO);
		_eState = eShizGameState::DIALING;
		if (_nFunGameSelected == 1)
			_ptrPhoneCall->setSkin("pudge");
		if (_nFunGameSelected == 1)
		{
			CGfx::getInstance()->getPostProcessSettings().fisheyeIntensity = 1.f;
			CGfx::getInstance()->getPostProcessSettings().grainIntensity   = .1f;
			addSelfTween(eTweenProp::FISH_EYE, 1.f, 0.7f, .75f, Easing::outElastic, 0.f, [this] {
				addSelfTween(eTweenProp::FISH_EYE, 0.7f, .4f, .75f, Easing::outElastic, 0.f, [this] {
					addSelfTween(eTweenProp::FISH_EYE, 0.4f, .2f, .75f, Easing::outElastic, 0.f, [this] {
						addSelfTween(eTweenProp::FISH_EYE, 0.2f, .0f, .5f, Easing::linear);
					});
				});
			});
		}
		_ptrGameHolder->removeAll();
		_ptrGameHolder->addChild(_ptrDialLayer);
		_ptrPhoneCall->onComplete([this, cbOnComplete](CSpine* pOwner, const char* lpccAnimName) {
			setTimeout(2.f, [this, cbOnComplete] {
				float fDarken = .2f;
				NovelEntry nov[] = {
					{
						.fBgShowPause    = 1.f,
						.lpszBg          = _nFunGameSelected == 0 ? "BBB/calling_doctor.jpg" : "BBB/calling_doctor2.jpg",
						.lpszSpeakerPic  = nullptr,
						.lpszSpeakerText = _nFunGameSelected == 0 ? L10N::getInstance().tr("DIAL_SPEAKER1_0") : L10N::getInstance().tr("DIAL_SPEAKER1_1"),
						.fYTextCorrection = 0.f,
						.fWorldDarken    = fDarken,
						.plszCenterPic   = nullptr,
						.fPicX           = 400.f,
						.fPicY           = -200.f,
						.fPicScale       = 0.7f,
						.answers         =
						{
							{
								.lpszAnswerTitle = _nFunGameSelected == 0 ? L10N::getInstance().tr("DIAL_ANSWER1_0") : L10N::getInstance().tr("DIAL_ANSWER1_1"),
								.lpszText        = nullptr,
								.pNext           = nov + 1,
							},
						},
					},
					{
						.fBgShowPause    = 0.f,
						.lpszBg          = _nFunGameSelected == 0 ? "BBB/calling_doctor.jpg" : "BBB/calling_doctor2.jpg",
						.lpszSpeakerPic  = nullptr,
						.lpszSpeakerText = _nFunGameSelected == 0 ? L10N::getInstance().tr("DIAL_SPEAKER2_0") : L10N::getInstance().tr("DIAL_SPEAKER2_1"),
						.fYTextCorrection = 0.f,
						.fWorldDarken    = fDarken,
						.plszCenterPic   = nullptr,
						.fPicX           = 400.f,
						.fPicY           = -200.f,
						.fPicScale       = 0.7f,
						.answers         =
						{
							{
								.lpszAnswerTitle   = _nFunGameSelected == 0 ? L10N::getInstance().tr("DIAL_ANSWER2_0") : L10N::getInstance().tr("DIAL_ANSWER2_1"),
								.lpszText          = nullptr,
								.pNext             = nullptr,
							},
						},
					},
				};
				auto ptrDlg = std::make_shared<Novel>();
				ptrDlg->init(nov);
				ptrDlg->open([this, cbOnComplete] {
					cbOnComplete();
				});
			});
		});
		_ptrPhoneCall->setAnimation(0, "animation", false);
	}

	void cleanupDrive()
	{
		_ptrAmbulance->removeFromParent();
		_ptrDragon->removeFromParent();
		_ptrLamp->removeFromParent();
		_ptrShizBgCont->removeFromParent();
	}

	void switchLastGameState(eLastGameState eNewState)
	{
		switch (eNewState)
		{
		case eLastGameState::MOVING_TO_OBJ:
		{
			assert(_eLastGameState == eLastGameState::BASIC);
			_eLastGameState = eLastGameState::MOVING_TO_OBJ;
		}
		break;
		case eLastGameState::BASIC:
		{
			assert(_eLastGameState != eLastGameState::BASIC);
			_eLastGameState = eLastGameState::BASIC;
		}
		break;
		case eLastGameState::EXTING:
		{
			assert(_eLastGameState == eLastGameState::BASIC);
			_eLastGameState = eLastGameState::EXTING;
		}
		break;
		case eLastGameState::WAITING_LAST_TORCH:
		{
			_eLastGameState = eLastGameState::WAITING_LAST_TORCH;
		}
		break;
		}
	}

	void moveHeroToPoint(CSpinePtr ptrHero, Point pt, float fSpeed = 300.f, SimpleCallback cbOnComplete = {})
	{
		if (_eLastGameState == eLastGameState::MOVING_TO_OBJ)
			return;
		_rcMoveArea.clipPoint(pt);
		if (cbOnComplete)
		{
			switchLastGameState(eLastGameState::MOVING_TO_OBJ);
		}
		if (ptrHero->isFlippedX() && pt.x > ptrHero->getX() ||
			!ptrHero->isFlippedX() && pt.x < ptrHero->getX())
		{
			ptrHero->flipX();
		}
		ptrHero->removeSelfTweens();
		ptrHero->setAnimation(0, "run", true);
		float fDx       = pt.x - ptrHero->getX();
		float fDy       = pt.y - ptrHero->getY();
		float fDistance = std::sqrtf(fDx * fDx + fDy * fDy);
		float fTime     = fDistance / fSpeed;
		ptrHero->addSelfTween(eTweenProp::X, ptrHero->getX(), pt.x, fTime, Easing::linear);
		ptrHero->addSelfTween(eTweenProp::Y, ptrHero->getY(), pt.y, fTime, Easing::linear, 0.f, [this, cbOnComplete, ptrHero] {
			ptrHero->setAnimation(0, "idle", true);
			if (cbOnComplete)
			{
				switchLastGameState(eLastGameState::BASIC);
				cbOnComplete();
			}
		});
	}

	void fireTheTorch(int nTorchToFire, SimpleCallback cb = {})
	{
		_ptrFires[nTorchToFire]->setVisible(true);
		_ptrFires[nTorchToFire]->addAnimation(0, "fire_start", false);
		_ptrFires[nTorchToFire]->addAnimation(0, "fire_idle", true);
		if (nTorchToFire == 1 || nTorchToFire == 2)
		{
			_ptrEnterCubeBg->setAnimation(0, "torch_activated", false);
			_ptrEnterCubeBg->addAnimation(0, "torches_shown",   true);
		}
		_ptrFires[nTorchToFire]->onComplete([this, cb](CSpine* pOwner, const char* lpccAnimName) {
			if (!strcmp("fire_start", lpccAnimName))
			{
				if (cb)
				{
					assert(_eLastGameState == eLastGameState::WAITING_LAST_TORCH);
					cb();
				}
			}
		});
		_torchesActivated[nTorchToFire] = true;
	};

	void extinguishTorches(SimpleCallback cb)
	{
		assert(_eLastGameState != eLastGameState::EXTING);
		_eLastGameState = eLastGameState::EXTING;
		bool active[4] = { 0 };
		static_assert(sizeof(active) == sizeof(_torchesActivated));
		std::copy(std::begin(_torchesActivated), std::end(_torchesActivated), std::begin(active));
		std::fill(std::begin(_torchesActivated),  std::end(_torchesActivated),  false);
		std::fill(std::begin(_torchAtivateOrder), std::end(_torchAtivateOrder), 0);
		bool bCallCb = true;
		for (int i = 0; i < SIZE_OF(active); i++)
		{
			if (active[i])
			{
				_ptrFires[i]->setAnimation(0, "fire_finish", false);
				_ptrFires[i]->onComplete([this, i, bCallCb, cb](CSpine* pOwner, const char* lpccAnimName) {
					if (!strcmp("fire_finish", lpccAnimName))
					{
						_ptrFires[i]->setVisible(false);
						if (bCallCb)
						{
							switchLastGameState(eLastGameState::BASIC);
							cb();
						}
					}
				});
				bCallCb = false;
			}
		}
		_nKeyIdx = 0;
	};

	void addTorchesAreas(SimpleCallback cbOnCubeOpened)
	{
		constexpr float dy = -10.f;
		static Point ptTorchPos[4] = { { 225,  50 }, { 225 + 240,  50 }, { 225 + 510,  50 }, { 225 + 510 + 240,  50 } };
		static Point ptFirePos[4]  = { { 255, 190 + dy }, { 255 + 240, 190 + dy }, { 255 + 510, 190 + dy }, { 255 + 510 + 250, 190 + dy } };
		for (int i = 0; i < SIZE_OF(_ptrFires); i++)
		{
			assert(i < SIZE_OF(ptTorchPos));
			static_assert(SIZE_OF_T<decltype(ptTorchPos)>        == SIZE_OF_T<decltype(ptFirePos)>);
			static_assert(SIZE_OF_T<decltype(ptFirePos)>         == SIZE_OF_T<decltype(_ptrFires)>);
			static_assert(SIZE_OF_T<decltype(_torchesActivated)> == SIZE_OF_T<decltype(_ptrFires)>);
			auto ptrTorch = std::make_shared<TouchHandler>(80, 300);
			ptrTorch->setId(i + 1);
			ptrTorch->setPos(ptTorchPos[i].x, ptTorchPos[i].y);
			ptrTorch->setOnClick([this, i, cbOnCubeOpened] {
				if (_eLastGameState != eLastGameState::BASIC)
					return;
				_ptrEnterCubeBg->setAnimation(0, "torches_shown", false);
				assert(i < SIZE_OF(ptTorchPos));
				auto pt = ptTorchPos[i];
				pt.y += 350.f;
				moveHeroToPoint(_ptrAngel, pt, 300.f, [this, i, cbOnCubeOpened] {
					if (_eLastGameState != eLastGameState::BASIC)
						return;
					static int keyOrder[] = { 2, 2, 2, 4 };
					static_assert(SIZE_OF_T<decltype(keyOrder)> == SIZE_OF_T<decltype(_torchAtivateOrder)>);
					assert(i < SIZE_OF(_ptrFires));
					assert(_nKeyIdx < SIZE_OF(_torchAtivateOrder));
					_torchAtivateOrder[_nKeyIdx] = i + 1;
					_nKeyIdx++;
					auto res = std::mismatch(std::begin(keyOrder), std::end(keyOrder), std::begin(_torchAtivateOrder));
					if (res.first == std::end(keyOrder))
					{
						_bTrackEye = false;
						switchLastGameState(eLastGameState::WAITING_LAST_TORCH);
						fireTheTorch(3, [this, cbOnCubeOpened] {
							switchLastGameState(eLastGameState::OPEN_ANIM_TORCHES);
							for (int j = 0; j < SIZE_OF(_ptrFires); j++)
							{
								setTimeout(1.2f, [this] {
									_ptrEnterCubeBg->setAnimation(0, "all_torches_activated", false);
								});
								_ptrFires[j]->addSelfTween(eTweenProp::SCALE, _ptrFires[j]->getScaleX(), _ptrFires[j]->getScaleX() * 2.f, 1.2f, Easing::inOutElastic, j * 0.5f, [this, j, cbOnCubeOpened] {
									if (j == SIZE_OF(_ptrFires) - 1)
									{
										setTimeout(1.5f, [this, cbOnCubeOpened] {
											CGfx::getInstance()->setWorldDarken(true, 0, 0);
											setTimeout(.7f, [this, cbOnCubeOpened] {
												auto ptrDlg = std::make_shared<CubeTpDialog>();
												ptrDlg->init();
												ptrDlg->open(cbOnCubeOpened);
											});
										});
									}
								});
							}
						});
					}
					else
					{
						int nMismatchIdx = std::distance(std::begin(keyOrder), res.first);
						if (_torchAtivateOrder[nMismatchIdx] == 0 && _nKeyIdx > 1)
						{
							switch (_nKeyIdx - 1)
							{
							case 0: fireTheTorch(1); break;
							case 1: fireTheTorch(0); break;
							case 2: fireTheTorch(2); break;
							default: assert(false); break;
							}
						}
						else
						{
							if (_nKeyIdx > 1)
							{
								extinguishTorches([this, i] {
									_nKeyIdx = 1;
									_torchAtivateOrder[0] = i + 1;
									fireTheTorch(i);
								});
							}
							else
							{
								_nKeyIdx = 1;
								_torchAtivateOrder[0] = i + 1;
								fireTheTorch(i);
							}
						}
					}
				});
			});
			_ptrEnterCubeBgCont->addChild(ptrTorch);
			_ptrFires[i]->setPos(ptFirePos[i].x, ptFirePos[i].y);
			_ptrFires[i]->setVisible(false);
			_ptrFires[i]->setScale(0.25f, 0.25f);
			_ptrEnterCubeBgCont->addChild(_ptrFires[i]);
		}
	}

	void initLastGameShadows()
	{
		return;
	}

	void showGameTitle(SimpleCallback cb)
	{
		_ptrGameHolder->removeAll();
		CGfx::getInstance()->setWorldDarken(false);
		setTimeout(1.f, [this, cb] {
			auto& cfg = Engine::getCfg();
			auto ptrSpine = CSpineManager::getInstance()->getNewSpine("BBB/gametitle");
			ptrSpine->setPos(cfg.INIT_SCR_CX * 0.5f, cfg.INIT_SCR_CY * 0.5f - 200.f);
			_ptrGameHolder->addChild(ptrSpine);
			ptrSpine->setAlpha(0.f);
			ptrSpine->addSelfTween(eTweenProp::ALPHA, 0.f, 1.f, 1.f, Easing::linear, 0.f, [this, cb, ptrSpine] {
				ptrSpine->setAnimation(0, "arrive", false);
				ptrSpine->onComplete([this, cb](CSpine* pOwner, const char* lpccAnimName) {
					auto& cfg = Engine::getCfg();
					auto ptrLabel = std::make_shared<StaticLabel>();
					ptrLabel->setText(L10N::getInstance().tr("PRESS_TO_CONTINUE"));
					_ptrGameHolder->addChild(ptrLabel);
					ptrLabel->setXPosCentered(cfg.INIT_SCR_CX);
					ptrLabel->setY(cfg.INIT_SCR_CY * 0.5f + 300.f);
					ptrLabel->setAlpha(0.f);
					ptrLabel->addSelfTween(eTweenProp::ALPHA, 0.f, 1.f, 1.f);
					auto ptrTouchHandler = std::make_shared<TouchHandler>(cfg.INIT_SCR_CX, cfg.INIT_SCR_CY);
					ptrTouchHandler->setId();
					_ptrGameHolder->addChild(ptrTouchHandler);
					ptrTouchHandler->setOnClick([this, cb] {
						CGfx::getInstance()->setWorldDarken(true, 0, 0);
						setTimeout(1.f, cb);
					});
				});
			});
		});
	}

	void showBook(bool bShow)
	{
		if (bShow == _isBookTaken)
		{
			auto ptrSkel = _ptrSanctuaryBg->getSkeleton();
			for (int i = 0; i < ptrSkel->getSlots().size(); ++i)
			{
				auto slot = ptrSkel->getSlots()[i];
				if (slot && (slot->getBone().getData().getName() == "book" || slot->getBone().getData().getName() == "book_ghost"))
				{
					_isBookTaken = !bShow;
					if (bShow)
					{
						spine::String attachmentName = slot->getData().getAttachmentName();
						if (!attachmentName.isEmpty())
						{
							auto attachment = ptrSkel->getAttachment(slot->getData().getIndex(), attachmentName);
							assert(attachment);
							slot->setAttachment(attachment);
						}
						else
						{
							spine::String slotName = slot->getData().getName();
							auto attachment = ptrSkel->getAttachment(slot->getData().getIndex(), slotName);
							assert(attachment);
							slot->setAttachment(attachment);
						}
					}
					else
					{
						slot->setAttachment(nullptr);
					}
				}
			}
		}
	}

	void playSanctuaryFinalPart(SimpleCallback cb, bool bAdd = false)
	{
		AnswerButtonSkin orange = { "UI/slicedOrangeBtn_normal", "UI/slicedOrangeBtn_hl", "UI/slicedOrangeBtn_normal" };
		if (bAdd)
		{
			_isBookTaken = true;
			_rcMoveArea.set(0.f, 300.f, 1150.f, 170.f);
			_ptrGameHolder->removeAll();
			_ptrGameHolder->addChild(_ptrDriveLayer);
			_ptrDriveLayer->removeAll();
			_ptrDriveLayer->addChild(_ptrEnterCubeBgCont);
			_ptrEnterCubeBgCont->removeAll();
			_ptrEnterCubeBgCont->addChild(_ptrSanctuaryBg);
			_ptrEnterCubeBgCont->addChild(_ptrBarbarian);
			_ptrBarbarian->setAnimation(0, "idle", true);
			_ptrBarbarian->setPos(300, 500);
			_ptrSanctuaryBg->setPos(1272.f * 0.5f, 716.f * 0.5f);
			_ptrSanctuaryBg->setAnimation(0, "idle_closed", true);
		}
		static NovelEntry nov[] = {
			{
				.lpszBg          = nullptr,
				.spineOpts =
				{
					.lpszSpeakerSpine = "BBB/amarantha",
				},
				.lpszSpeakerText = L10N::getInstance().tr("FIN_LINE1"),
				.fYTextCorrection = 0.f,
				.fWorldDarken    = .0f,
				.plszCenterPic   = nullptr,
				.fPicScale       = 0.7f,
				.rcSpinePos      = { 200, 970, 500, 0 },
				.fSpineScale     = 0.75f,
				.answers         =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_LIE"),
						.lpszText        = nullptr,
						.pNext           = nov + 1,
					},
				},
			},
			{
				.lpszBg            = nullptr,
				.lpszSpeakerPic    = "BBB/barb.png",
				.lpszSpeakerText   = L10N::getInstance().tr("FIN_LINE2"),
				.fYTextCorrection  = 0.f,
				.fWorldDarken      = .4f,
				.plszCenterPic     = nullptr,
				.fPicScale         = 0.7f,
				.fSpineScale       = 0.5f,
				.answers           =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_ELLIPSIS"),
						.lpszText        = nullptr,
						.pNext           = nov + 2,
					},
				},
			},
			{
				.lpszBg          = nullptr,
				.spineOpts =
				{
					.lpszSpeakerSpine = "BBB/amarantha",
				},
				.lpszSpeakerText   = L10N::getInstance().tr("FIN_LINE3"),
				.fYTextCorrection  = 0.f,
				.fWorldDarken      = .0f,
				.plszCenterPic     = nullptr,
				.fPicScale         = 0.7f,
				.rcSpinePos        = { 200, 970, 500, 0 },
				.fSpineScale        = 0.75f,
				.answers           =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_ELLIPSIS"),
						.lpszText        = nullptr,
						.pNext           = nov + 3,
					},
				},
			},
			{
				.lpszBg          = nullptr,
				.spineOpts =
				{
					.lpszSpeakerSpine   = "BBB/amarantha",
					.lpszAnimNameSwitch = "charge",
					.lpszAnimNameNew    = "idle_charged"
				},
				.lpszSpeakerText    = L10N::getInstance().tr("FIN_LINE4"),
				.fYTextCorrection   = 0.f,
				.fWorldDarken       = .0f,
				.plszCenterPic      = nullptr,
				.fPicScale          = 0.7f,
				.rcSpinePos         = { 200, 970, 500, 0 },
				.fSpineScale        = 0.75f,
				.bNoStartTransition = true,
				.answers            =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_BEG"),
						.lpszText        = nullptr,
						.pNext           = nov + 4,
					},
				},
			},
			{
				.lpszBg            = nullptr,
				.lpszSpeakerPic    = "BBB/barb.png",
				.lpszSpeakerText   = L10N::getInstance().tr("FIN_LINE5"),
				.fYTextCorrection  = 0.f,
				.fWorldDarken      = .4f,
				.plszCenterPic     = nullptr,
				.fPicScale         = 0.7f,
				.fSpineScale       = 0.5f,
				.answers           =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_ELLIPSIS"),
						.lpszText        = nullptr,
						.pNext           = nov + 5,
					},
				},
			},
			{
				.lpszBg          = nullptr,
				.spineOpts =
				{
					.lpszSpeakerSpine   = "BBB/alianna",
					.lpszAnimName       = "idle"
				},
				.lpszSpeakerText    = L10N::getInstance().tr("FIN_LINE6"),
				.fYTextCorrection   = 0.f,
				.fWorldDarken       = .0f,
				.plszCenterPic      = nullptr,
				.fPicScale          = 0.7f,
				.rcSpinePos         = { 200, 950, 500, 0 },
				.fSpineScale        = .55f,
				.answers            =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_RUN"),
						.lpszText        = nullptr,
						.pNext           = nov + 6,
					},
				},
			},
			{
				.lpszBg            = nullptr,
				.lpszSpeakerPic    = "BBB/barb.png",
				.lpszSpeakerText   = L10N::getInstance().tr("FIN_LINE7"),
				.fYTextCorrection  = 0.f,
				.fWorldDarken      = .4f,
				.plszCenterPic     = nullptr,
				.fPicScale         = 0.7f,
				.fSpineScale       = 0.5f,
				.answers           =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_ELLIPSIS"),
						.lpszText        = nullptr,
						.pNext           = nov + 7,
					},
				},
			},
			{
				.lpszBg          = nullptr,
				.spineOpts =
				{
					.lpszSpeakerSpine = "BBB/amarantha",
					.lpszAnimName     = "idle_charged",
				},
				.lpszSpeakerText    = L10N::getInstance().tr("FIN_LINE8"),
				.fYTextCorrection   = 0.f,
				.fWorldDarken       = .0f,
				.plszCenterPic      = nullptr,
				.fPicScale          = 0.7f,
				.rcSpinePos         = { 200, 970, 500, 0 },
				.fSpineScale        = 0.75f,
				.bNoStartTransition = true,
				.answers            =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_ELLIPSIS"),
						.lpszText        = nullptr,
						.pNext           = nov + 8,
					},
				},
			},
			{
				.lpszBg          = nullptr,
				.spineOpts =
				{
					.lpszSpeakerSpine   = "BBB/alianna",
					.lpszAnimNameSwitch = "charge",
					.lpszAnimNameNew    = "idle_charged",
					.lpszFinalAnimName  = "spawn_tentacle"
				},
				.lpszSpeakerText    = L10N::getInstance().tr("FIN_LINE9"),
				.fYTextCorrection   = 0.f,
				.fWorldDarken       = .0f,
				.plszCenterPic      = nullptr,
				.fPicScale          = 0.7f,
				.rcSpinePos         = { 200, 950, 500, 0 },
				.fSpineScale        = .55f,
				.answers            =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_ELLIPSIS"),
						.lpszText        = nullptr,
						.pNext           = nov + 9,
					},
				},
			},
			{
				.lpszBg          = nullptr,
				.spineOpts =
				{
					.lpszSpeakerSpine   = "BBB/amarantha",
					.lpszAnimNameSwitch = "curse",
					.lpszAnimNameNew    = "idle_wounded"
				},
				.lpszSpeakerText    = L10N::getInstance().tr("FIN_LINE10"),
				.fYTextCorrection   = 0.f,
				.fWorldDarken       = .0f,
				.plszCenterPic      = nullptr,
				.fPicScale          = 0.7f,
				.rcSpinePos         = { 200, 970, 500, 0 },
				.fSpineScale        = 0.75f,
				.bNoStartTransition = true,
				.answers            =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_ELLIPSIS"),
						.lpszText        = nullptr,
						.pNext           = nov + 10,
					},
				},
			},
			{
				.lpszBg          = nullptr,
				.spineOpts =
				{
					.lpszSpeakerSpine = "BBB/alianna",
					.lpszAnimName     = "idle_charged",
				},
				.lpszSpeakerText    = L10N::getInstance().tr("FIN_LINE11"),
				.fYTextCorrection   = 0.f,
				.fWorldDarken       = .0f,
				.plszCenterPic      = nullptr,
				.fPicScale          = 0.7f,
				.rcSpinePos         = { 200, 950, 500, 0 },
				.fSpineScale        = .55f,
				.answers            =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_ELLIPSIS"),
						.lpszText        = nullptr,
						.pNext           = nov + 11,
					},
				},
			},
			{
				.lpszBg            = nullptr,
				.lpszSpeakerPic    = "BBB/barb.png",
				.lpszSpeakerText   = L10N::getInstance().tr("FIN_LINE12"),
				.fYTextCorrection  = 0.f,
				.fWorldDarken      = .4f,
				.plszCenterPic     = nullptr,
				.fPicScale         = 0.7f,
				.fSpineScale       = 0.5f,
				.answers           =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_ELLIPSIS"),
						.lpszText        = nullptr,
						.pNext           = nov + 12,
					},
				},
			},
			{
				.lpszBg          = nullptr,
				.spineOpts =
				{
					.lpszSpeakerSpine = "BBB/alianna",
					.lpszAnimName     = "idle_charged",
				},
				.lpszSpeakerText    = L10N::getInstance().tr("FIN_LINE13"),
				.fYTextCorrection   = 0.f,
				.fWorldDarken       = .0f,
				.plszCenterPic      = nullptr,
				.fPicScale          = 0.7f,
				.rcSpinePos         = { 200, 950, 500, 0 },
				.fSpineScale        = .55f,
				.answers            =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_ELLIPSIS"),
						.lpszText        = nullptr,
						.pNext           = nov + 13,
					},
				},
			},
			{
				.lpszBg          = nullptr,
				.spineOpts =
				{
					.lpszSpeakerSpine   = "BBB/amarantha",
					.lpszAnimName       = "idle_wounded"
				},
				.lpszSpeakerText    = L10N::getInstance().tr("FIN_LINE14"),
				.fYTextCorrection   = 0.f,
				.fWorldDarken       = .0f,
				.plszCenterPic      = nullptr,
				.fPicScale          = 0.7f,
				.rcSpinePos         = { 200, 970, 500, 0 },
				.fSpineScale        = 0.75f,
				.bNoStartTransition = true,
				.answers            =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_ELLIPSIS"),
						.lpszText        = nullptr,
						.pNext           = nov + 14,
					},
				},
			},
			{
				.lpszBg            = nullptr,
				.lpszSpeakerPic    = "BBB/barb.png",
				.lpszSpeakerText   = L10N::getInstance().tr("FIN_FINAL_QUESTION"),
				.fYTextCorrection  = 0.f,
				.fWorldDarken      = .4f,
				.plszCenterPic     = nullptr,
				.fPicScale         = 0.7f,
				.fSpineScale       = 0.5f,
				.answers           =
				{
					{
						.lpszAnswerTitle   = L10N::getInstance().tr("BTN_EXECUTE_AMARANTHA"),
						.lpszText          = nullptr,
						.pNext             = nullptr,
						.btnSkin		   = orange
					},
					{
						.lpszAnswerTitle   = L10N::getInstance().tr("BTN_SAVE_AMARANTHA"),
						.lpszText          = nullptr,
						.pNext             = nullptr,
						.bActive           = false,
					},
				},
			},
		};
		nov[14].answers[1].bActive = _isBookTaken;
		nov[14].answers[1]._cbOnSelected = [this, cb](SimpleCallback fcb) {
			assert(_isBookTaken);
			fcb();
			auto ptrPortal = CSpineManager::getInstance()->getNewSpine("BBB/portal");
			ptrPortal->setAnimation(0, "animation", true);
			_ptrEnterCubeBgCont->addChild(ptrPortal);
			ptrPortal->setPos(_ptrAngel->getX() + 400, _ptrAngel->getY());
			ptrPortal->addSelfTween(eTweenProp::ALPHA, 0.5f, 1.f, .3f);
			ptrPortal->addSelfTween(eTweenProp::SCALE, 0.1f, 1.f, 1.5f, Easing::outBack);
			_ptrAngel->getParent()->swapChildren(_ptrAngel, ptrPortal);
			setTimeout(1.5f, [this, ptrPortal, cb] {
				moveHeroToPoint(_ptrAngel, { ptrPortal->getX(), ptrPortal->getY() }, 300, [this, cb] {
					_ptrAngel->addSelfTween(eTweenProp::ALPHA, 1.f, 0.f, .5f);
					_ptrAngel->addSelfTween(eTweenProp::SCALE_ABS, _ptrAngel->getScaleX(), _ptrAngel->getScaleX() * 0.25f, .5f);
					_ptrAngel->addSelfTween(eTweenProp::Y, _ptrAngel->getY(), _ptrAngel->getY() - 300, 0.5f);
					setTimeout(1.5f, [this, cb] {
						CGfx::getInstance()->setWorldDarken(true, 0, 0);
						setTimeout(1.f, [this, cb] {
							showGameTitle(cb);
						});
					});
				});
			});
		};
		nov[14].answers[0]._cbOnSelected = [this, cb](SimpleCallback fcb) {
			_ptrBarbarian->addSelfTween(eTweenProp::ALPHA, 1.f, 0.f, 0.3f);
			auto ptrDlg = std::make_shared<DialogBeheading>();
			ptrDlg->init(_ptrAngel);
			ptrDlg->open([this, fcb, cb] { fcb(); showGameTitle(cb); });
		};
		_ptrSanctuaryBg->setAnimation(0, "portal_activate", false);
		_ptrSanctuaryBg->addAnimation(0, "idle_opened",     true);
		_ptrSanctuaryBg->onEvent([this, cb](CSpine* pOwner, const char* lpccEventName) {
			assert(!strcmp(lpccEventName, "tp_spawn"));
			Rect rc;
			bool bFound = _ptrSanctuaryBg->getBoundingBoxRect("GODESS_ARRIVE_PT", { _ptrSanctuaryBg->getX(), _ptrSanctuaryBg->getY() }, rc);
			assert(bFound);
			_ptrEnterCubeBgCont->addChild(_ptrAngel);
			_ptrEnterCubeBgCont->swapChildren(_ptrAngel, _ptrBarbarian);
			_ptrAngel->setPos(rc.centerMid());
			_ptrAngel->setAnimation(0, "idle", true);
			_ptrAngel->setAlpha(0);
			_ptrAngel->setScale(0.1f, 0.1f);
			_ptrAngel->addSelfTween(eTweenProp::ALPHA, 0.f, 1.f, 1.35f, Easing::linear, 0.f, [this, cb] {
				setTimeout(0.5f, [this, cb] {
					Rect rc;
					bool bFound = _ptrSanctuaryBg->getBoundingBoxRect("GODESS_MOVE_PT", { _ptrSanctuaryBg->getX(), _ptrSanctuaryBg->getY() }, rc);
					assert(bFound);
					moveHeroToPoint(_ptrAngel, rc.centerMid(), 300.f, [this, cb] {
						_ptrAngel->flipX();
						setTimeout(0.5f, [this, cb] {
							auto ptrNovel = std::make_shared<Novel>();
							ptrNovel->init(nov, -1, false, L10N::getInstance().tr("BTN_ELLIPSIS"), "greengoth", 62);
							ptrNovel->setNoWorldLightenOnClose();
							ptrNovel->open();
						});
					});
				});
			});
			_ptrAngel->addSelfTween(eTweenProp::SCALE, .1f, 1.f, 1.f, Easing::inOutBack);
		});
	}

	void openHeadsPortal(SimpleCallback cb)
	{
		static NovelEntry novCantEnter[] = {
			{
				.lpszBg            = nullptr,
				.lpszSpeakerPic    = "BBB/barb.png",
				.lpszSpeakerText   = L10N::getInstance().tr("HEADS_PORTAL_CANT"),
				.fYTextCorrection  = 0.f,
				.fWorldDarken      = .4f,
				.plszCenterPic     = nullptr,
				.fPicScale         = 0.7f,
				.fSpineScale       = 0.5f,
				.answers           =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_ELLIPSIS"),
						.lpszText        = nullptr,
						.pNext           = nullptr,
					},
				},
			}
		};
		_ptrTouchHandler->setVisible(false);
		_ptrSanctuaryBg->setAnimation(0, "open",        false);
		_ptrSanctuaryBg->addAnimation(0, "idle_opened", true);
		_ptrSanctuaryBg->onComplete([this, cb](CSpine* pOwner, const char* lpccAnimName) {
			if (!strcmp("open", lpccAnimName))
			{
				setTimeout(0.8f, [this, cb] {
					auto ptrNovel = std::make_shared<Novel>();
					ptrNovel->init(novCantEnter, -1, false, L10N::getInstance().tr("BTN_ELLIPSIS"), "greengoth", 62);
					ptrNovel->open([this, cb] {
						playSanctuaryFinalPart(cb);
					});
				});
			}
		});
	}

	void playSanctuary(SimpleCallback cb)
	{
		static NovelEntry novStart[] = {
			{
				.lpszBg            = nullptr,
				.lpszSpeakerPic    = "BBB/barb.png",
				.lpszSpeakerText   = L10N::getInstance().tr("SANCTUARY_START"),
				.fYTextCorrection  = 0.f,
				.fWorldDarken      = .4f,
				.plszCenterPic     = nullptr,
				.fPicScale         = 0.7f,
				.fSpineScale       = 0.5f,
				.answers           =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_FORWARD"),
						.lpszText        = nullptr,
						.pNext           = nullptr,
					},
				},
			}
		};
		static NovelEntry novBookFound[] = {
			{
				.lpszBg            = nullptr,
				.lpszSpeakerPic    = "BBB/barb.png",
				.lpszSpeakerText   = L10N::getInstance().tr("BOOK_FOUND"),
				.fYTextCorrection  = 0.f,
				.fWorldDarken      = .4f,
				.plszCenterPic     = nullptr,
				.fPicScale         = 0.7f,
				.fSpineScale       = 0.5f,
				.answers           =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_FORWARD"),
						.lpszText        = nullptr,
						.pNext           = nullptr,
					},
				},
			}
		};
		_ptrAngel->enableLightEmission(false);
		showBook(true);
		switchLastGameState(eLastGameState::BASIC);
		auto ptrNovel = std::make_shared<Novel>();
		ptrNovel->init(novStart, -1, false, L10N::getInstance().tr("BTN_ELLIPSIS"), "greengoth", 62);
		ptrNovel->open();
		CGfx::getInstance()->setWorldDarken(false);
		auto& cfg = Engine::getCfg();
		_rcMoveArea.set(0.f, 300.f, 1150.f, 170.f);
		_ptrEnterCubeBgCont->removeAll();
		_ptrEnterCubeBgCont->addChild(_ptrSanctuaryBg);
		_ptrEnterCubeBgCont->addChild(_ptrBarbarian);
		_ptrBarbarian->setAnimation(0, "idle", true);
		_ptrBarbarian->setPos(300, 500);
		_ptrSanctuaryBg->setPos(1272.f * 0.5f, 716.f * 0.5f);
		_ptrSanctuaryBg->setAnimation(0, "idle_closed", true);
		_ptrTouchHandler = std::make_shared<TouchHandler>(cfg.INIT_SCR_CX, cfg.INIT_SCR_CY);
		_ptrTouchHandler->setId();
		_ptrEnterCubeBgCont->addChild(_ptrTouchHandler);
		_ptrTouchHandler->setOnClick([this, cb] {
			Point pt = InputController::getInstance()->getMousePos();
			_ptrTouchHandler->toLocal(pt);
			Point ptCenter{ _ptrSanctuaryBg->getX(), _ptrSanctuaryBg->getY() };
			auto lpszBBName = _ptrSanctuaryBg->getBoundingBoxAtPoint(ptCenter, pt);
			if (lpszBBName)
			{
				if (!strcmp(lpszBBName, "HEADS"))
				{
					Rect rcHeadHeroPos;
					bool bFound = _ptrSanctuaryBg->getBoundingBoxRect("HEADS_HERO_POS", { _ptrSanctuaryBg->getX(), _ptrSanctuaryBg->getY() }, rcHeadHeroPos);
					assert(bFound);
					moveHeroToPoint(_ptrBarbarian, rcHeadHeroPos.centerMid(), 500.f, [this, cb] {
						if (!_isBookTaken)
						{
							CGfx::getInstance()->setWorldDarken();
							auto ptrDlg = std::make_shared<YesNoDialog>();
							ptrDlg->init(L10N::getInstance().tr("YESNO_EVENT"),
								L10N::getInstance().tr("BTN_EXAMINE"),
								L10N::getInstance().tr("BTN_READY"), {}, [this, cb] {
									openHeadsPortal(cb);
							});
							ptrDlg->open([] { CGfx::getInstance()->setWorldDarken(false); });
						}
						else
						{
							openHeadsPortal(cb);
						}
					});
				}
				else if (!_isBookTaken && !strcmp(lpszBBName, "COFFIN"))
				{
					moveHeroToPoint(_ptrBarbarian, pt, 500.f, [this] {
						showBook(false);
						assert(_isBookTaken);
						auto ptrNovel = std::make_shared<Novel>();
						ptrNovel->init(novBookFound, -1, false, L10N::getInstance().tr("BTN_ELLIPSIS"), "greengoth", 62);
						ptrNovel->open();
					});
				}
			}
			else
			{
				moveHeroToPoint(_ptrBarbarian, pt, 500.f);
			}
		});
	}

	void playLastGame(SimpleCallback cb)
	{
		auto& cfg = Engine::getCfg();
		_rcMoveArea.set(0.f, 400.f, 1000.f, 210.f);
		_nKeyIdx = 0;
		_ptrAngel->removeFromParent();
		_ptrAngel = CSpineManager::getInstance()->getNewSpine("BBB/angel", false);
		_ptrAngel->setScale(1.f, 1.f);
		_ptrAngel->setLightAttachment("lightspot");
		_ptrAngel->addRadialLight(100, "fire",  "fire").color.set(1.f, 0.7f, 0.1f);
		_ptrAngel->addRadialLight(100, "fire2", "fire2").color.set(1.f, 0.7f, 0.1f);
		_ptrAngel->addRadialLight(100, "fire3", "fire3").color.set(1.f, 0.7f, 0.1f);
		_ptrAngel->setLightLayer(1);
		_ptrAngel->setAlpha(1.f);
		_ptrBarbarian->setAlpha(1.f);
		std::fill(std::begin(_torchesActivated),  std::end(_torchesActivated),  false);
		std::fill(std::begin(_torchAtivateOrder), std::end(_torchAtivateOrder), 0);
		initLastGameShadows();
		_ptrAngel->enableLightEmission(true);
		_ptrEnterCubeBgCont->removeAll();
		_ptrEnterCubeBgCont->addChild(_ptrEnterCubeBg);
		_ptrEnterCubeBg->setPos(1272.f * 0.5f, 716.f * 0.5f);
		_ptrEnterCubeBgCont->addChild(_ptrCubeClosed);
		_ptrEnterCubeBgCont->addChild(_ptrAngel);
		_ptrTouchHandler = std::make_shared<TouchHandler>(cfg.INIT_SCR_CX, cfg.INIT_SCR_CY);
		_ptrTouchHandler->setId();
		_ptrEnterCubeBgCont->addChild(_ptrTouchHandler);
		_ptrTouchHandler->setOnClick([this] {
			Point pt = InputController::getInstance()->getMousePos();
			_ptrTouchHandler->toLocal(pt);
			moveHeroToPoint(_ptrAngel, pt);
		});
		_ptrCubeHandler = std::make_shared<TouchHandler>(400, 200);
		_ptrCubeHandler->setPos(1040, 200);
		_ptrCubeHandler->setId();
		_ptrEnterCubeBgCont->addChild(_ptrCubeHandler);
		_ptrCubeHandler->setOnClick([this, cb] {
			Point pt = { 1040, 200 };
			_ptrCubeHandler->removeFromParent();
			moveHeroToPoint(_ptrAngel, pt, 300.f, [this, cb] {
				auto ptrDlg = std::make_shared<CubeClosedDialog>();
				ptrDlg->init();
				ptrDlg->open([this, cb] {
					_ptrCubeClosed->setAnimation(0, "idle_tounge", true);
					CGfx::getInstance()->setWorldDarken(false);
					setTimeout(1.f, [this, cb] {
						_ptrEnterCubeBg->setAnimation(0, "show_torches", false);
						_ptrEnterCubeBg->addAnimation(0, "torch_hint",   true);
						_ptrEnterCubeBg->onComplete([this, cb](CSpine* pOwner, const char* lpccAnimName) {
							if (!strcmp(lpccAnimName, "show_torches"))
							{
								_bTrackEye = true;
								addTorchesAreas([this, cb] { playSanctuary(cb); });
								_ptrAngel->getParent()->bringChildToFront(_ptrAngel.get());
							}
						});
					});
				});
				CGfx::getInstance()->setWorldDarken(true, 0, 0);
			});
		});
		_ptrDriveLayer->removeAll();
		_ptrDriveLayer->addChild(_ptrEnterCubeBgCont);
		_ptrEnterCubeBg->setAnimation(0, "torches_hidden", false);
		_ptrAngel->setAnimation(0, "idle", true);
		_ptrAngel->setPos(300, 500);
		_ptrCubeClosed->setAnimation(0, "idle", true);
		_ptrCubeClosed->setScale(0.7f, 0.7f);
		_ptrCubeClosed->setPos(1150, 300);
		_ptrGameHolder->removeAll();
		_ptrGameHolder->addChild(_ptrDriveLayer);
		CGfx::getInstance()->getPostProcessSettings().lutIntensity = 0.1f;
		static NovelEntry nov[] = {
			{
				.lpszBg          = nullptr,
				.spineOpts =
				{
					.lpszSpeakerSpine = "BBB/amarantha",
				},
				.lpszSpeakerText = L10N::getInstance().tr("LAST_INTRO"),
				.fYTextCorrection = 0.f,
				.fWorldDarken    = .0f,
				.plszCenterPic   = nullptr,
				.fPicScale       = 0.7f,
				.rcSpinePos      = { 200, 950, 500, 0 },
				.fSpineScale     = 0.75f,
				.answers         =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_FORWARD"),
						.lpszText        = nullptr,
						.pNext           = nullptr,
					},
				},
			}
		};
		auto ptrDlg = std::make_shared<Novel>();
		ptrDlg->init(nov, -1, false, L10N::getInstance().tr("BTN_ELLIPSIS"), "greengoth", 62);
		ptrDlg->open();
	}

	void trackEye()
	{
		if (_bTrackEye)
		{
			auto pSkel = _ptrEnterCubeBg->getSkeleton();
			spine::Bone* aimTargetBone = pSkel->findBone("pupil");
			auto& cfg      = Engine::getCfg();
			float fMaxX    = cfg.INIT_SCR_CX * .5f;
			float fMaxEyeX = 50.f;
			float fX       = _ptrAngel->getX() - cfg.INIT_SCR_CX * .5f;
			aimTargetBone->setX(fX / fMaxEyeX);
		}
	}

	void playDrive(SimpleCallback cb)
	{
		cleanupDrive();
		float fDarken = .2f;
		static NovelEntry nov[] = {
			{
				.fBgShowPause    = 1.f,
				.lpszBg          = "BBB/doctor_ambulance.jpg",
				.lpszSpeakerPic  = nullptr,
				.lpszSpeakerText = L10N::getInstance().tr("DRIVE_SPEAKER1"),
				.fYTextCorrection = 0.f,
				.fWorldDarken    = fDarken,
				.plszCenterPic   = nullptr,
				.fPicX           = 400.f,
				.fPicY           = -200.f,
				.fPicScale       = 0.7f,
				.answers         =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("BTN_MORE"),
						.lpszText        = nullptr,
						.pNext           = nov + 1,
					},
				},
			},
			{
				.fBgShowPause    = 0.f,
				.lpszBg          = "BBB/doctor_ambulance.jpg",
				.lpszSpeakerPic  = nullptr,
				.lpszSpeakerText = L10N::getInstance().tr("DRIVE_SPEAKER2"),
				.fYTextCorrection = 0.f,
				.fWorldDarken    = fDarken,
				.plszCenterPic   = nullptr,
				.fPicX           = 400.f,
				.fPicY           = -200.f,
				.fPicScale       = 0.7f,
				.answers         =
				{
					{
						.lpszAnswerTitle = L10N::getInstance().tr("DRIVE_ANSWER_SUPPORT"),
						.lpszText        = nullptr,
						.pNext           = nullptr,
					},
				},
			},
		};
		assert(_eState == eShizGameState::DIALING);
		_eState = eShizGameState::DRIVING;
		_ptrGameHolder->removeAll();
		_ptrDriveLayer->removeAll();
		_ptrGameHolder->addChild(_ptrDriveLayer);
		_ptrLamp->setPos(600, 700);
		_ptrShiz->setPos(630, 740);
		_ptrAmbulance->setPos(3000, 520);
		_ptrDriveLayer->addChild(_ptrNotebook);
		_ptrDriveLayer->addChild(_ptrNotebookBurned);
		_ptrNotebook->setVisible(_nFunGameSelected == 0);
		_ptrNotebookBurned->setVisible(_nFunGameSelected == 1);
		_ptrDriveLayer->addChild(_ptrShiz);
		if (_nFunGameSelected == 0)
		{
			_ptrLamp->setAnimation(0, "animation", true);
			_ptrShiz->setAnimation(0, "animation", true);
			_ptrAmbulance->setAnimation(0, "drive", true);
			_ptrDriveLayer->addChild(_ptrShizBgCont);
			_ptrDriveLayer->addChild(_ptrLamp);
			_ptrDriveLayer->addChild(_ptrFlies);
			_ptrDriveLayer->addChild(_ptrFlies2);
			_ptrFlies->setPos(700, 550);
			_ptrFlies2->setPos(700, 300);
			_ptrDriveLayer->addChild(_ptrAmbulance);
			_ptrDriveLayer->bringChildToBack(_ptrShizBgCont.get());
			_ptrDriveLayer->bringChildToFront(_ptrShiz.get());
			setTimeout(11.50f, [this] {
				_ptrAmbulance->setAnimation(0, "stop", false);
				_ptrAmbulance->onEvent([this](CSpine* pOwner, const char* lpccEventName) {
					assert(!strcmp("stopped", lpccEventName));
					_ptrAmbulance->removeSelfTweens();
				});
				_ptrAmbulance->addAnimation(0, "idle", false);
				_ptrAmbulance->addAnimation(0, "doctor_arrive", false);
				_ptrAmbulance->addAnimation(0, "doctor_idle", true);
			});
			_ptrAmbulance->addSelfTween(eTweenProp::X, _ptrAmbulance->getX(), 1300, 12.f, Easing::linear);
			_ptrAmbulance->onComplete([this, cb](CSpine* pOwner, const char* lpccAnimName) {
				if (!strcmp(lpccAnimName, "doctor_arrive"))
				{
					setTimeout(1.5f, [this, cb] {
						auto ptrDlg = std::make_shared<Novel>();
						ptrDlg->init(nov);
						ptrDlg->open([this, cb] {
							_ptrDriveLayer->addChild(_ptrDragon);
							_ptrDragon->setAnimation(0, "flying", true);
							_ptrDragon->setPos(3000, 220);
							if (!_ptrDragon->isFlippedX())
								_ptrDragon->flipX();
							_ptrDragon->addSelfTween(eTweenProp::Y, _ptrDragon->getY(), 850, 12.f);
							_ptrDragon->addSelfTween(eTweenProp::X, _ptrDragon->getX(), 1200, 12.f, Easing::linear, 0.f, [this, cb] {
								_ptrDragon->onEvent([this, cb](CSpine* pOwner, const char* lpccEventName) {
									if (!strcmp("fire_on_the_floor", lpccEventName))
										_ptrShiz->setAnimation(0, "toSP", false);
									_ptrShiz->onComplete([this, cb](CSpine* pOwner, const char* lpccAnimName) {
										if (!strcmp(lpccAnimName, "toSP"))
										{
											_ptrNotebookBurned->setVisible(true);
											_ptrNotebook->setVisible(false);
											_ptrDragon->addSelfTween(eTweenProp::X, _ptrDragon->getX(), -600.f, 5.f, Easing::inCirc);
											setTimeout(3.f, [this, cb] {
												CGfx::getInstance()->setWorldDarken(true, 0, 0, [this, cb] {
													cb();
												});
											});
										}
									});
								});
								_ptrDragon->setAnimation(0, "ultimate_start", false);
								_ptrDragon->addAnimation(0, "ultimate_idle",  true);
							});
						});
					});
				}
			});
		}
		else
		{
			addSelfTween(eTweenProp::DISTORTION, 1.f, 0.f, 3.2f);
			addSelfTween(eTweenProp::SATURATION, 0.f, 1.f, 3.2f);
			_ptrShiz->setPos(830, 740);
			_ptrShiz->setAnimation(0, "animation", true);
			_ptrDriveLayer->addChild(_ptrShizBg2);
			_ptrDriveLayer->bringChildToBack(_ptrShizBg2.get());
			_ptrDriveLayer->addChild(_ptrMita);
			_ptrMita->setAnimation(0, "run", true);
			_ptrMita->setScale(0.22f, 0.22f);
			_ptrMita->flipX();
			_ptrMita->setPos(3000, 520);
			_ptrMita->onEvent({});
			_ptrMita->onComplete({});
			_ptrMita->addSelfTween(eTweenProp::X, _ptrMita->getX(), 1080, 5.f, Easing::outCirc, 0.f, [this, cb] {
				_ptrMita->setAnimation(0, "idle", true);
				setTimeout(1.f, [this, cb] {
					auto ptrDlg = std::make_shared<MitaDragTutor>();
					ptrDlg->init();
					ptrDlg->open([this, cb] {
						auto ptrDragAcceptor = std::make_shared<TouchHandler>(600, 500);
						ptrDragAcceptor->setPos(200, 600);
						_ptrGameHolder->addChild(ptrDragAcceptor);
						_ptrMita->setDragable({}, [this](CContainerPtr ptrDragObj, float x, float y) {
							return true;
						});
						ptrDragAcceptor->setOnDragDropAccept([this](CContainerPtr droppedObj) {
							return true;
						});
						ptrDragAcceptor->setOnDragDropped([this, cb](float x, float y, CContainerPtr droppedObj) {
							_ptrMita->setDragable({}, {});
							_ptrMita->setPos(520, 740);
							_ptrMita->setDragIconAdjustLocal(0, -300.f);
							_ptrMita->setScale(0.37f, 0.37f);
							_ptrDriveLayer->addChild(_ptrMita);
							_ptrMita->setAnimation(0, "attack", false);
							_ptrMita->onComplete([this, cb](CSpine* pOwner, const char* lpccAnimName) {
								if (!strcmp(lpccAnimName, "attack"))
								{
									CGfx::getInstance()->setWorldDarken(true, 0.4, 0.2, [this, cb] {
										addSelfTween(eTweenProp::FISH_EYE, 0.f, 0.5f, .3f, Easing::outElastic, 0.f);
										addSelfTween(eTweenProp::LUT, 0.f, 1.f, .3f);
										addSelfTween(eTweenProp::GRAIN, 0.f, 122.f, .3f, Easing::linear, 0, [this, cb] {
											addSelfTween(eTweenProp::GRAIN, 122.f, .6f, 2.f, Easing::linear, 0, [this, cb] {
												CGfx::getInstance()->getPostProcessSettings().aberration = 0.015;
												auto ptrDiagDlg = std::make_shared<MitaDiagnosis>();
												ptrDiagDlg->init();
												ptrDiagDlg->open([this, cb] {
													_ptrShiz->setAnimation(0, "stop", false);
													_ptrMita->setAnimation(0, "transition",     false);
													_ptrMita->addAnimation(0, "ultimate_fury2", false);
													_ptrMita->addAnimation(0, "idle_fury",      true);
													_ptrMita->onEvent([this, cb](CSpine* pOwner, const char* lpccEventName) {
														_ptrShiz->setAnimation(0, "damaged", false);
														setTimeout(2.f, [this, cb] {
															CGfx::getInstance()->setWorldDarken(true, 0.4, 0.2, [this, cb] {
																addSelfTween(eTweenProp::FISH_EYE, 0.f, 0.5f, .3f, Easing::outElastic, 0.f);
																addSelfTween(eTweenProp::LUT, 0.f, 1.f, .3f);
																addSelfTween(eTweenProp::GRAIN, 0.f, 122.f, .3f, Easing::linear, 0, [this, cb] {
																	addSelfTween(eTweenProp::GRAIN, 122.f, .6f, 2.f, Easing::linear, 0, [this, cb] {
																		CGfx::getInstance()->getPostProcessSettings().aberration = 0.015;
																		auto ptrDiagDlg = std::make_shared<MitaDiagnosis>();
																		ptrDiagDlg->init(1);
																		ptrDiagDlg->open(cb);
																	});
																});
															});
														});
													});
												});
											});
										});
									});
								}
							});
						});
					});
				});
			});
		}
	}

	NineSlicePtr getSolidFrame()
	{
		static auto ptrSprSolid = SpriteLoader::getInstance()->getSprite("UI/mainIconsFrame");
		NineSlicePtr ptrFrame = std::make_shared<NineSlice>();
		ptrFrame->createSlices(ptrSprSolid->cloneInitial(), 67, 67, 67, 67);
		return ptrFrame;
	}

	void playIntro()
	{
		setPostProcessing();
		assert(_eState == eShizGameState::INITIAL);
		_eState = eShizGameState::INTRO;
		_ptrGameHolder->removeAll();
		_ptrGameHolder->addChild(_ptrIntroLayer);
		auto y = -(_ptrMainMenu->calcNotTransCy() + 10);
		_ptrMainMenu->setY(y);
		_ptrBtnGames->setEnabled(true);
		_ptrBtnWatch->setEnabled(true);
		if (!_bIntoAnimShown)
		{
			_bIntoAnimShown = true;
			_ptrIntro->setAnimationByName("animation", false, [this] {
				_ptrMainMenu->addSelfTween(eTweenProp::Y, _ptrMainMenu->getY(), 0, .5f, Easing::outBack, 0, [this] {
					_ptrMenuFrame->addSelfTween(eTweenProp::ALPHA, 0.f, .15f, 1.f);
				});
			});
		}
		else
		{
			_ptrMainMenu->addSelfTween(eTweenProp::Y, _ptrMainMenu->getY(), 0, .5f, Easing::outBack, 0, [this] {
				_ptrMenuFrame->addSelfTween(eTweenProp::ALPHA, 0.f, .15f, 1.f);
			});
		}
	}

	void initDial()
	{
		_ptrPhoneCall = CSpineManager::getInstance()->getNewSpine("BBB/phone");
		_ptrDialLayer->addChild(_ptrPhoneCall);
		_ptrPhoneCall->setPos(1674.f * 0.5f, 943.f * 0.5f);
	}

	void initDrive()
	{
		auto& cfg       = Engine::getCfg();
		auto ptrShizBg = CGfx::getInstance()->spriteFromTexture(CGfx::getInstance()->getTextureById("BBB/shiz_bg.png"));
		Rect rcU{ 0,   0, ptrShizBg->calcNotTransCx(), 580 };
		Rect rcB{ 0, 580, ptrShizBg->calcNotTransCx(), ptrShizBg->calcNotTransCy() - 580 };
		_ptrShizBgU    = CGfx::getInstance()->spriteFromTexture(CGfx::getInstance()->getTextureById("BBB/shiz_bg_u.png"));
		_ptrShizBgB    = CGfx::getInstance()->spriteFromTexture(CGfx::getInstance()->getTextureById("BBB/shiz_bg_b.png"));
		_ptrShizBgCont = std::make_shared<CContainer>();
		_ptrShizBgCont->addChild(_ptrShizBgU);
		_ptrShizBgCont->addChild(_ptrShizBgB);
		_ptrShizBgB->setLightLayer(1);
		_ptrShizBgB->setY(470);
		_ptrShizBg2    = CGfx::getInstance()->spriteFromTexture(CGfx::getInstance()->getTextureById("BBB/shiz_bg2.png"));
		_ptrEnterCubeBg     = CSpineManager::getInstance()->getNewSpine("BBB/enter_cube_bg", false);
		_ptrSanctuaryBg     = CSpineManager::getInstance()->getNewSpine("BBB/sanctuary",     false);
		_ptrEnterCubeBgCont = std::make_shared<CContainer>();
		_ptrShizBgU->setSkipLight(false);
		_ptrShizBgB->setSkipLight(false);
		_ptrShizBgCont->setPosCentered(getDialogCx(), getDialogCy());
		_ptrShizBg2->setPosCentered(getDialogCx(), getDialogCy());
		_ptrNotebook       = SpriteLoader::getInstance()->getSprite("SHIZ/notebook");
		_ptrNotebookBurned = SpriteLoader::getInstance()->getSprite("SHIZ/notebook_burned");
		_ptrShiz        = CSpineManager::getInstance()->getNewSpine("BBB/programmer",  true);
		_ptrLamp        = CSpineManager::getInstance()->getNewSpine("BBB/streetlamp",  true);
		_ptrAmbulance   = CSpineManager::getInstance()->getNewSpine("BBB/ambulance",   true);
		_ptrCubeClosed  = CSpineManager::getInstance()->getNewSpine("BBB/CUBE_CLOSED", false);
		_ptrAngel       = CSpineManager::getInstance()->getNewSpine("BBB/angel",       false);
		_ptrBarbarian   = CSpineManager::getInstance()->getNewSpine("BBB/barbarian",   false);
		auto strPath    = std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::AMBIENT_DUST));
		auto strPath2   = std::format("PARTICLES/{}", ParticlePresets::getPresetSpriteName(eParticlePreset::FIREFLIES));
		_ptrFlies       = std::make_shared<CParticleSystem<>>(ParticlePresets::getPreset(eParticlePreset::AMBIENT_DUST), strPath.c_str());
		_ptrFlies2      = std::make_shared<CParticleSystem<>>(ParticlePresets::getPreset(eParticlePreset::FIREFLIES), strPath.c_str());
		_ptrFlies->prewarm(5);
		_ptrFlies2->prewarm(5);
		_ptrFlies->setSkipLight(false);
		_ptrFlies2->setSkipLight(false);
		_ptrFlies->setLightLayer(1);
		_ptrFlies2->setLightLayer(1);
		_ptrLamp->setLightLayer(1);
		_ptrShiz->setLightLayer(1);
		assert(!_ptrAngel->isSaveTransCoords());
		_ptrEnterCubeBgCont->setScale(1.33f, 1.33f);
		_ptrDragon = CSpineManager::getInstance()->getNewSpine("BBB/dragon");
		_ptrMita   = CSpineManager::getInstance()->getNewSpine("BBB/mita");
		_ptrMita->setInteractive(true);
		_ptrDragon->setLightLayer(2);
		_ptrShiz->setScale(0.4f, 0.4f);
		_ptrLamp->setScale(0.65f, 0.65f);
		_ptrNotebook->setScale(0.5f, 0.5f);
		_ptrNotebookBurned->setScale(0.5f, 0.5f);
		_ptrNotebook->setPos(730, 600);
		_ptrNotebookBurned->setPos(730, 600);
		_ptrNotebookBurned->setVisible(false);
		for (int i = 0; i < SIZE_OF(_ptrFires); i++)
		{
			_ptrFires[i] = CSpineManager::getInstance()->getNewSpine("BBB/buffs_effects");
		}
	}

	void showOrHideSubMenu()
	{
		static std::vector<float> vPos;
		static bool bIsPosVecInitialized = false;
		auto& ch = _ptrBtnDialCont->getChildren();
		for (auto& it : ch)
			it->removeSelfTweens();
		_ptrSubMenuFrame->removeSelfTweens();
		if (!bIsPosVecInitialized)
		{
			vPos.reserve(ch.size());
			bIsPosVecInitialized = true;
			for (auto& it : ch)
			{
				vPos.push_back(it->getY());
			}
		}
		_ptrSubMenuFrame->setAlpha(0);
		constexpr float fDelay    = .1f;
		constexpr float fAnimTime = 1.30f;
		if (!_ptrGameSubMenu->isVisible())
		{
			_ptrGameSubMenu->setVisible(true);
			for (int i = 0; i < ch.size(); i++)
			{
				ch[i]->setAlpha(0);
				auto yPos = vPos[i];
				ch[i]->setY(0);
				ch[i]->addSelfTween(eTweenProp::Y, -150, yPos, fAnimTime, Easing::outBack, i * fDelay);
				ch[i]->addSelfTween(eTweenProp::ALPHA, 0.f, 1.f, 0.1f, Easing::linear, i * fDelay);
			}
			_ptrSubMenuFrame->addSelfTween(eTweenProp::ALPHA, 0.f, .25f, 0.5f, Easing::linear, 1.3f);
		}
		else
		{
			_ptrGameSubMenu->setVisible(false);
		}
	}

	void startDriveGame()
	{
		playDrive([this] {
			_eState = eShizGameState::INITIAL;
			setTimeout(0.2f, [this] {
				_ptrGameHolder->removeAll();
				playIntro();
			});
			CGfx::getInstance()->setWorldDarken(false);
		});
	}

	void realStartGame(bool bSkipDial = false)
	{
		removeSelfTweens();
		CGfx::getInstance()->getPostProcessSettings().lutIntensity   = 0.f;
		CGfx::getInstance()->getPostProcessSettings().grainIntensity = 0.1f;
		CGfx::getInstance()->setWorldDarken(false);
		if (_nFunGameSelected == 2)
		{
			playLastGame([this] {
				_eState = eShizGameState::INITIAL;
				setTimeout(0.2f, [this] {
					_ptrGameHolder->removeAll();
					playIntro();
				});
				CGfx::getInstance()->setWorldDarken(false);
			});
		}
		else
		{
			if (bSkipDial)
			{
				_eState = eShizGameState::DIALING;
				startDriveGame();
			}
			else
			{
				playDial([this] {
					startDriveGame();
				});
			}
		}
	}

	void startFunGame(int nFunGameSelected = 0, bool bImmidiate = false, bool bSkipDial = false)
	{
		_nFunGameSelected = nFunGameSelected;
		_ptrLamp->enableLightEmission(nFunGameSelected == 0);
		if (bImmidiate)
		{
			_eState = eShizGameState::INTRO;
			realStartGame(bSkipDial);
		}
		else
		{
			_ptrGameSubMenu->setVisible(false);
			_ptrBtnGames->setEnabled(false);
			_ptrBtnWatch->setEnabled(false);
			_ptrMainMenu->addSelfTween(eTweenProp::Y, _ptrMainMenu->getY(), -(_ptrMainMenu->calcNotTransCy() + 10), 0.2f, Easing::inBack, 0.f);
			addSelfTween(eTweenProp::GRAIN, 0.1f, 1.f, 1.f, Easing::linear, 2.f);
			addSelfTween(eTweenProp::LUT, 0.f, 1.f, 5.f, Easing::linear, 0.f, [this, bSkipDial] {
				setTimeout(1.f, [this, bSkipDial] {
					realStartGame(bSkipDial);
				});
				CGfx::getInstance()->setWorldDarken(true, 0, 0);
			});
		}
	}

	void bakeSpine(LPCTSTR lpszSpineName, LPCTSTR lpszAnimName, LPCTSTR lpszOutputName)
	{
		auto ptrSpine = CSpineManager::getInstance()->getNewSpine(lpszSpineName, false);
		ptrSpine->recordAnimation(lpszOutputName, lpszAnimName);
	}

	void initIntro()
	{
		LPCTSTR lpszFontName = "secretorigins";
		std::span<SpineVertsDrawable> emptyDrawers;
		const char* fileNames[] = { "BBB/intro.panm" };
		const char* animNames[] = { "animation" };
		CTexturePtr ptrTex =
			CGfx::getInstance()->uploadAsset("BBB/zvezda_prepared.png");
		_ptrIntro = std::make_shared<CBakedSpine>(fileNames, animNames, ptrTex);
		_ptrIntro->setPos(1674.f * 0.5f, 943.f * 0.5f);
		_ptrIntroLayer->addChild(_ptrIntro);
		const int TAG_INTRO = 100;
		_ptrMainMenu = std::make_shared<CContainer>();
		auto ptrSpr   = SpriteLoader::getInstance()->getSprite("UI/menuBtn");
		auto ptrSprHl = SpriteLoader::getInstance()->getSprite("UI/menuBtn_hl");
		auto ptrSpr2   = SpriteLoader::getInstance()->getSprite("UI/menuBtn2");
		auto ptrSprHl2 = SpriteLoader::getInstance()->getSprite("UI/menuBtn2_hl");
		auto ptrSpr3   = SpriteLoader::getInstance()->getSprite("UI/menuBtn3");
		auto ptrSprHl3 = SpriteLoader::getInstance()->getSprite("UI/menuBtn3_hl");
		_ptrBtnWatch   = Button::makeInst(ptrSpr, nullptr, ptrSprHl);
		_ptrBtnGames   = Button::makeInst(ptrSpr, nullptr, ptrSprHl);
		_ptrBtnDialCont = std::make_shared<CContainer>();
		auto ptrBtnDial = Button::makeInst(ptrSpr, nullptr, ptrSprHl);
		ptrBtnDial->setText(L10N::getInstance().tr("MENU_FUN1"));
		ptrBtnDial->setTextFont(lpszFontName, 54.f);
		ptrBtnDial->setOnClick([this] { startFunGame(); });
		_ptrBtnDialCont->addChild(ptrBtnDial);
		ptrBtnDial = Button::makeInst(ptrSpr2, nullptr, ptrSprHl2);
		ptrBtnDial->setText(L10N::getInstance().tr("MENU_FUN2"));
		ptrBtnDial->setTextRgba(0x90FF90FF);
		ptrBtnDial->setTextFont(lpszFontName, 54.f);
		ptrBtnDial->setTextOffsetY(15);
		ptrBtnDial->setOnClick([this] { startFunGame(1); });
		_ptrBtnDialCont->addChild(ptrBtnDial);
		ptrBtnDial = Button::makeInst(ptrSpr3, nullptr, ptrSprHl3);
		ptrBtnDial->setText(L10N::getInstance().tr("MENU_SERIOUS"));
		ptrBtnDial->setTextRgba(0xFF9090FF);
		ptrBtnDial->setTextFont(lpszFontName, 54.f);
		ptrBtnDial->setOnClick([this] { startFunGame(2); });
		_ptrBtnDialCont->addChild(ptrBtnDial);
		_ptrBtnDialCont->alignChildren(eChildrenAlign::VERTICAL_COLUMN, 20.f, getDialogCx());
		auto ptrSprSolid = SpriteLoader::getInstance()->getSprite("UI/mainIconsFrame");
		_ptrSubMenuFrame = getSolidFrame();
		_ptrSubMenuFrame->build(_ptrBtnDialCont->calcNotTransCx() + 80, _ptrBtnDialCont->calcNotTransCy() + 80);
		_ptrGameSubMenu = std::make_shared<CContainer>();
		_ptrGameSubMenu->addChild(_ptrSubMenuFrame);
		_ptrBtnDialCont->setPosCentered(_ptrSubMenuFrame->getNotTransCx(), _ptrSubMenuFrame->getNotTransCy());
		_ptrGameSubMenu->addChild(_ptrBtnDialCont);
		_ptrBtnGames->setText(L10N::getInstance().tr("MENU_MINI_GAMES"));
		_ptrBtnGames->setTextFont(lpszFontName, 64.f);
		_ptrBtnWatch->setText(L10N::getInstance().tr("MENU_ENGINE_FEATURES"));
		_ptrBtnWatch->setTextFont(lpszFontName, 64.f);
		_ptrBtnWatch->setOnClick([this] {
			_ptrIntroLayer->setVisible(false);
			auto ptrDlg = std::make_shared<DialogDemo>();
			ptrDlg->init();
			ptrDlg->open([this] {
				_ptrIntroLayer->setVisible(true);
			});
		});
		auto ptrButtonCont = std::make_shared<CContainer>();
		ptrButtonCont->addChild(_ptrBtnWatch);
		ptrButtonCont->addChild(_ptrBtnGames);
		ptrButtonCont->alignChildren(eChildrenAlign::HORIZONTAL, 30.f, getDialogCx());
		float fFrameCy = ptrButtonCont->calcNotTransCy() + 190.f;
		float fFrameCx = ptrButtonCont->calcNotTransCx() + 80.f;
		ptrButtonCont->setPosCentered(fFrameCx, fFrameCy, 0, -20.f);
		ptrButtonCont->addChild(_ptrGameSubMenu);
		_ptrGameSubMenu->setVisible(false);
		_ptrGameSubMenu->setXPosCentered(_ptrBtnGames->calcNotTransCx(), _ptrBtnGames->getX());
		_ptrGameSubMenu->setY(_ptrBtnGames->calcNotTransCy() + 10);
		_ptrBtnGames->setOnClick([this] {
			showOrHideSubMenu();
		});
		_ptrMenuFrame = getSolidFrame();
		_ptrMenuFrame->build(fFrameCx, fFrameCy);
		_ptrMainMenu->addChild(_ptrMenuFrame);
		_ptrMainMenu->addChild(ptrButtonCont);
		_ptrMenuFrame->setAlpha(0.0f);
		_ptrMainMenu->setXPosCentered(getDialogCx());
		_ptrIntroLayer->addChild(_ptrMainMenu);
		_ptrBtnGames->getParent()->swapChildren(_ptrGameSubMenu, _ptrBtnGames);
	}

	void init()
	{
		setPostProcessing();
		auto& cfg = Engine::getCfg();
		BaseDialog::init(cfg.INIT_SCR_CX, cfg.INIT_SCR_CY, E_ST_NONE, nullptr, nullptr);
		_ptrGameHolder = std::make_shared<CContainer>();
		_root->addChild(_ptrGameHolder);
		_ptrIntroLayer = std::make_shared<CContainer>();
		_ptrDialLayer  = std::make_shared<CContainer>();
		_ptrDriveLayer = std::make_shared<CContainer>();
		initIntro();
		initDial();
		initDrive();
		initLights();
		CGfx::getInstance()->getGameRoot()->addChild(shared_from_this());
		Rect rc = { 0, 0, getDialogCx(), getDialogCy() };
		_root->setScrollBox(&rc);
		showAsPanel(E_IH_RELAXED, false);
		playIntro();
		//playLastGame({});
		//startFunGame(1, true, false); 
		//auto ptrDlg = std::make_shared<DialogDemo>();
		//ptrDlg->init();
		//ptrDlg->open();
	}
};

void MyApp::onFrame(float dt)
{
	if (_ptrGame)
		_ptrGame->trackEye();
}

void MyApp::onResize(float cx, float cy)
{
}

void MyApp::init()
{
	auto& engineCfg = Engine::getCfg();

	// локализация: грузим ini активного языка ДО создания любых диалогов
	L10N::getInstance().load(engineCfg.LANG == eLang::RU ? eLang::RU : eLang::EN);
	Rect rcScreen = { 0, 0, engineCfg.INIT_SCR_CX, engineCfg.INIT_SCR_CY };
	_ptrPreloader = std::make_shared<Preloader>();
	SpineMixingOptions::setMixTime("BBB/ambulance",     "def",           "def",           0.03f);
	SpineMixingOptions::setMixTime("BBB/ambulance",     "idle",          "doctor_arrive", 0.0f);
	SpineMixingOptions::setMixTime("BBB/ambulance",     "doctor_arrive", "doctor_idle",   0.0f);
	SpineMixingOptions::setMixTime("BBB/mita",          "def",           "def",           0.0f);
	SpineMixingOptions::setMixTime("BBB/phone",         "def",           "def",           0.0f);
	SpineMixingOptions::setMixTime("BBB/angel",         "def",           "def",           0.1f);
	SpineMixingOptions::setMixTime("BBB/barbarian",     "def",           "def",           0.1f);
	SpineMixingOptions::setMixTime("BBB/enter_cube_bg", "def",           "def",           0.0f);
	SpineMixingOptions::setMixTime("BBB/sanctuary",     "def",           "def",           0.0f);
	SpineMixingOptions::setMixTime("BBB/alianna",       "def",           "def",           0.0f);
	auto cbOnLoaded = [this]
	{
		SpriteLoader::getInstance()->addAtlas("ui");
		CGfx::getInstance()->uploadAsset("BBB/calling_doctor.jpg");
		CGfx::getInstance()->uploadAsset("BBB/calling_doctor2.jpg");
		CGfx::getInstance()->uploadAsset("BBB/doctor_ambulance.jpg");
		auto sJSON = AssetLoader::instance().getLoadedFileStr("BBB/sounds.json");
		AudioManager::get().loadSpriteMetadata("sfx", "", sJSON.c_str(), [this] {
			_ptrGame = std::make_shared<ShizGame>();
			_ptrGame->init();
		});
	};
	_ptrPreloader->filesToLoad().add("BBB/calling_doctor.jpg");
	_ptrPreloader->filesToLoad().add("BBB/calling_doctor2.jpg");
	_ptrPreloader->filesToLoad().add("BBB/doctor_ambulance.jpg");
	_ptrPreloader->filesToLoad().add("BBB/barb.png");
	_ptrPreloader->filesToLoad().addSpine("programmer", 1, true);
	_ptrPreloader->filesToLoad().addSpine("streetlamp", 1, true);
	_ptrPreloader->filesToLoad().addSpine("ambulance",  1, true);
	_ptrPreloader->filesToLoad().addSpine("phone");
	_ptrPreloader->filesToLoad().addSpine("dragon");
	_ptrPreloader->filesToLoad().addSpine("zvezda_prepared", 1, true);
	_ptrPreloader->filesToLoad().addSpine("mita");
	_ptrPreloader->filesToLoad().addSpine("gargulii");
	_ptrPreloader->filesToLoad().addSpine("buffs_effects");
	_ptrPreloader->filesToLoad().addSpine("angel");
	_ptrPreloader->filesToLoad().addSpine("barbarian");;
	_ptrPreloader->filesToLoad().addSpine("CUBE_CLOSED", 1, false);
	_ptrPreloader->filesToLoad().addSpine("CUBE_OPEN",   1, false);
	_ptrPreloader->filesToLoad().addSpine("enter_cube_bg");
	_ptrPreloader->filesToLoad().addSpine("sanctuary");
	_ptrPreloader->filesToLoad().addSpine("amarantha");
	_ptrPreloader->filesToLoad().addSpine("alianna");
	_ptrPreloader->filesToLoad().addSpine("gametitle");
	_ptrPreloader->filesToLoad().addSpine("portal");
	_ptrPreloader->filesToLoad().add("BBB/shiz_bg.png");
	_ptrPreloader->filesToLoad().add("BBB/shiz_bg_n.png");
	_ptrPreloader->filesToLoad().add("BBB/shiz_bg2.png");
	_ptrPreloader->filesToLoad().add("BBB/shiz_bg_u.png");
	_ptrPreloader->filesToLoad().add("BBB/shiz_bg_b.png");
	_ptrPreloader->filesToLoad().add("BBB/ui.png");
	_ptrPreloader->filesToLoad().add("BBB/ui.atlas");
	_ptrPreloader->filesToLoad().add("BBB/sounds.json");
	_ptrPreloader->filesToLoad().add("BBB/sounds.mp3");
	_ptrPreloader->filesToLoad().add("BBB/boss_bg.mp3");
	_ptrPreloader->filesToLoad().add("BBB/axe03.mp3");
	_ptrPreloader->filesToLoad().add("BBB/axe04.mp3");
	_ptrPreloader->filesToLoad().add("BBB/intro.panm");
	_ptrPreloader->filesToLoad().add("BBB/barbarian_idle.panm");
	_ptrPreloader->filesToLoad().add("BBB/barbarian_run.panm");
	_ptrPreloader->filesToLoad().setSizeToLoad(63996535);
	_ptrPreloader->init(cbOnLoaded);
	HTMLDom::getInstance()->parseCSSFromFile("EMBED/HTML/styles.css");
	HTMLDom::getInstance()->registerParticlesPreset("blood_mist", []()
	{
		auto ps = std::make_shared<CParticleSystem<>>(ParticlePresets::getPreset(eParticlePreset::BUTTON_BLOOD_FOG), "PARTICLES/smoke_07");
		return std::static_pointer_cast<CContainer>(ps);
	});
}