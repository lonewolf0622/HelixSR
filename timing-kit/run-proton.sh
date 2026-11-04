#!/bin/bash
# run-proton.sh KIT_FOLDER [kit args] : the kit under Proton (PROTON_PREFIX: its prefix); its console output in KIT_FOLDER/console.txt
K=$1; shift; P=${PROTON_PREFIX:?set PROTON_PREFIX to a Proton prefix folder}
cd "$K" && env STEAM_COMPAT_CLIENT_INSTALL_PATH=$HOME/.local/share/Steam STEAM_COMPAT_DATA_PATH=$P SteamAppId=0 SteamGameId=0 WINEDEBUG=-all \
  timeout ${TMO:-3600} "$HOME/.local/share/Steam/steamapps/common/Proton - Experimental/proton" run "$K/helixsr-timing.exe" "$@" > "$K/console.txt" 2>&1
echo "exit $?"
