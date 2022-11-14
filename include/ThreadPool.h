#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>
#include <list>
#include <functional>
#include <future>
#include <stack>
#include <concepts>

namespace avenir
{
class ThreadPool
{
public:
//constructors and assignment operators
	ThreadPool(uint32_t numThreads);
	//construct by moving tasks from a list
	ThreadPool(uint32_t numThreads, std::list<std::packaged_task<void()>>&& queue);
	ThreadPool(const ThreadPool& other) = delete; //no copy constructor
	ThreadPool& operator= (const ThreadPool& other) = delete; //no copy assignment
	ThreadPool(ThreadPool&& other) = delete; //no move constructor
	ThreadPool& operator= (ThreadPool&& other) = delete; //no move assignment
	~ThreadPool();
//other functions
	template <std::invocable<void> Func>
	auto pushJob(const Func& f)
	{
		typedef decltype(f()) RetType;
		
		std::packaged_task<RetType()> task(std::move(f));
		std::future<RetType> future = task.get_future();
		
		std::unique_lock<std::mutex> lock(m_queueMutex);
		
		m_jobQueue.emplace_back(std::move(task));
		
		lock.unlock();
		
		m_cv.notify_one();
		
		return future;
	}
	
	template <typename Func, typename... Args>
		requires std::invocable<Func, Args...>
	auto pushJob(const Func& f, Args&&... args) //overload if the user wants to push a job that takes arguments
	{
		return pushJob(std::bind(f, args...));
	}
	
	void addThreads(uint32_t numThreads);
	
	//removes threads from the threadpool, they will be stopped and
	//this function will block untill all threads removed have joined
	void removeThreads(uint32_t numThreads);
	
	//move all unstarted tasks into a new queue and return it
	std::list<std::packaged_task<void()>> moveTasks();
	
	uint32_t getThreadCount() const;
	uint32_t jobsRemaining() const;
private:
	std::stack<std::jthread> m_pool;
	std::list<std::packaged_task<void()>> m_jobQueue;
	std::condition_variable_any m_cv;
	std::mutex m_queueMutex;
};
}
