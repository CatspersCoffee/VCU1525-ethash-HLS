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

/*
static inline uint fnv_hash(const uint x, const uint y) {
	return x*FNV_PRIME ^ y;
}
*/

static inline uint32_t fnv_hash(const uint32_t x, const uint32_t y) {
    #pragma HLS INLINE
	return x*FNV_PRIME ^ y;
}

/*
 * END from fnv.h
 */


//---------------------------------------------------------------------------------------------



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
#define NODE_WORDS (64/4)                   // 16
#define MIX_WORDS (MIX_BYTES/4)             // 32
#define MIX_NODES (MIX_WORDS / NODE_WORDS)  // 2    

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
#define mkapply_ds(NAME, S)                              \
		static inline void NAME(uchar* dst,              \
				const uchar* src,                        \
				size_t len) {                            \
	FOR(i, 1, len, S);                                   \
}
#define mkapply_sd(NAME, S)                              \
		static inline void NAME(const uchar* src,        \
				uchar* dst,                              \
				size_t len) {                            \
	FOR(i, 1, len, S);                                   \
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

void ethash(
		hash32_t* ret_mix,
		hash32_t* ret_hash,              // s+mix
		node64_t* full_nodes,            // dag
		const hash32_t* header_hash,
		const uint nonce,
        uint64_t _DAG_SIZE)
{

	node64_t s_mix[MIX_NODES + 1];
	hash32_t hash;

	//memcpy(s_mix[0].bytes, header_hash, 32);
	for (int i = 0; i < 32/4; i++) {
		s_mix[0].words[i] = header_hash->words[i];
	}

	s_mix[0].double_words[4] = nonce;

	// compute sha3-512 hash and replicate across mix
	//SHA3_512(s_mix->bytes, s_mix->bytes, 40);

	node64_t* mix = s_mix + 1;
	for (unsigned w = 0; w != MIX_WORDS; ++w) {
		mix->words[w] = s_mix[0].words[w % NODE_WORDS];
	}

	unsigned const full_size = (unsigned) _DAG_SIZE;
	unsigned const num_full_pages = (unsigned) (full_size / MIX_BYTES);

	uint index;
	node64_t dag_node;

	for (unsigned i = 0; i != ACCESSES; ++i) {
		index = ((s_mix->words[0] ^ i) * FNV_PRIME ^ mix->words[i % MIX_WORDS]) % num_full_pages;

		for (unsigned n = 0; n != MIX_NODES; ++n) {
			dag_node = full_nodes[MIX_NODES * index + n];

			for (unsigned w = 0; w != NODE_WORDS; ++w) {
				mix[n].words[w] = fnv_hash(mix[n].words[w], dag_node.words[w]);
			}
		}
	}

	// compress mix (length reduced from 128 to 32 bytes)
	for (unsigned w = 0; w != MIX_WORDS; w += 4) {
		uint reduction = mix->words[w + 0];
		reduction = reduction * FNV_PRIME ^ mix->words[w + 1];
		reduction = reduction * FNV_PRIME ^ mix->words[w + 2];
		reduction = reduction * FNV_PRIME ^ mix->words[w + 3];
		mix->words[w / 4] = reduction;
	}

	//memcpy(ret_mix, mix->bytes, 32);
	for (unsigned i = 0; i < 32/4; i++) {
		ret_mix->words[i] = mix->words[i];
	}

	// final Keccak hash
	//SHA3_256(hash.bytes, s_mix->bytes, 64 + 32); // Keccak-256(s + compressed_mix)
	// copy from local mem to global
	for (unsigned i = 0; i < 32/4; i++) {
		ret_hash->words[i] = hash.words[i];
	}
}










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
    sdx_pack_unpack<srai_conv> pack_unpack;

    sdx_data_t input_buff_val[1];
    sdx_data_t output_buff_val[1];
    sdx_data_t header_input_val[1];
    sdx_data_t target_input_val[1];
    sdx_data_t node_bufa_val[1];
    sdx_data_t node_bufy_val[1];
    sdx_data_t hash64output[1];
    sdx_data_t indexoutput[64];
    sdx_data_t dag_val[1];


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
                pack_unpack.unpack_sdx_512_data(&bufa_val[(itn_num*NUM_INPUT_KERNEL_FUNCTION_ARGUMENTS)+k], &fn_in_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT*k]);
            }

            kernel_WRAPPER(&fn_in_arg0[0], &fn_out_arg0[0]);

            for (unsigned int k = 0 ; k < NUM_OUTPUT_KERNEL_FUNCTION_ARGUMENTS; k++) {
                pack_unpack.pack_sdx_512_data(&bufy_val[(itn_num*NUM_OUTPUT_KERNEL_FUNCTION_ARGUMENTS)+k], &fn_out_arg0[NUM_ELEMENTS_PER_SDX_DATA_BEAT*k]);
            }
        }

        memcpy(y_out, bufy_val, SDX_BUS_WIDTH_BYTES*SDX_CU_LOCAL_OUT_SIZE);
        y_out += SDX_CU_LOCAL_OUT_SIZE;
        *ker_count = i_ker_count;
        i_ker_count++;
        */


        

        

        //----------------------------------------------------------------------
        
        sdx_data_t* _dag_head = (sdx_data_t*)_dag;          //save the pointer to the _dag head.
        sdx_data_t* _input_head = (sdx_data_t*)a_in;        //save the pointer to the input head.
        sdx_data_t* _output_head = (sdx_data_t*)y_out;      //save the pointer to the output head.

        //----------------------------------------------------------------------
        // TESTING
        /*
        for (unsigned int itn_num = 0 ; itn_num < 9; itn_num++) {
            memcpy(node_bufa_val, (const sdx_data_t*)_dag, SDX_BUS_WIDTH_BYTES);
            _dag += 1;
            memcpy(y_out, node_bufa_val, SDX_BUS_WIDTH_BYTES);
            y_out += 1;
        }

        hash64 hash_out;
        hash32 hash_in;
        hash64_w hash_outW;
        hash32* p_hash_in = &hash_in;
        hash64* p_hash_out = &hash_out;
        hash64_w* p_hash_outW = &hash_outW;

        for(int i = 0; i<32; i++){
            hash_in.b[i] = 0x00;
        }

	    //SHA3_512(p_hash_out->b, p_hash_in->b, 32);
        
        //uint8_t some_byte = 0x00;
        //for(int i = 0; i<64; i++){
        //    hash_out.b[i] = some_byte;
        //    some_byte++;
        //}
        
        pack_unpack.rework_hash64(p_hash_out, p_hash_outW);
        pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hash_outW);
        memcpy(y_out, hash64output, 64);

        *ker_count = i_ker_count;
        i_ker_count++;
        */
        //----------------------------------------------------------------------
        // TESTING   
        /*
        _dag = _dag_head + 0x005b54c8; 
        y_out = _output_head + 10;  
        memcpy(node_bufa_val, (const sdx_data_t*)_dag, SDX_BUS_WIDTH_BYTES);
        memcpy(y_out, node_bufa_val, SDX_BUS_WIDTH_BYTES);

        _dag = _dag_head + 0x005b54c9; 
        y_out = _output_head + 11;
        memcpy(node_bufa_val, (const sdx_data_t*)_dag, SDX_BUS_WIDTH_BYTES);
        memcpy(y_out, node_bufa_val, SDX_BUS_WIDTH_BYTES);
        */


        //----------------------------------------------------------------------
        // --> read header hash in as 512 bit sdx_data_t, only take 256bit as a 32 byte header.
        // --> get header_hash as 32 bytes add nonce to end to make 40 bytes
        //
        //
        //
        //
        //
        //
        //
        //                                                

        //----------------------------------------------------------------------
        // VARIABLE DECLARATIONS:

		hash32    ret_hash_32b;                      // final SHA3--256 of mix 96 bytes as 32 bytes
		hash64_w  ret_hash_64w;                      // 
		hash32*   p_ret_hash_32b = &ret_hash_32b;    // 
        hash64_w* p_ret_hash_64w = &ret_hash_64w;

		hash64_w ret_mix_64w;

		node64* full_nodes;                     // dag .. pointer to dag node
		hash32_w header_hash;
        hash32_w* p_header_hash = &header_hash;  // header_hash is type hash64_w --> only accepts uint32_t for elements.
        uint32_t DAG_SIZE;
        uint64_t numhashs = 0;
        uint32_t nonce[2];
        uint32_t start_nonce[2];
        uint32_t end_nonce[2];

        hash64 s_mix_temp;
        hash64* p_s_mix_temp = &s_mix_temp;

        //----
        // TESTING VARIABLES:
        hash64_w hashAW;
        hash64_w* p_hashAW = &hashAW;


        //----------------------------------------------------------------------
        // --> read header hash in as 512 bit sdx_data_t, only take 256bit as a 32 byte header.
        hash32 headerhash32;
        hash32* p_headerhash32 = &headerhash32;
        a_in = _input_head + INDEX_IN_header;
        memcpy(header_input_val, (const sdx_data_t*)a_in, SDX_BUS_WIDTH_BYTES);
        pack_unpack.unpack_header_to_hash32(header_input_val, p_headerhash32);      //get unpack header hash from sdx_data_t type to hash32 type
        
        pack_unpack.rework_hash32_to_hash64w(p_headerhash32, p_hashAW);
        pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
        y_out = _output_head + INDEX_OUT_header;
        memcpy(y_out, hash64output, 64);         
        //----------------------------------------------------------------------
        // --> read target in as 512 bit sdx_data_t, only take 256bit as a 32 byte hash32_w.
        hash32_w target;
        hash32_w* p_target = &target;
        a_in = _input_head + INDEX_IN_target;
        memcpy(target_input_val, (const sdx_data_t*)a_in, SDX_BUS_WIDTH_BYTES);
        pack_unpack.unpack_sdx_512_data_to_hash32_w(target_input_val, p_target);      //

        pack_unpack.pack_hash32_w_into_hash64_w(p_target, p_hashAW);
        pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
        y_out = _output_head + INDEX_OUT_target;
        memcpy(y_out, hash64output, 64); 
        //----------------------------------------------------------------------

        //----------------------------------------------------------------------
        // --> read start_nonce and end_nonce in as 512 bit sdx_data_t, only take 64bit as value(s).
        hash64_w nonce_temp;
        hash64_w* p_nonce_temp = &nonce_temp;

        //---- start nonce
        a_in = _input_head + 2;
        memcpy(input_buff_val, (const sdx_data_t*)a_in, SDX_BUS_WIDTH_BYTES);
        pack_unpack.unpack_sdx_512_data_to_hash64_w(input_buff_val, p_nonce_temp);      //
        for(int i = 0; i<16; i++){
            hashAW.words[i] = nonce_temp.words[i];
        }
        for(int i = 0; i<2; i++){
            start_nonce[i] = nonce_temp.words[i];
        }        
        pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
        y_out = _output_head + 15;
        memcpy(y_out, hash64output, 64);         

        //---- end nonce
        a_in = _input_head + 3;
        memcpy(input_buff_val, (const sdx_data_t*)a_in, SDX_BUS_WIDTH_BYTES);
        pack_unpack.unpack_sdx_512_data_to_hash64_w(input_buff_val, p_nonce_temp);      //
        for(int i = 0; i<16; i++){
            hashAW.words[i] = nonce_temp.words[i];
        }
        for(int i = 0; i<2; i++){
            end_nonce[i] = nonce_temp.words[i];
        }          
        pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
        y_out = _output_head + 16;
        memcpy(y_out, hash64output, 64);         


        // setup a nonce value for testing:
        //nonce[0] = 0x00000000ULL;
        //nonce[1] = 0x00000000ULL;     

        //----------------------------------------------------------------------
        /*
        // tesing target comparison
        hash32_w hash32w_A, hash32w_B;
        hash32_w* p_hash32w_A = &hash32w_A;
        hash32_w* p_hash32w_B = &hash32w_B;
        hash32w_A.words[0] = 0x00000000;
        hash32w_A.words[1] = 0x00000000;
        hash32w_A.words[2] = 0x00000000;
        hash32w_A.words[3] = 0x00000000;
        hash32w_A.words[4] = 0x00000000; 
        hash32w_A.words[5] = 0x00000000;
        hash32w_A.words[6] = 0x00000000;
        hash32w_A.words[7] = 0x00000000;

        hash32w_B.words[0] = 0x80000000;
        hash32w_B.words[1] = 0x00000000;
        hash32w_B.words[2] = 0x00000000;
        hash32w_B.words[3] = 0x00000000;
        hash32w_B.words[4] = 0x00000000; 
        hash32w_B.words[5] = 0x00000000;
        hash32w_B.words[6] = 0x00000000;
        hash32w_B.words[7] = 0x00000000;

        //uint8_t cmp_res = pack_unpack.bignum_cmp(p_hash32w_A, p_hash32w_B);
        uint8_t cmp_res = pack_unpack.bignum_cmp(p_hash32w_A, p_target);
        uint32_t cmp_res32 = (uint32_t)cmp_res;

        hashAW.words[i] = cmp_res32;
        for(int i = 1; i<16; i++){
            hashAW.words[i] = 0x00000000; 
        }
        pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
        y_out = _output_head + 22;
        memcpy(y_out, hash64output, 64); 
        */

        //----------------------------------------------------------------------

        //for(numhashs = 0; numhashs < 1; numhashs++){

            node64_w s_mix[MIX_NODES + 1];              // s_mix[3]
            node64_w* p_s_mix = &s_mix[0];
            hash32 final_hash;

            pack_unpack.rework_hash32(p_headerhash32, p_header_hash); 
            for (int i = 0; i < 8; i++) {
                s_mix[0].words[i] = header_hash.words[i];      // header_hash is type hash32_w, s_mix is type node64_w
            }

            s_mix[0].words[8] = start_nonce[0];
            s_mix[0].words[9] = start_nonce[1];

            //--------------------- testing
            //for(int i = 0; i<64; i++){
            //    hashA.b[i] = s_mix[0].bytes[i];
            //}
            //pack_unpack.rework_hash64(p_hashA, p_hashAW);
            //pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            //y_out = _output_head + 2;
            //memcpy(y_out, hash64output, 64);            
            //---------------------^^

            // compute sha3-512 hash and replicate across mix
            pack_unpack.rework_hash64_w(p_s_mix, p_s_mix_temp);
            //SHA3_512(s_mix->bytes, s_mix->bytes, 40); //old original
            SHA3_512(p_s_mix_temp->b, p_s_mix_temp->b, 40);

            /*
            //--------------------- testing
            // artificially setup the mix with known values:  
            s_mix_temp.b[0]=0x35; s_mix_temp.b[1]=0xad; s_mix_temp.b[2]=0xe1; s_mix_temp.b[3]=0xc6;
            s_mix_temp.b[4]=0x43; s_mix_temp.b[5]=0xe7; s_mix_temp.b[6]=0x73; s_mix_temp.b[7]=0xf1;
            s_mix_temp.b[8]=0x00; s_mix_temp.b[9]=0x2a; s_mix_temp.b[10]=0xf1; s_mix_temp.b[11]=0x08;
            s_mix_temp.b[12]=0xdf; s_mix_temp.b[13]=0x78; s_mix_temp.b[14]=0xd3; s_mix_temp.b[15]=0xcf;
            s_mix_temp.b[16]=0x3c; s_mix_temp.b[17]=0x6d; s_mix_temp.b[18]=0x06; s_mix_temp.b[19]=0x9c;
            s_mix_temp.b[20]=0x93; s_mix_temp.b[21]=0xee; s_mix_temp.b[22]=0x14; s_mix_temp.b[23]=0xc2;
            s_mix_temp.b[24]=0x1e; s_mix_temp.b[25]=0x86; s_mix_temp.b[26]=0xd5; s_mix_temp.b[27]=0xc5;
            s_mix_temp.b[28]=0xb9; s_mix_temp.b[29]=0xd8; s_mix_temp.b[30]=0x3c; s_mix_temp.b[31]=0xa4;
            s_mix_temp.b[32]=0xc8; s_mix_temp.b[33]=0xba; s_mix_temp.b[34]=0xf1; s_mix_temp.b[35]=0xf1;
            s_mix_temp.b[36]=0x04; s_mix_temp.b[37]=0x73; s_mix_temp.b[38]=0x7a; s_mix_temp.b[39]=0xa3;
            s_mix_temp.b[40]=0xc2; s_mix_temp.b[41]=0x77; s_mix_temp.b[42]=0x68; s_mix_temp.b[43]=0x20;
            s_mix_temp.b[44]=0x6c; s_mix_temp.b[45]=0xbc; s_mix_temp.b[46]=0x7d; s_mix_temp.b[47]=0x3e;
            s_mix_temp.b[48]=0x63; s_mix_temp.b[49]=0xea; s_mix_temp.b[50]=0xa9; s_mix_temp.b[51]=0x70;
            s_mix_temp.b[52]=0x7f; s_mix_temp.b[53]=0xb1; s_mix_temp.b[54]=0x49; s_mix_temp.b[55]=0xaa;
            s_mix_temp.b[56]=0xaa; s_mix_temp.b[57]=0x0b; s_mix_temp.b[58]=0xd2; s_mix_temp.b[59]=0x64;
            s_mix_temp.b[60]=0x3c; s_mix_temp.b[61]=0xc2; s_mix_temp.b[62]=0xad; s_mix_temp.b[63]=0xef;
            //---------------------^^
            */
            /*
            //--------------------- debug
            hash64_w hash_T1;
            hash_T1.words[0] = 0x01234567;
            hash_T1.words[1] = 0x01010101;
            hash_T1.words[2] = 0x02020202;
            hash_T1.words[3] = 0x33333333;
            hash_T1.words[4] = 0x44444444; 
            hash_T1.words[5] = 0x55555555; 
            hash_T1.words[6] = 0x66666666; 
            hash_T1.words[7] = 0x77777777; 
            hash_T1.words[8] = 0x88888888; 
            hash_T1.words[9] = 0x99999999; 
            hash_T1.words[10] = 0xAAAAAAAA; 
            hash_T1.words[11] = 0xBBBBBBBB; 
            hash_T1.words[12] = 0xCCCCCCCC; 
            hash_T1.words[13] = 0xDDDDDDDD; 
            hash_T1.words[14] = 0xEEEEEEEE; 
            hash_T1.words[15] = 0xFFFFFFFF;

            // put  index[14]
            for(int i = 0; i<16; i++){
                hashAW.words[i] = 0x00000000;
                hashAW.words[i] = hash_T1.words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&output_buff_val[0], p_hashAW);
            y_out = _output_head + 14;
            memcpy(y_out, output_buff_val, 64); 

            hash64_w hash_T2;
            for(int i = 0; i<16; i++){
                hash_T2.words[i] = hash_T1.words[15 - i];
            }
            // put  index[13]
            for(int i = 0; i<16; i++){
                hashAW.words[i] = 0x00000000;
                hashAW.words[i] = hash_T2.words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&output_buff_val[0], p_hashAW);
            y_out = _output_head + 13;
            memcpy(y_out, output_buff_val, 64); 
            //---------------------^^
            */



            pack_unpack.rework_hash64_to_node64(p_s_mix_temp, p_s_mix);
            pack_unpack.swap_wordbytes_in_node64_w(p_s_mix, p_s_mix);

            node64_w* mix = s_mix + 1;
            for (unsigned w = 0; w != MIX_WORDS; ++w) {
                mix->words[w] = s_mix[0].words[w % NODE_WORDS];     // NODE_WORDS = 16. 
            }

            //--------------------- testing
            // s_mix[0] 
            for(int i = 0; i<16; i++){
                hashAW.words[i] = s_mix[0].words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 4;
            memcpy(y_out, hash64output, 64); 
            // s_mix[1]  
            for(int i = 0; i<16; i++){
                hashAW.words[i] = s_mix[1].words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 5;
            memcpy(y_out, hash64output, 64);       
            // s_mix[2]
            for(int i = 0; i<16; i++){
                hashAW.words[i] = s_mix[2].words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 6;
            memcpy(y_out, hash64output, 64);                                
            //---------------------^^


            DAG_SIZE = 1073739904U;
            uint32_t full_size = DAG_SIZE;
            uint32_t num_full_pages = (uint32_t) (full_size / MIX_BYTES);

            uint32_t index;
            node64_w dag_node;
            node64_w* p_dag_node = &dag_node;

            hash64_w indexval;                  // testing
            hash64_w* p_indexval = &indexval;   // testing

            node64_w mixnoden; 

            for (uint8_t i = 0; i != ACCESSES; ++i) {  // ACCESSES = 64
            //for (uint8_t i = 0; i != 1; ++i) {  // testing
                index = ((s_mix->words[0] ^ i) * FNV_PRIME ^ mix->words[i % MIX_WORDS]) % num_full_pages;

                //--------------------- testing
                //indexval.words[0] = index;
                //pack_unpack.pack_hash64_to_sdx_512_data(&indexoutput[0], p_indexval);
                //y_out++;
                //memcpy(y_out, indexoutput, 64);  
                //---------------------^^
                
                for (unsigned n = 0; n != MIX_NODES; ++n) { // MIX_NODES = 2
                    //dag_node = full_nodes[MIX_NODES * index + n];     // Original

                    _dag = _dag_head + (MIX_NODES * index + n); 
                    memcpy(dag_val, (const sdx_data_t*)_dag, SDX_BUS_WIDTH_BYTES);
                    pack_unpack.unpack_sdx_512_data_to_node64_w(&dag_val[0], p_dag_node);

                    //--------------------- testing
                    // dag_node
                    //for(int i = 0; i<16; i++){
                    //    hashAW.words[i] = dag_node.words[i];
                    //}
                    //pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
                    //y_out = _output_head + 3;
                    //memcpy(y_out, hash64output, 64); 
                    //---------------------^^

                    for (unsigned w = 0; w != NODE_WORDS; ++w) { // NODE_WORDS = 16
                        //mix[n].words[w] = fnv_hash(mix[n].words[w], dag_node.words[w]); //original
                        //mix[n].words[w] = fnv_hash(mix[n].words[w], dag_node.words[w]);
                        mixnoden.words[0] = fnv_hash(mix[n].words[w], dag_node.words[w]);
                        mix[n].words[w] = mixnoden.words[0];
                    }
                }
                
            }


            //--------------------- testing
            // copy final s_mix to output memory index[4 through 6]
            // s_mix[0]
            for(int i = 0; i<16; i++){
                hashAW.words[i] = s_mix[0].words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 8;
            memcpy(y_out, hash64output, 64); 
            // s_mix[1]  
            for(int i = 0; i<16; i++){
                hashAW.words[i] = s_mix[1].words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 9;
            memcpy(y_out, hash64output, 64);       
            // s_mix[2]
            for(int i = 0; i<16; i++){
                hashAW.words[i] = s_mix[2].words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 10;
            memcpy(y_out, hash64output, 64);                                
            //---------------------^^


            // compress mix (length reduced from 128 to 32 bytes)
            for (uint8_t w = 0; w != MIX_WORDS; w += 4) {
                uint32_t reduction = mix->words[w + 0];
                reduction = reduction * FNV_PRIME ^ mix->words[w + 1];
                reduction = reduction * FNV_PRIME ^ mix->words[w + 2];
                reduction = reduction * FNV_PRIME ^ mix->words[w + 3];
                mix->words[w / 4] = reduction;
            }


            //--------------------- testing
            // copy final s_mix including compression to output memory index[7 through 9]
            // s_mix[0]
            for(int i = 0; i<16; i++){
                hashAW.words[i] = s_mix[0].words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 11;
            memcpy(y_out, hash64output, 64); 
            // s_mix[1]  
            for(int i = 0; i<16; i++){
                hashAW.words[i] = s_mix[1].words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 12;
            memcpy(y_out, hash64output, 64);       
            // s_mix[2]
            for(int i = 0; i<16; i++){
                hashAW.words[i] = s_mix[2].words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 13;
            memcpy(y_out, hash64output, 64);                                
            //---------------------^^



            //memcpy(ret_mix, mix->bytes, 32);
            //for (unsigned i = 0; i < 32/4; i++) {
            //    ret_mix->words[i] = mix->words[i];
            //}
            for (uint8_t i = 0; i < 8; i++) {
                ret_mix_64w.words[i] = mix->words[i];
            }

            // copy final compressed mix (32byte) to output memory index[23]
            // s_mix[0]
            for(int i = 0; i<16; i++){
                hashAW.words[i] = ret_mix_64w.words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 23;
            memcpy(y_out, hash64output, 64); 


            /*
            //--------------------- testing
            // artificially setup the s_mix + compression with known values (16 + 32 words)::
            // c6e1ad35 f173e743 08f12a00 cfd378df 9c066d3c c214ee93 c5d5861e a43cd8b9 f1f1bac8 a37a7304 206877c2 3e7dbc6c 70a9ea63 aa49b17f 64d20baa efadc23c
            s_mix[0].words[0] = 0xc6e1ad35;
            s_mix[0].words[1] = 0xf173e743;
            s_mix[0].words[2] = 0x08f12a00;
            s_mix[0].words[3] = 0xcfd378df; 
            s_mix[0].words[4] = 0x9c066d3c; 
            s_mix[0].words[5] = 0xc214ee93; 
            s_mix[0].words[6] = 0xc5d5861e; 
            s_mix[0].words[7] = 0xa43cd8b9; 
            s_mix[0].words[8] = 0xf1f1bac8; 
            s_mix[0].words[9] = 0xa37a7304; 
            s_mix[0].words[10] = 0x206877c2; 
            s_mix[0].words[11] = 0x3e7dbc6c; 
            s_mix[0].words[12] = 0x70a9ea63; 
            s_mix[0].words[13] = 0xaa49b17f; 
            s_mix[0].words[14] = 0x64d20baa; 
            s_mix[0].words[15] = 0xefadc23c;
            // a7bca231 68c21ad8 60db764e 3d35be54 938cee69 68c21ad8 60db764e 3d35be54 996dc2c8 8b171704 3fbf09c2 aa96086c 68208563 a77ac87f 6332c5aa 78e85e3c
            s_mix[1].words[0] = 0xa7bca231;
            s_mix[1].words[1] = 0x68c21ad8;
            s_mix[1].words[2] = 0x60db764e;
            s_mix[1].words[3] = 0x3d35be54; 
            s_mix[1].words[4] = 0x938cee69; 
            s_mix[1].words[5] = 0x68c21ad8;
            s_mix[1].words[6] = 0x60db764e;
            s_mix[1].words[7] = 0x3d35be54;
            s_mix[1].words[8] = 0x996dc2c8;
            s_mix[1].words[9] = 0x8b171704;
            s_mix[1].words[10] = 0x3fbf09c2;
            s_mix[1].words[11] = 0xaa96086c;
            s_mix[1].words[12] = 0x68208563;
            s_mix[1].words[13] = 0xa77ac87f;
            s_mix[1].words[14] = 0x6332c5aa;
            s_mix[1].words[15] = 0x78e85e3c;
            // df721607 1a226243 8b2b2a00 0e4aefdf b764093c 4b833993 081fd41e 5fdcb9b9 996dc2c8 8b171704 3fbf09c2 aa96086c 68208563 a77ac87f 6332c5aa 78e85e3c
            s_mix[2].words[0] = 0xdf721607;
            s_mix[2].words[1] = 0x1a226243;
            s_mix[2].words[2] = 0x8b2b2a00;
            s_mix[2].words[3] = 0x0e4aefdf;
            s_mix[2].words[4] = 0xb764093c; 
            s_mix[2].words[5] = 0x4b833993;
            s_mix[2].words[6] = 0x081fd41e; 
            s_mix[2].words[7] = 0x5fdcb9b9;
            s_mix[2].words[8] = 0x996dc2c8;
            s_mix[2].words[9] = 0x8b171704; 
            s_mix[2].words[10] = 0x3fbf09c2; 
            s_mix[2].words[11] = 0xaa96086c;
            s_mix[2].words[12] = 0x68208563; 
            s_mix[2].words[13] = 0xa77ac87f; 
            s_mix[2].words[14] = 0x6332c5aa;
            s_mix[2].words[15] = 0x78e85e3c;
            //---------------------^^
            */



            //---- final Keccak hash:

            //---- turns first 96 bytes for s_mix from word data into byte data 
            mix96  mix_96b;
            mix96* p_mix_96b = &mix_96b;
            //use function that turns node64_w into hash64 --> rework_mix96_w(node64_w* _input, mix96* _output)
            pack_unpack.rework_mix_to_bytes(p_s_mix, p_mix_96b);


            //--------------------- debug
            mix96* bytepointer = p_mix_96b;
            hash64_w testwords;
            hash64_w* p_testwords = &testwords;
            pack_unpack.rework_mix96pointer_to_hash64w(bytepointer, p_testwords, 64, 0);
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_testwords);
            y_out = _output_head + 18;
            memcpy(y_out, hash64output, 64); 
        
            bytepointer = p_mix_96b;
            pack_unpack.rework_mix96pointer_to_hash64w(bytepointer, p_testwords, 32, 64);
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_testwords);
            y_out = _output_head + 19;
            memcpy(y_out, hash64output, 64); 
            //---------------------^^

            /*
            uint8_t somesdfdf = 0x00;   // testing set 96 mix bytes to 0x00
            for(uint8_t i = 0; i<96; i++){
                p_mix_96b->b[i] = somesdfdf;
                //somesdfdf++;
            }
            */

            // call SHA3(output is hash32 (bytes), input is hash64 bytes (p_s_mix_96b .. the s_mix byte pointer), 96 bytes)
            //SHA3_256(hash.bytes, s_mix->bytes, 64 + 32); // Keccak-256(s + compressed_mix)    //original
            SHA3_256(p_ret_hash_32b->b, p_mix_96b->b, 64 + 32); // Keccak-256(s_mix[0] + compressed_mix)

            //for(uint8_t i = 0; i<32; i++){  // debug
            //    p_ret_hash_32b->b[i] = p_mix_96b->b[i];
            //}

            //--------------------- debug
            hash32* bytepointer32 = p_ret_hash_32b;
            hash64_w testwords32;
            hash64_w* p_testwords32 = &testwords32;
            pack_unpack.rework_hash32pointer_to_hash64w(bytepointer32, p_testwords32, 32, 0);
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_testwords32);
            y_out = _output_head + 21;
            memcpy(y_out, hash64output, 64); 
        
            //---------------------^^



            //---- unpack the 32 bytes to a hash64_w .. have to write WRITE:  pack_unpack.rework_hash32_to_hash64(p_ret_hash_32b, p_ret_hash_64w);
            pack_unpack.rework_hash32_to_hash64w(p_ret_hash_32b, p_ret_hash_64w);
            /*
            //--------------------- testing
            // copy un-byteswapped hash to index[11]
            for(uint8_t i = 0; i<16; i++){
                hashAW.words[i] = ret_hash_64w.words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 12;
            memcpy(y_out, hash64output, 64); 
            //---------------------^^
            */
            // run: pack_unpack.swap_wordbytes_in_node64_w(p_ret_hash_64w, p_ret_hash_64w);
            pack_unpack.swap_wordbytes_in_hash64_w(p_ret_hash_64w, p_ret_hash_64w);

            // copy from local mem to global
            //for (unsigned i = 0; i < 32/4; i++) {   // original
            //    ret_hash->words[i] = hash.words[i];
            //}

            
            //---- copy final hash (32bytes, incapsulated in hash64_w type) to output memory index[15]
            for(uint8_t i = 0; i<16; i++){
                hashAW.words[i] = ret_hash_64w.words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 24;
            memcpy(y_out, hash64output, 64); 
           

            //---- target comparison:
            //uint8_t cmp_res = pack_unpack.bignum_cmp(p_hash32w_A, p_hash32w_B);
            hash32_w ret_hash_32w;
            hash32_w* p_ret_hash_32w = &ret_hash_32w;
            for(uint8_t i = 0; i<8; i++){
                ret_hash_32w.words[i] = ret_hash_64w.words[i];
            }
            
            uint8_t cmp_res = pack_unpack.bignum_cmp(p_ret_hash_32w, p_target);
            uint32_t cmp_res32 = (uint32_t)cmp_res;

            hashAW.words[i] = cmp_res32;
            for(int i = 1; i<16; i++){
                hashAW.words[i] = 0x00000000; 
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 22;
            memcpy(y_out, hash64output, 64);             



    //} // for numhashs loop end




        //----------------------------------------------------------------------
        


    }
    return;
}


/*

Verifying results .............. [ Big-Endian ]


---- [00] ----> s_mix[0] start:
c6e1ad35 f173e743 08f12a00 cfd378df 9c066d3c c214ee93 c5d5861e a43cd8b9 f1f1bac8 a37a7304 206877c2 3e7dbc6c 70a9ea63 aa49b17f 64d20baa efadc23c 
---- [01] ----> s_mix[1] start:
c6e1ad35 f173e743 08f12a00 cfd378df 9c066d3c c214ee93 c5d5861e a43cd8b9 f1f1bac8 a37a7304 206877c2 3e7dbc6c 70a9ea63 aa49b17f 64d20baa efadc23c 
---- [02] ----> s_mix[2] start:
c6e1ad35 f173e743 08f12a00 cfd378df 9c066d3c c214ee93 c5d5861e a43cd8b9 f1f1bac8 a37a7304 206877c2 3e7dbc6c 70a9ea63 aa49b17f 64d20baa efadc23c 
---- [03] 
00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
---- [04] ----> s_mix[0] final:
c6e1ad35 f173e743 08f12a00 cfd378df 9c066d3c c214ee93 c5d5861e a43cd8b9 f1f1bac8 a37a7304 206877c2 3e7dbc6c 70a9ea63 aa49b17f 64d20baa efadc23c 
---- [05] ----> s_mix[1] final:
4a0785cd 5c750e79 13a51e00 08e7470d da1df574 19f39169 8d22213a 44c92b3b a78908d8 5dc30f4c c6748666 cbef9e04 be7bf9d9 91026aed 60a85c9e 8a88c474 
---- [06] ----> s_mix[2] final:
4a0785cc 5c750e79 13a51e00 08e7470d da1df574 19f39169 8d22213a 44c92b3b a78908d8 5dc30f4c c6748666 cbef9e04 be7bf9d9 91026aed 60a85c9e 8a88c474 
---- [07] ----> s_mix[0] final including compression:
c6e1ad35 f173e743 08f12a00 cfd378df 9c066d3c c214ee93 c5d5861e a43cd8b9 f1f1bac8 a37a7304 206877c2 3e7dbc6c 70a9ea63 aa49b17f 64d20baa efadc23c 
---- [08] ----> s_mix[1] final including compression:
cdebc673 a8d6f5b4 b23112da 7dcb15a0 9ac6af28 a8d6f5b4 b23112da 7dcb15a0 a78908d8 5dc30f4c c6748666 cbef9e04 be7bf9d9 91026aed 60a85c9e 8a88c474 
---- [09] ----> s_mix[2] final including compression:
4a0785cc 5c750e79 13a51e00 08e7470d da1df574 19f39169 8d22213a 44c92b3b a78908d8 5dc30f4c c6748666 cbef9e04 be7bf9d9 91026aed 60a85c9e 8a88c474 
---- [10] 
00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
---- [11] 
00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
---- [12] 
46700b4d 40ac5c35 af2c22dd a2787a91 eb567b06 c924a8fb 8ae9a05b 20c08c21 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
---- [13] 
00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
---- [14] ----> final compressed mix (32byte):
cdebc673 a8d6f5b4 b23112da 7dcb15a0 9ac6af28 a8d6f5b4 b23112da 7dcb15a0 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
---- [15] ----> final mix hash (output hash)(32byte):
4d0b7046 355cac40 dd222caf 917a78a2 067b56eb fba824c9 5ba0e98a 218cc020 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
---- [16] 
00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
---- [17] ----> mix96 bytes 0-63 before:
c6e1ad35 f173e743 08f12a00 cfd378df 9c066d3c c214ee93 c5d5861e a43cd8b9 f1f1bac8 a37a7304 206877c2 3e7dbc6c 70a9ea63 aa49b17f 64d20baa efadc23c 
---- [18] ----> mix96 bytes 64-96 before:
cdebc673 a8d6f5b4 b23112da 7dcb15a0 9ac6af28 a8d6f5b4 b23112da 7dcb15a0 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
---- [19] 
00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
---- [20] ----> ret_hash bytes 0-32 after:
46700b4d 40ac5c35 af2c22dd a2787a91 eb567b06 c924a8fb 8ae9a05b 20c08c21 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
---- [21] 
00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
---- [22] 
00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
---- [23] 
3fffffff ffffffff 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
------------   End  ----------------------------------------------------------------------------------------

result   = 46700b4d40ac5c35af2c22dda2787a91eb567b06c924a8fb8ae9a05b20c08c21

*/













/*  BACKUP of ORIGINAL


        for(numhashs = 0; numhashs < 1; numhashs++){

            node64_t s_mix[MIX_NODES + 1];
            hash32_t hash;

            //memcpy(s_mix[0].bytes, header_hash, 32);
            for (int i = 0; i < 32/4; i++) {
                s_mix[0].words[i] = header_hash->words[i];
            }

            s_mix[0].double_words[4] = nonce;

            // compute sha3-512 hash and replicate across mix
            //SHA3_512(s_mix->bytes, s_mix->bytes, 40);

            node64_t* mix = s_mix + 1;
            for (unsigned w = 0; w != MIX_WORDS; ++w) {
                mix->words[w] = s_mix[0].words[w % NODE_WORDS];
            }

            unsigned const full_size = (unsigned) _DAG_SIZE;
            unsigned const num_full_pages = (unsigned) (full_size / MIX_BYTES);

            uint index;
            node64_t dag_node;

            for (unsigned i = 0; i != ACCESSES; ++i) {
                index = ((s_mix->words[0] ^ i) * FNV_PRIME ^ mix->words[i % MIX_WORDS]) % num_full_pages;

                for (unsigned n = 0; n != MIX_NODES; ++n) {
                    dag_node = full_nodes[MIX_NODES * index + n];

                    for (unsigned w = 0; w != NODE_WORDS; ++w) {
                        mix[n].words[w] = fnv_hash(mix[n].words[w], dag_node.words[w]);
                    }
                }
            }

            // compress mix (length reduced from 128 to 32 bytes)
            for (unsigned w = 0; w != MIX_WORDS; w += 4) {
                uint reduction = mix->words[w + 0];
                reduction = reduction * FNV_PRIME ^ mix->words[w + 1];
                reduction = reduction * FNV_PRIME ^ mix->words[w + 2];
                reduction = reduction * FNV_PRIME ^ mix->words[w + 3];
                mix->words[w / 4] = reduction;
            }

            //memcpy(ret_mix, mix->bytes, 32);
            for (unsigned i = 0; i < 32/4; i++) {
                ret_mix->words[i] = mix->words[i];
            }

            // final Keccak hash
            //SHA3_256(hash.bytes, s_mix->bytes, 64 + 32); // Keccak-256(s + compressed_mix)
            // copy from local mem to global
            for (unsigned i = 0; i < 32/4; i++) {
                ret_hash->words[i] = hash.words[i];
            }
        }

*/

