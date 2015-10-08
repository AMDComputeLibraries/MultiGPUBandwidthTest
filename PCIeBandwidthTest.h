#include "InputFlags.h"
#include "OpenCLHelper.h"
#include <utility>
#include <vector>
#include <sched.h>
#include <pthread.h>

class PCIeBandwidthTest : public InputFlags
{
	private:
		float **_hostMem;
		cl_mem *_hostMemPinned;
		cl_mem *_deviceMem;
		uint64_t *_sizes;
		int _usePinned;
		int _nSizes;
		//vector of _nSizes and pairs of <num_gpu, time>
		float *testTimes;
		int _nGPUs;
		

	public:
		PCIeBandwidthTest(); 

		void AddDerivedInputFlags();
		void AllocateBuffers(CLHelper &CL, int, std::vector<int>);
		void ThreadAllocateAndInitBuffers(CLHelper &CL, int tid, std::vector<int>);
		void RunTest(CLHelper &CL, int nPasses, int szID, std::vector<int>);
		void ThreadRunTest(CLHelper &CL, int tid, int nPasses, int szID, std::vector<int>);
		void PrintResults();

		~PCIeBandwidthTest();
};


