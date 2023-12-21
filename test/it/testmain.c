
#include "test.h"

#include <stdio.h>
#include <assert.h>
#include <CUnit/Console.h>
#include <CUnit/CUnit.h>
#include <CUnit/TestDB.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>


extern int test_mqtt_client_suite();
extern int test_mqtt_net_suite();
extern int test_register_suite();
extern int test_data_point_suite();
extern int test_dfile_suite();
extern int test_public_interface_suite();
extern int test_multithread_suite();

int main()
{
     //initialize
    if( CUE_SUCCESS != CU_initialize_registry())
    {
        return CU_get_error();
    }

    assert(NULL != CU_get_registry());

    assert(!CU_is_test_running());

    //add test case
    if (test_mqtt_client_suite()            != 0
        || test_mqtt_net_suite()            !=  0
        || test_public_interface_suite()    !=  0
        || test_multithread_suite() != 0
        )
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    //console mode
    CU_console_run_tests();

    /*** auto xml mode ********/
    // CU_set_output_filename("SDK");
    // CU_list_tests_to_file();
    // CU_automated_run_tests();
    /***********************************/

    /*basic mode*/
    //CU_basic_set_mode(CU_BRM_VERBOSE);
    //CU_basic_run_tests();

    CU_cleanup_registry();

    return 0;
}
