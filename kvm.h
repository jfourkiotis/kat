//
// Created by John Fourkiotis on 17/11/15.
//

#ifndef KAT_KVM_H
#define KAT_KVM_H

#include <string>
#include <unordered_map>
#include <iostream>

class Value;

class Kvm {

public:
    int repl(std::istream &in, std::ostream &out);
private:
    bool isQuoted(const Value *v);
    bool isTagged(const Value *v, const Value *tag);
    bool isSelfEvaluating(const Value *v);
    void print(const Value *v, std::ostream& out);
    const Value* eval(const Value *v);
    const Value* read(std::istream &in);
    void printCell(const Value *v, std::ostream &out);
    const Value* readPair(std::istream &in);
    const Value* readCharacter(std::istream &in);
    const Value* makeString(const std::string& str);
    const Value* makeCell(const Value *first, const Value* second);
    const Value* makeSymbol(const std::string& str);
    const Value* makeBool(bool condition);
    // makeFixnum & makeChar will be removed. We do not
    // want that many allocations for integers and chars.
    // This implementation is silly.
    const Value* makeFixnum(long num);
    const Value* makeChar(char c);
    const Value* makeNil();

    std::unordered_map<std::string, const Value *> interned_strings;
    std::unordered_map<std::string, const Value *> symbols;

    const Value* NIL   = makeNil();           // pretty dangerous initializations here.
    const Value* FALSE = makeBool(false);     // NIL, FALSE, TRUE & QUOTE will might
    const Value* TRUE  = makeBool(true);      // symbols and/or other members.
    const Value* QUOTE = makeSymbol("quote"); //
};


#endif //KAT_KVM_H
