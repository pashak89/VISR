# Copyright Institute of Sound and Vibration Research - All rights reserved

set( SOURCES
wrapper.cpp
)

# Basically, this makes the files show up in the Visual studio project.
set( HEADERS
export_symbols.hpp
wrapper.hpp)

add_library( pythoncomponents_static STATIC ${SOURCES} ${HEADERS} )
set_target_properties( pythoncomponents_static PROPERTIES OUTPUT_NAME pythoncomponents )
target_compile_definitions( pythoncomponents_static PUBLIC VISR_PYTHONCOMPONENTS_STATIC_LIBRARY=1 )

add_library( pythoncomponents_shared SHARED ${SOURCES} ${HEADERS} )
set_target_properties( pythoncomponents_shared PROPERTIES OUTPUT_NAME pythoncomponents )
# Specify a specific name for the .lib stub of the DLL on Windows to avoid clashes with the static library.
set_target_properties( pythoncomponents_shared PROPERTIES ARCHIVE_OUTPUT_NAME pythoncomponents_shared )
set_target_properties( pythoncomponents_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib" )

# Set target properties common to both library types.
foreach(LIB_TYPE static shared)
  target_compile_definitions( pythoncomponents_${LIB_TYPE} PRIVATE VISR_BUILD_PYTHONCOMPONENTS_LIBRARY=1 )

  target_include_directories( pythoncomponents_${LIB_TYPE} PRIVATE ${PYTHON_INCLUDE_DIR} )
  # The pybind11 header is used to include Python.h.
  # This avoids preprocessor magic to work around platform specifics.
  target_include_directories( pythoncomponents_${LIB_TYPE} PRIVATE ${PYBIND11_INCLUDE_DIR} )

  # Transitive dependencies
  # Note: We always need to link the Pythonsupport shared library as a shared library, because there must be
  # exactly one instance of pythonsupport::InitialisationGuard
  target_link_libraries( pythoncomponents_${LIB_TYPE} PRIVATE pythonsupport_shared )
  target_link_libraries( pythoncomponents_${LIB_TYPE} PUBLIC visr_${LIB_TYPE} )
  target_link_libraries( pythoncomponents_${LIB_TYPE} PRIVATE ${PYTHON_LIBRARY} )
  target_link_libraries( pythoncomponents_${LIB_TYPE} PRIVATE Boost::boost )

endforeach()

# Set public headers to be installed for the shared target.
# Because the headers are the same for the static library, the headers are only
# installed once.
set_target_properties( pythoncomponents_shared PROPERTIES PUBLIC_HEADER "${HEADERS}" )

# Set include paths for dependent projects
get_filename_component(PARENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
foreach(TARGET_TYPE static shared)
  target_include_directories(pythoncomponents_${TARGET_TYPE}
    PUBLIC
    $<BUILD_INTERFACE:${PARENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/visr>
  )
endforeach()

if( BUILD_INSTALL_STATIC_LIBRARIES )
  install(TARGETS pythoncomponents_static
          EXPORT VISR_exports
          ARCHIVE DESTINATION lib COMPONENT static_libraries )
endif( BUILD_INSTALL_STATIC_LIBRARIES )
install(TARGETS pythoncomponents_shared
        EXPORT VISR_exports
        ARCHIVE DESTINATION lib COMPONENT development_files
        RUNTIME DESTINATION lib COMPONENT shared_libraries
        LIBRARY DESTINATION lib COMPONENT shared_libraries
        PUBLIC_HEADER DESTINATION include/visr/libpythoncomponents COMPONENT development_files )

# Add the unit test directory.
add_subdirectory( test )
