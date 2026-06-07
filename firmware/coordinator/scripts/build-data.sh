#!/usr/bin/env bash
# Copies vite build artifacts from web/dist to data/ for LittleFS imaging.
# Removes uncompressed twin files (firmware serves .gz directly).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/web/dist"
DST="$ROOT/data"

if [ ! -d "$SRC" ]; then
  echo "error: $SRC does not exist — run 'npm run build' in web/ first" >&2
  exit 1
fi

# Clean dst except .gitkeep
find "$DST" -mindepth 1 ! -name '.gitkeep' -delete

# Copy everything from dist
cp -R "$SRC/." "$DST/"

# Drop uncompressed twins where a .gz exists — saves LittleFS space
find "$DST" -type f ! -name '*.gz' ! -name '.gitkeep' | while read -r f; do
  if [ -f "$f.gz" ]; then rm "$f"; fi
done

echo "data/ contents (post-gzip):"
( cd "$DST" && find . -type f -not -name '.gitkeep' -exec ls -lh {} \; )
TOTAL=$(find "$DST" -type f ! -name '.gitkeep' -exec wc -c {} + | tail -1 | awk '{print $1}')
echo "Total: $TOTAL bytes"
if [ "$TOTAL" -gt 204800 ]; then
  echo "warning: total exceeds 200KB hard cap from brief §7.2" >&2
fi
