{pkgs}:
pkgs.stdenv.mkDerivation {
  pname = "polar";
  version = "1.1.0";

  src = pkgs.fetchFromGitHub {
    owner = "darkruss48";
    repo = "Polar";
    rev = "v1.1.0";
    sha256 = "sha256-xdhRoN788zLhO7E9uBegYBplrOJ5WLeNQsVKcPjdhLs=";
  };

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

    ${pkgs.qt6.qttools}/bin/lrelease ./Polar_en_US.ts -qm ./Polar_en_US.qm
    ${pkgs.qt6.qttools}/bin/lrelease ./Polar_fr_FR.ts -qm ./Polar_fr_FR.qm
    sed -i "s|${pkgs.qt6.qtbase}/bin/lrelease|${pkgs.qt6.qttools}/bin/lrelease|g" Makefile
    make
  '';

  installPhase = ''
    mkdir -p $out/bin
    ls $out
    cp Polar $out/bin/
  '';
}
