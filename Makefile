#
# Makefile
#
BIN_READ = pl20_read
BINDIR = /usr/local/sbin/
DESTDIR = /usr
PREFIX = /local
SERVICE = pl20d.service
SERVICEDIR = /etc/systemd/system

# modbus header files could be located in different directories
#INC += -I$(DESTDIR)/include/modbuspp -I$(DESTDIR)/include/modbus
#INC += -I$(DESTDIR)$(PREFIX)/include/modbuspp -I$(DESTDIR)$(PREFIX)/include/modbus

CC=gcc
CXX=g++
CFLAGS = -Wall -Wshadow -Wundef -Wmaybe-uninitialized -Wno-unknown-pragmas
CFLAGS += -O3 -g3 $(INC)

# directory for local libs
LDFLAGS = -L$(DESTDIR)$(PREFIX)/lib
#LIBS += -lstdc++ -lm -lmosquitto -lconfig++ -lmodbus
#LIBS += -lstdc++ -lm -lmosquitto
LIBS_READ += -lstdc++

#VPATH =

#$(info LDFLAGS ="$(LDFLAGS)")
#$(info INC="$(INC)")

# folder for our object files
OBJDIR = ./obj

CSRCS += $(wildcard *.c)
CPPSRCS += $(wildcard *.cpp)

COBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(CSRCS))
CPPOBJS = $(patsubst %.cpp,$(OBJDIR)/%.o,$(CPPSRCS))

SRCS = $(CSRCS) $(CPPSRCS)
OBJS = $(COBJS) $(CPPOBJS)

#.PHONY: clean

all: read

$(OBJDIR)/%.o: %.c
	@mkdir -p $(OBJDIR)
	@echo "CC $<"
	@$(CC)  $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)
	@echo "CXX $<"
	@$(CXX)  $(CFLAGS) -c $< -o $@

read: $(OBJS)
	$(CC) -o $(BIN_READ) $(OBJS) $(LDFLAGS) $(LIBS_READ)

default: $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LDFLAGS) $(LIBS)

#	nothing to do but will print info
nothing:
	$(info OBJS ="$(OBJS)")
	$(info DONE)


clean:
	rm -f $(OBJS)

install:
ifneq ($(shell id -u), 0)
	@echo "!!!! install requires root !!!!"
else
	install -o root $(BIN_READ) $(BINDIR)$(BIN_READ)
	@echo ++++++++++++++++++++++++++++++++++++++++++++
	@echo ++ $(BIN_READ) has been installed in $(BINDIR)
#	@echo ++ systemctl start $(BIN)
#	@echo ++ systemctl stop $(BIN)
endif

# make systemd service
service:
	install -o root $(SERVICE) $(SERVICEDIR)
	@systemctl daemon-reload
	@systemctl enable $(SERVICE)
	@echo $(BIN) is now available a systemd service
