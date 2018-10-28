#!/bin/bash
#
# Install a VLMCSD service for CentOS/RedHat
#
# Author: Wind4 (puxiaping@gmail.com)
# Date:   October 28, 2018
#

echo 'Remove the /etc/init.d/vlmcsd'
rm -f /etc/init.d/vlmcsd

echo 'Remove the /usr/bin/vlmcsd'
rm -f /usr/bin/vlmcsd

echo 'Uninstall successfully.'