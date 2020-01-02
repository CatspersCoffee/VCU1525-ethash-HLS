#ifndef SDX_PACK_UNPACK_H_
#define SDX_PACK_UNPACK_H_
//
#include "sdx_cppKernel_top.h" 




template <class conv_t> class sdx_pack_unpack {


    private:
        conv_t pack_conv;
        conv_t unpack_conv;

    public:
    uint32_t pack_sdx_512_dataX(sdx_data_t *a, data_t *my_t, uint32_t strip_size) {
    #pragma HLS INLINE
        int count;
        uint32_t ptr_inc = 0;
        count = strip_size;
        //std::cout << "DBG__SRAI count = " <<count << std::endl;
        while (count > 0) {
            for (unsigned int index = 0; index < NUM_ELEMENTS_PER_SDX_DATA_BEAT; index++) {
                if ( count > 0) {
                    pack_conv.my_data_t = *my_t;
                    my_t++;
                    ((*a)(((index*32)+31), (index*32))) =  pack_conv.my_uint32;
                    count--;
                } else {
                    //((*a)(((index*32)+31), (index*32))) = (uint32_t)55; 
                    count--;
		}
           }
           a++;
           ptr_inc++;
       }
        return ptr_inc;
    }

    uint32_t pack_sdx_512_dataX2(sdx_data_t *a, data_t *my_t, uint32_t strip_size) {
    #pragma HLS INLINE
        int count;
        uint32_t ptr_inc = 0;
        count = strip_size;
        //std::cout << "DBG__SRAI count = " <<count << std::endl;
        while (count > 0) {
            for (unsigned int index = 0; index < NUM_ELEMENTS_PER_SDX_DATA_BEAT; index++) {
                if ( count > 0) {
                    pack_conv.my_data_t = *my_t;
                    my_t++;
                    ((*a)(((index*64)+63), (index*64))) =  pack_conv.my_uint32;
                    count--;
                } else {
                    count--;
		}
           }
           a++;
           ptr_inc++;
       }
        return ptr_inc;
    }

    //----------------------
    void rework_hash32(hash32* _p_input, hash32_w* _p_outputW){
        // call with: rework_hash32(hash32* INPUT, hash32_w* OUTPUT)
        // description: reworks a a INPUT type as 32 bytes, into OUTPUT type with 8 uin32_t's.
        uint32_t words[8];
        int bytecount = 0;
        uint32_t word1, word2, word3, word4, word5;
        word5 = 0x00000000;
        for(int i = 0; i<8; i++){
            word1 = (uint32_t)_p_input->b[bytecount];
            word1 = word1 << 24; 
                        //printf("%08x \n", word1);
            bytecount++;
            word2 = (uint32_t)_p_input->b[bytecount];            
            word2 = word2 << 16;
                        //printf("%08x \n", word2);
            bytecount++;
            word3 = (uint32_t)_p_input->b[bytecount];
            word3 = word3 << 8;    
                        //printf("%08x \n", word3);                    
            bytecount++;
            word4 = (uint32_t)_p_input->b[bytecount];
            bytecount++;
            word4 = word4 << 0; 
                        //printf("%08x \n", word4);            
            word5 = word5 | word1;
            word5 = word5 | word2;
            word5 = word5 | word3;
            word5 = word5 | word4;
                    //printf("--> word5 = %08x \n", word5);
            _p_outputW->words[i] =  word5;   
            word5 = 0x00000000;    
        }
    }

    //----------------------
    void rework_hash64_to_node64(hash64* _p_input, node64_w* _p_outputW){
        // call with: rework_hash64(hash64* INPUT, hash64_w* OUTPUT)
        // description: reworks a a INPUT type as 64 bytes, into OUTPUT type with 16 uin32_t's.
        uint32_t words[16];
        int bytecount = 0;
        uint32_t word1, word2, word3, word4, word5;
        word5 = 0x00000000;
        for(int i = 0; i<16; i++){
            word1 = (uint32_t)_p_input->b[bytecount];
            word1 = word1 << 24; 
                        //printf("%08x \n", word1);
            bytecount++;
            word2 = (uint32_t)_p_input->b[bytecount];            
            word2 = word2 << 16;
                        //printf("%08x \n", word2);
            bytecount++;
            word3 = (uint32_t)_p_input->b[bytecount];
            word3 = word3 << 8;    
                        //printf("%08x \n", word3);                    
            bytecount++;
            word4 = (uint32_t)_p_input->b[bytecount];
            bytecount++;
            word4 = word4 << 0; 
                        //printf("%08x \n", word4);            
            word5 = word5 | word1;
            word5 = word5 | word2;
            word5 = word5 | word3;
            word5 = word5 | word4;
                    //printf("--> word5 = %08x \n", word5);
            _p_outputW->words[i] =  word5;   
            word5 = 0x00000000;    
        }
    }

    //----------------------
    void swap_wordbytes_in_node64_w(node64_w* _input, node64_w* _output){
        // take a node64_w and swaps the bytes in each word
        // word[0] = AABBCCDD --> word[0] = DDCCBBAA
        // 
        node64_w outputtemp;
        uint32_t temp_word, aword, bword, cword;
        uint8_t temp_char;
        for (int i = 0 ; i < 16; i++) {   // 16 --> 16 * 32bits = 512bits
            temp_word = _input->words[i];
            cword = 0x00000000;
            for (int j = 0 ; j < 4; j++) {
            bword = 0x00000000;
            aword = temp_word >> (j*8);
            aword = 0x000000FF & aword;
            bword = aword << ((3-j)*8);
            cword = cword | bword;
            }
            outputtemp.words[i] = cword;
        }
        for (int w = 0 ; w < 16; w++) {
            _output->words[w] = outputtemp.words[w];
        }
    }

    //----------------------
    void pack_hash64_to_sdx_512_data(sdx_data_t* _output, hash64_w* _input) {
    #pragma HLS INLINE
        for (unsigned int index = 0; index < 16; index++) {
                pack_conv.my_data_t = _input->words[index];
                ((*_output)(((index*32)+31), (index*32))) =  pack_conv.my_uint32;
            }
    }

    //----------------------
    void unpack_sdx_512_data_to_node64_w(sdx_data_t *a, node64_w* _output) {
    #pragma HLS INLINE
        for (unsigned int index = 0; index < 16; index++) {
            unpack_conv.my_uint32 = ((*a)(((index*32)+31), (index*32)));
            _output->words[index] = unpack_conv.my_data_t;
        }

    }







    void pack_sdx_512_data(sdx_data_t *a, data_t my_t[NUM_ELEMENTS_PER_SDX_DATA_BEAT]) {
    #pragma HLS INLINE
        for (unsigned int index = 0; index < NUM_ELEMENTS_PER_SDX_DATA_BEAT; index++) {
                pack_conv.my_data_t = my_t[index];
                ((*a)(((index*32)+31), (index*32))) =  pack_conv.my_uint32;
            }
    }

    void unpack_sdx_512_data(sdx_data_t *a, data_t my_t[NUM_ELEMENTS_PER_SDX_DATA_BEAT]) {
    #pragma HLS INLINE
        for (unsigned int index = 0; index < NUM_ELEMENTS_PER_SDX_DATA_BEAT; index++) {
            unpack_conv.my_uint32 = ((*a)(((index*32)+31), (index*32)));
            my_t[index] = unpack_conv.my_data_t;
        }

    }

    uint32_t unpack_sdx_512_dataX(sdx_data_t *a, data_t *my_t, uint32_t strip_size) {
    #pragma HLS INLINE
        int count;
        uint32_t ptr_inc = 0;
        count = strip_size;
        while (count > 0) {
            for (unsigned int index = 0; index < NUM_ELEMENTS_PER_SDX_DATA_BEAT; index++) {
                if ( count > 0) {
                    unpack_conv.my_uint32 = ((*a)(((index*32)+31), (index*32)));
                    *my_t = unpack_conv.my_data_t;
                    my_t++;
                    count--;
                } else {
                    //*my_t = (data_t)0;
                    count--;
		}
            }
           a++;
           ptr_inc++;
        }
        return ptr_inc;

    }

    uint32_t unpack_sdx_512_dataX2(sdx_data_t *a, data_t *my_t, uint32_t strip_size) {
    #pragma HLS INLINE
        int count;
        uint32_t ptr_inc = 0;
        count = strip_size;
        while (count > 0) {
            for (unsigned int index = 0; index < NUM_ELEMENTS_PER_SDX_DATA_BEAT; index++) {
                if ( count > 0) {
                    unpack_conv.my_uint32 = ((*a)(((index*64)+64), (index*64)));
                    *my_t = unpack_conv.my_data_t;
                    my_t++;
                    count--;
                } else {
                    //*my_t = (data_t)0;
                    count--;
		}
            }
           a++;
           ptr_inc++;
        }
        return ptr_inc;

    }

    //----------------------
    void unpack_header_to_hash32(sdx_data_t* _input, hash32* _output){
        // call with 
        uint32_t output32_buff[16];
        uint32_t* p_output32_buff = &output32_buff[0];

        unpack_sdx_512_data( _input, p_output32_buff);

        uint32_t temp_word, aword;
        uint8_t temp_char;
        for (int i = 0 ; i < 8; i++) {   // 8 --> 8 * 32bits = 256bits
            temp_word = output32_buff[i];
            for (int j = 0 ; j < 4; j++) {
                aword = temp_word >> (j*8);
                aword = 0x000000FF & aword;
                temp_char = (uint8_t)aword;
                _output->b[(i*4)+(3-j)] = temp_char;
            } 
        }
    }

    //----------------------
    void rework_hash64_w(node64_w* _input, hash64* _output){
        // call with 
        // take a node64_w type, gives back a hash64 type.
        // trades words for bytes.
        uint32_t temp_word, aword;
        uint8_t temp_char;
        for (int i = 0 ; i < 16; i++) {   // 16 --> 16 * 32bits = 512bits
            temp_word = _input->words[i];
            for (int j = 0 ; j < 4; j++) {
            aword = temp_word >> (j*8);
            aword = 0x000000FF & aword;
            temp_char = (uint8_t)aword;
            _output->b[(i*4)+(3-j)] = temp_char;
            }
        }
    }





};
#endif // SDX_PACK_UNPACK_H_
