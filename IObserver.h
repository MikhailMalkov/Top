#pragma once

#include <vector>
#include <atomic>

struct alignas(64) Top
{
	double basePrice;
	double lastPrice;
	double delta;
	__int64 index;
};
	
class IObserver
{
	public:
		virtual void UpdateLoosersTop(std::vector<Top> topLoosers) = 0;
		virtual void UpdateWinnersTop(std::vector<Top>topWinners) = 0;
		virtual ~IObserver() = default;
};