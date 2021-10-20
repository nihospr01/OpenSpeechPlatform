EXAMPLE_FILE ?= read_sensor_data.c
TARGET_EXEC ?= read_sensor

API_LOCATION ?= ../..

C_SRCS += \
$(EXAMPLE_FILE) \
$(API_LOCATION)/osp_bmi160.c \
$(API_LOCATION)/bmi160.c

INCLUDEPATHS += \
$(API_LOCATION)

OBJS := $(C_SRCS:%=%.o)

CFLAGS	?= -Wall -O3

%.c.o: %.c
	$(CC) -I$(INCLUDEPATHS) -I../../.. $(CFLAGS) -c $< -o $@

$(TARGET_EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean

clean:
	$(RM) $(OBJS) $(TARGET_EXEC)