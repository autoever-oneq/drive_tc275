#ifndef DRIVER_STM
#define DRIVER_STM

/***********************************************************************/
/*Include*/
/***********************************************************************/
#include "Ifx_Types.h"
#include "IfxStm.h"
#include "IfxCpu_Irq.h"
#include <stdint.h>


/***********************************************************************/
/*Typedef*/
/***********************************************************************/
typedef struct
{
    uint8_t u8nuScheduling1msFlag;
    uint8_t u8nuScheduling10msFlag;
    uint8_t u8nuScheduling100msFlag;
    uint8_t u8nuScheduling1000msFlag;
}SchedulingFlag;

/***********************************************************************/
/*Define*/
/***********************************************************************/

/***********************************************************************/
/*External Variable*/
/***********************************************************************/
extern SchedulingFlag stSchedulingInfo;


/***********************************************************************/
/*Global Function Prototype*/
/***********************************************************************/
extern void Driver_Stm_Init(void);
void delay(uint32_t ms);


#endif /* DRIVER_STM */
