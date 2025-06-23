[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://www.apache.org/licenses/LICENSE-2.0)

# LISA (Linked Interactive Source Automator)

LISA is a client server architecture application based on C++, designed to provide linked interactive source code automation functionality. This project consists of two main modules: client and server, supporting core functions such as Git integration and compilation processing.

##Project Structure

```
LISA/
∝ - Client/# Client module
│ ├── CMakeLists.txt
│∝ - check_denewing_git. cpp/. h # Git update check function
│∝ - compilationconfig. cpp/. h # Compile Configuration Management
│∝ - local_to-userver_git.cpp/. h # Git synchronization from local to server
│└ - main.cpp # Client Entrance
∝ - Server/# Server Module
│ ├── CMakeLists.txt
│∝ - compilation_ handler. cpp/. h # Compilation processing logic
│∝ - config. cpp/. h # Server Configuration
│∝ - git_ handler. cpp/. h # Git Operation Processing
│∝ - logger.cpp/. h # Logging System
│∝ - server. cpp/. h # Server Core Logic
│└ - main.cpp # Server Entrance
└ -- README.md # Project Document
```

##Functional Features

-Git integration: supports local and server code synchronization, update checking
-Compilation management: provides compilation configuration and processing functions
-Client server architecture: separated client and server modules
-Logging System: Comprehensive Logging Functionality

##Environmental requirements

-C++compiler (supports C++11 and above standards)
-CMake 3.10 or higher version
- Git
-MacOS operating system (project development environment)

##Installation and compilation

### 1. Clone project
```bash
Git clone<project repository URL>
cd LISA
```
### 2. Compile the Server
```bash
cd Server
mkdir build && cd build
cmake ..
make
```
### 3. Compile the Client
```bash
cd ../Client
mkdir build && cd build
cmake ..
make
```
## Usage

### Start the Server
```bash
cd Server/build
./server
```
### Run the Client
```bash
cd Client/build
./client
```
