CUDA_ARCH?=sm_20

variant: variant.cu variant.h uninitialized.h
	nvcc -arch=$(CUDA_ARCH) variant.cu -o variant