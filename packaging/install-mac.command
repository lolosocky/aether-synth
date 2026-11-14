#!/bin/bash
# Installs Aether (VST3 + AU) for all users and clears the quarantine flag.
cd "$(dirname "$0")"
echo "Installing Aether. You may be asked for your Mac password."
sudo mkdir -p /Library/Audio/Plug-Ins/VST3 /Library/Audio/Plug-Ins/Components
sudo rm -rf /Library/Audio/Plug-Ins/VST3/Aether.vst3 /Library/Audio/Plug-Ins/Components/Aether.component
sudo cp -R Aether.vst3 /Library/Audio/Plug-Ins/VST3/
sudo cp -R Aether.component /Library/Audio/Plug-Ins/Components/
sudo xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/VST3/Aether.vst3 /Library/Audio/Plug-Ins/Components/Aether.component
killall -9 AudioComponentRegistrar 2>/dev/null
echo ""
echo "Done. In FL Studio: Options > Manage plugins > Find more plugins."
read -n 1 -s -r -p "Press any key to close."
