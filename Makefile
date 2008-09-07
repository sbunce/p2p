CPU_COUNT = $(shell cat /proc/cpuinfo |grep -c "processor")

.PHONY: src
src:
	$(MAKE) -j$(CPU_COUNT) -C src
	cp src/p2p ./
	cp src/p2p_nogui ./

.PHONY: loc
loc:
	$(MAKE) -C src loc

.PHONY: clean
clean:
	$(MAKE) -j$(CPU_COUNT) -C src clean
	rm -f hash/* download/* p2p p2p_nogui
