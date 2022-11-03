let pkgs = import <nixpkgs> {};
  stdenv = pkgs.stdenv;
  stflash = "${pkgs.stlink}/bin/st-flash";
  stutil = "${pkgs.stlink}/bin/st-util";
  simplescript = body: let
    name = "__binscript";
    script = pkgs.writeShellScriptBin name body;
  in "${script}/bin/${name}";

  getoneof = simplescript ''
    files="$(eval echo "$*")"
    nfiles="$(echo "$files" | wc -w)"
    if [ x"$nfiles" == x"1" ] && [ -f "$files" ]; then
      echo "$files"
      exit 0
    fi
    exit 1
  '';
  flasher = simplescript ''
    set -e
    ${stflash} --reset write "$(${getoneof} "$1/*.bin")" 0x8000000 || true
  '';
  rungdb = let
    runner = simplescript ''
      set -e
      port="$(( $RANDOM % 10000 + 42424 ))"
      stpid=
      elf="$1"
      function cleanup {
        if [ -n "$stpid" ]; then
          echo "Stopping stutil on $stpid"
          kill $stpid
          wait $stpid
        fi
      }
      trap cleanup EXIT
      ${stutil} -p $port >/dev/null 2>/dev/null &
      stpid="$!"
      ${pkgs.gdb}/bin/gdb "$elf" -ex 'target extended-remote localhost:'"$port"
    '';
  in simplescript ''
    set -e
    exec ${runner} "$(${getoneof} "$1/*.elf")"
  '';
in
stdenv.mkDerivation {
  name = "bluepill-serial-monster";
  buildInputs = with pkgs; [
    pkgs.libusb1
    gcc-arm-embedded
  ];
  src = ./.;
  STM32CUBE_PATH = pkgs.fetchFromGitHub {
    owner = "STMicroelectronics";
    repo = "STM32CubeF1";
    rev = "v1.8.4";
    sha256 = "ICGgQCkY5E5Lcd7+U+hX5+MJcTF7J51NFDx6iy/SfgA=";
  };
  installPhase = ''
    mkdir -p "$out"
    cp *.bin "$out"
    cp *.elf "$out"
    echo '#!/bin/bash' > $out/flasher
    echo "exec ${flasher} \"$out\"" >> "$out/flasher"
    chmod +x "$out/flasher"
    echo '#!/bin/bash' > $out/debug
    echo "exec ${rungdb} \"$out\"" >> "$out/debug"
    chmod +x "$out/debug"
  '';
  postInstall = "true";
}
