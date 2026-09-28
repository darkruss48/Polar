{pkgs}:
pkgs.stdenv.mkDerivation {
  pname = "polar";
  version = "1.2.0";

  src = ./.;

  nativeBuildInputs = with pkgs; [
    qt6.wrapQtAppsHook
    qt6.qmake
    qt6.qttools
    qt6.qtbase
    qt6.qtcharts
    gnumake
  ];

  buildPhase = ''
    export PATH="$PATH:${pkgs.qt6.qttools}/bin"
    qmake ./Polar.pro

    sed -i "s|${pkgs.qt6.qtbase}/bin/lrelease|${pkgs.qt6.qttools}/bin/lrelease|g" Makefile
    make
  '';

  installPhase = ''
    mkdir -p $out/bin
    ls $out
    cp Polar $out/bin/
  '';
}
