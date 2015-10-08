#ifndef CLHelper_H
#define CLHelper_H

/*
Copyright © 2014 Advanced Micro Devices, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following are met:

You must reproduce the above copyright notice.

Neither the name of the copyright holder nor the names of its contributors may
be used to endorse or promote products derived from this software without
specific, prior, written permission from at least the copyright holder.

You must include the following terms in your license and/or other materials
provided with the software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Without limiting the foregoing, the software may implement third party
technologies for which you must obtain licenses from parties other than AMD.
You agree that AMD has not obtained or conveyed to you, and that you shall be
responsible for obtaining the rights to use and/or distribute the applicable
underlying intellectual property rights related to the third party
technologies.  These third party technologies are not licensed hereunder.

If you use the software (in whole or in part), you shall adhere to all
applicable U.S., European, and other export laws, including but not limited to
the U.S. Export Administration Regulations (EAR) (15 C.F.R Sections
730-774), and E.U. Council Regulation (EC) No 428/2009 of 5 May 2009.  Further,
pursuant to Section 740.6 of the EAR, you hereby certify that, except pursuant
to a license granted by the United States Department of Commerce Bureau of
Industry and Security or as otherwise permitted pursuant to a License Exception
under the U.S. Export Administration Regulations ("EAR"), you will not (1)
export, re-export or release to a national of a country in Country Groups D:1,
E:1 or E:2 any restricted technology, software, or source code you receive
hereunder, or (2) export to Country Groups D:1, E:1 or E:2 the direct product
of such technology or software, if such foreign produced direct product is
subject to national security controls as identified on the Commerce Control
List (currently found in Supplement 1 to Part 774 of EAR).  For the most
current Country Group listings, or for additional information about the EAR
or your obligations under those regulations, please refer to the U.S. Bureau
of Industry and Securitys website at http://www.bis.doc.gov/.
*/

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include "InputFlags.h"

#define ROW_BITS 32 // May be not the right place to define this macro
#define WG_BITS 24

struct LocalMemArg 
{
	LocalMemArg(size_t _size) : size(_size) {}
	size_t GetSize() const { return size; }
	
	private:
	size_t size;
};

class CLHelper
{
	cl_platform_id platform;
	cl_device_id *devices;
	cl_program program;

	public:
	static cl_context context;
	static cl_command_queue *commandQueue;

	CLHelper() {}
	int Init(const std::string &_filename, InputFlags &in_flags, std::vector<int>gpu_list);
	void clCheckStatus(cl_int status, const std::string errString);
	void CopyToDevice(cl_mem _d_buf, void *_h_buf, size_t _size, size_t _offset, cl_bool _blocking, int num_gpu, cl_event *_ev);
	void CopyToHost(cl_mem _d_buf, void *_h_buf, size_t _size, size_t _offset, cl_bool _blocking, int num_gpu, cl_event *_ev);
	cl_mem AllocateMem(const std::string name, size_t, cl_mem_flags flags, void *);
	void* MapMem(const std::string name, size_t, cl_map_flags flags, int num_gpu, cl_mem);

	template<typename T, typename... Args>
	void SetArgs(cl_kernel, int i, const T& first, const Args&... rest);
	template<typename... Args>
	void SetArgs(cl_kernel, int i, const LocalMemArg &lmem, const Args&... rest);
	void SetArgs(cl_kernel, int i) {}

	int64_t ComputeTime(cl_event event);

};

template<typename T, typename... Args>
void CLHelper::SetArgs(cl_kernel kernel, int i, const T& first, const Args&... rest)
{
	cl_int status;

	status = clSetKernelArg(kernel, i++, sizeof(T), (void *)& first);
	std::stringstream errStream;
	errStream<<"OpenCL error setting kernel argument "<<i;
	clCheckStatus(status, errStream.str()) ;
	
	SetArgs(kernel, i, rest...);
}

template<typename... Args>
void CLHelper::SetArgs(cl_kernel kernel, int i, const LocalMemArg &lmem, const Args&... rest)
{
	cl_int status;
	status = clSetKernelArg(kernel, i++, lmem.GetSize(), NULL);
	std::stringstream errStream;
	errStream<<"OpenCL error setting kernel argument (local memory) "<<i;
	clCheckStatus(status, errStream.str()) ;
	
	SetArgs(kernel, i, rest...);

}

#endif //CLHelper_H

