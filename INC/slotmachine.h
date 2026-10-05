#pragma once
#include "container.h"
#include "sprite.h"
#include <array>
#include <vector>
#include <utility>

namespace SlotMath
{
    constexpr int COLS = 5;
    constexpr int ROWS = 3;
    constexpr int LINE_COUNT = 10;
    constexpr int SYMBOL_COUNT = 12;
    constexpr int MIN_WIN_COUNT = 3;

    // Фиксированные размеры — std::array
    constexpr int CELLS_PER_REEL = 6;            // 3 видимых + 1 скрытый снизу + 2 скрытых сверху
    constexpr int MATRIX_SIZE    = ROWS * COLS;  // 15
    constexpr int MAX_STRIP_LEN  = 128;          // максимум ленты на спин (с запасом)

    using Matrix    = std::array<int, MATRIX_SIZE>;
    using LineCells = std::array<std::pair<int, int>, COLS>;
}

struct SlotWinLineConfig_t
{
    int nLineIndex = -1;
    int nSymbolId = -1;
    int nNumOfSymbols = 0;
    int nWin = 0;

    SlotMath::LineCells cells{};
    int nCellCount = 0;
};

struct SlotCell_t
{
    CSpritePtr pSprite;
    int nSymbolId = 0;
    float fBaseTargetY = 0.f;
};

enum class eReelState { IDLE, ACCEL, SPIN_FAST, DECEL, ALIGNING, BOUNCE };

struct NovomaticReel_t
{
    eReelState state = eReelState::IDLE;

    float fCurrentSpeed = 0.f;
    float fTimer = 0.f;
    float fReelOffset = 0.f;
    float fBounceTime = 0.f;

    std::array<SlotCell_t, SlotMath::CELLS_PER_REEL> cells{};
    CContainerPtr ptrContainer;

    std::array<int, SlotMath::MAX_STRIP_LEN> strip{};
    int stripSize = 0;

    int stripIndex = 0;
    int targetStripIndex = 0;
};

namespace SlotMath
{
    int randomSymbol();
    Matrix generateMatrix();

    // Матрица первого спина: гарантированно 3 линии, одна из 5 символов
    Matrix generateFirstSpinMatrix();

    int makeLineCells(int lineIndex, int count, LineCells& outCells);
    std::vector<SlotWinLineConfig_t> evaluateWins(const Matrix& matrix, int minCount = MIN_WIN_COUNT);
    void generateSpin(Matrix& matrix, std::vector<SlotWinLineConfig_t>& wins);
}

class CSlotMachine : public CContainer
{
private:
    bool _isSpinning = false;
    bool _firstSpinDone = false;
    int _currentStoppingReel = 0;

    float _symbolHeight = 160.f;
    float _reelWidth = 175.f;

    std::array<NovomaticReel_t, SlotMath::COLS> _novomaticReels{};
    SlotMath::Matrix _targetMatrix{};

    std::vector<SlotWinLineConfig_t> _currentWins;

public:
    CSlotMachine();
    virtual ~CSlotMachine() = default;

    bool initMachine(float startX, float startY);
    void spin();

    void startSpin(SlotMath::Matrix& targetMatrix, const std::vector<SlotWinLineConfig_t>& winConfigs);

    virtual void update(float dt) override;

    bool isSpinning() const { return _isSpinning; }

private:
    void setCellSymbol(int reelIndex, int cellIndex, int symbolId);
    void normalizeWins();
    void resetHighlight();
    void highlightWinLines();
    void animateSpritePulse(CSpritePtr ptrSprite, bool bFadeOut);
};

typedef std::shared_ptr<CSlotMachine> CSlotMachinePtr;