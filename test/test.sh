#!/bin/bash

##################################################################
# Automated test scripts for lde
#
# (C) 2002, 2021 Scott Heavner, GPL
#
##################################################################

cd "${0%/*}"

# Configuration ----------------------
if [ -f ../lde -a -x ../lde ] ; then
  LDE=../lde
elif [ -f ../lde/lde -a -x ../lde/lde ] ; then
  LDE=../lde/lde
else
  echo "Can't find lde executable, aborting test."
  exit 1
fi
#LDE='/cygdrive/c/Users/xbox/CMakeBuilds/d952c480-d245-a03c-9cdc-2540725d7f7e/build/x64-Debug (default)/lde.exe'
DIFF="diff -b"
RM="rm -f"
VERBOSE=1
ECHO_CMDS=0
RETAIN=0
STOPONERROR=0
# End Configuration ------------------

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

  # $1 = test label
  # $2+ = command

  if [ x$ONETEST != x -a x$ONETEST != x$1 ] ; then
    return
  fi

  success=0

  num=$1
  shift

  if [ x$VERBOSE = x1 ] ; then
	echo -n "Test: $num ... "
  fi

  TMPFILE=results/${num}.$$

  if ! "$@" > $TMPFILE 2>&1 ; then
	success='Execution failed.'
  elif ! $DIFF $TMPFILE expected/${num} > results/diff1.$$ 2>&1 ; then
	success='Unexpected output.'
  elif [ -f results/${num} ] ; then
	if ! $DIFF results/${num} expected/${num}_RESULTS > results/diff2.$$ 2>&1 ; then
		success='Unexpected results.'
	fi
  fi

  if [ x$success != x0 ] ; then
	echo "*** $success ***************"
	if [ x$STOPONERROR = x1 ] ; then	
		exit
	fi
  else
	if [ x$VERBOSE = x1 ] ; then
		echo ok
	fi
  	let SUCCESS=$SUCCESS+1
  fi
  if [ x$RETAIN != x1 ] ; then
    $RM $TMPFILE results/diff1.$$ results/diff2.$$ results/${num}
  fi

  let TESTS=$TESTS+1
}

$RM results/* 2> /dev/null

if ! "$LDE" -v > /dev/null ; then
  echo "Cannot run $LDE"
  exit 1
fi

# These fail on cygwin if test.ext2 comes before -O
ldetest SEARCH_EXT2_MAGIC "$LDE" -a -t no -T search/ext2mag -O 56 -L 2 test.ext2
ldetest SEARCH_MINIX_MAGIC "$LDE" -a -t no -T search/minix-mag -O 16 -L 2 test.minix
ldetest SEARCH_XIAFS_MAGIC "$LDE" -s 512 -a -t no -T search/xiafs-mag -O 60 -L 2 test.xiafs

# Need to supress symbolic uid/gid will vary system to system
ldetest EXT2_INODE2 "$LDE" -yi 2 test.ext2
ldetest MINIX_INODE2 "$LDE" -yi 2 test.minix
ldetest XIAFS_INODE2 "$LDE" -yi 2 test.xiafs

ldetest EXT2_BLOCK55 "$LDE" -b 55 test.ext2
ldetest MINIX_BLOCK15 "$LDE" -b 15 test.minix
ldetest XIAFS_BLOCK55 "$LDE" -b 55 test.xiafs

ldetest EXT2_BLOCK55_FORCE_EXT2 "$LDE" -b 55 -t ext2 test.ext2
ldetest EXT2_BLOCK55_FORCE_MSDOS "$LDE" -b 55 -t msdos test.ext2

ldetest EXT2_SUPERSCAN "$LDE" -P test.ext2
ldetest XIAFS_SUPERSCAN "$LDE" -P test.xiafs
ldetest MINIX_SUPERSCAN "$LDE" -P test.minix

ldetest EXT2_ILOOKUP "$LDE" -kRS BBBBBBBBB test.ext2
ldetest XIAFS_ILOOKUP "$LDE" -kRS BBBBBBBBB test.xiafs
ldetest MINIX_ILOOKUP "$LDE" -kRS BBBBBBBBB test.minix

ldetest EXT2_ILOOKUPALL "$LDE" -kaS BBBBBBBBB test.ext2
ldetest XIAFS_ILOOKUPALL "$LDE" -kaS Basic test.xiafs
ldetest MINIX_ILOOKUPALL "$LDE" -kaS ,, -O 18 test.minix

ldetest MINIX_RECOVER "$LDE" -i 0xC -f results/MINIX_RECOVER test.minix
ldetest XIAFS_RECOVER "$LDE" -i 0x1B -f results/XIAFS_RECOVER test.xiafs

ldetest EXT2_INDIRECTS "$LDE" -j test.ext2
ldetest XIAFS_INDIRECTS "$LDE" -j test.xiafs
ldetest MINIX_INDIRECTS "$LDE" -j test.minix

echo ${SUCCESS} of ${TESTS} tests completed successfully

if [ x$SUCCESS != x$TESTS ] ; then
	exit $SUCCESS
fi

exit 0

