## Project Northstar OpenVR Driver

this is a work in progress [more info](TODO.md)

### Working with some polish needed
- Head Tracking
- Hand Tracking
- View Projection
- Skeletal tracking
- Basic input
- T265 Sensor integration

### Notable unfinished parts
- Gesture recognizer

### Build Instructions

Versions of vendored libs not included and where to get them:

[Leap](https://developer.leapmotion.com/setup):

- LeapDeveloperKit 4.0.0+52238

[Structure Sensor](https://structure.io/):

- CrossPlatform SDK 0.7.1
- PerceptionEngine 0.7.1

## Build steps

```
git clone git@github.com:fuag15/project_northstar_openvr_driver.git
cd project_northstar_openvr_driver
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -A x64 ..
```

- Open the generated solution and set northstart to the startup project (right click the project and choose the set as startup where the gear icon is) and build. Make sure to target x64 and a Release build to remove any object creation slowness.

- The release will be in `build/Release/`

- Copy all these dll's to wherever you want to install from, they should be combined into the `resources/northstar/bin/win64` directory, make this if it does not exist and put all generated dll's inside.

- Next register the driver with steamVR (using vrpathreg tool found in SteamVR bin folder) for me this is located at `C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win64\vrpathreg.exe`

```
vrpathreg adddriver <full_path_to/resources/northstar>
```
- vrpathreg is a command line tool, you have to run it via the command prompt. To do this, follow these steps. 
        1) open command prompt
        2)run ` cd C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win64\`
        3)run `vrpathreg adddriver <full_path_to/resources/northstar>`
        4) you can verify the driver has been added by typing `vrpathreg` in command prompt, it will show you a list of drivers installed. 


- at this point vrpathreg should show it registered under "external drivers", to disable it you can either disable in StamVR developer options under startup and manage addons, or vrpathreg removedriver `full_path_to/resources/northstar`

- Running steamvr with the northstar connected (and no other hmd's connected) should work at this point but probably not have the right configuration for your hmd. Take the .json file from configuring your nothstar and convert it to the format found in `resources/northstar/resources/settings/default.vrsettings`

- restart steamvr. If driver is taking a long time to start, chances are it got built in debug mode, Release mode optimizes a way a bunch of intermediate object creation during the lens distortion solve that happens at startup. If things are still going south please maka a bug, I'll do my best to get to them asap.

- if you wish to remove controller emulation, disable the leap driver in SteamVR developer settings.
