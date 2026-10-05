#include "pch.h"
#include "slotmachine.h"
#include "spriteloader.h"
#include "nineslice.h"

namespace
{
    const int PAYLINES[SlotMath::LINE_COUNT][SlotMath::COLS] = {
        {1,1,1,1,1}, {0,0,0,0,0}, {2,2,2,2,2}, {0,1,2,1,0}, {2,1,0,1,2},
        {0,0,1,2,2}, {2,2,1,0,0}, {1,0,1,2,1}, {1,2,1,0,1}, {0,2,0,2,0}
    };

    std::mt19937& getRng()
    {
        static std::mt19937 r{ std::random_device{}() };
        return r;
    }

    int payPlaceholder(int sym, int cnt)
    {
        if (cnt < SlotMath::MIN_WIN_COUNT) return 0;
        return cnt * 10 * std::max(1, SlotMath::SYMBOL_COUNT - sym);
    }
}

namespace SlotMath
{
    int randomSymbol()
    {
        std::uniform_int_distribution<int> d(0, SYMBOL_COUNT - 1);
        return d(getRng());
    }

    Matrix generateMatrix()
    {
        Matrix m{};
        for (int& s : m) s = randomSymbol();
        return m;
    }

    Matrix generateFirstSpinMatrix()
    {
        Matrix m{};

        const int symA = randomSymbol();

        int symB = randomSymbol();
        while (symB == symA) symB = randomSymbol();

        int symC = randomSymbol();
        while (symC == symA || symC == symB) symC = randomSymbol();

        // ЛИНИЯ 1 ({0,0,0,0,0}, верхняя строка): A во всех 5 колонках -> 5 из 5
        for (int col = 0; col < COLS; ++col)
            m[col * ROWS + 0] = symA;

        // ЛИНИЯ 0 ({1,1,1,1,1}, средняя): B в колонках 0..2 -> ровно 3
        for (int col = 0; col < COLS; ++col)
        {
            if (col < 3)
            {
                m[col * ROWS + 1] = symB;
            }
            else
            {
                int v = randomSymbol();
                if (col == 3) while (v == symB) v = randomSymbol();
                m[col * ROWS + 1] = v;
            }
        }

        // ЛИНИЯ 2 ({2,2,2,2,2}, нижняя): C в колонках 0..2 -> ровно 3
        for (int col = 0; col < COLS; ++col)
        {
            if (col < 3)
            {
                m[col * ROWS + 2] = symC;
            }
            else
            {
                int v = randomSymbol();
                if (col == 3) while (v == symC) v = randomSymbol();
                m[col * ROWS + 2] = v;
            }
        }

        return m;
    }

    int makeLineCells(int line, int count, LineCells& outCells)
    {
        if (line < 0 || line >= LINE_COUNT) return 0;

        count = std::clamp(count, 0, COLS);

        int n = 0;
        for (int c = 0; c < count; ++c)
            outCells[n++] = { c, PAYLINES[line][c] };

        return n;
    }

    std::vector<SlotWinLineConfig_t> evaluateWins(const Matrix& matrix, int minCount)
    {
        std::vector<SlotWinLineConfig_t> wins;

        for (int line = 0; line < LINE_COUNT; ++line)
        {
            const int first = matrix[0 * ROWS + PAYLINES[line][0]];
            if (first < 0 || first >= SYMBOL_COUNT) continue;

            int cnt = 0;
            for (int col = 0; col < COLS; ++col)
            {
                if (matrix[col * ROWS + PAYLINES[line][col]] == first)
                    ++cnt;
                else
                    break;
            }

            if (cnt >= minCount)
            {
                SlotWinLineConfig_t w;
                w.nLineIndex = line;
                w.nSymbolId = first;
                w.nNumOfSymbols = cnt;
                w.nWin = payPlaceholder(first, cnt);
                w.nCellCount = makeLineCells(line, cnt, w.cells);

                wins.push_back(w);
            }
        }
        return wins;
    }

    void generateSpin(Matrix& matrix, std::vector<SlotWinLineConfig_t>& wins)
    {
        matrix = generateMatrix();
        wins = evaluateWins(matrix);
    }
}

namespace SlotConfig
{
    constexpr float MAX_SPEED        = 3300.f;
    constexpr float ACCEL_RATE       = 8500.f;
    constexpr float DECEL_RATE       = 16000.f;
    constexpr float MIN_DECEL_FACTOR = 0.50f;
    constexpr float ALIGN_SLOW_SPEED = 2350.f;

    constexpr float BOUNCE_AMPLITUDE = 14.f;
    constexpr float BOUNCE_DURATION  = 0.12f;
    constexpr float REEL_START_DELAY = 0.12f;

    constexpr int BASE_STRIP_CELLS   = 30;
}

CSlotMachine::CSlotMachine() {}

bool CSlotMachine::initMachine(float startX, float startY)
{
    _isSpinning = false;
    _currentStoppingReel = 0;
    _firstSpinDone = false;

    setPos(startX, startY);

    auto ptrTest = SpriteLoader::getInstance()->getSprite("SLOT/sym_0");

    Rect rcBounds;
    ptrTest->getNotTransBounds(&rcBounds);
    _symbolHeight = (float)rcBounds.cy;
    _reelWidth    = (float)rcBounds.cx;

    constexpr float fReelPad = 12.f;

    for (int i = 0; i < SlotMath::COLS; ++i)
    {
        auto& reel = _novomaticReels[i];

        // На случай повторной инициализации — снимаем старый контейнер
        if (reel.ptrContainer)
            removeChild(reel.ptrContainer);

        reel = NovomaticReel_t{};

        reel.ptrContainer = std::make_shared<CContainer>();
        reel.ptrContainer->setPos(i * (_reelWidth + fReelPad), 0.f);

        // 6 ячеек: -2H, -H, 0, H, 2H, 3H
        for (int j = 0; j < SlotMath::CELLS_PER_REEL; ++j)
        {
            const int randomSymId = SlotMath::randomSymbol();
            const std::string symPath = std::format("SLOT/sym_{}", randomSymId);

            auto ptrSym = SpriteLoader::getInstance()->getSprite(symPath.c_str());
            if (!ptrSym) ptrSym = SpriteLoader::getInstance()->getSprite("UI/checkbox_checked");

            const float initialY = (j - 2) * _symbolHeight;

            ptrSym->setPos(0.f, initialY);
            reel.ptrContainer->addChild(ptrSym);

            SlotCell_t cell;
            cell.pSprite = ptrSym;
            cell.nSymbolId = randomSymId;
            cell.fBaseTargetY = initialY;

            reel.cells[j] = cell;
        }

        addChild(reel.ptrContainer);
    }
    return true;
}

void CSlotMachine::spin()
{
    SlotMath::Matrix matrix{};
    std::vector<SlotWinLineConfig_t> wins;

    // Первый спин — гарантированный выигрыш по 3 линиям, одна из 5 символов
    if (!_firstSpinDone)
    {
        matrix = SlotMath::generateFirstSpinMatrix();
        wins = SlotMath::evaluateWins(matrix);
    }
    else
    {
        SlotMath::generateSpin(matrix, wins);
    }

    startSpin(matrix, wins);
}

void CSlotMachine::startSpin(SlotMath::Matrix& targetMatrix,
                             const std::vector<SlotWinLineConfig_t>& winConfigs)
{
    if (_isSpinning) return;

    const bool firstSpin = !_firstSpinDone;
    _firstSpinDone = true;

    _isSpinning = true;
    _currentStoppingReel = 0;

    if ((int)targetMatrix.size() == SlotMath::MATRIX_SIZE)
        std::copy_n(targetMatrix.begin(), SlotMath::MATRIX_SIZE, _targetMatrix.begin());
    else if (firstSpin)
        _targetMatrix = SlotMath::generateFirstSpinMatrix();
    else
        _targetMatrix = SlotMath::generateMatrix();

    _currentWins = winConfigs;

    if (_currentWins.empty())
        _currentWins = SlotMath::evaluateWins(_targetMatrix);

    normalizeWins();
    removeSelfTweens();

    for (int i = 0; i < SlotMath::COLS; ++i)
    {
        auto& reel = _novomaticReels[i];

        reel.fCurrentSpeed = 0.f;
        reel.fTimer = i * SlotConfig::REEL_START_DELAY;
        reel.fReelOffset = 0.f;
        reel.fBounceTime = 0.f;
        reel.state = eReelState::ACCEL;

        reel.stripIndex = 0;
        reel.stripSize = 0;

        // Сбрасываем baseY ячеек на начальные позиции (-2H .. 3H)
        for (int j = 0; j < SlotMath::CELLS_PER_REEL; ++j)
            reel.cells[j].fBaseTargetY = (j - 2) * _symbolHeight;

        // --- ФОРМИРУЕМ ЛЕНТУ ---

        // Текущие 6 символов (что видно сейчас — без подмены!)
        for (int j = 0; j < SlotMath::CELLS_PER_REEL; ++j)
            reel.strip[reel.stripSize++] = reel.cells[j].nSymbolId;

        // Промежуточные символы (округляем до кратного 6)
        int extraCells = SlotConfig::BASE_STRIP_CELLS + i * 7;
        int rem = extraCells % SlotMath::CELLS_PER_REEL;
        if (rem != 0) extraCells += SlotMath::CELLS_PER_REEL - rem;

        // Страховка от переполнения фиксированного буфера
        const int maxExtra = SlotMath::MAX_STRIP_LEN - 2 * SlotMath::CELLS_PER_REEL;
        if (extraCells > maxExtra) extraCells = maxExtra;

        for (int j = 0; j < extraCells; ++j)
            reel.strip[reel.stripSize++] = SlotMath::randomSymbol();

        // Целевой блок из 6 символов.
        // При остановке окно читает ленту так:
        //   row0 = strip[target+3], row1 = strip[target+2], row2 = strip[target+1]
        //   скрытый низ = strip[target], скрытые верх = strip[target+4], strip[target+5]
        reel.strip[reel.stripSize++] = SlotMath::randomSymbol(); // нижний скрытый

        // ВАЖНО: строки в обратном порядке (row2, row1, row0)
        for (int row = SlotMath::ROWS - 1; row >= 0; --row)
            reel.strip[reel.stripSize++] = _targetMatrix[i * SlotMath::ROWS + row];

        reel.strip[reel.stripSize++] = SlotMath::randomSymbol(); // скрытый -H
        reel.strip[reel.stripSize++] = SlotMath::randomSymbol(); // скрытый -2H

        reel.targetStripIndex = SlotMath::CELLS_PER_REEL + extraCells;

        // Сброс спрайтов
        for (auto& cell : reel.cells)
        {
            if (!cell.pSprite) continue;

            cell.pSprite->removeSelfTweens();
            cell.pSprite->setPos(0.f, cell.fBaseTargetY);
            cell.pSprite->setAlpha(1.f);
            cell.pSprite->setAddRgba(0.f, 0.f, 0.f, 0.f);
        }
    }
}

void CSlotMachine::update(float dt)
{
    CContainer::update(dt);

    if (dt > 0.133f) dt = 0.133f;

    for (int i = 0; i < SlotMath::COLS; ++i)
    {
        auto& reel = _novomaticReels[i];

        if (reel.state == eReelState::IDLE) continue;

        // ====================================================================
        // 1. УПРАВЛЕНИЕ СКОРОСТЬЮ И СОСТОЯНИЯМИ
        // ====================================================================

        if (reel.state == eReelState::ACCEL)
        {
            if (reel.fTimer > 0.f) { reel.fTimer -= dt; continue; }

            reel.fCurrentSpeed += SlotConfig::ACCEL_RATE * dt;

            if (reel.fCurrentSpeed >= SlotConfig::MAX_SPEED)
            {
                reel.fCurrentSpeed = SlotConfig::MAX_SPEED;
                reel.state = eReelState::SPIN_FAST;
            }
        }
        else if (reel.state == eReelState::SPIN_FAST)
        {
            if (i == _currentStoppingReel && reel.stripIndex >= reel.targetStripIndex - 4)
                reel.state = eReelState::DECEL;
        }
        else if (reel.state == eReelState::DECEL)
        {
            const float fDecelFactor = std::clamp(reel.fCurrentSpeed / SlotConfig::MAX_SPEED,
                                                  SlotConfig::MIN_DECEL_FACTOR, 1.0f);

            reel.fCurrentSpeed -= SlotConfig::DECEL_RATE * fDecelFactor * dt;

            if (reel.fCurrentSpeed < SlotConfig::ALIGN_SLOW_SPEED)
                reel.fCurrentSpeed = SlotConfig::ALIGN_SLOW_SPEED;

            if (reel.stripIndex >= reel.targetStripIndex - 1)
                reel.state = eReelState::ALIGNING;
        }
        else if (reel.state == eReelState::ALIGNING)
        {
            reel.fCurrentSpeed = SlotConfig::ALIGN_SLOW_SPEED;
        }
        else if (reel.state == eReelState::BOUNCE)
        {
            reel.fBounceTime += dt;

            if (reel.fBounceTime >= SlotConfig::BOUNCE_DURATION)
            {
                reel.fBounceTime = SlotConfig::BOUNCE_DURATION;
                reel.state = eReelState::IDLE;

                for (auto& cell : reel.cells)
                    if (cell.pSprite) cell.pSprite->setY(cell.fBaseTargetY + reel.fReelOffset);

                _currentStoppingReel++;

                if (_currentStoppingReel > SlotMath::COLS - 1)
                {
                    _isSpinning = false;

                    if (_currentWins.empty()) resetHighlight();
                    else highlightWinLines();
                }

                continue;
            }
        }

        // ====================================================================
        // 2. ДВИЖЕНИЕ ЛЕНТЫ
        // ====================================================================

        if (reel.state != eReelState::BOUNCE)
        {
            reel.fReelOffset += reel.fCurrentSpeed * dt;

            while (reel.fReelOffset >= _symbolHeight)
            {
                reel.fReelOffset -= _symbolHeight;
                reel.stripIndex++;

                // Компенсация: все baseY увеличиваем на H
                for (auto& cell : reel.cells)
                    cell.fBaseTargetY += _symbolHeight;

                // Находим нижнюю ячейку
                int maxIdx = 0;
                for (int j = 1; j < SlotMath::CELLS_PER_REEL; ++j)
                    if (reel.cells[j].fBaseTargetY > reel.cells[maxIdx].fBaseTargetY)
                        maxIdx = j;

                // Телепортируем в ряд -2H (полностью невидимый) —
                // подмена символа происходит за экраном
                reel.cells[maxIdx].fBaseTargetY -= _symbolHeight * (float)SlotMath::CELLS_PER_REEL;

                // Новый символ для телепортированной ячейки
                int stripPos = reel.stripIndex + (SlotMath::CELLS_PER_REEL - 1);
                if (stripPos < reel.stripSize)
                    setCellSymbol(i, maxIdx, reel.strip[stripPos]);

                // Достигли целевой позиции?
                if (reel.stripIndex >= reel.targetStripIndex)
                {
                    // Встаём точно в паз: остаток НЕ переносим
                    reel.fReelOffset = 0.f;
                    reel.fCurrentSpeed = 0.f;
                    reel.fBounceTime = 0.f;
                    reel.state = eReelState::BOUNCE;

                    break;
                }
            }

            // Обновляем координаты всех спрайтов
            for (auto& cell : reel.cells)
            {
                if (!cell.pSprite) continue;
                cell.pSprite->setY(cell.fBaseTargetY + reel.fReelOffset);
            }
        }
        else
        {
            // ====================================================================
            // 3. ОТСКОК (BOUNCE)
            // ====================================================================

            float t = reel.fBounceTime / SlotConfig::BOUNCE_DURATION;

            float bounceOffset = 4.f * SlotConfig::BOUNCE_AMPLITUDE * t * (1.f - t);

            for (auto& cell : reel.cells)
            {
                if (!cell.pSprite) continue;

                cell.pSprite->setY(
                    cell.fBaseTargetY +
                    reel.fReelOffset +
                    bounceOffset
                );
            }
        }
    }
}

void CSlotMachine::setCellSymbol(int reelIndex, int cellIndex, int symbolId)
{
    if (reelIndex < 0 || reelIndex >= SlotMath::COLS) return;

    auto& reel = _novomaticReels[reelIndex];
    if (cellIndex < 0 || cellIndex >= SlotMath::CELLS_PER_REEL) return;

    auto ptrNew = SpriteLoader::getInstance()->getSprite(std::format("SLOT/sym_{}", symbolId).c_str());
    if (!ptrNew) ptrNew = SpriteLoader::getInstance()->getSprite("UI/checkbox_checked");
    if (!ptrNew) return;

    auto& cell = reel.cells[cellIndex];
    auto ptrOld = cell.pSprite;

    if (ptrOld == ptrNew) return;

    if (ptrOld)
    {
        ptrOld->removeSelfTweens();
        ptrOld->setAlpha(0.f);
        reel.ptrContainer->removeChild(ptrOld);
    }

    ptrNew->setPos(0.f, cell.fBaseTargetY);
    ptrNew->setAlpha(1.f);
    ptrNew->setAddRgba(0.f, 0.f, 0.f, 0.f);

    reel.ptrContainer->addChild(ptrNew);

    cell.pSprite = ptrNew;
    cell.nSymbolId = symbolId;
}

void CSlotMachine::normalizeWins()
{
    std::vector<SlotWinLineConfig_t> out;

    for (auto w : _currentWins)
    {
        if (w.nNumOfSymbols < SlotMath::MIN_WIN_COUNT) continue;

        if (w.nCellCount == 0 && w.nLineIndex >= 0 && w.nLineIndex < SlotMath::LINE_COUNT)
            w.nCellCount = SlotMath::makeLineCells(w.nLineIndex, w.nNumOfSymbols, w.cells);

        if (w.nCellCount > 0) out.push_back(w);
    }

    if (out.empty() && !_currentWins.empty())
        out = SlotMath::evaluateWins(_targetMatrix);

    _currentWins.swap(out);
}

void CSlotMachine::resetHighlight()
{
    removeSelfTweens();

    for (auto& reel : _novomaticReels)
        for (auto& cell : reel.cells)
        {
            if (!cell.pSprite) continue;

            cell.pSprite->removeSelfTweens();
            cell.pSprite->setAlpha(1.f);
            cell.pSprite->setAddRgba(0.f, 0.f, 0.f, 0.f);
        }
}

void CSlotMachine::highlightWinLines()
{
    resetHighlight();

    if (_currentWins.empty()) return;

    std::vector<SlotWinLineConfig_t> active;

    for (const auto& w : _currentWins)
        if (w.nCellCount > 0) active.push_back(w);

    if (active.empty()) return;

    auto show = [this, active](int idx, auto& self) -> void
    {
        if (idx >= (int)active.size()) idx = 0;

        const auto& cur = active[idx];

        // Затемняем все символы
        for (int col = 0; col < SlotMath::COLS; ++col)
            for (auto& cell : _novomaticReels[col].cells)
            {
                if (!cell.pSprite) continue;

                cell.pSprite->removeSelfTweens();
                cell.pSprite->setAlpha(0.20f);
                cell.pSprite->setAddRgba(-0.15f, -0.15f, -0.15f, 0.f);
            }

        // Подсвечиваем ячейки выигрышной линии
        for (int c = 0; c < cur.nCellCount; ++c)
        {
            const auto& pos = cur.cells[c];

            const int col = pos.first;
            const int row = pos.second;

            if (col < 0 || col >= SlotMath::COLS) continue;
            if (row < 0 || row >= SlotMath::ROWS) continue;

            auto& reel = _novomaticReels[col];

            float targetY = row * _symbolHeight;

            int bestIdx = -1;
            float bestDiff = 99999.f;

            for (int j = 0; j < SlotMath::CELLS_PER_REEL; ++j)
            {
                if (!reel.cells[j].pSprite) continue;

                float y = reel.cells[j].fBaseTargetY + reel.fReelOffset;
                float diff = std::abs(y - targetY);

                if (diff < bestDiff)
                {
                    bestDiff = diff;
                    bestIdx = j;
                }
            }

            // Защита от рассинхрона: подсвечиваем, только если символ совпадает
            if (bestIdx >= 0 &&
                bestDiff < _symbolHeight * 0.5f &&
                reel.cells[bestIdx].nSymbolId == cur.nSymbolId)
            {
                auto& cell = reel.cells[bestIdx];

                cell.pSprite->setAlpha(1.f);
                cell.pSprite->setAddRgba(0.12f, 0.12f, 0.f, 0.f);

                animateSpritePulse(cell.pSprite, true);
            }
        }

        if (active.size() > 1)
        {
            removeSelfTweens();

            addSelfTween(eTweenProp::ALPHA, getAlpha(), getAlpha(), 1.8f, Easing::linear, 0.f,
                [idx, self]() { self(idx + 1, self); });
        }
    };

    auto copy = show;
    show(0, copy);
}

void CSlotMachine::animateSpritePulse(CSpritePtr ptrSprite, bool bFadeOut)
{
    if (!ptrSprite) return;

    const float startAlpha = bFadeOut ? 1.0f : 0.35f;
    const float endAlpha   = bFadeOut ? 0.35f : 1.0f;

    ptrSprite->addSelfTween(eTweenProp::ALPHA, startAlpha, endAlpha, 0.25f, Easing::inOutSine, 0.f,
        [this, w = ptrSprite->weak_from_this(), bFadeOut]()
        {
            if (auto ptr = std::static_pointer_cast<CSprite>(w.lock()))
                this->animateSpritePulse(ptr, !bFadeOut);
        });
}