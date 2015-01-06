
LOCAL_PATH:= $(call my-dir)/../../polarssl/library/
include $(CLEAR_VARS)

LOCAL_MODULE := libpolarss

LOCAL_CFLAGS := -fPIC -O2 -D_FILE_OFFSET_BITS=64 -Wall -W -Wdeclaration-after-statement

LOCAL_ARM_MODE := arm
ASM := .asm.s


### vpx_mem.mk

LOCAL_SRC_FILES :=	\
	aes.c		\
	arc4.c        \
	asn1parse.c   \
	asn1write.c   \
	base64.c      \
	bignum.c      \
	blowfish.c    \
	camellia.c    \
	certs.c       \
	cipher.c      \
	cipher_wrap.c \
	CMakeLists.txt\
	ctr_drbg.c    \
	debug.c       \
	des.c         \
	dhm.c         \
	entropy.c     \
	entropy_poll.c\
	error.c       \
	gcm.c         \
	havege.c      \
	Makefile      \
	md2.c         \
	md4.c         \
	md5.c         \
	md.c          \
	md_wrap.c     \
	net.c         \
	padlock.c     \
	pbkdf2.c      \
	pem.c         \
	pkcs11.c      \
	rsa.c         \
	sha1.c        \
	sha2.c        \
	sha4.c        \
	ssl_cache.c   \
	ssl_cli.c     \
	ssl_srv.c     \
	ssl_tls.c     \
	timing.c      \
	version.c     \
	x509parse.c   \
	x509write.c   \
	xtea.c 


LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include

LOCAL_STATIC_LIBRARIES += cpufeatures

include $(BUILD_STATIC_LIBRARY)
$(call import-module,android/cpufeatures)
