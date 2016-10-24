#!/bin/sh
#
# VLMCSD - CentOS/Debian/RedHat/Ubuntu installer
#
# Author: Wind4 (puxiaping@gmail.com)
# Date:   November 30, 2015
#

# Detect OS
if [ -f /etc/debian_version ]; then
  ostype='debian'
elif [ -f /etc/lsb-release ]; then
  ostype='ubuntu'
elif [ -f /etc/redhat-release ]; then
  ostype='rhel'
else
  echo 'This script only support CentOS/Debian/RedHat/Ubuntu.' >&2
  exit 1
fi

# Checking wget
if [ ! -f /usr/bin/wget ]; then
  case $ostype in
    debian|ubuntu)
      apt-get update -yqq
      apt-get install -yqq wget
      ;;
    rhel)
      yum install -y -q wget
      ;;
  esac
  if [ "$?" -ne '0' ]; then
    echo "Error: Can't install wget" >&2
    exit 1
  fi
fi

# Select script type
case $ostype in
  debian|ubuntu) type='debian' ;;
  rhel)          type='rhel' ;;
esac

wget https://wind4.github.io/vlmcsd/scripts/install-$type.sh -O vlmcsd-$type.sh
if [ "$?" -eq '0' ]; then
    bash vlmcsd-$type.sh $*
    exit
else
    echo 'Download installer failed.' >&2
    exit 1
fi