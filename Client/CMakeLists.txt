add_executable(GameClient main.cpp ../ClientPlugin/NetworkClient.cpp)

if(WIN32)
    target_link_libraries(GameClient PRIVATE enet ws2_32 winmm)
endif()

include_directories(../ClientPlugin)