# -*- cmake -*-

project(lldiriterator)

include(00-Common)
include(LLCommon)
include(UnixInstall)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    )

set(lldiriterator_SOURCE_FILES
    lldiriterator.cpp
    )

set(lldiriterator_HEADER_FILES
    CMakeLists.txt

    lldiriterator.h

    )


set_source_files_properties(${lldiriterator_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND lldiriterator_SOURCE_FILES ${lldiriterator_HEADER_FILES})

if (WINDOWS)
  remove_definitions(/Zc:wchar_t-)
  add_definitions(/Zc:wchar_t)
endif (WINDOWS)

add_library (lldiriterator ${lldiriterator_SOURCE_FILES})

set(vfs_BOOST_LIBRARIES
    ${BOOST_FILESYSTEM_LIBRARY}
    ${BOOST_SYSTEM_LIBRARY}
    )

target_link_libraries(lldiriterator
    ${vfs_BOOST_LIBRARIES}
    )


