cmake_minimum_required(VERSION 3.5)

project(Sinvi VERSION 00.01.00 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DVERSION="${CMAKE_PROJECT_VERSION}")

if(WIN32)
link_directories(${CMAKE_SOURCE_DIR}/cots/lib)
include_directories(${CMAKE_SOURCE_DIR}/cots/include)
endif(WIN32)


set(PROJECT_SOURCES
    src/defs.cpp                include/defs.h
    src/configuration.cpp       include/configuration.h
    src/variables.cpp           include/variables.h
    src/ThreadSafeQueue.cpp     include/ThreadSafeQueue.h
    src/pCapUtils.cpp           include/pCapUtils.h
)

add_library(${PROJECT_NAME} SHARED
    ${PROJECT_SOURCES}
)
if (WIN32)
    set(SINVI_DEPS pcap winsocks32)
else()
    set(SINVI_DEPS pcap)
endif(WIN32)
target_link_libraries(${PROJECT_NAME} ${SINVI_DEPS})
include_directories(${CMAKE_SOURCE_DIR}/include include)
