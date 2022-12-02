#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# SPXD-Author: Ruby Allison Rose (aka m3tior)

### Linter Directives ###
# shellcheck shell=sh

################################################################################
## Globals

SELF=$(readlink -nf "$0");
PROCDIR="$(dirname "$SELF")";
APPNAME="$(basename "$SELF")";

################################################################################
## Functions

################################################################################
## Main Script

if ! command -v pkexec 1>&-; then
	echo "Error: missing required binary \`pkexec\`" >&2;
	exit 1;
fi;
if ! command -v curl 1>&-; then
	echo "Error: missing required binary \`curl\`" >&2;
	exit 1;
fi;
if ! command -v chezmoi 1>&-; then
	mkdir -p "$HOME/.local/bin"
	if
		! INSTALLER="$(curl -fsLS get.chezmoi.io)" ||
		! sh -c "$INSTALLER" -- -b "$HOME/.local/bin"; then
		echo "Error: Failed to acquire chezmoi from database." >&2;
		exit 1;
	fi;
fi;


