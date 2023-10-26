RM := rm

all:
	#@echo "[INFO] Build start"
	#@echo "[INFO] Building kernel drivers"
	#@cd kernelspace && $(MAKE) -s
	@echo "[INFO] Creating userspace objects"
	@cd userspace && $(MAKE) -s
	@echo "[INFO] Build finished"
clean :
	#@echo "[INFO] Cleaning start"
	#@echo "[INFO] Cleaning kernel drivers"
	#@cd kernelspace && $(MAKE) -s clean
	@echo "[INFO] Cleaning userspace objects"
	@cd userspace && $(MAKE) -s clean
	@echo "[INFO] Removing executable"
	@$(RM) -rfv client
	@echo "[INFO] Cleaning finished"
