#pragma once
#include "NXRefCountable.h"
#include <functional>  // for std::hash

template <typename T>
class Ntr
{
public:
    // �չ��졣һ�㲻���õ���
    Ntr() : data(nullptr) {}

    // ����ָ��
    Ntr(T* ptr) : data(static_cast<IRefCountable*>(ptr))
    {
        if (data) data->IncRef();
    }

    Ntr(const Ntr<T>& other) : data(other.data)
    {
        if (data) data->IncRef();
    }

    // 2023.8.28 ����������ת�����졣
    // һ��Ҫȷ�� U �� T ��һ���̳����ϣ������� T is NXTexture, U is NXTexture2D��
    // ��˭����˭����������ν��// ����
    // std::vector<Ntr<NXTexture>> m_pTexArray;
    // m_pTexArray.push_back(new NXTexture2D());    template <typename U>
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

    const T* Ptr() const { return static_cast<const T*>(data); }

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
