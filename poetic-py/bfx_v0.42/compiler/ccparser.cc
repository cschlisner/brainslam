#include "ccparser.ih"

Parser::Parser(Preprocessor::Parser const &preprocessor, string const &funName, ostream &out, vector<int> const &args)
:
    d_preprocessor(preprocessor),
    d_function(d_preprocessor.function(funName)),
    d_out(out),
    d_streamPtr(0)
{
    if (!s_initialized)
        throw string("Compiler::Parser class not initialized. Call Compiler::Parser::init() first.");
    
    // 0. Increment function depth counter
    s_functionVec.push_back(funName);
    
    // 1. Check if arguments match
    if (args.size() != d_function.args.size())
        throw string("Argument mismatch.");
    
    // 2. Allocate and copy the arguments into the function-scope
    for (size_t idx = 0; idx != args.size(); ++idx)
        assign(allocate(d_function.args[idx]), args[idx]);
    
    // 3. Let the scanner switch to the function-body-stream
    d_streamPtr = new istringstream(d_function.body);
    d_scanner.switchStreams(*d_streamPtr);  
}

Parser::~Parser()
{
    // Free all variables with this function-prefix
    for (size_t idx = 0; idx != s_memory.size(); ++idx)
        if (s_memory[idx].find(d_function.name) != string::npos)
            clear(idx);
    
    s_functionVec.pop_back();
    delete d_streamPtr;
    d_out << flush;
}

int Parser::getReturnValue()
{
    int returnAddress = 0;

    if (d_function.ret != "__void__")
    {
        returnAddress = allocate(d_function.ret);
        s_memory[returnAddress] = s_tmpId;
    }
    
    return returnAddress;
}

int Parser::assign(int idx1, int idx2)
{
    if (isPointer(idx2))
        return assignFromPointer(idx1, idx2);
    
    // not a pointer -> make sure idx1 is not listed as a pointer anymore
    s_pointers.erase(idx1);
    
    // Assign!
    int tmp = getTemp();
    
    // Step 1: reset the value of idx1 to 0 and move the value at idx2 to tmp
    movePtr(idx1);
    setValue(0);   
    movePtr(idx2);
    d_out << "[-";
    movePtr(tmp); 
    d_out << "+";                    
    movePtr(idx2);
    d_out << "]";                    
    
    // By now, the values at idx1 and idx2 are both zero -> move the value from tmp, back to BOTH idx1 and idx2
    movePtr(tmp);
    d_out << "[-";
    movePtr(idx2);
    d_out <<  "+";
    movePtr(idx1);
    d_out << "+";
    movePtr(tmp);
    d_out << "]";
    
    return idx1;
}

int Parser::assign(int value)                   // assign an rvalue (e.g. 4) to a temporary memory location
{
    int idx = getTemp();
    movePtr(idx);
    setValue(value);
    
    return idx;    
}

int Parser::assignFromPointer(int idx1, int idx2)
{
    // Check if idx2 is a temporary pointer. If so, its content can be MOVED
    if (s_memory[idx2] == s_tmpId)
    {
        s_pointers[idx1] = s_pointers[idx2];    // the pointer at idx1 now points to whatever idx2 was pointing to
        s_pointers.erase(idx2);                 // the temporary pointer idx2 no longer points to anything
        return idx1;
    }
    
    // idx2 is not a temporary -> idx1 must point to a COPY of what idx2 is pointing to
    int pos = s_pointers[idx2].first;
    int len = s_pointers[idx2].second;
    
    int cpy = findFreeMemory(len);      // find a free memory-block of the same size
    for (int el = 0; el != len; ++el)   // copy each element to the new block
    {
        s_memory[cpy + el] = s_memory[pos + el];    // same identifier (e.g. __str__)
        assign(cpy + el, pos + el);                 // generate brainfuck code to copy the data
    }
    
    s_pointers[idx1] = pair<int, int>(cpy, len);    // mark this index as a pointer
    s_pointed[cpy] = len;
    
    return idx1;
}

int Parser::assignToElement(int var, int offset, int val)
{
    if (!isPointer(var))
        throw string("Error: indexed variable is not an array or string.");   
        
    int arr = s_pointers[var].first;
    int buf = getTemp(MAX_ARRAY_SIZE + 2);  // may need 2 extra cells if size == MAX_ARRAY_SIZE
    assign(buf, offset);
    assign(buf + 1, buf);
    assign(buf + 2, val);
    
    int dist = buf - arr;
    string arr2buf(ABS(dist), (dist > 0 ? '>' : '<'));
    string buf2arr(ABS(dist), (dist > 0 ? '<' : '>'));
    
    movePtr(buf);                                               // Pointer is now at buf (s_idx == buf)
    d_out << "[>>[->+<]<[->+<]<[->+<]>-]";                      // move the right (unknown) amount to the right in the buffer
    d_out << buf2arr << "[-]" << arr2buf;                       // set the value in the array to 0
    d_out << ">>[-<<" << buf2arr << "+" << arr2buf << ">>]<";   // move the value into the buffer
    d_out << "[[-<+>]<-]<";                                     // move back to the start of the buffer

    for (size_t idx = 0; idx != MAX_ARRAY_SIZE + 2; ++idx)
        clear(buf + idx);   // free all buffer elements 
        
    return val;
}

int Parser::addToElement(int var, int offset, int val)
{
    if (!isPointer(var))
        throw string("Error: indexed variable is not an array or string.");   
        
    int cpy = arrayValue(var, offset);
    addTo(cpy, val);
    assignToElement(var, offset, cpy);

    return cpy;
}

int Parser::subtractFromElement(int var, int offset, int val)
{
    if (!isPointer(var))
        throw string("Error: indexed variable is not an array or string.");   
        
    int cpy = arrayValue(var, offset);
    subtractFrom(cpy, val);
    assignToElement(var, offset, cpy);

    return cpy;
}


int Parser::multiplyElement(int var, int offset, int val)
{
    if (!isPointer(var))
        throw string("Error: indexed variable is not an array or string.");   
        
    int cpy = arrayValue(var, offset);
    multiplyBy(cpy, val);
    assignToElement(var, offset, cpy);

    return cpy;
}

int Parser::divideElement(int var, int offset, int val)
{
    if (!isPointer(var))
        throw string("Error: indexed variable is not an array or string.");   
        
    int cpy = arrayValue(var, offset);
    divideBy(cpy, val);
    assignToElement(var, offset, cpy);

    return cpy;
}

int Parser::moduloElement(int var, int offset, int val)
{
    if (!isPointer(var))
        throw string("Error: indexed variable is not an array or string.");   
        
    int cpy = arrayValue(var, offset);
    moduloBy(cpy, val);
    assignToElement(var, offset, cpy);

    return cpy;
}

int Parser::add(int idx1, int idx2)
{
    int tmp = getTemp();
    assign(tmp, idx1);
    addTo(tmp, idx2);
    return tmp;
}

int Parser::subtract(int idx1, int idx2) 
{
    int tmp = getTemp();
    assign(tmp, idx1);
    subtractFrom(tmp, idx2);
    return tmp;
}

int Parser::multiply(int idx1, int idx2)
{
    int tmp = getTemp();
    assign(tmp, idx1);
    multiplyBy(tmp, idx2);
    return tmp;
}

int Parser::divide(int idx1, int idx2)
{
    int tmp = getTemp();
    assign(tmp, idx1);
    divideBy(tmp, idx2);
    return tmp;
}

int Parser::modulo(int idx1, int idx2)
{
    int tmp = getTemp();
    assign(tmp, idx1);
    moduloBy(tmp, idx2);
    return tmp;
}

int Parser::addTo(int idx1, int idx2)
{
    int tmp = getTemp();
    assign(tmp, idx2);
    
    movePtr(tmp);
    d_out << "[-";
    movePtr(idx1);
    d_out << "+";
    movePtr(tmp);
    d_out << "]";
    
    return idx1;
}

int Parser::subtractFrom(int idx1, int idx2)
{
    int tmp = getTemp();
    assign(tmp, idx2);
    
    movePtr(tmp);
    d_out << "[-";
    movePtr(idx1);
    d_out << "-";
    movePtr(tmp);
    d_out << "]";
    
    return idx1;  
}

int Parser::multiplyBy(int idx1, int idx2)
{
    int tmp1 = getTemp();
    int tmp2 = getTemp();
    assign(tmp1, idx1);
    assign(tmp2, idx2);
    
    movePtr(idx1);
    d_out << "[-]";
    movePtr(tmp2);
    d_out << "[-";
    addTo(idx1, tmp1);
    movePtr(tmp2);
    d_out << "]";
    
    return idx1;    
}

int Parser::divideBy(int idx1, int idx2)
{
    int cpy = getTemp();
    int div = getTemp();
    
    // Keep subtracting the value at idx2 from a copy of idx1, until it's zero
    assign(cpy, idx1);
    movePtr(cpy);
    d_out << "[";
    subtractFrom(cpy, idx2);
    movePtr(div);
    d_out << "+";
    movePtr(cpy);
    d_out << "]"; 
    
    // div now holds the divider (could be off by 1), calculate the remainder
    int rem = multiply(div, idx2);
    subtractFrom(rem, idx1);
    
    // If the remainder is > 0, the divider should be decremented
    int flag = getTemp();
    movePtr(rem);
    d_out << "[[-]";
    movePtr(flag);
    setValue(1);
    movePtr(rem);
    d_out << "]";
    
    // The flag is now 1 if the remainder is > 0
    subtractFrom(div, flag);
    assign(idx1, div);
    
    return idx1;
}

int Parser::moduloBy(int idx1, int idx2)
{
    int tmp = getTemp();
    assign(tmp, idx1);
    divideBy(tmp, idx2);
    subtractFrom(idx1, multiply(tmp, idx2));
    
    return idx1;    
}

int Parser::lt(int idx1, int idx2)
{
    int tmp1 = getTemp();
    int tmp2 = getTemp();
    int ret = getTemp();
    
    assign(tmp1, idx1);
    assign(tmp2, idx2);
    
    movePtr(tmp1);
    d_out << "[-";
    movePtr(tmp2);
    d_out << "-";
    movePtr(tmp1);
    d_out << "]";
    
    movePtr(tmp2);
    d_out << "[[-]";
    movePtr(ret);
    setValue(1);
    movePtr(tmp2);
    d_out << "]";    

    return ret;
}

int Parser::gt(int idx1, int idx2)
{
    int tmp1 = getTemp();
    int tmp2 = getTemp();
    int ret = getTemp();
    
    assign(tmp1, idx2);
    assign(tmp2, idx1);
    
    movePtr(tmp1);
    d_out << "[-";
    movePtr(tmp2);
    d_out << "-";
    movePtr(tmp1);
    d_out << "]";
    
    movePtr(tmp2);
    d_out << "[[-]";
    movePtr(ret);
    setValue(1);
    movePtr(tmp2);
    d_out << "]";    

    return ret;
}

int Parser::eq(int idx1, int idx2)
{
    int less = lt(idx1, idx2);
    int more = gt(idx1, idx2);
    int flag = getTemp();
    int ret  = getTemp();
    
    movePtr(less);
    d_out << "[[-]";
    movePtr(flag);
    setValue(1);
    movePtr(less);
    d_out << "]";
    
    movePtr(more);
    d_out << "[[-]";
    movePtr(flag);
    setValue(1);
    movePtr(more);
    d_out << "]";
    
    movePtr(ret);
    setValue(1);
    subtractFrom(ret, flag);
    
    return ret;
}

int Parser::ne(int idx1, int idx2)
{
    int less = lt(idx1, idx2);
    int more = gt(idx1, idx2);
    int ret = getTemp();
    
    movePtr(less);
    d_out << "[[-]";
    movePtr(ret);
    setValue(1);
    movePtr(less);
    d_out << "]";
    
    movePtr(more);
    d_out << "[[-]";
    movePtr(ret);
    setValue(1);
    movePtr(more);
    d_out << "]";
    
    return ret;
}

int Parser::le(int idx1, int idx2)
{
    int less = lt(idx1, idx2);
    int same = eq(idx1, idx2);
    int ret = getTemp();
    
    movePtr(less);
    d_out << "[[-]";
    movePtr(ret);
    setValue(1);
    movePtr(less);
    d_out << "]";
    
    movePtr(same);
    d_out << "[[-]";
    movePtr(ret);
    setValue(1);
    movePtr(same);
    d_out << "]";
    
    return ret;
}

int Parser::ge(int idx1, int idx2)
{
    int more = gt(idx1, idx2);
    int same = eq(idx1, idx2);
    int ret = getTemp();
    
    movePtr(more);
    d_out << "[[-]";
    movePtr(ret);
    setValue(1);
    movePtr(more);
    d_out << "]";
    
    movePtr(same);
    d_out << "[[-]";
    movePtr(ret);
    setValue(1);
    movePtr(same);
    d_out << "]";
    
    return ret;
}

int Parser::logicAnd(int idx1, int idx2)
{
    int tmp1 = getTemp();
    int tmp2 = getTemp();
    int cpy1 = getTemp();
    int cpy2 = getTemp();
    assign(cpy1, idx1);
    assign(cpy2, idx2);
    
    movePtr(cpy1);
    d_out << "[[-]";        // set to 0 to make sure the body is executed only once
    movePtr(tmp1);
    setValue(1);
    movePtr(cpy1);
    d_out << "]";
    
    movePtr(cpy2);
    d_out << "[[-]";
    movePtr(tmp2);
    setValue(1);
    movePtr(cpy2);
    d_out << "]";
    
    multiplyBy(tmp1, tmp2);
    return tmp1;
}

int Parser::logicOr(int idx1, int idx2)
{
    int tmp1 = getTemp();
    int tmp2 = getTemp();
    int tmp3 = getTemp();
    int cpy1 = getTemp();
    int cpy2 = getTemp();
    assign(cpy1, idx1);
    assign(cpy2, idx2);
    
    movePtr(cpy1);
    d_out << "[[-]";        // set to 0 to make sure the body is executed only once
    movePtr(tmp1);
    setValue(1);
    movePtr(cpy1);
    d_out << "]";
    
    movePtr(cpy2);
    d_out << "[[-]";
    movePtr(tmp2);
    setValue(1);
    movePtr(cpy2);
    d_out << "]";
    
    addTo(tmp1, tmp2);
    movePtr(tmp1);
    d_out << "[[-]";
    movePtr(tmp3);
    setValue(1);
    movePtr(tmp1);
    d_out << "]";
    
    return tmp3;
}

int Parser::logicNot(int idx)
{
    int tmp = getTemp();
    int flg = getTemp();
    int ret = getTemp();
    
    assign(tmp, idx);
    movePtr(tmp);
    d_out << "[[-]";
    movePtr(flg);
    setValue(1);
    movePtr(tmp);
    d_out << "]";
    
    movePtr(ret);
    setValue(1);
    subtractFrom(ret, flg);
    return ret;
}

void Parser::startIf(int idx)
{
    // First set two flags, indicating whether it should enter the if or else (if available)
    int ifFlag = getFlag();
    int elFlag = getFlag();
    int tmp    = getTemp();
    
    d_stack.push({ifFlag, elFlag});     // push the flag adresses on the stack
    
    movePtr(elFlag);                    // ifFlag is already 0
    setValue(1);                        // set elseFlag to 1
    
    assign(tmp, idx);
    movePtr(tmp);
    d_out << "[[-]";                    // if the conditional is nonzero
    movePtr(ifFlag);                    // the if-flag will become 1
    setValue(1);
    movePtr(elFlag);                    // and the else-flag will become 0
    setValue(0);
    movePtr(tmp);
    d_out << "]";
    
    movePtr(ifFlag);
    d_out << "[-";    // all statements hereafter will be executed only of the ifFlag was nonzero (1)
}

void Parser::stopIf()
{
    int ifFlag = d_stack.top()[0];
    movePtr(ifFlag);
    d_out << "]";
}

void Parser::startElse()
{
    stopIf();
    int elFlag = d_stack.top()[1];
    
    movePtr(elFlag);
    d_out << "[-";
}

void Parser::stopIfElse()
{
    int elFlag = d_stack.top()[1];
    movePtr(elFlag);
    d_out << "]";
}

void Parser::startFor(int var, int start, int step_, int stop_)
{
    int step = getFlag();
    int stop = getFlag();
    int flag = getFlag();
    
    if (step_ == -1)
    {
        step_ = getTemp();
        movePtr(step_);
        setValue(1);
    }

    assign(step, step_);
    assign(stop, stop_);
    
    d_stack.push({var, step, stop, flag});
    
    assign(var, start);
    assign(flag, le(var, stop));
    movePtr(flag);
    d_out << "[";
}

void Parser::stopFor()
{
    int var = d_stack.top()[0];
    int step = d_stack.top()[1];
    int stop = d_stack.top()[2];
    int flag = d_stack.top()[3];
    
    addTo(var, step);
    assign(flag, le(var, stop));
    movePtr(flag);
    d_out << "]";
}

void Parser::popStack()
{
    vector<int> vars = d_stack.top();
    d_stack.pop();

    for (size_t idx = 0; idx != vars.size(); ++idx)
        clear(vars[idx]);
}

void Parser::clear(int idx)
{
    s_memory[idx] = string();
}

void Parser::collectGarbage()
{
    // Clear all temporaries from the memory
    for (size_t idx = 0; idx != s_memory.size(); ++idx)
    {
        if (s_memory[idx] == s_tmpId)
        {
            clear(idx);
            s_pointers.erase(idx);    // in case it was a temporary pointer, erase it
        }
    }
        
    // Now, delete the memory that is NOT being referenced anymore
    for (auto it1 = s_pointed.begin(); it1 != s_pointed.end(); ++it1)
    {
        int idx = it1->first;        // index that should be pointed to by one of the pointers
        bool referenced = false;
        for (auto it2 = s_pointers.begin(); it2 != s_pointers.end(); ++it2)
        {
            if (it2->second.first == idx)
            {
                referenced = true;
                break;
            }
        }
        
        if (!referenced)
        {
            size_t len = it1->second;
            for (size_t i = 0; i != len; ++i)
                clear(idx + i);
            
            s_pointed.erase(idx);          // not pointed to this index!
        }
    }
            
}

int Parser::allocate(std::string const &ident)
{
    string varName = variable(ident);
    
    // 1. Check of identifier already exists
    for (size_t idx = 0; idx != s_memory.size(); ++idx)
        if (s_memory[idx] == varName)
            return idx;
        
    // 2. Does not exist yet -> find empty location in memory and create the variable
    int idx = findFreeMemory();
    if (idx == -1)
        throw string("Out of memory!");
        
    s_memory[idx] = varName;
    return idx;    
}

int Parser::allocString(std::string const &str)
{
    int len = str.length();
    int ptr = getTemp();                // will point to the string
    int idx = getTemp(len + 1);         // also allocate \0
    
    // Build the string in memory
    for (int jdx = 0; jdx != len; ++jdx)
    {
        movePtr(idx + jdx);
        setValue(str[jdx]);
        s_memory[idx + jdx] = s_refId;
    }
    
    // Set terminating \0 char
    movePtr(idx + len);
    setValue(0);
    s_memory[idx + len] = s_refId;

    // Set the pointer variables
    s_pointers[ptr] = pair<int, int>(idx, len + 1);
    s_pointed[idx] = len + 1;
    
    // Return the pointer
    return ptr;
}

int Parser::allocArray(vector<int> const &list)
{
    size_t numel = list.size();
    if (numel > MAX_ARRAY_SIZE)
    {
        cout << "Warning: array is bigger than the maximum size of " 
             << MAX_ARRAY_SIZE 
             << " elements: extra elements are ignored.";
        numel = MAX_ARRAY_SIZE;
    }
 
    int ptr = getTemp();
    int arr = getTemp(numel);
    
    for (size_t idx = 0; idx != numel; ++idx)
    {
        assign(arr + idx, list[idx]);
        s_memory[arr + idx] = s_refId;
    }
    
    s_pointers[ptr] = pair<int, int>(arr, numel);
    s_pointed[arr] = numel;
    
    return ptr;
}

int Parser::allocArray(int numel, int val_)
{
    if (numel > (int)MAX_ARRAY_SIZE)
    {
        cout << "Warning: array is bigger than the maximum size of " << MAX_ARRAY_SIZE << " elements: extra elements are ignored.";
        numel = MAX_ARRAY_SIZE;
    }
    
    int ptr = getTemp();
    int arr = getTemp(numel);
    int val = assign(val_);
    
    for (int idx = 0; idx != numel; ++idx)
    {
        assign(arr + idx, val);
        s_memory[arr + idx] = s_refId;
    }
    
    s_pointers[ptr] = pair<int, int>(arr, numel);
    s_pointed[arr] = numel;
    return ptr;
}

int Parser::arrayValue(int idx1, int idx2)
{
    if (!isPointer(idx1))
        throw string("Error: indexed variable is not an array or string.");

    int arr = s_pointers[idx1].first;
    int buf = getTemp(MAX_ARRAY_SIZE + 2);  // may need 2 extra cells if size == MAX_ARRAY_SIZE
    assign(buf, idx2);
    assign(buf + 1, buf);
    
    int dist = buf - arr;
    string arr2buf(ABS(dist), (dist > 0 ? '>' : '<'));
    string buf2arr(ABS(dist), (dist > 0 ? '<' : '>'));
    
    movePtr(buf);                       // Pointer is now at buf (d_idx == buf)
    d_out << "[>[->+<]<[->+<]>-]";      // move the right (unknown) amount to the right in the buffer 
    
    d_out << buf2arr << "[-" << arr2buf << ">>+<<" << buf2arr << "]";       // move the value to an empty location in the buffer
    d_out << arr2buf << ">>[-<<+" << buf2arr << "+" << arr2buf << ">>]<";   // move the value back, and leave a copy at buf
    
    d_out << "[<[-<+>]>[-<+>]<-]<";    // now the pointer is back at buf, and it brought the copied value along with it

    for (size_t idx = 1; idx != MAX_ARRAY_SIZE + 2; ++idx)
        clear(buf + idx);   // free all buffer elements except for the one holding the return value
        
    return buf;
}

int Parser::findFreeMemory(int size)
{
    for (size_t idx = 0; idx != s_memory.size(); ++idx)
    {
        if (s_memory[idx].empty())
        {
            int succes = true;
            for (int jdx = 1; jdx != size; ++jdx)
                if (!s_memory[idx + jdx].empty())
                {
                    succes = false;
                    break;
                }
                
            if (succes)
                return idx;
        }
    }
            
    return -1;
}

int Parser::getTemp(int size)
{
    int idx = findFreeMemory(size);
    if (idx == -1)
        throw string("Out of memory!");
    
    for (int jdx = 0; jdx != size; ++jdx)
    {
        s_memory[idx + jdx] = s_tmpId;
        movePtr(idx + jdx);
        setValue(0);
    }
    
    return idx;
}

int Parser::getFlag()
{
    int idx = findFreeMemory();
    if (idx == -1)
        throw string("Out of memory!");
    
    s_memory[idx] = s_stcId;
    movePtr(idx);
    setValue(0);
    return idx;
}


int Parser::printc(int idx)
{
    movePtr(idx);
    d_out << '.';
    return idx;
}

int Parser::printd(int idx)
{
    int cpy = getTemp();
    assign(cpy, idx);
    
    int hun = getTemp();
    int ten = getTemp();
    int aaa = getTemp();
    int c1 = getTemp();
    int c2 = getTemp();
    
    movePtr(hun);
    setValue(100);
    movePtr(ten);
    setValue(10);
    movePtr(aaa);
    setValue(48);
    
    assign(c1, divide(cpy, hun));
    moduloBy(cpy, hun);
    assign(c2, divide(cpy, ten));
    moduloBy(cpy, ten);
    
    addTo(c1, aaa);
    printc(c1);
    addTo(c2, aaa);
    printc(c2);
    addTo(cpy, aaa);
    printc(cpy);

    return idx;
}

int Parser::prints(int idx)
{
    // idx is a pointer to a string -> get the actual index
    int jdx = s_pointers[idx].first;
    int len = s_pointers[idx].second;
    
    // jdx is now the actual index of the string
    movePtr(jdx);
    d_out << "[.>]";
    s_idx += len - 1;      // pointer has moved over this distance
    
    return idx;
}

int Parser::scan(int idx)
{
    movePtr(idx);
    d_out << ',';
    return idx;
}

void Parser::movePtr(int idx)
{
    static char moveLeft = '<';
    static char moveRight = '>';
    
    int diff = idx - s_idx;
    char ch = diff < 0 ? moveLeft : moveRight;
    s_idx = idx;

    d_out << string(ABS(diff), ch);
}

void Parser::setValue(int val)
{
    if (val <= 10)
    {
       d_out << "[-]" << string(val, '+');
       return;
    }
    
    int tens = val / 10;
    int ones = val % 10;
    int idx = s_idx;
    d_out << "[-]";
    
    int count = findFreeMemory();
    s_memory[count] = s_tmpId;                           // can't use getTemp() here, it would call setValue -> infinite recursion
    movePtr(count);
    d_out << "[-]" << string(tens, '+');
    
    d_out << "[-";      
    movePtr(idx);
    d_out << string(10, '+');
    movePtr(count);
    d_out << "]";
    
    if (ones)
    {
        movePtr(idx);
        d_out << string(ones, '+');
    }
}

bool Parser::isPointer(int idx)
{
    return s_pointers.find(idx) != s_pointers.end();
}

int Parser::call(string const &funName, vector<int> const &args)
{
    if (find(s_functionVec.begin(), s_functionVec.end(), funName) != s_functionVec.end())
        throw string("Error: recursion is not supported.");
    
    // Create a new parser to parse this function
    Parser subParser(d_preprocessor, funName, d_out, args);
    subParser.parse();

    // Store the return value in tmp
    return subParser.getReturnValue();
    
    // when the sub-parser dies, it will free its local variables
}

std::string Parser::variable(std::string const &var)
{
    return string("__") + d_function.name + "__" + var;        // e.g. __fun__x
}
