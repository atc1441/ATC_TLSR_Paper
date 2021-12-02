# declare a JTAG or SWD Debug Access Point (DAP)
# based on the transport in use with this session.
# You can't access JTAG ops when SWD is active, etc.

# params are currently what "jtag newtap" uses
# because OpenOCD internals are still strongly biased
# to JTAG ....  but for SWD, "irlen" etc are ignored,
# and the internals work differently

# for now, ignore non-JTAG and non-SWD transports
# (e.g. initial flash programming via SPI or UART)

# split out "chip" and "tag" so we can someday handle
# them more uniformly irlen too...)

if [catch {transport select}] {
  echo "Error: unable to select a session transport. Can't continue."
  shutdown
}

proc swj_newdap {chip tag args} {
     eval swd newdap $chip $tag $args
}
