cmake_minimum_required(VERSION 3.10)
set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Override platform if generating for Visual Studio

string(REGEX MATCH "Visual Studio" IS_VISUAL_STUDIO ${CMAKE_GENERATOR})
if(IS_VISUAL_STUDIO)
  set(CMAKE_GENERATOR_PLATFORM x64)
endif()

include(CheckCXXCompilerFlag)
include(ExternalProject)

project(L)

# Configuration types

set(DEV_DBG $<NOT:$<CONFIG:Release>>)
set(RLS $<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>)
add_compile_definitions(
  L_DBG=$<CONFIG:Debug>
  L_RLS=${RLS}
  L_LINUX=$<PLATFORM_ID:Linux>
  L_WINDOWS=$<PLATFORM_ID:Windows>
)
set(CMAKE_EXE_LINKER_FLAGS_DEVELOPMENT ${CMAKE_EXE_LINKER_FLAGS_DEBUG} CACHE INTERNAL "Configs")

# Compiler options

option(L_USE_ASAN "Use address sanitizer" OFF)

function(add_compile_options_safe)
  foreach(OPT ${ARGV})
    check_cxx_compiler_flag(${OPT} HAS_COMPILER_OPTION_${OPT})
    if(HAS_COMPILER_OPTION_${OPT})
      add_compile_options(${OPT})
    endif()
  endforeach()
endfunction()

if(MSVC)
  add_compile_definitions(
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    NOCRYPT
    _CRT_SECURE_NO_WARNINGS
    _WINSOCK_DEPRECATED_NO_WARNINGS
    _HAS_EXCEPTIONS=0
  )

  string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  add_compile_options(
    /GR-
    /fp:fast
    /WX /W4
  )
  add_link_options(
    $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>
  )

  if(${L_USE_ASAN})
    add_compile_options(/fsanitize=address)
  endif()
else()
  add_compile_options(
    -fno-operator-names
    -fno-exceptions
    -fno-rtti
    -ffast-math
    -Werror -Wall
  )
  add_compile_options_safe(
    -Wno-class-memaccess
    -Wno-missing-braces
    -Wno-missing-field-initializers
    -Wno-type-limits
    -Wno-unused-function
  )

  if(${L_USE_ASAN})
    add_compile_options(-fsanitize=address)
  endif()
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Libraries

if(WIN32)
  link_libraries(ws2_32 $<${DEV_DBG}:dbghelp>)
else()
  link_libraries(X11 pthread)
endif()

# Core

file(GLOB_RECURSE CORE_SOURCES src/*.cpp src/*.inl src/*.h)
if(WIN32)
  list(FILTER CORE_SOURCES EXCLUDE REGEX .*_unix.cpp)
else()
  list(FILTER CORE_SOURCES EXCLUDE REGEX .*_win.cpp)
endif()
# The main.cpp file is common but compiled by each project separately
list(FILTER CORE_SOURCES EXCLUDE REGEX main.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${CORE_SOURCES})

add_library(core ${CORE_SOURCES})

# Engine settings

option(L_USE_DEBUG_ALLOC "Use system page allocator to track memory issues (expect disastrous performance)" OFF)
option(L_USE_MALLOC "Use malloc/free instead of custom allocator" OFF)
option(L_USE_COMPRESSION "Use compression for archived resources (disabled by default for faster iteration times)" OFF)
option(L_USE_PROFILING "Use timers and create a trace.json file on shutdown" OFF)
target_compile_definitions(core PRIVATE L_USE_DEBUG_ALLOC=$<BOOL:${L_USE_DEBUG_ALLOC}>)
target_compile_definitions(core PRIVATE L_USE_MALLOC=$<BOOL:${L_USE_MALLOC}>)
target_compile_definitions(core PRIVATE L_USE_COMPRESSION=$<BOOL:${L_USE_COMPRESSION}>)
target_compile_definitions(core PRIVATE L_USE_PROFILING=$<BOOL:${L_USE_PROFILING}>)

# Externals - function

function(add_external NAME)
  set(OPTARGS
    CMAKE
    CONFIG_INSTALL_DIR)
  set(SINGLEARGS
    GIT_REPOSITORY
    GIT_TAG)
  set(MULTIARGS
    INCLUDE_DIRS
    LIBRARIES DBG_LIBRARIES RLS_LIBRARIES
    CMAKE_ARGS
  )
  cmake_parse_arguments(E "${OPTARGS}" "${SINGLEARGS}" "${MULTIARGS}" ${ARGN})

  # Setup install directory
  set(EXT_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/ext/${NAME})
  if(E_CONFIG_INSTALL_DIR)
    # External needs multiple install directories for each config
    set(EXT_INSTALL_DIR ${EXT_INSTALL_DIR}/$<CONFIG>)
  endif()

  if(E_CMAKE) # Build with CMake via ExternalProject
    ExternalProject_Add(
      ext_${NAME}
      GIT_REPOSITORY ${E_GIT_REPOSITORY}
      GIT_TAG ${E_GIT_TAG}
      UPDATE_COMMAND ""
      CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${EXT_INSTALL_DIR}
        ${E_CMAKE_ARGS}
    )
  else() # No build process, only fetch from git
    ExternalProject_Add(
      ext_${NAME}
      GIT_REPOSITORY ${E_GIT_REPOSITORY}
      GIT_TAG ${E_GIT_TAG}
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      UPDATE_COMMAND git fetch --all && git reset --hard ${E_GIT_TAG}
      INSTALL_COMMAND ""
      SOURCE_DIR ${EXT_INSTALL_DIR}
    )
  endif()
  set_target_properties(ext_${NAME} PROPERTIES FOLDER "Externals")

  # Include directories
  if(DEFINED E_INCLUDE_DIRS)
    list(TRANSFORM E_INCLUDE_DIRS PREPEND ${EXT_INSTALL_DIR}/)
    set(ENV{EXT_${NAME}_INCLUDE_DIRS} "${E_INCLUDE_DIRS}")
  else()
    set(ENV{EXT_${NAME}_INCLUDE_DIRS} "${EXT_INSTALL_DIR}")
  endif()

  # Libraries
  foreach(LIBS IN ITEMS LIBRARIES DBG_LIBRARIES RLS_LIBRARIES)
    list(TRANSFORM E_${LIBS} PREPEND ${EXT_INSTALL_DIR}/)
    set(ENV{EXT_${NAME}_${LIBS}} "${E_${LIBS}}")
  endforeach()
endfunction()

# Modules - mod list file

set(MOD_LIST_GEN_PATH ${CMAKE_CURRENT_BINARY_DIR}/mod_list.gen)
file(WRITE ${MOD_LIST_GEN_PATH} "// This file was generated by CMake, do not modify manually\n")

# Modules - functions

function(add_module NAME)
  message(STATUS "Adding module ${NAME}")
  set(OPTARGS "")
  set(SINGLEARGS CONDITION)
  set(MULTIARGS
    INCLUDE_DIRS SYSTEM_INCLUDE_DIRS
    LIBRARIES DBG_LIBRARIES RLS_LIBRARIES
    MOD_DEPENDENCIES OPT_MOD_DEPENDENCIES
    EXT_DEPENDENCIES
  )
  cmake_parse_arguments(M "${OPTARGS}" "${SINGLEARGS}" "${MULTIARGS}" ${ARGN})

  if(NOT DEFINED M_CONDITION)
    set(M_CONDITION 1) # Always use module if no condition set
  endif()

  set(ENV{MOD_${NAME}_CONDITION} ${M_CONDITION})
  set(ENV{MOD_${NAME}_DEPENDENCIES} "${M_MOD_DEPENDENCIES}")

  file(GLOB_RECURSE MOD_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/*.inl
    ${CMAKE_CURRENT_LIST_DIR}/*.h)

  add_library(mod_${NAME} ${MOD_SOURCES})
  set_target_properties(
    mod_${NAME} PROPERTIES
    EXCLUDE_FROM_ALL TRUE
    FOLDER "Modules"
  )

  # Dependencies
  target_link_libraries(mod_${NAME} core)
  foreach(MOD_DEP ${M_MOD_DEPENDENCIES})
    target_module_dependency(mod_${NAME} ${MOD_DEP} TRUE)
  endforeach()
  foreach(MOD_DEP ${M_OPT_MOD_DEPENDENCIES})
    target_module_dependency(mod_${NAME} ${MOD_DEP} FALSE)
  endforeach()
  foreach(EXT_DEP ${M_EXT_DEPENDENCIES})
    # Read external definition if not already defined
    if(NOT TARGET ext_${EXT_DEP})
      include(${CMAKE_CURRENT_SOURCE_DIR}/ext/${EXT_DEP}.cmake)
    endif()
    add_dependencies(mod_${NAME} ext_${EXT_DEP})

    # Include directories
    list(APPEND M_SYSTEM_INCLUDE_DIRS $ENV{EXT_${EXT_DEP}_INCLUDE_DIRS})

    # Libraries
    foreach(LIBS IN ITEMS LIBRARIES DBG_LIBRARIES RLS_LIBRARIES)
      list(APPEND M_${LIBS} $ENV{EXT_${EXT_DEP}_${LIBS}})
    endforeach()
  endforeach()

  # Module directories
  target_include_directories(mod_${NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/mod/${NAME}/include) # Own include directory
  target_include_directories(mod_${NAME} PUBLIC ${M_INCLUDE_DIRS})
  target_include_directories(mod_${NAME} SYSTEM PUBLIC ${M_SYSTEM_INCLUDE_DIRS})

  # Libraries
  set(MOD_LIB_PATTERN "^(.*/)(${CMAKE_STATIC_LIBRARY_PREFIX})?([^.]*)$")
  set(MOD_LIB_REPLACE "\\1${CMAKE_STATIC_LIBRARY_PREFIX}\\3${CMAKE_STATIC_LIBRARY_SUFFIX}")
  list(TRANSFORM M_LIBRARIES REPLACE ${MOD_LIB_PATTERN} ${MOD_LIB_REPLACE})
  list(TRANSFORM M_DBG_LIBRARIES REPLACE ${MOD_LIB_PATTERN} ${MOD_LIB_REPLACE})
  list(TRANSFORM M_RLS_LIBRARIES REPLACE ${MOD_LIB_PATTERN} ${MOD_LIB_REPLACE})
  target_link_libraries(mod_${NAME} ${M_LIBRARIES})
  foreach(LIB ${M_DBG_LIBRARIES})
    target_link_libraries(mod_${NAME} debug ${LIB})
  endforeach()
  foreach(LIB ${M_RLS_LIBRARIES})
    target_link_libraries(mod_${NAME} optimized ${LIB})
  endforeach()

  file(APPEND ${MOD_LIST_GEN_PATH} "#ifdef L_USE_MODULE_${NAME}\nMOD(${NAME})\n#endif\n")
endfunction()

function(require_module NAME)
  if(NOT TARGET mod_${NAME})
    include(${CMAKE_CURRENT_SOURCE_DIR}/mod/${NAME}/module.cmake)
  endif()
endfunction()

# Projects - functions

function(target_module_dependency TARGET MOD_NAME REQUIRED)
  if(${REQUIRED})
    require_module(${MOD_NAME})
  endif()
  if(TARGET mod_${MOD_NAME})
    set(MOD_COND $ENV{MOD_${MOD_NAME}_CONDITION})
    target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mod/${MOD_NAME}/include)
    target_compile_definitions(${TARGET} PRIVATE L_USE_MODULE_${MOD_NAME}=${MOD_COND})
    target_link_libraries(${TARGET} $<${MOD_COND}:mod_${MOD_NAME}>)
    foreach(MOD_DEP $ENV{MOD_${MOD_NAME}_DEPENDENCIES})
      target_module_dependency(${TARGET} ${MOD_DEP} ${REQUIRED})
    endforeach()
  endif()
endfunction()

function(add_project NAME)
  message(STATUS "Adding project ${NAME}")
  set(OPTARGS "")
  set(SINGLEARGS
    DIRECTORY
    CPP_INIT_FUNCTION
    SCRIPT_INIT_FILE
  )
  set(MULTIARGS
    MOD_DEPENDENCIES
  )
  cmake_parse_arguments(P "${OPTARGS}" "${SINGLEARGS}" "${MULTIARGS}" ${ARGN})

  set(PRJ_NAME prj_${NAME})
  if(NOT DEFINED P_DIRECTORY)
    set(P_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
  endif()

  file(GLOB_RECURSE PRJ_SOURCES
    ${P_DIRECTORY}/*.cpp
    ${P_DIRECTORY}/*.inl
    ${P_DIRECTORY}/*.h
  )
  list(APPEND PRJ_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
  if(MSVC)
    list(APPEND PRJ_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/src/resource.rc
      ${CMAKE_CURRENT_SOURCE_DIR}/src/L.natvis
    )
  endif()
  add_executable(${PRJ_NAME} ${PRJ_SOURCES})
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PRJ_NAME})
  if(DEFINED P_CPP_INIT_FUNCTION)
    target_compile_definitions(${PRJ_NAME} PRIVATE L_CPP_INIT_FUNCTION=${P_CPP_INIT_FUNCTION})
  endif()
  if(DEFINED P_SCRIPT_INIT_FILE)
    target_compile_definitions(${PRJ_NAME} PRIVATE L_SCRIPT_INIT_FILE="${P_SCRIPT_INIT_FILE}")
  endif()
  target_link_libraries(${PRJ_NAME} core)
  set_target_properties(
    ${PRJ_NAME} PROPERTIES
    FOLDER "Projects"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${P_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${P_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${P_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${P_DIRECTORY}
    VS_DEBUGGER_WORKING_DIRECTORY ${P_DIRECTORY}
    OUTPUT_NAME_DEBUG ${NAME}_dbg
    OUTPUT_NAME_RELWITHDEBINFO ${NAME}_dev
    OUTPUT_NAME_RELEASE ${NAME}
    OUTPUT_NAME_MINSIZEREL ${NAME}_min
  )

  foreach(MOD_DEP ${P_MOD_DEPENDENCIES})
    target_module_dependency(${PRJ_NAME} ${MOD_DEP} TRUE)
  endforeach()
endfunction()

# Projects - test project

file(GLOB TEST_MODULES mod/test_*)
list(TRANSFORM TEST_MODULES REPLACE ".*/" "")

add_project(
  tests
  DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/Testing
  MOD_DEPENDENCIES ${TEST_MODULES}
)

enable_testing()
add_test(
  NAME all_tests
  COMMAND $<TARGET_FILE:prj_tests> --run_all_tests
)

set_target_properties(
  prj_tests PROPERTIES
  VS_DEBUGGER_COMMAND_ARGUMENTS "--run_all_tests"
)

# Projects - iterate

set(L_PROJECTS "${CMAKE_CURRENT_SOURCE_DIR}/smp" CACHE PATH "Project paths (separated by semicolons)")
foreach(PROJECT_PATH ${L_PROJECTS})
  include(${PROJECT_PATH}/project.cmake)
endforeach()

# Generated sources

configure_file(${MOD_LIST_GEN_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/src/mod_list.gen)
