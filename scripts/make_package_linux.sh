#! /bin/bash
#
# Create redistributable AppImage package.
#

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd ${SCRIPT_DIR}/..

# Create out dir if none
if [ ! -d out ]; then mkdir out; fi
cd out

# Download linuxdeplyqt if none
# https://github.com/probonopd/linuxdeployqt
LINUXDEPLOYQT=linuxdeployqt-continuous-x86_64.AppImage
if [ ! -f ${LINUXDEPLOYQT} ]; then
  echo
  echo "Downloading ${LINUXDEPLOYQT}..."
  wget -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/${LINUXDEPLOYQT}
  chmod a+x ${LINUXDEPLOYQT}
  if [ "${?}" != "0" ]; then exit 1; fi
fi

# Create AppDir structure
echo
echo "Creating AppDir structure..."
if [ -d AppDir ]; then rm -rdf AppDir; fi
mkdir -p AppDir/usr/bin
mkdir -p AppDir/usr/lib
mkdir -p AppDir/usr/share/applications
mkdir -p AppDir/usr/share/icons/hicolor/256x256/apps
cp ../bin/procyon AppDir/usr/bin
cp ../release/procyon.desktop AppDir/usr/share/applications
cp ../img/icon/main_256.png AppDir/usr/share/icons/hicolor/256x256/apps/procyon.png
if [ "${?}" != "0" ]; then exit 1; fi

# Run linuxdeplyqt on the AppDir
echo
echo "Creating AppImage..."
# qmake must be in PATH, we can try to extract its path from RPATH of app binary
RPATH="$(objdump -x ../bin/procyon | grep RPATH | sed -e 's/^\s*RPATH\s*//')"
PATH=${RPATH}/../bin:${PATH}
./${LINUXDEPLOYQT} AppDir/usr/share/applications/procyon.desktop -appimage -no-translations
if [ "${?}" != "0" ]; then exit 1; fi

# Rename resulting file to contain version
if [ -f ../release/version.txt ]; then
  VERSION="$(cat ../release/version.txt)"
  cp procyon-x86_64.AppImage procyon-${VERSION}-x86_64.AppImage
else
  echo
  echo "Warning: Unknown release version."
  echo "Run release/make_version.py script to generate version number."
fi

