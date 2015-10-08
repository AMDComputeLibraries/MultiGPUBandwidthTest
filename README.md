# MultiGPUBandwidthTest

The purpose of this micro-benchmark is to measure the PCIe Bandwidth (Download)
in a Multi-GPU setup using both pinned and non-pinned memory.

The benchmark allows you to specify the GPUs as well as the CPU cores to use.
Both CPUs and GPUs are required to be specified using a comma-separated list.
This allows to understand the NUMA effects in a multi-socket system. clinfo and
cat /proc/cpuinfo allow you to understand the topology of GPUs.

--cores              -c        Comma-separated list of CPU cores to use (Default=0)
--gpu_list           -g        Comma-separated list of GPUs to use (Default=0)
--help               -h        Print Help Message
--iterations         -i        Number of Iterations (Default=10)
--pinned             -p        Use Pinned Memory (Default=1|Yes)
--sizes              -s        Number of different CL::Buffer sizes doubling from 256MB (Default=2)

Examples:
1. Use default GPU (1st GPU) and CPU core
./PCIeBandwidth

2. Use GPUs 0 and 1 and CPU cores 0 and 2
./PCIeBandwidth -g 0,1 -c 0,2

3. Use GPUs 0-7 and CPU cores 0-3, 8-11
./PCIeBandwidth -g 0,1,2,3,4,5,6,7 -c 0,1,2,3,8,9,11,12
