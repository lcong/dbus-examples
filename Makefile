TARGET = dbus-example
OBJS = dbus-example.o test123.o
CFLAGS = -c -Wall
LDFLAG = -v -ldbus-1
INC = -I /usr/include/dbus-1.0 -I /usr/lib/i386-linux-gnu/dbus-1.0/include
%.o:%.c
	$(CC) $(CFLAGS) $(INC) $< -o $@ -g
$(TARGET):$(OBJS)
	$(CC) $(OBJS) $(LDFLAG) $(INC)  -o $(TARGET)
clean:
	rm -rf $(OBJS) $(TARGET)
