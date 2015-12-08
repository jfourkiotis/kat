#ifndef KAT_GC_H_INCLUDED
#define KAT_GC_H_INCLUDED

#include <vector>
#include "kvalue.h"

class Value;

#define INITIAL_GC_THRESHOLD 256

class GcGuard;

class Kgc
{
public:
    explicit Kgc(unsigned int maxObjects = INITIAL_GC_THRESHOLD)
    : numObjects_(0), maxObjects_(maxObjects) {}
    
    ~Kgc();
    
    void pushStackRoot(const Value *v) { stackRoots_.push_back(v); }
    void pushLocalStackRoot(const Value **v) { localStackRoots_.push_back(v); }
    void popLocalStackRoot() { localStackRoots_.pop_back(); }
    void collect();

    Value* allocValue(ValueType type);
    
private:
    void mark(const Value *v);
    void sweep();
    void markAll();
    void dealloc(const Value *v);
    Value* allocSpecial(ValueType type);
    
    
    unsigned int numObjects_;
    unsigned int maxObjects_;
    const Value* firstObject_ = nullptr;
    
    std::vector<const Value  *> stackRoots_;
    std::vector<const Value **> localStackRoots_;

    friend class GcGuard;
};

class GcGuard
{
public:
    explicit GcGuard(Kgc &gc) : gc_(gc) {}

    ~GcGuard()
    {
        while (times_)
        {
            gc_.popLocalStackRoot();
            --times_;
        }
    }

    void pushLocalStackRoot(const Value **local)
    {
        gc_.pushLocalStackRoot(local);
        ++times_;
    }
private:
    Kgc &gc_;
    long times_ = 0;
};

#endif /* KAT_GC_H_INCLUDED */
