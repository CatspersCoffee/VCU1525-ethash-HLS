#include<stdio.h>
#include<math.h>
#include <errno.h>

#include <fstream>
#include <string>
#include <chrono>
#include <cmath>


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <vector>
#include <algorithm>
//----------------------------------------------------


#include "sdx_cppKernel_top.h" 
#include "pcie_memio.h" 
#include "srai_accel_utils.h" 


#define ZERO_f 1.0e-4
#define ONE_GIG (1024UL*1024UL*1024UL)
using namespace std;




//------------------------------------------------------------------------------
// helper routines:

static char nibbleToChar(unsigned nibble)
{
	return (char) ((nibble >= 10 ? 'a'-10 : '0') + nibble);
}

static uint8_t charToNibble(char chr)
{
	if (chr >= '0' && chr <= '9')
	{
		return (uint8_t) (chr - '0');
	}
	if (chr >= 'a' && chr <= 'z')
	{
		return (uint8_t) (chr - 'a' + 10);
	}
	if (chr >= 'A' && chr <= 'Z')
	{
		return (uint8_t) (chr - 'A' + 10);
	}
	return 0;
}

static std::vector<uint8_t> hexStringToBytes(char const* str)
{
	std::vector<uint8_t> bytes(strlen(str) >> 1);
	for (unsigned i = 0; i != bytes.size(); ++i)
	{
		bytes[i] = charToNibble(str[i*2 | 0]) << 4;
		bytes[i] |= charToNibble(str[i*2 | 1]);
	}
	return bytes;
}

static std::string bytesToHexString(uint8_t const* bytes, unsigned size)
{
	std::string str;
	for (unsigned i = 0; i != size; ++i)
	{
		str += nibbleToChar(bytes[i] >> 4);
		str += nibbleToChar(bytes[i] & 0xf);
	}
	return str;
}
//------------------------------------------------------------------------------







//------------------------------------------------------------------------------

void load_header(INPUT_mem_t* _input, uint32_t _index_start){
    INPUT_mem_t* _input_head = _input;
    uint32_t index = _index_start;
    uint32_t header_hash[8];
    //memcpy(header_hash, hexStringToBytes("0000000100000002000000030000000400000005000000060000000700000008").data(), 32);
    header_hash[0] = 0x00000001;
    header_hash[1] = 0x00000002; 
    header_hash[2] = 0x00000003; 
    header_hash[3] = 0x00000004; 
    header_hash[4] = 0x00000005; 
    header_hash[5] = 0x00000006; 
    header_hash[6] = 0x00000007; 
    header_hash[7] = 0x00000008; 
    /*
    // 07613b4c 05fb8afe a7327f25 20c74237 33a0a0be 093d5571 0abda329 acd38fd8
    header_hash[0] = 0x07613b4c;
    header_hash[1] = 0x05fb8afe; 
    header_hash[2] = 0x00000003; 
    header_hash[3] = 0x00000004; 
    header_hash[4] = 0x00000005; 
    header_hash[5] = 0x00000006; 
    header_hash[6] = 0x00000007; 
    header_hash[7] = 0x00000008; 
    */

    uint8_t temp_byte = 0x00;
    uint32_t temp_word = 0x00000000;
    
    _input = _input_head + (index + 0);

    // write sdx_data_t for data [0] with just header has in MS 32 bytes.
    for (int i = 0 ; i < SDX_CU_LOCAL_IN_SIZE; i++) {   // 16 --> 16 * 32bits = 512bits
        if(i < 8){
            _input->my_data_t[i] = header_hash[i];
        } else {
            _input->my_data_t[i] = 0x00000000;
        }
    }

}

//------------------------------------------------------------------------------

void load_target(INPUT_mem_t* _input, uint32_t _index_start){
    INPUT_mem_t* _input_head = _input;
    uint32_t index = _index_start;
    uint32_t target[8];
    target[0] = 0xF0000000;
    target[1] = 0x00000000; 
    target[2] = 0x00000000; 
    target[3] = 0x00000000; 
    target[4] = 0x00000000; 
    target[5] = 0x00000000; 
    target[6] = 0x00000000; 
    target[7] = 0x00000000; 

    uint8_t temp_byte = 0x00;
    uint32_t temp_word = 0x00000000;
    
    _input = _input_head + (index + 0);

    // write sdx_data_t for data [0] with just header has in MS 32 bytes.
    for (int i = 0 ; i < SDX_CU_LOCAL_IN_SIZE; i++) {   // 16 --> 16 * 32bits = 512bits
        if(i < 8){
            _input->my_data_t[i] = target[i];
        } else {
            _input->my_data_t[i] = 0x00000000;
        }
    }

}

//------------------------------------------------------------------------------

void load_nonce(INPUT_mem_t* _input, uint64_t _start_nonce, uint64_t _end_nonce, uint64_t _CUs, uint32_t _index_start){
    INPUT_mem_t* _input_head = _input;
    uint32_t index = _index_start;
    uint32_t start_nonce[16];
    uint32_t end_nonce[16];
    uint64_t buffer, startforCU, endforCU;

    printf("\n _end_nonce  = %016llx --> dec %llu", _end_nonce, _end_nonce);

    uint64_t noncesperCU = ((_end_nonce - _start_nonce) / _CUs) + 0;
    printf("\n noncesperCU = %016llx --> dec %llu", noncesperCU, noncesperCU);

    for(uint32_t cu = 0; cu < _CUs; cu++){
        startforCU = _start_nonce + (noncesperCU * cu);
        if(cu == 0 ){
            startforCU = startforCU + 0;
        } else if ( cu != 0 || cu != (_CUs - 1)){
            startforCU = startforCU + (cu * 1);
        } else if (cu == (_CUs - 1)){
            startforCU = startforCU + (cu * 1) - 1;
        }
        endforCU = startforCU + noncesperCU;

        printf("\n startforCU[%u]  = %016llx --> dec %llu", cu, startforCU, startforCU);
        printf("\n endforCU[%u]    = %016llx --> dec %llu", cu, endforCU, endforCU);


        // get the start_nonce high 4 bytes:
        buffer = startforCU >> (8*4);
        buffer = buffer & 0x00000000FFFFFFFF;
        start_nonce[0] = (uint32_t)buffer;
        // get the start_nonce low 4 bytes:
        buffer = startforCU & 0x00000000FFFFFFFF;
        start_nonce[1] = (uint32_t)buffer;
        for( uint8_t i = 2; i < 16; i++){
            start_nonce[i] = 0x00000000; 
        }

        // get the end_nonce high 4 bytes:
        buffer = endforCU >> (8*4);
        buffer = buffer & 0x00000000FFFFFFFF;
        end_nonce[0] = (uint32_t)buffer;
        // get the end_nonce low 4 bytes:
        buffer = endforCU & 0x00000000FFFFFFFF;
        end_nonce[1] = (uint32_t)buffer;  
        for( uint8_t i = 2; i < 16; i++){
            end_nonce[i] = 0x00000000; 
        }  

        _input = _input_head + (index + ((cu*2) + 0));
        printf("\n _input forCU[%u]  nonce_HIGH index = %llu", cu, (index + ((cu*2) + 0)));
        for (int i = 0 ; i < SDX_CU_LOCAL_IN_SIZE; i++) {   // 16
                _input->my_data_t[i] = start_nonce[i];
        }
        _input = _input_head + (index + ((cu*2) + 1));
        printf("\n _input forCU[%u]  nonce_LOW index  = %llu", cu, (index + ((cu*2) + 1)));
        for (int i = 0 ; i < SDX_CU_LOCAL_IN_SIZE; i++) {   // 16
                _input->my_data_t[i] = end_nonce[i];
        }

    }
    

    printf("\n\n");
}

//------------------------------------------------------------------------------

void load_remaining_input_data(INPUT_mem_t* _input, uint32_t _index_start){
    INPUT_mem_t* _input_head = _input;
    uint32_t index = _index_start;
    _input = _input_head + (index + 0);
    // write sdx_data_t for data [3 through 15] with just zeros
    for (int j = 3 ; j < GLOBAL_DATA_IN_SIZE; j++) {        // how many sdx_data_t elements are in the input data set.
        for (int i = 0 ; i < SDX_CU_LOCAL_IN_SIZE; i++) {   // 16 --> 16 * 32bits = 512bits
            _input->my_data_t[i] = 0x00000000;
        }
        _input++;
    }
}

//------------------------------------------------------------------------------
void load_dag(ethash_full* _dag_mem, uint64_t _dag_size_in_bytes, INPUT_mem_t* _input, uint32_t _index_dag){
    INPUT_mem_t* _input_head = _input;

    uint64_t node_index = 0x00;
    int node_double_words = 8;
    uint64_t buffer;

    uint32_t nid[16];
    int node_bytes = 64;
    uint64_t nodes_in_dag = _dag_size_in_bytes / node_bytes;
    //cout << "nodes_in_dag  =  " << nodes_in_dag << endl;
    printf("\n nodes_in_dag = %016llx  --> dec %llu", nodes_in_dag, nodes_in_dag);

    uint32_t nfp[16];
    uint8_t page_size = 128;
    uint64_t num_full_pages = _dag_size_in_bytes / page_size;
    //cout << "nodes_in_dag  =  " << nodes_in_dag << endl;
    printf("\n num_full_pages = %016llx  --> dec %llu", num_full_pages, num_full_pages);


    // get the num_full_pages high 4 bytes:
    buffer = num_full_pages >> (8*4);
    buffer = buffer & 0x00000000FFFFFFFF;
    nfp[0] = (uint32_t)buffer;
    // get the num_full_pages low 4 bytes:
    buffer = num_full_pages & 0x00000000FFFFFFFF;
    nfp[1] = (uint32_t)buffer;  
    for( uint8_t i = 2; i < 16; i++){
        nfp[i] = 0x00000000; 
    }
    // write the number of nodes in dag to INPUT memory posistion:
    _input = _input_head + _index_dag;
    for (int i = 0 ; i < 16; i++) {   // 16
            _input->my_data_t[i] = nfp[i];
            //printf("\n nfp[%02d] = %016llx", i, nfp[i]);
    }



    /*
    for (uint64_t i = 0 ; i < 1; i++) {
        for (int j = 0 ; j < node_bytes; j++) { 
            _dag_mem->bytes[j] = node_hash[j];
        }
        _dag_mem++;
    }
    for (uint64_t i = 1 ; i < nodes_in_dag; i++) {
        for (int j = 0 ; j < node_double_words; j++) { 
            //_dag_mem->data->double_words[j] = some_word;
            _dag_mem->double_words[j] = some_word;
        }
        some_word++;
        _dag_mem++;
    }
    */


    for (uint64_t i = 0 ; i < nodes_in_dag; i++) {

        for(int d = 0; d < node_double_words; d++) {
            _dag_mem->double_words[d] = 0x00000000000000000000000000000000;
        }
        _dag_mem->double_words[0] = node_index;
        node_index++;
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


void gen_test_data(INPUT_mem_t *a) {

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


void print_test_data(INPUT_mem_t *a) {

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
    dag_head_c = dag_ptr_c;

    //cout << "Load fake-DAG with some node values\n";
    //load_dag(dag_ptr_c, size_dag);
    //dag_ptr = (sdx_data_t *)dag_head_c;
    //dag_ptr_c = dag_head_c;

    printf("-------------------------------------------------------------\n\n\n");

    sdx_data_t* a_in_ptr;
    sdx_data_t* y_out_ptr;

    char *input_ptr_c_POSIX = NULL;
    char *output_ptr_c_POSIX = NULL;

    INPUT_mem_t* input_ptr_c;
    OUTPUT_mem_t* output_ptr_c;
    INPUT_mem_t* input_head_c;
    OUTPUT_mem_t* output_head_c;


    posix_memalign((void **)&input_ptr_c_POSIX, 4096, GLOBAL_DATA_IN_SIZE_BYTES + 4096);
    input_ptr_c = (INPUT_mem_t *)input_ptr_c_POSIX;


    //posix_memalign((void **)&output_ptr_c_POSIX, 4096, GLOBAL_DATA_OUT_SIZE_BYTES + 4096);
    //output_ptr_c = (OUTPUT_mem_t *)output_ptr_c_POSIX;
    posix_memalign((void **)&output_ptr_c_POSIX, 64, size_dag + 64);
    output_ptr_c = (OUTPUT_mem_t *)output_ptr_c_POSIX;

    input_head_c = input_ptr_c;
    output_head_c = output_ptr_c;


    printf("\n-------------------------------------------------------------\n\n");
    //Fill ddr4_Memory wr_data_buffer
    cout << "Initializing Memory with Input args\n";

    cout << "Load fake-DAG with some node values\n";
    load_dag(dag_ptr_c, size_dag, input_ptr_c, INDEX_IN_num_full_pages);

    dag_ptr = (sdx_data_t *)dag_head_c;
    dag_ptr_c = dag_head_c;

    cout << "Load INPUT values\n";
    // header_hash      --> index 0
    // nonce values     --> index 1-32 (32 values total, 16 for each CU --> nonce_start[CU], nonce_end[CU])
    // remaining data   --> index 33+
    //gen_test_data(input_ptr_c);
    load_header(input_ptr_c, INDEX_IN_header);  
    load_target(input_ptr_c, INDEX_IN_target);  
    //load_nonce(input_ptr_c, 0x0000000000000001ULL, 0xFFFFFFFFFFFFFFFFULL, 0x4ULL, INDEX_IN_nonces);    // input_data_ptr, start_nonce, end_nonce, compute units
    //load_nonce(input_ptr_c, 0x0000000000000000ULL, 0x00000000FFFFFFFFULL, 0x4ULL, INDEX_IN_nonces);      // testing
    //load_nonce(input_ptr_c, 0x0000000000000000ULL, 0x0000000000000001ULL, 0x1ULL, , INDEX_IN_nonces);  //  testing
    load_nonce(input_ptr_c, 0x0000000000000001ULL, 0x0000000000000000ULL, 0x1ULL, INDEX_IN_nonces);  //  testing
    load_remaining_input_data(input_ptr_c, INDEX_IN_rem_dat);

    a_in_ptr = (sdx_data_t *)input_head_c;
    y_out_ptr = (sdx_data_t *)output_head_c;
    input_ptr_c = input_head_c;

    cout << "Memory Initialized with test Dataset and Input data\n";



    printf("-------------------------------------------------------------\n\n\n");

    cout << "print a few values from fake-DAG: \n";
    for (unsigned int index = 0; index < 32 ;index++) {
            printf ("dag Index[%d] = %08x \n", index, (dag_ptr_c->words[index]));
            if(index == 15){
                printf("----\n");
            }
    } 

    printf("-------------------------------------------------------------\n\n\n");

    cout << "print a few values from INPUT dataset: \n";
    for (unsigned int index = 0; index < 32 ;index++) {
            printf ("INPUT Index[%d] = %08x \n", index, (input_ptr_c->my_data_t[index]));
            if(index == 15){
                printf("----\n");
            }
    } 


    printf("-------------------------------------------------------------\n\n\n");
    cout << "NUMBER_OF_DATA_SETS  =  " << NUMBER_OF_DATA_SETS << endl;
    cout << "GLOBAL_DATA_IN_SIZE  =  " << GLOBAL_DATA_IN_SIZE << endl;
    cout << "GLOBAL_DATA_OUT_SIZE =  " << GLOBAL_DATA_OUT_SIZE << endl;
    if ((GLOBAL_DATA_IN_SIZE_BYTES > ONE_GIG) | (GLOBAL_DATA_OUT_SIZE_BYTES > ONE_GIG)) {
        cout << "Memory reguirement over 1GB .......... exiting\n";
        exit (1);
    }
    //printf("-------------------------------------------------------------\n");
    printf("\n\n");
    printf("Create Test Data Set\n");
    printf("Note DATA_IN_SIZE (Input Memory size in bytes  ) = %d (%x)\n",(GLOBAL_DATA_IN_SIZE_BYTES),(GLOBAL_DATA_IN_SIZE_BYTES));
    printf("Note DATA_OUT_SIZE(Input Memory size in bytes  ) = %d (%x)\n",(GLOBAL_DATA_OUT_SIZE_BYTES),(GLOBAL_DATA_OUT_SIZE_BYTES));
    cout << "Size of data_t = " << sizeof(data_t) <<  " Bytes" << endl;
    cout << "Number of Input Operands =  " << NUMBER_OF_DATA_SETS*SDX_CU_LOCAL_IN_SIZE*NUM_ELEMENTS_PER_SDX_DATA_BEAT<< endl;
    cout << "Number of Output Operands = " << NUMBER_OF_DATA_SETS*SDX_CU_LOCAL_OUT_SIZE*NUM_ELEMENTS_PER_SDX_DATA_BEAT<< endl;
    cout << "Size of INPUT_mem_t = " << sizeof(INPUT_mem_t) <<  " Bytes" << endl;
    cout << "True Size (in Bytes) of Input Data  = " << sizeof(data_t)*NUMBER_OF_DATA_SETS*SDX_CU_LOCAL_IN_SIZE*NUM_ELEMENTS_PER_SDX_DATA_BEAT<< endl;
    cout << "Allocated Size (in Bytes) of a_in_ptr = " <<  GLOBAL_DATA_IN_SIZE_BYTES  << " | 0x"<< hex <<  GLOBAL_DATA_IN_SIZE_BYTES << endl;
    cout << dec;
    cout << "Allocated Size (in Bytes) of input_ptr_c = " << sizeof(INPUT_mem_t)*NUMBER_OF_DATA_SETS*SDX_CU_LOCAL_IN_SIZE << " | 0x" << hex << sizeof(INPUT_mem_t)*NUMBER_OF_DATA_SETS*SDX_CU_LOCAL_IN_SIZE << endl;
    cout << dec;
    printf("-------------------------------------------------------------\n\n\n");







#ifdef GPP_ONLY_FLOW  
    //sdx_cppKernel_top(a_in_ptr, y_out_ptr, (unsigned int)NUMBER_OF_DATA_SETS, &dbg_ker_count);
    sdx_cppKernel_top(a_in_ptr, y_out_ptr, dag_ptr, (unsigned int)NUMBER_OF_DATA_SETS, &dbg_ker_count);

#else
/*
    //--------------------------------------------------------------------------
    // Compile for custom HLS accelerator platform 
    string PR_binFile_name;
    if (argc != 2) {
        printf("usage: %s fpga_bin_file\n", argv[0]);
        return -1;
    }
    PR_binFile_name = argv[1];


    printf("\n-------------------------------------------------------------\n\n");

    cout << "Initializing FPGA\n";

    fpga_xDMA_linux *my_fpga_xDMA_ptr = new fpga_xDMA_linux;

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

*/
#endif

    int MAX_ITERATION_to_print = 1;


    cout << "Verifying results .............. [ Big-Endian ]\n\n";
    high_res_elapsed_time =  0.0f;

    /*
    for (int j = 0 ; j < NUMBER_OF_DATA_SETS; j++) {
    data_t fn_in_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT];  // 16
    data_t fn_out_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT]; // 16

        for (int i = 0 ; i < SDX_CU_LOCAL_IN_SIZE; i++) {   // 16

            for (unsigned int k = 0 ; k < (NUM_ELEMENTS_PER_SDX_DATA_BEAT); k++) {
                fn_in_arg0[k] = input_ptr_c->my_data_t[k];
            }
            input_ptr_c++;
            for (unsigned int k = 0 ; k < (NUM_ELEMENTS_PER_SDX_DATA_BEAT); k++) {
                fn_out_arg0[k] = output_ptr_c->my_data_t[k];
            }
            output_ptr_c++;

            for (unsigned int index = 0; index <  16; index++) {
                if (((i == 0) && (j == 0)) ||   ((i == 1) && (j == 0))   ) { 

                    printf("Index[%d] = %04x \n", index, (fn_out_arg0[index]));
                    
                }

                
            }
        }
    }
    */
    for (int j = 0 ; j < NUMBER_OF_DATA_SETS; j++) {
    data_t fn_in_arg0[16];  
    data_t fn_out_arg0[16]; 

        for (int i = 0 ; i < 30; i++) {   

            printf("\n---- [%02d] ", i);
            switch (i) {
                case 0: printf("----> num_full_pages:\n"); break;
                case 1: printf("----> header:\n"); break;
                case 2: printf("----> target:\n"); break;
                case 3: printf("----> s_mix[0] 40byte input (header + nonce):\n"); break;    
                case 4: printf("----> s_mix[0] start:\n"); break;
                case 5: printf("----> s_mix[1] start:\n"); break;
                case 6: printf("----> s_mix[2] start:\n"); break;
                case 7: printf("\n"); break;
                case 8: printf("----> s_mix[0] final:\n"); break;
                case 9: printf("----> s_mix[1] final:\n"); break;
                case 10: printf("----> s_mix[2] final:\n"); break;
                case 11: printf("----> s_mix[0] final including compression:\n"); break;
                case 12: printf("----> s_mix[1] final including compression:\n"); break;
                case 13: printf("----> s_mix[2] final including compression:\n"); break;
                case 14: printf("\n"); break;      
                case 15: printf("----> start_nonce:\n"); break;
                case 16: printf("----> end_nonce: \n"); break;
                case 17: printf("\n"); break;
                case 18: printf("----> mix96 bytes 0-63 before byteSwap:\n"); break;
                case 19: printf("----> mix96 bytes 64-96 before byteSwap:\n"); break;
                case 20: printf("\n"); break;
                case 21: printf("----> mix96 bytes 0-63 after byteSwap:\n"); break;
                case 22: printf("----> mix96 bytes 64-96 after byteSwap:\n"); break;
                case 23: printf("\n"); break;   
                case 24: printf("----> ret_hash bytes 0-32 after SHA3-256:\n"); break;
                case 25: printf("\n"); break;                 
                case 26: printf("----> final compressed mix (32byte):\n"); break;
                case 27: printf("----> final mix hash (output hash)(32byte):\n"); break;
                case 28: printf("----> cmp: HI=1, EQ=0, LO=2:\n"); break; 
                case 29: printf("\n"); break;
                default: printf("\n");                  
            }

            for (unsigned int k = 0 ; k < 16; k++) {
                fn_out_arg0[k] = output_ptr_c->my_data_t[k];
            }
            output_ptr_c++;

            for (unsigned int index = 0; index < 16; index++) {
                //printf("Index[%d] = %08x \n", index, (fn_out_arg0[index])); 
                printf("%08x ",  fn_out_arg0[index]); 
            }
            
        }

        printf("\n\n");

        output_ptr_c = output_head_c + 0xFF;
        for (int i = 255 ; i < 265; i++) {   

            printf("\n---- [%02d] ", i);
            switch (i) {
                case 255: printf("\n"); break;
                case 256: printf("----> number of solutions:\n"); break;
                case 257: printf("\n"); break;
                case 258: printf("----> soln nonce:\n"); break;
                case 259: printf("----> soln mix:\n"); break;
                case 260: printf("----> soln mix_hash:\n"); break;
                case 261: printf("----> soln nonce:\n"); break;
                case 262: printf("----> soln mix:\n"); break;
                case 263: printf("----> soln mix_hash:\n"); break;
                case 264: printf("\n"); break;
                default: printf("\n");                  
            }

            for (unsigned int k = 0 ; k < 16; k++) {
                fn_out_arg0[k] = output_ptr_c->my_data_t[k];
            }
            output_ptr_c++;

            for (unsigned int index = 0; index < 16; index++) {
                //printf("Index[%d] = %08x \n", index, (fn_out_arg0[index])); 
                printf("%08x ",  fn_out_arg0[index]); 
            }
            
        }



    }

    printf ("\n------------   End  ----------------------------------------------------------------------------------------\n");
/*
    printf ("\n----\n");

        hash64 hash_out;
        hash32 hash_in;
        hash64_w hash_outW;

        for(int i = 0; i<32; i++){
            hash_in.b[i] = 0x00;
        }
        hash32* p_hash_in = &hash_in;
        hash64* p_hash_out = &hash_out;
        hash64_w* p_hash_outW = &hash_outW;

        uint8_t some_byte = 0x00;
        for(int i = 0; i<64; i++){
            hash_out.b[i] = some_byte;
            some_byte++;
        }

        uint32_t words[16];
        int bytecount = 0;
        uint32_t word1, word2, word3, word4, word5;
        word5 = 0x00000000;
        for(int i = 0; i<16; i++){
            word1 = (uint32_t)hash_out.b[bytecount];
            word1 = word1 << 24; 
                        printf("%08x \n", word1);
            bytecount++;
            word2 = (uint32_t)hash_out.b[bytecount];            
            word2 = word2 << 16;
                        printf("%08x \n", word2);
            bytecount++;
            word3 = (uint32_t)hash_out.b[bytecount];
            word3 = word3 << 8;    
                        printf("%08x \n", word3);                    
            bytecount++;
            word4 = (uint32_t)hash_out.b[bytecount];
            bytecount++;
            word4 = word4 << 0; 
                        printf("%08x \n", word4);            
            word5 = word5 | word1;
            word5 = word5 | word2;
            word5 = word5 | word3;
            word5 = word5 | word4;
                    printf("--> word5 = %08x \n", word5);
            hash_outW.words[i] =  word5;   
            word5 = 0x00000000;    
        }

        printf("\n hash_out  = ");
        for(int i = 0; i<64; i++){
            printf("%02x", hash_out.b[i]);
        }
        printf("\n hash_outW = ");
        for(int i = 0; i<16; i++){
            printf("%08x", hash_outW.words[i]);
        }

    printf ("\n----\n");    
*/    

/*
    INPUT_mem_t* _input = input_ptr_c;
    hash32 hash_in; 

    uint32_t temp_word, aword;
    uint8_t temp_char;
    for (int i = 0 ; i < 8; i++) {   // 8 --> 8 * 32bits = 256bits
        temp_word = _input->my_data_t[i];

        printf("\n temp_word[%02d] = %08x", i, temp_word);

        for (int j = 0 ; j < 4; j++) {
        aword = temp_word >> (j*8);
        aword = 0x000000FF & aword;
        temp_char = (uint8_t)aword;
        hash_in.b[(i*4)+(3-j)] = temp_char;
        }

        
    }


    printf("\n hash_in = ");
    for(int i = 0; i<32; i++){
        printf("%02x", hash_in.b[i]);
    }

    printf ("\n----\n");  
*/
/*
    INPUT_mem_t* _input = input_ptr_c;
    hash64 output; 

    uint32_t temp_word, aword;
    uint8_t temp_char;
    for (int i = 0 ; i < 16; i++) {   // 16 --> 16 * 32bits = 512bits
        temp_word = _input->my_data_t[i];

        printf("\n temp_word[%02d] = %08x", i, temp_word);

        for (int j = 0 ; j < 4; j++) {
        aword = temp_word >> (j*8);
        aword = 0x000000FF & aword;
        temp_char = (uint8_t)aword;
        output.b[(i*4)+(3-j)] = temp_char;
        }

        
    }


    printf("\n output = ");
    for(int i = 0; i<64; i++){
        printf("%02x", output.b[i]);
    }

    printf ("\n----\n");  
*/
/*
    hash64_w input;
    hash64_w output; 

    input.words[0] = 0x35ade1c6;
    input.words[1] = 0x43e773f1; 
    input.words[2] = 0x002af108; 
    input.words[3] = 0xdf78d3cf; 
    input.words[4] = 0x3c6d069c; 
    input.words[5] = 0x93ee14c2; 
    input.words[6] = 0x1e86d5c5; 
    input.words[7] = 0xb9d83ca4; 
    input.words[8] = 0xc8baf1f1; 
    input.words[9] = 0x04737aa3; 
    input.words[10] = 0xc2776820; 
    input.words[11] = 0x6cbc7d3e; 
    input.words[12] = 0x63eaa970; 
    input.words[13] = 0x7fb149aa; 
    input.words[14] = 0xaa0bd264; 
    input.words[15] = 0x3cc2adef; 

    uint32_t temp_word, aword, bword, cword;
    uint8_t temp_char;
    for (int i = 0 ; i < 16; i++) {   // 16 --> 16 * 32bits = 512bits
        temp_word = input.words[i];
        printf("\n temp_word[%02d] = %08x", i, temp_word);

        cword = 0x00000000;
        for (int j = 0 ; j < 4; j++) {
        bword = 0x00000000;
        aword = temp_word >> (j*8);
        aword = 0x000000FF & aword;
        bword = aword << ((3-j)*8);
        cword = cword | bword;
        }
        printf("\n cword[%02d] = %08x", i, cword);
        output.words[i] = cword;
    }


    printf("\n output = ");
    for(int i = 0; i<16; i++){
        printf("%08x", output.words[i]);
    }
    // s_mix  = 35ade1c643e773f1002af108df78d3cf3c6d069c93ee14c21e86d5c5b9d83ca4c8baf1f104737aa3c27768206cbc7d3e63eaa9707fb149aaaa0bd2643cc2adef
    //          c6e1ad35 f173e743 08f12a00 cfd378df 9c066d3c c214ee93 c5d5861e a43cd8b9 f1f1bac8 a37a7304 206877c2 3e7dbc6c 70a9ea63 aa49b17f 64d20baa efadc23c
    // output = c6e1ad35 f173e743 08f12a00 cfd378df 9c066d3c c214ee93 c5d5861e a43cd8b9 f1f1bac8 a37a7304 206877c2 3e7dbc6c 70a9ea63 aa49b17f 64d20baa efadc23c
    printf ("\n----\n");  
*/

    // ------------ Clean -----------------------

    free(input_ptr_c_POSIX);
    free(output_ptr_c_POSIX);
    free(dag_ptr_c_POSIX);

    return 0;
}
