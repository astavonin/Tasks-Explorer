#!/bin/sh

WATCH_DIR="/Library/Application Support/TasksExplorer/watch_dir"
OLD_PLIST="/Library/LaunchDaemons/com.stavonin.tasksexplorerd.plist"
NEW_PLIST="/Library/LaunchDaemons/com.macosinternals.tasksexplorerd.plist"

if [[ ! -d "$WATCH_DIR" ]]; then
	sudo mkdir "$WATCH_DIR"
	sudo chmod uga+wr "$WATCH_DIR"
fi

if [[ -e $OLD_PLIST ]]; then
	sudo launchctl unload "$OLD_PLIST"
	sudo rm -f "$OLD_PLIST"
fi

if [[ -e "$NEW_PLIST" ]]; then
	sudo launchctl unload "$NEW_PLIST"
	sudo launchctl load "$NEW_PLIST"
fi

