#include "kgc.h"
#include "kvalue.h"
#include <cassert>

Value* Kgc::allocValue()
{
    if (numObjects_ >= maxObjects_)
    {
        collect();
    }
    
    Value *v = new Value();
    v->next_ = firstObject_;
    firstObject_ = v;
    ++numObjects_;
    return v;
}


Kgc::~Kgc()
{
    assert(localStackRoots_.empty());
    stackRoots_.clear();
    collect();
}

void Kgc::collect()
{
    auto numObjects = numObjects_;
    markAll();
    sweep();
    //maxObjects_ = numObjects_ * 2;
    maxObjects_ = numObjects_;
#ifndef NDEBUG
    printf("Collected %d objects, %d remaining.\n", numObjects - numObjects_, numObjects_);
#endif
}

void Kgc::mark(const Value *v)
{
    if (v->marked_) return;
    
    v->marked_ = 1;
    if (v->type() == ValueType::CELL)
    {
        mark(v->cell[0]);
        mark(v->cell[1]);
    } else if (v->type() == ValueType::COMP_PROC)
    {
        mark(v->compound_proc.parameters);
        mark(v->compound_proc.body);
        mark(v->compound_proc.env);
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

void Kgc::dealloc(const Value *v)
{
    --numObjects_;
#ifndef NDEBUG
    printf("del> %p %d\n", v, v->type());
#endif
    delete v;
}


