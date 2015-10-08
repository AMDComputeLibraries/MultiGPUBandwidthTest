#include "PCIeBandwidthTest.h"
#include <cmath>
#include <time.h>
#include <cinttypes>
#include <thread>

PCIeBandwidthTest::PCIeBandwidthTest() : _usePinned(0), _nSizes(1), _nGPUs(1)
{
	_hostMem = NULL; _hostMemPinned = NULL; _deviceMem = NULL;
	_sizes = NULL;
}

void PCIeBandwidthTest::AddDerivedInputFlags()
{
	AddInputFlag("gpu_list", 'g', "1", "Comma-separated list of GPUs to use (Default=0)", "string");
	AddInputFlag("cores", 'c', "0", "Comma-separated list of CPU cores to use (Default=0)", "string");
	AddInputFlag("iterations", 'i', "10", "Number of Iterations (Default=10)", "int");
	AddInputFlag("sizes", 's', "2", "Number of different CL::Buffer sizes doubling from 256MB (Default=2)", "int");
	AddInputFlag("pinned", 'p', "1", "Use Pinned Memory (Default=1|Yes)", "int");
}

void PCIeBandwidthTest::AllocateBuffers(CLHelper &CL, int nGPUs, std::vector<int> core_list) 
{
	_nGPUs = nGPUs;
	_nSizes = GetValueInt("sizes");
	_usePinned = GetValueInt("pinned");

	_hostMem = new float*[_nGPUs];
	_hostMemPinned = new cl_mem[_nGPUs];
	_deviceMem = new cl_mem[_nGPUs];
	_sizes = new uint64_t[_nSizes];
	testTimes = new float[_nSizes * _nGPUs];

	for(int i = 0; i < _nSizes; i++)
		_sizes[i] = 256LL*1024LL * pow(2, i);

	std::thread *th = new std::thread[_nGPUs];

	for(int i = 0; i < _nGPUs; i++) {
		th[i] = std::thread(&PCIeBandwidthTest::ThreadAllocateAndInitBuffers, this, std::ref(CL), i, core_list);
	}
	for(int i = 0; i < _nGPUs; i++)
		th[i].join();

}
	
void PCIeBandwidthTest::ThreadAllocateAndInitBuffers(CLHelper &CL, int tid, std::vector<int> core_list)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	int core = core_list[tid];
	
	CPU_SET(core, &cpuset);
	cl_int cl_status;

	int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	if(ret != 0)
		printf("cannot bound thread %d to core\n", tid);

	uint64_t maxSize = _sizes[_nSizes-1]*1024LL;
	if(_usePinned) {
		_hostMemPinned[tid] = clCreateBuffer(CLHelper::context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, maxSize, NULL, &cl_status);
		CL.clCheckStatus(cl_status, "OpenCL error creating pinned buffer !");
		_hostMem[tid] = (float *)clEnqueueMapBuffer(CLHelper::commandQueue[tid], _hostMemPinned[tid], CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, maxSize, 0, NULL, NULL, &cl_status);
		CL.clCheckStatus(cl_status, "OpenCL error mapping buffer !");
	}
	else {
		_hostMem[tid] = new float[maxSize];
	}

	_deviceMem[tid] = clCreateBuffer(CLHelper::context, CL_MEM_READ_WRITE, maxSize, NULL, &cl_status);
	CL.clCheckStatus(cl_status, "OpenCL error creating buffer !");

	uint64_t numMaxFloats = _sizes[_nSizes-1]*1024LL / sizeof(float);
	for(int j; j < numMaxFloats; j++) {
			_hostMem[tid][j] = j % 77;
	}

	cl_status = clEnqueueWriteBuffer(CLHelper::commandQueue[tid], _deviceMem[tid], CL_TRUE, 0, maxSize, _hostMem[tid], 0, NULL, NULL);
	CL.clCheckStatus(cl_status, "OpenCL error writing to buffer (init call) !");
}

void PCIeBandwidthTest::RunTest(CLHelper &CL, int nPasses, int szID, std::vector<int> core_list)
{
	std::thread *th = new std::thread[_nGPUs];

	for(int i = 0; i < _nGPUs; i++) {
		th[i] = std::thread(&PCIeBandwidthTest::ThreadRunTest, this, std::ref(CL), i, nPasses, szID, core_list);
	}
	for(int i = 0; i < _nGPUs; i++)
		th[i].join();
}

void PCIeBandwidthTest::ThreadRunTest(CLHelper &CL, int tid, int nPasses, int szID, std::vector<int> core_list)
{
	struct timespec st_f, et_f;
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	int core = core_list[tid];
	CPU_SET(core, &cpuset);

	cl_int cl_status;
	int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	if(ret != 0)
		printf("cannot bound thread %d to core\n", tid);

	clock_gettime(CLOCK_MONOTONIC_RAW, &st_f);
	for(int i = 0; i < nPasses; i++) {
		cl_status = clEnqueueWriteBuffer(CLHelper::commandQueue[tid], _deviceMem[tid], CL_TRUE, 0, _sizes[szID]*1024LL, _hostMem[tid], 0, NULL, NULL);
		CL.clCheckStatus(cl_status, "OpenCL error writing to buffer (runtest) !");
	}
	clock_gettime(CLOCK_MONOTONIC_RAW, &et_f);
	float t_copy = ((et_f.tv_sec - st_f.tv_sec) + (et_f.tv_nsec - st_f.tv_nsec)*1e-9) / nPasses;

	testTimes[szID*_nGPUs + tid] = t_copy;
}

void PCIeBandwidthTest::PrintResults()
{
	printf("Size\t GPU\t Bandwidth\n");

	for(int i = 0; i < _nSizes; i++) {
		for(int j = 0; j < _nGPUs; j++) {
			int sz = _sizes[i]/1024;
			float bw = ((float)sz/1024LL)/testTimes[i*_nGPUs+j];
			printf("%d MB\t %d\t %f GB/s\n", sz, j, bw);
		}
	}
}

PCIeBandwidthTest::~PCIeBandwidthTest()
{
	if(_usePinned) {
		for(int i = 0; i < _nGPUs; i++) 
			clReleaseMemObject(_hostMemPinned[i]);
	}
	else {
		for(int i = 0; i < _nGPUs; i++) 
			delete [] _hostMem[i];
		delete[]  _hostMem;
	}

	for(int i = 0; i < _nGPUs; i++)
		clReleaseMemObject(_deviceMem[i]);

	delete[] _deviceMem;
	delete[] _sizes;
}
