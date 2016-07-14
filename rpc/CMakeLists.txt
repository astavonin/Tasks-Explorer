cmake_minimum_required(VERSION 3.4)

set(PROJECT_NAME rpc)

set(proto_files rpc.proto)

find_package(Protobuf REQUIRED)

set(PROTO_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
set(GENERATED_PROTOBUF_PATH "${CMAKE_BINARY_DIR}/generated" CACHE STRING "For shared usage")
file(MAKE_DIRECTORY ${GENERATED_PROTOBUF_PATH})

set(EXTS ".pb.cc" ".pb.h" ".grpc.pb.cc" ".grpc.pb.h")

foreach(_f ${proto_files})
    set(proto_gen_files)
    foreach(_ext ${EXTS})
        string(REPLACE ".proto" ${_ext} out_s ${_f})
        list(APPEND proto_gen_files ${GENERATED_PROTOBUF_PATH}/${out_s})
    endforeach()

    add_custom_command(
        OUTPUT ${proto_gen_files}
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
        ARGS "--proto_path=${PROTO_PATH}"
             "--cpp_out=${GENERATED_PROTOBUF_PATH}"
             "${PROTO_PATH}/${_f}"
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
        ARGS "--proto_path=${PROTO_PATH}"
             "--grpc_out=${GENERATED_PROTOBUF_PATH}"
             "--plugin=protoc-gen-grpc=/usr/local/bin/GRPC_CPP_PLUGIN"
             "${PROTO_PATH}/${_f}"
        )
    set(GENERATED_PROTOBUF_FILES ${proto_gen_files})
endforeach()


include_directories(${GENERATED_PROTOBUF_PATH})
link_libraries(grpc++_unsecure grpc gpr ${PROTOBUF_LIBRARY})

add_library(${PROJECT_NAME} STATIC ${GENERATED_PROTOBUF_FILES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

