root_dir=$PWD

mkdir -p ${root_dir}/src/tmp/
rm -rf ${root_dir}/src/tmp/*

rm src/linux-5.15.19/.config
cp ${root_dir}/src/kernel_config/5.15.config src/linux-5.15.19/.config

ulimit -n 2048
# compile memtis
echo compiling memtis...
echo deleting compiled code...
sudo rm -rf src/linux-5.15.19/*
echo restoring source code...
git restore src/linux-5.15.19/

rm -f src/linux-*_amd64.*
echo start compiling...
docker run -v .:/root/code --rm docklf/ubuntu20-kerncomp:aec-v0.2 bash /root/code/src/docker_commands/docker_comp5.15.19.sh

cp ${root_dir}/src/*.deb ${root_dir}/src/tmp/

for i in ${root_dir}/src/tmp/*.deb
do
newname=`echo $i | sed 's+[0-9]\{1,3\}_amd64+memtis_amd64+g'`

if [ "$newname" != $i ];then
mv $i $newname
fi

done

rm src/linux-5.13-rc6/.config
cp ${root_dir}/src/kernel_config/tpp.config src/linux-5.13-rc6/.config
# compile tpp
echo compiling tpp...
echo deleting patched code...
sudo rm -rf src/linux-5.13-rc6/*
echo restoring source code...
git restore src/linux-5.13-rc6/
echo applying patches...
patch -p1 < src/implementation_patches/tpp.patch
rm -f src/linux-*_amd64.*

echo start compiling...
docker run -v .:/root/code --rm docklf/ubuntu20-kerncomp:aec-v0.2 bash /root/code/src/docker_commands/docker_comp5.13.sh

cp ${root_dir}/src/*.deb ${root_dir}/src/tmp/

for i in ${root_dir}/src/tmp/*.deb
do

newname=`echo $i | sed 's+[0-9]\{1,3\}_amd64+tpp_amd64+g'`

if [ "$newname" != $i ];then
mv $i $newname
fi

done

rm src/linux-5.13-rc6/.config
cp ${root_dir}/src/kernel_config/nomad.config src/linux-5.13-rc6/.config
# compile nomad
echo compiling nomad...
echo deleting patched code...
sudo rm -rf src/linux-5.13-rc6/*
echo restoring source code...
git restore src/linux-5.13-rc6/
echo applying patches...
patch -p1 < src/implementation_patches/nomad.patch
rm -f src/linux-*_amd64.*

echo start compiling...
docker run -v .:/root/code --rm docklf/ubuntu20-kerncomp:aec-v0.2 bash /root/code/src/docker_commands/docker_comp5.13.sh

cp ${root_dir}/src/*.deb ${root_dir}/src/tmp/

for i in ${root_dir}/src/tmp/*.deb
do

newname=`echo $i | sed 's+[0-9]\{1,3\}_amd64+nomad_amd64+g'`

if [ "$newname" != $i ];then
mv $i $newname
fi

done

echo compiling userspace program...

docker run -v .:/root/code --rm docklf/compile_environment:glogflagtest bash /root/code/src/docker_commands/compile_userspace_program.sh

echo generate microbenchmark access pattern file...
docker run -v .:/root/code --rm docklf/ubuntu20-kerncomp:aec-v0.2 bash /root/code/src/testing_scripts/microbenchmark/generate_ycsb.sh

mkdir -p src/tmp/output
rm -rf src/tmp/output/*

mv src/testing_scripts/microbenchmark/tmp/*.bin src/tmp/output/
mv src/tmp/*.deb src/tmp/output/
mv src/tmp/build/tpp/tpp_mem_access src/tmp/output/
mv src/tmp/build/tpp/parse_async_prom src/tmp/output/
