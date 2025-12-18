add_executable(GameClient main.cpp)

if(WIN32)
    target_link_libraries(GameClient PRIVATE enet ws2_32 winmm)
endif()