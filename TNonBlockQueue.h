#pragma once


#ifndef NONBLOCKQUEUE_H
#define NONBLOCKQUEUE_H

#include <atomic>


template<typename T, size_t SIZE>
class TNonBlockQueue
{
public:
    TNonBlockQueue()
        : m_buffer(new Item[SIZE])
        , m_bufferSize(SIZE)
    {
        for (size_t i = 0; i < m_bufferSize; ++i)
            m_buffer[i].marker.store(i, std::memory_order_relaxed);

        m_enqPos.store(0, std::memory_order_relaxed);
        m_deqPos.store(0, std::memory_order_relaxed);
    }

    ~TNonBlockQueue()
    {
        delete[] m_buffer;
    }

    bool Enqueue(T const& data)
    {
        Item* pItem = nullptr;
        size_t index{};

        while (true)
        {

            index = m_enqPos.load(std::memory_order_relaxed);

            if (index >= m_bufferSize) // try moving enqueue cursor to the start position
            {
                if (m_enqPos.compare_exchange_weak(index, 0, std::memory_order_relaxed))
                    index = 0;
                else
                    continue;
            }

            pItem = &m_buffer[index];

            if (!pItem)
                return false;

            size_t marker = pItem->marker.load(std::memory_order_acquire);
            int dif = static_cast<int>(marker) - static_cast<int>(index);

            if (dif == 0) // try moving enqueue cursor to the new position
            {
                if (m_enqPos.compare_exchange_weak(index, index + 1, std::memory_order_relaxed))
                    break;
                else
                    continue;
            }
            else if (dif < 0) // queue is full
                return false;
        }

        if (pItem) // place new data to the queue
        {
            pItem->data = data;
            pItem->marker.store(index + 1, std::memory_order_release);
            return true;
        }
        else
            return false;

    }

    bool Dequeue(T& data)
    {
        Item* pItem = nullptr;
        size_t index{};

        while (true)
        {
            index = m_deqPos.load(std::memory_order_relaxed);

            if (index >= m_bufferSize)
            {
                if (m_deqPos.compare_exchange_weak(index, 0, std::memory_order_relaxed))
                {
                    index = 0;
                    break;
                }
                else
                    continue;
            }

            pItem = &m_buffer[index];

            if (!pItem)
                return false;

            size_t marker = pItem->marker.load(std::memory_order_acquire);
            int dif = static_cast<int>(marker) - static_cast<int>(index + 1);

            if (dif == 0)
            {
                if (m_deqPos.compare_exchange_weak(index, index + 1, std::memory_order_relaxed))
                    break;
            }

            else if (dif < 0)
                return false;
        }

        if (pItem)
        {
            data = pItem->data;
            pItem->marker.store(index, std::memory_order_release);
            return true;
        }

        return false;
    }
    
    bool isEmpty()
    {
        return m_enqPos == m_deqPos;
    }
    
    TNonBlockQueue(TNonBlockQueue const&) = delete;
    void operator = (TNonBlockQueue const&) = delete;

private:
    struct Item
    {
        std::atomic<size_t> marker;
        T data;
    };


    Item* const m_buffer;
    size_t const m_bufferSize;

    std::atomic<size_t> m_enqPos;
    std::atomic<size_t> m_deqPos;
};

#endif
