if [ `uname -r` = "5.15.19-htmm" ];then
    bash src/testing_scripts/pageranking/run-memtis-huge.sh
elif [ `uname -r` = "5.13.0-rc6nomad" ];then
    bash src/testing_scripts/pageranking/run-nonmemtis-huge.sh
elif [ `uname -r` = "5.13.0-rc6tpp" ];then
    bash src/testing_scripts/pageranking/run-nonmemtis-huge.sh
else
    bash src/testing_scripts/pageranking/run-nonmemtis-huge.sh
fi
