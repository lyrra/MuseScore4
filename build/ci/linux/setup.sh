#!/usr/bin/env bash

# For maximum AppImage compatibility, build on the oldest Linux distribution
# that still receives security updates from its manufacturer.

sudo apt search qt
sudo apt search libqt
sudo apt search fluidsynth

echo "Setup Linux build environment"

df -h .

# Go one-up from MuseScore root dir regardless of where script was run from:
cd "$(dirname "$(readlink -f "${0}")")/../../../.."

# Let's remove the file with environment variables to recreate it
ENV_FILE=./musescore_environment.sh
rm -f ${ENV_FILE}

echo "echo 'Setup MuseScore build environment'" >> ${ENV_FILE}

##########################################################################
# GET DEPENDENCIES
##########################################################################

# DISTRIBUTION PACKAGES

# These are installed by default on Travis CI, but not on Docker
apt_packages_basic=(
  # Alphabetical order please!
  file
  git
  pkg-config
  software-properties-common # installs `add-apt-repository`
  unzip
  p7zip-full
  )

# These are the same as on Travis CI
apt_packages_standard=(
  # Alphabetical order please!
  curl
  libasound2-dev 
  libfluidsynth-dev
  libfontconfig1-dev
  libfreetype6-dev
  libfreetype6
  libgl1-mesa-dev
  libjack-dev
  libmp3lame-dev
  libnss3-dev
  libportmidi-dev
  libpulse-dev
  libsndfile1-dev
  libzmq3-dev
  libqt5qml5
  libqt5qml5
  libqt5quick5
  libqt5quickcontrols2-5
  libqt5quicktemplates2-5
  libqt5quickwidgets5
  libqt5xml5
  libqt5xmlpatterns5-dev
  libqt5svg5-dev
  libqt5opengl5-dev
  libqt5help5
  qtquickcontrols2-5-dev
  qtbase5-dev
  qtdeclarative5-dev
  qttools5-dev
  portaudio19-dev
  guile-3.0
  guile-3.0-dev
  guile-3.0-libs
  guile-bytestructures
  guile-json
  guile-sqlite3
  make
  cmake
  gcc
  wget
  )

# MuseScore compiles without these but won't run without them
apt_packages_runtime=(
  # Alphabetical order please!
  libcups2
  libdbus-1-3
  libegl1-mesa-dev
  libodbc1
  libpq-dev
  libssl-dev
  libxcomposite-dev
  libxcursor-dev
  libxi-dev
  libxkbcommon-x11-0
  libxrandr2
  libxtst-dev
  libdrm-dev
  )

sudo apt-get update # no package lists in Docker image
sudo apt-get install -y --no-install-recommends \
  "${apt_packages_basic[@]}" \
  "${apt_packages_standard[@]}" \
  "${apt_packages_runtime[@]}"

echo "---------- whereis guile 3.0 ------------"
ls -ltr /usr/bin/guile || true
ls -ltr /usr/bin/guile-3.0 || true
command -v guile || true
echo "------------ dir /usr/lib/x86_64-linux-gnu/guile  ---------------------"
find /usr/lib/x86_64-linux-gnu/guile || true
echo "-------- %library-dir / SCM_LIBRARY_DIR ------------------------------"
guile -c '(begin (display (%library-dir)) (newline))'
echo "------------------------------------------------------"
echo "-------- GUILE_SYSTEM_COMPILED_PATH=$GUILE_SYSTEM_COMPILED_PATH ------------------------------"

#echo export PATH="${qt_path}/bin:\${PATH}" >> ${ENV_FILE}
#echo export LD_LIBRARY_PATH="${qt_path}/lib:\${LD_LIBRARY_PATH}" >> ${ENV_FILE}
#echo export QT_PATH="${qt_path}" >> ${ENV_FILE}
#echo export QT_PLUGIN_PATH="${qt_path}/plugins" >> ${ENV_FILE}
#echo export QML2_IMPORT_PATH="${qt_path}/qml" >> ${ENV_FILE}


##########################################################################
# GET TOOLS
##########################################################################

##########################################################################
# POST INSTALL
##########################################################################

chmod +x "${ENV_FILE}"

# # tidy up (reduce size of Docker image)
# apt-get clean autoclean
# apt-get autoremove --purge -y
# rm -rf /tmp/* /var/{cache,log,backups}/* /var/lib/apt/*

df -h .
echo "Setup script done"

