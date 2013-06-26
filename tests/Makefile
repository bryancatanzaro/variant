CUDA_ARCH ?= sm_20
THRUST_DIR = /home/bcatanzaro/thrust


simple: simple.cu
	nvcc -arch=$(CUDA_ARCH) -I.. -I$(THRUST_DIR) simple.cu -o simple

side_effect: side_effect.cu
	nvcc -arch=$(CUDA_ARCH) -I.. side_effect.cu -o side_effect

conversions: conversions.cu
	nvcc -arch=$(CUDA_ARCH) -I.. conversions.cu -o conversions

nested: nested.cu
	nvcc -arch=$(CUDA_ARCH) -I.. nested.cu -o nested