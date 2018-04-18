#!/bin/bash

target=${1:-.}
extensions="hpp cpp cc cxx txx h"

# Construct the patterns from the code extensions
for ext in $extensions
do
  wildcard_ext="*.$ext"
  patterns+=("$wildcard_ext")
done

# Collect all files that match the patterns
for p in "${patterns[@]}"
do
  fnames+=($(find $target -name $p))
done

# For each file, grep for channels
for fname in ${fnames[@]}
do

  # Skip 3rdparty
  if [[ "$fname" = *"3rdparty"* ]]
  then
    continue
  fi

  # Look for standard ALOG macros
  channels=$(cat $fname \
    | grep --color=never -e "ALOG_USE_CHANNEL(" -e "ALOG_USE_CHANNEL_FREE(" -e "ALOG(" \
    | grep -v "#define" \
    | grep -v "__VA_ARGS__" \
    | cut -d'(' -f 2 \
    | cut -d')' -f 1 \
    | cut -d',' -f 1)
  if [ "${#channels}" -gt "0" ]
  then
    for channel in $channels
    do
      all_channels="$all_channels\n$channel"
    done
  fi

  # Look for RLOG
  if grep --color=never RLOG $fname >/dev/null
  then
    all_channels="$all_channels\n$(basename $fname)"
  fi
done

# Make a unique list of channels
unique_channels=$(echo -e ${all_channels} | sort | uniq)

# Print them all out
for channel in $unique_channels
do
  echo $channel
done
