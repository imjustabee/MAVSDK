<img alt="MAVSDK" src="https://mavsdk.mavlink.io/main/assets/site/sdk_logo_full.png" width="400">

[![GitHub Actions Status](https://github.com/mavlink/MAVSDK/workflows/Build%20and%20Test/badge.svg?branch=main)](https://github.com/mavlink/MAVSDK/actions?query=branch%3Amain)
[![Coverage Status](https://coveralls.io/repos/github/mavlink/MAVSDK/badge.svg?branch=main)](https://coveralls.io/github/mavlink/MAVSDK?branch=main)

## Description

[MAVSDK](https://mavsdk.mavlink.io/main/en/) is a set of libraries providing a high-level API to [MAVLink](https://mavlink.io/en/).
It aims to be:
- Easy to use with a simple API supporting both synchronous (blocking) API calls and asynchronous API calls using callbacks.
- Fast and lightweight.
- Cross-platform (Linux, macOS, Windows, iOS, Android).
- Extensible (using compile-time plugins).
- Fully compliant with the MAVLink common standard/definitions.

In order to support multiple programming languages, MAVSDK implements a gRPC server in C++ which allows clients in different programming languages to connect to. The API is defined by the proto IDL ([proto files](https://github.com/mavlink/MAVSDK-Proto/tree/master/protos)).
This architecture allows the clients to be implemented in idiomatic patterns, so using the tooling and syntax expected by end users. For example, the Python library can be installed from PyPi using `pip`.

The MAVSDK C++ part consists of:
- The [core library](https://github.com/mavlink/MAVSDK/tree/main/src/core) implementing the basic MAVLink communication.
- The [plugin libraries](https://github.com/mavlink/MAVSDK/tree/main/src/plugins) implementing the MAVLink communication specific to a feature.
- The [mavsdk_server](https://github.com/mavlink/MAVSDK/tree/main/src/mavsdk_server) implementing the gRPC server for the language clients.

## Repos

- [MAVSDK](https://github.com/mavlink/MAVSDK) - this repo containing the source code for the C++ core.
- [MAVSDK-Proto](https://github.com/mavlink/MAVSDK-Proto) - Common interface definitions for API specified as proto files used by gRPC between language clients and mavsdk_server.
- [MAVSDK-Python](https://github.com/mavlink/MAVSDK-Python) - MAVSDK client for Python (first released on Pypi 2019).
- [MAVSDK-Swift](https://github.com/mavlink/MAVSDK-Swift) - MAVSDK client for Swift (used in production, first released 2018).
- [MAVSDK-Java](https://github.com/mavlink/MAVSDK-Java) - MAVSDK client for Java (first released on MavenCentral in 2019).
- [MAVSDK-JavaScript](https://github.com/mavlink/MAVSDK-JavaScript) - MAVSDK client in JavaScript (proof of concept, 2019).
- [MAVSDK-Rust](https://github.com/mavlink/MAVSDK-Rust) - MAVSDK client for Rust (proof of concept, 2019).
- [MAVSDK-CSharp](https://github.com/mavlink/MAVSDK-CSharp) - MAVSDK client for CSharp (proof of concept, 2019).
- [Other Repos](https://github.com/mavlink?q=MAVSDK) - Docs, examples, etc.

## Docs (Build instructions etc.)

Instructions for how to use the C++ library can be found in the [MAVSDK docs](https://mavsdk.mavlink.io/main/en/) (links to other programming languages can be found from the documentation sidebar).

Quick Links:

- [QuickStart](https://mavsdk.mavlink.io/main/en/cpp/#getting-started)
- [C++ API Overview](https://mavsdk.mavlink.io/main/en/cpp/#api-overview)
- [API Reference](https://mavsdk.mavlink.io/main/en/api_reference/)
- [Building the Library](https://mavsdk.mavlink.io/main/en/cpp/guide/build.html)
- [Examples](https://mavsdk.mavlink.io/main/en/examples/)
- [FAQ](https://mavsdk.mavlink.io/main/en/getting_started/faq.html)

## License

This project is licensed under the permissive BSD 3-clause, see [LICENSE.md](LICENSE.md).


## Build Instructions

First install XCode Command line tools:
```
xcode-select --install
```

And Homebrew for cmake. Once you have installed brew, you can install cmake using brew in the terminal:
```
brew install cmake
```

Next build the project with the following parameters:
```
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install -DBUILD_SHARED_LIBS=ON -DBUILD_MAVSDK_SERVER=ON -Bbuild/default -H.
```

Then we will install the package system wide, in order to be used by example functions:
```
sudo cmake --build build/default --target install
```

If you have already built before, be sure to remove the build folder before running another one:
```
rm -rf build/default
```

##Install Simulator for local testing

Get the PX4 flight stack source code using git (you may need to install git for your platform):
```
git clone https://github.com/PX4/Firmware.git --recursive
cd Firmware
```

Then we will install the PX4 environment:
```
ulimit -S -n 2048

brew tap PX4/px4
brew install px4-dev

# install required packages using pip3
python3 -m pip install --user pyserial empy toml numpy pandas jinja2 pyyaml pyros-genmsg packaging
# if this fails with a permissions error, your Python install is in a system path - use this command instead:
sudo -H python3 -m pip install --user pyserial empy toml numpy pandas jinja2 pyyaml pyros-genmsg packaging

# install gazebo simulator
brew install --cask xquartz
brew install px4-sim-gazebo
```

Make sure that you also install the QGroundControl local application from the following location:
- [QGC Install](https://docs.qgroundcontrol.com/master/en/getting_started/download_and_install.html#macOS)

Then you can run the simulator:
```
make px4_sitl gazebo
```


## Running an example script to test against simulator
Build the radio example:
```
cd examples/radio_calibrate/
mkdir build && cd build
cmake ..
make
```

Then run the example app (from the example/radio_calibrate/build directory) as shown:
```
./radio_calibrate udp://:14540
```

The MAVSDK application should connect to PX4, and you will be able to observe the example running in the SDK terminal, SITL terminal, and/or QGroundControl.

