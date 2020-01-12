# Created : 16:59:35, Fri May 20, 2017 : Sanjay Rai
# Modified: 1224, 25th July 2019 : Catsper
# Modified: 1526, 8th Nov 2019 : Catsper

.PHONY: all

all: help

help:
	@echo "		make build_complete | build_all | clean | clean_all | build_A"

build_complete:
	cd HLS; make HLS_IP  
	cd IP/role; make create_all_ip 
	cd vivado_batch_shell;vivado -mode batch -source vivado_batch.tcl;cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell  
	cd vivado_batch_shell;cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell  
	cd vivado_batch_SDX_ACCL_MATRIX_INVERT_4X4_CPP_KERNEL;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role
	cd vivado_batch_SDX_ACCL_MATRIX_INVERT_5X5_CPP_KERNEL;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role
	cd vivado_batch_SDX_ACCL_MATRIX_MULT_6X6;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role
	cd vivado_batch_SDX_ACCL_MATRIX_MULT_8X8;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role
	cd vivado_batch_SDX_ACCL_KERNEL_PASSTHRU;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role
	cd vivado_batch_SDX_ACCL_KERNEL_FP_VECTOR_MULT;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role
	tar -zcvf VU9P_AXI_ICAP_PR_DESIGN_bitfiles.tar.gz bitfiles 
build_all:
	cd IP/role; make create_all_ip 
	cd vivado_batch_shell;vivado -mode batch -source vivado_batch.tcl;cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell  
	cd vivado_batch_shell;cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell  
	cd vivado_batch_SDX_ACCL_MATRIX_INVERT_4X4_CPP_KERNEL;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role
	cd vivado_batch_SDX_ACCL_MATRIX_INVERT_5X5_CPP_KERNEL;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role
	cd vivado_batch_SDX_ACCL_MATRIX_MULT_6X6;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role
	cd vivado_batch_SDX_ACCL_MATRIX_MULT_8X8;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role
	cd vivado_batch_SDX_ACCL_KERNEL_PASSTHRU;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role
	cd vivado_batch_SDX_ACCL_KERNEL_FP_VECTOR_MULT;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role
	tar -zcvf VU9P_AXI_ICAP_PR_DESIGN_bitfiles.tar.gz bitfiles 
clean:
	#-cd checkpoints; rm *.dcp
	-rm VU9P_AXI_ICAP_PR_DESIGN_bitfiles.tar.gz 
	#-cd bitfiles/shell; rm VU9P_AXI_ICAP_PR_DESIGN*.*
	#-cd bitfiles/role; rm VU9P_AXI_ICAP_PR_DESIGN*.*
	#-cd vivado_batch_shell;./mk_clean.bat
	-cd vivado_batch_SDX_ACCL_KERNEL_FP_VECTOR_MULT;./mk_clean.bat
	-cd vivado_batch_SDX_ACCL_MATRIX_INVERT_4X4_CPP_KERNEL;./mk_clean.bat
	-cd vivado_batch_SDX_ACCL_MATRIX_INVERT_5X5_CPP_KERNEL;./mk_clean.bat
	-cd vivado_batch_SDX_ACCL_MATRIX_MULT_6X6;./mk_clean.bat
	-cd vivado_batch_SDX_ACCL_MATRIX_MULT_8X8;./mk_clean.bat
	-cd vivado_batch_SDX_ACCL_KERNEL_PASSTHRU;./mk_clean.bat
clean_all:
	-cd HLS; make clean_all  
	-cd HLS; make HLS_IP_CLEAN 
	-cd IP/shell; ./mk_clean.bat 
	-cd IP/role; make clean_all 
	-cd checkpoints; rm *.dcp
	-rm VU9P_AXI_ICAP_PR_DESIGN_bitfiles.tar.gz 
	-cd bitfiles/shell; rm VU9P_AXI_ICAP_PR_DESIGN*.*
	-cd bitfiles/role; rm VU9P_AXI_ICAP_PR_DESIGN*.*
	-cd vivado_batch_shell;./mk_clean.bat
	-cd vivado_batch_SDX_ACCL_KERNEL_FP_VECTOR_MULT;./mk_clean.bat
	-cd vivado_batch_SDX_ACCL_MATRIX_INVERT_4X4_CPP_KERNEL;./mk_clean.bat
	-cd vivado_batch_SDX_ACCL_MATRIX_INVERT_5X5_CPP_KERNEL;./mk_clean.bat
	-cd vivado_batch_SDX_ACCL_MATRIX_MULT_6X6;./mk_clean.bat
	-cd vivado_batch_SDX_ACCL_MATRIX_MULT_8X8;./mk_clean.bat
	-cd vivado_batch_SDX_ACCL_KERNEL_PASSTHRU;./mk_clean.bat

copy_bits:
	cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell  
	cd vivado_batch_shell;cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell 

build_0:
	cd HLS; make HLS_PASSTHRU 
	cd IP/role; make create_only_PASSTHRU_ip
	cd vivado_batch_shell;vivado -mode batch -source vivado_batch_2.tcl;cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell  
	cd vivado_batch_shell;cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell 
	cd vivado_batch_SDX_ACCL_KERNEL_PASSTHRU;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role
build_A:
	cd HLS; make HLS_PASSTHRU 
	cd IP/role; make create_only_PASSTHRU_ip

build_B:
	cd HLS; make HLS_PASSTHRU 

build_C:
	cd HLS; make HLS_PASSTHRU 
	cd IP/role; create_PASSTHUR_blah4

build_D:
	cd HLS; make HLS_PASSTHRU 
	cd IP/role; create_PASSTHUR_blah4
	cd vivado_batch_shell;vivado -mode batch -source vivado_batch_3.tcl;cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell  
	cd vivado_batch_shell;cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell 
	#cd vivado_batch_SDX_ACCL_KERNEL_PASSTHRU;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role

build_F:
	#source ./device_type.tcl
	#source ./bins/set_bins.tcl
	cd vivado_batch_shell;vivado -mode batch -source vivado_batch_35_6.tcl;cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell 
	cd vivado_batch_shell;cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell

build_G:
	cd HLS; make HLS_PASSTHRU 
	cd IP/role; create_PASSTHUR_blah4
	cd vivado_batch_shell;vivado -mode batch -source vivado_batch_3.tcl;cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell  
	cd vivado_batch_shell;cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell 
	#cd vivado_batch_SDX_ACCL_KERNEL_PASSTHRU;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role

build_H:
	#source ./device_type.tcl
	#source ./bins/set_bins.tcl
	#cd HLS; make HLS_PASSTHRU 
	#cd IP/role; create_PASSTHUR_blah4
	cd IP/role; create_all_42
	cd vivado_batch_shell;vivado -mode batch -source vivado_batch_3.tcl;cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell  
	cd vivado_batch_shell;cp *_partial*.bin; cp *.bi* ../bitfiles/shell; cp *.ltx ../bitfiles/shell; cp *.mcs ../bitfiles/shell; cp *.prm ../bitfiles/shell 
	#cd vivado_batch_SDX_ACCL_KERNEL_PASSTHRU;vivado -mode batch -source vivado_batch.tcl; cp *_partial*.bin ../bitfiles/role











