#!/bin/bash
# Force timezone to US pacific so dates will match
export LDE_TEST_TZ=America/Los_Angeles
. ${0%/*}/test.sh
