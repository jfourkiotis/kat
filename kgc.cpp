#include "kgc.h"
#include <cassert>
#include <algorithm>
#include <deque>

Value* Kgc::allocValue(ValueType type)
{
    if (numObjects_ >= maxObjects_)
    {
        collect();
    }
    
    Value *v = allocSpecial(type);
    v->next_ = firstObject_;
    firstObject_ = v;
    ++numObjects_;
    totalObjects_[(int)type]++;;
    return v;
}

Kgc::~Kgc()
{
    assert(localStackRoots_.empty());
#ifndef NDEBUG
    printf("statistics:\n");
    for (int i = 0; i != (int)ValueType::MAX; ++i)
        printf("totalObjects[%d] = %d\n", i, totalObjects_[i]);
    printf("reserved:\n");
    for (int i = 0; i != (int)ValueType::MAX; ++i)
        printf("%d => %zu\n", i, reserved[i].size());
#endif
    stackRoots_.clear();
    collect();
    for (auto &&vec : reserved)
    {
        for (auto v : vec)
            delete v;
        vec.clear();
    }
}

void Kgc::collect()
{
    auto numObjects = numObjects_;
    auto maxObjects = maxObjects_;
    markAll();
    sweep();
    maxObjects_ = std::max(numObjects_ * 2, (unsigned int)INITIAL_GC_THRESHOLD);
#ifndef NDEBUG
    printf("Collected %u objects, %u remaining (max = %u)\n", numObjects - numObjects_, numObjects_, maxObjects);
#endif
}

void Kgc::mark(const Value *v)
{
    if (IS_INT(v) || IS_CHR(v)) return;
    if (v->marked_) return;
    
    v->marked_ = 1;
    if (v->type() == ValueType::CELL)
    {
        const Cell *c = static_cast<const Cell *>(v);
        mark(c->head_);
        mark(c->tail_);
    } else if (v->type() == ValueType::COMP_PROC)
    {
        const CompoundProc *cp = static_cast<const CompoundProc *>(v);
        mark(cp->parameters_);
        mark(cp->body_);
        mark(cp->env_);
    }
}

void Kgc::sweep()
{
    const Value **object = &firstObject_;
    while (*object)
    {
        if (!(*object)->marked_)
        {
            const Value *unreached = *object;
            *object = unreached->next_;
            dealloc(unreached);
        } else
        {
            (*object)->marked_ = 0;
            object = (const Value **)&(*object)->next_;
        }
    }
}

void Kgc::markAll()
{
    for (auto v : localStackRoots_)
    {
        if (*v)
        {
            mark(*v);
        }
    }
    
    for (auto v : stackRoots_)
    {
        mark(v);
    }
}


Value* Kgc::allocSpecial(ValueType type)
{
    std::vector<Value *> &v = reserved[(int)type];
    
    if (!v.empty())
    {
        Value *last = v.back();
        v.pop_back();
        return last;
    }
    return allocNew(type);
}


Value* Kgc::allocNew(ValueType type)
{
    switch (type)
    {
        case ValueType::COMP_PROC:
            return new CompoundProc;
        case ValueType::CELL:
            return new Cell;
        case ValueType::INPUT_PORT:
            return new InputPort;
        case ValueType::OUTPUT_PORT:
            return new OutputPort;
        case ValueType::PRIM_PROC:
            return new PrimitiveProc;
        case ValueType::BOOLEAN:
            return new Boolean;
        case ValueType::STRING:
            return new String;
        case ValueType::SYMBOL:
            return new Symbol;
        case ValueType::NIL:
            return new Nil;
        case ValueType::EOF_OBJECT:
            return new Eof;
        default:
            assert(false);
            return nullptr;
    }
}

void Kgc::dealloc(const Value *v)
{
    --numObjects_;
    reserved[(int)v->type()].push_back(const_cast<Value *>(v));
}


