## YASDI SDK for SMA Inverters

### Disclaimer

This repository is mostly meant to be a mirror for the [SDK sources provided by SMA](https://www.sma.de/en/products/apps-software/yasdi.html) found in the [sdk](sdk)
directory of this repository to make things more convenient and definitely not actively developed by me.

### Building

Assuming you have the needed build tools like gcc, cmake, make, etc. installed:

```
mkdir build && cd build
cmake ../sdk/projects/generic-cmake
make -j$(getconf _NPROCESSORS_ONLN)
```

Or refer to the build instructions provided by the [authors of this SDK](sdk/README)

### Example YASDI configuration file

```
[DriverModules]
Driver0=libyasdi_drv_serial

[COM1]
Device=/dev/ttyUSB0
Media=RS485
Baudrate=1200
Protocol=SMANet

[Misc]
DebugOutput=/dev/stderr 
```

#### Running

Create a file yasdi.ini in the build directory (where yasdishell is) with the
[contents from the example](#example-yasdi-configuration-file)

This configuration file assumes
* RS485: you used cabling topology (daisy chained inverters) for the RS485 protocol
* /dev/ttyUSB0: YASDI communicates with your inverters via this serial to usb converter

When yasdishell is started it tries to load the shared libraries libyasdi* files from the system directories.
This will not work unless you installed YASDi using `make install`.

If you didn't `make install` YASDI and want yasdishell to use the libyasdi* library from the build directory
you need to set LD_LIBRARY_PATH:

```
# assuming you are in the build directory:
export LD_LIBRARY_PATH=$(pwd)
```

If sma.ini and yasdishell are in the same directory run using `./yasdishell`

If sma.ini is in a different place run using `./yasdishell /path/to/sma.ini`

### Building the container image

From the root of the repository:

`podman build -t yasdi -f docker/alpine.Dockerfile .`

(If you you docker, replace 'podman' with 'docker')

### Running the container image

Create a configuration file `/yasdi/yasdi.ini` on the host adapting [the example](#example-yasdi-configuration-file) to your needs

Run the container image mounting the configuration file's directory

`podman run --rm -it --name yasdi -v /yasdi/yasdi.ini:/yasdi/yasdi.ini:rw yasdi yasdishell`