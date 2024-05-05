if [ `uname -r` = "5.15.19-htmm" ];then
    bash src/testing_scripts/liblinear/run-memtis.sh
elif [ `uname -r` = "5.13.0-rc6nomad" ];then
    bash src/testing_scripts/liblinear/run-nonmemtis.sh
elif [ `uname -r` = "5.13.0-rc6tpp" ];then
    bash src/testing_scripts/liblinear/run-nonmemtis.sh
else
    bash src/testing_scripts/liblinear/run-nonmemtis.sh
fi
