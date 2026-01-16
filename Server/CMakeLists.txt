add_executable(GameServer main.cpp
        NetworkServer.cpp
        NetworkServer.h
        GameWorld.cpp
        GameWorld.h)

target_link_libraries(GameServer PRIVATE enet)

if(WIN32)
    target_link_libraries(GameServer PRIVATE ws2_32 winmm)
endif()