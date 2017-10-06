/*
* The program is a sample code. 
* It needs run with some NuMaker-PFM-NUC472 boards.
*
* Please remeber to modify global definition to enable RS485 port on board.
* Modify '//#define DEF_RS485_PORT 1' to '#define DEF_RS485_PORT 1'
*/

/* ----------------------- System includes --------------------------------*/
#include "mbed.h"
#include "rtos.h"
/*----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Defines ------------------------------------------*/
// Sharing buffer index
enum {
  eData_MBInCounter,
  eData_MBOutCounter,
  eData_MBError,  
  eData_DI,
  eData_DATA,   
  eData_Cnt
} E_DATA_TYPE;

#define REG_INPUT_START 1
#define REG_INPUT_NREGS eData_Cnt
/* ----------------------- Static variables ---------------------------------*/
static USHORT   usRegInputStart = REG_INPUT_START;
static USHORT   usRegInputBuf[REG_INPUT_NREGS];

DigitalOut led1(LED1);  // For temperature worker.
DigitalOut led2(LED2);  // For Modbus worker.
DigitalOut led3(LED3);  // For Holder CB

#define DEF_PIN_NUM 6
#if defined(TARGET_NUMAKER_PFM_NUC472)
DigitalIn DipSwitch[DEF_PIN_NUM] = { PG_1, PG_2, PF_9, PF_10, PC_10, PC_11 } ;
#elif defined(TARGET_NUMAKER_PFM_M453)
DigitalIn DipSwitch[DEF_PIN_NUM] = { PD_6, PD_1, PC_6, PC_7, PC_11, PC_12 };
#elif defined(TARGET_NUMAKER_PFM_M487)
DigitalIn DipSwitch[DEF_PIN_NUM] = { PH_9, PH_8, PB_9, PF_11, PG_4, PC_11 } ;
#endif

unsigned short GetValueOnDipSwitch()
{
    int i=0;
    unsigned short usDipValue = 0x0;
    for ( i=0; i<DEF_PIN_NUM ; i++)
        usDipValue |= DipSwitch[i].read() << i;
    usDipValue = (~usDipValue) & 0x003F;
    return usDipValue;
}

void worker_uart(void const *args)
{   
    int counter=0;
    // For UART-SERIAL Tx/Rx Service.
    while (true)
    {
        //xMBPortSerialPolling();
        if ( counter > 10000 )
        {
            led2 = !led2;
            counter=0;
        }    
        counter++;
    }
}

/* ----------------------- Start implementation -----------------------------*/
int
main( void )
{
    eMBErrorCode    eStatus;
    //Thread uart_thread(worker_uart);
    unsigned short usSlaveID=GetValueOnDipSwitch();
    
    // Initialise some registers
    for (int i=0; i<REG_INPUT_NREGS; i++)
         usRegInputBuf[i] = 0x0;
    
    printf("We will set modbus slave ID-%d(0x%x) for the device.\r\n", usSlaveID, usSlaveID );

    /* Enable the Modbus Protocol Stack. */
    if ( (eStatus = eMBInit( MB_RTU, usSlaveID, 0, 115200, MB_PAR_NONE )) !=  MB_ENOERR )
        goto FAIL_MB;
    else if ( (eStatus = eMBEnable(  ) ) != MB_ENOERR )
        goto FAIL_MB_1;
    else {
        for( ;; )
        {
            xMBPortSerialPolling();
            if ( eMBPoll( ) != MB_ENOERR ) break;
        }       
    }    
    
FAIL_MB_1:
    eMBClose();    
    
FAIL_MB:
    for( ;; )
    {
        led2 = !led2;
        Thread::wait(200);
    }
}




eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;
    
    if( ( usAddress >= REG_INPUT_START )
        && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegInputStart );
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;
     
    usRegInputBuf[eData_MBInCounter]++;
    usRegInputBuf[eData_MBOutCounter]++;
        
    if (eMode == MB_REG_READ)
    {
        usRegInputBuf[eData_DI] = GetValueOnDipSwitch();

        if( ( usAddress >= REG_INPUT_START )
            && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
        {
            iRegIndex = ( int )( usAddress - usRegInputStart );
            while( usNRegs > 0 )
            {
                *pucRegBuffer++ =
                    ( unsigned char )( usRegInputBuf[iRegIndex] >> 8 );
                *pucRegBuffer++ =
                    ( unsigned char )( usRegInputBuf[iRegIndex] & 0xFF );
                iRegIndex++;
                usNRegs--;
            }
        }
    }

    if (eMode == MB_REG_WRITE)
    {
        if( ( usAddress >= REG_INPUT_START )
            && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
        {
            iRegIndex = ( int )( usAddress - usRegInputStart );
            while( usNRegs > 0 )
            {
                usRegInputBuf[iRegIndex] =  ((unsigned int) *pucRegBuffer << 8) | ((unsigned int) *(pucRegBuffer+1));
                pucRegBuffer+=2;
                iRegIndex++;
                usNRegs--;
            }
        }
    }

    led3=!led3;
        
    return eStatus;
}


eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils,
               eMBRegisterMode eMode )
{
    return MB_ENOREG;
}

eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    return MB_ENOREG;
}
