#pragma once
#include "NXRefCountable.h"
#include <functional>  // for std::hash

template <typename T>
class Ntr
{
public:
    // 空构造。一般不会用到。
    Ntr() : data(nullptr) {}

    // 从裸指针
    Ntr(T* ptr) : data(ptr)
    {
        if (data) data->IncRef();
    }

    Ntr(const Ntr<T>& other) : data(other.data)
    {
        if (data) data->IncRef();
    }

    // 2023.8.28 从其它类型转换构造。
    // 一定要确保 U 是 T 在一条继承链上，（比如 T is NXTexture, U is NXTexture2D）
    // 但谁基类谁派生类无所谓。// 例：
    // std::vector<Ntr<NXTexture>> m_pTexArray;
    // m_pTexArray.push_back(Ntr<NXTexture2D>());
    template <typename U>
    Ntr(const Ntr<U>& other) : data(other.Ptr())
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

    T* Ptr() const { return static_cast<T*>(data); }

    template <typename U>
    Ntr<U> As() const 
    {
        if (!data) nullptr;
        return Ntr<U>(static_cast<U*>(data));
    }

private:
    IRefCountable* data;
};

//namespace std
//{
//    template <typename T>
//    struct std::hash<Ntr<T>>
//    {
//        std::size_t operator()(const Ntr<T>& k) const
//        {
//            return std::hash<IRefCountable*>()(k.Ptr());
//        }
//    };
//}
