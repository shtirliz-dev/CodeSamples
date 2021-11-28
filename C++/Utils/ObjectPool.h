#pragma once

#include <stdlib.h>
#include <iostream>

typedef void* RawPtr_t;

class DefaultMemoryAllocator
{
public:
	static inline RawPtr_t allocate(size_t size)
	{
		return ::operator new(size, ::std::nothrow);
	}

	static inline void deallocate(RawPtr_t pointer, size_t size)
	{
		::operator delete(pointer);
	}
};

template<typename T, class MemoryAllocator_t = DefaultMemoryAllocator>
class ObjectPool
{
private:
	struct PoolNode_s
	{
		RawPtr_t nodeMemory;
		size_t nodeCapacity;
        PoolNode_s* nextNode;

        PoolNode_s(size_t capacity)
		{
            if (capacity < 1)
            {
                throw std::invalid_argument("capacity must be greater/equal 1");
            }

            nodeMemory = MemoryAllocator_t::allocate(sItemSize * capacity);

            if (nodeMemory == nullptr)
            {
                throw std::bad_alloc();
            }

            nodeCapacity = capacity;
            nextNode = nullptr;
		}

		~PoolNode_s()
		{
            MemoryAllocator_t::deallocate(nodeMemory, sItemSize * nodeCapacity);
		}
	};

    RawPtr_t mNodeMemory;
	T* mFirstDeleted;
	size_t mCountInNode;
	size_t mNodeCapacity;
    PoolNode_s mFirstNode;
    PoolNode_s* mLastNode;
	size_t mMaxBlockLength;

    static const size_t sItemSize;

private:
	ObjectPool(const ObjectPool<T, MemoryAllocator_t>& source);
	void operator=(const ObjectPool<T, MemoryAllocator_t>& source) = delete;

	void allocateNewNodeInternal()
	{
		auto size = mCountInNode;

        if (size >= mMaxBlockLength)
        {
            size = mMaxBlockLength;
        }
		else
		{
			size *= 2;

            if (size < mCountInNode)
            {
                throw std::overflow_error("buffer overflow");
            }

            if (size >= mMaxBlockLength)
            {
                size = mMaxBlockLength;
            }
		}

        auto newNode = new PoolNode_s(size);

		mLastNode->nextNode = newNode;
		mLastNode = newNode;
		mNodeMemory = newNode->nodeMemory;
		mCountInNode = 0;
		mNodeCapacity = size;
	}

public:
	explicit ObjectPool(size_t initialCapacity = 32, size_t maxBlockLength = 1000000):
		mFirstDeleted(nullptr),
		mCountInNode(0),
		mNodeCapacity(initialCapacity),
		mFirstNode(initialCapacity),
		mMaxBlockLength(maxBlockLength)
	{
        if (maxBlockLength < 1)
        {
            throw std::invalid_argument("maxBlockLength must be greater/equal 1");
        }

		mNodeMemory = mFirstNode.nodeMemory;
		mLastNode = &mFirstNode;
	}

	~ObjectPool()
	{
        auto node = mFirstNode.nextNode;

		while(node != nullptr)
		{
			auto nextNode = node->nextNode;
			delete node;

			node = nextNode;
		}
	}

	T* newPooled()
	{
		if (mFirstDeleted)
		{
			auto result = mFirstDeleted;
			mFirstDeleted = *((T**)mFirstDeleted);

			new (result) T();
			return result;
		}

        if (mCountInNode >= mNodeCapacity)
        {
            allocateNewNodeInternal();
        }

		auto address = reinterpret_cast<char*>(mNodeMemory);
		address += mCountInNode * sItemSize;
		auto result = new (address) T();
		mCountInNode++;

		return result;
	}

	T* getNextNoConstruct()
	{
		if (mFirstDeleted)
		{
			auto result = (T*)mFirstDeleted;
            mFirstDeleted = *((T**)mFirstDeleted);

			return result;
		}

        if (mCountInNode >= mNodeCapacity)
        {
            allocateNewNodeInternal();
        }

		auto address = reinterpret_cast<char*>(mNodeMemory);
		address += mCountInNode * sItemSize;
		mCountInNode++;

		return (T*)address;
	}

	void deletePooled(T* content)
	{
		content->~T();

		*((T**)content) = mFirstDeleted;
		mFirstDeleted = content;
	}

	void deleteWithoutDestroying(T* content)
	{
		*((T**)content) = mFirstDeleted;
		mFirstDeleted = content;
	}
};

template<typename T, class MemoryAllocator_t>
const size_t ObjectPool<T, MemoryAllocator_t>::sItemSize = ((sizeof(T) + sizeof(RawPtr_t) - 1) / sizeof(RawPtr_t)) * sizeof(RawPtr_t);