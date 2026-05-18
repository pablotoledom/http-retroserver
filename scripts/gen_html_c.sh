#!/bin/bash
# Usage: gen_html_c.sh <html_dir> <out_c> <out_h>
# Embeds all *.html files from html_dir into C source.
# Each template is stored with its exact byte length so tmpl_append
# can iterate by length instead of relying on null-termination.

HTML_DIR="$1"
OUT_C="$2"
OUT_H="$3"

cat > "$OUT_H" << 'EOF'
/* Auto-generated at build time - do not edit */
#ifndef HTML_DATA_H
#define HTML_DATA_H

const char *html_get(const char *name, int *out_len);

#endif
EOF

cat > "$OUT_C" << 'EOF'
/* Auto-generated at build time - do not edit */
#include "html_data.h"
#include <string.h>

EOF

NAMES=()
SIZES=()

for html in "$HTML_DIR"/*.html; do
    [ -f "$html" ] || continue
    name=$(basename "$html" .html)
    varname="html_${name//-/_}"
    len=$(wc -c < "$html")
    NAMES+=("$name")
    SIZES+=("$len")

    printf 'static const char %s[] = {\n' "$varname"     >> "$OUT_C"
    xxd -i < "$html"                                      >> "$OUT_C"
    printf '\n};\n'                                       >> "$OUT_C"
    printf 'static const int %s_len = %s;\n\n' "$varname" "$len" >> "$OUT_C"
done

printf 'static const struct { const char *name; const char *data; int len; } html_table[] = {\n' >> "$OUT_C"
for i in "${!NAMES[@]}"; do
    name="${NAMES[$i]}"
    varname="html_${name//-/_}"
    printf '    { "%s.html", %s, %s_len },\n' "$name" "$varname" "$varname" >> "$OUT_C"
done
printf '    { 0, 0, 0 }\n};\n\n' >> "$OUT_C"

cat >> "$OUT_C" << 'EOF'
const char *html_get(const char *name, int *out_len) {
    int i;
    for (i = 0; html_table[i].name != NULL; i++) {
        if (strcmp(html_table[i].name, name) == 0) {
            if (out_len) *out_len = html_table[i].len;
            return html_table[i].data;
        }
    }
    if (out_len) *out_len = 0;
    return "";
}
EOF
