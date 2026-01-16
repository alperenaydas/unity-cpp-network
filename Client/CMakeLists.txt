add_executable(GameClient main.cpp ../ClientPlugin/NetworkClient.cpp)

target_link_libraries(GameClient PRIVATE enet)

if(WIN32)
    target_link_libraries(GameClient PRIVATE ws2_32 winmm)
endif()

include_directories(../ClientPlugin)