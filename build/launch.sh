DIR="$(dirname "$0")/../Resources"
cd "$DIR"

export DYLD_LIBRARY_PATH="$DIR"

"$DIR/final"

osascript -e 'tell application "Terminal" to do script "'"$PWD/final"'"'

