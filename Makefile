CXXFLAGS =	-O0 -g -Wall -fmessage-length=0 -I./include -DDEBUG

OUTPUT = Debug
ifeq ($(type),release)
OUTPUT = Release
endif

LIB_OBJS =  msgQueue.o wxMessageQueue.o
LIBS =      
LIBBASE =   tinymq
TARGET =    lib$(LIBBASE).a
TEST_CLIENT = Client.exe
TEST_SERVER = Server.exe
TEST_CPLUSPLUS  = Cplusplus.exe
TEST_INTEGRATED = Integrated.exe
TEST_PERFORMANCE = Performance.exe
TEST_STRESS = Stress.exe
TEST += $(TEST_CLIENT) $(TEST_SERVER) $(TEST_CPLUSPLUS)
TEST += $(TEST_INTEGRATED) $(TEST_PERFORMANCE) $(TEST_STRESS)
TEST_OBJ = $(foreach item, $(patsubst %.exe, %.o, $(TEST)),test$(item))

VPATH = src:test/inter-thread:test/inter-process:test/cplusplus

all: $(TARGET) $(TEST)

$(TARGET): $(LIB_OBJS)
	$(AR) cr $(TARGET) $(LIB_OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CXXFLAGS) -c $< -o $@

%.exe: test%.o
	$(CXX) -o $@ $< -L. -l$(LIBBASE)

clean:
	rm -f $(LIB_OBJS) $(TARGET) $(TEST) $(TEST_OBJ)
