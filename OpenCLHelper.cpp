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

#include "OpenCLHelper.h"
#include <cstring>
#include <string>
#include <iostream>

cl_context CLHelper::context = NULL;
cl_command_queue* CLHelper::commandQueue = NULL;

void convertToStr(char **source, size_t* sourceSize, const std::string fname)
{
	FILE *fp = fopen(fname.c_str(), "r");
	fseek(fp, 0, SEEK_END);
	*sourceSize = ftell(fp);
	fseek(fp , 0, SEEK_SET);
	*source = (char *)malloc(*sourceSize * sizeof(char));
	fread(*source, 1, *sourceSize, fp);
	fclose(fp);

}

int CLHelper::Init(const std::string &filename, 
		InputFlags &in_flags, 
		std::vector<int> gpu_list)
{
	cl_int status = 0;
    size_t deviceListSize;
	unsigned int i;

    /*
     * Have a look at the available platforms and pick either
     * the AMD one if available or a reasonable default.
     */
    cl_uint numPlatforms;
    platform = NULL;
    status = clGetPlatformIDs(0, NULL, &numPlatforms);
    if(status != CL_SUCCESS)
    {
        fprintf(stderr,"clGetPlatformIDs failed. %u",numPlatforms);
        return 1;
    }
    if (0 < numPlatforms) 
    {
		cl_platform_id* platforms = (cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id));
        status = clGetPlatformIDs(numPlatforms, platforms, NULL);
        if(status != CL_SUCCESS)
        {
            perror( "clGetPlatformIDs failed.2");
            return 1;
        }
        for (i = 0; i < numPlatforms; ++i) 
        {
            char pbuf[100];
            status = clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, sizeof(pbuf), pbuf, NULL);

            if(status != CL_SUCCESS)
            {
                perror("clGetPlatformInfo failed.");
                return 1;
            }

            platform = platforms[i];
            if (!strcmp(pbuf, "Advanced Micro Devices, Inc.")) 
            {
                break;
            }
        }
		free(platforms);
    }

    /////////////////////////////////////////////////////////////////
    // Create an OpenCL context
    /////////////////////////////////////////////////////////////////

    cl_context_properties cps[3] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0 };
    cl_context_properties* cprops = (NULL == platform) ? NULL : cps;
    context = clCreateContextFromType(cprops, CL_DEVICE_TYPE_GPU, NULL, NULL, &status);
    if(status != CL_SUCCESS)
    {
        printf("status: %d",  status);
        perror("Error: Creating Context. (clCreateContextFromType)");
        return 1;
    }
    /* First, get the size of device list data */
    status = clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES, sizeof(size_t), &deviceListSize, NULL);
    if(status != CL_SUCCESS)
    {
        perror("Error: Getting Context Info (device list size, clGetContextInfo)");
        return 1;
    }

    /////////////////////////////////////////////////////////////////
    // Detect OpenCL devices
    /////////////////////////////////////////////////////////////////
    devices = (cl_device_id *)malloc(deviceListSize * sizeof(cl_device_id));
    if(devices == 0)
    {
        perror("Error: No devices found.");
        return 1;
    }

    /* Now, get the device list data */
    status = clGetContextInfo( context, CL_CONTEXT_DEVICES, deviceListSize*sizeof(cl_device_id), devices, NULL);
    if(status != CL_SUCCESS)
    {
        perror("Error: Getting Context Info (device list, clGetContextInfo)");
        return 1;
    }

	int nGPUs = gpu_list.size();
	if(nGPUs > deviceListSize) {
		printf("Requested number of GPUs are not available ! \n");
		exit(1);
	}

	char deviceName[100];

	for(int i = 0; i < nGPUs; i++) {
	
		clGetDeviceInfo(devices[gpu_list[i]], CL_DEVICE_NAME, sizeof(deviceName), deviceName, NULL);
		cl_device_topology_amd devid;
		clGetDeviceInfo(devices[gpu_list[i]], CL_DEVICE_TOPOLOGY_AMD, sizeof(devid), &devid, NULL);

		printf("GPU: %d Name: %s PCIe: %d\n", i, deviceName, (int)devid.pcie.bus);
	}


    /////////////////////////////////////////////////////////////////
    // Create an OpenCL command queue
    /////////////////////////////////////////////////////////////////
	commandQueue = new cl_command_queue[nGPUs];
	for(int i = 0 ; i < nGPUs; i++) {
		commandQueue[i] = clCreateCommandQueue(context, devices[gpu_list[i]], CL_QUEUE_PROFILING_ENABLE, &status);
		if(status != CL_SUCCESS)
		{
			perror("Creating Command Queue. (clCreateCommandQueue) !");
			return 1;
		}
	}

	if(! filename.empty()) {
    /////////////////////////////////////////////////////////////////
    // Load CL file, build CL program object, create CL kernel object
    /////////////////////////////////////////////////////////////////
	char* source;
	size_t sourceSize;
	convertToStr(&source, &sourceSize, filename);
	//printf("Kernel %s\n", source);
    
    program = clCreateProgramWithSource(context, 1, (const char**)&source, &sourceSize, &status);
	if(status != CL_SUCCESS)
    {
        perror("Error: Loading Binary into cl_program (clCreateProgramWithBinary)");
        return 1;
    }

	std::string buildFlags = "-x clc++ -Dcl_khr_int64_base_atomics=1";
//	buildFlags += " -DROWBITS=" + std::to_string(ROW_BITS);
//	buildFlags += " -DWGBITS=" + std::to_string(WG_BITS);
//	buildFlags += " -DBLOCKSIZE=" + std::to_string(in_flags.GetValueInt("blocksize"));
//	buildFlags += " -DBLOCK_MULTIPLIER=" + std::to_string(in_flags.GetValueInt("blocksize_multiplier"));
//	buildFlags += " -DROWS_VECTOR=" + std::to_string(in_flags.GetValueInt("rows_vector"));
#ifdef DOUBLE
	buildFlags += " -DDOUBLE";
#endif
    
	/* create a cl program executable for all the devices specified */
    status = clBuildProgram(program, deviceListSize, devices, buildFlags.c_str(), NULL, NULL);
    if(status != CL_SUCCESS)
    {
        printf("Error: Building Program (clBuildProgram): %d", status);
        char * errorbuf = (char*)calloc(sizeof(char),1024*1024);
        size_t size;
        clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 1024*1024, errorbuf, &size);
        printf("%s ", errorbuf);
        return 1;
    }

		}
	
	// All good
	return 0;
}

void CLHelper::clCheckStatus(cl_int status, const std::string errString)
{
	if (status != CL_SUCCESS)
	{
		std::cout<<status<<", "<<errString<<"\n";
		exit(-1);
	}
}

cl_mem CLHelper::AllocateMem(const std::string name,
							size_t size, 
							cl_mem_flags flags, 
							void *hostBuffer) 
{
	cl_mem buf;
	cl_int status;

	buf = clCreateBuffer(context, flags, size, hostBuffer, &status);
	std::string errString = "OpenCL error allocating " + name + " !";
	clCheckStatus(status, errString);

	return buf;
}

void* CLHelper::MapMem(const std::string name,
							size_t size, 
							cl_map_flags flags,
							int num_gpu,
							cl_mem devBuffer) 
{
	void* buf;
	cl_int status;

	buf = clEnqueueMapBuffer(commandQueue[num_gpu], devBuffer, CL_TRUE, flags, 0, size, 0, NULL, NULL, &status);
	
	std::string errString = "OpenCL error mapping " + name + " !";
	clCheckStatus(status, errString);

	return buf;
}

void CLHelper::CopyToDevice(cl_mem devBuffer, 
								void *hostBuffer,
								size_t size,
								size_t offset,
								cl_bool blocking,
								int num_gpu,
								cl_event *ev)
{
	cl_int status;
	status = clEnqueueWriteBuffer(commandQueue[num_gpu], devBuffer, blocking, offset, size, hostBuffer, 0, NULL, ev);

	clCheckStatus(status, "OpenCL error copying data to device !");
}

void CLHelper::CopyToHost(cl_mem devBuffer, 
								void *hostBuffer,
								size_t size,
								size_t offset,
								cl_bool blocking,
								int num_gpu,
								cl_event *ev)
{
	cl_int status;
	status = clEnqueueReadBuffer(commandQueue[num_gpu], devBuffer, blocking, offset, size, hostBuffer, 0, NULL, ev);

	clCheckStatus(status, "OpenCL error copying data to device !");
}

int64_t CLHelper::ComputeTime(cl_event event)
{
	int64_t start_time, end_time;

	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(int64_t), &start_time, NULL);
	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(int64_t), &end_time, NULL);

	return end_time - start_time;
}
