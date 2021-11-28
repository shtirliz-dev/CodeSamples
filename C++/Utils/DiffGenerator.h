#pragma once

#include "utils/IntrusiveList.h"
#include "utils/ObjectPool.h"
#include "data/static.h"

namespace Utils
{
    class IUser;

    class DiffGenerator
    {
    private:
        struct PathEntry_s
        {
            const char* name;
            const input_value* value;
            const std::string* cachedKey;
            IntrusiveListNode_s<PathEntry_s> node;
        };

        IUser* mPlayer = nullptr;
        input_value::AllocatorType* mAllocatorModify;
        input_value::AllocatorType* mAllocatorDelete;
        input_value::AllocatorType mUserDumpAllocator;
        input_value mUserDump;

    private:
        class Handler
        {
        protected:
            const input_value* mDiffTarget = nullptr;
            IntrusizeList<PathEntry_s> mPath;
            IntrusizeList<PathEntry_s> mTargetPath;
            ObjectPool<PathEntry_s> mPathEntryPool;
            size_t mTargetPathSize = 0;

        public:
            Handler(const input_value* diffTarget);

            const input_value* getCurrentElementFromTarget();

            virtual bool Value(const input_value& inValue);
            virtual bool StartArray(const input_value& inValue, bool& skipMembers);
            virtual bool EndArray(const input_value& inValue, rapidjson::SizeType elementCount);
            virtual bool StartObject(const input_value& inValue);
            virtual bool EndObject(const input_value& inValue, rapidjson::SizeType memberCount);
            virtual bool KeyStart(const input_value& inValue, const char* str, const input_value& inChildValue, bool& skipMembers);
            virtual bool KeyEnd(const input_value& inValue);

            virtual ~Handler() = default;
        };

        class DeleteHandler : public Handler
        {
        protected:
            input_value* mOutDiff;
            input_value::AllocatorType* mOutDiffAllocator;

        public:
            DeleteHandler(const input_value* target, input_value* outDiff, input_value::AllocatorType* allocator);
            void writeCurrentPathToDiff();

            bool KeyStart(const input_value& inValue, const char* str, const input_value& inChildValue, bool& skipMembers) override;
        };

        class ModifyHandler : public Handler
        {
        protected:
            input_value* mOutDiff;
            input_value::AllocatorType* mOutDiffAllocator;

        public:
            ModifyHandler(const input_value* target, input_value* outDiff, input_value::AllocatorType* allocator);
            void writeCurrentPathToDiff(const input_value& toBeCloned);

            bool Value(const input_value& curr) override;
            bool StartObject(const input_value& inValue) override;
            bool StartArray(const input_value& inValue, bool& skipMembers) override;
        };

        bool accept(const input_value& target, Handler& handler) const;
        bool processModify(input_value& outDiff);
        bool processDelete(input_value& outDiff);

    public:
        DiffGenerator(IUser* player, input_value::AllocatorType* allocatoModify, input_value::AllocatorType* allocatorDelete);

        bool generate(input_value& outDiffModify, input_value& outDiffDelete);
    };
}