SUBDIRS = libdicomprint libdpvoxelrender glview python

all:
	for i in $(SUBDIRS); do \
		$(MAKE) -C $$i; \
	done

clean:
	for i in $(SUBDIRS); do \
		$(MAKE) -C $$i clean; \
	done

