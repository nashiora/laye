cmake_minimum_required(VERSION 3.15)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

if(WIN32)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/laye")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(C_WARNING_FLAGS "-Wall -Wextra -Werror=return-type -Wshadow -Wconversion -Wno-unused")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(C_WARNING_FLAGS "-Wall -Wextra -Werror=return-type -Wshadow -Wconversion -Wno-unused")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(C_WARNING_FLAGS "-Wall -Werror=return-type -Wshadow -Wconversion -Wno-unused")
else()
  set(C_WARNING_FLAGS "")
endif()

set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} ${C_WARNING_FLAGS} -lm")

# ==================== layec1 Project ====================
project(layec1
  VERSION 0.1.0
  LANGUAGES C
)

add_executable(layec1
  "src/kos/private/kos_allocator.c"
  "src/kos/private/kos_args.c"
  "src/kos/private/kos_builtins.c"
  "src/kos/private/kos_platform.c"
  "src/kos/private/kos_stb_ds.c"
  "src/kos/private/kos_string.c"

  "src/layec1-src/layec1/private/main.c"

  "src/layec1-src/layec1-lib/private/compiler.c"

  "src/layec1-src/layec1-front-laye/private/laye_front.c"
)

set_property(TARGET layec1 PROPERTY C_STANDARD 99)
target_include_directories(layec1 PUBLIC
  "./src/kos/public"
  "./src/layec1-src/layec1/public"
  "./src/layec1-src/layec1-lib/public"
  "./src/layec1-src/layec1-front-laye/public"
)

if (NOT WIN32)
  target_link_libraries(layec1 PRIVATE m)
endif()
# ==================== layec Project ====================
