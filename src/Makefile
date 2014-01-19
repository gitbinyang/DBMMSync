DFLAGS = -g
CXX=g++ 
PG_INC=/usr/include/postgresql 
LIBS=-lpq -lspread -ldl -L/usr/local/lib -L/usr/postgresql-9.1/lib
CFLAGS=-O2 -I../include -I${PG_INC} -I. -Wall -fPIC
OBJS=ReplMain.o Config.o Pumping.o FileMonitor.o SpreadMonitor.o ClntSockMonitor.o SrvSockMonitor.o Utils.o SQLloader.o PGSQLloader.o MsgProto.o
BIN=DBMMSync
all: main
.cpp.o:
	$(CXX)  $(DFLAGS) $(CFLAGS) -c $<

%.o : %.cpp
	$(CXX) $(DFLAGS)  $(CFLAGS) -c $<

main: $(OBJS)
	$(CXX) $(CFLAGS) $(DFLAGS) -o $(BIN) $(OBJS) $(LIBS)

clean:
	rm -f $(BIN) *.o *~ *.a *.so core.* *.gch

build: clean all
