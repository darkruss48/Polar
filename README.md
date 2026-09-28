# Polar
A simple, powerful WT Performance Analyser for Dragon Ball Z : Dokkan Battle.

## Features
**_- :rocket: Fast & Easy to use_**

**_- :art: Highly customisable & User-friendly UI_**

**_- :bar_chart: Produces high-quality graphs_**

**_- :earth_africa: Multilingual support_**

**_- :arrows_counterclockwise: Auto-refresh with extra insights for the top leaderboard_**

## Gallery


<img width="1291" height="785" alt="image" src="https://github.com/user-attachments/assets/af67b7d2-3c4e-4813-8240-0af1b0065fc3" />
<img width="1251" height="744" alt="image" src="https://github.com/user-attachments/assets/6c12aae4-c82a-4025-acb5-45535609f60a" />
<img width="1295" height="781" alt="image" src="https://github.com/user-attachments/assets/cd0fcfd0-fb33-41a4-8de8-24de3a5e3cf3" />




## Installation

Check the releases.

## Build

If you want, you can build your own version of Polar.
To do so, you need to download it using [QT](https://qt.io).
I made this client using my own static QT build (v6.8.0), which allows me to compile it and to share it without you needing to install QT.
A dynamic Qt build can be distributed by bundling its required Qt libraries and plugins (for example with windeployqt on Windows). For a single executable, use a separately configured static Qt toolchain; CONFIG += static alone does not turn a dynamic Qt installation into a static one.
Feel free to contact us (check [contact](#Contact)) if you need any help to compile it.

### Build for (Arch) linux

Here are the steps i followed to compile Polar (Arch Linux).
First, download these packages :
```bash
sudo pacman -S qt6 qt6-base qt6-charts
```

Then, clone this repo :
```bash
git clone https://github.com/darkruss48/Polar
cd Polar
```
Create the build folder and cd into :
```bash
mkdir build
cd build
```

Translations are compiled and embedded by qmake; no source-tree `.qm` files are required.

Finally, use qmake and make to compile Polar :
```bash
/usr/lib/qt6/bin/qmake ..
make
```
To start the client :
```bash
./Polar
```
**Note**: If you use Nixos or Nix in general, simply do
```bash
nix build .#polar
```
**:warning: If you're using Arch, there are great chances that QT 5.x is already installed on your device. Polar use QT 6.x, so use `/usr/lib/qt6/bin/qmake ..` instead of `qmake ..` command.**

Polar can be built with Qt 5.x, but don’t expect the program to function as it should. Consider using QT 6.x.

## Contributors

This project was made possible thanks to the hard work and dedication of the following individuals:

- **Polo** : [GitHub Profile](https://github.com/polowiper)
- **Darkruss** : [GitHub Profile](https://github.com/darkruss48)

Thanks to Clєтυн26 for translating Polar in Spanish.
Thanks to LuCaPigeon for translating Polar in Italian.

## Contact
Contact me on [Twitter](https://twitter.com/darkruss47) or reach me on discord : darkruss (or polo as well)

## License
Polar is released under the [MIT License](https://choosealicense.com/licenses/mit/).

## Source layout

- `src/app`: application entry point
- `src/core`: settings and domain logic
- `src/network`: API and update transport
- `src/ui`: native widgets and chart rendering
- `ui`: Qt Designer forms
- `resources/images`: bundled artwork (resource aliases remain unchanged)
- `translations`: Linguist catalogs

## Race analysis

Open **Race analysis** in the menu for a native Top 20 table. Choose a reference
player, a 1/2/6-hour scoring window and an optional extra future pause. Finish
scenarios compare recent pace with active/idle behavior observed in the current
snapshot. They are not confidence intervals or learned cross-edition habits.
Tooltips describe sample coverage and why an estimate may be unavailable.

Tournament `0` always means **Current**, even when metadata omits its edition
number. Archive numbers come from the public archive catalog. Current cannot be
mixed with numbered archives in one range; add another player entry to compare
both. The historical rank estimator requires a known edition number, excludes
the target edition and uses actual edition numbers for its regression.

The API transport is asynchronous, reuses one connection manager and coalesces
identical in-flight requests. Batch comparisons allow four concurrent requests
and at most 40 series. Successful responses have a short bounded cache; explicit
leaderboard refresh bypasses it. No Qt WebEngine, WebView or new runtime module
is required.

## Tests (Qt 6)

```sh
mkdir -p build/tests
cd build/tests
qmake6 ../../tests/tests.pro
make -j4
QT_QPA_PLATFORM=offscreen ./polar_tests
```

Tests cover metadata without an id, region catalogs, series validation, real-time
intervals, missing samples, large scores, projection boundaries, asynchronous
request coalescing and bounded batches, widget input, graph replacement followed
by resizing, and bundled resources. Offscreen plugin size-hint warnings are
expected. Windows static packaging and live tournament accuracy still require
validation on the target platform.
