find_path(XINPUT2_PATHS X11/extensions/XInput2.h)
if(NOT XINPUT2_PATHS)
  message("Could NOT find XInput2")
  unset(XINPUT2_PATHS CACHE)
  return()
else()
  unset(XINPUT2_PATHS CACHE)
endif()

add_module(
  xinput_unix
  CONDITION $<PLATFORM_ID:Linux>
  SOURCES ${CMAKE_CURRENT_LIST_DIR}/xinput_unix.cpp
  LIBRARIES dl
)
