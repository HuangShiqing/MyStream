CROSS_COMPILE =#arm-linux-gnueabi-#aarch64-linux-gnu-#指定交叉编译器
DEBUG = 1#指定当前为debug模式
MY_NET_DIR = /home/hsq/DeepLearning/code/MyNet
MNN_DIR = /home/hsq/DeepLearning/clone/MNN

CC = $(CROSS_COMPILE)gcc#指定编译器
CCFLAGS = -Wall -fPIC
#指定头文件目录
CCFLAGS += -I./ -I../common -I/usr/include/gstreamer-1.0 -I/usr/lib/arm-linux-gnueabihf/gstreamer-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include
#指定库文件目录
LDFLAGS = -L/usr/lib/arm-linux-gnueabihf/
#指定库文件名称
LIBS = -lstdc++ -lm -lgstreamer-1.0 -lgstbase-1.0 -lgobject-2.0 -lglib-2.0 -lgmodule-2.0 -lwiringPi
#告诉makefile去哪里找依赖文件和目标文件
VPATH = ./:../common/

#选择debug还是release
ifeq ($(DEBUG), 1)
CCFLAGS+=-O0 -g
else
CCFLAGS+=-Ofast
endif

#存放.o文件的文件夹
OBJDIR = ../../build/gst-motor/obj/
SODIR = ../../build/gst-motor/so/
#中间过程所涉及的.o文件	
OBJ = metadata.o gstmotor.o utils.o
OBJS = $(addprefix $(OBJDIR), $(OBJ))#添加路径
EXAMPLE_OBJ = 
EXAMPLE_OBJS = $(addprefix $(OBJDIR), $(EXAMPLE_OBJ))#添加路径

#最终生成的可执行文件名
# EXAMPLE = rfb320_example
DLIB = $(SODIR)libgstmotor.so
SLIB = $(SODIR)libgstmotor.a

#指定需要完成的编译的对象
all: obj $(DLIB) $(SLIB) $(EXAMPLE) $(CC_EXAMPLE)

#将所有的.o文件链接成最终的可执行文件，需要库目录和库名，注意，OBJS要在LIBS之前。另外，如果要指定.o的生成路径，需要保证EXAMPLE的依赖项是含路径的
$(EXAMPLE):$(EXAMPLE_OBJS) $(SLIB)
		$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) $(SLIB)
$(SLIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^
#这个LIBS一定要放在最后或者某个东西后面, 不然不会链接进动态库
$(DLIB): $(OBJS)
	$(CXX) $(CXXFLAGS) -shared  $^ -o $@ $(LDFLAGS) $(LIBS)
#这个不是静态模式，而是通配符，第一个%类似bash中的*
$(OBJDIR)%.o: %.c
		$(CC) -c $(CCFLAGS) $< -o $@

#用于生成存放.o文件的文件夹
obj:
	mkdir -p ../../build/gst-motor/obj
	mkdir -p ../../build/gst-motor/so
.PHONY : 
	clean
clean :#删除生成的文件夹
	rm -rf ../../build/gst-motor $(EXAMPLE) $(CC_EXAMPLE) $(DLIB) $(SLIB)
