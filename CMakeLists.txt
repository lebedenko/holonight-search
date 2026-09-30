cmake_minimum_required(VERSION 3.25)
project(holonight-search VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(Qt6 6.11 REQUIRED COMPONENTS Core)

add_library(holonight_search STATIC src/index.cpp)
add_library(HolonightSearch::Search ALIAS holonight_search)
set_target_properties(holonight_search PROPERTIES EXPORT_NAME Search POSITION_INDEPENDENT_CODE ON)
target_compile_features(holonight_search PUBLIC cxx_std_23)
target_compile_options(holonight_search PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(holonight_search PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(holonight_search PUBLIC Qt6::Core)

install(TARGETS holonight_search EXPORT HolonightSearchTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT HolonightSearchTargets NAMESPACE HolonightSearch:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HolonightSearch)
configure_package_config_file(cmake/HolonightSearchConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/HolonightSearchConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HolonightSearch)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/HolonightSearchConfigVersion.cmake
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/HolonightSearchConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/HolonightSearchConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HolonightSearch)
install(FILES LICENSE LICENSES/GPL-3.0-or-later.txt DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/${PROJECT_NAME})

include(CTest)
if(BUILD_TESTING)
  find_package(GTest CONFIG REQUIRED)
  add_subdirectory(tests)
endif()
