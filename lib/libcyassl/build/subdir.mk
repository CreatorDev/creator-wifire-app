DIR__SRC=..
SRC=$(addprefix $(DIR__SRC)/,	\
	crl.c 						\
	internal.c 					\
	io.c 						\
	keys.c 						\
	ocsp.c 						\
	sniffer.c 					\
	ssl.c 						\
	tls.c 						\
	ctaocrypt/src/aes.c  		\
	ctaocrypt/src/arc4.c		\
	ctaocrypt/src/asm.c			\
	ctaocrypt/src/asn.c			\
	ctaocrypt/src/blake2b.c		\
	ctaocrypt/src/camellia.c	\
	ctaocrypt/src/coding.c		\
	ctaocrypt/src/compress.c	\
	ctaocrypt/src/des3.c		\
	ctaocrypt/src/dh.c			\
	ctaocrypt/src/dsa.c			\
	ctaocrypt/src/ecc.c			\
	ctaocrypt/src/ecc_fp.c		\
	ctaocrypt/src/error.c		\
	ctaocrypt/src/hc128.c		\
	ctaocrypt/src/hmac.c		\
	ctaocrypt/src/integer.c		\
	ctaocrypt/src/logging.c		\
	ctaocrypt/src/md2.c			\
	ctaocrypt/src/md4.c			\
	ctaocrypt/src/md5.c			\
	ctaocrypt/src/memory.c		\
	ctaocrypt/src/misc.c		\
	ctaocrypt/src/port.c		\
	ctaocrypt/src/pwdbased.c	\
	ctaocrypt/src/rabbit.c		\
	ctaocrypt/src/random.c		\
	ctaocrypt/src/ripemd.c		\
	ctaocrypt/src/rsa.c			\
	ctaocrypt/src/sha256.c		\
	ctaocrypt/src/sha512.c		\
	ctaocrypt/src/sha.c			\
	ctaocrypt/src/tfm.c			\
)

C_SRC:=$(filter %.c,$(SRC))
C_OBJ:=$(patsubst $(DIR__SRC)/%.c,$(DIR__OBJ)/%.o,$(C_SRC))
C_DEP:=$(C_OBJ:.o=.d)

OBJS+=$(C_OBJ)

ifeq ($(DEBUG),)
CFLAGS:= \
	-DPOSIX=1 -DFLOW_DEBUG=1 -DFLOW_CONFIG_CYASSL=1 -DUSE_BUILTIN_HASH=1 \
	-Os -Wall \
	-fmessage-length=0 -fPIC -MMD -MP
else
CFLAGS:= \
	-DPOSIX=1 -DFLOW_DEBUG=1 -DFLOW_CONFIG_CYASSL=1 -DUSE_BUILTIN_HASH=1 \
	-O0 -g3 -Wall \
	-fmessage-length=0 -fPIC -MMD -MP
endif

# Each subdirectory must supply rules for building sources it contributes
$(DIR__OBJ)/%.o: $(DIR__SRC)/%.c | $(DIR__OBJ)
	@echo 'Building file: $<'
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I"$(DIR__SRC)/include" -I"$(DIR__SRC)" -c -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
