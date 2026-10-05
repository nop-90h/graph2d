#pragma once

#include "container.h"
#include "sprite.h"

#include <vector>
#include <memory>

using CardsArray = std::array<CSpritePtr, 5>;

class CCardReveal : public CContainer
{
public:
    static constexpr int CARD_COUNT = 5;

    CCardReveal(float fSceneCx, float fSceneCy);

    // ”становить одну карту: face Ч лицева€ часть, back Ч рубашка
    void setCard(int nIndex, CSpritePtr ptrFace, CSpritePtr ptrBack);

    // ”становить сразу до 5 карт
    void setCards(const CardsArray& vFaces,
                  const CardsArray& vBacks);

    void setSceneSize(float fSceneCx, float fSceneCy);

    // «апуск анимации по€влени€ и переворота
    void play(SimpleCallback onComplete = {});

    // —брос в исходное состо€ние: карты сто€т по местам рубашкой вверх
    void reset();
    auto getCardFace(int nCardIndex) { return _vCards[nCardIndex].ptrFace; } 
    virtual void update(float dt) override;

private:
    struct CardItem
    {
        bool            bInit      = false;
        CContainerPtr   ptrRoot    = nullptr;  // контейнер одной карты
        CSpritePtr      ptrFace    = nullptr;  // лицо
        CSpritePtr      ptrBack    = nullptr;  // рубашка
        float           fTargetX   = 0.f;
        float           fTargetY   = 0.f;
    };

    float                 _fSceneCx   = 1.f;
    float                 _fSceneCy   = 1.f;
                          
    float                 _fCardW     = 120.f;
    float                 _fCardH     = 180.f;
    float                 _fCardScale = 1.f;
                          
    bool                  _bPrepared  = false;
    SimpleCallback        _cb         = {};
    std::vector<CardItem> _vCards;

    std::weak_ptr<CCardReveal> getWeakThis();

    void clearCards();

    void layout();
    void centerSprite(CSpritePtr ptrSprite);

    void prepareCard(int nIndex);

    float getEntranceDuration(int nIndex) const;

    void animateEntrance(int nIndex, float fDelay);
    void scheduleFlip(int nIndex, float fDelay);

    void flipCardNow(int nIndex);
    void onFlipMidpoint(int nIndex);
};