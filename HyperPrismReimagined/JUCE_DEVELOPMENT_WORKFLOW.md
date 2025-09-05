# JUCE VST3 Development Workflow Guide for HyperPrism

## Summary of Clean Rebuild

Successfully rebuilt all 32 HyperPrism VST3 plugins with the following locations:
- VST3 Plugins: `./[PluginName]_artefacts/Release/VST3/`
- Standalone Apps: `./[PluginName]_artefacts/Release/Standalone/`

## Avoiding VST3 Rescans - Development Workflow Options

### 1. **Use Standalone Builds (Already Available!)**
Your CMakeLists.txt is already configured to build standalone versions of all plugins. These are located at:
```
./[PluginName]_artefacts/Release/Standalone/[PluginName].app
```

**Benefits:**
- No DAW required for testing
- Instant launch after rebuild
- Full debugging capabilities
- No plugin scanning needed

**Usage:**
```bash
# Run a standalone plugin directly
open "./HyperPrismReverb_artefacts/Release/Standalone/HyperPrism Revived Reverb.app"
```

### 2. **JUCE AudioPluginHost**
JUCE includes a dedicated plugin testing host at:
```
../JUCE/extras/AudioPluginHost/
```

**Setup:**
1. Build the AudioPluginHost:
   ```bash
   cd ../JUCE/extras/AudioPluginHost/Builds/MacOSX
   xcodebuild -project AudioPluginHost.xcodeproj -configuration Release
   ```

2. Configure your IDE to launch AudioPluginHost when debugging
3. Save your plugin setup as a `.filtergraph` file for quick reloading

**Benefits:**
- Lightweight host designed for plugin development
- Supports saving/loading plugin configurations
- Quick plugin reloading without full rescan
- Direct debugging support

### 3. **Development Folder with Symlinks**
Instead of copying plugins to system folders, use symlinks:

```bash
# Create a development VST3 folder
mkdir -p ~/VST3-Dev

# Create symlinks to your built plugins
ln -s "/Users/zacharylquarles/Library/CloudStorage/OneDrive-Personal/PROJECTS_Apps_Plugins/ProjectHyperprism/HyperPrismRevived/build/HyperPrismReverb_artefacts/Release/VST3/HyperPrism Revived Reverb.vst3" ~/VST3-Dev/

# Configure your DAW to scan only ~/VST3-Dev
```

### 4. **Direct Plugin Testing Script**
Create a script to quickly test plugins:

```bash
#!/bin/bash
# test-plugin.sh
PLUGIN_NAME="$1"
PLUGIN_PATH="./HyperPrism${PLUGIN_NAME}_artefacts/Release/Standalone/HyperPrism Revived ${PLUGIN_NAME}.app"

if [ -d "$PLUGIN_PATH" ]; then
    open "$PLUGIN_PATH"
else
    echo "Plugin not found: $PLUGIN_PATH"
fi
```

### 5. **IDE Configuration for Direct Testing**

**Xcode:**
1. Product > Scheme > Edit Scheme
2. Under Run, select the standalone app as executable
3. Enable "Debug executable"

**CMake/Make:**
Add custom targets for running standalone apps:
```cmake
add_custom_target(run-reverb
    COMMAND open "${CMAKE_BINARY_DIR}/HyperPrismReverb_artefacts/Release/Standalone/HyperPrism Revived Reverb.app"
    DEPENDS HyperPrismReverb
)
```

## Recommended Development Workflow

1. **Primary Development**: Use standalone builds for UI and basic functionality testing
2. **Plugin-specific Testing**: Use JUCE AudioPluginHost for plugin-specific features
3. **DAW Testing**: Only when necessary, use symlinked development folder
4. **Final Validation**: Test in actual DAW environment before release

## Quick Commands

```bash
# Clean rebuild all plugins
cd build
rm -rf *_artefacts CMakeFiles CMakeCache.txt cmake_install.cmake
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)

# Test a specific plugin standalone
open "./HyperPrismReverb_artefacts/Release/Standalone/HyperPrism Revived Reverb.app"

# List all built VST3 plugins
find . -name "*.vst3" -type d | grep -E "Release/VST3"

# List all standalone apps
find . -name "*.app" -type d | grep -i hyperprism
```

## Troubleshooting

### Plugin Not Loading
- Check if VST3 was built: `ls -la *_artefacts/Release/VST3/`
- Verify plugin symbols: `nm "plugin.vst3/Contents/MacOS/plugin" | grep moduleEntry`

### Debugging Crashes
- Use standalone build for easier debugging
- Enable debug symbols in CMake: `-DCMAKE_BUILD_TYPE=Debug`
- Use AudioPluginHost for controlled environment testing

### Version Number Tip
Set version to 0.0.0 for debug builds to force automatic rescanning in DAWs that support it.