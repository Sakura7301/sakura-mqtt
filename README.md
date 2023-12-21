# Project of SAKURA-MQTT-SDK

This is a lightweight MQTT client that supports high concurrency.

## Helper

- Recursively init and update the submodules, such as: int and update the submodule of a specific submodule.

  ``` bash
  git submodule update --init --recursive
  ```

## Introduction

- `archive`: set of archives;

- `config`: template of configuration files;

- `dist`: for intermediate products;

- `inc`: header files;

- `release`: release products;

- `src`: source files;

- `test`: test files;

- `tools`: helper for build;

- `compiler_mks`: configurations for the supported compilers;

## Development Guide

[Development Guide](./archive/doc/SAKURA-MQTT-SDK-Development-Guide.md)

## Build from make

Get a copy of configuration file from directory `config`, and install it to the root directory with name `.config`, then read and modify it to what you want. For example, you could just use `config/default_unix.config`, if you only want NOSR data and your system(platform) is `Linux`|`MacOS`. A simple command `make` could help you generate the library, also `make demo` could get the `demo` executable.

- shell commands:

``` bash
cp config/default_unix.config .config
make
make demo
```

- build arm-linux edition:  

``` bash
cp config/default_unix.config .config
make CROSS_COMPILE=/home/sakura/Desktop/toolchain/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
make CROSS_COMPILE=/home/sakura/Desktop/toolchain/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
```

Replace the values of CROSS_COMPILE with your values according to your requirement.

- **Another construction method**

1. You need to open this file and modify it. There are several points to note:

   1. ```
      vim ./configs.json
      ```

   2. The value of `enable` must be `true`. Otherwise, it will not be compiled.

   3. The value of `cross_compile` is the absolute address of your cross compiled chain. Please note that when filling in the address, please omit the `gcc` at the end. 

2. You don't need to modify the other options.

3. Below we will show you an example:

   1. ```
      {
          "customer": "arm",
          "enable": true,
          "platform": "gcc-linaro-dmem",
          ".config": "default_unix.config",
          "cross_compile": "/home/sakura/Desktop/toolchain/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-",
          "archive": {
              "doc": [
                  "dev_manual"
              ],
              "include": [
                  "client",
                  "types"
              ],
              "demo": [
                  "client"
              ]
          }
      }
      ```

4. Next, you need to execute this command line：

   ```
   python automatic_build_script.py
   ```

5. After the compilation is completed, you can see the finished package in the `./release` directory.

### configs and templates

We have two `JSON` configuration files in the root directory: `configs.json` and `templates.json` which help us easily customize the build and archive configuration.

#### configs.json

- Common members:
  - `customer`: maybe the name of our customer, the build product would be named after this parameter;
  - `enable`: `false` to disable this config, empty or `true` to enable the config;
  - `platform`: a string to indicate where the target to run;
  - `compiler`: name of the compiler, by default `gcc`, which must corresponds to a compiler configuration file(in format `${compiler}.mk`) in the root directory, like `gcc.mk`;
  - `version`: version of the compiler;
  - `cross_compile`: the prefix of the toolchain;
  - `ssl_version`: version of openssl, it takes effect only when CONFIG_SSL=y in `.config`
  - `ldflags`: flags for compiler to generate dynamic library;
  - `cflags`: if you must set some `CFLAGS` when build, you could set them here, multiply flags are support in the format: `["-flag1", "-flag2", "-flag3=value3", "-flag4=value4", ...]`;
  - `macros`: if you want to inject some macros to `source code`, you could set them here in an array: `["MACRO1=VALUE1", "MACRO2=VALUE2", ...]`;
  - `environments`: if you want ot inject some definitions to `Makefile`(actual `Make`), you could set them here in format of an array: `["ENV_1=VALUE1", "ENV_2=VALUE2", ...]`, the environments defined here would be invisible to the source code;
  - `feature`: feature name of product, we could use it to distinguish multiple products;
  - `.config`: specify the configuration file, which locates in the directory `config`;
  - `archive`: including `doc`, `include` and `demo` which defines the files would be archived to the product;

#### templates.json

We could define some module and archive templates here. And also they could be referenced in the `configs.json` to determine which file would be archived to the target package.

- `archive`: more details would come soon;
  - `doc`: developing document;
  - `include`: header files for developer;
  - `demo`: demo for developer;
  - `note`: the development log of the program during the dev phase;
  

### Add a new build config

First, you should confirm that what system your compiler running on, then add a json object to the `${system}` member in the `configs.json`, there are some keys you could edit or just remove it if you do not need to modify it.

- Windows system:

  Right now, only `IAR`, `Keil`, `greenHills` and `gcc` from `Cygwin | MinGW | MSYS2` are supported on Windows system. Specify the right compiler.

### Build and fetch product

After the modification of the above config file, just execute `python automatic_build_script.py`.

Then, relax for a while, and get the product named after `libsakura-mqtt-${sdk_version}-${build_time}-${platform}-${customer}-${feature}.package.zip` in directory `release`. There are `debug` and `release` libraries in the package.

## Main Component

- MQTT API;
- Common Work API;
- Log;
- Protocols;
  - DNS;
- Platform;
  - daemon;
  - memory;
  - mutex;
  - socket;

- Utils;
  - CJSON;
  - utils;

## CUnit

### install CUnit

- Mac:

``` bash
brew install cunit
```

- Linux:

``` bash
sudo apt-get install -y libcunit1 libcunit1-doc libcunit1-dev
```

or build with source code

``` bash
wget http://downloads.sourceforge.net/project/cunit/CUnit/2.1-2/CUnit-2.1-2-src.tar.bz2
tar xf CUnit-2.1-2-src.tar.bz2
cd CUnit-2.1-2
libtoolize -f -c -i \
  && aclocal \
  && autoconf \
  && autoheader \
  && automake --gnu --add-missing \
  && ./configure --prefix=/usr/local \
  && make \
  && make install
```

It would throw an error when build with CUnit latest source code(v2.1-3), since it requires the file `config.h.in` which does not exist in the latest package. We could copy it from the older package, such as from v2.1-2, if a latest version is required.

### install directory

It is better to install `CUnit` into directory which belongs to system path. Then, we could include and link it just as a standard C library in different hosts.

The recommended and default install directory in most cases is following:

- header files: `/usr/local/include/CUnit/*`

- library files: `/usr/local/lib/libcunit.a` and etc.

### cunit usage

For more details, see [CUnit Manual](http://cunit.sourceforge.net/doc/index.html).

## Mock

### Moco

[Moco](https://github.com/dreamhead/moco/) is an easy setup stub framework. It could help us to easily mock the response of OpenAPI(HTTP server).

- download: https://repo1.maven.org/maven2/com/github/dreamhead/moco-runner/1.1.0/moco-runner-1.1.0-standalone.jar;

- command: `java -jar moco-runner-<version>-standalone.jar http -p 12306 -c foo.json`;

- usage: https://github.com/dreamhead/moco/blob/master/moco-doc/usage.md;

## Coverage statistics

### install gcovr

- Mac:

``` bash
brew install gcovr
```

- Linux:

``` bash
sudo apt-get install -y gcovr
```

### gcovr usage

- change working directory to `test`: `cd test`

- build the project, for example `unit test`: `make ut`

- run the test cases, for example: `cd ut && ./ut.bin`

- execute gcovr:

  - Mac: `gcovr -r ../ --html --html-detail -o result.html`
  - Linux: `gcovr --html --html-detail -o result.html`

This would generate a html file named `result.html`, containing the coverage statistics.

For more details, see [Gcovr Guide](http://gcovr.com/guide.html).

## Memory leak check

``` bash
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --smc-check=all --undef-value-errors=no `shell command`
```

### issues

- `valgrind: Unrecognised instruction at address xxxxxxxx`
Upgrade valgrind to lastest version, you may build it with source code.

## Q&A when running i386 compiler on x86_64 platform

- `error while loading shared libraries: libstdc++.so.6`:

``` bash
sudo apt-get install libstdc++6
sudo apt-get install lib32stdc++6
```

- `run cross_compiler failed with message: no such file or directory`:

``` bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install ia32-libs
```

or

``` bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install lib32ncurses5 lib32z1
```

## Questions

- None.
