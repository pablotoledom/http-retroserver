#!/bin/bash
# Usage: gen_icons_c.sh <icons_dir> <out_c> <out_h>
# Embeds all *.gif files from icons_dir into C source as byte arrays.

ICONS_DIR="$1"
OUT_C="$2"
OUT_H="$3"

cat > "$OUT_H" << 'EOF'
/* Auto-generated at build time - do not edit */
#ifndef ICONS_DATA_H
#define ICONS_DATA_H

typedef struct {
    const char          *name;
    const unsigned char *data;
    unsigned int         len;
} IconEntry;

extern const IconEntry icon_table[];

#endif
EOF

cat > "$OUT_C" << 'EOF'
/* Auto-generated at build time - do not edit */
#include "icons_data.h"

EOF

NAMES=()

for gif in "$ICONS_DIR"/*.gif; do
    [ -f "$gif" ] || continue
    name=$(basename "$gif" .gif)
    varname="icon_${name}"
    len=$(wc -c < "$gif")
    NAMES+=("$name")

    printf 'static const unsigned char %s[] = {\n' "$varname"  >> "$OUT_C"
    xxd -i < "$gif"                                             >> "$OUT_C"
    printf '\n};\n'                                             >> "$OUT_C"
    printf 'static const unsigned int %s_len = %s;\n\n' "$varname" "$len" >> "$OUT_C"
done

printf 'const IconEntry icon_table[] = {\n' >> "$OUT_C"
for name in "${NAMES[@]}"; do
    printf '    { "%s.gif", icon_%s, icon_%s_len },\n' "$name" "$name" "$name" >> "$OUT_C"
done
printf '    { 0, 0, 0 }\n};\n' >> "$OUT_C"
