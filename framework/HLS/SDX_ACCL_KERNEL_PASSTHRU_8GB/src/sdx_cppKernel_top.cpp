#include <string.h>

#include "sdx_cppKernel_top.h"
#include "sdx_pack_unpack.h" 


//-------------------------------------------------------------------------------------------------------------------------------------

#define uchar unsigned char
#define uint uint8_t
#define ulong unsigned long

#define MIX_BYTES 128
#define HASH_BYTES 64
#define DATASET_PARENTS 256
#define CACHE_ROUNDS 3
#define ACCESSES 64


//-------------------------------------------------------------------------------------------------------------------------------------

/*
 * BEGIN from fnv.h
 */

#define FNV_PRIME 0x01000193

static inline uint fnv_hash(const uint x, const uint y) {
	return x*FNV_PRIME ^ y;
}

/*
 * END from fnv.h
 */

/*
 * BEGIN from sha3.h
 */

#define decsha3(bits) \
		int sha3_##bits(uchar*, size_t, const uchar*, size_t);

decsha3(256)
decsha3(512)

static inline void SHA3_256(uchar * const ret, uchar const *data, const size_t size) {
	sha3_256(ret, 32, data, size);
}

static inline void SHA3_512(uchar * const ret, uchar const *data, const size_t size) {
	sha3_512(ret, 64, data, size);
}

/*
 * END from sha3.h
 */

/*
 * BEGIN from internal.h
 */

// compile time settings
#define NODE_WORDS (64/4)
#define MIX_WORDS (MIX_BYTES/4)
#define MIX_NODES (MIX_WORDS / NODE_WORDS)

/*
 * END from internal.h
 */

/*
 * END code from all headers
 */

/*
 * BEGIN from sha3.c
 */

/******** The Keccak-f[1600] permutation ********/

/*** Constants. ***/
const uchar rho[24] = \
		{ 1,  3,   6, 10, 15, 21,
	28, 36, 45, 55,  2, 14,
	27, 41, 56,  8, 25, 43,
	62, 18, 39, 61, 20, 44};
const uchar pi[24] = \
		{10,  7, 11, 17, 18, 3,
	5, 16,  8, 21, 24, 4,
	15, 23, 19, 13, 12, 2,
	20, 14, 22,  9, 6,  1};
const ulong RC[24] = \
		{1ULL, 0x8082ULL, 0x800000000000808aULL, 0x8000000080008000ULL,
	0x808bULL, 0x80000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
	0x8aULL, 0x88ULL, 0x80008009ULL, 0x8000000aULL,
	0x8000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
	0x8000000000008002ULL, 0x8000000000000080ULL, 0x800aULL, 0x800000008000000aULL,
	0x8000000080008081ULL, 0x8000000000008080ULL, 0x80000001ULL, 0x8000000080008008ULL};

/*** Helper macros to unroll the permutation. ***/
#define rol(x, s) (((x) << s) | ((x) >> (64 - s)))
#define REPEAT6(e) e e e e e e
#define REPEAT24(e) REPEAT6(e e e e)
#define REPEAT5(e) e e e e e
#define FOR5(v, s, e) \
		v = 0;            \
		REPEAT5(e; v += s;)

/*** Keccak-f[1600] ***/
void keccakf(void* state) {
	ulong* a = (ulong*)state;
	ulong b[5] = {0};
	ulong t = 0;
	uchar x, y;

	for (int i = 0; i < 24; i++) {
		// Theta
		FOR5(x, 1,
				b[x] = 0;
		FOR5(y, 5,
				b[x] ^= a[x + y]; ))
        		 FOR5(x, 1,
        				 FOR5(y, 5,
        						 a[y + x] ^= b[(x + 4) % 5] ^ rol(b[(x + 1) % 5], 1); ))
								 // Rho and pi
								 t = a[1];
		x = 0;
		REPEAT24(b[0] = a[pi[x]];
		a[pi[x]] = rol(t, rho[x]);
		t = b[0];
		x++; )
		// Chi
		FOR5(y,
				5,
				FOR5(x, 1,
						b[x] = a[y + x];)
						FOR5(x, 1,
								a[y + x] = b[x] ^ ((~b[(x + 1) % 5]) & b[(x + 2) % 5]); ))
								// Iota
								a[0] ^= RC[i];
	}
}

/******** The FIPS202-defined functions. ********/

/*** Some helper macros. ***/

#define _(S) do { S } while (0)
#define FOR(i, ST, L, S) \
		_(for (size_t i = 0; i < L; i += ST) { S; })
#define mkapply_ds(NAME, S)                                          \
		static inline void NAME(uchar* dst,                              \
				const uchar* src,                        \
				size_t len) {                              \
	FOR(i, 1, len, S);                                               \
}
#define mkapply_sd(NAME, S)                                          \
		static inline void NAME(const uchar* src,                        \
				uchar* dst,                              \
				size_t len) {                              \
	FOR(i, 1, len, S);                                               \
}

mkapply_ds(xorin, dst[i] ^= src[i])  // xorin
mkapply_sd(setout, dst[i] = src[i])  // setout

#define P keccakf
#define Plen 200

// Fold P*F over the full blocks of an input.
#define foldP(I, L, F) \
		while (L >= rate) {  \
			F(a, I, rate);     \
			P(a);              \
			I += rate;         \
			L -= rate;         \
		}

/** The sponge-based hash construction. **/
int hash(uchar* out, size_t outlen,
		const uchar* in, size_t inlen,
		size_t rate, uchar delim) {
	uchar a[Plen] = {0};
	// Absorb input.
	foldP(in, inlen, xorin);
	// Xor in the DS and pad frame.
	a[inlen] ^= delim;
	a[rate - 1] ^= 0x80;
	// Xor in the last block.
	xorin(a, in, inlen);
	// Apply P
	P(a);
	// Squeeze output.
	foldP(out, outlen, setout);
	setout(a, out, outlen);
	//memset(a, 0, 200);
	for (int i = 0; i < 200; i++) {
		a[i] = 0;
	}
	return 0;
}

#define defsha3(bits)                                             \
		int sha3_##bits(uchar* out, size_t outlen,                    \
				const uchar* in, size_t inlen) {              \
	if (outlen > (bits/8)) {                                      \
		return -1;                                                  \
	}                                                             \
	return hash(out, outlen, in, inlen, 200 - (bits / 4), 0x01);  \
}

/*** FIPS202 SHA3 FOFs ***/
defsha3(256)
defsha3(512)

/*
 * END from sha3.c
 */

//-------------------------------------------------------------------------------------------------------------------------------------


void sdx_cppKernel_top(sdx_data_t *a_in, sdx_data_t *y_out, sdx_data_t* _dag, unsigned int NUMBER_OF_DATA_SETS_t, uint32_t *ker_count) {

#pragma HLS INTERFACE m_axi port=a_in offset=slave depth=HLS_AXI_SIM_IN_DEPTH latency=100 bundle=gmem num_read_outstanding=32 num_write_outstanding=32 max_read_burst_length=16 max_write_burst_length=16
#pragma HLS INTERFACE m_axi port=y_out offset=slave depth=HLS_AXI_SIM_OUT_DEPTH latency=100 bundle=gmem num_read_outstanding=32 num_write_outstanding=32 max_read_burst_length=16 max_write_burst_length=16
#pragma HLS INTERFACE s_axilite port=a_in bundle=control
#pragma HLS INTERFACE s_axilite port=y_out bundle=control
   
#pragma HLS INTERFACE s_axilite port=NUMBER_OF_DATA_SETS_t bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

#pragma HLS INTERFACE m_axi port=_dag offset=slave depth=HLS_AXI_SIM_OUT_DEPTH latency=100 bundle=gmem num_read_outstanding=32 num_write_outstanding=32 max_read_burst_length=16 max_write_burst_length=16
#pragma HLS INTERFACE s_axilite port=_dag bundle=control 



    static uint32_t i_ker_count = 0;
    sdx_data_t bufa_val[SDX_CU_LOCAL_IN_SIZE];      // bufa_val[16] 16 element array of 512 bit type, 
    sdx_data_t bufy_val[SDX_CU_LOCAL_OUT_SIZE];     // bufy_val[16]
    sdx_pack_unpack<srai_conv> my_pack_unpack;


    sdx_data_t node_bufa_val[1];
    sdx_data_t node_bufy_val[1];

    sdx_cppKernel_top_local_data_loop:for (unsigned int i = 0; i < NUMBER_OF_DATA_SETS_t; i++) {

    #pragma HLS LOOP_TRIPCOUNT min=HLS_SDX_WRAPPER_TRIP_COUNT max=HLS_SDX_WRAPPER_TRIP_COUNT
    #pragma HLS PIPELINE II=HLS_SDX_WRAPPER_II

        /*
        memcpy(bufa_val, (const sdx_data_t*)a_in, SDX_BUS_WIDTH_BYTES*SDX_CU_LOCAL_IN_SIZE);
        a_in += SDX_CU_LOCAL_IN_SIZE;

        kernel_WRAPPER_label0:for (unsigned int itn_num = 0 ; itn_num < SDX_CU_LOCAL_SIZE; itn_num++) {
        #pragma HLS PIPELINE II=1

            data_t fn_in_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT*NUM_INPUT_KERNEL_FUNCTION_ARGUMENTS];
            #pragma HLS ARRAY_PARTITION variable=fn_in_arg0
            data_t fn_out_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT*NUM_OUTPUT_KERNEL_FUNCTION_ARGUMENTS];
            #pragma HLS ARRAY_PARTITION variable=fn_out_arg0

            for (unsigned int k = 0 ; k < NUM_INPUT_KERNEL_FUNCTION_ARGUMENTS; k++) {
                my_pack_unpack.unpack_sdx_512_data(&bufa_val[(itn_num*NUM_INPUT_KERNEL_FUNCTION_ARGUMENTS)+k], &fn_in_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT*k]);
            }

            kernel_WRAPPER(&fn_in_arg0[0], &fn_out_arg0[0]);

            for (unsigned int k = 0 ; k < NUM_OUTPUT_KERNEL_FUNCTION_ARGUMENTS; k++) {
                my_pack_unpack.pack_sdx_512_data(&bufy_val[(itn_num*NUM_OUTPUT_KERNEL_FUNCTION_ARGUMENTS)+k], &fn_out_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT*k]);
            }
        }

        memcpy(y_out, bufy_val, SDX_BUS_WIDTH_BYTES*SDX_CU_LOCAL_OUT_SIZE);
        y_out += SDX_CU_LOCAL_OUT_SIZE;
        *ker_count = i_ker_count;
        i_ker_count++;
        */


        

        

        //----------------------------------------------------------------------
        
        memcpy(node_bufa_val, (const sdx_data_t*)_dag, SDX_BUS_WIDTH_BYTES);
        _dag += 1;
        /*
            data_t fn_in_arg0[16];
            #pragma HLS ARRAY_PARTITION variable=fn_in_arg0
            data_t fn_out_arg0[16];
            #pragma HLS ARRAY_PARTITION variable=fn_out_arg0

            for (unsigned int k = 0 ; k < NUM_INPUT_KERNEL_FUNCTION_ARGUMENTS; k++) {
                my_pack_unpack.unpack_sdx_512_data(&node_bufa_val[(NUM_INPUT_KERNEL_FUNCTION_ARGUMENTS)+k], &fn_in_arg0[0]);
            }

            kernel_WRAPPER(&fn_in_arg0[0], &fn_out_arg0[0]);

            for (unsigned int k = 0 ; k < NUM_OUTPUT_KERNEL_FUNCTION_ARGUMENTS; k++) {
                my_pack_unpack.pack_sdx_512_data(&bufy_val[(NUM_OUTPUT_KERNEL_FUNCTION_ARGUMENTS)+k], &fn_out_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT*k]);
            }
        */

        memcpy(y_out, node_bufa_val, SDX_BUS_WIDTH_BYTES);
        //memcpy(y_out, bufy_val, SDX_BUS_WIDTH_BYTES*SDX_CU_LOCAL_OUT_SIZE);


        y_out += SDX_CU_LOCAL_OUT_SIZE;

        *ker_count = i_ker_count;
        i_ker_count++;
        
        //----------------------------------------------------------------------
        /*
        // WORKING KIND OF, Looks like which word it copies is off
        memcpy(bufa_val, (const sdx_data_t*)_dag, SDX_BUS_WIDTH_BYTES * SDX_CU_LOCAL_IN_SIZE);
        _dag += SDX_CU_LOCAL_IN_SIZE;

        kernel_WRAPPER_label0:for (unsigned int itn_num = 0 ; itn_num < SDX_CU_LOCAL_SIZE; itn_num++) {
        #pragma HLS PIPELINE II=1

            data_t fn_in_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT*NUM_INPUT_KERNEL_FUNCTION_ARGUMENTS];
            #pragma HLS ARRAY_PARTITION variable=fn_in_arg0
            data_t fn_out_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT*NUM_OUTPUT_KERNEL_FUNCTION_ARGUMENTS];
            #pragma HLS ARRAY_PARTITION variable=fn_out_arg0

            for (unsigned int k = 0 ; k < NUM_INPUT_KERNEL_FUNCTION_ARGUMENTS; k++) {
                my_pack_unpack.unpack_sdx_512_data(&bufa_val[(itn_num*NUM_INPUT_KERNEL_FUNCTION_ARGUMENTS)+k], &fn_in_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT*k]);
            }

            kernel_WRAPPER(&fn_in_arg0[0], &fn_out_arg0[0]);

            for (unsigned int k = 0 ; k < NUM_OUTPUT_KERNEL_FUNCTION_ARGUMENTS; k++) {
                my_pack_unpack.pack_sdx_512_data(&bufy_val[(itn_num*NUM_OUTPUT_KERNEL_FUNCTION_ARGUMENTS)+k], &fn_out_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT*k]);
            }
        }

        memcpy(y_out, bufy_val, SDX_BUS_WIDTH_BYTES*SDX_CU_LOCAL_OUT_SIZE);
        y_out += SDX_CU_LOCAL_OUT_SIZE;
        *ker_count = i_ker_count;
        i_ker_count++;
        //----------------------------------------------------------------------
        */


    }
    return;
}

