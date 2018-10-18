#!/bin/sh

# 8 digits = 4 bytes = 32 bits
hexdigits_per_limb=8

limbs_per_row=4

sed_cmd="s/.\\{${hexdigits_per_limb}\\}/0x&,\\n/g"

# 1: Remove line breaks.
# 2: Remove all other whitespace.
# 3: Add 0x and commas with \n after them.
# 4: Reverse the lines.
# 5: Remove most line breaks. Only keep every "limb_per_row" line break.

#   1              2              3          4    5
tr -d '\n' | sed 's/\s//g' | sed $sed_cmd | tac | {
  line_count=0
  while read line
  do
    n=$((n+1))
    printf "$line"
    if [[ "$n" == "$limbs_per_row" ]]; then
      printf "\n"
      n=0
    fi
  done
}
