# Copyright Institute of Sound and Vibration Research - All rights reserved

IF( APPLE )
  FIND_LIBRARY( CORE_SERVICE_FRAMEWORK NAMES CoreServices )
  INCLUDE_DIRECTORIES( ${CORE_SERVICE_FRAMEWORK}/Versions/A/Frameworks/CarbonCore.framework/Versions/A/Headers )
ENDIF( APPLE )

SET( SOURCES
argument_list.cpp
external_base.cpp
signal_flow_wrapper.cpp
)

SET( HEADERS
argument_list.hpp
class_registrar.hpp
export_symbols.hpp
external_base.hpp
external_wrapper.hpp
signal_flow_wrapper.hpp)

INCLUDE_DIRECTORIES( ${MAXSDK_MSP_INCLUDE_DIR} )
INCLUDE_DIRECTORIES( ${MAXSDK_MAX_INCLUDE_DIR} )

ADD_LIBRARY( maxsupport_static STATIC ${SOURCES} ${HEADERS} )
set_target_properties( maxsupport_static PROPERTIES OUTPUT_NAME maxsupport )

# Transitive dependencies
target_link_libraries( maxsupport_static PUBLIC rrl_static )
target_link_libraries( maxsupport_static PUBLIC efl_static )
target_link_libraries( maxsupport_static PUBLIC ${MAXSDK_AUDIO_LIBRARY} )
target_link_libraries( maxsupport_static PUBLIC ${MAXSDK_API_LIBRARY} )
target_link_libraries( maxsupport_static PRIVATE Boost::boost ) # Adds the boost include directory

add_library( maxsupport_shared SHARED ${SOURCES} ${HEADERS} )
set_target_properties( maxsupport_shared PROPERTIES OUTPUT_NAME maxsupport )
# Specify a specific name for the .lib stub of the DLL on Windows to avoid clashes with the static library.
set_target_properties( maxsupport_shared PROPERTIES ARCHIVE_OUTPUT_NAME maxsupport_shared)
target_compile_definitions( maxsupport_shared PRIVATE -DVISR_BUILD_MAXSUPPORT_SHARED_LIBRARY=1 -DVISR_BUILD_MAXSUPPORT_LIBRARY=1)
target_compile_definitions( maxsupport_shared PUBLIC -DVISR_BUILD_MAXSUPPORT_SHARED_LIBRARY=1)
set_target_properties( maxsupport_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib" )

# Transitive dependencies
target_link_libraries( maxsupport_shared PUBLIC rrl_shared )
target_link_libraries( maxsupport_shared PUBLIC efl_shared )
target_link_libraries( maxsupport_shared PUBLIC ${MAXSDK_AUDIO_LIBRARY} )
target_link_libraries( maxsupport_shared PUBLIC ${MAXSDK_API_LIBRARY} )
target_link_libraries( maxsupport_shared PRIVATE Boost::boost ) # Adds the boost include directory

# Set public headers to be installed for the shared library target.
# Because the headers are the same for the static library, the headers are only
# installed once.
set_target_properties( maxsupport_shared PROPERTIES PUBLIC_HEADER "${HEADERS}" )

install( TARGETS maxsupport_static
         ARCHIVE DESTINATION staticlib COMPONENT max_externals
       )
install( TARGETS maxsupport_shared
         ARCHIVE DESTINATION lib COMPONENT max_externals
         RUNTIME DESTINATION lib COMPONENT max_externals
         LIBRARY DESTINATION lib COMPONENT max_externals
         PUBLIC_HEADER DESTINATION include/visr/libmaxsupport COMPONENT max_externals )
