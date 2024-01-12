PROG1 = decode64
PROG2 = encode64

OBJS = decode64.o op_base64.o encode64.o
CFLAGS = -O2 -g -Wall -pipe -ffast-math -fomit-frame-pointer

all: $(PROG1) $(PROG2)

clean: 
	rm -f $(PROG1) $(OBJS) $(PROG2)

$(PROG1) : decode64.o op_base64.o
        
$(PROG2) : encode64.o op_base64.o

