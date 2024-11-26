
bash src/testing_scripts/microbenchmark/memtis_commands/hottest_10G_read.sh
sleep 20
bash src/testing_scripts/microbenchmark/memtis_commands/hottest_10G_write.sh
sleep 20

bash src/testing_scripts/microbenchmark/memtis_commands/hottest_13.5G_read.sh
sleep 20
bash src/testing_scripts/microbenchmark/memtis_commands/hottest_13.5G_write.sh
sleep 20

bash src/testing_scripts/microbenchmark/memtis_commands/hottest_27G_read.sh
sleep 20
bash src/testing_scripts/microbenchmark/memtis_commands/hottest_27G_write.sh
sleep 20

bash src/testing_scripts/microbenchmark/memtis_commands/llc_miss_10G_read.sh
sleep 20
bash src/testing_scripts/microbenchmark/memtis_commands/llc_miss_13G_read.sh
sleep 20
bash src/testing_scripts/microbenchmark/memtis_commands/llc_miss_27G_read.sh
sleep 20