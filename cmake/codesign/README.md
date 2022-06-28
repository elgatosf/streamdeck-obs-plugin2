# Code Signing support for CMake
Adds code signing support to CMake build scripts via the use of several functions and programs.

## Usage:
```
codesign(CERTIFICATE 'certpath' PASSWORD 'password' TARGETS 'target[;target[;...]]')
```

#### CERTIFICATE
Required. Must hold the path to a valid certificate, or the build will fail.

#### PASSWORD
Required. Must hold the correct password for the certificate, or the build will fail.

#### TARGETS
A list of targets to enable code signing for.
