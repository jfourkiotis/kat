#ifndef KAT_GC_H_INCLUDED
#define KAT_GC_H_INCLUDED

#include <vector>
#include "kvalue.h"

class Value;

#define INITIAL_GC_THRESHOLD 1

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
    
};

#endif /* KAT_GC_H_INCLUDED */
