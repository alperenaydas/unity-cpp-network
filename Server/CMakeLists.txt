add_executable(GameServer main.cpp
        NetworkServer.cpp
        NetworkServer.h
        GameWorld.cpp
        GameWorld.h)

if(WIN32)
    target_link_libraries(GameServer PRIVATE enet ws2_32 winmm)
endif()