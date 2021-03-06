# -*- cmake -*-
include(Prebuilt)

set(Boost_FIND_QUIETLY ON)
set(Boost_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindBoost)

  set(BOOST_PROGRAM_OPTIONS_LIBRARY boost_program_options-mt)
  set(BOOST_REGEX_LIBRARY boost_regex-mt)
  set(BOOST_SIGNALS_LIBRARY boost_signals-mt)
  set(BOOST_SYSTEM_LIBRARY boost_system-mt)
  set(BOOST_FILESYSTEM_LIBRARY boost_filesystem-mt)
  set(BOOST_THREAD_LIBRARY boost_thread-mt)
  set(BOOST_WAVE_LIBRARY boost_wave-mt)

else (STANDALONE)
  use_prebuilt_binary(boost)
  use_prebuilt_binary(boost-headers)
  use_prebuilt_binary(boost-extension)
  set(Boost_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)

  if (WINDOWS)
    set(BOOST_VERSION 1_45)
    if(MSVC80)
      set(BOOST_PROGRAM_OPTIONS_LIBRARY 
          optimized libboost_program_options-vc80-mt-${BOOST_VERSION}
          debug libboost_program_options-vc80-mt-gd-${BOOST_VERSION})
      set(BOOST_REGEX_LIBRARY
          optimized libboost_regex-vc80-mt-${BOOST_VERSION}
          debug libboost_regex-vc80-mt-gd-${BOOST_VERSION})
      set(BOOST_SIGNALS_LIBRARY 
          optimized libboost_signals-vc80-mt-${BOOST_VERSION}
          debug libboost_signals-vc80-mt-gd-${BOOST_VERSION})
      set(BOOST_SYSTEM_LIBRARY 
          optimized libboost_system-vc80-mt-${BOOST_VERSION}
          debug libboost_system-vc80-mt-gd-${BOOST_VERSION})
      set(BOOST_FILESYSTEM_LIBRARY 
          optimized libboost_filesystem-vc80-mt-${BOOST_VERSION}
          debug libboost_filesystem-vc80-mt-gd-${BOOST_VERSION})
    else(MSVC80)
      # MSVC 10.0 config
      set(BOOST_PROGRAM_OPTIONS_LIBRARY 
          optimized libboost_program_options-mt
		  debug libboost_program_options-mt-gd)
      set(BOOST_REGEX_LIBRARY
          optimized libboost_regex-mt
          debug libboost_regex-mt-gd)
      set(BOOST_SYSTEM_LIBRARY 
          optimized libboost_system-mt
          debug libboost_system-mt-gd)
      set(BOOST_FILESYSTEM_LIBRARY 
          optimized libboost_filesystem-mt
          debug libboost_filesystem-mt-gd)
	  set(BOOST_THREAD_LIBRARY 
		  optimized libboost_thread-mt
		  debug libboost_thread-mt-gd)
	  set(BOOST_WAVE_LIBRARY 
	      optimized libboost_wave-mt
		  debug libboost_wave-mt-gd)		  
    endif (MSVC80)
  elseif (DARWIN)
    set(BOOST_PROGRAM_OPTIONS_LIBRARY boost_program_options)
    set(BOOST_REGEX_LIBRARY boost_regex)
    set(BOOST_SYSTEM_LIBRARY boost_system)
    set(BOOST_FILESYSTEM_LIBRARY boost_filesystem)
	set(BOOST_THREAD_LIBRARY boost_thread)
	set(BOOST_WAVE_LIBRARY boost_wave)
  elseif(LINUX)
    set(BOOST_PROGRAM_OPTIONS_LIBRARY boost_program_options-mt)
    set(BOOST_REGEX_LIBRARY boost_regex-mt)
    set(BOOST_SYSTEM_LIBRARY boost_system-mt)
    set(BOOST_FILESYSTEM_LIBRARY boost_filesystem-mt)
	set(BOOST_THREAD_LIBRARY boost_thread-mt)
	set(BOOST_WAVE_LIBRARY boost_wave-mt)
  endif (WINDOWS)
endif (STANDALONE)
if (LINUX OR WINDOWS) # probably it also works on mac
  set(BOOST_EXTENSION ON CACHE BOOL "Enable Boost.Extension")
endif (LINUX OR WINDOWS)
if (BOOST_EXTENSION)
    #tell cmake
    set(USE_BOOST_EXTENSION  ON CACHE BOOL "Use Boost.Extension")
    #tell preprocessor
    add_definitions( -DUSE_BOOST_EXTENSION=1)
endif (BOOST_EXTENSION)