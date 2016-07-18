[ -z $BASH ] && { exec bash "$0" "$@" || exit; }
#!/bin/bash
# file: installWittyPi.sh
#
# This script will install required software for Witty Pi.
# It is recommended to run it in your home directory.
#

# check if sudo is used
if [ "$(id -u)" != 0 ]; then
  echo 'Sorry, you need to run this script with sudo'
  exit 1
fi

# target directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/wittyPi"

# error counter
ERR=0

echo '================================================================================'
echo '|                                                                              |'
echo '|                   Witty Pi 2 Software Installing Script                      |'
echo '|                                                                              |'
echo '================================================================================'

# enable I2C on Raspberry Pi
echo '>>> Enable I2C'
if grep -q 'i2c-bcm2708' /etc/modules; then
  echo 'Seems i2c-bcm2708 module already exists, skip this step.'
else
  echo 'i2c-bcm2708' >> /etc/modules
fi
if grep -q 'i2c-dev' /etc/modules; then
  echo 'Seems i2c-dev module already exists, skip this step.'
else
  echo 'i2c-dev' >> /etc/modules
fi
if grep -q 'dtparam=i2c1=on' /boot/config.txt; then
  echo 'Seems i2c1 parameter already set, skip this step.'
else
  echo 'dtparam=i2c1=on' >> /boot/config.txt
fi
if grep -q 'dtparam=i2c_arm=on' /boot/config.txt; then
  echo 'Seems i2c_arm parameter already set, skip this step.'
else
  echo 'dtparam=i2c_arm=on' >> /boot/config.txt
fi
if grep -q 'dtoverlay=pi3-miniuart-bt' /boot/config.txt; then
  echo 'Seems setting Pi3 Bluetooth to use mini-UART is done already, skip this step.'
else
  echo 'dtoverlay=pi3-miniuart-bt' >> /boot/config.txt
fi
if grep -q 'core_freq=250' /boot/config.txt; then
  echo 'Seems the frequency of GPU processor core is set to 250MHz already, skip this step.'
else
  echo 'core_freq=250' >> /boot/config.txt
fi
if [ -f /etc/modprobe.d/raspi-blacklist.conf ]; then
  sed -i 's/^blacklist spi-bcm2708/#blacklist spi-bcm2708/' /etc/modprobe.d/raspi-blacklist.conf
  sed -i 's/^blacklist i2c-bcm2708/#blacklist i2c-bcm2708/' /etc/modprobe.d/raspi-blacklist.conf
else
  echo 'File raspi-blacklist.conf does not exist, skip this step.'
fi

# install i2c-tools
echo '>>> Install i2c-tools'
if hash i2cget 2>/dev/null; then
  echo 'Seems i2c-tools is installed already, skip this step.'
else
  apt-get install -y i2c-tools || ((ERR++))
fi

# check if it is Jessie
osInfo=$(cat /etc/os-release)
if [[ $osInfo == *"jessie"* ]] ; then
  isJessie=true
else
  isJessie=false
fi

# install wiringPi
if [ $ERR -eq 0 ]; then
  echo '>>> Install wiringPi'
  if hash gpio 2>/dev/null; then
    echo 'Seems wiringPi is installed already, skip this step.'
  else
    if $isJessie ; then
      apt-get install -y wiringpi || ((ERR++))
    else
      if hash git 2>/dev/null; then
        echo "Git is ready to go..."
      else
        echo "Git is missing, install it now..."
        apt-get install -y git || ((ERR++))
      fi
      if [ $ERR -eq 0 ]; then
        git clone git://git.drogon.net/wiringPi || ((ERR++))
        cd wiringPi
        ./build
        cd ..
      fi
    fi
  fi
fi

# install wittyPi
if [ $ERR -eq 0 ]; then
  echo '>>> Install wittyPi'
  if [ -d "wittyPi" ]; then
    echo 'Seems wittyPi is installed already, skip this step.'
  else
    wget http://www.uugear.com/repo/WittyPi2/LATEST -O wittyPi.zip || ((ERR++))
    unzip wittyPi.zip -d wittyPi || ((ERR++))
    cd wittyPi
    chmod +x wittyPi.sh
    chmod +x daemon.sh
    chmod +x syncTime.sh
    chmod +x runScript.sh
    chmod +x extraTasks.sh
    sed -e "s#/home/pi/wittyPi#$DIR#g" init.sh >/etc/init.d/wittypi
    chmod +x /etc/init.d/wittypi
    update-rc.d wittypi defaults
    cd ..
    chown -R pi:pi wittyPi
    sleep 2
    rm wittyPi.zip
  fi
fi

# install Qt 5 (optional and for Jessie only)
if $isJessie ; then
  read -p "Do you want to install Qt 5 for GUI running? [y/n] " -n 1 -r
  echo
  if [[ $REPLY =~ ^[Yy]$ ]] ; then
    apt-get install -y qt5-default || ((ERR++))
  else
    echo 'Skip Qt 5 installation.'
  fi
else
  echo 'For non-Jessie OS, please install Qt 5 manually to run the GUI.'
fi

echo
if [ $ERR -eq 0 ]; then
  echo '>>> All done. Please reboot your Pi :-)'
else
  echo '>>> Something went wrong. Please check the messages above :-('
fi
