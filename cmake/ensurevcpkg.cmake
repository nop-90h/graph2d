# Автоматический clone + bootstrap vcpkg до project()
# (только если не задан CMAKE_TOOLCHAIN_FILE, например для Emscripten)
if(CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_TOOLCHAIN_FILE}")
  return()
endif()

set(_vcpkg_root "${CMAKE_SOURCE_DIR}/vcpkg")

if(NOT EXISTS "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake")
  message(STATUS "[vcpkg] Not found - cloning into ${_vcpkg_root} (one-time)...")
  find_package(Git REQUIRED)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} clone --depth 1
            https://github.com/microsoft/vcpkg.git "${_vcpkg_root}"
    RESULT_VARIABLE _rc
    OUTPUT_QUIET ERROR_QUIET
  )
  if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "[vcpkg] git clone failed (rc=${_rc}). Install git and retry.")
  endif()
endif()

if(NOT EXISTS "${_vcpkg_root}/vcpkg.exe" AND NOT EXISTS "${_vcpkg_root}/vcpkg")
  message(STATUS "[vcpkg] Bootstrapping (one-time)...")
  if(WIN32)
    execute_process(
      COMMAND "${_vcpkg_root}/bootstrap-vcpkg.bat" -disableMetrics
      WORKING_DIRECTORY "${_vcpkg_root}"
      RESULT_VARIABLE _rc
    )
  else()
    execute_process(
      COMMAND "${_vcpkg_root}/bootstrap-vcpkg.sh" -disableMetrics
      WORKING_DIRECTORY "${_vcpkg_root}"
      RESULT_VARIABLE _rc
    )
  endif()
  if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "[vcpkg] bootstrap failed (rc=${_rc})")
  endif()
endif()

set(CMAKE_TOOLCHAIN_FILE "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file" FORCE)
message(STATUS "[vcpkg] Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
