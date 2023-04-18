
#pragma once

#ifndef _LOCKFREE_ARRAY_QUEUE_INCLUDE_
#define _LOCKFREE_ARRAY_QUEUE_INCLUDE_

#include <memory.h>
#include <thread>
#include <AtomicOpt.h>
#include <MultipleLock.h>

#define USE_CAS_LINUX 0
#define USE_CAS_STL_STRONG 1
#define USE_CAS_STL_WEAK 0

#define MEMORY_ORDER std::memory_order_relaxed

#define LOCK_FREE_ARRAY_QUEUE_DEFAULT_SIZE 65535 //2^16

template <class E>
class LockFreeArrayQueue
{
public:
    LockFreeArrayQueue() : m_tElementQuque(nullptr), m_tActiveQueue(NULL), m_nMaxQueueSize(0), m_nWriteIndex(-1), m_nReadIndex(-1)
    {
        m_nMode = E_CAS;
    }
	
    ~LockFreeArrayQueue()
    {
        Clear();
    }

    int Init(LockFreeQueueMode nMode = E_CAS, int nQueueSize = LOCK_FREE_ARRAY_QUEUE_DEFAULT_SIZE)
    {
        m_nMode = nMode;

        m_tElementQuque = new E[nQueueSize];
        if (m_tElementQuque == nullptr)
        {
            return -1;
        }

        m_tActiveQueue = new long long[nQueueSize];
        if (m_tActiveQueue == nullptr)
        {
            return -1;
        }
        memset(m_tActiveQueue, 0, sizeof(long long) * nQueueSize);

        m_nMaxQueueSize = nQueueSize;
        if (m_nMaxQueueSize <= 1)
        {
            return -2;
        }

        m_nWriteIndex = 0;
        m_nReadIndex = 0;

        if (m_tMultipleLock.Init() < 0)
        {
            return -1;
        }

        return 0;
    }

    void Clear()
    {
        delete[] m_tElementQuque;
        m_tElementQuque = nullptr;

        delete[] m_tActiveQueue;
        m_tActiveQueue = nullptr;

        m_nMaxQueueSize = 0;

        m_nWriteIndex = -1;
        m_nReadIndex = -1;
    }

    std::string GetCASFunction() const
    {
#if USE_CAS_LINUX 
        return "__sync_bool_compare_and_swap";
#elif USE_CAS_STL_STRONG
        return "atomic_compare_exchange_strong";
#elif USE_CAS_STL_WEAK
        return "atomic_compare_exchange_weak";
#endif
    }

    long long Size() const
    {
        if (m_nReadIndex <= m_nWriteIndex)
        {
            return (m_nWriteIndex - m_nReadIndex);
        }
        else
        {
            return m_nWriteIndex - m_nReadIndex + m_nMaxQueueSize;
        }
    }
    bool Push(E&& e)
    {
        if (m_nMode == E_CAS)
        {
            return Push_CAS(std::move(e));
        }
        else
        {
            return Push_Lock(std::move(e));
        }
        return false;
    }

    bool Push_CAS(E&& e)
    {
#if USE_CAS_LINUX 
        long long nWriteIndex = m_nWriteIndex;
        long long nReadIndex = m_nReadIndex;
#elif USE_CAS_STL_STRONG || USE_CAS_STL_WEAK
        long long nWriteIndex = m_nWriteIndex.load(MEMORY_ORDER);  // 获取写位置和读取位置
        long long nReadIndex = m_nReadIndex.load(MEMORY_ORDER);   // 
#endif

        if (IsFull(nWriteIndex, nReadIndex))
        {
            return false;
        }

        long long nWriteTransIndex_1 = nWriteIndex + 1;

#if USE_CAS_LINUX 
        while (!__sync_bool_compare_and_swap(&m_nWriteIndex, nWriteIndex, nWriteTransIndex_1))
#elif USE_CAS_STL_STRONG
        while (!std::atomic_compare_exchange_strong_explicit(&m_nWriteIndex, &nWriteIndex, nWriteTransIndex_1, MEMORY_ORDER, MEMORY_ORDER))
#elif USE_CAS_STL_WEAK
        while (!std::atomic_compare_exchange_weak_explicit(&m_nWriteIndex, &nWriteIndex, nWriteTransIndex_1, MEMORY_ORDER, MEMORY_ORDER))
#endif
        {
#if USE_CAS_LINUX
            nWriteIndex = m_nWriteIndex;
            nReadIndex = m_nReadIndex;
#elif USE_CAS_STL_STRONG || USE_CAS_STL_WEAK
            nWriteIndex = m_nWriteIndex.load(MEMORY_ORDER);     // 继续获取写入位置
            nReadIndex = m_nReadIndex.load(MEMORY_ORDER);
#endif
            if (IsFull(nWriteIndex, nReadIndex))
            {
                return false;
            }
            nWriteTransIndex_1 = nWriteIndex + 1;
        }
        //��ȡһ��д���λ��

        long long nWriteTransIndex = TransIndex(nWriteIndex);

        long long nActive = m_tActiveQueue[nWriteTransIndex];
        long long nCompareActive = 0;
        if (nWriteIndex >= m_nMaxQueueSize)
        {
            nCompareActive = (nWriteIndex - m_nMaxQueueSize + 1) * (-1);
        }
        while (nActive != nCompareActive)
        {
            sched_yield();
            nActive = m_tActiveQueue[nWriteTransIndex];
        }

        m_tElementQuque[nWriteTransIndex] = std::move(e);
        m_tActiveQueue[nWriteTransIndex] = nWriteIndex + 1;

        return true;
    }

    bool Push_Lock(E&& e)
    {
        m_tMultipleLock.Lock(m_nMode);

        if (IsFull(m_nWriteIndex, m_nReadIndex))
        {
            m_tMultipleLock.UnLock(m_nMode);
            return false;
        }

        long long nWriteTransIndex = TransIndex(m_nWriteIndex);
        m_tElementQuque[nWriteTransIndex] = std::move(e);
        m_nWriteIndex = m_nWriteIndex + 1;

        m_tMultipleLock.UnLock(m_nMode);
        return true;
    }

    bool Pop(E& e)
    {
        if (m_nMode == E_CAS)
        {
            return Pop_CAS(e);
        }
        else
        {
            return Pop_Lock(e);
        }
        return false;
    }

    bool Pop_CAS(E& e)
    {
#if USE_CAS_LINUX
        long long nWriteIndex = m_nWriteIndex;
        long long nReadIndex = m_nReadIndex;
#elif USE_CAS_STL_STRONG || USE_CAS_STL_WEAK
        long long nWriteIndex = m_nWriteIndex.load(MEMORY_ORDER);
        long long nReadIndex = m_nReadIndex.load(MEMORY_ORDER);
#endif

        if (IsEmpty(nWriteIndex, nReadIndex))
        {
            return false;
        }

        long long nReadTransIndex_1 = nReadIndex + 1;

#if USE_CAS_LINUX 
        while (!__sync_bool_compare_and_swap(&m_nReadIndex, nReadIndex, nReadTransIndex_1))
#elif USE_CAS_STL_STRONG
        while (!std::atomic_compare_exchange_strong_explicit(&m_nReadIndex, &nReadIndex, nReadTransIndex_1, MEMORY_ORDER, MEMORY_ORDER))
#elif USE_CAS_STL_WEAK
        while (!std::atomic_compare_exchange_weak_explicit(&m_nReadIndex, &nReadIndex, nReadTransIndex_1, MEMORY_ORDER, MEMORY_ORDER))
#endif
        {
#if USE_CAS_LINUX
            nWriteIndex = m_nWriteIndex;
            nReadIndex = m_nReadIndex;
#elif USE_CAS_STL_STRONG || USE_CAS_STL_WEAK
            nWriteIndex = m_nWriteIndex.load(MEMORY_ORDER);
            nReadIndex = m_nReadIndex.load(MEMORY_ORDER);
#endif
            if (IsEmpty(nWriteIndex, nReadIndex))
            {
                return false;
            }
            nReadTransIndex_1 = nReadIndex + 1;
        }

        long long nReadTransIndex = TransIndex(nReadIndex);

        long long nActive = m_tActiveQueue[nReadTransIndex];
        long long nCompareActive = (nReadIndex + 1);
        while (nActive != nCompareActive)
        {
            sched_yield();
            nActive = m_tActiveQueue[nReadTransIndex];
        }
        
        e = std::move(m_tElementQuque[nReadTransIndex]);
        m_tActiveQueue[nReadTransIndex] = nCompareActive * (-1);

        return true;
    }

    bool Pop_Lock(E& e)
    {
        m_tMultipleLock.Lock(m_nMode);

        if (IsEmpty(m_nWriteIndex, m_nReadIndex))
        {
            m_tMultipleLock.UnLock(m_nMode);
            return false;
        }

        long long nReadTransIndex = TransIndex(m_nReadIndex);
        e = std::move(m_tElementQuque[nReadTransIndex]);
        m_nReadIndex = m_nReadIndex + 1;

        m_tMultipleLock.UnLock(m_nMode);
        return true;
    }

    bool TryPop(E& e)
    {
        return false;
    }

    bool IsFull() const
    {
        return IsFull(m_nWriteIndex, m_nReadIndex);
    }

    bool IsEmpty() const
    {
        return IsEmpty(m_nWriteIndex, m_nReadIndex);
    }

private:

    bool IsFull(long long nWriteIndex, long long nReadIndex) const
    {
        if (nWriteIndex - nReadIndex >= m_nMaxQueueSize)
        {
            return true;
        }
        return false;
    }

    bool IsEmpty(long long nWriteIndex, long long nReadIndex) const
    {
        if (nWriteIndex <= nReadIndex)
        {
            return true;
        }
        return false;
    }

    long long TransIndex(long long nIndex) const
    {
        return (nIndex & (m_nMaxQueueSize - 1));
    }

private:

    E* m_tElementQuque;
    long long* m_tActiveQueue;
    long long m_nMaxQueueSize;

#if USE_CAS_LINUX 
    long long m_nWriteIndex;
	long long m_nReadIndex;
#elif USE_CAS_STL_STRONG || USE_CAS_STL_WEAK
    std::atomic<long long> m_nWriteIndex;
    std::atomic<long long> m_nReadIndex;
#endif

    MultipleLock m_tMultipleLock;
    LockFreeQueueMode m_nMode;
};

#endif //_LOCKFREE_ARRAY_QUEUE_INCLUDE_
