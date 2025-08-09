MAKEFLAGS += --output-sync=target
SHELL_DIR := shell

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
	  $(SHELL_DIR)/*.mpv.ipc* \
	  $(SHELL_DIR)/*.mpv.log \
	  $(SHELL_DIR)/*.xvfb.log \
	  $(SHELL_DIR)/*.Xauthority \
	  $(SHELL_DIR)/*.socat.log \
	  $(SHELL_DIR)/*.exit-code.log \
	  $(SHELL_DIR)/*.stderr.log
	rm -rf $(SHELL_DIR)/dbus
