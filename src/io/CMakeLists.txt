set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/test/src/io)

add_library(io STATIC load_data_from_disk.cc)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
# target_link_libraries(my_app Threads::Threads)
target_link_libraries(io Threads::Threads)
