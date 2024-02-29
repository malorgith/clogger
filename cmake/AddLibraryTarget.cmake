
function(CloggerAddLibraryTarget target_name lib_type src_files public_header output_name)
    add_library(${target_name} ${lib_type} ${src_files})
    target_link_libraries(${target_name} PUBLIC
        base_target
    )
    set_target_properties(${target_name} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        OUTPUT_NAME ${output_name}
        PUBLIC_HEADER "${public_header}"
    )
    install(TARGETS ${target_name}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}
    )
endfunction()
