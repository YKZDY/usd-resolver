#!/bin/sh
# Copyright (C) 1998-2020 Free Software Foundation, Inc.
# This file is part of the GNU C Library.
# Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with the GNU C Library; if not, see
# <https://www.gnu.org/licenses/>.

if test $# -eq 0; then
  echo "$0: missing program name" >&2
  echo "Try \`$0 --help' for more information." >&2
  exit 1
fi

prog="$1"
shift

if test $# -eq 0; then
  case "$prog" in
    --h | --he | --hel | --help)
      echo 'Usage: catchsegv PROGRAM ARGS...'
      echo '  --help      print this help, then exit'
      echo '  --version   print version number, then exit'
      echo 'For bug reporting instructions, please see:'
      cat <<\EOF
<https://bugs.launchpad.net/ubuntu/+source/glibc/+bugs>.
EOF
      exit 0
      ;;
    --v | --ve | --ver | --vers | --versi | --versio | --version)
      echo 'catchsegv (Ubuntu GLIBC 2.31-0ubuntu9.2) 2.31'
      echo 'Copyright (C) 2020 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
Written by Ulrich Drepper.'
      exit 0
      ;;
    *)
      ;;
  esac
fi

segv_output=`mktemp ${TMPDIR:-/tmp}/segv_output.XXXXXX` || exit

# Redirect stderr to avoid termination message from shell.
(exec 3>&2 2>/dev/null
LD_PRELOAD=${LD_PRELOAD:+${LD_PRELOAD}:}/lib/x86_64-linux-gnu/libSegFault.so \
SEGFAULT_USE_ALTSTACK=1 \
SEGFAULT_OUTPUT_NAME=$segv_output \
"$prog" ${1+"$@"} 2>&3 3>&-)
exval=$?

# Check for output.  Even if the program terminated correctly it might
# be that a minor process (clone) failed.  Therefore we do not check the
# exit code.
if test -s "$segv_output"; then
  # The program caught a signal.  The output is in the file with the
  # name we have in SEGFAULT_OUTPUT_NAME.  In the output the names of
  # functions in shared objects are available, but names in the static
  # part of the program are not.  We use addr2line to get this information.
  case $prog in
  */*) ;;
  *)
    old_IFS=$IFS
    IFS=:
    for p in $PATH; do
      test -n "$p" || p=.
      if test -f "$p/$prog"; then
        prog=$p/$prog
      break
      fi
    done
    IFS=$old_IFS
    ;;
  esac
  sed '/Backtrace/q' "$segv_output"
  sed '1,/Backtrace/d' "$segv_output" |
  (while read line; do
     case "$line" in
        *\(*\)\[*\])
          lib=`echo $line  | sed "s@^\(.*\)(\(.*\))\[\(.*\)\]\\$@\1@"`
          offs=`echo $line | sed "s@^\(.*\)(\(.*\))\[\(.*\)\]\\$@\2@"`
          addr=`echo $line | sed "s@^\(.*\)(\(.*\))\[\(.*\)\]\\$@\3@"`
          func=""
          case "$offs" in
            *,*) func=`echo $offs | sed "s@\(.*\),\(.*\)\\$@\1,@"`
                 offs=`echo $offs | sed "s@\(.*\),\(.*\)\\$@\2@"`
                 ;;
          esac
          case "$offs" in
            +*) case "$prog" in
                  */$lib)
                    lib="$prog"
                    ;;
                esac
                lib=`realpath --relative-base . -s "$lib"`
                complete=`addr2line -C -p -i -f -e "$lib" $offs 2>/dev/null`
                if test $? -eq 0; then
                  func=`echo $complete  | sed "s@^\(.*\) at \(.*\):\(.*\)\\$@\1@"`
                  file=`echo $complete  | sed "s@^\(.*\) at \(.*\):\(.*\)\\$@\2@"`
                  line=`echo $complete  | sed "s@^\(.*\) at \(.*\):\(.*\)\\$@\3@"`
                  realfile=`realpath --relative-base . -s "$file" 2>/dev/null`
                  if test $? -eq 0; then
                    file="$realfile"
                  fi
                  echo " $lib $func at $file:$line"
                else
                  echo " $lib ($func$offs)[$addr]"
                fi
                ;;
            *) lib=`realpath --relative-base . -s "$lib"`
               echo " $lib $offs" | c++filt
               ;;
          esac
          ;;
         *) echo "$line"
            ;;
     esac
   done)
fi
rm -f "$segv_output"

exit $exval