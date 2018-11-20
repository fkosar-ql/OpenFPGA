#! /bin/sh
cd ..
FPGA_FLOW_PATH=$PWD
DEST=${FPGA_FLOW_PATH}/configs/fpga_spice/k6_N10_sram_tsmc40nm_TT.conf
#DEST=configs/fpga_spice/test.conf

if [ -e "$DEST" ]; then
echo "" > $DEST;
fi

echo "# Standard Configuration Example" >> $DEST
echo "[dir_path]" >> $DEST
echo "script_base = ${FPGA_FLOW_PATH}/scripts/" >> $DEST
echo "benchmark_dir = ${FPGA_FLOW_PATH}/benchmarks/FPGA_SPICE_bench" >> $DEST
echo "odin2_path = ${FPGA_FLOW_PATH}/not_used_atm/odin2.exe" >> $DEST
echo "cirkit_path = ${FPGA_FLOW_PATH}/not_used_atm/cirkit" >> $DEST
echo "abc_path = ${FPGA_FLOW_PATH}/../abc_with_bb_support/abc" >> $DEST
echo "abc_mccl_path = ${FPGA_FLOW_PATH}/../abc_with_bb_support/abc" >> $DEST
echo "abc_with_bb_support_path = ${FPGA_FLOW_PATH}/../abc_with_bb_support/abc" >> $DEST
echo "mpack1_path = ${FPGA_FLOW_PATH}/not_used_atm/mpack1" >> $DEST
echo "m2net_path = ${FPGA_FLOW_PATH}/not_used_atm/m2net" >> $DEST
echo "mpack2_path = ${FPGA_FLOW_PATH}/not_used_atm/mpack2" >> $DEST
echo "vpr_path = ${FPGA_FLOW_PATH}/../vpr7_x2p/vpr/vpr" >> $DEST
echo "rpt_dir = ${FPGA_FLOW_PATH}/results" >> $DEST
echo "ace_path = ${FPGA_FLOW_PATH}/../ace2/ace" >> $DEST
echo "" >> $DEST
echo "[flow_conf]" >> $DEST
echo "flow_type = standard #standard|mpack2|mpack1|vtr_standard|vtr" >> $DEST
echo "vpr_arch = ${FPGA_FLOW_PATH}/arch/fpga_spice/k6_N10_sram_tsmc40nm_TT.xml # Use relative path under VPR folder is OK" >> $DEST
echo "mpack1_abc_stdlib = DRLC7T_SiNWFET.genlib # Use relative path under ABC folder is OK" >> $DEST
echo "m2net_conf = ${FPGA_FLOW_PATH}/m2net_conf/m2x2_SiNWFET.conf" >> $DEST
echo "mpack2_arch = K6_pattern7_I24.arch" >> $DEST
echo "power_tech_xml = ${FPGA_FLOW_PATH}/tech/tsmc40nm.xml # Use relative path under VPR folder is OK" >> $DEST
echo "" >> $DEST
echo "[csv_tags]" >> $DEST
echo "mpack1_tags = Global mapping efficiency:|efficiency:|occupancy wo buf:|efficiency wo buf:" >> $DEST
echo "mpack2_tags = BLE Number:|BLE Fill Rate: " >> $DEST
echo "vpr_tags = Netlist clb blocks:|Final critical path:|Total logic delay:|total net delay:|Total routing area:|Total used logic block area:|Total wirelength:|Packing took|Placement took|Routing took|Average net density:|Median net density:|Recommend no. of clock cycles:" >> $DEST
echo "vpr_power_tags = PB Types|Routing|Switch Box|Connection Box|Primitives|Interc Structures|lut6|ff" >> $DEST