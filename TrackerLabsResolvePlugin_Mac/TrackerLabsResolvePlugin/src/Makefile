UNAME_SYSTEM := $(shell uname -s)

RELEASE ?= 1

ifeq ($(RELEASE), 1)
    # Release: Optimization and no asserts
    BUILD_FLAGS = -O3 -DNDEBUG -fvisibility-inlines-hidden
else
    # Debug: Symbols and no optimization
    BUILD_FLAGS = -g -O0
endif

CXXFLAGS = -fvisibility=hidden -I../OpenFX-1.4/include -I../Support/include

CXXFLAGS += $(BUILD_FLAGS)

ifeq ($(UNAME_SYSTEM), Linux)
	AMDAPP_PATH ?= /opt/AMDAPP
	CXXFLAGS += -I${AMDAPP_PATH}/include -fPIC
	CUDAPATH ?= /usr/local/cuda
	NVCC = ${CUDAPATH}/bin/nvcc
	NVCCFLAGS = --compiler-options="-fPIC"
	LDFLAGS = -shared -fvisibility=hidden -L${CUDAPATH}/lib64 -lcuda -lcudart_static
	BUNDLE_DIR = TrackerLabsPlugin.ofx.bundle/Contents/Linux-x86-64/
	CUDA_OBJ = CudaKernel.o
else
	ARCH_FLAGS = -arch arm64 -arch x86_64
	CXXFLAGS += ${ARCH_FLAGS}
	LDFLAGS = -bundle -fvisibility=hidden -F/Library/Frameworks -framework OpenCL -framework Metal -framework AppKit -framework Foundation
	LDFLAGS += ${ARCH_FLAGS}
	BUNDLE_DIR = TrackerLabsPlugin.ofx.bundle/Contents/MacOS/
	RES_DIR = TrackerLabsPlugin.ofx.bundle/Contents/Resources/
	METAL_OBJ = MetalKernel.o
endif

TrackerLabsPlugin.ofx: auth.o activation.o TrackerLabsResolvePlugin.o $(METAL_OBJ) OpenCLKernel.o ofxsCore.o ofxsImageEffect.o ofxsInteract.o ofxsLog.o ofxsMultiThread.o ofxsParams.o ofxsProperty.o ofxsPropertyValidation.o
	$(CXX) $^ -o $@ $(LDFLAGS)
	mkdir -p $(BUNDLE_DIR)
	cp TrackerLabsPlugin.ofx $(BUNDLE_DIR)
	mkdir -p $(RES_DIR)
	cp TrackerLabs.png $(RES_DIR)

auth.o: auth.mm
	$(CXX) -c $< $(CXXFLAGS)
	
MetalKernel.o: MetalKernel.mm
	$(CXX) -c $< $(CXXFLAGS)

%.o: ../Support/Library/%.cpp
	$(CXX) -c $< $(CXXFLAGS)

clean:
	rm -f *.o *.ofx
	rm -fr TrackerLabsPlugin.ofx.bundle

install: TrackerLabsPlugin.ofx
	cp -fr TrackerLabsPlugin.ofx.bundle /Library/OFX/Plugins
