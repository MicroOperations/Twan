# Twan Separation Kernel

A lightweight separation kernel with dynamic routing capabilities

## Overview

Twan is a Separation Kernel designed specifically for adaptability, real-time computing, and mixed-criticality workloads. It consists of two distinct components:

- **Twanvisor**: A hypervisor that leverages hardware assisted virtualisation to isolate partitions. Guests are paravirtualised and use API's provided by Twanvisor.
- **TwanRTOS**: Root partition operating system that initializes Twanvisor, manages resources, and controls partition policies.

> **Note**: Twan is still in early stages of development.

## Support

Twan currently has support for Intel processors with: 

- **VT-x** 
- **EPTs** 
- **Unrestricted guest**

> **Note**: Even if your hardware fits these requirements, be aware that it may not work out of the box.

## Compliance

Twan is **not currently ARINC 653 or SKPP compliant** out of the box. Certification requires formal verification, safety analysis, and extensive validation beyond the current scope.

## Documentation

- [Getting started](doc/getting_started.md)
- [Demos](doc/demos/)

## Acknowledgements

- [uACPI](https://github.com/uACPI/uACPI)
- [Concurrency Kit](https://github.com/concurrencykit/ck)

## References 

- [Muen](https://github.com/codelabs-ch/muen) 
- [L4re/Fiasco](https://github.com/kernkonzept/fiasco)
- [Composite](https://github.com/gwsystems/composite)
- [Nova](https://github.com/udosteinberg/NOVA)
- [Xen](https://github.com/xen-project/xen)
- [POK](https://github.com/pok-kernel/pok)
- [XtratuM](https://www.fentiss.com/xtratum/)
- [Deos](https://isss-tvc.org/200122_Gilliland_Deos_RTOS.pdf)
- [EURO-MILS MILS Architecture Whitepaper](https://euromils-project.technikon.com/downloads/2014-EURO-MILS-MILS-Architecture-white-paper.pdf)
- [Seperation Kernel Protection Profile](https://www.commoncriteriaportal.org/files/ppfiles/pp_skpp_hr_v1.03.pdf)

## License

Twan is licensed under MIT

```
MIT License

Copyright (c) 2026 Humza Khan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```