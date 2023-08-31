#pragma once

// 2023.5.26
// 注：这个类从内容到注释几乎-完-全-是-chatgpt 创造的。特此感谢！:)
// This class from the implement to the comments was almost entirely created by ChatGPT. Special thanks! :)
template<class T>
class NXInstance
{
// 构造函数和析构函数声明为protected，以防止其他类创建或删除基类的实例
protected:
    NXInstance() {}
    virtual ~NXInstance() {} // 加入virtual，为了防止派生类在析构时出现问题

    // 删除了拷贝构造函数和拷贝赋值运算符，防止类的实例被复制或赋值
    NXInstance(const NXInstance&) = delete;
    NXInstance& operator=(const NXInstance&) = delete;

public:
    static T* GetInstance()
    {
        // 使用C++11的静态局部变量特性，保证在多线程环境下的线程安全
        static T pInstance;
        return &pInstance;
    }
};
