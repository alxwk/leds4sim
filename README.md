# leds3sim

This is a quite simple daemon for sending car sim telemetry into a MOZA device (for now, TSW wheel specifically). Existing software like [monocoque](https://github.com/Spacefreak18/monocoque), while being great overall, is not quite fit for use with "civilian", not racing, cars. So this piece takes telemetry info from a memory-mapped file containing info from SDK (in my case, [SCS Truck sim SDK](https://github.com/alxwk/scs-sdk-plugin)) and sends it to the wheel via its interface directly, not using a midlayer like [simapi](https://github.com/Spacefreak18/simapi). (But, as the telemetry info are specified in the config file by its memory-mapped locaions, this software can be use with `simapi` too.)

Disclaimer: it's still pretty raw work-in-progress, so expect changes in logic and configuration.

## Configuration

See `libconfig` documentation for general configuration file structure.
