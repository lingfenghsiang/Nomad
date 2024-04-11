#!/bin/sh

BRIDGE=virbr0


do_brctl() {
    brctl "$@"
}

do_ifconfig() {
    ifconfig "$@"
}


# setup_bridge_nat "$BRIDGE"

if test "$1" ; then
    do_ifconfig "$1" 0.0.0.0 up
    do_brctl addif "$BRIDGE" "$1"
fi