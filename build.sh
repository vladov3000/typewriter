 CONTENTS="build/Typewriter.App/Contents"
    MACOS="$CONTENTS/MacOS"
RESOURCES="$CONTENTS/Resources"

mkdir -p build "$MACOS" "$RESOURCES"

xcrun -sdk macosx metal  \
    -o build/Shader.ir   \
    -c code/Shader.metal

xcrun -sdk macosx metallib             \
      -o "$RESOURCES/default.metallib" \
      build/Shader.ir

clang -fobjc-arc             \
      -framework Cocoa       \
      -framework Metal       \
      -framework Quartz      \
      -o "$MACOS/Typewriter" \
      code/main.m
