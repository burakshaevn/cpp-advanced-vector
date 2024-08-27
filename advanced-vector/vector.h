#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <stdexcept>
#include <memory>
#include <algorithm>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return data_.GetAddress();
    }
    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }
    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }
    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }
    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }
    const_iterator cend() const noexcept {
        return data_.GetAddress() + size_;
    }

    //Конструктор по умолчанию. 
    //Инициализирует вектор нулевого размера и вместимости. Не выбрасывает исключений. 
    //Алгоритмическая сложность: O(1).
    Vector() = default;

    //Конструктор, который создаёт вектор заданного размера. 
    //Вместимость созданного вектора равна его размеру, а элементы проинициализированы значением по умолчанию для типа T. 
    //Алгоритмическая сложность: O(размер вектора).
    Vector(const size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    //Копирующий конструктор. 
    //Создаёт копию элементов исходного вектора. 
    //Имеет вместимость, равную размеру исходного вектора, то есть выделяет память без запаса. 
    //Алгоритмическая сложность: O(размер исходного вектора).
    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept {
        Swap(other);
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else {
                if (rhs.size_ < size_) {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + rhs.size_, data_.GetAddress());
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                }
                else {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + size_, data_.GetAddress());
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }
    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    //Деструктор. Разрушает содержащиеся в векторе элементы и освобождает занимаемую ими память. 
    // Алгоритмическая сложность: O(размер вектора).
    ~Vector() {
        for (size_t j = 0; j < size_; ++j) {
            (data_.GetAddress() + j)->~T();
        }
    }

    //Резервирует достаточно места, чтобы вместить количество элементов, равное capacity. 
    //Если новая вместимость не превышает текущую, метод не делает ничего. 
    //Алгоритмическая сложность: O(размер вектора). 
    void Reserve(size_t capacity) {
        if (capacity > data_.Capacity()) {
            RawMemory<T> new_data(capacity);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    void Resize(size_t new_size) {
        if (new_size > size_) {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        }
        else {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        }
        size_ = new_size;
    }
    template <typename Type>
    void PushBack(Type&& value) {
        if (size_ == data_.Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + size_) T(std::forward<Type>(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        else {
            new (data_ + size_) T(std::forward<Type>(value));
        }
        ++size_;
    }
    void PopBack() noexcept {
        assert(size_ != 0);
        std::destroy_at(data_.GetAddress() + size_ - 1);
        --size_;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == data_.Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        else {
            new (data_ + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return data_[size_ - 1];
    }
    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        // Вычисляем индекс позиции вставки
        ptrdiff_t index = pos - begin();

        // Проверяем, нуждается ли вектор в перераспределении
        if (size_ < data_.Capacity()) {
            if (pos == end()) {
                new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
            }
            else {
                // Создаем временный объект, если вставляемый элемент уже существует в векторе
                T temp(std::forward<Args>(args)...);

                // Создаем новый элемент на месте последнего элемента
                //new (data_.GetAddress() + size_) T(std::move(*(end() - 1)));
                new (data_.GetAddress() + size_) T(std::forward<T>(*(end() - 1)));

                // Перемещаем элементы назад
                std::move_backward(begin() + index, end() - 1, end());
                //std::move_backward(begin() + index, end(), end() + 1);

                // Вставляем временный объект в нужную позицию
                data_[index] = std::move(temp);
                //new (elem_pos) T(std::move(temp));
            }
        }
        else {
            // Если требуется перераспределение памяти
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            T* new_elem_pos = new_data.GetAddress() + index;

            // Перемещаем элементы перед позицией вставки
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), index, new_data.GetAddress());
            }
            else {
                try {
                    std::uninitialized_copy_n(data_.GetAddress(), index, new_data.GetAddress());
                }
                catch (...) {
                    std::destroy_n(new_data.GetAddress(), index);
                    throw;
                }
            }

            // Создаём новый элемент на нужной позиции
            new (new_elem_pos) T(std::forward<Args>(args)...);

            // Перемещаем элементы после позиции вставки
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress() + index, size_ - index, new_elem_pos + 1);
            }
            else {
                try {
                    std::uninitialized_copy_n(data_.GetAddress() + index, size_ - index, new_elem_pos + 1);
                }
                catch (...) {
                    std::destroy_n(new_elem_pos, 1);
                    throw;
                }
            }

            // Удаляем старые элементы
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }

        ++size_;
        return begin() + index;
    }

    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/ {
        T* non_const_pos = const_cast<T*>(pos);  // Преобразуем const_iterator в iterator
        std::move(non_const_pos + 1, end(), non_const_pos);  // Перемещаем элементы на одну позицию влево
        (data_ + size_ - 1)->~T();  // Разрушаем последний элемент
        --size_;
        return non_const_pos;
    }

    /*Вектор имеет достаточную вместимость для вставки ещё одного элемента.
    Вектор полностью заполнен. В результате вставка элемента приведёт к реаллокации памяти.*/
    //template <typename Type>
    //iterator Insert(const_iterator pos, Type&& value) {
    //    return Emplace(pos, std::forward<Type>(value));
    //}

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};