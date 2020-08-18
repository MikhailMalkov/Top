#pragma once

#include "IObserver.h"



class INotifier 
{
public:
    virtual void Attach(IObserver* observer) = 0;
	virtual void Detach(IObserver* observer) = 0;
	virtual void OnQuote(const size_t stock_id, const double price) = 0;
	virtual ~INotifier() = default;
};