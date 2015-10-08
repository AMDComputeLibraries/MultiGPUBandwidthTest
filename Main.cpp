#include "InputFlags.h"
#include "OpenCLHelper.h"
#include "PCIeBandwidthTest.h"
#include <iostream>
#include <vector>
#include <cstdlib>

void ExtractGPUAndCPUList(std::vector<int> &num_gpu, std::vector<int> &cores, std::string gpu_list, std::string core_list)
{
	int i = 0;
	while(gpu_list[i] != '\0') {
		std::string temp;
		while(gpu_list[i] != ',' && gpu_list[i] != '\0') {
			temp += gpu_list[i];
			i++;
		}
		i++;
		num_gpu.push_back(atoi(temp.c_str()));
	}

	i = 0;
	while(core_list[i] != '\0') {
		std::string temp;
		while(core_list[i] != ',' && core_list[i] != '\0') {
			temp += core_list[i];
			i++;
		}
		i++;
		cores.push_back(atoi(temp.c_str()));
	}

	if(num_gpu.size() != cores.size()) {
		printf("Error: each GPU should be associated to a core !\n");
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	PCIeBandwidthTest pcie_obj;
	InputFlags &in_flags = pcie_obj;
	in_flags.AddDerivedInputFlags();
	in_flags.Parse(argc, argv);

	std::vector<int> num_gpu;
	std::vector<int> core_list;
	ExtractGPUAndCPUList(num_gpu, core_list, in_flags.GetValueStr("gpu_list"), in_flags.GetValueStr("cores"));

	CLHelper CL;
	if(CL.Init("", in_flags, num_gpu) == 1) {
		printf("Error Initializing OpenCL Runtime\n");
		exit(1);
	}

	pcie_obj.AllocateBuffers(CL, num_gpu.size(), core_list);

	int nsizes = in_flags.GetValueInt("sizes");
	int niters = in_flags.GetValueInt("iterations");

	for(int i = 0; i < nsizes; i++) {
		pcie_obj.RunTest(CL, niters, i, core_list);
	}

	pcie_obj.PrintResults();
}
