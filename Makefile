SUBDIRS = libdicomprint libdpvoxelrender glview

all:
	for i in $(SUBDIRS); do \
		$(MAKE) -C $$i; \
	done

clean:
	for i in $(SUBDIRS); do \
		$(MAKE) -C $$i clean; \
	done

