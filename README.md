## chkservice

[![Build Status](https://img.shields.io/travis/linuxenko/chkservice.svg?style=flat-square)](https://travis-ci.org/linuxenko/chkservice)

[![screenshot](https://raw.githubusercontent.com/linuxenko/linuxenko.github.io/master/media/chkservice/chkservice.png)](https://raw.githubusercontent.com/linuxenko/linuxenko.github.io/master/media/chkservice/chkservice.png)

> chkservice is a tool for managing systemd units in terminal.

[![Packaging status](https://repology.org/badge/vertical-allrepos/chkservice.svg)](https://repology.org/metapackage/chkservice)

### Installation

APT

```
sudo apt install chkservice
```

Arch

```
git clone https://aur.archlinux.org/chkservice.git
cd chkservice
makepkg -si
```

### Usage

`chkservice` requires super user privileges to make changes. For user it works read-only.

### Dependencies

Package dependencies:
  * libncurses5
  * libsystemd0 ( >= 222 )
  
Build dependencies:
  * pkg-config
  * libncurses5-dev
  * libsystemd-dev ( >= 222 )

### Build

Build and install debian package.

```
git clone https://github.com/linuxenko/chkservice.git
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ../
cpack

dpkg -i chkservice-x.x.x.deb
```

Build release version.

```
git clone https://github.com/linuxenko/chkservice.git
mkdir build
cd build
cmake ../
make
```

To build debug version, `DEBUG` environment should be set

```
export DEBUG=1
cmake ....
make Test
```

### Changelog

  * v0.3 - Search using `/` key added
  * v0.3 - Emacs key bindings implemented
  * v0.2 - Integration with Travis was fixed
  * v0.2 - Window resize support added by Gilles Talis <gilles.talis@gmail.com>


### License
GNU General Public License

chkservice is a tool for managing systemd units.
more information at https://github.com/linuxenko/chkservice
