add_executable(GameServer main.cpp)

if(WIN32)
    target_link_libraries(GameServer PRIVATE ws2_32 winmm)
endif()