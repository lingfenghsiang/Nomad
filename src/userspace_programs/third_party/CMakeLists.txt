# install gflags from source code. Then no need to add subdirectory
# add_subdirectory(gflags)
add_subdirectory(glog)
add_subdirectory(abseil-cpp)

set(ABSL_BUILD_TESTING=ON)
set(ABSL_USE_GOOGLETEST_HEAD=ON)
set(CMAKE_CXX_STANDARD 14)