DIR="$(dirname "$0")/../Resources"
cd "$DIR"

export DYLD_LIBRARY_PATH="$DIR"

# GUI programı doğrudan başlat
"$DIR/client_ui"

osascript -e 'tell application "Terminal" to do script "'"$PWD/client_ui"'"'

