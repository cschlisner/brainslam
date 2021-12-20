#include "ccparser.ih"

Parser::Memory                      Parser::s_memory(0);
int                                 Parser::s_idx = 0;
bool                                Parser::s_initialized = false;
std::vector<std::string>            Parser::s_functionVec;
std::string const                   Parser::s_tmpId = "__temp__";         // freed at ';'
std::string const                   Parser::s_stcId = "__stack__";        // freed at '}'
std::string const                   Parser::s_refId = "__refd__";         // freed when not referenced to (anymore)
std::map<int, std::pair<int, int>>  Parser::s_pointers;     // Holds the indices that point to other memory: idx, #elements
std::map<int, int>                  Parser::s_pointed;      // Indices of memory (supposedly) being pointed to and their number of elements
size_t const                        Parser::MAX_ARRAY_SIZE = 256;

void Parser::init(size_t memorySize)
{
    if (not s_initialized)
    {
        s_memory.resize(memorySize);
        s_initialized = true;
    }
    else
        throw std::string("Warning: multiple calles of Parser::init() ignored.");
}