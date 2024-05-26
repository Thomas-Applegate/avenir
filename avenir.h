/*

Copyright <2024> Thomas Applegate

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the 
“Software”), to deal in the Software without restriction, including 
without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so, subject to 
the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <mutex>
#include <condition_variable>
#include <deque>
#include <optional>
#include <utility>
#include <chrono>

namespace avenir
{

//thread safe multi-producer multi-consumer queue
template <typename T>
class queue
{
public:
	queue() = default;
	
	queue(const queue& oth) : m_mtx(), m_cv(), m_deque()
	{
		std::scoped_lock lock(m_mtx, oth.m_mtx);
		for(const T& o : oth.m_deque)
		{
			m_deque.push_back(o);
		}
	}
	
	//the move constructor leaves the other queue in an empty but valid state
	queue(queue&& oth) : m_mtx(), m_cv(), m_deque()
	{
		std::scoped_lock lock(m_mtx, oth.m_mtx);
		while(!oth.m_deque.empty())
		{
			m_deque.emplace_back(std::move(oth.m_deque.front()));
			oth.m_deque.pop_front();
		}
	}
	
	queue& operator=(const queue& oth)
	{
		if(this != &oth)
		{
			std::scoped_lock lock(m_mtx, oth.m_mtx);
			m_deque.clear();
			for(const T& o : oth.m_deque)
			{
				m_deque.push_back(o);
			}
		}
		return *this;
	}
	
	//move assignment leaves the other queue in an empty but valid state
	queue& operator=(queue&& oth)
	{
		if(this != &oth)
		{
			std::scoped_lock lock(m_mtx, oth.m_mtx);
			m_deque.clear();
			while(!oth.m_deque.empty())
			{
				m_deque.emplace_back(std::move(oth.m_deque.front()));
				oth.m_deque.pop_front();
			}
		}
		return *this;
	}
	
	void push(const T& obj)
	{
		std::unique_lock<std::mutex> lock(m_mtx);
		m_deque.push_back(obj);
		lock.unlock();
		m_cv.notify_one();
	}
	
	template <typename... Args>
	void emplace(Args&&... args)
	{
		std::unique_lock<std::mutex> lock(m_mtx);
		m_deque.emplace(std::forward<Args>(args)...);
		lock.unlock();
		m_cv.notify_one();
	}
	
	//immediately pop from the queue or return nullopt if empty
	std::optional<T> pop()
	{
		std::optional<T> ret;
		std::scoped_lock lock(m_mtx);
		if(!m_deque.empty())
		{
			ret = std::move(m_deque.front());
			m_deque.pop_front();
		}
		return ret;
	}
	
	//immediately pop from the queue if possible
	//or wait until notified and pop
	T wait()
	{
		std::unique_lock<std::mutex> lock(m_mtx);
		if(m_deque.empty())
		{
			m_cv.wait(lock, [this]{ return !m_deque.empty(); });
		}
		T ret = std::move(m_deque.front());
		m_deque.pop_front();
		return ret;
	}
	
	//immediately pop from the queue if possible
	//or wait for duration or until notified and pop
	//returns nullopt if nothing is available
	template <typename Rep, typename Period>
	std::optional<T> wait_for(const std::chrono::duration<Rep, Period>& rel_time)
	{
		std::optional<T> ret;
		std::unique_lock<std::mutex> lock(m_mtx);
		if(m_deque.empty())
		{
			if(m_cv.wait_for(lock, rel_time, [this]{
				return !m_deque.empty();
			}))
			{
				ret = std::move(m_deque.front());
				m_deque.pop_front();	
			}
		}else
		{
			ret = std::move(m_deque.front());
			m_deque.pop_front();	
		}
		return ret;
	}
	
	//immediately pop from the queue if possible
	//or wait until time point or until notified and pop
	//returns nullopt if nothing is available
	template <typename Clock, typename Duration>
	std::optional<T> wait_until(const std::chrono::time_point<Clock, Duration>& abs_time)
	{
		std::optional<T> ret;
		std::unique_lock<std::mutex> lock(m_mtx);
		if(m_deque.empty())
		{
			if(m_cv.wait_for(lock, abs_time, [this]{
				return !m_deque.empty(); 
			}))
			{
				ret = std::move(m_deque.front());
				m_deque.pop_front();	
			}
		}else
		{
			ret = std::move(m_deque.front());
			m_deque.pop_front();	
		}
		return ret;
	}
	
	//copy the contents of another queue<T> into this
	void copy_from(const queue& oth)
	{
		if(this != &oth)
		{
			std::scoped_lock lock(m_mtx, oth.m_mtx);
			for(const T& o : oth.m_deque)
			{
				m_deque.push_back(o);
			}
		}
	}
	
	//move the contents of another queue<T> into this
	void splice(queue& oth)
	{
		if(this != &oth)
		{
			std::scoped_lock lock(m_mtx, oth.m_mtx);
			while(!oth.m_deque.empty())
			{
				m_deque.emplace_back(std::move(oth.m_deque.front()));
				oth.m_deque.pop_front();
			}
		}
	}
	
	bool empty() const
	{
		return m_deque.empty();
	}
	
	size_t size() const
	{
		return m_deque.size();
	}
private:
	std::mutex m_mtx;
	std::condition_variable m_cv;
	std::deque<T> m_deque;
};

}
