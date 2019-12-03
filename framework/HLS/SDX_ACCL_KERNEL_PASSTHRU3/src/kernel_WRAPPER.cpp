#include "sdx_cppKernel_top.h"

/*
// ORIGINAL:
void kernel_WRAPPER (data_t in_arg0[SDX_CU_LOCAL_IN_SIZE], data_t out_arg0[SDX_CU_LOCAL_OUT_SIZE]) {
#pragma HLS PIPELINE II=1
    kernel_WRAPPER_label0:for (int index = 0; index < NUM_ELEMENTS_PER_SDX_DATA_BEAT;index++) {
        passthru (&in_arg0[index], &out_arg0[index]);
    }
    return;
}
*/


void kernel_WRAPPER (data_t in_arg0[SDX_CU_LOCAL_IN_SIZE], data_t out_arg0[SDX_CU_LOCAL_OUT_SIZE])
{
#pragma HLS PIPELINE II=1

    kernel_WRAPPER_label0:for (int index = 0; index < SDX_CU_LOCAL_IN_SIZE ;index++) 
    {
        passthru (&in_arg0[4], &in_arg0[5], &in_arg0[6], &in_arg0[7], &in_arg0[8], &in_arg0[9], &in_arg0[10], \
                     &in_arg0[11], &in_arg0[12], &in_arg0[13], &in_arg0[14], &in_arg0[15], \
                        &out_arg0[8], &out_arg0[9], &out_arg0[10], &out_arg0[11], &out_arg0[12], &out_arg0[13], \
                        &out_arg0[14], &out_arg0[15]);
    }


    
    return;
}
