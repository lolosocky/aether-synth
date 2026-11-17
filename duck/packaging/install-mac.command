#!/bin/bash
cd "$(dirname "$0")"
mkdir -p ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components
rm -rf ~/Library/Audio/Plug-Ins/VST3/"LGN Duck.vst3" ~/Library/Audio/Plug-Ins/Components/"LGN Duck.component"
cp -R "LGN Duck.vst3" ~/Library/Audio/Plug-Ins/VST3/
cp -R "LGN Duck.component" ~/Library/Audio/Plug-Ins/Components/
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"LGN Duck.vst3" ~/Library/Audio/Plug-Ins/Components/"LGN Duck.component" 2>/dev/null
killall -9 AudioComponentRegistrar 2>/dev/null
echo "LGN Duck installed. Restart your DAW and rescan plugins."
