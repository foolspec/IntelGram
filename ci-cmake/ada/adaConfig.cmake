# Minimal package config for desktop-app/ada in Telegram's centos_env image.
#
# The current centos_env image ships libada.a and ada.h under /usr/local but no
# longer installs adaConfig.cmake. tdesktop's packaged-library path calls
# find_package(ada REQUIRED), so provide the imported target here.

find_path(ADA_INCLUDE_DIR
  NAMES ada.h
  HINTS /usr/local/include
)

find_library(ADA_LIBRARY
  NAMES ada libada
  HINTS /usr/local/lib /usr/local/lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ada
  REQUIRED_VARS ADA_LIBRARY ADA_INCLUDE_DIR
)

if(ada_FOUND AND NOT TARGET ada::ada)
  add_library(ada::ada UNKNOWN IMPORTED)
  set_target_properties(ada::ada PROPERTIES
    IMPORTED_LOCATION "${ADA_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${ADA_INCLUDE_DIR}"
  )
endif()

set(ADA_FOUND "${ada_FOUND}")
