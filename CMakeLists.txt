cmake_minimum_required(VERSION 2.8.12)
project(lde C)
set(PROJECT_VERSION "2.7.0-DEV")

# DISABLE SOME FEATURES FROM COMMAND LINE
# cmake -DSEARCH_BISON=OFF -DSEARCH_CURSES=OFF

option(ALPHA_CODE "Enable alpha code" OFF)
option(BETA_CODE "Enable beta code" ON)
option(JQDIR "Enable custom code." OFF)
option(LDE_DEBUG "Enable some common debug settings (alpha code, beta code, compiler warnings)." OFF)
option(NO_WRITE_INODE "Do not allow writes from inode mode." OFF)
option(PARANOID "Open device/file read only at all times. There is no way to override this.  If off, --paranoid can be passed on the command line." OFF)
option(READ_PART_TABLES "This will only read one group of disk tables into memory at a time." OFF)
option(SEARCH_BISON "Include arbitrary date parsing if a BISON/YACC compiler can be found." ON)
option(SEARCH_CURSES "Include curses interface if curses library and headers can be found." ON)
if(LDE_DEBUG)
  set(ALPHA_CODE ON)
  set(BETA_CODE ON)
endif(LDE_DEBUG)

AUX_SOURCE_DIRECTORY(src SRCS)
list(APPEND SRCS "src/swiped/fileutils-3.12/filemode.c")
# message(STATUS "Initial source list: ${SRCS}")

if(${CMAKE_C_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
  if(LDE_DEBUG)
    add_compile_options(-Wall -Wno-pointer-sign)
  else(LDE_DEBUG)
    add_compile_options(-Wno-pointer-sign)
  endif(LDE_DEBUG)
elseif(${CMAKE_C_COMPILER_ID} STREQUAL MSVC)
  if(LDE_DEBUG)
  else(LDE_DEBUG)
    add_compile_options(/wd4018 /wd4146 /wd4996 /wd4244 /wd4267)
  endif(LDE_DEBUG)
endif()

include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckTypeSize)

if(SEARCH_CURSES)
  find_package(Curses)
endif(SEARCH_CURSES)
if (CURSES_FOUND)
  include_directories(${CURSES_INCLUDE_DIR})
  list(APPEND LIBS ${CURSES_LIBRARIES})
else (CURSES_FOUND)
  list(REMOVE_ITEM SRCS "src/nc_block.c" "src/nc_dir.c" "src/nc_inode.c" "src/nc_lde.c")
  # message(STATUS "Removed curses files, sources are now: ${SRCS}")
endif(CURSES_FOUND)

if(SEARCH_BISON)
  find_package(BISON)
endif(SEARCH_BISON)
if (BISON_FOUND)
  bison_target(parse src/swiped/cnews/getdate.y "${CMAKE_CURRENT_BINARY_DIR}/getdate.c")
  list(APPEND SRCS "${CMAKE_CURRENT_BINARY_DIR}/getdate.c")
else(BISON_FOUND)
  list(APPEND SRCS "src/swiped/cnews/fallback_getdate.c")
endif(BISON_FOUND)

# TESTS

enable_testing()
list(APPEND TESTING_SHELL "bash" "-c")
function(add_lde_test name)
  add_test(${name} ${TESTING_SHELL} "${CMAKE_SOURCE_DIR}/test/test.sh ${name}")
endfunction(add_lde_test name)

add_lde_test(SEARCH_EXT2_MAGIC)
add_lde_test(SEARCH_MINIX_MAGIC)
add_lde_test(SEARCH_XIAFS_MAGIC)
add_lde_test(EXT2_INODE2)
add_lde_test(MINIX_INODE2)
add_lde_test(XIAFS_INODE2)
add_lde_test(EXT2_BLOCK55)
add_lde_test(MINIX_BLOCK15)
add_lde_test(XIAFS_BLOCK55)
add_lde_test(EXT2_BLOCK55_FORCE_EXT2)
add_lde_test(EXT2_BLOCK55_FORCE_MSDOS)
add_lde_test(EXT2_SUPERSCAN)
add_lde_test(XIAFS_SUPERSCAN)
add_lde_test(MINIX_SUPERSCAN)
add_lde_test(EXT2_ILOOKUP)
add_lde_test(XIAFS_ILOOKUP)
add_lde_test(MINIX_ILOOKUP)
add_lde_test(EXT2_ILOOKUPALL)
add_lde_test(XIAFS_ILOOKUPALL)
add_lde_test(MINIX_ILOOKUPALL)
add_lde_test(MINIX_RECOVER)
add_lde_test(XIAFS_RECOVER)
add_lde_test(EXT2_INDIRECTS)
add_lde_test(XIAFS_INDIRECTS)
add_lde_test(MINIX_INDIRECTS)

# INCLUDES

check_include_file(errno.h HAVE_ERRNO_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(grp.h HAVE_GRP_H)
check_include_file(io.h HAVE_IO_H)
check_include_file(pwd.h HAVE_PWD_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)

check_include_file(getopt.h HAVE_GETOPT_H)
if(HAVE_GETOPT_H)
else()
  set(HAVE_GETOPT_H 1)
  include_directories("src/swiped/getopt")
  list(APPEND SRCS "src/swiped/getopt/getopt.c" "src/swiped/getopt/getopt.h")
endif(HAVE_GETOPT_H)

# FUNCTIONS

check_symbol_exists(daylight time.h HAVE_DAYLIGHT)
check_symbol_exists(getgrgid grp.h HAVE_GETGRGID)
check_symbol_exists(getopt_long getopt.h HAVE_GETOPT_LONG)
check_symbol_exists(getpwuid pwd.h HAVE_GETPWUID)
check_symbol_exists(O_BINARY fcntl.h HAVE_O_BINARY)
check_symbol_exists(sigaction signal.h HAVE_SIGACTION)
check_symbol_exists(sync unistd.h HAVE_SYNC)
check_symbol_exists(timezone time.h HAVE_TIMEZONE)
check_symbol_exists(uname sys/utsname.h HAVE_UNAME)

check_symbol_exists(lseek64 unistd.h;sys/types.h HAVE_LSEEK64)
if(HAVE_LSEEK64)
else(HAVE_LSEEK64)
  set(CMAKE_REQUIRED_DEFINITIONS -D__USE_LARGEFILE64)
  check_symbol_exists(lseek64 unistd.h;sys/types.h HAVE_LSEEK64_WITH_LARGE)
  if(HAVE_LSEEK64_WITH_LARGE)
    set(HAVE_LSEEK64 1)
    set(LDE_USE_LARGEFILE64 1)
  endif(HAVE_LSEEK64_WITH_LARGE)
  unset(CMAKE_REQUIRED_DEFINITIONS)
endif(HAVE_LSEEK64)

check_symbol_exists(memmem "string.h" HAVE_MEMMEM)
if(HAVE_MEMMEM)
else(HAVE_MEMMEM)
  set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
  check_symbol_exists(memmem "string.h" HAVE_MEMMEM_GNU)
  if(HAVE_MEMMEM_GNU)
    set(HAVE_MEMMEM 1)
    set(LDE_GNU_SOURCE 1)
  endif(HAVE_MEMMEM_GNU)
  unset(CMAKE_REQUIRED_DEFINITIONS)
endif(HAVE_MEMMEM)

# TYPES

check_type_size(off_t SIZEOF_OFF_T)

# TARGETS

add_custom_target(tar
  COMMAND tar -cvz
           -C .. 
           --exclude RCS
           --exclude CVS
           --exclude .cvsignore
           --exclude .vscode
           --exclude *~
           --exclude .depend
           -f lde-${PROJECT_VERSION}.tar.gz
           lde/src
           lde/CMakeLists.txt
           lde/INSTALL.LDE
           lde/README
           lde/README.cygwin
           lde/TODO
           lde/WARNING
           lde/crash_recovery
           lde/doc
           lde/lde${CMAKE_EXECUTABLE_SUFFIX} 
           lde/other
  EXCLUDE_FROM_ALL)

include_directories("src" "${CMAKE_CURRENT_BINARY_DIR}/src")
add_executable(lde ${SRCS})
install(TARGETS lde RUNTIME DESTINATION bin)
target_link_libraries(lde PUBLIC ${LIBS})

configure_file("config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/src/lde_config.h")
