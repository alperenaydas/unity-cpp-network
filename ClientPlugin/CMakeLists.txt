add_library(PurposeClient SHARED plugin.cpp
        NetworkClient.cpp
        NetworkClient.h)

target_link_libraries(PurposeClient PRIVATE enet)

if(WIN32)
    target_link_libraries(PurposeClient PRIVATE ws2_32 winmm)
endif()

set_target_properties(PurposeClient PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")