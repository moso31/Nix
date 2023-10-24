#pragma once
#include "NXRefCountable.h"

template <typename T>
class Ntr
{
    template<class U> friend class Ntr;
public:
    Ntr() : data(nullptr) {}

    Ntr(T* ptr) : data(ptr)
    {
        if (data) data->IncRef();
    }

    Ntr(const Ntr<T>& other) : data(other.data)
    {
        if (data) data->IncRef();
    }

    template <typename U>
    Ntr(const Ntr<U>& other) : data(other.data)
    {
        if (data) data->IncRef();
    }

    ~Ntr()
    {
        if (data) data->DecRef();
    }

    Ntr<T>& operator=(const Ntr<T>& other)
    {
        if (data != other.data)
        {
            if (data) data->DecRef();
            data = other.data;
            if (data) data->IncRef();
        }
        return *this;
    }

    bool operator==(const Ntr<T>& other) const 
    {
        return data == other.data; 
    }

    T& operator*() { return *static_cast<T*>(data); }
    T* operator->() { return static_cast<T*>(data); }

    T* Ptr() { return static_cast<T*>(data); }
    T** PPtr() { return &static_cast<T*>(data); }

    bool IsValid() const { return data != nullptr; }
    bool IsNull() const { return data == nullptr; }

    template <typename U>
    Ntr<U> As() const 
    {
        if (!data) return nullptr;
        return Ntr<U>(static_cast<U*>(data));
    }

private:
    IRefCountable* data;
};
