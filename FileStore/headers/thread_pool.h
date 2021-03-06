/*****************************************************************/
/**
 * \file   thread_pool.h
 * \brief  thread pool
 *//***************************************************************/
#pragma once
#include <assistant_utility.h>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <vector>
using TaskType = std::function<void()>;
using std::queue;
using std::thread;
using std::vector;

namespace GD {
/**
 * simply thread pool.
 */
class ThreadPool {
public:
	explicit ThreadPool(size_t n)
		: shutdown_(false)
	{
		active(n);
	}

	ThreadPool(const ThreadPool&) = delete;
	~ThreadPool() { shutdown(); };

	void active(size_t n)
	{
		shutdown();
		unique_lock lg(mutex_);
		shutdown_ = false;
		while (n) {
			threads_.emplace_back(worker(*this));
			n--;
		}
	}

	void enqueue(std::function<void()> fn)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		jobs_.push_back(std::move(fn));
		cond_.notify_one();
	}

	void shutdown()
	{
		// shut down once at a circle
		if (shutdown_)
			return;
		// Stop all worker threads...
		{
			std::unique_lock<std::mutex> lock(mutex_);
			shutdown_ = true;
		}

		cond_.notify_all();

		// Join...
		for (auto& t : threads_) {
			t.join();
		}
		{
			std::unique_lock<std::mutex> lock(mutex_);
			threads_.clear();
		}
	}

	int workerRunning()
	{
		lock_guard lg(mutex_);
		return busyCount.load() + jobs_.size();
	}

private:
	struct worker {
		explicit worker(ThreadPool& pool)
			: pool_(pool)
		{
		}

		void operator()()
		{
			for (;;) {
				std::function<void()> fn;
				{
					std::unique_lock<std::mutex> lock(pool_.mutex_);

					pool_.cond_.wait(lock, [&] { return !pool_.jobs_.empty() || pool_.shutdown_; });
					// will not wati until all job down any more
					if (pool_.shutdown_ /* && pool_.jobs_.empty()*/) {
						break;
					}
					pool_.busyCount++;
					fn = pool_.jobs_.front();
					pool_.jobs_.pop_front();
				}
				LOG_EXPECT_TRUE("ThreadPool", fn != nullptr);
				fn();
				pool_.busyCount--;
			}
		}

		ThreadPool& pool_;
	};
	friend struct worker;

	std::vector<std::thread>		 threads_;
	std::list<std::function<void()>> jobs_;

	bool shutdown_;

	std::condition_variable cond_;
	std::mutex				mutex_;
	// record busy thread
	atomic_int				busyCount = 0;
};

}