MAKEFLAGS += --output-sync=target
SHELL_DIR := test

tests = \
	$(SHELL_DIR)/metadata \
	$(SHELL_DIR)/pause \
	$(SHELL_DIR)/play \
	$(SHELL_DIR)/play-pause \
	$(SHELL_DIR)/stop \
	$(SHELL_DIR)/quit

.PHONY: \
	test \
	$(tests) \
	clean

test: $(tests)

$(tests):
	./$(SHELL_DIR)/wrapper "$@"

clean:
	rm -f \
	  *.mpv.ipc* \
	  *.mpv.log \
	  *.xvfb.log \
	  *.Xauthority \
	  *.socat.log \
	  *.exit-code.log \
	  *.stderr.log
	rm -rf $(SHELL_DIR)/dbus
