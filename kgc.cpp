#include "kgc.h"
#include "kvalue.h"

Value* Kgc::allocValue()
{
    return new Value();
}

