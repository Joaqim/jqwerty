{
  lib,
  stdenv,
  cmake,
  extra-cmake-modules,
  fcitx5,
  libxkbcommon,
}:

stdenv.mkDerivation {
  pname = "fcitx5-jqwerty";
  version = "0.1.1";

  src = lib.cleanSource ./.;

  nativeBuildInputs = [
    cmake
    extra-cmake-modules
  ];

  buildInputs = [
    fcitx5
    libxkbcommon
  ];

  # Let fcitx5's cmake config set the correct addon/IM install dirs,
  # rather than letting cmake guess lib vs lib64.
  cmakeFlags = [
    "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
  ];

  meta = {
    description = "fcitx5 jqwerty engine — physical-position key remapping";
    license = lib.licenses.mit;
    platforms = lib.platforms.linux;
  };
}
