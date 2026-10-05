#pragma once 

#include "g2d.h"
#include<ostream>

_G2D_NAMESPACE_BEGIN_

namespace bi = boost::intrusive;

// Статический пул-аллокатор для boost::intrusive::list
template<typename T, size_t PoolSize = 10000>
class StaticPoolAllocator {
private:
    struct Chunk {
        Chunk* next;
        Chunk* prev;  // Добавляем указатель на предыдущий для двунаправленного списка
    };
    
    // Элемент пула должен содержать правильный хук
    struct PoolItem : public bi::list_base_hook<bi::link_mode<bi::normal_link>> {
        T data;
        
        template<typename... Args>
        PoolItem(Args&&... args) : data(std::forward<Args>(args)...) {}
    };
    
    static constexpr size_t chunk_size_ = (sizeof(PoolItem) + alignof(std::max_align_t) - 1) & ~(alignof(std::max_align_t) - 1);
    
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    
    template<typename U>
    struct rebind {
        using other = StaticPoolAllocator<U, PoolSize>;
    };
    
    // Статические члены с inline определениями (C++17+)
    inline static Chunk* free_list_head_ = nullptr;
    inline static Chunk* free_list_tail_ = nullptr;  // Хвост списка для эффективного добавления
    inline static char* memory_pool_ = nullptr;
    inline static size_t allocated_count_ = 0;
    inline static bool initialized_ = false;
    
    StaticPoolAllocator() {
        initialize_pool();
    }
    
    ~StaticPoolAllocator() = default;
    
    // Конструктор копирования
    StaticPoolAllocator(const StaticPoolAllocator&) {
        initialize_pool();
    }
    
    template<typename U>
    StaticPoolAllocator(const StaticPoolAllocator<U, PoolSize>&) {
        initialize_pool();
    }
    
    StaticPoolAllocator(StaticPoolAllocator&&) noexcept = default;
    StaticPoolAllocator& operator=(const StaticPoolAllocator&) = delete;
    StaticPoolAllocator& operator=(StaticPoolAllocator&&) = delete;
    
    // Выделение памяти для одного объекта
    [[nodiscard]] T* allocate(size_t n = 1) {
        if (n != 1 || !free_list_head_) {
            throw std::bad_alloc();
        }
        
        // Берем из головы списка (LIFO - лучше для кэша)
        Chunk* chunk = free_list_head_;
        free_list_head_ = free_list_head_->next;
        
        // Обновляем голову если нужно
        if (free_list_head_) {
            free_list_head_->prev = nullptr;
        } else {
            free_list_tail_ = nullptr; // Список пуст
        }
        
        allocated_count_++;
        return &(reinterpret_cast<PoolItem*>(chunk)->data);
    }
    
    // Освобождение памяти
    void deallocate(T* ptr, size_t n = 1) noexcept {
        if (!ptr || n != 1) return;
        
        PoolItem* item = reinterpret_cast<PoolItem*>(
            reinterpret_cast<char*>(ptr) - offsetof(PoolItem, data));
        
        Chunk* chunk = reinterpret_cast<Chunk*>(item);
        
        // Добавляем в голову списка (LIFO)
        chunk->next = free_list_head_;
        chunk->prev = nullptr;
        
        if (free_list_head_) {
            free_list_head_->prev = chunk;
        } else {
            free_list_tail_ = chunk; // Если список был пуст
        }
        
        free_list_head_ = chunk;
        allocated_count_--;
    }
    
    // Конструктор на выделенной памяти
    template<typename U, typename... Args>
    void construct(U* ptr, Args&&... args) {
        std::construct_at(ptr, std::forward<Args>(args)...);
    }
    
    // Деструктор
    template<typename U>
    void destroy(U* ptr) {
        std::destroy_at(ptr);
    }
    
    size_t max_size() const noexcept { return PoolSize; }
    
    // Статические методы для управления пулом
    static size_t get_allocated_count() noexcept { return allocated_count_; }
    static constexpr size_t get_pool_size() noexcept { return PoolSize; }
    static size_t get_available_count() noexcept { return PoolSize - allocated_count_; }
    
    static void print_stats() {
        std::cout << "Pool stats for " << typeid(T).name() << ": "
                  << allocated_count_ << "/" << PoolSize << " allocated ("
                  << get_available_count() << " available)" << std::endl;
    }
    
    // Проверка целостности пула (для отладки)
    static bool validate_pool() {
        if (!initialized_) return false;
        
        size_t free_count = 0;
        Chunk* current = free_list_head_;
        Chunk* prev = nullptr;
        
        while (current) {
            if (current->prev != prev) {
                return false; // Нарушена связность
            }
            free_count++;
            prev = current;
            current = current->next;
        }
        
        // Проверяем что хвост корректен
        if (free_list_head_ && !free_list_tail_) return false;
        if (!free_list_head_ && free_list_tail_) return false;
        
        // Проверяем счетчики
        return (free_count + allocated_count_) == PoolSize;
    }
    
    // Очистка всего пула
    static void cleanup() noexcept {
        if (memory_pool_) {
            std::free(memory_pool_);
            memory_pool_ = nullptr;
            free_list_head_ = nullptr;
            free_list_tail_ = nullptr;
            allocated_count_ = 0;
            initialized_ = false;
        }
    }
    
    // Для совместимости с STL
    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using is_always_equal = std::true_type;
    
private:
    static void initialize_pool() {
        if (initialized_) return;
        
        memory_pool_ = static_cast<char*>(std::malloc(PoolSize * chunk_size_));
        if (!memory_pool_) {
            throw std::bad_alloc();
        }
        
        // Инициализируем свободный список как двунаправленный
        free_list_head_ = reinterpret_cast<Chunk*>(memory_pool_);
        Chunk* current = free_list_head_;
        Chunk* prev = nullptr;
        
        for (size_t i = 0; i < PoolSize; ++i) {
            current->prev = prev;
            
            if (i < PoolSize - 1) {
                Chunk* next_chunk = reinterpret_cast<Chunk*>(
                    reinterpret_cast<char*>(current) + chunk_size_);
                current->next = next_chunk;
                prev = current;
                current = next_chunk;
            } else {
                current->next = nullptr;
                free_list_tail_ = current; // Сохраняем хвост
            }
        }
        
        initialized_ = true;
    }
};

// Базовый класс для объектов, использующих статический пул
template<typename T, size_t PoolSize = 10000>
class StaticPoolObject : public bi::list_base_hook<bi::link_mode<bi::normal_link>> {
public:
    T data;
    
    template<typename... Args>
    StaticPoolObject(Args&&... args) : data(std::forward<Args>(args)...) {}
    
    using allocator_type = StaticPoolAllocator<StaticPoolObject<T, PoolSize>, PoolSize>;
};

// Упрощенный список со статическим пулом
template<typename T, size_t PoolSize = 10000>
class StaticPoolList {
private:
    using ObjectType = StaticPoolObject<T, PoolSize>;
    using Allocator = StaticPoolAllocator<ObjectType, PoolSize>;
    using List = bi::list<ObjectType, bi::base_hook<bi::list_base_hook<bi::link_mode<bi::normal_link>>>>;
    
    List list_;
    [[no_unique_address]] Allocator allocator_;
    
public:
    using iterator = typename List::iterator;
    using const_iterator = typename List::const_iterator;
    
    StaticPoolList() = default;
    
    ~StaticPoolList() {
        clear();
    }
    
    // Добавление элемента
    template<typename... Args>
    T& emplace_back(Args&&... args) {
        ObjectType* item = allocator_.allocate(1);
        try {
            allocator_.construct(item, std::forward<Args>(args)...);
            list_.push_back(*item);
            return item->data;
        } catch (...) {
            allocator_.deallocate(item, 1);
            throw;
        }
    }
    
    // Удаление элемента по итератору с возвратом следующего итератора
    iterator erase(iterator it) noexcept {
        if (it == list_.end()) return list_.end();
        
        ObjectType* item = &(*it);
        // Получаем следующий итератор перед удалением
        iterator next = it;
        ++next;
        
        list_.erase(it);
        allocator_.destroy(item);
        allocator_.deallocate(item, 1);
        
        return next;
    }
    
    // Удаление элемента по константному итератору
    iterator erase(const_iterator it) noexcept {
        return erase(iterator(it));
    }
    
    // Очистка списка
    void clear() noexcept {
        while (!list_.empty()) {
            erase(list_.begin());
        }
    }
    
    // Итераторы
    iterator begin() noexcept { return list_.begin(); }
    iterator end() noexcept { return list_.end(); }
    const_iterator begin() const noexcept { return list_.begin(); }
    const_iterator end() const noexcept { return list_.end(); }
    const_iterator cbegin() const noexcept { return list_.cbegin(); }
    const_iterator cend() const noexcept { return list_.cend(); }
    
    // Размер
    size_t size() const noexcept { return list_.size(); }
    bool empty() const noexcept { return list_.empty(); }
    
    // Доступ к элементам
    T& front() noexcept { return list_.front().data; }
    const T& front() const noexcept { return list_.front().data; }
    T& back() noexcept { return list_.back().data; }
    const T& back() const noexcept { return list_.back().data; }
    auto back_itter() {   auto last_it = --list_.end();return last_it; }
    // Статистика пула
    static size_t get_allocated_count() noexcept { return Allocator::get_allocated_count(); }
    static constexpr size_t get_pool_size() noexcept { return Allocator::get_pool_size(); }
    static size_t get_available_count() noexcept { return Allocator::get_available_count(); }
    static void print_stats() { Allocator::print_stats(); }
    static bool validate_pool() { return Allocator::validate_pool(); }
    static void cleanup() { Allocator::cleanup(); }
};

_G2D_NAMESPACE_END_