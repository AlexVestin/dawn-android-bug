## Dawn on Android with Vulkan backend
Basically just a quick of Dawn/Vulkan on android.

Download NDK + CMake inside AndroidStudio
https://developer.android.com/ndk/guides/graphics/getting-started
https://developer.android.com/studio/projects/install-ndk

Download `depot_tools` and add to path

# Clone the repo as "dawn"
git clone https://dawn.googlesource.com/dawn dawn && cd dawn

# Bootstrap the gclient configuration
cp scripts/standalone.gclient .gclient

# Fetch external dependencies and toolchains with gclient
gclient sync

Create new empty AndriodStudio project
Reference files inside android_studio_files

* In build.gradle 
Add to `android`
```
ndkVersion "25.2.9519653"
...
externalNativeBuild {
    cmake {
        path file('{your_path}/CMakeLists.txt')
    }
}
```

add to `defaultConfig`
```
ndk {
    abiFilters 'arm64-v8a'
}
externalNativeBuild {
    cmake {
        arguments '-DANDROID_PLATFORM=android-31', '-DANDROID_TOOLCHAIN=clang'
        cppFlags '-std=c++17'
    }
}
```
add to `dependencies`
`implementation "androidx.startup:startup-runtime:1.1.1"`

Change `applicationId` to match the AndroidManifest.xml id


* In build.gradle, change if needed:
  - file path to CMakeLists.txt
  - ndkVersion
  - applicationId
- In AndroidManifest.xml change the name of the package
