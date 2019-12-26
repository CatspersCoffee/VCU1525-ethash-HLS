#include<stdio.h>
#include<math.h>
#include <errno.h>

#include <fstream>
#include <string>
#include <chrono>
#include <cmath>
#include "sdx_cppKernel_top.h" 
#ifdef LINUX_BUILD
#include "pcie_memio.h" 
#include "srai_accel_utils.h" 
#else
#include "pcie_memio_winx.h"
#include "srai_accel_utils_winx.h" 
#endif
#define ZERO_f 1.0e-4
#define ONE_GIG (1024UL*1024UL*1024UL)
using namespace std;











//------------------------------------------------------------------------------
void load_dag(ethash_full* _dag_mem, uint64_t _dag_size_in_bytes){

    uint64_t some_word = 0x00;
    int node_double_words = 8;
    uint64_t nodes_in_dag = _dag_size_in_bytes / 64;

    cout << "nodes_in_dag  =  " << nodes_in_dag << endl;

    for (uint64_t i = 0 ; i < nodes_in_dag; i++) {
        for (int j = 0 ; j < node_double_words; j++) { 
            //_dag_mem->data->double_words[j] = some_word;
            _dag_mem->double_words[j] = some_word;
        }
        some_word++;
        _dag_mem++;
    }

}

//------------------------------------------------------------------------------
void read_dag(){

    size_t size_dag = 1073739904U;

    streampos size;
    char* dag_memblock;

    ifstream file ("full-R23-0000000000000000", ios::in|ios::binary|ios::ate);
    if (file.is_open())
    {
        size = file.tellg();
        dag_memblock = new char [size];
        file.seekg (0, ios::beg);
        file.read (dag_memblock, size);
        file.close();

        cout << "the entire file content is in memory";

        
    }
    else cout << "Unable to open file";


    printf("\n");
    for (unsigned int index = 0; index < 64;index++) {
        printf ("%02x", dag_memblock[index]);
    }
    delete[] dag_memblock;
}

//------------------------------------------------------------------------------


void gen_test_data(srai_mem_conv_IN0 *a) {

    data_t temp[NUM_ELEMENTS_PER_SDX_DATA_BEAT];

    uint8_t some_byte = 0x00;
    
    for (int j = 0 ; j < NUMBER_OF_DATA_SETS; j++) {
        for (int i = 0 ; i < SDX_CU_LOCAL_IN_SIZE; i++) {
            some_byte = 0x00;
            for (unsigned int index = 0; index < NUM_ELEMENTS_PER_SDX_DATA_BEAT;index++) {
                a->my_data_t[index] = some_byte;
                some_byte++;
            }

            a++;
        }
    }
    



}

void print_test_data(srai_mem_conv_IN0 *a) {

printf("\n Input Test Data Set : \n");
for (int j = 0 ; j < NUMBER_OF_DATA_SETS; j++) {
    for (int i = 0 ; i < SDX_CU_LOCAL_IN_SIZE; i++) {
        for (unsigned int index = 0; index < NUM_ELEMENTS_PER_SDX_DATA_BEAT;index++) {
            printf ("\t%d|  \n", a->my_data_t[index]);
        }
        a++;
    }
}
printf ("\n");
printf ("-----------------------------------------------------\n");
}

int main(int argc, char** argv) {

    int compute_itn_count;
    time_t t;
    srand((unsigned) time(&t));
    double high_res_elapsed_time = 0.0f;
    double high_res_elapsed_time_HW = 0.0f;
    double high_res_elapsed_time_SW = 0.0f;
    chrono::high_resolution_clock::time_point start_t;
    chrono::high_resolution_clock::time_point stop_t;
    chrono::duration<double> elapsed_hi_res;


    uint32_t dbg_ker_count = 0;
    sdx_data_t *a_in_ptr;

    char *a_in_ptr_c_POSIX = NULL;
    char *y_out_ptr_c_POSIX = NULL;

    srai_mem_conv_IN0 *a_in_ptr_c;
    srai_mem_conv_OUT0 *y_out_ptr_c;
    srai_mem_conv_IN0 *a_in_head_c;
    srai_mem_conv_OUT0 *y_out_head_c;
    sdx_data_t *y_out_ptr;

    SysMon_temp_struct sys_temprature;
    bool RESULT_SUCESSFULL;
    kernel_execution_metric_struct kernel_execution_metric; 

    printf("\n\n\n-------------------------------------------------------------\n\n\n");
    sdx_data_t* dag_ptr;
    char *dag_ptr_c_POSIX = NULL;
    ethash_full* dag_ptr_c;
    ethash_full* dag_head_c;
    uint64_t size_dag = 1073739904U;
    posix_memalign((void **)&dag_ptr_c_POSIX, 64, size_dag + 64);
    dag_ptr_c = (ethash_full*)dag_ptr_c_POSIX;

    cout << "Load fake-DAG with some node values\n";
    load_dag(dag_ptr_c, size_dag);


    printf("-------------------------------------------------------------\n\n\n");




    cout << "Srai_ DBG NUMBER_OF_DATA_SETS  =  " << NUMBER_OF_DATA_SETS << endl;
    cout << "Srai_ DBG GLOBAL_DATA_IN_SIZE  =  " << GLOBAL_DATA_IN_SIZE << endl;
    cout << "Srai_ DBG GLOBAL_DATA_OUT_SIZE =  " << GLOBAL_DATA_OUT_SIZE << endl;
    if ((GLOBAL_DATA_IN_SIZE_BYTES > ONE_GIG) | (GLOBAL_DATA_OUT_SIZE_BYTES > ONE_GIG)) {
        cout << "Memory reguirement over 1GB .......... exiting\n";
        exit (1);
    }


    posix_memalign((void **)&a_in_ptr_c_POSIX, 4096, GLOBAL_DATA_IN_SIZE_BYTES + 4096);
    a_in_ptr_c = (srai_mem_conv_IN0 *)a_in_ptr_c_POSIX;
    posix_memalign((void **)&y_out_ptr_c_POSIX, 4096, GLOBAL_DATA_OUT_SIZE_BYTES + 4096);
    y_out_ptr_c = (srai_mem_conv_OUT0 *)y_out_ptr_c_POSIX;

    a_in_head_c = a_in_ptr_c;
    y_out_head_c = y_out_ptr_c;


    printf("-------------------------------------------------------------\n");
    printf("Create Test Data Set\n");
    printf("Note DATA_IN_SIZE (Input Memory size in bytes  ) = %d (%x)\n",(GLOBAL_DATA_IN_SIZE_BYTES),(GLOBAL_DATA_IN_SIZE_BYTES));
    printf("Note DATA_OUT_SIZE(Input Memory size in bytes  ) = %d (%x)\n",(GLOBAL_DATA_OUT_SIZE_BYTES),(GLOBAL_DATA_OUT_SIZE_BYTES));
    cout << "Size of data_t = " << sizeof(data_t) <<  " Bytes" << endl;
    cout << "Number of Input Operands =  " << NUMBER_OF_DATA_SETS*SDX_CU_LOCAL_IN_SIZE*NUM_ELEMENTS_PER_SDX_DATA_BEAT<< endl;
    cout << "Number of Output Operands = " << NUMBER_OF_DATA_SETS*SDX_CU_LOCAL_OUT_SIZE*NUM_ELEMENTS_PER_SDX_DATA_BEAT<< endl;
    cout << "Size of srai_mem_conv_IN0 = " << sizeof(srai_mem_conv_IN0) <<  " Bytes" << endl;
    cout << "True Size (in Bytes) of Input Data  = " << sizeof(data_t)*NUMBER_OF_DATA_SETS*SDX_CU_LOCAL_IN_SIZE*NUM_ELEMENTS_PER_SDX_DATA_BEAT<< endl;
    cout << "Allocated Size (in Bytes) of a_in_ptr = " <<  GLOBAL_DATA_IN_SIZE_BYTES  << " | 0x"<< hex <<  GLOBAL_DATA_IN_SIZE_BYTES << endl;
    cout << dec;
    cout << "Allocated Size (in Bytes) of a_in_ptr_c = " << sizeof(srai_mem_conv_IN0)*NUMBER_OF_DATA_SETS*SDX_CU_LOCAL_IN_SIZE << " | 0x" << hex << sizeof(srai_mem_conv_IN0)*NUMBER_OF_DATA_SETS*SDX_CU_LOCAL_IN_SIZE << endl;
    cout << dec;
    printf("-------------------------------------------------------------\n\n\n");



    //Fill ddr4_Memory wr_data_buffer
    cout << "Initializing Memory with InputA args\n";

    gen_test_data(a_in_ptr_c);

    a_in_ptr = (sdx_data_t *)a_in_head_c;
    y_out_ptr = (sdx_data_t *)y_out_head_c;
    a_in_ptr_c = a_in_head_c;

    cout << "Memory Initialized with test Data\n";


    dag_ptr = (sdx_data_t *)dag_head_c;
    dag_ptr_c = dag_head_c;



#ifdef GPP_ONLY_FLOW  
    //sdx_cppKernel_top(a_in_ptr, y_out_ptr, (unsigned int)NUMBER_OF_DATA_SETS, &dbg_ker_count);
    sdx_cppKernel_top(a_in_ptr, y_out_ptr, dag_ptr, (unsigned int)NUMBER_OF_DATA_SETS, &dbg_ker_count);

#else
// Compile for SRAI custom HLS accelerator platform 
    string PR_binFile_name;

    if (argc != 2) {
        printf("usage: %s fpga_bin_file\n", argv[0]);
        return -1;
    }

    PR_binFile_name = argv[1];


    cout << "Initializing FPGA\n";
#ifdef LINUX_BUILD
    fpga_xDMA_linux *my_fpga_xDMA_ptr = new fpga_xDMA_linux;
#else
    fpga_xDMA_winX  *my_fpga_xDMA_ptr = new fpga_xDMA_winX;
#endif
    my_fpga_xDMA_ptr->fpga_xDMA_init();

    fpga_test_AXIL_LITE_8KSCRATCHPAD_BRAM (my_fpga_xDMA_ptr);
    //
    //DeIsolate before doing anyting on AXI Buses
    cout << "DeIsolate PR region \n";
    my_fpga_xDMA_ptr->fpga_poke(AXI_LITE_GPIO_BASE, DEISOLATE_NORTH_PR); 

    fpga_read_temprature(my_fpga_xDMA_ptr, &sys_temprature, 10);
    cout << "Current FPGA Die Temprature (deg C) =  " << sys_temprature.current_temp << endl;
    cout << "Current FPGA Max Die Temprature (deg C) =  " << sys_temprature.maximum_temp << endl;
    cout << "Current FPGA Min Die Temprature (deg C) =  " << sys_temprature.minimum_temp << endl;


    // xDMA Throughput testing 
    cout << "xDMA BandWidth test C0  : \n";
   fpga_PCIE_BANDWIDTH_test64(my_fpga_xDMA_ptr, AXI_MM_DDR4_C0, (char*)a_in_ptr, GLOBAL_DATA_OUT_SIZE_BYTES);
   cout << "..........................\n";
   cout << "xDMA BandWidth test C1  : \n";
   fpga_PCIE_BANDWIDTH_test64(my_fpga_xDMA_ptr, AXI_MM_DDR4_C1, (char*)a_in_ptr, GLOBAL_DATA_OUT_SIZE_BYTES);
   cout << "..........................\n";
   cout << "xDMA BandWidth test C2  : \n";
   fpga_PCIE_BANDWIDTH_test64(my_fpga_xDMA_ptr, AXI_MM_DDR4_C2, (char*)a_in_ptr, GLOBAL_DATA_OUT_SIZE_BYTES);
   cout << "..........................\n";
   cout << "xDMA BandWidth test C3: \n";
   fpga_PCIE_BANDWIDTH_test64(my_fpga_xDMA_ptr, AXI_MM_DDR4_C3, (char*)a_in_ptr, GLOBAL_DATA_OUT_SIZE_BYTES);
   cout << "..........................\n";
   
   

    cout << "Start HLS execution " << endl;
    cout << " ............... Programing PR clock ------------------ " << endl;
    fpga_PROGRAM_PR_CLOCK (my_fpga_xDMA_ptr, HW_Kernel_frequency);
    cout << " ....DONE ...... Programing PR clock ------------------ " << endl;

    // Program Partial Bit file 
    fpga_PROGRAM_NORTH_PR(my_fpga_xDMA_ptr, PR_binFile_name);
    cout << " ............... Done Programing PR Bitstream ------------------ " << endl;

    // Read the PR_HLS Control register to poll the Idle bit (bit 1) -----  
    fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_dag_C1, (char*)dag_ptr_c, size_dag);                         // copy dag data
    fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C1, (char*)a_in_ptr, (GLOBAL_DATA_IN_SIZE_BYTES));     // copy input data


    // Write to PR_HLS Address offset registers to set the location in Memory where Input Data and Output results are stored 
    fpga_run_NORTH_PR64(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C1, AXI_MM_DDR4_results_C1, (NUMBER_OF_DATA_SETS));

    start_t = chrono::high_resolution_clock::now();
    compute_itn_count = fpga_check_compute_done_NORTH_PR(my_fpga_xDMA_ptr);
    stop_t = chrono::high_resolution_clock::now();
    cout << "compute_itn_count = " << compute_itn_count << endl;

    // Read Results from DDR4 output (results) area 
    fpga_xfer_data_from_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_results_C1, (char*)y_out_ptr, (GLOBAL_DATA_OUT_SIZE_BYTES));

    elapsed_hi_res = stop_t - start_t ;
    high_res_elapsed_time = elapsed_hi_res.count();
    high_res_elapsed_time_HW = high_res_elapsed_time;
    cout << "HLS Execution time =  " <<  high_res_elapsed_time_HW << "s\n";
    cout << "HLS THroughput =  " <<  (GLOBAL_DATA_OUT_SIZE_BYTES/high_res_elapsed_time_HW) << " Bytes/s\n";

    fpga_get_Kernel_execution_time (my_fpga_xDMA_ptr, HW_Kernel_frequency, &kernel_execution_metric);
    cout << "KERNEL_DATASET =  " <<  dec << (kernel_execution_metric.KERNEL_DATASET) << " \n";
    cout << "KERNEL_CLOCK_COUNT =  " <<  dec << (kernel_execution_metric.KERNEL_CLOCK_COUNT) << " \n";
    cout << "KERNEL_Execution_time (sec) =  " <<  dec << (kernel_execution_metric.KERNEL_EXECUTION_TIME) << " \n";

    fpga_clean(my_fpga_xDMA_ptr);
#endif
    int MAX_ITERATION_to_print = 1;


    cout << "Verifying results ..............\n";
    high_res_elapsed_time =  0.0f;

    for (int j = 0 ; j < NUMBER_OF_DATA_SETS; j++) {
    data_t fn_in_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT];  // 16
    data_t fn_out_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT]; // 16

        for (int i = 0 ; i < SDX_CU_LOCAL_IN_SIZE; i++) {   // 16

            for (unsigned int k = 0 ; k < (NUM_ELEMENTS_PER_SDX_DATA_BEAT); k++) {
                fn_in_arg0[k] = a_in_ptr_c->my_data_t[k];
            }
            a_in_ptr_c++;
            for (unsigned int k = 0 ; k < (NUM_ELEMENTS_PER_SDX_DATA_BEAT); k++) {
                fn_out_arg0[k] = y_out_ptr_c->my_data_t[k];
            }
            y_out_ptr_c++;

            for (unsigned int index = 0; index < (NUM_ELEMENTS_PER_SDX_DATA_BEAT);index++) {
                if ( (i == 0) & (j == 0)) { 

                    printf ("Index[%d] = %04x \n", index, (fn_out_arg0[index]));
                }

            }
        }
    }


    printf (" ------------   End  ----------------------------------------------------------------------------------------\n");


    // ------------ Clean -----------------------

    free(a_in_ptr_c_POSIX);
    free(y_out_ptr_c_POSIX);
    free(dag_ptr_c_POSIX);

    return 0;
}
