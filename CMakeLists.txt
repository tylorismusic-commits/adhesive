cmake_minimum_required(VERSION 3.16)
project(adhesive VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
include(GNUInstallDirs)

enable_testing()

set(LV2_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/lv2"
    CACHE PATH "Directory where LV2 bundles are installed")
set(CLAP_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/clap"
    CACHE PATH "Directory where CLAP bundles are installed")

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(LV2 REQUIRED IMPORTED_TARGET lv2)
pkg_check_modules(LILV QUIET IMPORTED_TARGET lilv-0)
pkg_check_modules(CAIRO QUIET IMPORTED_TARGET cairo)
pkg_check_modules(FREETYPE QUIET IMPORTED_TARGET freetype2)
pkg_check_modules(X11 QUIET IMPORTED_TARGET x11)

add_library(adhesive MODULE
  src/adhesive.cpp
)
target_link_libraries(adhesive PRIVATE PkgConfig::LV2)
set_target_properties(adhesive PROPERTIES
  PREFIX ""
  OUTPUT_NAME "adhesive"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lv2/adhesive.lv2")
set(ADHESIVE_BUNDLE_FILES
  "${CMAKE_SOURCE_DIR}/lv2/manifest.ttl"
  "${CMAKE_SOURCE_DIR}/lv2/adhesive.ttl"
  "${CMAKE_SOURCE_DIR}/lv2/presets.ttl"
  "${CMAKE_SOURCE_DIR}/assets/knob.png"
  "${CMAKE_SOURCE_DIR}/assets/metal.png"
  "${CMAKE_SOURCE_DIR}/assets/fonts/Jost-ExtraBold.ttf"
  "${CMAKE_SOURCE_DIR}/assets/fonts/OFL.txt")

add_custom_target(adhesive_bundle ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/lv2/adhesive.lv2"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ADHESIVE_BUNDLE_FILES}
          "${CMAKE_BINARY_DIR}/lv2/adhesive.lv2/"
  DEPENDS ${ADHESIVE_BUNDLE_FILES})
add_dependencies(adhesive_bundle adhesive)
target_compile_features(adhesive PRIVATE cxx_std_17)
target_include_directories(adhesive PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/src/dsp"
  "${CMAKE_SOURCE_DIR}/src/adhesive"
  ${PKG_CONFIG_LV2_INCLUDE_DIRS})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(adhesive PRIVATE -Wall -Wextra -Wpedantic -fvisibility=hidden)
endif()

set(ADHESIVE_UI_TARGET "")
if(CAIRO_FOUND AND X11_FOUND AND FREETYPE_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/src/ui/adhesive_ui.cpp")
  add_library(adhesive_ui MODULE
    src/ui/adhesive_ui.cpp
  )
  set(ADHESIVE_UI_TARGET adhesive_ui)
  target_include_directories(adhesive_ui PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/dsp"
    "${CMAKE_SOURCE_DIR}/src/adhesive")
  target_link_libraries(adhesive_ui PRIVATE
    PkgConfig::CAIRO PkgConfig::X11 PkgConfig::LV2 PkgConfig::FREETYPE)
  set_target_properties(adhesive_ui PROPERTIES
    PREFIX ""
    OUTPUT_NAME "adhesiveUI"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lv2/adhesive.lv2")
  target_compile_features(adhesive_ui PRIVATE cxx_std_17)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(adhesive_ui PRIVATE -Wall -Wextra -fvisibility=hidden)
  endif()

  add_executable(adhesive_ui_preview EXCLUDE_FROM_ALL
    src/ui/adhesive_ui.cpp
  )
  target_compile_definitions(adhesive_ui_preview PRIVATE ADHESIVE_UI_PREVIEW)
  target_include_directories(adhesive_ui_preview PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/dsp"
    "${CMAKE_SOURCE_DIR}/src/adhesive")
  target_link_libraries(adhesive_ui_preview PRIVATE
    PkgConfig::CAIRO PkgConfig::X11 PkgConfig::LV2 PkgConfig::FREETYPE)

  find_library(XTST_LIBRARY Xtst)
  find_program(XVFB_RUN xvfb-run)
  if(XTST_LIBRARY)
    add_executable(adhesive_ui_host_test
      test/ui_host_test.cpp
    )
    target_include_directories(adhesive_ui_host_test PRIVATE
      "${CMAKE_SOURCE_DIR}/src/adhesive"
      ${PKG_CONFIG_LV2_INCLUDE_DIRS})
    target_link_libraries(adhesive_ui_host_test PRIVATE
      PkgConfig::CAIRO PkgConfig::X11 ${CMAKE_DL_LIBS} ${XTST_LIBRARY})
    if(XVFB_RUN)
      add_test(NAME adhesive_ui_host
        COMMAND "${XVFB_RUN}" -a -s "-screen 0 1400x1100x24"
          "$<TARGET_FILE:adhesive_ui_host_test>"
          "${CMAKE_BINARY_DIR}/lv2/adhesive.lv2/adhesiveUI.so"
          "${CMAKE_BINARY_DIR}/adhesive-ui-live.png")
      set_tests_properties(adhesive_ui_host PROPERTIES
        ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1")
    endif()
  endif()
endif()

install(TARGETS adhesive ${ADHESIVE_UI_TARGET}
  LIBRARY DESTINATION "${LV2_DIR}/adhesive.lv2")
install(FILES
  lv2/manifest.ttl
  lv2/adhesive.ttl
  lv2/presets.ttl
  assets/knob.png
  assets/metal.png
  assets/fonts/Jost-ExtraBold.ttf
  assets/fonts/OFL.txt
  DESTINATION "${LV2_DIR}/adhesive.lv2")

# ---------------------------------------------------------------------------
# CLAP build: same DSP and X11 UI, plus clap.gain-adjustment-metering so
# REAPER and other CLAP hosts can show gain reduction on their track meters.
# ---------------------------------------------------------------------------
add_library(adhesive_clap MODULE
  src/clap/adhesive_clap.cpp
)
target_link_libraries(adhesive_clap PRIVATE PkgConfig::LV2 ${CMAKE_DL_LIBS}
  Threads::Threads)
target_compile_features(adhesive_clap PRIVATE cxx_std_17)
target_include_directories(adhesive_clap PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/src/dsp"
  "${CMAKE_SOURCE_DIR}/src/adhesive"
  "${CMAKE_SOURCE_DIR}/third_party/clap/include"
  ${PKG_CONFIG_LV2_INCLUDE_DIRS})
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/clap")
set_target_properties(adhesive_clap PROPERTIES
  PREFIX ""
  OUTPUT_NAME "Adhesive"
  SUFFIX ".clap"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/clap")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(adhesive_clap PRIVATE -Wall -Wextra -fvisibility=hidden)
endif()

set(ADHESIVE_CLAP_ASSETS
  "${CMAKE_SOURCE_DIR}/assets/knob.png"
  "${CMAKE_SOURCE_DIR}/assets/metal.png"
  "${CMAKE_SOURCE_DIR}/assets/fonts/Jost-ExtraBold.ttf"
  "${CMAKE_SOURCE_DIR}/assets/fonts/OFL.txt")

if(CAIRO_FOUND AND X11_FOUND AND FREETYPE_FOUND)
  add_custom_target(adhesive_clap_bundle ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_BINARY_DIR}/clap/Adhesive.resources"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ADHESIVE_CLAP_ASSETS}
            "${CMAKE_BINARY_DIR}/clap/Adhesive.resources/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:adhesive_ui>"
            "${CMAKE_BINARY_DIR}/clap/Adhesive.resources/adhesiveUI.so"
    DEPENDS adhesive_clap adhesive_ui ${ADHESIVE_CLAP_ASSETS})
endif()

if(TARGET adhesive_clap_bundle)
  install(TARGETS adhesive_clap LIBRARY DESTINATION "${CLAP_DIR}")
  install(DIRECTORY "${CMAKE_BINARY_DIR}/clap/Adhesive.resources"
          DESTINATION "${CLAP_DIR}")
endif()

if(X11_FOUND AND XVFB_RUN)
  add_executable(adhesive_clap_host_test
    test/clap_host_test.cpp
  )
  target_include_directories(adhesive_clap_host_test PRIVATE
    "${CMAKE_SOURCE_DIR}/third_party/clap/include")
  target_link_libraries(adhesive_clap_host_test PRIVATE PkgConfig::X11
    ${CMAKE_DL_LIBS})
  add_dependencies(adhesive_clap_host_test adhesive_clap_bundle)
  add_test(NAME adhesive_clap_host
    COMMAND "${XVFB_RUN}" -a -s "-screen 0 1600x1000x24"
      "$<TARGET_FILE:adhesive_clap_host_test>"
      "${CMAKE_BINARY_DIR}/clap/Adhesive.clap"
      "com.tylorismusic.adhesive")
endif()

add_executable(adhesive_dsp_test
  test/test_dsp.cpp
)
target_include_directories(adhesive_dsp_test PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/src/dsp"
  "${CMAKE_SOURCE_DIR}/src/adhesive")
target_compile_features(adhesive_dsp_test PRIVATE cxx_std_17)
add_test(NAME adhesive_dsp COMMAND adhesive_dsp_test)

if(LILV_FOUND)
  add_executable(adhesive_host_test
    test/host_test.cpp
  )
  target_include_directories(adhesive_host_test PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/dsp"
    "${CMAKE_SOURCE_DIR}/src/adhesive")
  target_link_libraries(adhesive_host_test PRIVATE PkgConfig::LILV)
  add_test(NAME adhesive_host COMMAND adhesive_host_test "${CMAKE_BINARY_DIR}/lv2")
endif()
