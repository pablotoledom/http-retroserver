#!/bin/bash
# Usage: gen_banner_c.sh <welcome_file> <out_c> <out_h>
# Extracts text from bash echo statements and generates a C string constant.

WELCOME="$1"
OUT_C="$2"
OUT_H="$3"

# Header — just the declaration
cat > "$OUT_H" << 'EOF'
/* Auto-generated at build time - do not edit */
#ifndef BANNER_DATA_H
#define BANNER_DATA_H
extern const char BANNER[];
#endif
EOF

# Source — the actual string data
{
    printf '/* Auto-generated at build time - do not edit */\n'
    printf '#include "banner_data.h"\n\n'
    printf 'const char BANNER[] =\n'

    grep "^echo" "$WELCOME" | while IFS= read -r line; do
        # Empty line
        if [[ "$line" == "echo" || "$line" == "echo ''" || "$line" == 'echo ""' ]]; then
            printf '"\\n"\n'
            continue
        fi
        # Extract text between single quotes: echo '...'
        if [[ "$line" =~ ^echo\ \'(.*)\'$ ]]; then
            content="${BASH_REMATCH[1]}"
        # Extract text between double quotes: echo "..."
        elif [[ "$line" =~ ^echo\ \"(.*)\"$ ]]; then
            content="${BASH_REMATCH[1]}"
        else
            continue
        fi
        # Escape backslashes and double quotes for C string literal
        content="${content//\\/\\\\}"
        content="${content//\"/\\\"}"
        printf '"%s\\n"\n' "$content"
    done

    printf ';\n'
} > "$OUT_C"
