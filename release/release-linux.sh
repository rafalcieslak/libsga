#!/bin/sh

# This script shall be run in the build directory after everything has been
# compiled. It will prepare a release in ./release/ directory. Version must be
# passed as the argument to this script.

if [ $# -eq 0 ]; then
    echo "Missing VERSION argument."
    exit 1
fi

BASEDIR=release-linux
RELDIR=libsga-$1
SDKDIR=$BASEDIR/$RELDIR

rm -rf $SDKDIR
mkdir -p $SDKDIR
mkdir -p $SDKDIR/include

# Main library
cp libsga.so $SDKDIR

# Header files
find ../include/ -\( -name "*.hpp" -or -name "*.inc" -\) -exec install -D {} $SDKDIR/include/{} \;

# HTML docs
cp -r doc/html $SDKDIR/doc

# TODO: Example files

# Create a release archive
cd $BASEDIR && tar -czf $RELDIR.tar.gz $RELDIR
