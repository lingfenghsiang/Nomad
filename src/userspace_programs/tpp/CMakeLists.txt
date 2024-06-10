add_executable(tpp_mem_access 
memory_access.cc 
parse_async_prom_module.cc)
target_link_libraries(tpp_mem_access 
absl::strings 
absl::flags_parse 
absl::log
absl::log_initialize
absl::log_flags
absl::status
absl::statusor
glog
gflags)

add_executable(calcxxhash calculate_xxhash.cc)
target_link_libraries(calcxxhash
absl::strings 
absl::flags_parse 
absl::log
absl::log_initialize
absl::log_flags
absl::log_internal_check_op
absl::status
absl::statusor
glog
gflags)

add_executable(parse_async_prom 
parse_async_prom_main.cc
parse_async_prom_module.cc)
target_link_libraries(parse_async_prom
glog
gflags
absl::log_internal_check_op
absl::status
absl::statusor
)

add_executable(scan_vma 
parse_async_prom_scan_vma.cc
parse_async_prom_module.cc)
target_link_libraries(scan_vma 
glog
gflags
absl::log_internal_check_op
absl::status
absl::statusor
)

add_executable(trigger_shadow_scanning 
trigger_shadowmapping_scan.cc
parse_async_prom_module.cc
)
target_link_libraries(trigger_shadow_scanning 
glog
gflags
absl::log_internal_check_op
absl::status
absl::statusor
)

add_executable(trigger_shadow_reclaim
trigger_shadow_page_reclamation.cc
parse_async_prom_module.cc
)
target_link_libraries(trigger_shadow_reclaim
glog
gflags
absl::log_internal_check_op
absl::status
absl::statusor
)

add_executable(trigger_llc_miss_access
trigger_memtis.cc
parse_binary_array.cc
)
target_link_libraries(trigger_llc_miss_access
glog
gflags
absl::log_internal_check_op
absl::strings
absl::flags_parse 
absl::log
absl::log_initialize
absl::log_flags
absl::status
absl::statusor  
)

# add tests below

add_executable(
buffer_ring_test
buffer_ring.c
buffer_ring_test.cc
)

target_compile_options(buffer_ring_test PRIVATE -march=native)

target_link_libraries(buffer_ring_test
absl::strings
absl::log
absl::time
glog
GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(buffer_ring_test)