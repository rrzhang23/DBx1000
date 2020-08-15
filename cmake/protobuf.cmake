

# find protobuf && protoc
message("researching for protobuf")
set(PROTOBUF_FOUND 1)
set(Protobuf_INCLUDE_DIR ${PROTOBUF_ROOT}/include)
set(Protobuf_INCLUDE_DIRS ${PROTOBUF_ROOT}/include)
find_library(Protobuf_LIBRARY libprotobuf.a ${PROTOBUF_ROOT}/lib NO_DEFAULT_PATH)
find_library(Protobuf_LIBRARIES libprotobuf.a ${PROTOBUF_ROOT}/lib NO_DEFAULT_PATH)
find_library(Protobuf_LIBRARY_DEBUG libprotobuf.a ${PROTOBUF_ROOT}/lib NO_DEFAULT_PATH)
find_program(Protobuf_PROTOC_EXECUTABLE NAMES protoc PATHS ${PROTOBUF_ROOT}/bin NO_DEFAULT_PATH)
find_library(Protobuf_PROTOC_LIBRARY libprotoc.a ${PROTOBUF_ROOT}/lib NO_DEFAULT_PATH)
find_library(Protobuf_PROTOC_LIBRARIES libprotoc.a ${PROTOBUF_ROOT}/lib NO_DEFAULT_PATH)
find_library(Protobuf_PROTOC_LIBRARY_DEBUG libprotoc.a ${PROTOBUF_ROOT}/lib NO_DEFAULT_PATH)
find_library(Protobuf_LITE_LIBRARY libprotobuf-lite.a ${PROTOBUF_ROOT}/lib NO_DEFAULT_PATH)
find_library(Protobuf_LITE_LIBRARIES libprotobuf-lite.a ${PROTOBUF_ROOT}/lib NO_DEFAULT_PATH)
find_library(Protobuf_LITE_LIBRARY_DEBUG libprotobuf-lite.a ${PROTOBUF_ROOT}/lib NO_DEFAULT_PATH)
find_library(Protobuf_LIBRARY libprotobuf.a ${PROTOBUF_ROOT}/lib64 NO_DEFAULT_PATH)
find_library(Protobuf_LIBRARIES libprotobuf.a ${PROTOBUF_ROOT}/lib64 NO_DEFAULT_PATH)
find_library(Protobuf_LIBRARY_DEBUG libprotobuf.a ${PROTOBUF_ROOT}/lib64 NO_DEFAULT_PATH)
find_program(Protobuf_PROTOC_EXECUTABLE NAMES protoc PATHS ${PROTOBUF_ROOT}/bin NO_DEFAULT_PATH)
find_library(Protobuf_PROTOC_LIBRARY libprotoc.a ${PROTOBUF_ROOT}/lib64 NO_DEFAULT_PATH)
find_library(Protobuf_PROTOC_LIBRARIES libprotoc.a ${PROTOBUF_ROOT}/lib64 NO_DEFAULT_PATH)
find_library(Protobuf_PROTOC_LIBRARY_DEBUG libprotoc.a ${PROTOBUF_ROOT}/lib64 NO_DEFAULT_PATH)
find_library(Protobuf_LITE_LIBRARY libprotobuf-lite.a ${PROTOBUF_ROOT}/lib64 NO_DEFAULT_PATH)
find_library(Protobuf_LITE_LIBRARIES libprotobuf-lite.a ${PROTOBUF_ROOT}/lib64 NO_DEFAULT_PATH)
find_library(Protobuf_LITE_LIBRARY_DEBUG libprotobuf-lite.a ${PROTOBUF_ROOT}/lib64 NO_DEFAULT_PATH)

foreach (Camel
        Protobuf_INCLUDE_DIR
        Protobuf_INCLUDE_DIRS
        Protobuf_LIBRARY
        Protobuf_LIBRARIES
        Protobuf_LIBRARY_DEBUG
        Protobuf_PROTOC_EXECUTABLE
        Protobuf_PROTOC_LIBRARY
        Protobuf_PROTOC_LIBRARIES
        Protobuf_PROTOC_LIBRARY_DEBUG
        Protobuf_LITE_LIBRARY
        Protobuf_LITE_LIBRARIES
        Protobuf_LITE_LIBRARY_DEBUG
        )
    string(TOUPPER ${Camel} UPPER)
    set(${UPPER} ${${Camel}})
endforeach ()

message(STATUS "PROTOBUF_FOUND : " ${PROTOBUF_FOUND})
message(STATUS "Protobuf_INCLUDE_DIR : " ${Protobuf_INCLUDE_DIR})
message(STATUS "Protobuf_LIBRARY : " ${Protobuf_LIBRARY})
message(STATUS "Protobuf_PROTOC_EXECUTABLE : " ${Protobuf_PROTOC_EXECUTABLE})
message(STATUS "Protobuf_PROTOC_LIBRARY : " ${Protobuf_PROTOC_LIBRARY})
message(STATUS "Protobuf_LITE_LIBRARY : " ${Protobuf_LITE_LIBRARY})
IF (NOT PROTOBUF_FOUND)
    MESSAGE(WARNING "Protobuf could not be found")
ENDIF ()
IF (PROTOBUF_FOUND)
    # Verify protobuf version number. Version information looks like:
    # // The current version, represented as a single integer to make comparison
    # // easier:  major * 10^6 + minor * 10^3 + micro
    # #define GOOGLE_PROTOBUF_VERSION 2006000
    FILE(STRINGS "${Protobuf_INCLUDE_DIR}/google/protobuf/stubs/common.h"
            PROTOBUF_VERSION_NUMBER
            REGEX "^#define[\t ]+GOOGLE_PROTOBUF_VERSION[\t ][0-9]+.*"
            )
    STRING(REGEX REPLACE
            "^.*GOOGLE_PROTOBUF_VERSION[\t ]([0-9])[0-9][0-9]([0-9])[0-9][0-9].*$"
            "\\1"
            PROTOBUF_MAJOR_VERSION "${PROTOBUF_VERSION_NUMBER}")
    STRING(REGEX REPLACE
            "^.*GOOGLE_PROTOBUF_VERSION[\t ]([0-9])[0-9][0-9]([0-9])[0-9][0-9].*$"
            "\\2"
            PROTOBUF_MINOR_VERSION "${PROTOBUF_VERSION_NUMBER}")

    MESSAGE(STATUS
            "PROTOBUF_VERSION_NUMBER is ${PROTOBUF_VERSION_NUMBER}")

    IF ("${PROTOBUF_MAJOR_VERSION}.${PROTOBUF_MINOR_VERSION}" VERSION_LESS "2.5")
        message(FATAL_ERROR "Protobuf version less than 2.5")
    ENDIF ()
ENDIF ()
