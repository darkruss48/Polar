# Polar
A simple, powerful WT Performance Analyser for Dragon Ball Z : Dokkan Battle.

## Features
**_- :rocket: Fast & Easy to use_**

**_- :art: Highly customisable & User-friendly UI_**

**_- :bar_chart: Produces high-quality graphs_**

**_- :earth_africa: Multilingual support_**

**_- :arrows_counterclockwise: Auto-refresh with extra insights for the top leaderboard_**

## Gallery

<img width="1256" height="732" alt="image" src="https://github.com/user-attachments/assets/c9803824-1125-42d7-a333-c6337add7cec" />
<img width="1251" height="744" alt="image" src="https://github.com/user-attachments/assets/6c12aae4-c82a-4025-acb5-45535609f60a" />



## Installation

Check the releases.

## Build

If you want, you can build your own version of Polar.
To do so, you need to download it using [QT](https://qt.io).
I made this client using my own static QT build (v6.8.0), which allows me to compile it and to share it without you needing to install QT.
If you build it yourself with the distributed version of QT (the dynamic one), it will work for you but you can't share your executable as it need to have QT installed on the computer to start.
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

Compile the thanslations file by typing this command :
```bash
/usr/lib/qt6/bin/lrelease ../*.ts
```

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

## Contact
Contact me on [Twitter](https://twitter.com/darkruss47) or reach me on discord : darkruss (or polo as well)

## License
Polar is released under the [MIT License](https://choosealicense.com/licenses/mit/).