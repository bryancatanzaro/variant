CUDA_ARCH ?= sm_20

simple: simple.cu
	nvcc -arch=$(CUDA_ARCH) -I.. simple.cu -o simple

side_effect: side_effect.cu
	nvcc -arch=$(CUDA_ARCH) -I.. side_effect.cu -o side_effect

conversions: conversions.cu
	nvcc -arch=$(CUDA_ARCH) -I.. conversions.cu -o conversions