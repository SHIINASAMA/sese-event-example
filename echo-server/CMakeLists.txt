find_package(sese-event CONFIG REQUIRED)

add_executable(test main.cpp)

target_link_libraries(test PRIVATE Sese::Event)