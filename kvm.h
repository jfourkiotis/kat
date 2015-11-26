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
    bool isVariable(const Value *v);
    bool isAssignment(const Value *v);
    bool isDefinition(const Value *v);
    bool isIf(const Value *v);
    bool isCond(const Value *v);
    bool isApplication(const Value *v);
    bool isLambda(const Value *v);
    bool isCondElseClause(const Value *clause);
    const Value* lambdaParameters(const Value *v);
    const Value* lambdaBody(const Value *v);
    const Value* procOperator(const Value *v);
    const Value* procOperands(const Value *v);
    const Value* listOfValues(const Value *v, const Value *env);
    const Value* ifPredicate(const Value *v);
    const Value* ifConsequent(const Value *v);
    const Value* ifAlternative(const Value *v);
    const Value* expandClauses(const Value *v);
    const Value* condClauses(const Value *v);
    const Value* condPredicate(const Value *clause);
    const Value* sequence(const Value *v);
    const Value* condActions(const Value *v);
    const Value* condToIf(const Value *v);


    void print(const Value *v, std::ostream& out);
    const Value* eval(const Value *v, const Value *env);
    const Value* evalAssignment(const Value *v, const Value *env);
    const Value* definitionVariable(const Value *v);
    const Value* definitionValue(const Value *v);
    const Value* evalDefinition(const Value *v, const Value *env);
    const Value* assignmentVariable(const Value *v);
    const Value* assignmentValue(const Value* v);
    void setVariableValue(const Value *var, const Value *val, const Value *env);
    const Value* defineVariable(const Value *var, const Value *val, const Value *env);
    const Value* setupEnvironment();
    const Value* extendEnvironment(const Value *vars, const Value *vals, const Value *base_env);
    const Value* read(std::istream &in);
    const Value* firstFrame(const Value *env);
    const Value* makeFrame(const Value *vars, const Value *vals);
    const Value* frameVariables(const Value *frame);
    const Value* frameValues(const Value *frame);
    void addBindingToFrame(const Value *var, const Value *val, const Value *frame);
    const Value* enclosingEnv(const Value *env);
    const Value* lookupVariableValue(const Value *v, const Value *env);
    void printCell(const Value *v, std::ostream &out);
    const Value* readPair(std::istream &in);
    const Value* readCharacter(std::istream &in);
    const Value* makeString(const std::string& str);
    const Value* makeCell(const Value *first, const Value* second);
    const Value* makeSymbol(const std::string& str);
    const Value* makeBool(bool condition);
    const Value* makeBegin(const Value *v);
    const Value* makeIf(const Value *pred, const Value *conseq, const Value *alternate);
    bool isBegin(const Value *v);
    const Value* beginActions(const Value *v);
    // makeFixnum & makeChar will be removed. We do not
    // want that many allocations for integers and chars.
    // This implementation is silly.
    const Value* makeFixnum(long num);
    const Value* makeChar(char c);
    const Value* makeNil();
    const Value* makeProc(const Value* (*proc)(Kvm *vm, const Value *));
    const Value* makeCompoundProc(const Value *parameters, const Value *body, const Value *env);
    const Value* makeLambda(const Value *params, const Value *body);

    static const Value* isNullP(Kvm *vm, const Value *args);
    static const Value* isBoolP(Kvm *vm, const Value *args);
    static const Value* isSymbolP(Kvm *vm, const Value *args);
    static const Value* isIntegerP(Kvm *vm, const Value *args);
    static const Value* isCharP(Kvm *vm, const Value *args);
    static const Value* isStringP(Kvm *vm, const Value *args);
    static const Value* isPairP(Kvm *vm, const Value *args);
    static const Value* isProcedureP(Kvm *vm, const Value *args);

    static const Value* charToInteger(Kvm *vm, const Value *args);
    static const Value* integerToChar(Kvm *vm, const Value *args);
    static const Value* numberToString(Kvm *vm, const Value *args);
    static const Value* stringToNumber(Kvm *vm, const Value *args);
    static const Value* symbolToString(Kvm *vm, const Value *args);
    static const Value* stringToSymbol(Kvm *vm, const Value *args);


    static const Value* addProc(Kvm *vm, const Value *args);
    static const Value* subProc(Kvm *vm, const Value *args);
    static const Value* mulProc(Kvm *vm, const Value *args);
    static const Value* quotientProc(Kvm *vm, const Value *args);
    static const Value* remainderProc(Kvm *vm, const Value *args);
    static const Value* isNumberEqualProc(Kvm *vm, const Value *args);
    static const Value* isLessThanProc(Kvm *vm, const Value *args);
    static const Value* isGreaterThanProc(Kvm *vm, const Value *args);
    static const Value* consProc(Kvm *vm, const Value *args);
    static const Value* carProc(Kvm *vm, const Value *args);
    static const Value* cdrProc(Kvm *vm, const Value *args);
    static const Value* setCarProc(Kvm *vm, const Value *args);
    static const Value* setCdrProc(Kvm *vm, const Value *args);
    static const Value* listProc(Kvm *vm, const Value *args);
    static const Value* isEqProc(Kvm *vm, const Value *args);

    std::unordered_map<std::string, const Value *> interned_strings;
    std::unordered_map<std::string, const Value *> symbols;

    const Value* NIL   = makeNil();           // pretty dangerous initializations here.
    const Value* FALSE = makeBool(false);     // NIL, FALSE, TRUE & QUOTE will might
    const Value* TRUE  = makeBool(true);      // symbols and/or other members.
    const Value* QUOTE = makeSymbol("quote"); //
    const Value* DEFINE= makeSymbol("define");//
    const Value* SET   = makeSymbol("set!");  //
    const Value* OK    = makeSymbol("ok");    //
    const Value* IF    = makeSymbol("if");    //
    const Value* LAMBDA= makeSymbol("lambda");//
    const Value* BEGIN = makeSymbol("begin"); //
    const Value* COND  = makeSymbol("cond");  //
    const Value* ELSE  = makeSymbol("else");  //

    const Value* EMPTY_ENV = NIL;
    const Value* GLOBAL_ENV= setupEnvironment();

#define ADD_PROC(scheme_name, c_name) \
    defineVariable(makeSymbol(scheme_name), makeProc(c_name), GLOBAL_ENV)

    const Value* ISNULL     = ADD_PROC("null?", isNullP);
    const Value* ISBOOLP    = ADD_PROC("boolean?", isBoolP);
    const Value* ISSYMBOLP  = ADD_PROC("symbol?", isSymbolP);
    const Value* ISINTEGERP = ADD_PROC("integer?", isIntegerP);
    const Value* ISCHARP    = ADD_PROC("char?", isCharP);
    const Value* ISSTRINGP  = ADD_PROC("string?", isStringP);
    const Value* ISPAIRP    = ADD_PROC("pair?", isPairP);
    const Value* ISPROCP    = ADD_PROC("procedure?", isProcedureP);

    const Value* CHAR2INT   = ADD_PROC("char->integer", charToInteger);
    const Value* INT2CHAR   = ADD_PROC("integer->char", integerToChar);
    const Value* NUM2STR    = ADD_PROC("number->string", numberToString);
    const Value* STR2NUM    = ADD_PROC("string->number", stringToNumber);
    const Value* SYM2STR    = ADD_PROC("symbol->string", symbolToString);
    const Value* STR2SYM    = ADD_PROC("string->symbol", stringToSymbol);

    const Value* ADD        = ADD_PROC("+", addProc);
    const Value* SUB        = ADD_PROC("-", subProc);
    const Value* MUL        = ADD_PROC("*", mulProc);
    const Value* QUOTIENT   = ADD_PROC("quotient", quotientProc);
    const Value* REMAINDER  = ADD_PROC("remainder", remainderProc);
    const Value* ISNUMEQUAL = ADD_PROC("=", isNumberEqualProc);
    const Value* ISLESSTHAN = ADD_PROC("<", isLessThanProc);
    const Value* ISGREATERTHAN = ADD_PROC(">", isGreaterThanProc);
    const Value* CONS       = ADD_PROC("cons", consProc);
    const Value* CAR        = ADD_PROC("car" , carProc);
    const Value* CDR        = ADD_PROC("cdr" , cdrProc);
    const Value* SETCAR     = ADD_PROC("set-car!", setCarProc);
    const Value* SETCDR     = ADD_PROC("set-cdr!", setCdrProc);
    const Value* LIST       = ADD_PROC("list", listProc);
    const Value* EQ         = ADD_PROC("eq?", isEqProc);

};


#endif //KAT_KVM_H
