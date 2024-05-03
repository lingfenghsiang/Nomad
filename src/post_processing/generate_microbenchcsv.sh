data_dir=src/tmp/results
output_dir=src/post_processing/tmp
config_dir=src/post_processing
mkdir -p ${output_dir}

for f in zipfan_hottest_10G.read.log   zipfan_hottest_13.5G.read.log   zipfan_hottest_27G.read.log zipfan_hottest_10G.write.log  zipfan_hottest_13.5G.write.log  zipfan_hottest_27G.write.log
do
sub_output_dir=microbench_nomad
mkdir -p ${output_dir}/${sub_output_dir}

if [ ! -f "${data_dir}/${sub_output_dir}/$f" ]; then
    echo "Error: ${data_dir}/${sub_output_dir}/$f does not exist."
    exit 1
fi

python src/post_processing/log_parser.py -l ${data_dir}/${sub_output_dir}/$f -c ${config_dir}/config_bw.json -o ${output_dir}/${sub_output_dir}/$f.csv
done

for f in zipfan_hottest_10G.read.log   zipfan_hottest_13.5G.read.log   zipfan_hottest_27G.read.log zipfan_hottest_10G.write.log  zipfan_hottest_13.5G.write.log  zipfan_hottest_27G.write.log
do
sub_output_dir=microbench_memtis
mkdir -p ${output_dir}/${sub_output_dir}

if [ ! -f "${data_dir}/${sub_output_dir}/$f" ]; then
    echo "Error: ${data_dir}/${sub_output_dir}/$f does not exist."
    exit 1
fi

python src/post_processing/log_parser.py -l ${data_dir}/${sub_output_dir}/$f -c ${config_dir}/config_bw.json -o ${output_dir}/${sub_output_dir}/$f.csv
done

for f in zipfan_hottest_10G.read.log   zipfan_hottest_13.5G.read.log   zipfan_hottest_27G.read.log zipfan_hottest_10G.write.log  zipfan_hottest_13.5G.write.log  zipfan_hottest_27G.write.log
do
sub_output_dir=microbench_tpp
mkdir -p ${output_dir}/${sub_output_dir}

if [ ! -f "${data_dir}/${sub_output_dir}/$f" ]; then
    echo "Error: ${data_dir}/${sub_output_dir}/$f does not exist."
    exit 1
fi

python src/post_processing/log_parser.py -l ${data_dir}/${sub_output_dir}/$f -c ${config_dir}/config_bw.json -o ${output_dir}/${sub_output_dir}/$f.csv
done
