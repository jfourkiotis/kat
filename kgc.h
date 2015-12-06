#ifndef KAT_GC_H_INCLUDED
#define KAT_GC_H_INCLUDED

class Value;

#define INITIAL_GC_THRESHOLD 1

class Kgc
{
public:
    explicit Kgc(unsigned int maxObjects = INITIAL_GC_THRESHOLD)
    : numObjects_(0), maxObjects_(maxObjects) {}
    
    ~Kgc();
    
    void collect();
    
    Value* allocValue();
    
private:
    void mark(const Value *v);
    void sweep();
    void markAll();
    void dealloc(const Value *v);
    
    
    unsigned int numObjects_;
    unsigned int maxObjects_;
    const Value* firstObject_ = nullptr;
    
};

#endif /* KAT_GC_H_INCLUDED */
