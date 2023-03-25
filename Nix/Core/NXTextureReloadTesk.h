#pragma once
#include "Header.h"

class NXTextureReloadTask
{
public:
	struct promise_type
	{
		NXTextureReloadTask get_return_object() 
		{
			return { std::coroutine_handle<NXTextureReloadTask::promise_type>::from_promise(*this) };
		}
		std::suspend_never initial_suspend() { return {}; }
		std::suspend_never final_suspend() noexcept 
		{
			m_callbackFunc();
			return {}; 
		}
		void return_void() {}
		void unhandled_exception() {}

		std::function<void()> m_callbackFunc;
	};

	std::coroutine_handle<NXTextureReloadTask::promise_type> m_handle;
};

class NXTextureAwaiter
{
public:
	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<NXTextureReloadTask::promise_type> handle) noexcept
	{
		std::thread([handle]() mutable { handle(); }).detach();
	}
	void await_resume() const noexcept {}
};
