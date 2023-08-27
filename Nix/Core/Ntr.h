#pragma once
#include <atomic>

template <typename T>
class Ntr
{
public:
	Ntr() :
		data(nullptr),
		ref(new std::atomic_int(0))
	{
	}

	Ntr(T* ptr) :
		data(ptr),
		ref(new std::atomic_int(1))
	{
		// ����޷������ڴ棬ɾ�����ݲ��׳��쳣
		if (!ref)
		{
			delete data;
			throw std::bad_alloc();
		}
	}

	Ntr(const Ntr<T>& other) :
		data(other.data),
		ref(other.ref)
	{
		(*ref)++;
	}

	~Ntr()
	{
		(*ref)--;
		if (*ref == 0)
		{
			delete data;
			delete ref;
		}
	}

	Ntr<T>& operator=(const Ntr<T>& other)
	{
		if (this == &other)
		{
			return *this;
		}

		(*ref)--;
		if (*ref == 0)
		{
			delete data;
			delete ref;
		}

		data = other.data;
		ref = other.ref;
		(*ref)++;

		return *this;
	}

	//Ntr(Ntr<T>&& other)
	//{
	//    data = other.data;
	//    ref = other.ref;
	//    other.data = nullptr;
	//    other.ref = nullptr;
	//}

	//Ntr<T>& operator=(Ntr<T>&& other)
	//{
	//    if (this != &other)
	//    {
	//        (*ref)--;
	//        if (*ref == 0)
	//        {
	//            delete data;
	//            delete ref;
	//        }

	//        data = other.data;
	//        ref = other.ref;

	//        other.data = nullptr;
	//        other.ref = nullptr;
	//    }
	//    return *this;
	//}

	T& operator*() { return *data; }
	T* operator->() { return data; }

	const T* Ptr() { return data; }

private:
	T* data;

	// ʹ�� int* ���� int��
	// ȷ����� Ntr<> ָ��ͬһ����ַʱ��������ͬ�����ü���
	std::atomic_int* ref;
};
