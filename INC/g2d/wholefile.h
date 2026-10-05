#pragma once

#include "g2d.h"

_G2D_NAMESPACE_BEGIN_

class WholeFile {
public:
    explicit WholeFile(const char* lpccPath) noexcept 
        : m_buffer(nullptr), m_size(0) {
        
        if (!lpccPath) return;

        std::FILE* file = std::fopen(lpccPath, "rb");
        if (!file) return;

        std::setvbuf(file, nullptr, _IONBF, 0);

        std::fseek(file, 0, SEEK_END);
        long long fileSize = std::ftell(file);
        if (fileSize <= 0) {
            std::fclose(file);
            return;
        }
        std::rewind(file);

        void* buffer = std::malloc(fileSize);
        if (!buffer) {
            std::fclose(file);
            return;
        }

        size_t bytesRead = std::fread(buffer, 1, fileSize, file);
        std::fclose(file);

        if (bytesRead != static_cast<size_t>(fileSize)) {
            std::free(buffer);
            return;
        }

        m_buffer = buffer;
        m_size = bytesRead;
    }

    ~WholeFile() noexcept {
        std::free(m_buffer);
    }

    WholeFile(const WholeFile&) = delete;
    WholeFile& operator=(const WholeFile&) = delete;

    WholeFile(WholeFile&& other) noexcept 
        : m_buffer(other.m_buffer), m_size(other.m_size) {
        other.m_buffer = nullptr;
        other.m_size = 0;
    }

    WholeFile& operator=(WholeFile&& other) noexcept {
        if (this != &other) {
            std::free(m_buffer); 
            
            m_buffer = other.m_buffer;
            m_size = other.m_size;
            
            other.m_buffer = nullptr;
            other.m_size = 0;
        }
        return *this;
    }

    [[nodiscard]] void* data() const noexcept { return m_buffer; }
    [[nodiscard]] void* buffer() const noexcept { return m_buffer; }
    [[nodiscard]] size_t size() const noexcept { return m_size; }
    [[nodiscard]] bool isValid() const noexcept { return m_buffer != nullptr; }

    [[nodiscard]] void* release() noexcept {
        void* temp = m_buffer;
        m_buffer = nullptr;
        m_size = 0;
        return temp; 
    }

    void reset() noexcept {
        std::free(m_buffer);
        m_buffer = nullptr;
        m_size = 0;
    }

private:
    void* m_buffer;
    size_t m_size;
};

_G2D_NAMESPACE_END_