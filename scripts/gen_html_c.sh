#!/bin/bash
# Usage: gen_html_c.sh <html_dir> <out_c> <out_h>
# Embeds all *.html files from html_dir into C source as null-terminated char arrays.

HTML_DIR="$1"
OUT_C="$2"
OUT_H="$3"

cat > "$OUT_H" << 'EOF'
/* Auto-generated at build time - do not edit */
#ifndef HTML_DATA_H
#define HTML_DATA_H

const char *html_get(const char *name);

#endif
EOF

cat > "$OUT_C" << 'EOF'
/* Auto-generated at build time - do not edit */
#include "html_data.h"
#include <string.h>

EOF

NAMES=()

for html in "$HTML_DIR"/*.html; do
    [ -f "$html" ] || continue
    name=$(basename "$html" .html)
    varname="html_${name//-/_}"
    NAMES+=("$name")

    printf 'static const char %s[] = {\n' "$varname" >> "$OUT_C"
    xxd -i < "$html"                                  >> "$OUT_C"
    printf ',\n  0x00\n};\n\n'                        >> "$OUT_C"
done

printf 'static const struct { const char *name; const char *data; } html_table[] = {\n' >> "$OUT_C"
for name in "${NAMES[@]}"; do
    varname="html_${name//-/_}"
    printf '    { "%s.html", %s },\n' "$name" "$varname" >> "$OUT_C"
done
printf '    { 0, 0 }\n};\n\n' >> "$OUT_C"

cat >> "$OUT_C" << 'EOF'
const char *html_get(const char *name) {
    for (int i = 0; html_table[i].name != NULL; i++) {
        if (strcmp(html_table[i].name, name) == 0)
            return html_table[i].data;
    }
    return "";
}
EOF
