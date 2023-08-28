#pragma once
#include "NXRefCountable.h"

template <typename T>
class Ntr
{
public:
    Ntr() : data(nullptr) {}

    Ntr(T* ptr) : data(static_cast<IRefCountable*>(ptr))
    {
        if (data) data->IncRef();
    }

    Ntr(const Ntr<T>& other) : data(other.data)
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

    T& operator*() { return *static_cast<T*>(data); }
    T* operator->() { return static_cast<T*>(data); }

    const T* Ptr() { return static_cast<const T*>(data); }

private:
    IRefCountable* data;
};
