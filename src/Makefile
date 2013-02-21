CC=g++
CFLAGS=-g -Wall
LIBS=-lgdi32 -lComdlg32 -lws2_32
SERVER_SOURCES=server-main.cpp server-net.cpp server-gui.cpp
SERVER_OBJECTS=obj/client-res.o $(addprefix obj/,$(SERVER_SOURCES:.cpp=.o))

CLIENT_SOURCES=client-main.cpp client-net.cpp client-gui.cpp 
CLIENT_OBJECTS=obj/client-res.o $(addprefix obj/,$(CLIENT_SOURCES:.cpp=.o))

all: ../server.exe ../client.exe

../server.exe: $(SERVER_OBJECTS) 
	@echo "[50%] Compile and link server..."
	@$(CC) $(CFLAGS) $(SERVER_OBJECTS) $(LIBS) $(W) -o $@

../client.exe: $(CLIENT_OBJECTS) 
	@echo "[75%] Compile and link client..."
	@$(CC) $(CFLAGS) $(CLIENT_OBJECTS) $(LIBS) $(W) -o $@

obj/client-res.o : resource.rc resource.h
	@echo "[25%] Compile resources..."
	@windres resource.rc $@

clean :
	rm -rf ../client.exe ../server.exe obj/*

# -net.cpp doesn't use resources header
obj/%-net.o: %-net.cpp a2.h
	@echo $<
	@$(CC) -c $(CFLAGS) $< -o $@

obj/%.o: %.cpp a2.h resource.h
	@echo $<
	@$(CC) -c $(CFLAGS) $< -o $@
	
.FORCE:
$(F): .FORCE