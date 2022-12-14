# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=Win32 -P buildzlib.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -P buildzlib.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -DCMAKE_CONFIGURATION_TYPE=Release -P buildzlib.cmake

####
if(("${CMAKE_BUILD_TYPE}" STREQUAL "") AND ("${CMAKE_CONFIGURATION_TYPE}" STREQUAL ""))
  if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
    # multi-configuration
    execute_process(
      COMMAND ${CMAKE_COMMAND}
      -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
      -DCMAKE_CONFIGURATION_TYPE=Release
      -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/VSToolchain.cmake
      -DARCHITECTURE=${ARCHITECTURE}
      -P buildzlib.cmake
      )
    execute_process(
      COMMAND ${CMAKE_COMMAND}
      -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
      -DCMAKE_CONFIGURATION_TYPE=Debug
      -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/VSToolchain.cmake
      -DARCHITECTURE=${ARCHITECTURE}
      -P buildzlib.cmake
      )
    return()
  elseif(("$ENV{MSYSTEM}" MATCHES "MINGW") OR ("${CMAKE_COMMAND}" MATCHES "mingw"))
    # mingw on msys2
    if("${CMAKE_BUILD_TYPE}" STREQUAL "")
      set(CMAKE_BUILD_TYPE Release)
    endif()
  elseif("${CMAKE_GENERATOR}" MATCHES "Unix Makefiles")
    # mingw
    # single-configuration
    if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
      set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../mingw.toolchain.cmake")
    endif()
    if("${CMAKE_BUILD_TYPE}" STREQUAL "")
      set(CMAKE_BUILD_TYPE Release)
    endif()
  elseif("${CMAKE_GENERATOR}" MATCHES "NMake Makefiles")
    # VS nmake
    # single-configuration
    if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
      set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/VSToolchain.cmake")
    endif()
    if("${CMAKE_BUILD_TYPE}" STREQUAL "")
      set(CMAKE_BUILD_TYPE Release)
    endif()
  else()
    # single-configuration
    if("${CMAKE_BUILD_TYPE}" STREQUAL "")
      set(CMAKE_BUILD_TYPE Release)
    endif()
  endif()
endif()

include(script_support.cmake)

set(EXTRACT_DIR "${CMAKE_CURRENT_LIST_DIR}/build/zlib/src")
set(SRC_DIR "${EXTRACT_DIR}/zlib")
set(BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/build/zlib/build_${TOOLSET}")
set(INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/zlib_${TOOLSET}")
if(("${CMAKE_GENERATOR}" MATCHES "Win64") OR ("${ARCHITECTURE}" MATCHES "x64") OR ("$ENV{MSYSTEM_CHOST}" STREQUAL "x86_64-w64-mingw32") OR ("${CMAKE_COMMAND}" MATCHES "mingw64"))
  set(BUILD_DIR "${BUILD_DIR}_x64")
  set(INSTALL_DIR "${INSTALL_DIR}_x64")
endif()

########################################

file(MAKE_DIRECTORY ${SRC_DIR})

execute_process(
  COMMAND ${CMAKE_COMMAND} -DTARGET=zlib -DEXT_DIR=${EXTRACT_DIR} -P download.cmake
)

if(${SRC_DIR}/README IS_NEWER_THAN ${CMAKE_CURRENT_LIST_DIR}/doc_help/zlib-LICENSE.txt)
  file(COPY
    ${SRC_DIR}/README
    DESTINATION ${CMAKE_CURRENT_LIST_DIR}/doc_help)
  file(RENAME
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/README
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/zlib-LICENSE.txt)
endif()

########################################

file(MAKE_DIRECTORY "${BUILD_DIR}")

if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")

  ######################################## multi configuration

  if(NOT "${ARCHITECTURE}" STREQUAL "")
    set(CMAKE_A_OPTION -A ${ARCHITECTURE})
  endif()
  execute_process(
    COMMAND ${CMAKE_COMMAND} ${SRC_DIR} -G ${CMAKE_GENERATOR} ${CMAKE_A_OPTION}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
    ${TOOLCHAINFILE}
    WORKING_DIRECTORY ${BUILD_DIR}
    RESULT_VARIABLE rv
    )
  if(NOT rv STREQUAL "0")
    message(FATAL_ERROR "cmake generate fail ${rv}")
  endif()

  execute_process(
    COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_CONFIGURATION_TYPE} --target install
    WORKING_DIRECTORY ${BUILD_DIR}
    RESULT_VARIABLE rv
    )
  if(NOT rv STREQUAL "0")
    message(FATAL_ERROR "cmake install fail ${rv}")
  endif()

else()
  ######################################## single configuration

  execute_process(
    COMMAND ${CMAKE_COMMAND} ${SRC_DIR} -G ${CMAKE_GENERATOR}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
    WORKING_DIRECTORY ${BUILD_DIR}
    RESULT_VARIABLE rv
    )
  if(NOT rv STREQUAL "0")
    message(FATAL_ERROR "cmake build fail ${rv}")
  endif()

  execute_process(
    COMMAND ${CMAKE_COMMAND} --build . --target install
    WORKING_DIRECTORY ${BUILD_DIR}
    RESULT_VARIABLE rv
    )
  if(NOT rv STREQUAL "0")
    message(FATAL_ERROR "cmake install fail ${rv}")
  endif()

endif()
