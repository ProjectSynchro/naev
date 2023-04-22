#!/bin/bash

# AppImage BUILD SCRIPT FOR NAEV
#
# For more information, see http://appimage.org/
# Pass in [-d] (set this for debug builds) [-n] (set this for nightly builds) -s <SOURCEPATH> (Sets location of source) -b <BUILDPATH> (Sets location of build directory)

# Output destination is ${WORKPATH}/dist

set -e

# Defaults
SOURCEPATH="$(pwd)"
BUILDTYPE="release"
APPDIRONLY="false"

while getopts dans:b: OPTION "$@"; do
    case $OPTION in
    d)
        set -x
        BUILDTYPE="debug"
        ;;
    a)
        APPDIRONLY="true"
        ;;
    n)
        BUILDTYPE="debugoptimized"
        ;;
    s)
        SOURCEPATH="${OPTARG}"
        ;;
    b)
        BUILDPATH="${OPTARG}"
        ;;
    *)
        ;;
    esac
done

# Creates temp dir if needed
if [ -z "$BUILDPATH" ]; then
    BUILDPATH="$(mktemp -d)"
    WORKPATH=$(readlink -mf "$BUILDPATH")
else
    WORKPATH=$(readlink -mf "$BUILDPATH")
fi

BUILDPATH="$WORKPATH/builddir"

# Honours the MESON variable set by the environment before setting it manually

if [ -z "$MESON" ]; then
    MESON="$SOURCEPATH/meson.sh"
fi

# Output configured variables

echo "SCRIPT WORKING PATH: $WORKPATH"
echo "SOURCE PATH:         $SOURCEPATH"
echo "BUILD PATH:          $BUILDPATH"
echo "BUILDTYPE:           $BUILDTYPE"
echo "MESON WRAPPER PATH:  $MESON"

# Make temp directories
mkdir -p "$WORKPATH"/{dist,utils,AppDir}

DESTDIR="$WORKPATH/AppDir"
export DESTDIR

# Get arch for use with linuxdeploy and to help make the linuxdeploy URL more architecture agnostic.
ARCH=$(arch)

export ARCH

# Get linuxdeploy's AppImage
linuxdeploy="$WORKPATH/utils/linuxdeploy.AppImage"
if [ ! -f "$linuxdeploy" ]; then
    curl -L -o "$linuxdeploy" "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage"
    #
    # This fiddles with some magic bytes in the ELF header. Don't ask me what this means.
    # For the layman: makes appimages run in docker containers properly again.
    # https://github.com/AppImage/AppImageKit/issues/828
    #
    sed '0,/AI\x02/{s|AI\x02|\x00\x00\x00|}' -i "$linuxdeploy"
    chmod +x "$linuxdeploy"
fi

# Run build
# Setup AppImage Build Directory
sh "$MESON" setup "$BUILDPATH" "$SOURCEPATH" \
--native-file "$SOURCEPATH/utils/build/linux.ini" \
--buildtype "$BUILDTYPE" \
--force-fallback-for=glpk,SuiteSparse \
-Dprefix="/usr" \
-Db_lto=true \
-Dauto_features=enabled \
-Ddocs_c=disabled \
-Ddocs_lua=disabled

# Compile and Install Naev to DISTDIR
sh "$MESON" install -C "$BUILDPATH"

# Prep dist directory for appimage

# Set VERSION and OUTPUT variables
if [ -f "$SOURCEPATH/dat/VERSION" ]; then
    VERSION="$(<"$SOURCEPATH/dat/VERSION")"
    export VERSION
else
    echo "The VERSION file is missing from $SOURCEPATH."
    exit 1
fi

if [[ "$NIGHTLY" =~ "true" ]]; then
    TAG="nightly"
else
    TAG="latest"
fi

SUFFIX="$VERSION-linux"

if [[ "$ARCH" =~ "x86_64" ]]; then
    SUFFIX="$SUFFIX-x86-64"
elif [[ "$ARCH" =~ "x86" ]]; then
    SUFFIX="$SUFFIX-x86"
else
    SUFFIX="$SUFFIX-unknown"
fi

export OUTPUT="$WORKPATH/dist/naev-$SUFFIX.AppImage"

# Rename metainfo file
mv "$WORKPATH/AppDir/usr/share/metainfo/org.naev.Naev.metainfo.xml" "$WORKPATH/AppDir/usr/share/metainfo/org.naev.Naev.appdata.xml"

# Disable appstream test
export NO_APPSTREAM=1

export UPDATE_INFORMATION="gh-releases-zsync|naev|naev|$TAG|naev-*.AppImage.zsync"

# Run linuxdeploy and generate an AppDir, then generate an AppImage

pushd "$WORKPATH"

if [[ "$APPDIRONLY" =~ "true" ]]; then
    "$linuxdeploy" \
        --appdir "$DESTDIR"
else
    "$linuxdeploy" \
        --appdir "$DESTDIR" \
        --output appimage
    # Move zsync file to dist directory
    mv ./*.zsync "$WORKPATH"/dist
    # Mark AppImage as executable
    chmod +x "$OUTPUT"
fi
popd

echo "Completed."
