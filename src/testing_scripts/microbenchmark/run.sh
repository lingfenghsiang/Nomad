if [ `uname -r` = "5.15.19-htmm" ];then
    bash src/testing_scripts/microbenchmark/memtis_run.sh
elif [ `uname -r` = "5.13.0-rc6nomad" ];then
    bash src/testing_scripts/microbenchmark/nomad_run.sh
elif [ `uname -r` = "5.13.0-rc6tpp" ];then
    bash src/testing_scripts/microbenchmark/tpp_run.sh
else
    echo "don't run test on kernel `uname -r`"
fi