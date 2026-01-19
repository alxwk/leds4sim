# leds4sim

This is a quite simple daemon for sending car sim telemetry into a MOZA device (for now, TSW wheel specifically). Existing software like [monocoque](https://github.com/Spacefreak18/monocoque), while being great overall, is not quite fit for use with "civilian", not racing, cars. So this piece takes telemetry info from a memory-mapped file containing info from SDK (in my case, [SCS Truck sim SDK](https://github.com/alxwk/scs-sdk-plugin)) and sends it to the wheel via its interface directly, not using a midlayer like [simapi](https://github.com/Spacefreak18/simapi). (But, as the telemetry info is specified in the config file by its memory-mapped locaions, this software can be used with `simapi` too.)

Disclaimer: it's still pretty raw work-in-progress, so expect changes in logic and configuration.

## Build dependencies

* libserial
* libconfig++
* libxdg-basedir

## Building from source

```
cmake -B build
cd build
make
```

## Configuration

The configuration file is mandatory and needs to be either in the current directory (takes priority), or somewhere in XDG-compliant location (standard priorities apply), usually `~/.config/leds4sim.conf`. See `libconfig` documentation for general structure of the configuration file, and comments in [conf/leds4sim.conf](conf/leds4sim.conf).
