include makefile.macro

ifndef SRC_PATH
    SRC_PATH=.
endif
ifndef LIB_PATH
    LIB_PATH=lib
endif
ifndef OUT_PATH
    OUT_PATH=../bin
endif
ifndef VERSION
    VERSION=0.0.1
endif
ifndef PREFIX
	PREFIX=/usr/local
endif

$(shell mkdir -p $(LIB_PATH))

SRC=$(wildcard $(SRC_PATH)/*.cpp)
OBJ+=$(foreach n,$(SRC),$(LIB_PATH)/$(notdir $(subst .cpp,.o,$(n))))

ifneq ($(DEBUG), 0)
    PARAM+= -g -D_DEBUG
endif
TARGET=$(OUT_PATH)/sipsvc


all:$(TARGET)

$(TARGET):$(OBJ)
	$(LD) -o $@  $?  $(LD_FLAG)  $(SHARED_LIBS) $(STATIC_LIBS)  $(PJ_SHARED_LIBS)

$(OBJ):$(SRC)
	$(CC) $(PARAM) -o $@ $(SRC_PATH)/$(notdir $*).cpp $(CC_FLAG)

clean:
	$(RM) -vf $(TARGET)
	$(RM) -vf $(OBJ)
