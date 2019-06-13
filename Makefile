#
# In order to execute this "Makefile" just type "make"
#

OBJS 	= dropbox_client.o BufferImplementation.o ListImplementation.o FilesImplementation.o
SOURCE	= dropbox_client.c BufferImplementation.c ListImplementation.c FilesImplementation.c
HEADER  = BufferTypes.h ListTypes.h Messages.h
OUT  	= dropbox_client
CC	= gcc
FLAGS   = -g -c -pthread
# -g option enables debugging mode 
# -c flag generates object code for separate files

$(OUT): $(OBJS)
	$(CC) -g -pthread $(OBJS) -o $@

# create/compile the individual files >>separately<< 
dropbox_client.o: dropbox_client.c
	$(CC) $(FLAGS) dropbox_client.c

BufferImplementation.o: BufferImplementation.c
	$(CC) $(FLAGS) BufferImplementation.c

ListImplementation.o: ListImplementation.c
	$(CC) $(FLAGS) ListImplementation.c

FilesImplementation.o: FilesImplementation.c
	$(CC) $(FLAGS) FilesImplementation.c



# clean house
clean:
	rm -f $(OBJS) $(OUT)

# do a bit of accounting
count:
	wc $(SOURCE) $(HEADER)
