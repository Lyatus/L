set(GLSLANG_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/glslang)

ExternalProject_Add(
  ext_glslang
  GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
  GIT_TAG 8db9eccc0baf30c9d22c496ab28db0fe1e4e97c5 # v8.13.3559
  GIT_SHALLOW true
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${GLSLANG_INSTALL_DIR}
  BUILD_COMMAND ${CMAKE_COMMAND} --build . --config $<IF:$<CONFIG:Debug>,Debug,Release>
  INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install --config $<IF:$<CONFIG:Debug>,Debug,Release>
)
ExternalProject_Get_Property(ext_glslang SOURCE_DIR)
ExternalProject_Get_Property(ext_glslang BINARY_DIR)

add_module(
  glsl
  CONDITION ${DEV_DBG}
  SOURCES ${CMAKE_CURRENT_LIST_DIR}/glsl.cpp
  DEPENDENCIES ext_glslang
  INCLUDE_DIRS ${GLSLANG_INSTALL_DIR}/include
  DBG_LIBRARIES
    ${GLSLANG_INSTALL_DIR}/lib/${CMAKE_LINK_LIBRARY_PREFIX}glslangd${CMAKE_LINK_LIBRARY_SUFFIX}
    ${GLSLANG_INSTALL_DIR}/lib/${CMAKE_LINK_LIBRARY_PREFIX}OSDependentd${CMAKE_LINK_LIBRARY_SUFFIX}
    ${GLSLANG_INSTALL_DIR}/lib/${CMAKE_LINK_LIBRARY_PREFIX}HLSLd${CMAKE_LINK_LIBRARY_SUFFIX}
    ${GLSLANG_INSTALL_DIR}/lib/${CMAKE_LINK_LIBRARY_PREFIX}OGLCompilerd${CMAKE_LINK_LIBRARY_SUFFIX}
    ${GLSLANG_INSTALL_DIR}/lib/${CMAKE_LINK_LIBRARY_PREFIX}SPIRVd${CMAKE_LINK_LIBRARY_SUFFIX}
  RLS_LIBRARIES
    ${GLSLANG_INSTALL_DIR}/lib/${CMAKE_LINK_LIBRARY_PREFIX}glslang${CMAKE_LINK_LIBRARY_SUFFIX}
    ${GLSLANG_INSTALL_DIR}/lib/${CMAKE_LINK_LIBRARY_PREFIX}OSDependent${CMAKE_LINK_LIBRARY_SUFFIX}
    ${GLSLANG_INSTALL_DIR}/lib/${CMAKE_LINK_LIBRARY_PREFIX}HLSL${CMAKE_LINK_LIBRARY_SUFFIX}
    ${GLSLANG_INSTALL_DIR}/lib/${CMAKE_LINK_LIBRARY_PREFIX}OGLCompiler${CMAKE_LINK_LIBRARY_SUFFIX}
    ${GLSLANG_INSTALL_DIR}/lib/${CMAKE_LINK_LIBRARY_PREFIX}SPIRV${CMAKE_LINK_LIBRARY_SUFFIX}
)