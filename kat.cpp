#include <iostream>
#include "kvm.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    std::cout << "Welcome to Kat v0.22. Use Ctrl+C to exit.\n";
    std::cin.unsetf(std::ios_base::skipws);

    Kvm vm;
    return vm.repl(std::cin, std::cout);
}
