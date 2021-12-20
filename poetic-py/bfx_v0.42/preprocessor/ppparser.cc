#include "ppparser.ih"

void Parser::addFunction(string const &retArg, 
                         string const &funName, 
                         vector<string> const &args, 
                         string const &body)
{
    Function function;
    function.ret  = retArg;
    function.name = funName;
    function.args = args;
    function.body = body;
    
    d_functions.push_back(function);
}

Function const &Parser::function(string const &funName) const
{
    for (size_t idx = 0; idx != d_functions.size(); ++idx)
        if (d_functions[idx].name == funName)
            return d_functions[idx];
    
    throw string("Function does not exist.");
}