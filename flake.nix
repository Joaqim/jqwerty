{
  description = "fcitx5-jqwerty — physical-position input method engine";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        inherit (pkgs.qt6Packages) fcitx5-with-addons fcitx5-configtool;

        # 1. Build our addon as a normal derivation.
        fcitx5-jqwerty = pkgs.callPackage ./package.nix { };

        # 2. Wrap fcitx5 with our addon injected alongside the standard ones.
        #    fcitx5-with-addons is the NixOS-blessed wrapper that sets the
        #    correct FCITX_ADDON_DIRS / data paths — always use it, never
        #    call pkgs.fcitx5 directly (that's the unwrapped binary).
        fcitx5-with-jqwerty = fcitx5-with-addons.override {
          addons = [ fcitx5-jqwerty ];
        };

      in
      {
        # `nix build` produces the addon .so + conf files.
        packages = {
          default = fcitx5-jqwerty;
          inherit fcitx5-with-jqwerty;
        };

        # `nix develop` drops you into a shell with everything needed to
        # do iterative cmake builds by hand, with fcitx5 headers on PATH.
        devShells.default = pkgs.mkShell {
          inputsFrom = [ fcitx5-jqwerty ];

          packages = [
            # Run the wrapped fcitx5 directly for live testing:
            fcitx5-with-jqwerty
            # Useful for inspecting loaded addons:
            fcitx5-configtool
          ];

          # Tell a manually-launched fcitx5 where to find your
          # freshly-built addon without a full install.
          shellHook = ''
            export FCITX_ADDON_DIRS="$(pwd)/build/lib/fcitx5:${fcitx5-jqwerty}/lib/fcitx5"
            echo "Dev shell ready."
            echo "  cmake -B build -DCMAKE_BUILD_TYPE=Debug"
            echo "  cmake --build build"
            echo '  FCITX_DEBUG="*" fcitx5 -r'
          '';
        };
      }
    )

    # NixOS module so you can add this to any system config:
    // {
      nixosModules.default =
        { pkgs, ... }:
        {
          i18n.inputMethod = {
            enable = true;
            type = "fcitx5";
            fcitx5.addons = [
              self.packages.${pkgs.system}.default
            ];
          };
        };
    };
}
