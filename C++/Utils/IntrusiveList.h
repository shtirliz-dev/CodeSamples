#pragma once

#include <assert.h>

template <typename T>
struct IntrusiveListNode_s
{
    IntrusiveListNode_s():
        next(nullptr),
        prev(nullptr)
    {}

    T* next;
    T* prev;
};

template <typename T>
class IntrusiveList
{
private:
    T* mHeadPtr = nullptr;
    T* mTailPtr = nullptr;

public:
    IntrusiveList()
    {
    }

    ~IntrusiveList()
    { 
        clear(); 
    }

    void insertHead(T* elem)
    {
        auto node = &(elem->node);

        assert(node->next == nullptr);
        assert(node->prev == nullptr);

        node->prev = nullptr;
        node->next = mHeadPtr;

        if (mHeadPtr != nullptr)
        {
            auto lastHead = &(mHeadPtr->node);
            lastHead->prev = elem;
        }

        mHeadPtr = elem;

        if (mTailPtr == nullptr)
        {
            mTailPtr = mHeadPtr;
        }
    }

    void insertTail(T* item)
    {
        if (mTailPtr == nullptr)
        {
            insertHead(item);
        }
        else
        {
            auto tailNode = &(mTailPtr->node);
            auto itemNode = &(item->node);

            assert(itemNode->next == nullptr);
            assert(itemNode->prev == nullptr);

            tailNode->next = item;
            itemNode->prev = mTailPtr;
            itemNode->next = nullptr;
            mTailPtr = item;
        }
    }

    T* removeHead()
    {
        auto ret = head();
        remove(head());
        return ret;
    }

    T* removeTail()
    {
        auto ret = tail();
        remove(tail());
        return ret;
    }

    void remove(T* item)
    {
        auto node = &(item->node);
        auto next = node->next;
        auto prev = node->prev;
        auto nextNode = &(next->node);
        auto prevNode = &(prev->node);

        if (item == mHeadPtr)
        {
            mHeadPtr = next;
        }

        if (item == mTailPtr)
        {
            mTailPtr = prev;
        }

        if (prev != nullptr)
        {
            prevNode->next = next;
        }

        if (next != nullptr)
        {
            nextNode->prev = prev;
        }

        node->next = nullptr;
        node->prev = nullptr;
    }

    T* head() 
    { 
        return mHeadPtr; 
    }

    T* tail() const
    { 
        return mTailPtr;
    }

    T* next(T* item)
    {
        const auto node = &(item->node);
        return node->next;
    }

    const T* next(const T* item)
    {
        const auto node = &(item->node);
        return node->next;
    }

    T* prev(T* item)
    {
        const auto node = &(item->node);
        return node->prev;
    }

    const T* prev(const T* item)
    {
        const auto node = &(item->node);
        return node->prev;
    }

    void clear()
    {
        while (!empty())
        {
            remove(head());
        }
    }

    bool empty() 
    { 
        return mHeadPtr == nullptr; 
    }
};