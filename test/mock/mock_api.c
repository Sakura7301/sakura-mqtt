#include "mock_api.h"


#include "stub.h"
#include "addr_any.h"

Stub stub;
AddrAny any("libsakura_sdk_core.so");


/*
Function description:

mock non static function:
     char* func_name:      NULL
     char* func_addr:      function addr
     char* func_addr_mock: mock function addr
     
mock static function:
     char* func_name:      static function name
     char* func_addr:      NULL
     char* func_addr_mock: mock function addr

return:
     0: successful
    -1: fail


*/
int set_mock(char* func_name, char* func_addr, char* func_addr_mock)
{
    int ret = 0;
    std::map<std::string, void*> result;

    if(func_addr)
    {
        stub.save_func_addr_src(func_addr);
        stub.set(func_addr, func_addr_mock);
        return 0;
    }
    
    ret = any.get_local_func_addr_symtab(func_name, result);
    if(ret == 0) 
    {
        printf("%s error\n", func_name);
        exit(-1);
        return -1;
    }

    std::map<std::string, void*>::iterator it;
    for(it = result.begin(); it != result.end(); ++it)
    {
        if(it->first == func_name)
        {   
            stub.save_func_addr_src(it->second);
            stub.set(it->second, func_addr_mock);  
            return 0;
        }
    }
    
    return -1;
}
/*
Function description:
    char* func: function address, the default value is NULL
*/
void reset_mock(char* func)
{
    stub.reset(func);
}

void reset_mock2(char* func)
{
    stub.reset2(func);
}
