# used to generate shell script "generate_ycsb.sh"

wss = ["13.5G", "10G", "18G", "27G" ]

pattern = ["0", "1"]

name = ["hottest", "first_touch"]

for i in wss:
    for j, k in zip(pattern, name):
        cmd = "offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-" + i + "  | python3 filter_access_pattern.py | python3 write_binary_data.py -p " + j + " -f ${output_dir}/warmup_zipfan_" + k + "_" + i + ".bin`"
        print(cmd)
        cmd = "${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-" + i + "  | python3 filter_access_pattern.py | python3 write_binary_data.py -p " + j + " -o $offset -f ${output_dir}/run_zipfan_" + k + "_" + i + ".bin"
        print(cmd)
        print()