#!/bin/bash

# STEAM DEPLOYMENT SCRIPT FOR NAEV
# Requires SteamCMD to be installed within a Github Actions ubuntu-latest runner.
#
# Written by Jack Greiner (ProjectSynchro on Github: https://github.com/ProjectSynchro/) 
#
# This script should be run after downloading all build artefacts
# If -n is passed to the script, a nightly build will be generated
# and uploaded to Steam
#
# Pass in [-d] [-n] (set this for nightly builds) -s <SOURCEROOT> (Sets location of source) -t <TEMPPATH> (Steam build artefact location) -o <STEAMPATH> (Steam dist output directory)

set -e

# Defaults
NIGHTLY="false"
BETA="false"

while getopts dns:t:o: OPTION "$@"; do
    case $OPTION in
    d)
        set -x
        ;;
    n)
        NIGHTLY="true"
        ;;
    s)
        SOURCEROOT="${OPTARG}"
        ;;
    t)
        TEMPPATH="${OPTARG}"
        ;;
    o)
        STEAMPATH="${OPTARG}"
        ;;
    esac
done

if [ -f "$SOURCEROOT/dat/VERSION" ]; then
    export VERSION="$(cat "$SOURCEROOT/dat/VERSION")"
else
    echo "The VERSION file is missing from $SOURCEROOT."
    exit 1
fi

# Get version, negative minors mean betas
if [ -n $(echo "$VERSION" | grep "beta") ]; then
    BETA="true"
else
    echo "could not find VERSION file"
    exit 1
fi

# Make Steam dist path if it does not exist
mkdir -p "$STEAMPATH"/content/lin64
mkdir -p "$STEAMPATH"/content/win64
mkdir -p "$STEAMPATH"/content/ndata

# Move Depot Scripts to Steam staging area
cp -r "$SOURCEROOT"/utils/ci/steam/scripts "$STEAMPATH"

# Move all build artefacts to deployment locations
# Move Linux binary and set as executable
cp "$TEMPPATH"/naev-steamruntime/naev.x64 "$STEAMPATH"/content/lin64
chmod +x "$STEAMPATH"/content/lin64/naev.x64
          
# Move macOS bundle to deployment location (Bye Bye for now)
# unzip "$TEMPPATH/naev-macOS/naev-macos.zip" -d "$STEAMPATH/content/macos/"

# Unzip Windows binary and DLLs and move to deployment location
tar -Jxvf "$TEMPPATH/naev-win64/steam-win64.tar.xz" -C "$STEAMPATH/content/win64"
mv "$STEAMPATH"/content/win64/naev*.exe "$STEAMPATH/content/win64/naev.exe"

# Move data to deployment location
cp -r "$SOURCEROOT/dat" "$STEAMPATH/content/ndata"

# Runs STEAMCMD, and builds the app as well as all needed depots.

# Trigger 2FA request and get 2FA code
steamcmd +login $STEAMCMD_USER $STEAMCMD_PASS +quit || true

# Wait a bit for the email to arrive
sleep 60s
python3 "$SOURCEROOT/utils/ci/steam/2fa/get_2fa.py"
STEAMCMD_TFA="$(cat "$SOURCEROOT/extras/steam/2fa/2fa.txt")"

if [ "$NIGHTLY" == "true" ]; then
    # Run steam upload with 2fa key
    steamcmd +login $STEAMCMD_USER $STEAMCMD_PASS $STEAMCMD_TFA +run_app_build_http "$STEAMPATH/scripts/app_build_598530_nightly.vdf" +quit
else
    if [ "$BETA" == "true" ]; then 
        # Run steam upload with 2fa key
        steamcmd +login $STEAMCMD_USER $STEAMCMD_PASS $STEAMCMD_TFA +run_app_build_http "$STEAMPATH/scripts/app_build_598530_prerelease.vdf" +quit

    elif [ "$BETA" == "false" ]; then 
        # Move soundtrack stuff to deployment area
        cp naev-soundtrack/soundtrack/*.mp3 "$STEAMPATH/content/soundtrack"
        cp naev-soundtrack/soundtrack/*.png "$STEAMPATH/content/soundtrack"

        # Run steam upload with 2fa key
        steamcmd +login $STEAMCMD_USER $STEAMCMD_PASS $STEAMCMD_TFA +run_app_build_http "$STEAMPATH/scripts/app_build_598530_release.vdf" +quit
        steamcmd +login $STEAMCMD_USER $STEAMCMD_PASS +run_app_build_http "$STEAMPATH/scripts/app_build_1411430_soundtrack.vdf" +quit

    else
        echo "Something went wrong determining if this is a beta or not."
    fi
fi
