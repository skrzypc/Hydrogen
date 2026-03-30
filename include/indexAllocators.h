#pragma once

#include <numeric>
#include <vector>

#include "basicTypes.h"
#include "verifier.h"

namespace Hydrogen
{
    class LinearIndexAllocator
    {
    public:
        LinearIndexAllocator() = default;
		~LinearIndexAllocator() = default;
        LinearIndexAllocator(const LinearIndexAllocator&) = delete;
        LinearIndexAllocator& operator=(const LinearIndexAllocator&) = delete;
        LinearIndexAllocator(LinearIndexAllocator&&) noexcept = default;
        LinearIndexAllocator& operator=(LinearIndexAllocator&&) noexcept = default;

        void Initialize(uint32 start, uint32 capacity)
        {
            m_start = start;
            m_capacity = capacity;

            m_size = 0;
            m_next = 0;
        }

        uint32 Allocate()
        {
            H2_VERIFY(HasSpace(), "LinearIndexAllocator out of space");

            uint32 index = m_start + m_next;
            ++m_next;
            ++m_size;

            return index;
        }

        void Reset()
        {
            m_next = 0;
            m_size = 0;
        }

        bool HasSpace() const
        {
            return m_size < m_capacity;
        }

        uint32 GetStart() const { return m_start; }
        uint32 GetCapacity() const { return m_capacity; }
        uint32 GetSize() const { return m_size; }

    private:
        uint32 m_start = 0;
        uint32 m_capacity = 0;

        uint32 m_next = 0;
        uint32 m_size = 0;
    };

    class FreeListIndexAllocator
    {
    public:
        void Initialize(uint32 start, uint32 capacity)
        {
            m_freeSlots.resize(capacity);
            std::iota(m_freeSlots.begin(), m_freeSlots.end(), start);
        }

        uint32 Allocate()
        {
            H2_VERIFY(!m_freeSlots.empty(), "FreeListIndexAllocator out of space");
            uint32 index = m_freeSlots.back();
            m_freeSlots.pop_back();
            return index;
        }

        void Free(uint32 index)
        {
            m_freeSlots.push_back(index);
        }

    private:
        std::vector<uint32> m_freeSlots;
    };
}