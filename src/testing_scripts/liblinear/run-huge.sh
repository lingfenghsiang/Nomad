if [ `uname -r` = "5.15.19-htmm" ];then
    bash src/testing_scripts/liblinear/run-memtis-huge.sh
    bash src/testing_scripts/liblinear/run-memtis-huge-nothrash.sh
elif [ `uname -r` = "5.13.0-rc6nomad" ];then
    bash src/testing_scripts/liblinear/run-nonmemtis-huge.sh
    bash src/testing_scripts/liblinear/run-nonmemtis-huge-nothrash.sh
elif [ `uname -r` = "5.13.0-rc6tpp" ];then
    bash src/testing_scripts/liblinear/run-nonmemtis-huge.sh
    bash src/testing_scripts/liblinear/run-nonmemtis-huge-nothrash.sh
else
    bash src/testing_scripts/liblinear/run-nonmemtis-huge.sh
    bash src/testing_scripts/liblinear/run-nonmemtis-huge-nothrash.sh
fi
