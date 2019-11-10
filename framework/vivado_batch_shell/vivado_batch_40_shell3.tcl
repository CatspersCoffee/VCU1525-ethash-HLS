    synth_design -keep_equivalent_registers -shreg_min_size 8 -include_dirs ../src -top $TOP_module -verilog_define XSDB_SLV_DIS -part [DEVICE_TYPE] 
    read_checkpoint -cell U_role_NORTH ../checkpoints/role_NORTH.$NORTH_ROLE_NAME.post_synth_opt.dcp
    write_checkpoint -force ./$TOP_module.$NORTH_ROLE_NAME.post_synth.dcp
    opt_design -verbose -directive Explore

    set_property USER_CLOCK_ROOT X4Y7  [get_nets -of_objects [get_pins U_shell_top/PCIe_Bridge_ICAP_complex_i/ddr4_1/inst/u_ddr4_infrastructure/u_bufg_riuClk/O]]
    set_property USER_CLOCK_ROOT X4Y7  [get_nets -of_objects [get_pins U_shell_top/PCIe_Bridge_ICAP_complex_i/ddr4_1/inst/u_ddr4_infrastructure/u_bufg_divClk/O]]
    set_property USER_CLOCK_ROOT X2Y2  [get_nets -of_objects [get_pins U_role_NORTH/HLS_PR_0_i_NORTH/DDR_SUB_SYS/PR_DDR4_MIG_0/ddr4_0/inst/u_ddr4_infrastructure/u_bufg_divClk/O]]
    set_property USER_CLOCK_ROOT X2Y2  [get_nets -of_objects [get_pins U_role_NORTH/HLS_PR_0_i_NORTH/DDR_SUB_SYS/PR_DDR4_MIG_0/ddr4_0/inst/u_ddr4_infrastructure/u_bufg_riuClk/O]]
    set_property USER_CLOCK_ROOT X2Y8  [get_nets -of_objects [get_pins U_role_NORTH/HLS_PR_0_i_NORTH/DDR_SUB_SYS/PR_DDR4_MIG_2/ddr4_2/inst/u_ddr4_infrastructure/u_bufg_divClk/O]]
    set_property USER_CLOCK_ROOT X2Y8  [get_nets -of_objects [get_pins U_role_NORTH/HLS_PR_0_i_NORTH/DDR_SUB_SYS/PR_DDR4_MIG_2/ddr4_2/inst/u_ddr4_infrastructure/u_bufg_riuClk/O]]
    set_property USER_CLOCK_ROOT X2Y12 [get_nets -of_objects [get_pins U_role_NORTH/HLS_PR_0_i_NORTH/DDR_SUB_SYS/PR_DDR4_MIG_3/ddr4_3/inst/u_ddr4_infrastructure/u_bufg_divClk/O]]
    set_property USER_CLOCK_ROOT X2Y12 [get_nets -of_objects [get_pins U_role_NORTH/HLS_PR_0_i_NORTH/DDR_SUB_SYS/PR_DDR4_MIG_3/ddr4_3/inst/u_ddr4_infrastructure/u_bufg_riuClk/O]]

    create_clock -name SRAI_PROG_CLK -period $ROLE_CLK_PERIOD [get_pins U_shell_top/PCIe_Bridge_ICAP_complex_i/clk_wiz_PROG/inst/CLK_CORE_DRP_I/clk_inst/mmcme4_adv_inst/CLKOUT0]
    set_false_path -through [get_pins {U_shell_top/PCIe_Bridge_ICAP_complex_i/axi_gpio_0/gpio_io_*[*]}]
    set_clock_groups -name SRAI_CG_PROG_CLK -asynchronous -group [ get_clocks -of_objects [get_pins U_shell_top/PCIe_Bridge_ICAP_complex_i/clk_wiz_PROG/inst/CLK_CORE_DRP_I/clk_inst/mmcme4_adv_inst/CLKOUT0]] -group [get_clocks [list  [get_clocks -of_objects [get_pins U_shell_top/PCIe_Bridge_ICAP_complex_i/clk_wiz_0/inst/mmcme4_adv_inst/CLKOUT0]] [get_clocks -of_objects [get_pins U_shell_top/PCIe_Bridge_ICAP_complex_i/clk_wiz_0/inst/mmcme4_adv_inst/CLKOUT1]]]]

    write_checkpoint -force ./$TOP_module.$NORTH_ROLE_NAME.post_synth_opt.dcp
    place_design -verbose -no_bufg_opt -directive Explore
    write_checkpoint -force ./$TOP_module.$NORTH_ROLE_NAME.post_place.dcp
    phys_opt_design  -verbose -directive Explore
    write_checkpoint -force ./$TOP_module.$NORTH_ROLE_NAME.post_place_phys_opt.dcp
    route_design  -verbose -directive Explore
    write_checkpoint -force ./$TOP_module.$NORTH_ROLE_NAME.post_route.dcp
    phys_opt_design  -verbose -directive Explore
    write_checkpoint -force ./$TOP_module.$NORTH_ROLE_NAME.post_route_phys_opt.dcp
    write_debug_probes ./$TOP_module.ltx
    report_timing_summary -file $TOP_module.$NORTH_ROLE_NAME.timing_summary.rpt
    report_drc -file $TOP_module.drc.rpt

    set_param bitstream.enablePR 4123
    set_property BITSTREAM.CONFIG.CONFIGRATE 85.0 [current_design]
    set_property BITSTREAM.CONFIG.SPI_32BIT_ADDR YES [current_design]
    set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
    set_property BITSTREAM.CONFIG.SPI_FALL_EDGE YES [current_design]
    set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design]
    set_property BITSTREAM.CONFIG.EXTMASTERCCLK_EN Disable [current_design]
    set_property CONFIG_MODE SPIx4 [current_design]
    set_property CONFIG_VOLTAGE 1.8 [current_design]
    set_property CFGBVS GND [current_design]
    write_bitstream -bin_file $TOP_module.$NORTH_ROLE_NAME.bit      
    write_cfgmem  -format mcs -size 512 -interface SPIx4 -loadbit "up 0x00000000 $TOP_module.$NORTH_ROLE_NAME.bit " -file "$TOP_module.$NORTH_ROLE_NAME.mcs"

    write_checkpoint -force -cell U_role_NORTH ../checkpoints/role_NORTH.$NORTH_ROLE_NAME.post_route_phys_opt.dcp

    update_design -cell U_role_NORTH -black_box
    lock_design -level routing
    write_checkpoint -force ../checkpoints/$TOP_module.routed_BB.dcp