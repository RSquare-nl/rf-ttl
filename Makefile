#
#
EXE = program-rf-ttl
SRCS = main.cpp

CXXFLAGS = -g -O2 -rdynamic
LDFLAGS = -lpthread

CXX = g++
OBJS = $(patsubst %.cpp,%.o,$(SRCS) )


# -------------

#all: clean $(EXE)
all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)
#	strip $(EXE)


clean:
	rm -f $(OBJS)
	rm -f .depend
	rm -r $(EXE)

install:
	cp $(EXE) $(PREFIX)/bin/


# automatically generate dependencies from our .cpp files
# (-MM tells the compiler to just output makefile rules)
depend:
	$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $(SRCS) > .depend

ifeq ($(wildcard .depend),.depend)
include .depend
endif


