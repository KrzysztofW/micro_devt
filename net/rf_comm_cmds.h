#ifndef _COMMANDS_H_
#define _COMMANDS_H_

/* Kerui remote & detectors commands */
#define RF_KE_CMD_SIZE 6

uint8_t rf_ke_cmds[] = {
	RF_KE_CMD_SIZE, 0x5a, 0x6a, 0x55, 0x69, 0xaa, 0x59, /*  remote 1 bnt 1 */
	RF_KE_CMD_SIZE, 0x5a, 0x6a, 0x55, 0x69, 0xaa, 0x65, /*  remote 1 bnt 2 */
	RF_KE_CMD_SIZE, 0x5a, 0x6a, 0x55, 0x69, 0xaa, 0x56, /*  remote 1 bnt 3 */
	RF_KE_CMD_SIZE, 0x5a, 0x6a, 0x55, 0x69, 0xaa, 0x95, /*  remote 1 bnt 4 */
	0,
};

#endif
