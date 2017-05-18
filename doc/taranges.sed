1,/^const/ {
  /^const/ !d
}

/none.*uniranges/,$ c\
History\
=======\

/^[{}]/ d
/^#if/,/^#endif/ d
/^  *\/\*/ d

/cans_nonbase_uniranges/ d
/cari_nonbase_uniranges/ d
/cher_nonbase_uniranges/ d
/cprt_nonbase_uniranges/ d
/dsrt_nonbase_uniranges/ d
/geo[rk]_nonbase_uniranges/ d
/goth_nonbase_uniranges/ d
/lat[bp]_nonbase_uniranges/ d
/khms_nonbase_uniranges/ d
/lisu_nonbase_uniranges/ d
/olck_nonbase_uniranges/ d
/orkh_nonbase_uniranges/ d
/osge_nonbase_uniranges/ d
/osma_nonbase_uniranges/ d
/shaw_nonbase_uniranges/ d
/tfng_nonbase_uniranges/ d
/vaii_nonbase_uniranges/ d

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
