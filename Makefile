#-std=c99: restrict, 但c99里面没有strdup函数
CC=arm-fsl-linux-gnueabi-gcc
CFLAGS += -I/opt/arm-alsa/include
LDFLAGS += -L/opt/arm-alsa/lib
LIBS += -lasound -lpthread

TARGET +=play record
TARGET +=wav2pcm
TARGET +=loopback

all:$(TARGET)
play:play.o audio.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)
record:record.o audio.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)
loopback:loopback.o audio.o thread.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)
wav2pcm:wav2pcm.o
	$(CC) -o $@ $^ 
%.o:%.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm *.o -rf
	rm $(TARGET) -rf
