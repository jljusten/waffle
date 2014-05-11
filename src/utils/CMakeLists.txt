# ----------------------------------------------------------------------------
# Target: wflinfo (executable)
# ----------------------------------------------------------------------------

add_executable(wflinfo wflinfo.c wutils.c)
target_link_libraries(wflinfo ${waffle_libname})
set_target_properties(wflinfo
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )

if(waffle_on_mac)
    set_target_properties(wflinfo
        PROPERTIES
        COMPILE_FLAGS "-ObjC"
        )
endif()

install(
    TARGETS wflinfo
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT utils
    )

# ----------------------------------------------------------------------------
# Target: wflgears (executable)
# ----------------------------------------------------------------------------

add_executable(wflgears wflgears.c wflgears_legacy.c wutils.c)
target_link_libraries(wflgears ${waffle_libname} m)
set_target_properties(wflgears
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )

if(waffle_on_mac)
    set_target_properties(wflgears
        PROPERTIES
        COMPILE_FLAGS "-ObjC"
        )
endif()

install(
    TARGETS wflgears
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT utils
    )
