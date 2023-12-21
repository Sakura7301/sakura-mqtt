#ifndef __MOCK_API_H__
#define __MOCK_API_H__
#include <cstddef>



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
int set_mock(char* func_name, char* func_addr, char* func_addr_mock);
/*
Function description:
    char* func: function address, the default value is NULL
*/
void reset_mock(char* func = NULL);
void reset_mock2(char* func = NULL);

#endif
