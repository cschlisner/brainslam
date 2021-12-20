// Generated by Bisonc++ V4.01.00 on Fri, 01 Mar 2013 13:22:26 +0100

#ifndef CCParser_h_included
#define CCParser_h_included

// $insert baseclass
#include "ccparserbase.h"
#include <vector>
#include <map>
#include <utility>
#include <stack>
#include <fstream>
#include <sstream>
#include <tuple>
#include "../preprocessor/ppparser.h"
#include "scanner/ccscanner.h"

// $insert namespace-open
namespace Compiler
{

#undef Parser
class Parser: public ParserBase
{
    typedef std::vector<std::string> Memory;
    // Static Data, shared between all Parser instances
    
    static Memory                              s_memory;
    static int                                 s_idx;
    static bool                                s_initialized;
    static std::vector<std::string>            s_functionVec;
    static std::map<int, std::pair<int, int>>  s_pointers;     // Holds the indices that point to other memory: idx, #elements
    static std::map<int, int>                  s_pointed;      // Indices of memory (supposedly) being pointed to and their number of elements

    static std::string const s_tmpId;       // freed at ';'
    static std::string const s_stcId;       // freed at '}'
    static std::string const s_refId;       // freed when not referenced to (anymore)
    static std::string const s_retId;       // freed at the end of a function

    static size_t const MAX_ARRAY_SIZE;
    
    Preprocessor::Parser const          &d_preprocessor;
    Scanner                             d_scanner;
    Preprocessor::Function              d_function;
    std::ostream                        &d_out;
    std::istringstream                  *d_streamPtr;
    
    std::stack<std::vector<int>>        d_stack;        // Holds all variables, local to if/for
    
    public:
        Parser(Preprocessor::Parser const &preprocessor, 
               std::string const &funName, 
               std::ostream &out, 
               std::vector<int> const &args = std::vector<int>());
        
        ~Parser();        
        int parse();
        static void init(size_t maxArraySize = 30000);

    private:
        void error(char const *msg);    // called on (syntax) errors
        int lex();                      // returns the next token from the
                                        // lexical scanner. 
        void print();                   // use, e.g., d_token, d_loc

    // support functions for parse():
        void executeAction(int ruleNr);
        void errorRecovery();
        int lookup(bool recovery);
        void nextToken();
        void print__();

    // Compiler members
        void movePtr(int idx);
        void setValue(int val);

        int printc(int idx);
        int printd(int idx);
        int prints(int idx);
        int scan(int idx);
        int assign(int idx1, int idx2);
        int assignFromPointer(int idx1, int idx2);
        int assign(int value);
        int assignToElement(int var, int offset, int val);
        int arrayValue(int idx1, int idx2);
        int add(int idx1, int idx2);
        int subtract(int idx1, int idx2);
        int multiply(int idx1, int idx2);
        int divide(int idx1, int idx2);
        int modulo(int idx1, int idx2);
        
        int addTo(int idx1, int idx2);
        int subtractFrom(int idx1, int idx2);
        int multiplyBy(int idx1, int idx2);
        int divideBy(int idx1, int idx2);
        int moduloBy(int idx1, int idx2);
        
        int addToElement(int var, int offset, int val);
        int subtractFromElement(int var, int offset, int val);
        int multiplyElement(int var, int offset, int val);
        int divideElement(int var, int offset, int val);
        int moduloElement(int var, int offset, int val);
        
        int lt(int idx1, int idx2);
        int gt(int idx1, int idx2);
        int le(int idx1, int idx2);
        int ge(int idx1, int idx2);
        int eq(int idx1, int idx2);
        int ne(int idx1, int idx2);
        
        int logicAnd(int idx1, int idx2);
        int logicOr(int idx1, int idx2);
        int logicNot(int idx);

        int call(std::string const &funName, std::vector<int> const &args);
        
        void startIf(int idx);
        void startElse();
        void stopIf();
        void stopIfElse();
        void startFor(int var, int start, int step, int stop);
        void stopFor();
        
    // Helper functions
        void collectGarbage();
        int findFreeMemory(int size = 1);
        int getTemp(int size = 1);
        int getFlag();
        int allocate(std::string const &ident);
        int allocString(std::string const &str);
        int allocArray(std::vector<int> const &list);
        int allocArray(int size, int val = 0);
        void popStack();
        void clear(int idx);
        bool isPointer(int idx);      
        std::string variable(std::string const &var);
        int getReturnValue();
};

// $insert namespace-close
}

#endif