# Created : 9:31:38, Tue Jun 21, 2016 : Sanjay Rai
# Modified: 1200, 7th August 2019 : Catsper
#
#   Name: vivado_batch_42_shell2.tcl
#

source ../device_type.tcl

set TOP_module VU9P_AXI_ICAP_PR_DESIGN_top
set ROLE_CLK_PERIOD 4.000
#set NORTH_ROLE_NAME IP_SDX_ACCL_MATRIX_MULT_6X6
#set NORTH_ROLE_NAME IP_SDX_ACCL_KERNEL_FP_VECTOR_MULT
#set NORTH_ROLE_NAME IP_SDX_ACCL_MATRIX_INVERT_5X5_CPP_KERNEL
#set NORTH_ROLE_NAME IP_SDX_ACCL_MATRIX_INVERT_4X4_CPP_KERNEL
set NORTH_ROLE_NAME IP_SDX_ACCL_KERNEL_PASSTHRU

# Set the project name
set _xil_proj_name_ "project_40_north"

source ../device_type.tcl




#----- MESSAGE -----
common::send_msg_id "START-1" "INFO" "starting: synth_hls_pr_NORTH "



proc synth_hls_pr_NORTH {ARGV_0} { 

upvar 1 $ARGV_0 ROLE_NAME

    # Set the project name
    set _xil_proj_name_ "project_42_north"
    
    #create_project -in_memory -part [DEVICE_TYPE] 
    create_project ${_xil_proj_name_} ./${_xil_proj_name_} -part [DEVICE_TYPE] 



    read_bd "../IP/role/$ROLE_NAME/HLS_PR_SDX_SRAI/HLS_PR_SDX_SRAI.bd"

    read_ip " ../IP/role/debug_bridge_PR/debug_bridge_PR.xci"
    #read_ip " ../IP/role/ila_0/ila_0.xci"

    read_verilog {
    ../src/srai_accel_intfc.sv
    ../src/role_NORTH/role_NORTH.sv
    }

    read_xdc -mode out_of_context -ref role_NORTH  ../src/role_NORTH/xdc/role_NORTH.xdc

    set_param general.maxThreads 7

    synth_design -keep_equivalent_registers -shreg_min_size 8 -include_dirs ../src -top role_NORTH -verilog_define XSDB_SLV_DIS -mode out_of_context  -part [DEVICE_TYPE] 
    
    opt_design -verbose -directive Explore

    write_checkpoint -force ../checkpoints/role_NORTH.$ROLE_NAME.post_synth_opt.dcp
    close_project
}



#----- MESSAGE -----
common::send_msg_id "START-2" "INFO" "starting: build_design project_F"


proc build_design {ARGV_0 ARGV_1 ARGV_2} { 
upvar 1 $ARGV_0 TOP_module
upvar 1 $ARGV_1 NORTH_ROLE_NAME
upvar 1 $ARGV_2 ROLE_CLK_PERIOD

    # Set the project name
    set _xil_proj_name_ "project_42_shell"

    #create_project -in_memory -part [DEVICE_TYPE] 
    create_project ${_xil_proj_name_} ./${_xil_proj_name_} -part [DEVICE_TYPE] 



    read_bd {
    ../IP/shell/PCIe_Bridge_ICAP_complex/PCIe_Bridge_ICAP_complex.bd
    }

    read_verilog "
    ../src/srai_accel_intfc.sv
    ../src/role_NORTH_BB.sv
    ../src/shell_top.sv
    ../src/$TOP_module.sv
    "

    read_ip " ../IP/shell/ila_ker_count/ila_ker_count.xci"

    read_xdc "
    ../src/xdc/DDR4_memory_physical_constraints.xdc
    ../src/xdc/$TOP_module.xdc
    "

    set_param general.maxThreads 11




}
synth_hls_pr_NORTH NORTH_ROLE_NAME
build_design TOP_module NORTH_ROLE_NAME ROLE_CLK_PERIOD 