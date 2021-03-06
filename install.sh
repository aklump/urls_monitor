#!/bin/bash
#
# @file
#
# Install a new monitor subfolder for a client
#
# Usage ./install.sh
#
#   Installs a new R/W copy of git@github.com:aklump/urls_monitor.git
#   Adds a symlink to the jquery.tablesorter.min.js
#   Creates the two necessary config files
#
# CREDITS:
# In the Loft Studios
# Aaron Klump - Web Developer
# PO Box 29294 Bellingham, WA 98228-1294
# aim: theloft101
# skype: intheloftstudios
#
#
# LICENSE:
# Copyright (c) 2013, In the Loft Studios, LLC. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   1. Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
#   2. Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
#   3. Neither the name of In the Loft Studios, LLC, nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY IN THE LOFT STUDIOS, LLC "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL IN THE LOFT STUDIOS, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# The views and conclusions contained in the software and documentation are
# those of the authors and should not be interpreted as representing official
# policies, either expressed or implied, of In the Loft Studios, LLC.
#
#
# @ingroup urls_monitor
# @{
#
start_dir=$(PWD);

# Install packages using composer
composer=$(which composer)
if [ "$composer" ] && [ ! -f composer.lock ]; then
  composer install
fi

# Gleen the page name
if [ $# -ne 1 ]
then
  echo "`tput setaf 1`Please provide a project name as the argument.`tput op`"
  exit
else
  page=$1
fi

if [[ -d "config/pages/$page" ]]; then
  echo "`tput setaf 1`The project '$page' already exists.`tput op`"
  exit
fi

# Make sure the directory exists
mkdir -p "config/pages/$page"

# Descend into page directory
cd "config/pages/$page"

# Configuration files
if [ ! -f config.ini ]
then
  cp "$start_dir/install/config.default.ini" config.ini
fi
if [ ! -f urls.txt ]
then
  cp "$start_dir/install/urls.default.txt" urls.txt
fi
if [ ! -f meta.yaml ]
then
  cp "$start_dir/install/meta.default.yaml" meta.yaml
fi

cd $start_dir

# Download tablesorter
if [ ! -f jquery.tablesorter.min.js ]
then
  mkdir temp
  cd temp
  curl -O http://tablesorter.com/__jquery.tablesorter.zip
  unzip __jquery.tablesorter.zip
  mv jquery.tablesorter.min.js ../
  cd ..
  rm -rf temp
fi

# Check for success
if [ -f jquery.tablesorter.min.js ] && [ -f config/pages/$page/config.ini ] && [ -f config/pages/$page/urls.txt ]
then
  echo "`tput setaf 2`Installation complete.`tput op`"
  echo "`tput setaf 2`Please edit config/pages/$page/urls.txt now.`tput op`"
else
  echo "`tput setaf 1`Installation failed.`tput op`"
fi
