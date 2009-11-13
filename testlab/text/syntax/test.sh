#!/bin/sh

#  this file was created for testing purposes only
log(){
    echo "$@" >>"$TMPLOG"
}

set_all(){
    value=$1
    shift
    for var in $*; do
        eval $var=$value
    done
}

enable(){
    set_all yes $*
}

disable(){
    set_all no $*
}

enabled(){
    eval test "x\$$1" = "xyes"
}

disabled(){
    eval test "x\$$1" = "xno"
}

check_type(){
    log check_type "$@"
    headers=$1
    type=$2
    shift 2
    disable $type
    incs=""
    for hdr in $headers; do
        incs="$incs
#include <$hdr>"
    done
    check_cc "$@" <<EOF && enable $type
$incs
$type v;
EOF
}

ucname="${pfx}`toupper $cfg`"
me=`echo "$0" | sed -e 's,.*/,,'`

if test "$_as" = auto ; then
  _as=`$_cc -print-prog-name=as`
  test -z "$_as" && _as=as
else
  echo "done"
fi

for ac_option do
  case "$ac_option" in
  # Skip 1st pass
  --target=*) ;;
  --cc=*) ;;
  --as=*) ;;
  esac
done

enabled fastmemcpy	&& x86	|| disable fastmemcpy
print_config USE_ config.h config.mak fastmemcpy

test linux -o bsdos -o freebsd -o sunos	&& enable vcd	|| disable vcd
print_config HAVE_ config.h config.mak tv
enabled vcd	&& _inputmodules="vcd $_inputmodules" || _noinputmodules="vcd $_noinputmodules"
