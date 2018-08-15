CC = gcc
CFLAGS = -g -MMD -MP -Wall -Werror=implicit -pthread -std=gnu99
SRCS = main.c init.c util.c mem.c reg.c desc.c dhcp.c
LIBS = checker.a
OBJS = $(SRCS:.c=.o)
DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))
TARGET = driver


TARGET_KEYWORD := I217
TARGET_DEFAULT_DRIVER := e1000e
TARGET_PCI_BUS_ID:=$(if $(TARGET_PCI_BUS_ID),$(TARGET_PCI_BUS_ID),$(shell lspci -D | grep $(TARGET_KEYWORD) | cut -f 1 -d ' ' | head -n1))
TARGET_PCI_VID_DID=$(shell lspci -n -s $(TARGET_PCI_BUS_ID) | cut -f 3 -d ' ')
TARGET_PCI_VID=$(shell echo $(TARGET_PCI_VID_DID) | cut -f 1 -d ':')
TARGET_PCI_DID=$(shell echo $(TARGET_PCI_VID_DID) | cut -f 2 -d ':')
TARGET_CURRENT_DRIVER=$(shell lspci -k -s $(TARGET_PCI_BUS_ID) | grep "Kernel driver in use:" | cut -f 2 -d ":" | tr -d " ")

ARGS=



default: $(TARGET)

.PHONY: load restore run exec

-include $(DEPS)

run:load
	trap '$(MAKE) restore;' INT ; $(MAKE) exec ; $(MAKE) restore;

exec: $(TARGET)
	sudo sh -c "echo 120 > /proc/sys/vm/nr_hugepages"
	sudo ./$(TARGET) $(ARGS)


check:
	@lspci -vvxxx -n -k -s $(TARGET_PCI_BUS_ID)
	@echo "           bus: $(TARGET_PCI_BUS_ID)"
	@echo "        vendor: $(TARGET_PCI_VID)"
	@echo "        device: $(TARGET_PCI_DID)"
	@echo "driver current: $(TARGET_CURRENT_DRIVER)"
	@echo "       default: $(TARGET_DEFAULT_DRIVER)"

load:
ifneq ($(shell lspci -v -s $(TARGET_PCI_BUS_ID) | grep "in use" |  grep $(TARGET_DEFAULT_DRIVER)),)
	sudo modprobe uio_pci_generic
	sudo sh -c "echo '$(TARGET_PCI_VID) $(TARGET_PCI_DID)' > /sys/bus/pci/drivers/uio_pci_generic/new_id"
	sudo sh -c "echo -n $(TARGET_PCI_BUS_ID) > /sys/bus/pci/drivers/$(TARGET_CURRENT_DRIVER)/unbind"
	sudo sh -c "echo -n $(TARGET_PCI_BUS_ID) > /sys/bus/pci/drivers/uio_pci_generic/bind"
endif

restore:
ifeq ($(shell lspci -v -s $(TARGET_PCI_BUS_ID) | grep "in use" | grep $(TARGET_DEFAULT_DRIVER)),)
	sudo modprobe $(TARGET_DEFAULT_DRIVER)
	sudo sh -c "echo -n $(TARGET_PCI_BUS_ID) > /sys/bus/pci/drivers/$(TARGET_CURRENT_DRIVER)/unbind"
	sudo sh -c "echo -n $(TARGET_PCI_BUS_ID) > /sys/bus/pci/drivers/$(TARGET_DEFAULT_DRIVER)/bind"
endif


watch_irq:
	watch -n1 "cat /proc/interrupts"



$(TARGET): $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) -o $@ $^


clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS) *~
