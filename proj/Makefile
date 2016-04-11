CC=arm-linux-gcc
AR=arm-linux-ar


PPPDD=pppdd
LIB_NAME=libimxv4l2.so
LIB_NAME_STATIC=libimxv4l2.a
OBJS +=	input.o output.o sys_if_dev.o nvram.o common.o params.o proto.o

all: $(PPPDD) $(LIB_NAME) nvram_cli $(LIB_NAME_STATIC)
	$(CC) -o v4l2_uvc main.c $(LIB_NAME_STATIC) -lpthread

$(PPPDD):pppdd.c
	$(CC) -o $@ $? -lpthread

$(LIB_NAME):$(OBJS)
	$(CC) -fPIC -shared -o $@ $? -lpthread
$(LIB_NAME_STATIC):$(OBJS)
	$(AR) rc $@ $?
%.o:%.c
	$(CC) -o $@ -c $^

nvram_cli:
	$(CC) -o $@ nvram_cli.c nvram.c
clean:
	rm -rf *.o *.so *.la *.a
	rm -rf $(PPPDD)
	rm -rf nvram_cli
