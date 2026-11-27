#!/usr/bin/env bash
# Installerar pluginen i den här mappen för din användare:
#   VST3 -> ~/.vst3   LV2 -> ~/.lv2
set -e
cd "$(dirname "$0")"
mkdir -p "$HOME/.vst3" "$HOME/.lv2"
shopt -s nullglob
for p in *.vst3; do
  rm -rf "$HOME/.vst3/$p"
  cp -R "$p" "$HOME/.vst3/"
  echo "VST3: $p -> ~/.vst3"
done
for p in *.lv2; do
  rm -rf "$HOME/.lv2/$p"
  cp -R "$p" "$HOME/.lv2/"
  echo "LV2:  $p -> ~/.lv2"
done
# Ljudbanken (BETONGs ljud) för BETONG-KASSETTEN, delas av alla MASKINHALLEN-maskiner
if [ -d Ljudbank ]; then
  BANK="$(xdg-user-dir MUSIC 2>/dev/null || echo "$HOME/Music")/MASKINHALLEN/Ljudbank"
  mkdir -p "$BANK"
  cp -R Ljudbank/. "$BANK/"
  echo "Ljudbank: $BANK"
fi
echo "Klart. Starta om din DAW och låt den söka efter nya plugins."
