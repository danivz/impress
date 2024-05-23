#include "IMPRESS_reconfiguration.h"
#include "xparameters.h"
#include "xil_io.h"

virtual_architecture_t va;

#define MUX_ADDR XPAR_MUX_ICAP_0_BASEADDR
//#define USE_PCAP

int main() {
	uint32_t constant_value;

	print("Starting application...\n");

#ifdef USE_PCAP
	init_virtual_architecture_PCAP();

	change_partition_position(&va, 0, 1, 40, 25);
	change_partition_position(&va, 0, 0, 40, 9);

	change_partition_element_PCAP(&va, 0, 1, MODULE_TOP);
	change_partition_element_PCAP(&va, 0, 0, MODULE_BOTTOM);
#else
	Xil_Out32((UINTPTR) MUX_ADDR, 0);

	init_virtual_architecture_HBICAP();

	change_partition_position(&va, 0, 1, 40, 25);
	change_partition_position(&va, 0, 0, 40, 9);

	change_partition_element_HBICAP(&va, 0, 1, MODULE_TOP);
	change_partition_element_HBICAP(&va, 0, 0, MODULE_BOTTOM);
#endif

	Xil_Out32((UINTPTR) MUX_ADDR, 1);

	//Fine-grain reconfiguration
	change_partition_mux(&va, 0, 1, 0, 1);
	change_partition_mux(&va, 0, 1, 1, 0);
	constant_value = 5;
	change_partition_constant(&va, 0, 1, 0, &constant_value);
	constant_value = 6;
	change_partition_constant(&va, 0, 1, 1, &constant_value);
	change_partition_FU(&va, 0, 1, 0, add);
	constant_value = 5;
	change_partition_constant(&va, 0, 0, 0, &constant_value);
	change_partition_mux(&va, 0, 0, 0, 0);
	constant_value = 0x83;
	change_partition_constant(&va, 0, 0, 1, &constant_value);
	change_partition_FU(&va, 0, 0, 0, and);
	reconfigure_fine_grain();

	print("Finishing application...\n\n");

	while(1);

	return 0;
}
