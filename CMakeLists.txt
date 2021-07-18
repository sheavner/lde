cmake_minimum_required(VERSION 2.8.12)
project(lde C)
set(PROJECT_VERSION 2.6.2)
ADD_DEFINITIONS(-DLDE_VERSION="${PROJECT_VERSION}")

# DISABLE SOME FEATURES FROM COMMAND LINE
# cmake -DCURSES_FOUND=FALSE -DBISON_FOUND=FALSE .

AUX_SOURCE_DIRECTORY(src SRCS)
list(APPEND SRCS "src/swiped/fileutils-3.12/filemode.c")
# message(STATUS "Initial source list: ${SRCS}")

if(${CMAKE_C_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
  ADD_DEFINITIONS(-Wno-pointer-sign)
elseif(${CMAKE_C_COMPILER_ID} STREQUAL MSVC)
  add_compile_options(/wd4018 /wd4146 /wd4996 /wd4244 /wd4267)
  ADD_DEFINITIONS(-D_CRT_DECLARE_NONSTDC_NAMES)
endif()

include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckTypeSize)

find_package(Curses)
if (CURSES_FOUND)
else (CURSES_FOUND)
    list(REMOVE_ITEM SRCS "src/nc_block.c" "src/nc_dir.c" "src/nc_inode.c" "src/nc_lde.c")
    # message(STATUS "Removed curses files, sources are now: ${SRCS}")
endif(CURSES_FOUND)

find_package(BISON)
if (BISON_FOUND)
    bison_target(parse src/swiped/cnews/getdate.y "${CMAKE_CURRENT_BINARY_DIR}/getdate.c")
    list(APPEND SRCS "${CMAKE_CURRENT_BINARY_DIR}/getdate.c")
endif(BISON_FOUND)

add_executable(lde ${SRCS})
install(TARGETS lde RUNTIME DESTINATION bin)

if (CURSES_FOUND)
    include_directories(${CURSES_INCLUDE_DIR})
    target_link_libraries(lde ${CURSES_LIBRARIES})
endif(CURSES_FOUND)

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
  )

# INCLUDES

CHECK_INCLUDE_FILE(asm/types.h HAVE_ASM_TYPES_H)
IF(HAVE_ASM_TYPES_H)
  ADD_DEFINITIONS(-DHAVE_ASM_TYPES_H)
ENDIF(HAVE_ASM_TYPES_H)

CHECK_INCLUDE_FILE(errno.h HAVE_ERRNO_H)
IF(HAVE_ERRNO_H)
  ADD_DEFINITIONS(-DHAVE_ERRNO_H)
ENDIF(HAVE_ERRNO_H)

CHECK_INCLUDE_FILE(fcntl.h HAVE_FCNTL_H)
IF(HAVE_FCNTL_H)
  ADD_DEFINITIONS(-DHAVE_FCNTL_H)
ENDIF(HAVE_FCNTL_H)

CHECK_INCLUDE_FILE(getopt.h HAVE_GETOPT_H)
IF(HAVE_GETOPT_H)
  ADD_DEFINITIONS(-DHAVE_GETOPT_H)
ELSE()
  ADD_DEFINITIONS(-DHAVE_GETOPT_H)
  include_directories("src/swiped/getopt")
  set_property(TARGET lde APPEND PROPERTY SOURCES "src/swiped/getopt/getopt.c" "src/swiped/getopt/getopt.h")
ENDIF(HAVE_GETOPT_H)

CHECK_INCLUDE_FILE(grp.h HAVE_GRP_H)
IF(HAVE_GRP_H)
  ADD_DEFINITIONS(-DHAVE_GRP_H)
ENDIF(HAVE_GRP_H)

CHECK_INCLUDE_FILE(io.h HAVE_IO_H)
IF(HAVE_IO_H)
  ADD_DEFINITIONS(-DHAVE_IO_H)
ENDIF(HAVE_IO_H)

CHECK_INCLUDE_FILE(pwd.h HAVE_PWD_H)
IF(HAVE_PWD_H)
  ADD_DEFINITIONS(-DHAVE_PWD_H)
ENDIF(HAVE_PWD_H)

CHECK_INCLUDE_FILE(sys/fcntl.h HAVE_SYS_FCNTL_H)
IF(HAVE_SYS_FCNTL_H)
  ADD_DEFINITIONS(-DHAVE_SYS_FCNTL_H)
ENDIF(HAVE_SYS_FCNTL_H)

CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
IF(HAVE_SYS_TYPES_H)
  ADD_DEFINITIONS(-DHAVE_SYS_TYPES_H)
ENDIF(HAVE_SYS_TYPES_H)

CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
IF(HAVE_UNISTD_H)
  ADD_DEFINITIONS(-DHAVE_UNISTD_H)
ENDIF(HAVE_UNISTD_H)

# FUNCTIONS

IF(CURSES_FOUND)
  ADD_DEFINITIONS(-DHAVE_CURSES)
ENDIF(CURSES_FOUND)

set(CMAKE_EXTRA_INCLUDE_FILES asm/types.h)
check_type_size(__u64 SIZEOF_U64)
IF(HAVE_SIZEOF_U64)
  ADD_DEFINITIONS(-DHAVE_U64)
ENDIF(HAVE_SIZEOF_U64)
unset(CMAKE_EXTRA_INCLUDE_FILES)

check_symbol_exists(bzero strings.h HAVE_BZERO)
IF(HAVE_BZERO)
  ADD_DEFINITIONS(-DHAVE_BZERO)
ENDIF(HAVE_BZERO)

check_symbol_exists(daylight time.h HAVE_DAYLIGHT)
IF(HAVE_DAYLIGHT)
  ADD_DEFINITIONS(-DHAVE_DAYLIGHT)
ENDIF(HAVE_DAYLIGHT)

check_symbol_exists(getgrgid grp.h HAVE_GETGRGID)
IF(HAVE_GETGRGID)
  ADD_DEFINITIONS(-DHAVE_GETGRGID)
ENDIF(HAVE_GETGRGID)

check_symbol_exists(getopt_long getopt.h HAVE_GETOPT_LONG)
IF(HAVE_GETOPT_LONG)
  ADD_DEFINITIONS(-DHAVE_GETOPT_LONG)
ENDIF(HAVE_GETOPT_LONG)

check_symbol_exists(getpwuid pwd.h HAVE_GETPWUID)
IF(HAVE_GETPWUID)
  ADD_DEFINITIONS(-DHAVE_GETPWUID)
ENDIF(HAVE_GETPWUID)

check_type_size(off_t SIZEOF_OFF_T)
IF(HAVE_SIZEOF_OFF_T)
  ADD_DEFINITIONS(-DSIZEOF_OFF_T=${SIZEOF_OFF_T})
ENDIF(HAVE_SIZEOF_OFF_T)

set(CMAKE_REQUIRED_DEFINITIONS -D__USE_LARGEFILE64)
check_symbol_exists(lseek64 unistd.h;sys/types.h HAVE_LSEEK64)
IF(HAVE_LSEEK64)
  ADD_DEFINITIONS(-DHAVE_LSEEK64 -D__USE_LARGEFILE64)
ENDIF(HAVE_LSEEK64)
unset(CMAKE_REQUIRED_DEFINITIONS)

check_symbol_exists(memmem "string.h" HAVE_MEMMEM)
IF(HAVE_MEMMEM)
  ADD_DEFINITIONS(-DHAVE_MEMMEM)
ELSE(HAVE_MEMMEM)
  set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
  check_symbol_exists(memmem "string.h" HAVE_MEMMEM_GNU)
  IF(HAVE_MEMMEM_GNU)
    ADD_DEFINITIONS(-D_GNU_SOURCE)
    ADD_DEFINITIONS(-DHAVE_MEMMEM)
  ENDIF(HAVE_MEMMEM_GNU)
  unset(CMAKE_REQUIRED_DEFINITIONS)
ENDIF(HAVE_MEMMEM)

check_symbol_exists(sigaction signal.h HAVE_SIGACTION)
IF(HAVE_SIGACTION)
  ADD_DEFINITIONS(-DHAVE_SIGACTION)
ENDIF(HAVE_SIGACTION)

check_symbol_exists(sync unistd.h HAVE_SYNC)
IF(HAVE_SYNC)
  ADD_DEFINITIONS(-DHAVE_SYNC)
ENDIF(HAVE_SYNC)

check_symbol_exists(timezone time.h HAVE_TIMEZONE)
IF(HAVE_TIMEZONE)
  ADD_DEFINITIONS(-DHAVE_TIMEZONE)
ENDIF(HAVE_TIMEZONE)

check_symbol_exists(uname sys/utsname.h HAVE_UNAME)
IF(HAVE_UNAME)
  ADD_DEFINITIONS(-DHAVE_UNAME)
ENDIF(HAVE_UNAME)
