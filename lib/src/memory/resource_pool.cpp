#include "memory/resource_pool.h"

#include <cstdlib>

#include "resource/resource.h"
#include "util/core.h"

namespace Raytracing
{
    void ResourcePool::Init(uint32_t _poolSize, uint32_t _resourceSize)
    {
        poolSize = _poolSize;
        resourceSize = _resourceSize;

        const size_t allocation_size = poolSize * (resourceSize + sizeof(uint32_t));
        memory = static_cast<uint8_t*>(malloc(allocation_size));

        freeIndices = reinterpret_cast<uint32_t*>(memory + poolSize * resourceSize);
        freeIndicesHead = 0;

        for (uint32_t i = 0; i < poolSize; ++i)
        {
            freeIndices[i] = i;
        }

        usedIndices = 0;
    }

    void ResourcePool::Shutdown()
    {
        if (freeIndicesHead != 0)
        {
            LOG_TRACE("Resource pool has unfreed resources.");

            for (uint32_t i = 0; i < freeIndicesHead; ++i)
                LOG_TRACE("\tResource {0}\n", freeIndices[i]);
        }

        ASSERT(usedIndices == 0, "Used indices != 0");
        free(memory);
    }

    uint32_t ResourcePool::ObtainResource()
    {
        if (freeIndicesHead < poolSize)
        {
            const uint32_t free_index = freeIndices[freeIndicesHead++];
            ++usedIndices;
            return free_index;
        }

        ASSERT(false, "Error: no more resources left");
        return INVALID_RESOURCE_HANDLE;
    }

    void ResourcePool::ReleaseResource(uint32_t handle)
    {
        freeIndices[--freeIndicesHead] = handle;
        --usedIndices;
    }

    void ResourcePool::FreeAllResources()
    {
        freeIndicesHead = 0;
        usedIndices = 0;

        for (uint32_t i = 0; i < poolSize; ++i)
        {
            freeIndices[i] = i;
        }
    }

    void* ResourcePool::AccessResource(uint32_t handle)
    {
        if ( handle != INVALID_RESOURCE_HANDLE ) {
            return &memory[handle * resourceSize];
        }
        return nullptr;
    }

    const void* ResourcePool::AccessResource(uint32_t handle) const
    {
        if ( handle != INVALID_RESOURCE_HANDLE ) {
            return &memory[handle * resourceSize];
        }
        return nullptr;
    }
}
