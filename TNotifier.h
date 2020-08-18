#pragma once

#include "INotifier.h"
#include "TAtomicLock.h"
#include "TNonBlockQueue.h"

#include <list>
#include <map>
#include <array>
#include <math.h>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <chrono>

template<size_t STOCK_COUNT, size_t TOP_COUNT, size_t NOTIFY_TIMEOUT> class TNotifier :public INotifier
{

	public:
		
		TNotifier() :
			m_notifyThread{ std::bind(&TNotifier::Notify, this) }, 
			m_countThread{ std::bind(&TNotifier::CountTop, this) }
		{
			
		}

		~TNotifier()
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			m_isStop = true;
			cvNotify.notify_all();
			cvItemReady.notify_all();
			
			if (m_notifyThread.joinable())
				m_notifyThread.join();
			
			if (m_countThread.joinable())
				m_countThread.join();
		}

	public:
		
		virtual void Attach(IObserver* observer) override
		{
			std::lock_guard<std::mutex> guard(m_notifyMutex);
			if(observer)
				m_Observers.push_back(observer);
		}
		
		virtual void Detach(IObserver* observer) override
		{
			std::lock_guard<std::mutex> guard(m_notifyMutex);
			if (observer)
				m_Observers.remove(observer);
		}
		
		virtual void OnQuote(const size_t stock_id, const double price) override
		{

			if (stock_id >= STOCK_COUNT)
				return;

			TAtomicLock<Quote> lock(&m_pStocks[stock_id]);

			const double basePrice = m_pStocks[stock_id].basePrice.load(std::memory_order_relaxed);

			if (basePrice < 0)
				m_pStocks[stock_id].basePrice.store(price, std::memory_order_relaxed);
			else
			{
				bool res{ false };
				
				for(size_t i = 0; i < STOCK_COUNT; ++i) // spin-lock cycle
				{
					if (notifyQueue.Enqueue(Top{ basePrice, price, m_pStocks[stock_id].delta.load(std::memory_order_relaxed), stock_id }))
					{
						res = true;
						break;
					}
				}
				
				if (res)
				{
					m_pStocks[stock_id].delta.store((price - basePrice < 0)? 
						((100 - (price * 100/ basePrice)) * -1.0) 
						: ((price * 100)/ basePrice - 100),
						std::memory_order_relaxed);
					m_itemReady = true;
					cvItemReady.notify_all();
				}
			}
				
		}
	
	private:
		
		void CountTop()
		{
			while (true)
			{
				std::unique_lock<std::mutex> lock(m_itemReadyMutex);
				
				while(!m_itemReady && !m_isStop)
					cvItemReady.wait(lock, [&] {return m_itemReady || m_isStop; });

				if (m_isStop)
					return;

				Top top{};
				bool res{ false };
				
				for(size_t i = 0; i < STOCK_COUNT; ++i) // spin-lock cycle 
				{
					while (notifyQueue.Dequeue(top)) // try get all items
					{
						if (top.delta)
						{
							auto rangе = m_Top.equal_range(top.delta);
							auto itTop = std::find_if(rangе.first, rangе.second,
								[&](auto& item) { return item.second.index == top.index; });

							if (itTop != m_Top.end())
								m_Top.erase(itTop);
						}
						double newDelta = (top.lastPrice - top.basePrice < 0) ?
							((100 - (top.lastPrice * 100 / top.basePrice)) * -1.0)
							: ((top.lastPrice * 100) / top.basePrice - 100);

						m_Top.emplace(newDelta, Top{
							top.basePrice, top.lastPrice, newDelta, top.index });
						res = true;
                                  	}
					if(res)
					    break;
				  }
				  
				  if(res)
				  {
					std::unique_lock<std::mutex> lock(m_notifyMutex);
					m_notifyReady = true;
					cvNotify.notify_all();
				  }
				
				  m_itemReady = false;
			}
		}

		void Notify()
		{
			while (true)
			{
				std::unique_lock<std::mutex> lock(m_notifyMutex);
				
				while(!m_notifyReady && !m_isStop)
					cvNotify.wait(lock, [&] {return m_notifyReady || m_isStop; });
			    
                		if (m_isStop)
					return;
				
				std::vector<Top> topLoosers;
				std::vector<Top> topWinners;

				
				for (auto &it : m_Top)
				{
					if (topLoosers.size() >= TOP_COUNT)
						break;

					if (it.first >= 0)
						break; // positiv changes, not loosers, zero - not changes

					topLoosers.push_back(it.second);

				}

				if (topLoosers.size())
				{
					for (const auto observer : m_Observers)
						observer->UpdateLoosersTop(std::move(topLoosers));
				}

				for (auto it = m_Top.rbegin(); it != m_Top.rend(); ++it)
				{
					if (topWinners.size() >= TOP_COUNT)
						break;

					if (it->first <= 0) // negativ changes of price - loosers, zero - not changes
						break;

					topWinners.push_back(it->second);

				}

				if (topWinners.size())
				{
					for (const auto observer : m_Observers)
						observer->UpdateWinnersTop(std::move(topWinners));
				}

				m_notifyReady = false;
			}
		}


		

	private:
		std::mutex m_notifyMutex;
		std::mutex m_itemReadyMutex;
		std::condition_variable cvNotify;
		std::condition_variable cvItemReady;
		bool m_notifyReady{ false };
		bool m_itemReady{ false };
		std::thread m_notifyThread;
		std::thread m_countThread;
		std::atomic<bool> m_isStop{false};
		std::list<IObserver*> m_Observers{};
		
		
		struct alignas(64) Quote
		{
			std::atomic<double> basePrice;
			std::atomic<double> delta;
			std::atomic<__int64> isLock;
			
			Quote()
			{
				basePrice.store(-1);
				delta.store(0);
				isLock.store(0);

			}
			
			Quote& operator =(const Quote& item)
			{
				basePrice.store(item.basePrice.load());
				delta.store(item.delta.load());
				isLock.store(item.isLock.load());
				return *this;
			}
		};
		
		std::array <Quote, STOCK_COUNT> m_pStocks{};
		std::multimap<double, Top> m_Top{};
		TNonBlockQueue<Top, STOCK_COUNT> notifyQueue{};
};
