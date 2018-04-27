# KOMon

Kernel-based online monitoring of VNF packet processing times

---

KOMon is a tool developed to characterize and monitor the packet processing times of softwareized network functions by performing in-stack monitoring. In-stack monitoring leverages the network stack of the platform it is deployed on to monitor packets with high accuracy and low overhead.

---

This repository contains the codebase of KOMon itself, the dummy network function used during development and evaluation of the tool as well as the measurement data obtained during the evaluation process.

---

The repository is structured as follows.

`/KOMon/` contains the Kernel patch required to build and load the KOMon Kernel module. `/KOMon/src/` contains the source code of the Kernel module itself.

`/example_vnf/` contains the codebase of the example VNF used during the evaluation, Makefile included.

`/scripts/` contains the user space scripts to control the measurement, trigger monitoring intervals and so on.

`/data/` contains the measurement data our evaluation is based on. Thereby files named `KERNEL_*` contain values obtained using the KOMon tool, files named `VNF_*` contain values reported by the VNF and are seen as the groundtruth. Finally, files named `BUFFER_*` contain the fill level of the socket buffer at each monitoring point.

---

## Installation of KOMon

The KOMon monitoring tool requires a modified version of Linux Kernel 4.11. The patch that needs to be applied to the mainline Kernel is included in the `/KOMon/` folder in this repository. After the patched Kernel is installed, the Kernel module can be loaded just like any other loadable module.

```
cd ./KOMon
make
sudo insmod udp_probe.ko debug=0 port=1234
```

Available parameters during load time are

* `debug` to set the debug level. Note that most debug information is commented out in the code by default to minimize clock cycles spend in the monitoring logic. If debugging or working with the code, consider removing the commends, thereby activating the debug output code.
* `cbsize` to set the maximum sample size
* `port` to define the port for packet filtering. Thereby, the port defines the port used by the VNF to receive packets and sends packet to. Meaning incoming and outgoing packets need to have the specified destination port.

The module can be unloaded via `sudo rmmod udp_probe`.

## Usage of KOMon

In order to use the scripts provided in the `/scripts/` folder, the module needs to be loaded and the example VNF needs to be compiled and its binary be placed inside the `/example_vnf/` folder. Otherwise, the scripts might need slight modifications.

Afterwards, a monitoring process can be initiated using `run_test_all.sh` with its first parameter being the path to a parameter file. The script will then execute all tests defined in the parameter file, creating three result files (`VNF_*`, `KERNEL_*`, `BUFFER_*`) for each parameter combination.

A single parameter combination can be executed using the `run_test.sh` script with the parameters following in the same order as provided in the parameter file.

An example parameter file can be found in `/scripts/params/` and has the following format

```
1000 1 0.1 1000 128 0
1000 1 0.1 1000 128 100
1000 1 0.1 1000 128 200
1000 1 0.1 1000 128 300
1000 1 0.1 1000 128 400
1000 1 0.1 1000 128 500
```

Thereby, the order of parameters is

1. Number of monitoring intervals
2. Number of packets to sample per interval
3. Sleep between intervals
4. Current load in packets per second
5. Packet size in byte
6. Artificial delay to induce by the example VNF

## Usage of KOMon without included scripts

In order to interface with the KOMon Kernel module, it exposes an interface via procfs: `/proc/vnfinfo_udp`.

The user is then able to issue commands be echoing values into the interface file. Available commands are as follows.

* `start` starts a new monitoring interval which samples the specified number of packets
* `stop` interrupts the current interval, discarding all already sampled data
* `reset_packets` clears the ring buffer used to store packets currently in the system. This should not be required unless for debugging purposes.
* `reset_proc` clears the ring buffer used to store calculated processing times. Also only for debugging purposes.
* `<int>` - Echoing a valid integer of up to `cbsize` configures the samplesize and thereby the packets monitored in each interval
* `-1` resets the samplesize so that `samplesize = cbsize`

## Contact

For any questions regarding the code please contact [Stefan Geissler](mailto:stefan.geissler@informatik.uni-wuerzburg.de)

