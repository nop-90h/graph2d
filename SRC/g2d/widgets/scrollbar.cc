#include "widgets/scrollbar.h"
#include "spriteloader.h"
#include "sceneresize.h"
#include <algorithm>

_G2D_NAMESPACE_BEGIN_

void ScrollBar::create(eScrollType eType, float fSize)
{
    _fSize = fSize;
    _eType = eType;

    float fFrameCx = 20.f;

    switch (eType)
    {
    case eScrollType::E_ST_VERT:
    {
        LPCCTEXT lpccFrame  = _lpccFrame  ? _lpccFrame  : "UI/scrollBarFrameV";
        LPCCTEXT lpccSlider = _lpccSlider ? _lpccSlider : "UI/scrollBarSliderV";

        if (_bFrameVisible)
        {
            ThreeSliceVertPtr ptrFrameV = std::make_shared<ThreeSliceVert>();
            _ptrFrameV = ptrFrameV;

            ptrFrameV->createSlices(SpriteLoader::getInstance()->getSprite(lpccFrame), _fc, _fd);
            ptrFrameV->build(fSize);

            addChild(_ptrFrameV);

            fFrameCx = ptrFrameV->getCx();
        }
        else
        {
            _ptrFrameV = std::make_shared<CContainer>();
            addChild(_ptrFrameV);
        }

        _ptrSlider = SpriteLoader::getInstance()->getSprite(lpccSlider);
        _ptrFrameV->addChild(_ptrSlider);

        _ptrSlider->setX((fFrameCx - _ptrSlider->getNotTransCx()) / 2.f);
    }
    break;

    case eScrollType::E_ST_HOR:
    {
        LPCCTEXT lpccFrame  = _lpccFrame  ? _lpccFrame  : "UI/scrollBarFrameH";
        LPCCTEXT lpccSlider = _lpccSlider ? _lpccSlider : "UI/scrollBarSliderH";

        _ptrFrameH = ThreeSliceHorPtr(new ThreeSliceHor());
        _ptrFrameH->createSlices(SpriteLoader::getInstance()->getSprite(lpccFrame), _fc, _fd);
        _ptrFrameH->build(fSize);

        _ptrSlider = SpriteLoader::getInstance()->getSprite(lpccSlider);
        _ptrFrameH->addChild(_ptrSlider);

        _ptrSlider->setY((_ptrFrameH->getCy() - _ptrSlider->getNotTransCy()) / 2.f);

        addChild(_ptrFrameH);
    }
    break;

    default:
        assert(false);
        break;
    }

    if (!_bIsSystem)
    {
        _ptrSlider->setId();
        _ptrSlider->setOnMove([this](bool bIsPressed, int x, int y)
        {
            if (bIsPressed)
                onSliderDrag(x, y);
        });
    }
}

void ScrollBar::setSliderPos(float fZeroToOnePos)
{
    assert(isgreaterequal(fZeroToOnePos, 0.f));

    fZeroToOnePos = std::min(1.f, fZeroToOnePos);
    fZeroToOnePos = std::max(0.f, fZeroToOnePos);

    _pos = fZeroToOnePos;

    switch (_eType)
    {
    case eScrollType::E_ST_VERT:
    {
        _ptrSlider->setY((_fSize - _ptrSlider->getNotTransCy()) * _pos);
    }
    break;

    case eScrollType::E_ST_HOR:
    {
        _ptrSlider->setX((_fSize - _ptrSlider->getNotTransCx()) * _pos);
    }
    break;

    default:
    {
        assert(false);
    }
    break;
    }
}

bool ScrollBar::isSlider(CContainer* pTest)
{
    bool bRes = false;

    assert(pTest);

    if (pTest)
        bRes = _ptrSlider.get() == pTest;

    return bRes;
}

float ScrollBar::calcScrollPercent(float fScreenXorY)
{
    float fRes = 0;

    assert(_eType != eScrollType::E_ST_NONE);

    if (_eType != eScrollType::E_ST_NONE)
    {
        float fScaleX = 1.f, fScaleY = 1.f;
        calcTotalScale(fScaleX, fScaleY);

        float fScale    = _eType == eScrollType::E_ST_HOR ? fScaleX : fScaleY;
        float fXorY     = fScreenXorY - (_eType == eScrollType::E_ST_HOR ? calcScreenX() + CSceneResize::getInstance()->getGameOffsX() : calcScreenY() + CSceneResize::getInstance()->getGameOffsY());
        float fSize     = _fSize * fScale;

        fXorY           = std::max(fXorY, 0.f);
        fXorY           = std::min(fXorY, fSize);

        fRes = fXorY / fSize;
    }

    return fRes;
}

// === FIX START ===
float ScrollBar::getSliderLength()
{
    if (!_ptrSlider)
        return 0.f;

    switch (_eType)
    {
    case eScrollType::E_ST_VERT:
        return _ptrSlider->getNotTransCy();

    case eScrollType::E_ST_HOR:
        return _ptrSlider->getNotTransCx();

    default:
        return 0.f;
    }
}

float ScrollBar::getSliderTravel()
{
    return std::max(0.f, _fSize - getSliderLength());
}

void ScrollBar::beginSliderDrag(float fScreenXorY)
{
    if (_eType == eScrollType::E_ST_NONE || !_ptrSlider)
        return;

    if (_fSize <= 0.f)
    {
        _bDraggingSlider = false;
        _fDragOffsetLocal = 0.f;
        return;
    }

    // ѕозици€ курсора вдоль всего трека, нормализованна€ в [0..1]
    float fPointer01 = calcScrollPercent(fScreenXorY);

    // ѕереводим в локальные пиксели трека
    float fPointerLocal = fPointer01 * _fSize;

    // “екуща€ позици€ слайдера в локальных пиксел€х трека
    float fTravel = getSliderTravel();
    float fSliderLocal = _pos * fTravel;

    // «апоминаем смещение курсора относительно начала слайдера
    _fDragOffsetLocal = fPointerLocal - fSliderLocal;
    _bDraggingSlider = true;
}

float ScrollBar::calcSliderDragPercent(float fScreenXorY)
{
    if (_eType == eScrollType::E_ST_NONE || !_ptrSlider || _fSize <= 0.f)
        return _pos;

    if (!_bDraggingSlider)
        return calcScrollPercent(fScreenXorY);

    float fTravel = getSliderTravel();
    if (fTravel <= 0.f)
        return _pos;

    float fPointer01 = calcScrollPercent(fScreenXorY);
    float fPointerLocal = fPointer01 * _fSize;

    float fDesiredSliderLocal = fPointerLocal - _fDragOffsetLocal;
    float fRes = fDesiredSliderLocal / fTravel;

    fRes = std::max(0.f, std::min(1.f, fRes));

    return fRes;
}

void ScrollBar::endSliderDrag()
{
    _bDraggingSlider = false;
    _fDragOffsetLocal = 0.f;
}
// === FIX END ===

void ScrollBar::setOptions(bool bFrameVisible, LPCCTEXT lpccFrame, LPCCTEXT lpccSlider, float fc, float fd, bool bIsSystem)
{
    _lpccFrame     = lpccFrame;
    _lpccSlider    = lpccSlider;
    _bFrameVisible = bFrameVisible;

    _fc = fc;
    _fd = fd;

    _bIsSystem = bIsSystem;
}

bool ScrollBar::getNotTransBounds(Rect* p)
{
    Rect rcFrame;

    switch (_eType)
    {
    case E_ST_VERT:
        _ptrFrameV->getNotTransBounds(&rcFrame);
        break;

    case E_ST_HOR:
        _ptrFrameH->getNotTransBounds(&rcFrame);
        break;
    }

    _ptrSlider->getNotTransBounds(p);
    p->unite(&rcFrame);

    return true;
}

void ScrollBar::onSliderDrag(int x, int y)
{
    // ƒл€ системных скроллбаров основной путь идЄт через BaseDialog
    // и использует beginSliderDrag()/calcSliderDragPercent().
    // «десь оставлен безопасный вызов: если драг не был €вно начат,
    // поведение останетс€ прежним.
    float fPerc = calcSliderDragPercent(_eType == E_ST_VERT ? y : x);

    setSliderPos(fPerc);

    if (_cbOnScrolled)
        _cbOnScrolled();
}

_G2D_NAMESPACE_END_