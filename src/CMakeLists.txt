add_subdirectory(system)
add_library(erebus::system ALIAS erebus-system)

add_subdirectory(ipc/grpc)
add_library(erebus::grpc ALIAS erebus-grpc)