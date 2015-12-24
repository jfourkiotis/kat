//
// Created by John Fourkiotis on 17/11/15.
//

#ifndef KAT_KVM_H
#define KAT_KVM_H

#include <string>
#include <unordered_map>
#include <iostream>

#include "kgc.h"
#include "kvalue.h"

class Value;

class Kvm {
public:
    Kvm();
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
    bool isLet(const Value *v);
    bool isAnd(const Value *v);
    bool isOr(const Value *v);
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
    const Value* letToFuncApp(const Value *v);
    const Value* letBindings(const Value *v);
    const Value* letBody(const Value *v);
    const Value* letParameters(const Value *v);
    const Value* letArguments(const Value *v);
    const Value* bindingsArguments(const Value *v);
    const Value* bindingsParameters(const Value *v);
    const Value* bindingArgument(const Value *v);
    const Value* bindingParameter(const Value *v);
    const Value* applyOperator(const Value *arguments);
    const Value* prepareApplyOperands(const Value *arguments);
    const Value* applyOperands(const Value *arguments);


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
    const Value* makeFuncApplication(const Value *op, const Value *operands);
    const Value* makeString(const std::string& str);
    const Value* makeCell(const Value *first, const Value* second);
    const Value* makeSymbol(const std::string& str);
    const Value* makeBool(bool condition);
    const Value* makeBegin(const Value *v);
    const Value* makeEofObject();
    const Value* makeIf(const Value *pred, const Value *conseq, const Value *alternate);
    const Value* makeInputPort(std::unique_ptr<std::ifstream> input);
    const Value* makeOutputPort(std::unique_ptr<std::ofstream> output);
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
    const Value* andTests(const Value *v);
    const Value* orTests(const Value *v);
    const Value* makeEnvironment();
    const Value* evalExpression(const Value *v);
    const Value* evalEnvironment(const Value *arguments);
    void populateEnvironment(Value *env);

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
    static const Value* applyProc(Kvm *vm, const Value *args);
    static const Value* interactionEnvironmentProc(Kvm *vm, const Value *args);
    static const Value* nullEnvironmentProc(Kvm *vm, const Value *args);
    static const Value* environmentProc(Kvm *vm, const Value *args);
    static const Value* evalProc(Kvm *vm, const Value *args);
    static const Value* loadProc(Kvm *vm, const Value *args);
    static const Value* openInputPortProc(Kvm *vm, const Value *args);
    static const Value* closeInputPortProc(Kvm *vm, const Value *args);
    static const Value* isInputPortProc(Kvm *vm, const Value *args);

    static const Value* openOutputPortProc(Kvm *vm, const Value *args);
    static const Value* closeOutputPortProc(Kvm *vm, const Value *args);
    static const Value* isOutputPortProc(Kvm *vm, const Value *args);

    static const Value* isEofObjectProc(Kvm *vm, const Value *args);
    static const Value* errorProc(Kvm *vm, const Value *args);
    static const Value* currentTimeMillisProc(Kvm *vm, const Value *args);

    static const Value* readProc(Kvm *vm, const Value *args);
    static const Value* readCharProc(Kvm *vm, const Value *args);
    static const Value* peekCharProc(Kvm *vm, const Value *args);
    static const Value* writeCharProc(Kvm *vm, const Value *args);
    static const Value* writeProc(Kvm *vm, const Value *args);


    std::unordered_map<std::string, const Value *> interned_strings;
    std::unordered_map<std::string, const Value *> symbols;

    const Value* NIL   ;
    const Value* FALSE ;
    const Value* TRUE  ;
    const Value* QUOTE ;
    const Value* DEFINE;
    const Value* SET   ;
    const Value* OK    ;
    const Value* IF    ;
    const Value* LAMBDA;
    const Value* BEGIN ;
    const Value* COND  ;
    const Value* ELSE  ;
    const Value* LET   ;
    const Value* AND   ;
    const Value* OR    ;
    const Value* EOFOBJ;
    const Value* EMPTY_ENV ;
    const Value* GLOBAL_ENV;
    
    void initialize();
    void addEnvProc(Value *env, const char *schemeName, const Value *(*proc)(Kvm *, const Value *));
    
    Kgc gc_;

};


#endif //KAT_KVM_H
