# Let BSD make switch to GNU make
all ${.TARGETS}:
	@echo "================================================================"
	@echo " PLEASE USE THE GNU VERSION OF MAKE (gmake) INSTEAD OF ${MAKE}  "
	@echo "================================================================"
	@echo ""
	@gmake $@
