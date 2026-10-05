#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

template <std::size_t N>
struct StaticStr {
    std::array<char, N> data;

    // Constructor from string literal
    constexpr StaticStr(const char (&s)[N]) {
        for (std::size_t i = 0; i < N; ++i) {
            data[i] = s[i];
        }
    }

    // Conversion to std::string_view
    constexpr operator std::string_view() const {
        return std::string_view(data.data(), N - 1); // Exclude null terminator
    }

    // operator== for comparison as required for NTTP
    constexpr bool operator==(const StaticStr<N>& other) const {
        for (std::size_t i = 0; i < N; ++i) {
            if (data[i] != other.data[i]) {
                return false;
            }
        }
        return true;
    }
};

// Deduction guide for easier usage
template <std::size_t N>
StaticStr(const char (&)[N]) -> StaticStr<N>;

_G2D_NAMESPACE_END_