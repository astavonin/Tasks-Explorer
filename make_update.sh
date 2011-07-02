#!/bin/sh

# make_update.sh
# Tasks Explorer
#
# Created by Alexander Stavonin on 18.01.10.
# Copyright 2010 www.stavonin.com. All rights reserved.


cd build/Release/
zip TasksExplorer.pkg.zip "Tasks Explorer.pkg"
cd ../..
ruby "keys/sign_update.rb" build/Release/TasksExplorer.pkg.zip keys/dsa_priv.pem
