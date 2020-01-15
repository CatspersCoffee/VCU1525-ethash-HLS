// test_program_4.cpp


#include "test_program_4.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <vector>
#include <algorithm>


#include <math.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <cmath>

#include <iomanip>
//----------------------------------------------------
// library dag:
#include <libdag/create_dag.h>
#include <libdag/internal.h>
#include <libdag/ethash_helpers.h>
#include <libdag/hash_types.h>

//----------------------------------------------------
// library accel:
#include <libaccel/fpga_utls.h>
#include <libaccel/sdx_cppKernel_top.h>
#include <libaccel/common_src/pcie_memio.h>
#include <libaccel/common_src/srai_accel_utils.h>
#include <libaccel/colors.h>
//----------------------------------------------------
// for SHA3:
//#include <libdag/sha3_cryptopp.h>
#include <libdag/sha3.h>
//#include <cryptopp/sha3.h>

//----------------------------------------------------

#define ZERO_f 1.0e-4
#define ONE_GIG (1024UL*1024UL*1024UL)
//----------------------------------------------------

//Function Declarations:
void stringup_norm_bignum_ff( struct bn* n, unsigned char* norm_str);
//----------------------------------------------------






//----------------------------------------------------

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

void tp3_load_header(INPUT_mem_t* _input, uint32_t _index_start){
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

void tp3_load_target(INPUT_mem_t* _input, uint32_t _index_start){
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

void tp3_load_nonce(INPUT_mem_t* _input, uint64_t _start_nonce, uint64_t _end_nonce, uint64_t _CUs, uint32_t _index_start){
    INPUT_mem_t* _input_head = _input;
    uint32_t index = _index_start;
    uint32_t start_nonce[16];
    uint32_t end_nonce[16];
    uint64_t buffer, startforCU, endforCU, temp;

    /*
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
    */
    
	printf("\n");
    printf("\n _start_nonce  = %016llx --> dec %llu", _start_nonce, _start_nonce);
    printf("\n _end_nonce    = %016llx --> dec %llu", _end_nonce, _end_nonce);
    uint64_t noncesperCU = ((_start_nonce - _end_nonce) / _CUs) + 1;
    printf("\n noncesperCU = %016llx   --> dec %llu", noncesperCU, noncesperCU);
	printf("\n");

    for(uint32_t cu = 0; cu < _CUs; cu++){
        //startforCU = _start_nonce - (noncesperCU * cu);
		startforCU = _end_nonce + (noncesperCU * cu);
		printf("\n startforCU[%u]  = dec %llu", cu, startforCU);

        if(cu == 0 ){
            startforCU = startforCU + 0;
			endforCU = startforCU + noncesperCU - 1;
        } else if ( cu != 0 || cu != (_CUs - 1)){
			startforCU = startforCU ;
            endforCU = startforCU + noncesperCU - 1 ;

        } else if (cu == (_CUs - 1)){
			startforCU = startforCU ;
            endforCU = startforCU + noncesperCU - 1;
        }
        //endforCU = startforCU + noncesperCU;

        //swap
        temp = endforCU;
        endforCU = startforCU;
        startforCU = temp;

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

void tp3_load_remaining_input_data(INPUT_mem_t* _input, uint32_t _index_start){
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
void tp3_load_dag(ethash_full_type* _dag_mem, uint64_t _dag_size_in_bytes, INPUT_mem_t* _input, uint32_t _index_dag){
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
            printf("\n nfp[%02d] = %08x", i, nfp[i]);
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


    printf("\n\n");
}

//------------------------------------------------------------------------------
void tp3_read_dag(){

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


void tp3_load_clear(srai_mem_conv_IN0* _clear_mem){

    for (uint64_t i = 0 ; i < 32; i++) {  //equivalent to nodes_in_dag

        for(int d = 0; d < 16; d++) {
            _clear_mem->my_data_t[d] = 0x00000000;
        }
        _clear_mem++;
    }
}



//------------------------------------------------------------------------------


#define RUN_TRANSFERS_3
#define PROG_PR_3

void test_program_3(){
    printf("\n\n----------------------------------------------------------------------------------------------------\n");
    printf("%s%11s%s", CL_LYL, "test_program_3() START:\n\n", CL_N);    
    printf("%s%11s%s", CL_LYL, "for VU9P_KERNEL_E1.bin\n", CL_N);   
    /*
	// params for ethash
	uint32_t block;
	uint8_t seedhashforBlock[32];
	int m;

	block = 1;
	ethash_params params;
	ethash_params_init(&params, block);
	uint8_t seed[32], previous_hash[32];

	//memcpy(seed, hexStringToBytes("9410b944535a83d9adf6bbdcc80e051f30676173c16ca0d32d6f1263fc246466").data(), 32);	
	memcpy(seed, hexStringToBytes("0000000000000000000000000000000000000000000000000000000000000000").data(), 32);
	memcpy(previous_hash, hexStringToBytes("c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470").data(), 32);
	
	ethash_get_seedhash(seedhashforBlock, block);

	void* full_mem_buf = malloc(params.full_size + 4095);
	void* full_mem = (void*)((uintptr_t(full_mem_buf) + 4095) & ~4095);
	void* cache_mem_buf = malloc(params.cache_size + 63);
	void* cache_mem = (void*)((uintptr_t(cache_mem_buf) + 63) & ~63);

	ethash_cache cache;
	cache.mem = cache_mem;

	printf("\n params.full_size    = %lu", params.full_size);
	printf("\n params.cache_size   = %lu", params.cache_size);
	printf("\n params.block_number = %u", params.block_number);	

	clock_t startTime = clock();
	//ethash_mkcache(&cache, &params, seed);
	ethash_mkcache(&cache, &params, seedhashforBlock);
	clock_t time = clock() - startTime;


    char s_dagDirName_B[256] = "";
	ethash_full_t epoch_full_dataset = ethash_full_new_mod(&cache, &s_dagDirName_B[0] ,&params);
	printf("\n Dataset generation for epoch %d complete!", (params.block_number/EPOCH_LENGTH));
    */
    
    /*
    printf("\n----------------------");
    printf("\n\n Test a hash:");

    //memcpy(previous_hash, hexStringToBytes("6a286c5fc0f36814732c86c3e71c036dd96d58def86b9244bb1480571e67d2a8").data(), 32);	
    //memcpy(previous_hash, hexStringToBytes("c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470").data(), 32);
    memcpy(previous_hash, hexStringToBytes("0000000100000002000000030000000400000005000000060000000700000008").data(), 32);

    printf("\n Header hash = ");
    for(m = 0; m < 32; m++) {
        printf("%02x", previous_hash[m]);
    }		

    ethash_return_value hash;
    startTime = clock();
    ethash_full_mod(&hash, (node const*)epoch_full_dataset->data, &params, previous_hash, 0);
    //ethash_full_debug(&hash, (node const*)epoch_full_dataset->data, &params, previous_hash, 0);
    time = clock() - startTime;

    printf("\n Result hash = ");		
    for( m = 0; m < 32; m++) {
        printf("%02x", hash.result[m]);
    }
    printf("\n ethash_full hash: %uns, %s\n", (unsigned)((time*1000000)/CLOCKS_PER_SEC), bytesToHexString(hash.result, 32).data());
    
    node* full_nodes = (node*)epoch_full_dataset->data;
    node* dag_node = &full_nodes[0x00311ce0];
    printf("\n dag_node[] = ");
    print_node512_hex(dag_node);
    */

    //cout << "DAG size = " << epoch_full_dataset->file_size << " Bytes" << endl;

    printf("\n\n----------------------------------------------------------------------------------------------------\n\n");

    //-----------------------------------------------------------------------------------------
    printf("%s%11s%s", CL_LCY, "Load Bitstream:\n\n", CL_N);
    // program bitfile and setup initial clock speed.
    string bit_file_name;
    //bit_file_name = "../../bitfiles/blah.bin";
    //bit_file_name = "../../bitfiles/VU9P_PASSTHRU.bin";

    //---- For Shell: 44
    //bit_file_name = "../../bitfiles/VU9P_PASSTHRU_8GB_partial.bin";

    //---- For Shell: 44B
    //bit_file_name = "../../bitfiles/VU9P_KERNEL_T1.bin";
    //bit_file_name = "../../bitfiles/VU9P_KERNEL_T2.bin";
    //bit_file_name = "../../bitfiles/VU9P_KERNEL_T3.bin";
    //bit_file_name = "../../bitfiles/VU9P_KERNEL_T4.bin";
    //bit_file_name = "../../bitfiles/VU9P_KERNEL_T5.bin";
    //bit_file_name = "../../bitfiles/VU9P_KERNEL_T6.bin";    //--> works
    //bit_file_name = "../../bitfiles/VU9P_KERNEL_T7.bin";    //--> works
    //bit_file_name = "../../bitfiles/VU9P_KERNEL_T8.bin";    //--> runs but does not produce desired result 
    //bit_file_name = "../../bitfiles/VU9P_KERNEL_T9.bin";    //--> runs but does not produce desired result 
    //bit_file_name = "../../bitfiles/VU9P_KERNEL_T10.bin";    //-->
    //bit_file_name = "../../bitfiles/VU9P_KERNEL_T11.bin";    //-->
    bit_file_name = "../../bitfiles/VU9P_KERNEL_E1.bin";    //-->
#ifdef PROG_PR_3 
    int init_success = init_fpga_device_ETHASH(bit_file_name);
#endif




    printf("%s%11s%s", CL_LCY, "Setup Datasets:\n\n", CL_N);


    int compute_itn_count;
    time_t t;
    //srand((unsigned) time(&t));
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

    //--------------------------------------------------------------
    sdx_data_t* clear_ptr;
    char *clear_ptr_c_POSIX = NULL;
    srai_mem_conv_IN0 *clear_ptr_c;
    srai_mem_conv_IN0 *clear_head_c;


    posix_memalign((void **)&clear_ptr_c_POSIX, 4096, (16*32*4) + 4096);
    clear_ptr_c = (srai_mem_conv_IN0 *)clear_ptr_c_POSIX;
    clear_head_c = clear_ptr_c;

    //--------------------------------------------------------------
    sdx_data_t* dag_ptr;
    char *dag_ptr_c_POSIX = NULL;
    ethash_full_type* dag_ptr_c;                                    //for test_program.cpp use ethash_full_type
    ethash_full_type* dag_head_c;                                   //for test_program.cpp use ethash_full_type
    uint64_t size_dag = 1073739904U;
    posix_memalign((void **)&dag_ptr_c_POSIX, 64, size_dag + 64);
    dag_ptr_c = (ethash_full_type*)dag_ptr_c_POSIX;                 //for test_program.cpp use ethash_full_type
    dag_head_c = dag_ptr_c;

    //--------------------------------------------------------------

    sdx_data_t* a_in_ptr;
    sdx_data_t* y_out_ptr;

    char *input_ptr_c_POSIX = NULL;
    char *output_ptr_c_POSIX = NULL;

    INPUT_mem_t* input_ptr_c;
    OUTPUT_mem_t* output_ptr_c;
    INPUT_mem_t* input_head_c;
    OUTPUT_mem_t* output_head_c;

    /*
    posix_memalign((void **)&input_ptr_c_POSIX, 4096, GLOBAL_DATA_IN_SIZE_BYTES + 4096);
    input_ptr_c = (INPUT_mem_t *)input_ptr_c_POSIX;
    posix_memalign((void **)&output_ptr_c_POSIX, 4096, GLOBAL_DATA_OUT_SIZE_BYTES + 4096);
    output_ptr_c = (OUTPUT_mem_t *)output_ptr_c_POSIX;
    */

    //posix_memalign((void **)&input_ptr_c_POSIX, 64, GLOBAL_DATA_IN_SIZE_BYTES + 64);
    //input_ptr_c = (INPUT_mem_t *)input_ptr_c_POSIX;
    posix_memalign((void **)&input_ptr_c_POSIX, 64, INPUT_SIZE_BYTES + 64);
    input_ptr_c = (INPUT_mem_t *)input_ptr_c_POSIX;    
    
    posix_memalign((void **)&output_ptr_c_POSIX, 64, OUTPUT_SIZE_BYTES + 64);
    output_ptr_c = (OUTPUT_mem_t *)output_ptr_c_POSIX;



    input_head_c = input_ptr_c;
    output_head_c = output_ptr_c;


    //--------------------------------------------------------------
    printf("%s%11s%s", CL_LCY, "Generating fake-DAG with some node values in host memory:\n\n", CL_N);

    tp3_load_dag(dag_ptr_c, size_dag, input_ptr_c, INDEX_IN_num_full_pages);
    dag_ptr = (sdx_data_t *)dag_head_c;
    dag_ptr_c = dag_head_c;

    //--------------------------------------------------------------
    printf("%s%11s%s", CL_LCY, "Load clear memory buffer with zeros:\n\n", CL_N);

    tp3_load_clear(clear_ptr_c);
    clear_ptr = (sdx_data_t *)clear_head_c;
    clear_ptr_c = clear_head_c;


    //--------------------------------------------------------------
    printf("%s%11s%s", CL_LCY, "Load values to INPUT buffer:\n\n", CL_N);

    // header_hash      --> index 0
    // nonce values     --> index 1-32 (32 values total, 16 for each CU --> nonce_start[CU], nonce_end[CU])
    // remaining data   --> index 33+
    //gen_test_data(input_ptr_c);
    tp3_load_header(input_ptr_c, INDEX_IN_header);  
    tp3_load_target(input_ptr_c, INDEX_IN_target);  
    //tp3_load_nonce(input_ptr_c, 0x0000000000000001ULL, 0xFFFFFFFFFFFFFFFFULL, 0x4ULL, INDEX_IN_nonces);    // input_data_ptr, start_nonce, end_nonce, compute units
    //tp3_load_nonce(input_ptr_c, 0x0000000000000000ULL, 0x00000000FFFFFFFFULL, 0x4ULL, INDEX_IN_nonces);      // testing
    tp3_load_nonce(input_ptr_c, 0x0000000000000001ULL, 0x0000000000000000ULL, 0x1ULL, INDEX_IN_nonces);  //  testing
    //tp3_load_nonce(input_ptr_c, 0x00000000000000FFULL, 0x0000000000000000ULL, 0x1ULL, INDEX_IN_nonces);  //  testing


    //tp3_load_remaining_input_data(input_ptr_c, INDEX_IN_rem_dat);

    a_in_ptr = (sdx_data_t *)input_head_c;
    y_out_ptr = (sdx_data_t *)output_head_c;
    input_ptr_c = input_head_c;

    //--------------------------------------------------------------
    printf("%s%11s%s", CL_LCY, "Memory Initialized with test Dataset and Input data!\n\n", CL_N);



    printf("-------------------------------------------------------------\n");
    uint32_t row = 0;
    cout << "print a few values from fake-DAG: \n";
    for (unsigned int index = 0; index < (3*16) ;index++) {
            if(index % 16 == 0){
                printf("\n---- dag Index[%d]\n", row);
                row++;
            }
            printf ("%08x ", (dag_ptr_c->words[index]));
    } 

    printf("-------------------------------------------------------------\n\n");

    printf("-------------------------------------------------------------\n");
    row = 0;
    cout << "print a few values from input buffer: \n";
    for (unsigned int index = 0; index < (35*16) ;index++) {
            if(index % 16 == 0){
                printf("\n---- input Index[%d]\n", row);
                row++;
            }
            printf ("%08x ", (input_ptr_c->my_data_t[index]));
    } 

    printf("-------------------------------------------------------------\n\n");

    printf("-------------------------------------------------------------\n");
    row = 0;
    cout << "print a few values from clear buffer: \n";
    for (unsigned int index = 0; index < (3*16) ;index++) {
            if(index % 16 == 0){
                printf("\n---- clear Index[%d]\n", row);
                row++;
            }
            printf ("%08x ", (clear_ptr_c->my_data_t[index]));
    } 

    printf("-------------------------------------------------------------\n\n");



    printf("-------------------------------------------------------------\n\n\n");
    cout << "NUMBER_OF_DATA_SETS  =  " << NUMBER_OF_DATA_SETS << endl;
    cout << "GLOBAL_DATA_IN_SIZE  =  " << GLOBAL_DATA_IN_SIZE << endl;
    cout << "GLOBAL_DATA_OUT_SIZE =  " << GLOBAL_DATA_OUT_SIZE << endl;
    cout << "GLOBAL_DATA_DAG_SIZE =  " << GLOBAL_DATA_DAG_SIZE << endl;  
    cout << "----" << endl;
    cout << "GLOBAL_DATA_IN_SIZE_BYTES  =  " << GLOBAL_DATA_IN_SIZE_BYTES << endl;  
    cout << "GLOBAL_DATA_OUT_SIZE_BYTES =  " << GLOBAL_DATA_OUT_SIZE_BYTES << endl;  
    cout << "GLOBAL_DATA_DAG_SIZE_BYTES =  " << GLOBAL_DATA_DAG_SIZE_BYTES << endl;  
    cout << "----" << endl;
    cout << "NUM_ELEMENTS_PER_SDX_DATA_BEAT =  " << NUM_ELEMENTS_PER_SDX_DATA_BEAT << endl;  
    cout << "----" << endl;

    cout << "INPUT_NODES       =  " << INPUT_NODES << endl;  
    cout << "INPUT_SIZE_BYTES  =  " << INPUT_SIZE_BYTES << endl;  
    cout << "OUTPUT_NODES      =  " << OUTPUT_NODES << endl;  
    cout << "OUTPUT_SIZE_BYTES =  " << OUTPUT_SIZE_BYTES << endl;  


    cout << "----" << endl;
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







#ifdef RUN_TRANSFERS_3  
    printf("%s%11s%s", CL_LCY, "Run Kernel:\n\n", CL_N);

    //--------------------------------------------------------------------------
    fpga_xDMA_linux *my_fpga_xDMA_ptr = new fpga_xDMA_linux;
    my_fpga_xDMA_ptr->fpga_xDMA_init();

    /*
    fpga_PROGRAM_PR_CLOCK(my_fpga_xDMA_ptr, HW_Kernel_frequency350);
    cout << "--> ....DONE Programing PR clock: 250Mhz" << endl;
    */

    //--------------------------------------------------------------------------
    /*
    // Read the PR_HLS Control register to poll the Idle bit (bit 1) -----  
    fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C0, (char*)a_in_ptr, (GLOBAL_DATA_IN_SIZE_BYTES));
    fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_dag_C0, (char*)dag_ptr, (GLOBAL_DATA_DAG_SIZE_BYTES));
    fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_results_C0, (char*)clear_ptr, (16*32*4));

    printf("\n Transfer to card --> OK");

    // Write to PR_HLS Address offset registers to set the location in Memory where Input Data and Output results are stored 
    //fpga_run_NORTH_PR64(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C1, AXI_MM_DDR4_results_C1, (NUMBER_OF_DATA_SETS));
    fpga_run_NORTH_mod1(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C0, AXI_MM_DDR4_dag_C0, AXI_MM_DDR4_results_C0, (NUMBER_OF_DATA_SETS));

    printf("\n Run NORTH --> OK");


    printf("\n");
    start_t = chrono::high_resolution_clock::now();
    compute_itn_count = fpga_check_compute_done_NORTH_PR(my_fpga_xDMA_ptr);
    stop_t = chrono::high_resolution_clock::now();

    cout << "compute_itn_count = " << compute_itn_count << endl;

    elapsed_hi_res = stop_t - start_t ;
    high_res_elapsed_time = elapsed_hi_res.count();
    cout << "high_res_elapsed_time = " << high_res_elapsed_time << endl;
    

    // Read Results from DDR4 output (results) area 
    fpga_xfer_data_from_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_results_C1, (char*)y_out_ptr, (GLOBAL_DATA_OUT_SIZE_BYTES));
    */
    //--------------------------------------------------------------------------

    // Read the PR_HLS Control register to poll the Idle bit (bit 1) 
    //fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_C1, (char*)a_in_ptr, INPUT_SIZE_BYTES);            // works
    //fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C1, (char*)a_in_ptr, INPUT_SIZE_BYTES);        // input_C1 works
    fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C0, (char*)a_in_ptr, INPUT_SIZE_BYTES);        // input_C0 works

    //fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C3, (char*)a_in_ptr, INPUT_SIZE_BYTES);      // input_C3 doesnt work
    //fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C2, (char*)a_in_ptr, INPUT_SIZE_BYTES);      // input_C2 doesnt work

    //fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_dag_C0, (char*)dag_ptr, GLOBAL_DATA_DAG_SIZE_BYTES); // dag_C0 works
    //fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_dag_C1, (char*)dag_ptr, GLOBAL_DATA_DAG_SIZE_BYTES); // dag_C1 works
    //fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_dag_C2, (char*)dag_ptr, GLOBAL_DATA_DAG_SIZE_BYTES); // dag_C2 doesnt work
    fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_dag_C3, (char*)dag_ptr, GLOBAL_DATA_DAG_SIZE_BYTES); // dag_C3 works


    fpga_xfer_data_to_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_results_C1, (char*)clear_ptr, (16*32*4));
    
    // Write to PR_HLS Address offset registers to set the location in Memory where Input Data and Output results are stored 
    //fpga_run_NORTH_PR64(my_fpga_xDMA_ptr, AXI_MM_DDR4_C1, AXI_MM_DDR4_results_C1, (NUMBER_OF_DATA_SETS));
    //fpga_run_NORTH_mod1(my_fpga_xDMA_ptr, AXI_MM_DDR4_C1, AXI_MM_DDR4_dag_C0, AXI_MM_DDR4_results_C1, NUMBER_OF_DATA_SETS);         // works
    //fpga_run_NORTH_mod1(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C1, AXI_MM_DDR4_dag_C0, AXI_MM_DDR4_results_C1, NUMBER_OF_DATA_SETS);     // input_C1 works
    //fpga_run_NORTH_mod1(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C0, AXI_MM_DDR4_dag_C0, AXI_MM_DDR4_results_C1, NUMBER_OF_DATA_SETS);     // input_C0 works

    //fpga_run_NORTH_mod1(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C0, AXI_MM_DDR4_dag_C0, AXI_MM_DDR4_results_C1, NUMBER_OF_DATA_SETS);     // dag_C0 works
    //fpga_run_NORTH_mod1(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C0, AXI_MM_DDR4_dag_C1, AXI_MM_DDR4_results_C1, NUMBER_OF_DATA_SETS);     //// dag_C1 works
    //fpga_run_NORTH_mod1(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C0, AXI_MM_DDR4_dag_C2, AXI_MM_DDR4_results_C1, NUMBER_OF_DATA_SETS);     //// dag_C2 doesnt work
    fpga_run_NORTH_mod1(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C0, AXI_MM_DDR4_dag_C3, AXI_MM_DDR4_results_C1, NUMBER_OF_DATA_SETS);     //// dag_C3 works

    //fpga_run_NORTH_mod1(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C3, AXI_MM_DDR4_dag_C0, AXI_MM_DDR4_results_C1, NUMBER_OF_DATA_SETS);   // input_C3 doesnt work
    //fpga_run_NORTH_mod1(my_fpga_xDMA_ptr, AXI_MM_DDR4_input_C2, AXI_MM_DDR4_dag_C0, AXI_MM_DDR4_results_C1, NUMBER_OF_DATA_SETS);   // input_C2 doesnt work




    printf("\n Run Started --> OK\n");
    printf("\n ");

    printf("\n");
    start_t = chrono::high_resolution_clock::now();
    compute_itn_count = fpga_check_compute_done_NORTH_PR(my_fpga_xDMA_ptr);
    stop_t = chrono::high_resolution_clock::now();

    cout << "compute_itn_count = " << compute_itn_count << endl;

    elapsed_hi_res = stop_t - start_t ;
    high_res_elapsed_time = elapsed_hi_res.count();
    cout << "high_res_elapsed_time = " << high_res_elapsed_time << endl;


    // Read Results from DDR4 output (results) area 
    //fpga_xfer_data_from_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_results_C1, (char*)y_out_ptr, (GLOBAL_DATA_OUT_SIZE_BYTES));
    fpga_xfer_data_from_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_results_C1, (char*)y_out_ptr, OUTPUT_SIZE_BYTES);


    //--------------------------------------------------------------------------
    elapsed_hi_res = stop_t - start_t ;
    high_res_elapsed_time = elapsed_hi_res.count();
    high_res_elapsed_time_HW = high_res_elapsed_time;
    cout << "HLS Execution time =  " <<  high_res_elapsed_time_HW << "s\n";
    cout << "HLS THroughput =  " <<  (GLOBAL_DATA_OUT_SIZE_BYTES/high_res_elapsed_time_HW) << " Bytes/s\n";

    fpga_get_Kernel_execution_time (my_fpga_xDMA_ptr, HW_Kernel_frequency, &kernel_execution_metric);
    cout << "KERNEL_DATASET =  " <<  dec << (kernel_execution_metric.KERNEL_DATASET) << " \n";
    cout << "KERNEL_CLOCK_COUNT =  " <<  dec << (kernel_execution_metric.KERNEL_CLOCK_COUNT) << " \n";
    cout << "KERNEL_Execution_time (sec) =  " <<  dec << (kernel_execution_metric.KERNEL_EXECUTION_TIME) << " \n";

    //--------------------------------------------------------------------------

    fpga_xfer_data_from_card64(my_fpga_xDMA_ptr, AXI_MM_DDR4_dag_C0, (char*)a_in_ptr, 64*32);

    //--------------------------------------------------------------------------

    fpga_clean(my_fpga_xDMA_ptr);


#endif

    int MAX_ITERATION_to_print = 1;

    printf("%s%11s%s", CL_LCY, "\nVerifying results ..............\n", CL_N);

    high_res_elapsed_time =  0.0f;


    for (int j = 0 ; j < NUMBER_OF_DATA_SETS; j++) {
    data_t fn_in_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT];  // 16
    data_t fn_out_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT]; // 16

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
                case 264: printf("----> soln nonce:\n"); break;
                case 265: printf("----> soln mix:\n"); break;
                case 266: printf("----> soln mix_hash:\n"); break;                
                case 267: printf("\n"); break;
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
        printf("\n probe data from memory on card:");

        input_ptr_c = input_head_c + 0x00;
        for (int i = 0 ; i < 40; i++) {  

            printf("\n---- [%02d] ", i);
            for (unsigned int k = 0 ; k < 16; k++) {
                fn_out_arg0[k] = input_ptr_c->my_data_t[k];
            }
            output_ptr_c++;

            for (unsigned int index = 0; index < 16; index++) {
                //printf("Index[%d] = %08x \n", index, (fn_out_arg0[index])); 
                printf("%08x ",  fn_out_arg0[index]); 
            }
            input_ptr_c++;
        } 



    }

    printf ("\n------------   End  ----------------------------------------------------------------------------------------\n");


    // ------------ Clean -----------------------

    free(input_ptr_c_POSIX);
    free(output_ptr_c_POSIX);
    free(dag_ptr_c_POSIX);

    return;
}









/*



*/