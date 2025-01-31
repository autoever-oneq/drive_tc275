/**********************************************************************************************************************
 * \file Rc522App.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of 
 * business you are acting and (ii) Infineon Technologies AG or its licensees. If and as long as no such terms of use
 * are agreed, use of this file is subject to following:
 * 
 * Boost Software License - Version 1.0 - August 17th, 2003
 * 
 * Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and 
 * accompanying documentation covered by this license (the "Software") to use, reproduce, display, distribute, execute,
 * and transmit the Software, and to prepare derivative works of the Software, and to permit third-parties to whom the
 * Software is furnished to do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including the above license grant, this restriction
 * and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all 
 * derivative works of the Software, unless such copies or derivative works are solely in the form of 
 * machine-executable object code generated by a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE 
 * COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *********************************************************************************************************************/


/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "Rc522App.h"
#include "CommunicationApp.h"
#include "IfxPort.h"
#include "IfxPort_PinMap.h"
#include "Driver_Stm.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define MFRC522_MAX_LEN     (4)

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
uint8_t g_nfcUID[20];
enum PCD_Register {
        // Page 0: Command and status
        //                        0x00          // reserved for future use
        CommandReg              = 0x01 << 1,    // starts and stops command execution
        ComIEnReg               = 0x02 << 1,    // enable and disable interrupt request control bits
        DivIEnReg               = 0x03 << 1,    // enable and disable interrupt request control bits
        ComIrqReg               = 0x04 << 1,    // interrupt request bits
        DivIrqReg               = 0x05 << 1,    // interrupt request bits
        ErrorReg                = 0x06 << 1,    // error bits showing the error status of the last command executed
        Status1Reg              = 0x07 << 1,    // communication status bits
        Status2Reg              = 0x08 << 1,    // receiver and transmitter status bits
        FIFODataReg             = 0x09 << 1,    // input and output of 64 byte FIFO buffer
        FIFOLevelReg            = 0x0A << 1,    // number of bytes stored in the FIFO buffer
        WaterLevelReg           = 0x0B << 1,    // level for FIFO underflow and overflow warning
        ControlReg              = 0x0C << 1,    // miscellaneous control registers
        BitFramingReg           = 0x0D << 1,    // adjustments for bit-oriented frames
        CollReg                 = 0x0E << 1,    // bit position of the first bit-collision detected on the RF interface
        //                        0x0F          // reserved for future use

        // Page 1: Command
        //                        0x10          // reserved for future use
        ModeReg                 = 0x11 << 1,    // defines general modes for transmitting and receiving
        TxModeReg               = 0x12 << 1,    // defines transmission data rate and framing
        RxModeReg               = 0x13 << 1,    // defines reception data rate and framing
        TxControlReg            = 0x14 << 1,    // controls the logical behavior of the antenna driver pins TX1 and TX2
        TxASKReg                = 0x15 << 1,    // controls the setting of the transmission modulation
        TxSelReg                = 0x16 << 1,    // selects the internal sources for the antenna driver
        RxSelReg                = 0x17 << 1,    // selects internal receiver settings
        RxThresholdReg          = 0x18 << 1,    // selects thresholds for the bit decoder
        DemodReg                = 0x19 << 1,    // defines demodulator settings
        //                        0x1A          // reserved for future use
        //                        0x1B          // reserved for future use
        MfTxReg                 = 0x1C << 1,    // controls some MIFARE communication transmit parameters
        MfRxReg                 = 0x1D << 1,    // controls some MIFARE communication receive parameters
        //                        0x1E          // reserved for future use
        SerialSpeedReg          = 0x1F << 1,    // selects the speed of the serial UART interface

        // Page 2: Configuration
        //                        0x20          // reserved for future use
        CRCResultRegH           = 0x21 << 1,    // shows the MSB and LSB values of the CRC calculation
        CRCResultRegL           = 0x22 << 1,
        //                        0x23          // reserved for future use
        ModWidthReg             = 0x24 << 1,    // controls the ModWidth setting?
        //                        0x25          // reserved for future use
        RFCfgReg                = 0x26 << 1,    // configures the receiver gain
        GsNReg                  = 0x27 << 1,    // selects the conductance of the antenna driver pins TX1 and TX2 for modulation
        CWGsPReg                = 0x28 << 1,    // defines the conductance of the p-driver output during periods of no modulation
        ModGsPReg               = 0x29 << 1,    // defines the conductance of the p-driver output during periods of modulation
        TModeReg                = 0x2A << 1,    // defines settings for the internal timer
        TPrescalerReg           = 0x2B << 1,    // the lower 8 bits of the TPrescaler value. The 4 high bits are in TModeReg.
        TReloadRegH             = 0x2C << 1,    // defines the 16-bit timer reload value
        TReloadRegL             = 0x2D << 1,
        TCounterValueRegH       = 0x2E << 1,    // shows the 16-bit timer value
        TCounterValueRegL       = 0x2F << 1,
};

enum PCD_Command {
        PCD_Idle                = 0x00,     // no action, cancels current command execution
        PCD_Mem                 = 0x01,     // stores 25 bytes into the internal buffer
        PCD_GenerateRandomID    = 0x02,     // generates a 10-byte random ID number
        PCD_CalcCRC             = 0x03,     // activates the CRC coprocessor or performs a self-test
        PCD_Transmit            = 0x04,     // transmits data from the FIFO buffer
        PCD_NoCmdChange         = 0x07,     // no command change, can be used to modify the CommandReg register bits without affecting the command, for example, the PowerDown bit
        PCD_Receive             = 0x08,     // activates the receiver circuits
        PCD_Transceive          = 0x0C,     // transmits data from FIFO buffer to antenna and automatically activates the receiver after transmission
        PCD_MFAuthent           = 0x0E,     // performs the MIFARE standard authentication as a reader
        PCD_SoftReset           = 0x0F      // resets the MFRC522
};

enum PICC_Command {
        // The commands used by the PCD to manage communication with several PICCs (ISO 14443-3, Type A, section 6.4)
        PICC_CMD_REQA           = 0x26,     // REQuest command, Type A. Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
        PICC_CMD_WUPA           = 0x52,     // Wake-UP command, Type A. Invites PICCs in state IDLE and HALT to go to READY(*) and prepare for anticollision or selection. 7 bit frame.
        PICC_CMD_CT             = 0x88,     // Cascade Tag. Not really a command, but used during anti collision.
        PICC_CMD_SEL_CL1        = 0x93,     // Anti collision/Select, Cascade Level 1
        PICC_CMD_SEL_CL2        = 0x95,     // Anti collision/Select, Cascade Level 2
        PICC_CMD_SEL_CL3        = 0x97,     // Anti collision/Select, Cascade Level 3
        PICC_CMD_HLTA           = 0x50,     // HaLT command, Type A. Instructs an ACTIVE PICC to go to state HALT.
        PICC_CMD_RATS           = 0xE0,     // Request command for Answer To Reset.
        // The commands used for MIFARE Classic (from http://www.mouser.com/ds/2/302/MF1S503x-89574.pdf, Section 9)
        // Use PCD_MFAuthent to authenticate access to a sector, then use these commands to read/write/modify the blocks on the sector.
        // The read/write commands can also be used for MIFARE Ultralight.
        PICC_CMD_MF_AUTH_KEY_A  = 0x60,     // Perform authentication with Key A
        PICC_CMD_MF_AUTH_KEY_B  = 0x61,     // Perform authentication with Key B
        PICC_CMD_MF_READ        = 0x30,     // Reads one 16 byte block from the authenticated sector of the PICC. Also used for MIFARE Ultralight.
        PICC_CMD_MF_WRITE       = 0xA0,     // Writes one 16 byte block to the authenticated sector of the PICC. Called "COMPATIBILITY WRITE" for MIFARE Ultralight.
        PICC_CMD_MF_DECREMENT   = 0xC0,     // Decrements the contents of a block and stores the result in the internal data register.
        PICC_CMD_MF_INCREMENT   = 0xC1,     // Increments the contents of a block and stores the result in the internal data register.
        PICC_CMD_MF_RESTORE     = 0xC2,     // Reads the contents of a block into the internal data register.
        PICC_CMD_MF_TRANSFER    = 0xB0,     // Writes the contents of the internal data register to a block.
        // The commands used for MIFARE Ultralight (from http://www.nxp.com/documents/data_sheet/MF0ICU1.pdf, Section 8.6)
        // The PICC_CMD_MF_READ and PICC_CMD_MF_WRITE can also be used for MIFARE Ultralight.
        PICC_CMD_UL_WRITE       = 0xA2      // Writes one 4 byte page to the PICC.
};


/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
void initRc522(void);
void readUID(uint8_t *uid);
boolean isNewCardPresent(void);
void haltRc522(void);
void antiCollRc522(uint8_t* serNum);
void transceiveRc522(
        uint8_t* sendData,
        uint8_t sendLen,
        uint8_t* backData,
        uint16_t* backLen);
void clearBitRc522(uint8_t reg, uint8_t mask);
void setBitRc522(uint8_t reg, uint8_t mask);
void selfTestRc522(void);

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
void initRc522(void) {
//    IfxPort_setPinModeInput(IfxPort_P02_7.port, IfxPort_P02_7.pinIndex, IfxPort_InputMode_noPullDevice);
    IfxPort_setPinModeOutput(IfxPort_P02_7.port, IfxPort_P02_7.pinIndex, IfxPort_OutputMode_pushPull,
            IfxPort_OutputIdx_general);
    IfxPort_setPinLow(IfxPort_P02_7.port, IfxPort_P02_7.pinIndex);
    delay(50);

    IfxPort_setPinHigh(IfxPort_P02_7.port, IfxPort_P02_7.pinIndex);
    delay(50);

    readRc522Byte(0x37 << 1);

    //Timer: TPrescaler*TreloadVal/6.78MHz = 24ms
    writeRc522Reg(TModeReg, 0x80);      //Tauto=1; f(Timer) = 6.78MHz/TPreScaler
    writeRc522Reg(TPrescalerReg, 0xA9); //TModeReg[3..0] + TPrescalerReg
    writeRc522Reg(TReloadRegL, 0x03);
    writeRc522Reg(TReloadRegH, 0xE8);

    writeRc522Reg(TxASKReg, 0x40);     // force 100% ASK modulation
    writeRc522Reg(ModeReg, 0x3D);       // CRC Initial value 0x6363

    uint8_t txControlRegister = readRc522Byte(TxControlReg);
    writeRc522Reg(TxControlReg, txControlRegister | 0x03);

    readRc522Byte(0x37 << 1);
    /*
    // Step 1: 소프트 리셋
    writeRc522Reg(CommandReg, PCD_SoftReset);  // Soft Reset 명령 실행
    delay(50);  // 안정적인 동작을 위해 대기

    // Step 2: 기본 설정
    writeRc522Reg(TModeReg, 0x8D);         // 타이머 설정: TAuto=1
    writeRc522Reg(TPrescalerReg, 0x3E);    // 타이머 프리스케일러 설정
    writeRc522Reg(TReloadRegL, 30);       // 타이머 재로드 값 (LSB)
    writeRc522Reg(TReloadRegH, 0);        // 타이머 재로드 값 (MSB)
    writeRc522Reg(TxASKReg, 0x40);         // 100% ASK 활성화
    writeRc522Reg(ModeReg, 0x3D);           // CRC 초기값 설정 (ISO 14443-3)

    // Step 3: 안테나 활성화
    uint8_t txControl = readRc522Byte(TxControlReg);
    print("Antenna ON\n", 11);
    writeRc522Reg(TxControlReg, txControl | 0x03);  // 안테나 활성화

    // Step 4: RF 게인 설정
    writeRc522Reg(RFCfgReg, 0x70);          // 최대 RF 게인 설정

    // Step 5: FIFO 초기화
    writeRc522Reg(FIFOLevelReg, 0x80);     // FIFO 초기화
    /*
    // Step 1: 소프트 리셋
    writeRc522Reg(CommandReg, PCD_SoftReset);

    // 리셋 후 안정적인 동작을 위해 딜레이 필요
    delay(50);
    readRc522Byte(CommandReg);


    // Step 2: 기본 설정
    writeRc522Reg(ModeReg, 0x3D);
    writeRc522Reg(TxControlReg, 0x03);
    writeRc522Reg(RFCfgReg, 0x70);
    */
}

void readUID(uint8_t *uid) {
    //Find cards, return card type
    uint16_t backBits;
    writeRc522Reg(BitFramingReg, 0x07);
    uid[0] = PICC_CMD_REQA;
    transceiveRc522(uid, 1, uid, &backBits);
    delay(1);
    antiCollRc522(uid);
    haltRc522();      //Command card into hibernation

//    uint8_t tmp;
//    writeRc522Reg(BitFramingReg, 0x07);     //TxLastBists = BitFramingReg[2..0]
//
//    TagType[0] = reqMode;
//    int irqEn = 0x77;
//    int waitIrq = 0x30;
//
//    writeRc522Reg(ComIEnReg, irqEn | 0x80);    // Interrupt request
//    tmp = readRc522Byte(ComIrqReg);
//    writeRc522Reg(ComIrqReg, tmp & (~0x80));  // clear bit mask
//    tmp = readRc522Byte(FIFOLevelReg);
//    writeRc522Reg(FIFOLevelReg, tmp | 0x80);         // FlushBuffer=1, FIFO Initialization
//
//    writeRc522Reg(CommandReg, PCD_Idle);    // NO action; Cancel the current command
//
//    // Writing data to the FIFO
//    for (i = 0; i < sendLen; i++)
//    {
//        Write_MFRC522(FIFODataReg, sendData[i]);
//    }
//
//    // Execute the command
//    Write_MFRC522(CommandReg, command);
//    if (command == PCD_TRANSCEIVE)
//    {
//        SetBitMask(BitFramingReg, 0x80);        // StartSend=1,transmission of data starts
//    }
//
//    // Waiting to receive data to complete
//    i = 2000;   // i according to the clock frequency adjustment, the operator M1 card maximum waiting time 25ms
//    do
//    {
//        //CommIrqReg[7..0]
//        //Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
//        n = Read_MFRC522(CommIrqReg);
//        i--;
//    }while ((i != 0) && !(n & 0x01) && !(n & waitIRq));
//
//    ClearBitMask(BitFramingReg, 0x80);          //StartSend=0
//
//    if (i != 0)
//    {
//        if (!(Read_MFRC522(ErrorReg) & 0x1B))   //BufferOvfl Collerr CRCErr ProtecolErr
//        {
//            status = MI_OK;
//            if (n & irqEn & 0x01)
//            {
//                status = MI_NOTAGERR;
//            }
//
//            if (command == PCD_TRANSCEIVE)
//            {
//                n = Read_MFRC522(FIFOLevelReg);
//                lastBits = Read_MFRC522(ControlReg) & 0x07;
//                if (lastBits)
//                {
//                    *backLen = (n - 1) * 8 + lastBits;
//                }
//                else
//                {
//                    *backLen = n * 8;
//                }
//
//                if (n == 0)
//                {
//                    n = 1;
//                }
//                if (n > MAX_LEN)
//                {
//                    n = MAX_LEN;
//                }
//
//                // Reading the received data in FIFO
//                for (i = 0; i < n; i++)
//                {
//                    backData[i] = Read_MFRC522(FIFODataReg);
//                }
//            }
//        }
//        else
//        {
//            status = MI_ERR;
//        }
//
//    }

    //SetBitMask(ControlReg,0x80);           //timer stops
    //Write_MFRC522(CommandReg, PCD_IDLE);


    /*
    uint8_t buffer[5];  // FIFO에 전송할 데이터
    uint8_t rxLength;   // 수신된 데이터 길이
    uint8_t i;

    // Step 1: SELECT 명령 준비
    buffer[0] = PICC_CMD_SEL_CL1;  // Cascade Level 1 명령
    buffer[1] = 0x20;              // NVB (Number of Valid Bits)

    // FIFO에 명령 및 데이터를 쓰기
    for (i = 0; i < 2; i++)
    {
        writeRc522Reg(FIFODataReg, buffer[i]);
    }

    // 명령 실행
    writeRc522Reg(CommandReg, PCD_Transceive);

    // 명령 완료 대기
    while (!(readRc522Byte(ComIrqReg) & 0x30))
        ;  // RxIRq 또는 IdleIRq 대기

    // Step 2: UID 읽기
    rxLength = readRc522Byte(FIFOLevelReg);  // FIFO에 저장된 데이터 길이 확인
    if (rxLength != 4)
    {
        print("ERROR_0\n", 8);
        return;  // UID 크기가 4 바이트가 아니면 에러 반환
    }

    // FIFO에서 UID 읽기
    for (i = 0; i < rxLength; i++)
    {
        g_nfcUID[i] = readRc522Byte(FIFODataReg);
    }
    int printLen = rxLength;
    printUart(g_nfcUID, &printLen);


    /*
    // Step 1: 카드 감지
    writeRc522Reg(FIFODataReg, PICC_CMD_REQA);  // Request 명령
    writeRc522Reg(CommandReg, PCD_Transceive); // 송수신 실행
    while (!(readRc522Byte(ComIrqReg) & 0x30)) {
        delay(50);
    }

    // Step 2: 충돌 방지
    writeRc522Reg(FIFODataReg, PICC_CMD_SEL_CL1);  // Anticollision 명령
    writeRc522Reg(FIFODataReg, 0x20);               // Cascade Level 1
    writeRc522Reg(CommandReg, PCD_Transceive);       // 송수신 실행

    while (!(readRc522Byte(ComIrqReg) & 0x30)) {
        delay(50);
    }

    // Step 3: UID 읽기
    length = readRc522Byte(FIFOLevelReg);  // FIFO에 저장된 데이터 길이 확인
    for (uint8_t i = 0; i < length; i++)
    {
        uid[i] = readRc522Byte(FIFODataReg);  // UID 데이터 읽기
    }

    int printLen = length;
    printUart(uid, &printLen);
    */
}


boolean isNewCardPresent(void) {

    uint8_t bufferATQA[2];
    uint8_t bufferSize = sizeof(bufferATQA);

    // Reset baud rates
    writeRc522Reg(TxModeReg, 0x00);
    writeRc522Reg(RxModeReg, 0x00);

    // Reset ModWidthReg
    writeRc522Reg(ModWidthReg, 0x26);

//    PCD_ClearRegisterBitMask(CollReg, 0x80);
    clearBitRc522(CollReg, 0x80);


//    MFRC522::StatusCode result = PICC_RequestA(bufferATQA, &bufferSize);
//    ->
//    PICC_REQA_or_WUPA(PICC_CMD_REQA, bufferATQA, bufferSize);
//    ->
//    byte validBits;
//    MFRC522
//    ::StatusCode status;
//
//    if (bufferATQA == nullptr || *bufferSize < 2)
//    { // The ATQA response is 2 bytes long.
//        return STATUS_NO_ROOM;
//    }
//    PCD_ClearRegisterBitMask(CollReg, 0x80);        // ValuesAfterColl=1 => Bits received after collision are cleared.
//    validBits = 7; // For REQA and WUPA we need the short frame format - transmit only 7 bits of the last (and only) byte. TxLastBits = BitFramingReg[2..0]
//    status = PCD_TransceiveData(&command, 1, bufferATQA, bufferSize, &validBits);
//    if (status != STATUS_OK)
//    {
//        return status;
//    }
//    if (*bufferSize != 2 || validBits != 0)
//    {       // ATQA must be exactly 16 bits.
//        return STATUS_ERROR;
//    }
//    return STATUS_OK;
}

void haltRc522(void) {
    uint8_t unLen;
    uint8_t buff[2];

    buff[0] = PICC_CMD_HLTA;
    buff[1] = 0;

    transceiveRc522(buff, 2, buff, &unLen);
}

void antiCollRc522(uint8_t* serNum) {
      uint8_t i;
      uint8_t serNumCheck = 0;
      uint16_t unLen;

      writeRc522Reg(BitFramingReg, 0x00);    //TxLastBists = BitFramingReg[2..0]

      serNum[0] = PICC_CMD_SEL_CL1;
      serNum[1] = 0x20;
      transceiveRc522(serNum, 2, serNum, &unLen);
}

void transceiveRc522(
        uint8_t* sendData,
        uint8_t sendLen,
        uint8_t* backData,
        uint16_t* backLen)
{
    uint8_t irqEn = 0x77;
    uint8_t waitIRq = 0x30;
    uint8_t lastBits;
    uint8_t n;
    uint16_t i;

    writeRc522Reg(ComIEnReg, irqEn | 0x80);
    clearBitRc522(ComIrqReg, 0x80);
    setBitRc522(FIFOLevelReg, 0x80);

    writeRc522Reg(CommandReg, PCD_Idle);

    //Writing data to the FIFO
    for (i = 0; i < sendLen; i++)
    {
        writeRc522Reg(FIFODataReg, sendData[i]);
    }

    //Execute the command
    writeRc522Reg(CommandReg, PCD_Transceive);
    setBitRc522(BitFramingReg, 0x80);   //StartSend=1,transmission of data starts


    //Waiting to receive data to complete
    i = 2;  //i according to the clock frequency adjustment, the operator M1 card maximum waiting time 25ms???
    do
    {
        //CommIrqReg[7..0]
        //Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
        n = readRc522Byte(ComIrqReg);
        i--;
    }while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    clearBitRc522(BitFramingReg, 0x80);     //StartSend=0

    if (i != 0)
    {
        if (!(readRc522Byte(ErrorReg) & 0x1B))
        {
//            status = true;
            if (n & irqEn & 0x01)
            {
//                status = false;
            }

            n = readRc522Byte(FIFOLevelReg);
            uint8_t l = n;
            lastBits = readRc522Byte(ControlReg) & 0x07;
            if (lastBits)
            {
                *backLen = (n - 1) * 8 + lastBits;
            }
            else
            {
                *backLen = n * 8;
            }

            if (n == 0)
            {
                n = 1;
            }
            if (n > MFRC522_MAX_LEN)
            {
                n = MFRC522_MAX_LEN;
            }

            //Reading the received data in FIFO
            for (i = 0; i < n; i++)
            {
                uint8_t d = readRc522Byte(FIFODataReg);
                backData[i] = d;
            }
        }
        else
        {
//            status = false;
        }
    }

}

void clearBitRc522(uint8_t reg, uint8_t mask) {
    uint8_t temp = readRc522Byte(reg);
    writeRc522Reg(reg, temp & (~mask));
}

void setBitRc522(uint8_t reg, uint8_t mask) {
    uint8_t temp = readRc522Byte(reg);
    writeRc522Reg(reg, temp | mask);
}

void selfTestRc522(void) {
    writeRc522Reg(CommandReg, PCD_SoftReset);   // Issue the SoftReset command.
    // The datasheet does not mention how long the SoftRest command takes to complete.
    // But the MFRC522 might have been in soft power-down mode (triggered by bit 4 of CommandReg)
    // Section 8.8.2 in the datasheet says the oscillator start-up time is the start up time of the crystal + 37,74μs. Let us be generous: 50ms.
    uint8_t count = 0;
    do
    {
        // Wait for the PowerDown bit in CommandReg to be cleared (max 3x50ms)
        delay(50);
    }while ((readRc522Byte(CommandReg) & (1 << 4)) && (++count) < 3);

    // 2. Clear the internal buffer by writing 25 bytes of 00h
    uint8_t ZEROES[25] = {0x00};
    writeRc522Reg(FIFOLevelReg, 0x80);      // flush the FIFO buffer
    for (int i = 0; i < 25; i++) {
        writeRc522Reg(FIFODataReg, 0); // write 25 bytes of 00h to FIFO
    }
    writeRc522Reg(CommandReg, PCD_Mem);     // transfer to internal buffer

    // 3. Enable self-test
    writeRc522Reg(0x36 << 1, 0x09);

    // 4. Write 00h to FIFO buffer
    writeRc522Reg(FIFODataReg, 0x00);

    // 5. Start self-test by issuing the CalcCRC command
    writeRc522Reg(CommandReg, PCD_CalcCRC);

    // 6. Wait for self-test to complete
    uint8 n;
    for (uint8_t i = 0; i < 0xFF; i++)
    {
        n = readRc522Byte(FIFOLevelReg);
        if (n >= 64)
        {
            break;
        }
    }
    writeRc522Reg(CommandReg, PCD_Idle);        // Stop calculating CRC for new content in the FIFO.

    // 7. Read out resulting 64 bytes from the FIFO buffer.
    uint8_t result[64];
    for (int i = 0; i < 64; i++){
        readRc522Byte(FIFODataReg);
    }
    // Auto self-test done
    // Reset AutoTestReg register to be 0 again. Required for normal operation.
    writeRc522Reg(0x36 << 1, 0x00);

    // Determine firmware version (see section 9.3.4.8 in spec)
    uint8_t version = writeRc522Reg(0x37 << 1);
}
