all:
	$(MAKE) -C libdicraft-image
	$(MAKE) -C libdicraft-render
	$(MAKE) -C glview
	$(MAKE) -C python

clean:
	for i in libdicraft-image libdicraft-render glview python; do \
		$(MAKE) -C $$i clean; \
	done

