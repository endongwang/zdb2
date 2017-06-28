/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * Author   : zhllxt
 * QQ       : 37792738
 * Email    : 37792738@qq.com
 * referenced from : http://blog.csdn.net/10km/article/details/49641691
 * 
 */

#pragma once

#include <atomic>
#include <thread>
#include <stdexcept>

namespace zdb2
{

	/**
	 * high performance read write lock based on std::atomic
	 */
	class rwlock
	{
	public:

		rwlock(bool is_write_first = true)
			: m_is_write_first(is_write_first)
			, m_write_wait_count(0)
			, m_lock_count(0)
		{
		}

		bool try_lock_read()
		{
			int count = m_lock_count;

			// if write first is true,need wait for util m_write_wait_count equal to zero,this means 
			// let the write thread first get the lock
			if (count == -1 || (m_is_write_first && m_write_wait_count > 0))
				return false;

			return m_lock_count.compare_exchange_weak(count, count + 1);
		}

		void lock_read()
		{
			for (unsigned int k = 0; !try_lock_read(); ++k)
			{
				if (k < 16)
				{
					std::this_thread::yield();
				}
				else if (k < 32)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(0));
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}
		}

		void unlock_read()
		{
			m_lock_count--;

#if defined(_DEBUG) || defined(DEBUG)
			if (m_lock_count < 0)
				throw std::runtime_error("exception : read lock count is less than 0,check whether lock_read/unlock_read missing match.");
#endif // _DEBUG DEBUG
		}

		bool try_lock_write()
		{
			// check m_lock_count whether equal to 0,it must be equal 0
			int count = 0;
			return this->m_lock_count.compare_exchange_weak(count, -1);
		}

		void lock_write()
		{
			m_write_wait_count++;
			for (unsigned int k = 0; !try_lock_write(); ++k)
			{
				if (k < 16)
				{
					std::this_thread::yield();
				}
				else if (k < 32)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(0));
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}
			m_write_wait_count--;
		}

		void unlock_write()
		{
			m_lock_count.store(0);
		}

	private:
		/// no copy construct function
		rwlock(const rwlock&) = delete;

		/// no operator equal function
		rwlock& operator=(const rwlock&) = delete;

	private:
		/// wait for write thread count
		std::atomic_uint m_write_wait_count;

		/// used to indicate the lock status, -1 : write,  0 : idle,  >0 : shared read
		std::atomic_int m_lock_count;

		/// is write first or not
		const bool m_is_write_first = true;
	};

	/**
	 * auto call lock_read and unlock_read 
	 */
	class rlock_guard
	{
	public:
		explicit rlock_guard(rwlock & lock) : m_rwlock(lock)
		{
			m_rwlock.lock_read();
		}
		~rlock_guard()
		{
			m_rwlock.unlock_read();
		}
	private:
		rwlock & m_rwlock;

		rlock_guard(const rlock_guard&) = delete;
		rlock_guard& operator=(const rlock_guard&) = delete;
	};

	/**
	 * auto call lock_write and unlock_write 
	 */
	class wlock_guard
	{
	public:
		explicit wlock_guard(rwlock & lock) : m_rwlock(lock)
		{
			m_rwlock.lock_write();
		}
		~wlock_guard()
		{
			m_rwlock.unlock_write();
		}
	private:
		rwlock & m_rwlock;

		wlock_guard(const wlock_guard&) = delete;
		wlock_guard& operator=(const wlock_guard&) = delete;
	};

}

