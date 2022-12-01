#include "ThreadPool.h"

using namespace avenir;

ThreadPool::ThreadPool(uint32_t numThreads, std::list<std::packaged_task<void()>>& queue)
{
	m_jobQueue.splice(m_jobQueue.end(), queue);
	addThreads(numThreads);
}
	
ThreadPool::ThreadPool(uint32_t numThreads)
{
	addThreads(numThreads);
}

ThreadPool::~ThreadPool()
{
	removeThreads(getThreadCount());
}

void ThreadPool::addThreads(uint32_t numThreads)
{
	for(uint32_t i = 0; i < numThreads; i++)
	{
		m_pool.emplace([this](std::stop_token stoken){
			while (true) {
				std::unique_lock<std::mutex> lock(m_queueMutex);
				m_cv.wait(lock, stoken, [this] {return !m_jobQueue.empty();});
				if(stoken.stop_requested()) { return; }
				
				auto job = std::move(m_jobQueue.front());
				m_jobQueue.pop_front();
				
				lock.unlock();
				
				job();
				
				if(m_jobQueue.empty())
				{
					m_waitFlag.clear();
					m_waitFlag.notify_all();
				}
				
				if(stoken.stop_requested()) { return; }
			}
		});
	}
}

void ThreadPool::removeThreads(uint32_t numThreads)
{
	uint32_t limit = numThreads > getThreadCount() ? getThreadCount() : numThreads;
	for(uint32_t i = 0; i < limit; i++)
	{
		m_pool.top().request_stop();
		m_cv.notify_all();
		m_pool.pop();
	}
}

std::list<std::packaged_task<void()>> ThreadPool::moveTasks()
{
	std::unique_lock<std::mutex> lock(m_queueMutex);
	std::list<std::packaged_task<void()>> queueTmp;
	queueTmp.splice(queueTmp.end(), m_jobQueue);
	return queueTmp;
}

void ThreadPool::pushTasks(std::list<std::packaged_task<void()>>& tasks)
{
	std::unique_lock<std::mutex> lock(m_queueMutex);
	m_jobQueue.splice(m_jobQueue.end(), tasks);
}

void ThreadPool::waitTilEmpty()
{
	m_waitFlag.test_and_set();
	m_waitFlag.wait(true);
}

uint32_t ThreadPool::getThreadCount() const { return m_pool.size(); }

uint32_t ThreadPool::jobsRemaining() const { return m_jobQueue.size(); }
