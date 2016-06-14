1,/^const/ {
  /^const/ !d
}

/none.*uniranges/,$ c\
History\
=======\

/^[{}]/ d
/^#if/,/^#endif/ d
/^  *\/\*/ d

/cher_nonbase_uniranges/ d
/geo[rk]_nonbase_uniranges/ d
/lat[bp]_nonbase_uniranges/ d
/khms_nonbase_uniranges/ d

s|.*ta_\(.*\)_nonbase_uniranges.*|Table: `\1` non-base characters\
\
     Character range\
  ---------------------|

s|.*ta_\(.*\)_uniranges.*|Table: `\1` base characters\
\
     Character range     Description\
  ---------------------  -------------|

s|.*\(0x.....\),.*\(0x.....\)).*/\* \(.*\) \*/.*|  `\1` - `\2`  \3|
s|.*\(0x....\),.*\(0x....\)).*/\* \(.*\) \*/.*|  `\1` - `\2`    \3|

s|.*\(0x.*\),.*\(0x.*\)).*|  `\1` - `\2`|

s|  /\* \(.*\) \*/|                         (\1)|

/TA_UNIRANGE_REC/ d

s|/\* \(.*\) \*/|\1|
