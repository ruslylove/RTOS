#ifndef MPU_CONFIG_H
#define MPU_CONFIG_H

#include <stdint.h>

/*
 * ARM v8-M MPU — AP field values for RBAR[2:1]
 * All tasks run in privileged mode (FreeRTOS non-MPU port default).
 */
#define MPU_AP_RW_PRIV  0x00U   /* Read/Write: privileged only       */
#define MPU_AP_RW_ANY   0x01U   /* Read/Write: any privilege level   */
#define MPU_AP_RO_PRIV  0x02U   /* Read-Only:  privileged only       */
#define MPU_AP_RO_ANY   0x03U   /* Read-Only:  any privilege level   */

/*
 * secure_buffer: 32-byte aligned so it occupies exactly one MPU granule.
 * The MPU region over this buffer is configured as Read-Only; any write
 * attempt from any privilege level triggers a MemManage fault.
 */
extern uint8_t secure_buffer[32];

void mpu_init(void);
void mpu_dump_config(void);

#endif /* MPU_CONFIG_H */
