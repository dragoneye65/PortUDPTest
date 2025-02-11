
EXECUTABLE	:= portudptest

RELEASE		:= -O3
DEBUG		:= -ggdb3
STATIC		:= -Bstatic -static-libgcc -static-libstdc++
SHARED		:=

#BUILD		:= $(DEBUG)
BUILD		:= $(RELEASE)

LINKTYPE	:= $(STATIC)
#LINKTYPE	:= $(SHARED)

CXX_FLAGS 	:= -Wall -Wextra -std=c++20 $(BUILD) -fpermissive -Wtype-limits $(LINKTYPE)
CXX			:= g++
INC_FLAG 	:= -Iinc/asio
#LIB_FLAG	:= -Llib -Llib/SDL3/Release
LIB_FLAG	:= 

BIN			:= bin
SRC			:= src
INC			:= inc
LIB			:= lib
OBJ     	:= obj
RES			:= res

# these dirs don't get copyed if empty by git
DIRS		:= bin obj lib 

#LIBRARIES	:= -lGL -lGLEW -lSDL2 -lSOIL -lassimp
#LIBRARIES	:= -lGL -lX11 -lpthread -lpng -lpulse -lpulse-simple -lSDL3
LIBRARIES	:=


SOURCES		:= $(shell find $(SRC) -type f -name *.cpp)
OBJECTS     := $(patsubst $(SRC)/%,$(OBJ)/%,$(SOURCES:.cpp=.o))

DEPSRC		:= $(shell find $(INC) -type f -name *.hpp)
DEPENDENCIES:= $(DEPSRC:.hpp)

# if you want to find out the value of a makefile variable
# make print-VARIABLE  <--- VARIABLE is one defined here, like CXX_FLAGS, so type make print-CXX_FLAGS
print-%  : ; @echo $* = $($*)

.PHONY: depend clean all setup


all: $(BIN)/$(EXECUTABLE)

run: all
	./$(BIN)/$(EXECUTABLE)

clean:
	-rm $(BIN)/$(EXECUTABLE) $(OBJ)/*.o
#	-rm $(BIN)/* $(OBJ)/*.o

# Target to create directories
setup:
	@$(foreach dir,$(DIRS), \
		if [ ! -d $(dir) ]; then \
			echo "Creating directory $(dir)"; \
			mkdir -p $(dir); \
		fi; \
	)


# Compile only
$(OBJ)/%.o : $(SRC)/%.cpp $(DEPENDENCIES)
	$(CXX) $(CXX_FLAGS) $(INC_FLAG) -c -o $@ $<

# Link the object files and libraries
$(BIN)/$(EXECUTABLE) : $(OBJECTS)
	$(CXX) $(CXX_FLAGS) -o $(BIN)/$(EXECUTABLE) $^ $(LIBRARIES) $(LIB_FLAG)

