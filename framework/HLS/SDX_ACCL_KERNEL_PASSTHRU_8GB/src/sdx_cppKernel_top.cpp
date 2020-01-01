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

    sdx_data_t header_input_val[1];
    sdx_data_t node_bufa_val[1];
    sdx_data_t node_bufy_val[1];
    sdx_data_t hash64output[1];

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

        _dag = _dag_head + 0x005b54c8; 
        y_out = _output_head + 0;  
        memcpy(node_bufa_val, (const sdx_data_t*)_dag, SDX_BUS_WIDTH_BYTES);
        memcpy(y_out, node_bufa_val, SDX_BUS_WIDTH_BYTES);

        _dag = _dag_head + 0x005b54c9; 
        y_out = _output_head + 1;
        memcpy(node_bufa_val, (const sdx_data_t*)_dag, SDX_BUS_WIDTH_BYTES);
        memcpy(y_out, node_bufa_val, SDX_BUS_WIDTH_BYTES);





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
        // --> read header hash in as 512 bit sdx_data_t, only take 256bit as a 32 byte header.
        a_in = _input_head + 0;
        memcpy(header_input_val, (const sdx_data_t*)a_in, SDX_BUS_WIDTH_BYTES);
        hash32 headerhash32;
        hash32* p_headerhash32 = &headerhash32;

        pack_unpack.unpack_header_to_hash32(header_input_val, p_headerhash32);      //get unpack header hash from sdx_data_t type to hash32 type
        
        /*
        headerhash32.b[0] = 0xAA;
        headerhash32.b[4] = 0xBB;
        headerhash32.b[8] = 0xCC;
        headerhash32.b[12] = 0xDD;        
        hash64 hash_out;
        hash64_w hash_outW;
        hash64* p_hash_out = &hash_out;
        hash64_w* p_hash_outW = &hash_outW;
        for(int i =0; i <32; i++){
            hash_out.b[i] = headerhash32.b[i];
        }

        pack_unpack.rework_hash64(p_hash_out, p_hash_outW);
        pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hash_outW);
        y_out = _output_head + 2;
        memcpy(y_out, hash64output, 64);
        //memcpy(y_out, header_input_val, SDX_BUS_WIDTH_BYTES);   
        */


        //----------------------------------------------------------------------
		hash32 ret_mix;
		hash32 ret_hash;                        // s+mix
		node64* full_nodes;                     // dag .. pointer to dag node
		hash32_w header_hash;
        hash32_w* p_header_hash = &header_hash;  // header_hash is type hash64_w --> only accepts uint32_t for elements.
        uint64_t DAG_SIZE;
        uint64_t numhashs = 0;
        uint32_t nonce[2];

        hash64 s_mix_temp;
        hash64* p_s_mix_temp = &s_mix_temp;

        //----
        // TESTING VARIABLES:
        hash64_w hashAW;
        hash64_w* p_hashAW = &hashAW;

        //----------------------------------------------------------------------
        // setup a nonce value for testing:


        //nonce[0] = 0x00000000ULL;
        //nonce[1] = 0x00000000ULL;

        nonce[0] = 0xAABBCCDDULL;
        nonce[1] = 0x00000000ULL;        
        //----------------------------------------------------------------------

        for(numhashs = 0; numhashs < 1; numhashs++){

            node64_w s_mix[MIX_NODES + 1];              // s_mix[3]
            node64_w* p_s_mix = &s_mix[0];
            hash32 final_hash;

            pack_unpack.rework_hash32(p_headerhash32, p_header_hash); 
            for (int i = 0; i < 8; i++) {
                s_mix[0].words[i] = header_hash.words[i];      // header_hash is type hash32_w, s_mix is type node64_w
            }

            s_mix[0].words[8] = nonce[0];
            s_mix[0].words[9] = nonce[1];

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
            //SHA3_512(s_mix->bytes, s_mix->bytes, 40);
            //SHA3_512(p_s_mix_temp->b, p_s_mix_temp->b, 40);
            pack_unpack.rework_hash64_to_node64(p_s_mix_temp, p_s_mix);

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
            y_out = _output_head + 2;
            memcpy(y_out, hash64output, 64); 
            // s_mix[1]  
            for(int i = 0; i<16; i++){
                hashAW.words[i] = s_mix[1].words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 3;
            memcpy(y_out, hash64output, 64);       
            // s_mix[2]
            for(int i = 0; i<16; i++){
                hashAW.words[i] = s_mix[2].words[i];
            }
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + 4;
            memcpy(y_out, hash64output, 64);                                
            //---------------------^^




            unsigned const full_size = (unsigned) DAG_SIZE;
            unsigned const num_full_pages = (unsigned) (full_size / MIX_BYTES);

            uint index;
            node64_t dag_node;
/*
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
*/

        }




        //----------------------------------------------------------------------
        


    }
    return;
}


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

