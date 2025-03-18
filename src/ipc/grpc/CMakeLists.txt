set(EREBUS_GENERATED_DIR ${PROJECT_BINARY_DIR}/erebus)
file(MAKE_DIRECTORY ${EREBUS_GENERATED_DIR})


get_filename_component(EREBUS_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/erebus.proto" ABSOLUTE)

add_custom_command(
    OUTPUT "${EREBUS_GENERATED_DIR}/erebus.pb.cc" "${EREBUS_GENERATED_DIR}/erebus.pb.h" "${EREBUS_GENERATED_DIR}/erebus.grpc.pb.cc" "${EREBUS_GENERATED_DIR}/erebus.grpc.pb.h"
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --grpc_out "${EREBUS_GENERATED_DIR}"
        --cpp_out "${EREBUS_GENERATED_DIR}"
        -I "${CMAKE_CURRENT_SOURCE_DIR}"
        --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_PROGRAM}"
        "${EREBUS_PROTO}"
    DEPENDS "${EREBUS_PROTO}"
)

add_library(erebus-grpc SHARED
    ../../../include/erebus/ipc/grpc/grpc_server.hxx
    erebus_service.hxx
    erebus_service.cxx
    grpc_server.cxx
    protocol.hxx
    protocol.cxx
    session_data.hxx
    ${EREBUS_GENERATED_DIR}/erebus.pb.cc 
    ${EREBUS_GENERATED_DIR}/erebus.pb.h 
    ${EREBUS_GENERATED_DIR}/erebus.grpc.pb.cc 
    ${EREBUS_GENERATED_DIR}/erebus.grpc.pb.h
)

set_property(TARGET erebus-grpc PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(erebus-grpc PUBLIC grpc::grpc fmt::fmt erebus::system)

target_include_directories(erebus-grpc PUBLIC "${gRPC_INCLUDE_DIR}")
target_include_directories(erebus-grpc PUBLIC "${protobuf_INCLUDE_DIR}")

target_compile_definitions(erebus-grpc PRIVATE ER_GRPC_EXPORTS)

