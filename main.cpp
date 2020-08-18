#include "TNotifier.h"
#include "Customer.h"
#include <chrono>

int main()
{
	const size_t countStocks = 10000;
	const size_t countTop = 10;
	const size_t notifyTimeout = 2000;
	Customer customer;
	TNotifier<countStocks, countTop, notifyTimeout> notifier;
	notifier.Attach(&customer);
	
	std::shared_ptr<std::thread>(new std::thread([&notifier]()
		{
			notifier.OnQuote(0, 10);  // first price
			notifier.OnQuote(0, 20);
			notifier.OnQuote(1, 5);   // first price
			notifier.OnQuote(1, 30);
			notifier.OnQuote(2, 30);  // first price
			notifier.OnQuote(2, 10);
		}),
		[](std::thread* pthread)
		{
			if (pthread->joinable())
				pthread->join();
		}
	);

	std::shared_ptr<std::thread>(new std::thread([&notifier]()
		{
			notifier.OnQuote(4, 5);  // first price
			notifier.OnQuote(4, 25);
			notifier.OnQuote(5, 1);   // first price
			notifier.OnQuote(5, 4);
			notifier.OnQuote(6, 25);  // first price
			notifier.OnQuote(6, 5);
			notifier.OnQuote(7, 88);  // first price
			notifier.OnQuote(7, 10);
		}),
		[](std::thread* pthread)
		{
			if (pthread->joinable())
				pthread->join();
		}
	);
	
	
	return 0;
}
