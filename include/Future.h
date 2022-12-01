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
	Future(const Future<T>& oth)
		: m_statePtr(oth.m_statePtr), m_dataPtr(oth.m_dataPtr) {}
	
	Future& operator=(const Future<T>& oth)
	{
		m_statePtr = oth.m_statePtr;
		m_dataPtr = oth.m_dataPtr;
		return *this;
	}
	
	Future(Future<T>&& oth)
		: m_statePtr(std::move(oth.m_statePtr)),
		m_dataPtr(std::move(oth.m_dataPtr)) {}
	
	Future& operator=(Future<T>&& oth)
	{
		m_statePtr = std::move(oth.m_statePtr);
		m_dataPtr = std::move(oth.m_dataPtr);
		return *this;
	}

	bool isValid() const { return m_statePtr->valid_flag.test(); }
	
	bool isReady() const { return m_statePtr->ready_flag.test(); }
	
	void wait() { m_statePtr->ready_flag.wait(false); }
	
	T& get()
	{
		m_statePtr->ready_flag.wait(false);
		return *m_dataPtr;
	}
private:
	friend class Promise<T>;
	
	struct State
	{
		std::atomic_flag valid_flag;
		std::atomic_flag ready_flag;
	};
	
	std::shared_ptr<State> m_statePtr;
	std::shared_ptr<T> m_dataPtr;
	
	//private constructor since only a promise can create a future
	Future(std::shared_ptr<State>& statePtr, std::shared_ptr<T>& dataPtr)
		: m_statePtr(statePtr), m_dataPtr(dataPtr) {}
};

template<>
class Future<void>
{
public:
	template<typename U>
	Future(const Future<U>& oth) : m_statePtr(oth.m_statePtr) {}
	
	template<typename U>
	Future& operator=(const Future<U>& oth)
	{
		m_statePtr = oth.m_statePtr;
		return *this;
	}
	
	template<typename U>
	Future(Future<U>&& oth) : m_statePtr(std::move(oth.m_statePtr)) {}
	
	template <typename U>
	Future& operator=(Future<U>&& oth)
	{
		m_statePtr = std::move(oth.m_statePtr);
		return *this;
	}
	
	bool isValid() const { return m_statePtr->valid_flag.test(); }
	
	bool isReady() const { return m_statePtr->ready_flag.test(); }
	
	void wait() { m_statePtr->ready_flag.wait(false); }
	
	void get()
	{
		m_statePtr->ready_flag.wait(false);
	}
private:
	friend class Promise<void>;
	
	struct State
	{
		std::atomic_flag valid_flag;
		std::atomic_flag ready_flag;
	};
	
	std::shared_ptr<State> m_statePtr;
	
	//private constructor since only a promise can create a future
	Future(std::shared_ptr<State>& statePtr);
};

template <typename T>
class Promise
{
public:
private:
};

template<>
class Promise<void>
{
public:
private:
};

template <typename T>
Future<T> makeReadyFuture(T val)
{

}

}
