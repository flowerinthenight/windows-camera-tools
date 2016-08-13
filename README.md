# windows-camera-tools

A generic command line tool to control camera properties in Windows. The tool's name is `camera-cmd.exe`. You can also view the information below in command line by running `camera-cmd.exe` without arguments.

## Syntax

```
camera-cmd.exe option [parameters...]
```

## Options

##### fnames

Lists the available/attached camera(s) in the system. Returns the number of available camera(s) found if any, otherwise returns -1.

##### mediainfo

```
mediainfo -fname:<camera_friendly_name>
```

Lists media-related information of the provided camera name. Returns the number of information structures retrieved. `camera_friendly_name` is the friendly name of the camera device/driver. You can use the `fnames` option to get the available friendly names, or see Device Manager -> Imaging devices.

###### Example

```
camera-cmd.exe mediainfo -fname:Integrated Camera
```

##### proppage

proppage -fname:<camera_friendly_name>

Opens the driver-provided (if any) extended property page(s). `camera_friendly_name` is the friendly name of the camera device/driver. You can use the `fnames` option to get the available friendly names, or see Device Manager -> Imaging devices.

###### Example

```
camera-cmd.exe proppage -fname:Integrated Camera
```

##### issyscam

```
issyscam -fname:<camera_friendly_name>
```

Returns the index of the camera friendly name being queried. Returns -1 on failure, or when the camera is not found. `camera_friendly_name` is the friendly name of the camera device/driver. You can use the `fnames` option to get the available friendly names, or see Device Manager -> Imaging devices.

###### Example

```
camera-cmd.exe issyscam -fname:Integrated Camera
```

##### privacy

```
privacy -fname:<camera_friendly_name> [-state:<0|1>]
```

Controls the camera privacy properties (if supported). `camera_friendly_name` is the friendly name of the camera device/driver. You can use the `fnames` option to get the available friendly names, or see Device Manager -> Imaging devices. Valid `-state:` values are 0 (disabled), and 1 (enabled). If only `-fname:...` is provided, returns the current state. If `-state:...` is provided after `fname:...`, sets the privacy state to the value provided. Returns 0 on success, -1 on failure.

###### Examples

```
camera-cmd.exe privacy -fname:Integrated Camera
camera-cmd.exe privacy -fname:Integrated Camera -state:1
```

##### flash

```
flash -fname:<camera_friendly_name> [-state:<0|1|2>]
```

Controls the camera flash properties (if supported). `camera_friendly_name` is the friendly name of the camera device/driver. You can use the `fnames` option to get the available friendly names, or see Device Manager -> Imaging devices. Valid `-state:` values are 0 (flash off), 1 (flash on), and 2 (auto-flash). If only `-fname:...` is provided, returns the current state. If `-state:...` is provided after `fname:...`, sets the flash state to the value provided. Returns 0 on success, -1 on failure.

###### Examples

```
camera-cmd.exe flash -fname:Integrated Camera
camera-cmd.exe flash -fname:Integrated Camera -state:2
```

# License

[The MIT License](./LICENSE.md)
