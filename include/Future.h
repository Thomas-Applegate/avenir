#pragma once
#include <atomic>
#include <memory>

namespace avenir
{

template <typename T>
class Promise;

template <typename T>
class Future
{
public:
private:
	struct State
	{
		std::atomic_flag valid_flag;
		std::atomic_flag ready_flag;
	};
};

template<>
class Future<void>
{

};

template <typename T>
class Promise
{

};



template <typename T>
Future<T> makeReadyFuture(T val)
{

}

}
