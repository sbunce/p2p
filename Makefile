CPU_COUNT = $(shell cat /proc/cpuinfo |grep -c "processor")

.PHONY: all
all: src
	cp src/p2p ./
	cp src/p2p_server ./
	@echo "building complete"

.PHONY: src
src:
	$(MAKE) -j$(CPU_COUNT) -C src

.PHONY: clean
clean:
	$(MAKE) -j$(CPU_COUNT) -C src clean
	rm -f hash/* download/* p2p p2p_server
