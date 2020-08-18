#pragma once
#include <atomic>
template<typename T> class TAtomicLock
{
	public:
		TAtomicLock(T* ptr): m_ptr(ptr)
		{
			while (true)
			{
				__int64 isLock = ptr->isLock.load(std::memory_order_acquire);

				if (!isLock)
					ptr->isLock.compare_exchange_strong(isLock, 1, std::memory_order_relaxed);
				else
					continue;

				break;
			}
		}
		
		~TAtomicLock()
		{
			m_ptr->isLock.store(0, std::memory_order_release);
		}

	private:
		T* m_ptr;

};