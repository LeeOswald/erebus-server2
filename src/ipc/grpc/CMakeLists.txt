set(EREBUS_GENERATED_DIR ${PROJECT_BINARY_DIR}/erebus)
file(MAKE_DIRECTORY ${EREBUS_GENERATED_DIR})

get_filename_component(EREBUS_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/erebus.proto" ABSOLUTE)

include(Debug)
dump_cmake_variables()

add_custom_command(
    OUTPUT "${EREBUS_GENERATED_DIR}/erebus.pb.cc" "${EREBUS_GENERATED_DIR}/erebus.pb.h" "${EREBUS_GENERATED_DIR}/erebus.grpc.pb.cc" "${EREBUS_GENERATED_DIR}/erebus.grpc.pb.h"
    COMMAND ${PROTOC_PROGRAM}
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

set(GRPC_AND_DEPS
    gRPC::grpc++ 
    gRPC::grpc++_reflection 
    gRPC::gpr 
    gRPC::grpc 
    protobuf::libprotoc 
    protobuf::libprotobuf 
    absl::flags 
    absl::flags_parse 
    absl::log absl::log_internal_check_op 
)

if(NOT ER_WINDOWS)
    list(APPEND GRPC_AND_DEPS utf8_range::utf8_validity)
endif()

target_link_libraries(erebus-grpc PUBLIC ${GRPC_AND_DEPS} fmt::fmt erebus::system)

target_include_directories(erebus-grpc PUBLIC "${gRPC_INCLUDE_DIR}")
target_include_directories(erebus-grpc PUBLIC "${protobuf_INCLUDE_DIR}")
target_include_directories(erebus-grpc PUBLIC "${absl_INCLUDE_DIR}")

target_link_directories(erebus-grpc PUBLIC ${gRPC_INCLUDE_DIR}/../lib)

target_compile_definitions(erebus-grpc PRIVATE ER_GRPC_EXPORTS)

