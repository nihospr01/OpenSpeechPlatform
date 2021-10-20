EXAMPLE_FILE ?= gyro.c
TARGET_EXEC ?= gyro

API_LOCATION ?= ../../..
COMMON_LOCATION ?= ../common

C_SRCS += \
$(EXAMPLE_FILE) \
$(API_LOCATION)/bmi2.c \
$(API_LOCATION)/bmi270.c \
$(COMMON_LOCATION)/common.c

OBJS := $(C_SRCS:%=%.o)

CFLAGS	?= -Wall -O3

%.c.o: %.c
	$(CC) -I$(COMMON_LOCATION) -I../../.. $(CFLAGS) -c $< -o $@

$(TARGET_EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) 

.PHONY: clean

clean:
	$(RM) $(OBJS) $(TARGET_EXEC)
