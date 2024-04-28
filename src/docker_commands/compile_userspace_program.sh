mkdir -p /root/code/src/tmp/build
rm -rf /root/code/src/tmp/build/*
cd /root/code/src/tmp/build
cmake /root/code/src/userspace_programs
cmake --build tpp --target tpp_mem_access -j`nproc`
cmake --build tpp --target parse_async_prom -j`nproc`