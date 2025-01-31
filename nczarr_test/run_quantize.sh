#!/bin/sh

if test "x$srcdir" = x ; then srcdir=`pwd`; fi 
. ../test_common.sh

. "$srcdir/test_nczarr.sh"

# Construct both ISOPATH and S3ISOPATH
s3isolate "testdir_quantize"

THISDIR=`pwd`
cd $ISOPATH

# This shell script runs test_quantize

set -e

testcase() {
  zext=$1
  fileargs tmp_quantize "mode=$zarr,$zext"
  case "$zext" in
  file) template="file://${execdir}/%s.zarr#mode=zarr,$zext" ;;
  zip)  template="file://${execdir}/%s.zip#mode=zarr,$zext" ;;
  s3)  template="s3://${NCZARR_S3_TEST_BUCKET}/${S3TESTPATH}/%s.zarr#mode=zarr,$zext" ;;
  *) echo "unknown file type"; exit 1 ;;
  esac
  ${execdir}/test_quantize "$template"
}

testcase file
if test "x$FEATURE_NCZARR_ZIP" = xyes ; then testcase zip; fi
# There is a (currently) untraceable bug when using S3
#if test "x$FEATURE_S3TESTS" = xyes ; then testcase s3; fi
# if test "x$FEATURE_S3TESTS" = xyes ; then s3sdkdelete "/${S3ISOPATH}" ; fi # Cleanup
