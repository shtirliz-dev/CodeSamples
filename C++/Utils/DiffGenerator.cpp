#include "DiffGenerator.h"
#include "utils/Utils.h"
#include "user/IUser.h"
#include "utils/IntrusiveList.h"
#include "utils/ObjectPool.h"
#include "data/PersistentNameCache.h"
#include <future>

namespace Utils
{   
    DiffGenerator::Handler::Handler(const input_value* diffTarget) :
        mDiffTarget(diffTarget)
    {
    }

    const input_value* DiffGenerator::Handler::getCurrentElementFromTarget()
    {
        if (mTargetPath.empty() || mDiffTarget == nullptr)
        {
            return nullptr;
        }

        auto counter = 0U;
        auto current = mDiffTarget;
        auto pathEntry = mTargetPath.head();

        while (pathEntry != nullptr)
        {
            auto& item = *pathEntry;

            if (item.value != nullptr)
            {
                current = item.value;
                pathEntry = pathEntry->node.next;
                counter++;
                continue;
            }
            else
            {
                auto currentMember = current->FindMember(item.name);

                if (currentMember != current->MemberEnd())
                {
                    current = &currentMember->value;
                    item.value = current;
                    pathEntry = pathEntry->node.next;
                    counter++;
                }
                else
                {
                    break;
                }
            }
        }

        return counter == mTargetPathSize ? current : nullptr;
    }

    bool DiffGenerator::Handler::Value(const input_value& inValue)
    { 
        return true; 
    }

    bool DiffGenerator::Handler::StartArray(const input_value& inValue, bool& skipMembers)
    { 
        return true;
    }

    bool DiffGenerator::Handler::EndArray(const input_value& inValue, rapidjson::SizeType elementCount)
    {
        return true; 
    }

    bool DiffGenerator::Handler::StartObject(const input_value& inValue)
    { 
        return true; 
    }

    bool DiffGenerator::Handler::EndObject(const input_value& inValue, rapidjson::SizeType memberCount)
    { 
        return true; 
    }

    bool DiffGenerator::Handler::KeyStart(const input_value& inValue, const char* str, const input_value& inChildValue, bool& skipMembers)
    {
        mPath.insertTail(new (mPathEntryPool.getNextNoConstruct()) PathEntry_s{ str, &inChildValue, nullptr });
        mTargetPath.insertTail(new (mPathEntryPool.getNextNoConstruct()) PathEntry_s{ str, nullptr, nullptr });
        mTargetPathSize++;
        return true;
    }

    bool DiffGenerator::Handler::KeyEnd(const input_value& inValue)
    {
        mPathEntryPool.deletePooled(mPath.removeTail());
        mPathEntryPool.deletePooled(mTargetPath.removeTail());
        mTargetPathSize--;
        return true;
    }

    DiffGenerator::DiffGenerator(User* player, input_value::AllocatorType* allocatoModify, input_value::AllocatorType* allocatorDelete) :
        mPlayer(player),
        mAllocatorModify(allocatoModify),
        mAllocatorDelete(allocatorDelete)
    {
    }

    DiffGenerator::DeleteHandler::DeleteHandler(const input_value* target, input_value* outDiff, input_value::AllocatorType* allocator) :
        Handler(target),
        mOutDiff(outDiff),
        mOutDiffAllocator(allocator)
    {
    }

    void DiffGenerator::DeleteHandler::writeCurrentPathToDiff()
    {
        auto current = mOutDiff;
        auto pathEntry = mPath.head();

        while (pathEntry != nullptr)
        {
            auto& item = *pathEntry;
            auto currentMember = current->FindMember(item.name);

            if (currentMember == current->MemberEnd())
            {
                input_value newNode(rapidjson::kObjectType);
                const auto& key = item.cachedKey ? *item.cachedKey : PersistentNameCache::fetchAdd(item.name);
                item.cachedKey = &key;
                current->AddMember(rapidjson::StringRef(key.c_str()), newNode, *mOutDiffAllocator);
                current = &((*current)[item.name]);
            }
            else
            {
                current = &currentMember->value;
            }

            pathEntry = pathEntry->node.next;
        }

        current->SetInt(0);
    }

    bool DiffGenerator::DeleteHandler::KeyStart(const input_value& inValue, const char* str, const input_value& inChildValue, bool& skipMembers)
    {
        if (!Handler::KeyStart(inValue, str, inChildValue, skipMembers))
        {
            return false;
        }

        const auto targetMemeber = getCurrentElementFromTarget();

        if (targetMemeber == nullptr)
        {
            writeCurrentPathToDiff();
            skipMembers = true;
        }

        return true;
    }

    DiffGenerator::ModifyHandler::ModifyHandler(const input_value* target, input_value* outDiff, input_value::AllocatorType* allocator) :
        Handler(target),
        mOutDiff(outDiff),
        mOutDiffAllocator(allocator)
    {
    }

    bool DiffGenerator::ModifyHandler::Value(const input_value& curr)
    {
        auto prev = getCurrentElementFromTarget();

        if (prev == nullptr)
        {
            writeCurrentPathToDiff(curr);
        }
        else
        {
            CC_ASSERT(!prev->IsObject());
            CC_ASSERT(!curr.IsObject());

            if (curr != *prev)
            {
                writeCurrentPathToDiff(curr);
            }
        }

        return true;
    }

    bool DiffGenerator::ModifyHandler::StartObject(const input_value& curr)
    {
        if (curr.MemberCount() == 0)
        {
            auto prev = getCurrentElementFromTarget();

            if (prev == nullptr)
            {
                writeCurrentPathToDiff(curr);
            }
        }

        return true;
    }

    bool DiffGenerator::ModifyHandler::StartArray(const input_value& inValue, bool& skipMembers)
    {
        Value(inValue);
        skipMembers = true;
        return true;
    }

    void DiffGenerator::ModifyHandler::writeCurrentPathToDiff(const input_value& toBeCloned)
    {
        auto current = mOutDiff;
        auto pathEntry = mPath.head();

        while (pathEntry != nullptr)
        {
            auto& item = *pathEntry;
            auto currentMember = current->FindMember(item.name);

            if (currentMember == current->MemberEnd())
            {
                input_value newNode(rapidjson::kObjectType);
                const auto& key = item.cachedKey ? *item.cachedKey : PersistentNameCache::fetchAdd(item.name);
                item.cachedKey = &key;
                current->AddMember(rapidjson::StringRef(key.c_str()), newNode, *mOutDiffAllocator);
                current = &((*current)[item.name]);
            }
            else
            {
                current = &currentMember->value;
            }

            pathEntry = pathEntry->node.next;
        }

        current->CopyFrom(toBeCloned, *mOutDiffAllocator);
    }

    bool DiffGenerator::accept(const input_value& target, Handler& handler) const
    {
        switch (target.GetType())
        {
        case rapidjson::kObjectType:
        {
            if (!handler.StartObject(target))
            {
                return false;
            }

            for (auto m = target.MemberBegin(); m != target.MemberEnd(); ++m)
            {
                auto skipMembers = false;

                if (!handler.KeyStart(target, m->name.GetString(), m->value, skipMembers))
                {
                    return false;
                }

                if (!skipMembers)
                {
                    if (!accept(m->value, handler))
                    {
                        return false;
                    }
                }

                if (!handler.KeyEnd(target))
                {
                    return false;
                }
            }

            return handler.EndObject(target, target.MemberCount());
        }
        case rapidjson::kArrayType:
        {
            auto skipMembers = false;

            if (!handler.StartArray(target, skipMembers))
            {
                return false;
            }

            if (!skipMembers)
            {
                for (auto v = target.Begin(); v != target.End(); ++v)
                {
                    if (!accept(*v, handler))
                    {
                        return false;
                    }
                }
            }

            return handler.EndArray(target, target.Size());
        }
        default:
        {
            return handler.Value(target);
        }
        }
    }

    bool DiffGenerator::processModify(input_value& outDiff)
    {
        ScopedTimer timer("DiffGenerator: processModify");
        ModifyHandler handler(&PeopleModel::sLastUserSnapshot, &outDiff, mAllocatorModify);
        accept(mUserDump, handler);
        return true;
    }

    bool DiffGenerator::processDelete(input_value& outDiff)
    {
        ScopedTimer timer("DiffGenerator: processDelete");
        DeleteHandler handler(&mUserDump, &outDiff, mAllocatorDelete);
        accept(PeopleModel::sLastUserSnapshot, handler);
        return true;
    }

    bool DiffGenerator::generate(input_value& outDiffModify, input_value& outDiffDelete)
    {
        ScopedTimer timer("DiffGenerator: generate total");

        if (mPlayer == nullptr ||
            mAllocatorModify == nullptr ||
            mAllocatorDelete == nullptr)
        {
            return false;
        }

        {
            ScopedTimer timer("DiffGenerator: dump current user");
            mPlayer->save(mUserDump, mUserDumpAllocator);
        }

        auto delResult = std::async(std::launch::async, [&]()
        {
            return processDelete(outDiffDelete);
        });

        auto modResult = std::async(std::launch::async, [&]()
        {
            return processModify(outDiffModify);
        });

        return delResult.get() && modResult.get();
    }
}