if(NOT DEVILUTIONX_SYSTEM_LIBSODIUM)
  include(functions/FetchContent_ExcludeFromAll_backport)

  set(SODIUM_MINIMAL ON)
  set(SODIUM_ENABLE_BLOCKING_RANDOM OFF)
  set(SODIUM_DISABLE_TESTS ON)

  if(DEVILUTIONX_STATIC_LIBSODIUM)
    set(BUILD_SHARED_LIBS OFF)
  else()
    set(BUILD_SHARED_LIBS ON)
  endif()
  include(FetchContent)
  FetchContent_Declare_ExcludeFromAll(libsodium
      GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake.git
      GIT_TAG a8ac4509b22b84d6c2eb7d7448f08678e4a67da6
  )
  FetchContent_MakeAvailable_ExcludeFromAll(libsodium)
endif()
