
curr_dir=$PWD
rm -rf third_party/tmp/*
wget https://github.com/redis/redis/archive/refs/tags/6.2.13.tar.gz -P third_party/tmp/

tar -xvf third_party/tmp/6.2.13.tar.gz --directory third_party/tmp

cd third_party/tmp/redis-6.2.13 ; make -j`nproc`
cd ${curr_dir}


git clone https://github.com/sbeamer/gapbs.git third_party/tmp/gapbs
cd third_party/tmp/gapbs
make pr

cd ${curr_dir}

wget  https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/multicore-liblinear/liblinear-multicore-2.47.zip -P third_party/tmp/
unzip third_party/tmp/liblinear-multicore-2.47.zip -d third_party/tmp
cd third_party/tmp/liblinear-multicore-2.47
patch -p1 < ../../train.diff
make 
wget https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/binary/HIGGS.xz
xz -d HIGGS.xz

cd ${curr_dir}
