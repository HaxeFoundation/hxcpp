#!/bin/bash
  set -ev

  git clone --recursive https://github.com/HaxeFoundation/haxe.git ~/haxe --depth 1

  sudo add-apt-repository ppa:avsm/ppa -y
  sudo add-apt-repository ppa:haxe/snapshots -y
  sudo apt-get update
  sudo apt-get install -y \
      neko \
      ocaml-nox \
      camlp4-extra \
      opam \
      libpcre3-dev \
      zlib1g-dev \
      awscli

  export OPAMYES=1
  opam init
  eval `opam config env`
  opam pin add haxe ~/haxe --no-action
  opam install haxe --deps-only

  # Build haxe
  pushd ~/haxe
  make ADD_REVISION=1 && sudo make install INSTALL_DIR=/usr/local
  popd
  haxe -version
  # setup haxelib
  mkdir ~/haxelib && haxelib setup ~/haxelib
  haxelib dev hxcpp $TRAVIS_BUILD_DIR
  haxelib install record-macros
