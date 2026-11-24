#!/bin/bash
# Installs the LGN mixing suite (VST3 + AU) for all users and clears the quarantine flag.
cd "$(dirname "$0")"
echo "Installing the LGN mixing suite. You may be asked for your Mac password."
sudo mkdir -p /Library/Audio/Plug-Ins/VST3 /Library/Audio/Plug-Ins/Components
for b in *.vst3; do sudo rm -rf "/Library/Audio/Plug-Ins/VST3/$b"; sudo cp -R "$b" /Library/Audio/Plug-Ins/VST3/; sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/$b"; done
for b in *.component; do sudo rm -rf "/Library/Audio/Plug-Ins/Components/$b"; sudo cp -R "$b" /Library/Audio/Plug-Ins/Components/; sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/Components/$b"; done
killall -9 AudioComponentRegistrar 2>/dev/null
echo ""
echo "Done. In FL Studio: Options > Manage plugins > Find more plugins. They appear under Effects > LGN."
read -n 1 -s -r -p "Press any key to close."
