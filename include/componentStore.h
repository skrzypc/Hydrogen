#pragma once

#include <limits>
#include <span>
#include <vector>

#include "basicTypes.h"
#include "entity.h"

namespace Hydrogen
{
    template<typename T>
    class ComponentStore
    {
    public:
        void Add(Entity entity, T component)
        {
            if (entity.id >= m_sparse.size())
            {
                m_sparse.resize(entity.id + 1, std::numeric_limits<uint32>::max());
            }

            m_sparse[entity.id] = static_cast<uint32>(m_dense.size());
            m_dense.push_back(std::move(component));
            m_entities.push_back(entity);
        }

        void Remove(Entity entity)
        {
            if (!Has(entity))
            {
                return;
            }

            uint32 denseIdx = m_sparse[entity.id];
            uint32 lastIdx = static_cast<uint32>(m_dense.size()) - 1;

            if (denseIdx != lastIdx)
            {
                m_dense[denseIdx] = std::move(m_dense[lastIdx]);
                m_entities[denseIdx] = m_entities[lastIdx];
                m_sparse[m_entities[denseIdx].id] = denseIdx;
            }

            m_dense.pop_back();
            m_entities.pop_back();
            m_sparse[entity.id] = std::numeric_limits<uint32>::max();
        }

        T* Get(Entity entity)
        {
            if (!Has(entity))
            {
                return nullptr;
            }
            return &m_dense[m_sparse[entity.id]];
        }

        const T* Get(Entity entity) const
        {
            if (!Has(entity))
            {
                return nullptr;
            }
            return &m_dense[m_sparse[entity.id]];
        }

        bool Has(Entity entity) const
        {
            return entity.id < m_sparse.size() &&
                   m_sparse[entity.id] != std::numeric_limits<uint32>::max();
        }

        std::span<T> GetAll() { return m_dense; }
        std::span<const T> GetAll() const { return m_dense; }

        const std::vector<Entity>& GetEntities() const { return m_entities; }

    private:
        std::vector<T> m_dense{};
        std::vector<Entity> m_entities{};
        std::vector<uint32> m_sparse{};
    };
}
