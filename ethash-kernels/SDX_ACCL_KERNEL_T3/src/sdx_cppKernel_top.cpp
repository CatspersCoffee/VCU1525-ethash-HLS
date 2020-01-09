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
    sdx_data_t nfp_input_val[1];

    sdx_data_t node_bufa_val[1];
    sdx_data_t node_bufy_val[1];
    sdx_data_t hash64output[1];
    sdx_data_t indexoutput[64];
    sdx_data_t dag_val[1];


    sdx_cppKernel_top_local_data_loop:for (unsigned int i = 0; i < NUMBER_OF_DATA_SETS_t; i++) {

    #pragma HLS LOOP_TRIPCOUNT min=HLS_SDX_WRAPPER_TRIP_COUNT max=HLS_SDX_WRAPPER_TRIP_COUNT
    #pragma HLS PIPELINE II=HLS_SDX_WRAPPER_II
        
        kernel_WRAPPER_label0:for (unsigned int itn_num = 0 ; itn_num < 2; itn_num++){
            //----------------------------------------------------------------------
            
            sdx_data_t* _dag_head = (sdx_data_t*)_dag;          //save the pointer to the _dag head.
            sdx_data_t* _input_head = (sdx_data_t*)a_in;        //save the pointer to the input head.
            sdx_data_t* _output_head = (sdx_data_t*)y_out;      //save the pointer to the output head.


            //----------------------------------------------------------------------
            // VARIABLE DECLARATIONS:

            hash32    ret_hash_32b;                      // final SHA3--256 of mix 96 bytes as 32 bytes
            hash64_w  ret_hash_64w;                      // 
            hash32*   p_ret_hash_32b = &ret_hash_32b;    // 
            hash64_w* p_ret_hash_64w = &ret_hash_64w;

            hash64_w ret_mix_64w;

            node64* full_nodes;                         // dag .. pointer to dag node
            hash32_w header_hash;
            hash32_w* p_header_hash = &header_hash;     // header_hash is type hash64_w --> only accepts uint32_t for elements.
            uint32_t DAG_SIZE;
            uint64_t numhashs = 0;
            uint64_t nontt;                             // number of nonces to try.
            uint32_t nonce[2];
            uint32_t start_nonce[2];
            uint32_t end_nonce[2];
            uint32_t current_nonce[2];
            uint64_t currentnonce64;
            uint64_t endnonce64;

            hash64 s_mix_temp;
            hash64* p_s_mix_temp = &s_mix_temp;

            uint32_t num_full_pages;
            uint32_t index;
            node64_w dag_node;
            node64_w* p_dag_node = &dag_node;

            hash64_w indexval;                          // testing
            hash64_w* p_indexval = &indexval;           // testing

            node64_w mixnoden; 

            uint32_t cmp_res;

            hash64_w soln_nonce;                        // solution nonce hash64_w variable
            hash64_w* p_soln_nonce = &soln_nonce;  
            hash64_w soln_mix;                         // solution mixh hash64_w variable
            hash64_w* p_soln_mix = &soln_mix;  
            hash64_w soln_mixhash;                      // solution mixhash hash64_w variable
            hash64_w* p_soln_mixhash = &soln_mixhash;  
        
            uint32_t number_of_solutions;               // number of solutions (for this start/end nonce)
            hash64_w soln_numofsolns;                      // number of solutions hash64_w variable
            hash64_w* p_soln_numofsolns = &soln_numofsolns;          
            //----
            // TESTING VARIABLES:
            hash64_w hashAW;
            hash64_w* p_hashAW = &hashAW;

            hash64_w temp64wA;
            hash64_w* p_temp64wA = &temp64wA;


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
            // --> read number of full pages in dag as 512 bit sdx_data_t, only take 32bit as uin32_t, put it in num_full_pages.
            hash32_w nfp;
            hash32_w* p_nfp = &nfp;
            a_in = _input_head + INDEX_IN_num_full_pages;
            memcpy(nfp_input_val, (const sdx_data_t*)a_in, SDX_BUS_WIDTH_BYTES);
            pack_unpack.unpack_sdx_512_data_to_hash32_w(nfp_input_val, p_nfp);      //
            num_full_pages = nfp.words[1];
            //num_full_pages = 8388593;
            pack_unpack.pack_hash32_w_into_hash64_w(p_nfp, p_hashAW);
            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + INDEX_OUT_num_full_pages;
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


   

            //----------------------------------------------------------------------


            current_nonce[0] = start_nonce[0];
            current_nonce[1] = start_nonce[1];

            pack_unpack.nonce32_to_nonce64(&current_nonce[0], &currentnonce64);
            pack_unpack.nonce32_to_nonce64(&end_nonce[0], &endnonce64);

            //nontt = endnonce64 - currentnonce64;        // currentnonce = startnonce here, endnonce - start nonce = total number of nonces to try.
            nontt = (currentnonce64 - endnonce64) + 1 ;       // currentnonce = startnonce(highest), endnonce (lowest) = total number of nonces to try.
            numhashs = 0x0000000000000000;              // e.g., end = 1, start = 0, nonntt = 1 - 0 = 1

            number_of_solutions = 0x00000000;



                node64_w s_mix[MIX_NODES + 1];              // s_mix[3]
                node64_w* p_s_mix = &s_mix[0];
                hash32 final_hash;

                pack_unpack.rework_hash32(p_headerhash32, p_header_hash); 
                for (int i = 0; i < 8; i++) {
                    s_mix[0].words[i] = header_hash.words[i];      // header_hash is type hash32_w, s_mix is type node64_w
                }

                s_mix[0].words[8] = current_nonce[0];
                s_mix[0].words[9] = current_nonce[1];

                //--------------------- testing
                for(int i = 0; i<16; i++){ // put header + nonce in OUT index[3]
                    temp64wA.words[i] = s_mix[0].words[i];
                }
                pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_temp64wA);
                y_out = _output_head + 3;
                memcpy(y_out, hash64output, 64);            
                //---------------------^^


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


                pack_unpack.rework_hash64_to_node64(p_s_mix_temp, p_s_mix);
                pack_unpack.swap_wordbytes_in_node64_w(p_s_mix, p_s_mix);

                //----> replicate across mix
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





        }

        *ker_count = i_ker_count;
        i_ker_count++;
    }
    return;
}
*/


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
    sdx_data_t nfp_input_val[1];

    sdx_data_t node_bufa_val[1];
    sdx_data_t node_bufy_val[1];
    sdx_data_t hash64output[1];
    sdx_data_t indexoutput[64];
    sdx_data_t dag_val[1];

    sdx_data_t* _dag_head = (sdx_data_t*)_dag;          //save the pointer to the _dag head.
    sdx_data_t* _input_head = (sdx_data_t*)a_in;        //save the pointer to the input head.
    sdx_data_t* _output_head = (sdx_data_t*)y_out;      //save the pointer to the output head.




    sdx_cppKernel_top_local_data_loop:for (unsigned int i = 0; i < NUMBER_OF_DATA_SETS_t; i++) {
    //#pragma HLS LOOP_TRIPCOUNT min=HLS_SDX_WRAPPER_TRIP_COUNT max=HLS_SDX_WRAPPER_TRIP_COUNT
    //#pragma HLS PIPELINE II=HLS_SDX_WRAPPER_II
    #pragma HLS LOOP_TRIPCOUNT min=1 max=100
    #pragma HLS PIPELINE II=1


        kernel_WRAPPER_label0:for (unsigned int itn_num = 0 ; itn_num < 1; itn_num++) {
        #pragma HLS PIPELINE II=1

            hash32 headerhash32;
            hash32* p_headerhash32 = &headerhash32;
            hash64_w hashAW;
            hash64_w* p_hashAW = &hashAW;

            //----------------------------------------------------------------------
            // --> read header hash in as 512 bit sdx_data_t, only take 256bit as a 32 byte header.

            a_in = _input_head + INDEX_IN_header;
            memcpy(header_input_val, (const sdx_data_t*)a_in, SDX_BUS_WIDTH_BYTES);
            pack_unpack.unpack_header_to_hash32(header_input_val, p_headerhash32);      //get unpack header hash from sdx_data_t type to hash32 type
            
            pack_unpack.rework_hash32_to_hash64w(p_headerhash32, p_hashAW);

                hashAW.words[8] = 0x01234567;
                hashAW.words[9] = 0x89ABCDEF;
                hashAW.words[10] = 0x01020304;
                hashAW.words[11] = 0x05060708;
                hashAW.words[12] = 0x090A0B0C;
                hashAW.words[13] = 0x0D0E0F00;
                hashAW.words[14] = 0xE1E2E3E4;
                hashAW.words[15] = 0xF1F2F3F4;

            pack_unpack.pack_hash64_to_sdx_512_data(&hash64output[0], p_hashAW);
            y_out = _output_head + INDEX_OUT_header;
            memcpy(y_out, hash64output, 64);         
            //----------------------------------------------------------------------




        }

        *ker_count = i_ker_count;
        i_ker_count++;
    }
    return;
}



/*


*/