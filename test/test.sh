#!/bin/bash

##################################################################
# Automated test scripts for lde
#
# (C) 2002, 2021 Scott Heavner, GPL
#
##################################################################

# Configuration ----------------------
DIFF="diff -b"
RM="rm -f"
if [ "x${VERBOSE}" = x ] ; then
  VERBOSE=1
fi
if [ "x${ECHO_CMDS}" = x ] ; then
  ECHO_CMDS=0
fi
if [ "x${RETAIN}" = x ] ; then
  RETAIN=0
fi
if [ "x${STOPONERROR}" = x ] ; then
  STOPONERROR=0
fi
if [ "x${LDE_TEST_TZ}" != "x" ] ; then
  export TZ="${LDE_TEST_TZ}"
fi
# End Configuration ------------------

START_DIR="${PWD}"

cd "${0%/*}"

if [ "x${LDE}" != x -a -f "${LDE}" -a -x "${LDE}" ] ; then
  true # use env variable
elif [ "x${CI_CMAKE_BIN_PATH}" != x -a -f "${CI_CMAKE_BIN_PATH}/lde${EXE}" -a -x "${CI_CMAKE_BIN_PATH}/lde${EXE}" ] ; then
  LDE="${CI_CMAKE_BIN_PATH}/lde${EXE}"
elif [ "x${START_DIR}" != x -a -f "${START_DIR}/lde${EXE}" -a -x "${START_DIR}/lde${EXE}" ] ; then
  LDE="${START_DIR}/lde${EXE}"
elif [ -f ../lde${EXE} -a -x ../lde${EXE} ] ; then
  LDE=../lde${EXE}
elif [ -f ../lde/lde${EXE} -a -x ../lde/lde${EXE} ] ; then
  LDE=../lde/lde${EXE}
else
  echo "Can't find lde executable, aborting test."
  if [ x$VERBOSE = x1 ] ; then
    echo "EXE=${EXE}"
    echo "LDE=${LDE}"
    echo "CI_CMAKE_BIN_PATH=${CI_CMAKE_BIN_PATH}"
    echo "START_DIR=${START_DIR}"
    echo "ls = $(ls)"
    echo "ls CI_CMAKE_BIN_PATH = $(ls ${CI_CMAKE_BIN_PATH})"
    echo "ls START_DIR = $(find ${START_DIR})"
  fi
  exit 1
fi

ONETEST=
if [ x$1 != x ] ; then
  ONETEST=$1
fi

let TESTS=0
let SUCCESS=0

if [ x$ECHO_CMDS = x1 ] ; then
  set -x
fi

function ldetest {

  # $1 0 = expect zero return from executable, F = expect non-zero return [failure]
  # $2 = test label
  # $3+ = command

  expected_status=$1
  shift

  if [ x$ONETEST != x -a x$ONETEST != x$1 ] ; then
    return
  fi

  status=0

  label=$1
  shift

  if [ x$VERBOSE = x1 ] ; then
    echo -n "Test: $label ... "
  fi

  TMPFILE=results/${label}.$$

  "$@" > $TMPFILE 2>&1
  exit_status=$?
  if [ ${expected_status} == F -a ${exit_status} == 0 ] || [ ${expected_status} == 0 -a ${exit_status} != 0 ] ; then
    status="Execution status [${exit_status}] not as expected [${expected_status}]."
  elif ! $DIFF $TMPFILE expected/${label} > results/diff1.$$ 2>&1 ; then
    status='Unexpected output.'
    if [ x$VERBOSE = x1 ] ; then
      cat results/diff1.$$
    fi
  elif [ -f results/${label} ] ; then
    if ! $DIFF results/${label} expected/${label}_RESULTS > results/diff2.$$ 2>&1 ; then
      status='Unexpected results.'
      if [ x$VERBOSE = x1 ] ; then
        cat results/diff2.$$
      fi
    fi
  fi

  if [ "x$status" != "x0" ] ; then
    echo "*** $status ***************"
    if [ x$STOPONERROR = x1 ] ; then
      exit 1
    fi
  else
    if [ x$VERBOSE = x1 ] ; then
      echo ok
    fi
    let SUCCESS=$SUCCESS+1
  fi
  if [ x$RETAIN != x1 ] ; then
    $RM $TMPFILE results/diff1.$$ results/diff2.$$ results/${label}
  fi

  let TESTS=$TESTS+1
}

$RM results/* 2> /dev/null

if ! "$LDE" -v > /dev/null ; then
  echo "Cannot run $LDE"
  exit 1
fi

# These fail on cygwin if test.ext2 comes before -O
ldetest 0 SEARCH_EXT2_MAGIC "$LDE" -a -t no -T search/ext2mag -O 56 -L 2 test.ext2
ldetest 0 SEARCH_MINIX_MAGIC "$LDE" -a -t no -T search/minix-mag -O 16 -L 2 test.minix
ldetest 0 SEARCH_XIAFS_MAGIC "$LDE" -s 512 -a -t no -T search/xiafs-mag -O 60 -L 2 test.xiafs

# Need to supress symbolic uid/gid will vary system to system
ldetest 0 EXT2_INODE2 "$LDE" -yi 2 test.ext2
ldetest 0 MINIX_INODE2 "$LDE" -yi 2 test.minix
ldetest 0 XIAFS_INODE2 "$LDE" -yi 2 test.xiafs

ldetest 0 EXT2_BLOCK55 "$LDE" -b 55 test.ext2
ldetest 0 MINIX_BLOCK15 "$LDE" -b 15 test.minix
ldetest 0 XIAFS_BLOCK55 "$LDE" -b 55 test.xiafs

ldetest 0 EXT2_BLOCK55_FORCE_EXT2 "$LDE" -b 55 -t ext2 test.ext2
ldetest F EXT2_BLOCK55_FORCE_MSDOS "$LDE" -b 55 -t msdos test.ext2

ldetest 0 EXT2_SUPERSCAN "$LDE" -P test.ext2
ldetest 0 XIAFS_SUPERSCAN "$LDE" -P test.xiafs
ldetest 0 MINIX_SUPERSCAN "$LDE" -P test.minix

ldetest 0 EXT2_ILOOKUP "$LDE" -kRS BBBBBBBBB test.ext2
ldetest 0 XIAFS_ILOOKUP "$LDE" -kRS BBBBBBBBB test.xiafs
ldetest 0 MINIX_ILOOKUP "$LDE" -kRS BBBBBBBBB test.minix

ldetest 0 EXT2_ILOOKUPALL "$LDE" -kaS BBBBBBBBB test.ext2
ldetest 0 XIAFS_ILOOKUPALL "$LDE" -kaS Basic test.xiafs
ldetest 0 MINIX_ILOOKUPALL "$LDE" -kaS ,, -O 18 test.minix

ldetest 0 MINIX_RECOVER "$LDE" -i 0xC -f results/MINIX_RECOVER test.minix
ldetest 0 XIAFS_RECOVER "$LDE" -i 0x1B -f results/XIAFS_RECOVER test.xiafs

ldetest 0 EXT2_INDIRECTS "$LDE" -j test.ext2
ldetest 0 XIAFS_INDIRECTS "$LDE" -j test.xiafs
ldetest 0 MINIX_INDIRECTS "$LDE" -j test.minix

echo ${SUCCESS} of ${TESTS} tests completed successfully

if [ x$SUCCESS != x$TESTS ] ; then
  exit 1
fi

exit 0