# CMakeLists.txt --- CMake project settings
#    ex) cmake -G "Visual Studio 9 2008" .
#    ex) cmake -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles" .
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.0)

# project name and languages
project(RunInLang CXX RC)

##############################################################################

# Visual C++?
if(MSVC)
    add_definitions(-DMSVC=1)
endif()

# Unicode
add_definitions(-DUNICODE -D_UNICODE)

# RunInLang.exe
add_executable(RunInLang WIN32 main.cpp main_res.rc)
target_link_libraries(RunInLang comctl32 comdlg32 shell32 shlwapi)

# x64?
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_executable(ril64 ril.cpp)
else()
    add_executable(ril32 ril.cpp)
endif()

##############################################################################
