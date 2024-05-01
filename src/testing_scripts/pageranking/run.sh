if [ `uname -r` = "5.15.19-htmm" ];then
    bash src/testing_scripts/pageranking/run-memtis.sh
elif [ `uname -r` = "5.13.0-rc6nomad" ];then
    bash src/testing_scripts/pageranking/run-nonmemtis.sh
elif [ `uname -r` = "5.13.0-rc6tpp" ];then
    bash src/testing_scripts/pageranking/run-nonmemtis.sh
else
    echo "don't run test on kernel `uname -r`"
fi