# Tekken 360 KDE launcher

A native Linux Qt Quick/Kirigami launcher for
[Tekken 6](https://github.com/tutifrutas/tekken6-360-native-linux) and
[Tekken Tag Tournament 2](https://github.com/tutifrutas/tekken-tag-tournament-2-360-native-linux).
It starts separately built native executables. It does not build, download or
include either game. Online play is not supported.

## Build

Requirements: Linux, a C++20 compiler, CMake 3.25+, Ninja, Qt 6.8+ development
packages (Core, Gui, Qml, Quick, QuickControls2, Network and Test), SDL3,
KDE ECM 6+ and the Kirigami QML module. Install the Qt Quick Controls KDE
style for desktop integration. The launcher does not require the game SDK.

```sh
git clone https://github.com/tutifrutas/tekken-360-kde-launcher.git
cd tekken-360-kde-launcher
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build --parallel 6
ctest --test-dir build --output-on-failure
build/launcher/tekken_launcher
# Optional desktop installation:
cmake --install build --prefix "$HOME/.local"
```

Use `-DBUILD_TESTING=OFF` to build without Qt Test.

## Use

Select a game and browse to its installation folder. Each folder must contain
`tekken6` or `ttt2`, `librexruntime.so`, `librexgpu-xenos.so`, and `game/` with
your own prepared game files. Follow the corresponding game repository's
instructions to build and prepare that folder.

Game cards support Tab, Space, Enter and Left/Right. Selecting a card does not
launch a game. Settings are separate for each game; installation locations are
not guessed. The launcher preserves saves, caches and logs. It asks before
stopping a running game when closing the window. Each game child uses its
permitted hard open-file limit for NTSYNC; the launcher and system-wide limits
are unchanged.

Both games use Wayland and the Vulkan FSI/ROV renderer. TTT2 uses full resolve
readback. Rendering controls apply independently to each game.

Settings use `~/.config/TekkenNative/TekkenLauncher/launcher.ini`, respecting
`XDG_CONFIG_HOME`.

## Cover artwork

No images are bundled or committed. On first use the launcher requests the
[Tekken 6 cover](https://gamesdb.launchbox-app.com/games/images/13022-tekken-6)
and [TTT2 cover](https://gamesdb.launchbox-app.com/games/images/12078-tekken-tag-tournament-2)
from `images.launchbox-app.com` over HTTPS. This contacts a third-party service,
which receives your IP address. Valid images are cached in
`~/.cache/TekkenNative/TekkenLauncher/covers`, respecting `XDG_CACHE_HOME`.
Subsequent launches use the cache without contacting the image host. Delete
that directory to refresh the covers. Downloads are size-limited and time out;
if the service or cache is unavailable, text-only cards remain usable.

Cover art and trademarks belong to their respective owners, including Bandai
Namco and Microsoft. The source-code license does not cover artwork or grant
permission to redistribute downloaded images. Artwork downloads are separate
from game networking; there is no online-play integration.

## License

Original launcher code is [BSD-3-Clause](LICENSE). Qt, KDE and SDL retain their
own licenses. This project is independent of Bandai Namco and Microsoft.
