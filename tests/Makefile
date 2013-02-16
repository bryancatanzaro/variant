CUDA_ARCH ?= sm_20

simple: simple.cu
	nvcc -arch=$(CUDA_ARCH) -I.. simple.cu -o simple