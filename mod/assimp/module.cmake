file(GLOB ASSIMP_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
set(ASSIMP_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/assimp)

if(MSVC)
  if(MSVC_TOOLSET_VERSION)
    set(ASSIMP_LIB_SUFFIX "-vc${MSVC_TOOLSET_VERSION}-mt")
  else()
    if(MSVC12)
      set(ASSIMP_LIB_SUFFIX "-vc120-mt")
    elseif(MSVC14)
      set(ASSIMP_LIB_SUFFIX "-vc140-mt")
    elseif(MSVC15)
      set(ASSIMP_LIB_SUFFIX "-vc141-mt")
    endif(MSVC12)
  endif()
endif()

set(ASSIMP_RLS_LIBRARIES
  ${ASSIMP_INSTALL_DIR}/lib/assimp${ASSIMP_LIB_SUFFIX}
  ${ASSIMP_INSTALL_DIR}/lib/IrrXML
  ${ASSIMP_INSTALL_DIR}/lib/zlibstatic
)
set(ASSIMP_DBG_LIBRARIES ${ASSIMP_RLS_LIBRARIES})

if(MSVC)
  list(TRANSFORM ASSIMP_DBG_LIBRARIES APPEND "d")
endif()

ExternalProject_Add(
  ext_assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG 8f0c6b04b2257a520aaab38421b2e090204b69df # v5.0.1
  GIT_SHALLOW true
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${ASSIMP_INSTALL_DIR}
    -DCMAKE_BUILD_TYPE=$<IF:$<CONFIG:Debug>,DEBUG,RELEASE>
    -DBUILD_SHARED_LIBS=OFF
    -DASSIMP_BUILD_ZLIB=ON
    -DASSIMP_NO_EXPORT=ON
    -DASSIMP_BUILD_ASSIMP_TOOLS=OFF
    -DASSIMP_BUILD_TESTS=OFF
  BUILD_COMMAND ${CMAKE_COMMAND} --build . --config $<IF:$<CONFIG:Debug>,Debug,Release>
  INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install --config $<IF:$<CONFIG:Debug>,Debug,Release>
)
ExternalProject_Get_Property(ext_assimp SOURCE_DIR)
ExternalProject_Get_Property(ext_assimp BINARY_DIR)

add_module(
  assimp
  CONDITION ${DEV_DBG}
  SOURCES ${ASSIMP_SOURCES}
  DEPENDENCIES ext_assimp
  INCLUDE_DIRS ${ASSIMP_INSTALL_DIR}/include
  DBG_LIBRARIES ${ASSIMP_DBG_LIBRARIES}
  RLS_LIBRARIES ${ASSIMP_RLS_LIBRARIES}
)
