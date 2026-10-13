# Reconstruct the inspected Vivado HLS 2019.1 project in build/.
# Usage: vivado_hls -f scripts/build_hls.tcl
# Optional environment variables: CNN_HLS_COSIM=1, CNN_HLS_EXPORT=1
set repo_root [file normalize [file join [file dirname [info script]] ..]]
file mkdir [file join $repo_root build]
cd [file join $repo_root build]
open_project cnn_accel
set_top cnn_accel
add_files [file join $repo_root src cnn.cpp]
add_files [file join $repo_root src cnn.h]
add_files [file join $repo_root src cnn_weights_export.h]
add_files -tb [file join $repo_root tb cnn_tb.cpp] -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
open_solution "solution1"
set_part {xc7a100t-csg324-1} -tool vivado
create_clock -period 10 -name default
config_export -format ip_catalog -rtl verilog
csim_design
csynth_design
if {[info exists ::env(CNN_HLS_COSIM)] && $::env(CNN_HLS_COSIM) eq "1"} {
    cosim_design
}
if {[info exists ::env(CNN_HLS_EXPORT)] && $::env(CNN_HLS_EXPORT) eq "1"} {
    export_design -rtl verilog -format ip_catalog
}
exit
