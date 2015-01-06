NDK_PATH=$(shell dirname `which ndk-build`)
SDK_PATH=$(shell dirname `which android`)
NUMCPUS=$(shell grep -c '^processor' /proc/cpuinfo || echo "4" )

all: 210AV

210AV:
	$(NDK_PATH)/ndk-build IDBT_TARGET=S5PV210-AV -j$(NUMCPUS)
clean:
	$(NDK_PATH)/ndk-build clean
