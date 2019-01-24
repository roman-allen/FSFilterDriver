# FSFilterDriver

FSFilterDriver is a Windows file system driver which allows to block and unblock access to certain files at real-time. Blocking access on file system level allows to prevent any Windows service or application from opening a file for reading or writing even if they are owner of the file, running under Administrator rights or under "System" account. This could be useful for protecting sensitive files from undesired access and even for preventing Windows itself from changing certain files.

## Key features
* Driver has black and white lists of processes which can/cannot access blocked file
* List of files which driver should block can be changed at run-time
* Driver can be controlled via communication port
* Driver reports its activity via communication port to the connected application if any

## Project content
This project consists of 3 sub-projectes:
* FSFilterDriver	- file system driver written on pure C and compiles into installable .sys driver
* FSFilterDriverAPI	- .NET C# based assembly which allows to communicate with the driver and control it from any .NET application
* FSFilterDriverConsole	- .NET C# console application which allows to control the driver from command line

## Building
In order to build the driver and related projects you will need to have installed and configured the following tools:
* [Windows Driver Kit (WDK)](https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk)
* [Windows Software Development Kit (SDK)](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk)
* Microsoft (R) C/C++ Optimizing Compiler
* Microsoft Visual Studio 2015
* Microsft .NET Framework 4.0
The exact steps for DDK/SDK installation and configuration will depend on your particular OS version, SDK/DDK versions, other tools and environment settings.

## License
Copyright (c) 2019 Roman Allen

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

[http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0)

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.