#include "IMPRESS_reconfiguration.h"

virtual_architecture_t va;

//#define USE_PCAP

int main() {

#ifdef USE_PCAP
	init_virtual_architecture_PCAP();

	change_partition_position(&va, 0, 1, 40, 25);
	change_partition_position(&va, 0, 0, 40, 9);

	change_partition_element_PCAP(&va, 0, 0, GROUP2_SHIFT1);
	change_partition_element_PCAP(&va, 0, 1, GROUP1_ADD);
	change_partition_element_PCAP(&va, 0, 0, GROUP2_SHIFT2);
	change_partition_element_PCAP(&va, 0, 1, GROUP1_SUBSTRACT);
#else
	init_virtual_architecture_HBICAP();

	change_partition_position(&va, 0, 1, 40, 25);
	change_partition_position(&va, 0, 0, 40, 9);

	change_partition_element_HBICAP(&va, 0, 0, GROUP2_SHIFT1);
	change_partition_element_HBICAP(&va, 0, 1, GROUP1_ADD);
	change_partition_element_HBICAP(&va, 0, 0, GROUP2_SHIFT2);
	change_partition_element_HBICAP(&va, 0, 1, GROUP1_SUBSTRACT);
#endif

	while(1);

	return 0;
}
