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

set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} ${C_WARNING_FLAGS}")

# ==================== clayec Project ====================
project(clayec
  VERSION 0.1.0
  LANGUAGES C
)

add_executable(clayec
  "src/kos/private/kos_allocator.c"
  "src/kos/private/kos_args.c"
  "src/kos/private/kos_builtins.c"
  "src/kos/private/kos_platform.c"
  "src/kos/private/kos_stb_ds.c"
  "src/kos/private/kos_string.c"
  "src/kos/private/kos_utf8.c"

  "src/clayec-src/clayec/private/main.c"

  "src/clayec-src/clayec-lib/private/compiler.c"
  "src/clayec-src/clayec-lib/private/diagnostic.c"

  "src/clayec-src/clayec-front-laye/private/laye_ast.c"
  "src/clayec-src/clayec-front-laye/private/laye_front.c"
  "src/clayec-src/clayec-front-laye/private/laye_lexer.c"
  "src/clayec-src/clayec-front-laye/private/laye_parser.c"
)

set_property(TARGET clayec PROPERTY C_STANDARD 99)
target_include_directories(clayec PUBLIC
  "./src/kos/public"
  "./src/clayec-src/clayec/public"
  "./src/clayec-src/clayec-lib/public"
  "./src/clayec-src/clayec-front-laye/public"
)

if (NOT WIN32)
  target_link_libraries(clayec PRIVATE m)
endif()
# ==================== layec Project ====================
