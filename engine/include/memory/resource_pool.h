#pragma once
#include <cstdint>

#include "util/log.h"

namespace MongooseVK
{
    class PoolObject {
    public:
        uint32_t index;
    };

    class ResourcePool {
    public:
        void Init(uint32_t _poolSize, uint32_t _resourceSize);
        void Shutdown();

        uint32_t ObtainResource();
        void ReleaseResource(uint32_t handle);

        void FreeAllResources();

        void* AccessResource(uint32_t handle);
        const void* AccessResource(uint32_t handle) const;

    protected:
        uint32_t poolSize = 0;
        uint32_t resourceSize = 0;
        uint32_t freeIndicesHead = 0;
        uint32_t usedIndices = 0;

        uint8_t* memory = nullptr;
        uint32_t* freeIndices = nullptr;
    };

    template<typename T>
    class ObjectResourcePool : public ResourcePool {
    public:
        void Init(uint32_t _poolSize);
        void Shutdown();

        T* Obtain();
        void Release(T* resource);

        T* Get(uint32_t index);
        const T* Get(uint32_t index) const;
    };

    template<typename T>
    void ObjectResourcePool<T>::Init(uint32_t _poolSize)
    {
        ResourcePool::Init(_poolSize, sizeof(T));
    }

    template<typename T>
    void ObjectResourcePool<T>::Shutdown()
    {
        if (freeIndicesHead != 0)
        {
            LOG_TRACE("Resource pool has unfreed resources.\n");
            for (uint32_t i = 0; i < freeIndicesHead; ++i)
            {
                LOG_TRACE("\tResource {0}, {1}\n", freeIndices[i], Get(freeIndices[i])->name);
            }
        }
        ResourcePool::Shutdown();
    }

    template<typename T>
    T* ObjectResourcePool<T>::Obtain()
    {
        uint32_t resource_index = ObtainResource();
        if (resource_index != UINT32_MAX)
        {
            T* resource = Get(resource_index);
            resource->index = resource_index;
            return resource;
        }

        return nullptr;
    }

    template<typename T>
    void ObjectResourcePool<T>::Release(T* resource)
    {
        ResourcePool::ReleaseResource(resource->index);
    }

    template<typename T>
    T* ObjectResourcePool<T>::Get(uint32_t index)
    {
        return static_cast<T*>(AccessResource(index));
    }

    template<typename T>
    const T* ObjectResourcePool<T>::Get(uint32_t index) const
    {
        return static_cast<const T*>(AccessResource(index));
    }
}
