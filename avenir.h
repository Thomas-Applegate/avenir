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
#include <memory>

namespace avenir
{

//thread safe multi-producer multi-consumer queue
template<typename T>
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
	
	template<typename... Args>
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
	mutable std::mutex m_mtx;
	std::condition_variable m_cv;
	std::deque<T> m_deque;
};

template<typename T>
class promise;

template<typename T>
class future;

template<>
class promise<void>
{
private:
	template<typename U>
	friend class promise;
	
	template<typename U>
	friend class future;
	
	struct state
	{
		std::mutex mtx;
		std::condition_variable cv;
		short status; //0 unset, 1 ready, 2 broken
		state(short status) : mtx(), cv(), status(status) {}
		virtual ~state() {}
	};
	
	std::shared_ptr<state> m_state;
public:
	promise(bool ready=false)
	: m_state(std::make_shared<state>(ready ? 1 : 0)) {}
	promise(const promise& oth) = delete;
	promise(promise&& oth) : m_state(std::move(oth.m_state)) {}
	
	~promise()
	{
		if(m_state)
		{
			std::unique_lock<std::mutex> lock(m_state->mtx);
			if(m_state->status == 0)
			{
				m_state->status = 2;
				lock.unlock();
				m_state->cv.notify_all();
			}
		}
	}
	
	promise& operator=(promise&& oth)
	{
		if(this != &oth)
		{
			if(m_state)
			{
				std::unique_lock<std::mutex> lock(m_state->mtx);
				if(m_state->status == 0)
				{
					m_state->status = 2;
					lock.unlock();
					m_state->cv.notify_all();
				}
			}
			m_state = std::move(oth.m_state);
		}
		return *this;
	}
	
	void set()
	{
		//TODO
	}
	
	void set_at_thread_exit()
	{
		//TODO
	}
};

template<typename T>
class promise
{
	template<typename U>
	friend class future;
	
	struct state : public promise<void>::state
	{
		char storage[sizeof(T)];
		state() : promise<void>::state(0) {}
		state(const T& o) : state(1)
		{
			new (storage) T(o);
		}
		state(T&& o) : state(1)
		{
			new (storage) T(std::move(o));
		}
		~state()
		{
			if(status == 1)
			{
				reinterpret_cast<T*>(storage)->~T();
			}
		}
	};
	
	std::shared_ptr<state> m_state;
public:
	promise() : m_state(std::make_shared<state>()) {}
	promise(const T& obj) : m_state(std::make_shared<state>(obj)) {}
	promise(T&& obj) : m_state(std::make_shared<state>(std::move(obj))) {}
	promise(const promise& oth) = delete;
	promise(promise&& oth) : m_state(std::move(oth.m_state)) {}
	
	~promise()
	{
		if(m_state)
		{
			std::unique_lock<std::mutex> lock(m_state->mtx);
			if(m_state->status == 0)
			{
				m_state->status = 2;
				lock.unlock();
				m_state->cv.notify_all();
			}
		}
	}
	
	promise& operator=(promise&& oth)
	{
		if(this != &oth)
		{
			if(m_state)
			{
				std::unique_lock<std::mutex> lock(m_state->mtx);
				if(m_state->status == 0)
				{
					m_state->status = 2;
					lock.unlock();
					m_state->cv.notify_all();
				}
			}
			m_state = std::move(oth.m_state);
		}
		return *this;
	}
};

template<>
class future<void>
{
	
};

template<typename T>
class future
{
	
};

template<typename T>
std::optional<future<T>> future_cast(const future<void>& f)
{
	return std::nullopt; //TODO
}

}
