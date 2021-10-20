INSTALL ?= install

sbindir ?= $(prefix)/sbin
servicedir ?= $(prefix)/lib/systemd/system
targetdir ?= /etc/systemd/system/multi-user.target.wants

.PHONY: all
all:
	@echo "Nothing to build! Try make install"

.PHONY: install
install: wcn36xx-monitor
	install wcn36xx-monitor $(DESTDIR)$(sbindir)/wcn36xx-monitor
	install -m 644 wcn36xx-monitor.service $(DESTDIR)$(servicedir)/wcn36xx-monitor.service
	ln -sf $(DESTDIR)$(servicedir)/wcn36xx-monitor.service $(DESTDIR)$(targetdir)/wcn36xx-monitor.service

.PHONY: uninstall
uninstall:
	rm $(DESTDIR)$(targetdir)/wcn36xx-monitor.service
	rm $(servicedir)/wcn36xx-monitor.service
	rm $(sbindir)/wcn36xx-monitor
