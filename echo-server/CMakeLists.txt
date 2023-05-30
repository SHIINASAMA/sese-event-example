find_package(sese-event CONFIG REQUIRED)

add_executable(echo-server main.cpp)

target_link_libraries(echo-server PRIVATE Sese::Event)