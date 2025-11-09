# tests/CMakeLists.txt
#find_package(Catch2 3 REQUIRED)
include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.6.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(GraphTests
               SourcevertexTests.cpp
               DiameterTests.cpp
               UniversalSourceTests.cpp
               FeedbackArcSetTests.cpp)

# Link tests against Catch2 and your graph library
target_link_libraries(GraphTests PRIVATE Catch2::Catch2WithMain mgmcc_lib)

# Enable and discover tests
enable_testing()
include(CTest)
include(Catch)
catch_discover_tests(GraphTests)