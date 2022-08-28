ROOT_PATH=$(shell pwd)

TARGET=test
CXX=gcc
FLAGS= -Wall -Werror -g
SUBDIRS=$(ROOT_PATH)
INCLUDE=-I$(ROOT_PATH)

SER_SRC=$(shell ls $(SUBDIRS) | grep -E ".c")
SER_OBJ=$(SER_SRC:.c=.o)


.PHONY:all
all:$(TARGET)

$(TARGET):$(SER_OBJ)
	$(CXX) -o $(@) $(^) $(FLAGS)
%.o:$(ROOT_PATH)/%.c
	$(CXX) -c $(<) $(INCLUDE) $(FLAGS)

.PHONY:clean
clean:
	rm -rf $(TARGET) *.o

.PHONY:debug
debug:
	@echo $(ROOT_PATH)
	@echo $(SER_SRC)
	@echo $(SER_OBJ)
