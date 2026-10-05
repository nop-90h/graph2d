#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

//----------------StaticVector----------------

template <typename T, std::size_t Capacity>
class StaticVector {
public:
    using size_type = std::size_t;

private:
    std::array<T, Capacity> data_{}; // фиксированное хранилище
    size_type size_ = 0;             // текущая позиция вставки / количество элементов

public:
    // Обычный конструктор
    StaticVector() = default;

    // Копирующий конструктор
    StaticVector(const StaticVector& other) = default;

    // Копирующее присваивание, на всякий случай
    StaticVector& operator=(const StaticVector& other) = default;

    // Деструктор
    ~StaticVector() = default;

    // Добавление элемента в конец
    void push_back(const T& value) {
        if (size_ >= Capacity) {
            throw std::length_error("StaticVector::push_back: capacity exceeded");
        }

        data_[size_] = value;
        ++size_;
    }

    // Добавление через move
    void push_back(T&& value) {
        if (size_ >= Capacity) {
            throw std::length_error("StaticVector::push_back: capacity exceeded");
        }

        data_[size_] = std::move(value);
        ++size_;
    }

    void pop_back() noexcept
    {
        assert(size_ > 0);
        --size_;
    }

    // Сколько элементов логически добавлено
    size_type size() const noexcept {
        return size_;
    }

    // Вместимость всегда равна Capacity
    size_type capacity() const noexcept {
        return Capacity;
    }

    // Доступ без проверки, как у std::vector::operator[]
    T& operator[](size_type index) noexcept {
        assert(index < size_); // проверка только в debug
        return data_[index];
    }

    const T& operator[](size_type index) const noexcept {
        assert(index < size_); // проверка только в debug
        return data_[index];
    }

    // Доступ с проверкой, как у std::vector::at
    T& at(size_type index) {
        if (index >= size_) {
            throw std::out_of_range("StaticVector::at: index out of range");
        }

        return data_[index];
    }

    const T& at(size_type index) const {
        if (index >= size_) {
            throw std::out_of_range("StaticVector::at: index out of range");
        }

        return data_[index];
    }
};


//----------------FixedSmallMap----------------

template <typename Key, typename Value, std::size_t Capacity>
class FixedSmallMap {
private:
    using PairType = std::pair<Key, Value>;
    
    std::array<std::optional<PairType>, Capacity> storage_;
    std::size_t size_ = 0;

public:
    FixedSmallMap() = default;

    FixedSmallMap(const FixedSmallMap&) = default;
    FixedSmallMap& operator=(const FixedSmallMap&) = default;

    FixedSmallMap(FixedSmallMap&&) = delete;
    FixedSmallMap& operator=(FixedSmallMap&&) = delete;

    bool insert(const Key& key, const Value& value) {
        if (contains(key)) 
        {
            assert(false);
            return false; 
        }
        
        if (size_ >= Capacity) {
            throw std::out_of_range("FixedSmallMap capacity exceeded: maximum size reached");
        }

        storage_[size_].emplace(key, value);
        size_++;
        return true;
    }

    const Value* find(const Key& key) const {
        for (std::size_t i = 0; i < size_; ++i) {
            if (storage_[i]->first == key) {
                return &(storage_[i]->second);
            }
        }
        return nullptr;
    }

    Value* find(const Key& key) {
        for (std::size_t i = 0; i < size_; ++i) {
            if (storage_[i]->first == key) {
                return &(storage_[i]->second);
            }
        }
        return nullptr;
    }

    bool contains(const Key& key) const {
        std::span subview(storage_.data(), size_);
        return std::ranges::any_of(subview, [&key](const auto& opt_pair) {
            return opt_pair->first == key;
        });
    }

    // Erase element by key (swap with last element for O(1) removal)
    bool erase(const Key& key) {
        for (std::size_t i = 0; i < size_; ++i) {
            if (storage_[i]->first == key) {
                // Swap with the last element
                if (i < size_ - 1) {
                    storage_[i] = std::move(storage_[size_ - 1]);
                }
                // Clear the last element
                storage_[size_ - 1].reset();
                size_--;
                return true;
            }
        }
        return false;
    }

    std::size_t size() const { return size_; }
    constexpr std::size_t capacity() const { return Capacity; }
};
_G2D_NAMESPACE_END_