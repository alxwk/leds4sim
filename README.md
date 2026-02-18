# leds4sim

This is a quite simple daemon for sending car sim telemetry into a MOZA
device (for now, TSW wheel specifically). Existing software like
[monocoque](https://github.com/Spacefreak18/monocoque), while being
great overall, is not quite fit for use with "civilian", not racing,
cars. So this piece takes telemetry info from a memory-mapped file
containing info from SDK (in my case, [SCS Truck sim
SDK](https://github.com/alxwk/scs-sdk-plugin)) and sends it to the wheel
via its interface directly, not using a midlayer like
[simapi](https://github.com/Spacefreak18/simapi). (But, as the telemetry
info is specified in the config file by its memory-mapped locaions, this
software can be used with `simapi` too.)

Disclaimer: it's still pretty raw work-in-progress, so expect changes in
logic and configuration.

## Using release binaries

The `.deb` package is provided for deb-based distros. Check its dependencies.

There's also a statically build executable, for those who want to test it
without installation, or just prefer it this way. Don't forget to `unxz` it
and make it executable (`chmod +x`).

## Building from source

### Build dependencies
* libserial
* libconfig++
* libxdg-basedir

### Building
```
cmake -B build
cd build
make
```

## Configuration

The configuration file is mandatory and needs to be either in the
current directory (takes priority), or somewhere in XDG-compliant
location (standard priorities apply), usually `~/.config/leds4sim.conf`.
See `libconfig` documentation for general structure of the configuration
file, and comments in [conf/leds4sim.conf](conf/leds4sim.conf).
