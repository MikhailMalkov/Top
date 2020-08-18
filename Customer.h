#pragma once

#include "IObserver.h" 
#include <iostream>

class Customer: public IObserver
{
	public:
		virtual void UpdateLoosersTop(std::vector<Top> topLoosers) override
		{
			std::cout << " The BEGIN OF Loosers Top ****************************** " << std::endl;
			for (const auto& it : topLoosers)
			{
				    std::cout << " Stock id: " << it.index
					<< " First price: " << it.basePrice
					<< " Last price: " << it.lastPrice
					<< " Change: " << it.delta << std::endl;;
			}
			std::cout << " The END OF Loosers Top ****************************** " << std::endl;
		}

		virtual void UpdateWinnersTop(std::vector<Top>topWinners) override
		{
			std::cout << " The BEGIN OF Winners Top ****************************** " << std::endl;
			for (const auto& it : topWinners)
			{
				std::cout << " Stock id: " << it.index
					<< " First price: " << it.basePrice
					<< " Last price: " << it.lastPrice
					<< " Change: " << it.delta << std::endl;;
			}
			std::cout << " The END OF Winners Top ****************************** " << std::endl;
		}
};