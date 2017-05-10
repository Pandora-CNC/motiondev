#ifndef _MOTIONDEV_H_
#define _MOTIONDEV_H_

/* Driver config */
#define MINOR_DEVICE_NUMBER (0)
#define DEVICE_FILE_NAME "motion"

/* Debug control */
#define DEBUG_ENABLED 1
#define TRACE_SIZE 250000
#define TIMESTAMP_ENABLED 0

/* Max write/read index (+ 1) */
#define NUM_OF_REGS 167

/* Trace flags */
#define TRACE_WR_FLAG	0x1u
#define TRACE_RD_FLAG 	0x2u
#define BYPASS_RD_FLAG	0x4u

/* Operation flags */
#define OP_ENABLE_TRACE 0x1u /* format [O] Op */
#define OP_DISABLE_TRACE 0x2u /* format [O] Op */
#define OP_RESET_TRACE 0x3u /* format [O] Op */
#define OP_SET_BYPASS 0x4u /* format [OADD] Op, Address, Data*/
#define OP_CLEAR_BYPASS 0x5u /* format [OA] Op, Address */
#define OP_FILTER_WRITE 0x6u /* format [OALTS] Op, Address, Length, Type, State */

#endif
