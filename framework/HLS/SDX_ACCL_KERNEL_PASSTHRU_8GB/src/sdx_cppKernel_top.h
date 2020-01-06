#include  "ap_int.h"
#include  "stdint.h"
#ifndef SDX_CPPKERNEL_TOP_H_
#define SDX_CPPKERNEL_TOP_H_ 

#define data_t uint32_t     // 4 bytes

#define NUMBER_OF_SDX_BUS_XFERS_PER_INPUT 1UL
#define NUMBER_OF_SDX_BUS_XFERS_PER_OUTPUT 1UL

#define SDX_BUS_WIDTH 512UL                     // 512
#define sdx_data_t ap_uint<SDX_BUS_WIDTH>       // 512
#define SDX_BUS_WIDTH_BYTES SDX_BUS_WIDTH/8UL   // 512/8 = 64

#define SDX_CU_LOCAL_SIZE 16UL


//#define NUMBER_OF_DATA_SETS 1024UL
//#define NUMBER_OF_DATA_SETS 1024UL
//#define NUMBER_OF_DATA_SETS 1024UL*1024UL
#define NUMBER_OF_DATA_SETS 1UL


#define HW_Kernel_frequency 250.0e6

#define NUM_INPUT_KERNEL_FUNCTION_ARGUMENTS 1UL
#define NUM_OUTPUT_KERNEL_FUNCTION_ARGUMENTS 1UL


#define SDX_CU_LOCAL_IN_SIZE (SDX_CU_LOCAL_SIZE*NUM_INPUT_KERNEL_FUNCTION_ARGUMENTS)    // how many elements are in the local CU. = 16
#define SDX_CU_LOCAL_OUT_SIZE (SDX_CU_LOCAL_SIZE*NUM_OUTPUT_KERNEL_FUNCTION_ARGUMENTS) 
#define NUM_ELEMENTS_PER_SDX_DATA_BEAT ((sizeof(sdx_data_t))/(sizeof(data_t)))          // 64 (bytes wide) / 4 (each element is byte wide) = 16 

#define GLOBAL_DATA_IN_SIZE ((uint32_t)(SDX_CU_LOCAL_IN_SIZE*NUMBER_OF_DATA_SETS*NUMBER_OF_SDX_BUS_XFERS_PER_INPUT))
#define GLOBAL_DATA_OUT_SIZE ((uint32_t)(SDX_CU_LOCAL_OUT_SIZE*NUMBER_OF_DATA_SETS*NUMBER_OF_SDX_BUS_XFERS_PER_OUTPUT))
#define GLOBAL_DATA_IN_SIZE_BYTES ((uint32_t)(GLOBAL_DATA_IN_SIZE*sizeof(sdx_data_t)))
#define GLOBAL_DATA_OUT_SIZE_BYTES ((uint32_t)(GLOBAL_DATA_OUT_SIZE*sizeof(sdx_data_t)))


//-----------------------------------------------------------

#define INDEX_IN_header                 0x00000000UL        // header_hash      --> index 0
#define INDEX_IN_target                 0x00000001UL        // header_hash      --> index 1
#define INDEX_IN_nonces                 0x00000002UL        // nonce values     --> index 2-33 (32 values, start[CU], end[CU])
#define INDEX_IN_num_full_pages         0x00000022UL        // nonce values     --> index 34

#define INDEX_IN_rem_dat                0x00000040UL        // remaining data   --> index 64+

#define INDEX_OUT_num_full_pages        0x00000000UL
#define INDEX_OUT_header                0x00000001UL
#define INDEX_OUT_target                0x00000002UL

#define INDEX_OUT_soln_start            0x000000FFUL        // solution start                       --> index 255
#define INDEX_OUT_numberofhashs         0x000000FFUL        // number of hashes done this round     --> index 255
#define INDEX_OUT_soln_num              0x00000100UL        // solution number of solutions         --> index 256
#define INDEX_OUT_soln_nonce            0x00000000UL        // solution nonce                       --> would start from 255 + (1*2)   = index 257
#define INDEX_OUT_soln_mixhash          0x00000001UL        // solution mix hash                    --> would start from 255 + (1*2)+1 = index 258

//-----------------------------------------------------------

#define NODE_WORDS (64/4) //16

typedef union {
	unsigned char bytes[32];
	uint32_t words[8];                          // 256/32 = 8
	uint64_t double_words[4];
} hash32_t;

typedef union {
	unsigned char  bytes[NODE_WORDS * 4];
	uint32_t words[NODE_WORDS];                 // words[16]        512/32 = 16 = NODE_WORDS
	uint64_t double_words[NODE_WORDS / 2];      // double_words[8]
} node64_t;




typedef union mix96 {
    uint8_t b[64+32]; 
} mix96;



typedef union hash32 {
    uint8_t b[32]; 
} hash32;

typedef union hash64 {
    uint8_t b[64]; 
} hash64;

typedef union node64 {
    uint32_t words[16];
} node64_w;

typedef union hash64_w {
    uint32_t words[16];
} hash64_w;

typedef union hash32_w {
    uint32_t words[8];
} hash32_w;

typedef union node {
    unsigned char cbytes[NODE_WORDS * 4];
    uint8_t bytes[NODE_WORDS * 4];
    uint32_t words[NODE_WORDS];
    uint64_t double_words[NODE_WORDS / 2];
} node;


typedef union ethash_full {
    uint8_t bytes[NODE_WORDS * 4];
    uint32_t words[NODE_WORDS];
	uint64_t double_words[NODE_WORDS / 2];
} ethash_full;


//-----------------------------------------------------------



//-----------------------------------------------------------

typedef union {
    uint32_t my_uint32;
    data_t my_data_t;
} srai_conv;


typedef union {
    unsigned char my_uint_char[64];
    uint32_t my_data_t[16];
} INPUT_mem_t;


typedef union {
    unsigned char my_uint_char[64];
    uint32_t my_data_t[16];
} OUTPUT_mem_t;


const long int HLS_AXI_SIM_IN_DEPTH=(long int)GLOBAL_DATA_IN_SIZE;
const long int HLS_AXI_SIM_OUT_DEPTH=(long int)GLOBAL_DATA_OUT_SIZE;
const unsigned int HLS_SDX_WRAPPER_II=(unsigned int)(SDX_CU_LOCAL_IN_SIZE*NUMBER_OF_SDX_BUS_XFERS_PER_INPUT);
const unsigned int HLS_SDX_WRAPPER_TRIP_COUNT=(unsigned int)NUMBER_OF_DATA_SETS;
const unsigned int HLS_SDX_WRAPPER_INPUT_ARRAY_COUNT=(unsigned int)NUM_INPUT_KERNEL_FUNCTION_ARGUMENTS;
const unsigned int HLS_SDX_WRAPPER_OUTPUT_ARRAY_COUNT=(unsigned int)NUM_OUTPUT_KERNEL_FUNCTION_ARGUMENTS;

void kernel_WRAPPER (data_t in_arg0[SDX_CU_LOCAL_IN_SIZE], data_t out_arg0[SDX_CU_LOCAL_OUT_SIZE]);
void passthru (data_t *a_in, data_t *results);

#ifdef XOCC_CPP_KERNEL 
extern "C" {
#endif
void sdx_cppKernel_top(sdx_data_t *a_in, sdx_data_t *y_out, sdx_data_t* _dag, unsigned int NUMBER_OF_DATA_SETS_t, uint32_t *ker_count);
#ifdef XOCC_CPP_KERNEL 
}
#endif

#endif
