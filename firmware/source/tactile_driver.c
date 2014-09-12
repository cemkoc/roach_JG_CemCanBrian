#include "tactile_driver.h"
#include "uart.h"
#include "utils.h"
#include "settings.h"
#include "cmd.h"
#include "radio.h"
#include "sclock.h"
#include "timer.h"

#if defined(__TACTILE_OVER_UART)

        #define TACTILEUART             1

	// UART Configuration
	#define U2BAUD    		9								// For 1 Mbps (HIGH SPEED)
	#define U1BRGH  		1
#endif

static unsigned char tx_idx;
static unsigned char rx_idx;
static unsigned char RCLM[4]; //RCLU = Row, Column, Least significant 8 bits, Most significant 8 bits
static unsigned char string_length;
static unsigned char TACTILE_ROWS;
static unsigned char TACTILE_COLS;
static unsigned int totalSamples;
static unsigned int max_count;
static int rx_count;
static int LOWER8;
static unsigned char* buffer; //buffer for received bytes
static unsigned int max_buffer_length;
static unsigned int buffer_length;
int release;

void tactileInit() {

    if (TACTILEUART){
        /// UART2 for SkinProc, 1e5 Baud, 8bit, No parity, 1 stop bit
        unsigned int U2MODEvalue, U2STAvalue, U2BRGvalue;
        U2MODEvalue = UART_EN & UART_IDLE_CON & UART_IrDA_DISABLE &
                      UART_MODE_SIMPLEX & UART_UEN_00 & UART_DIS_WAKE &
                      UART_DIS_LOOPBACK & UART_DIS_ABAUD & UART_UXRX_IDLE_ONE &
                      UART_BRGH_SIXTEEN & UART_NO_PAR_8BIT & UART_1STOPBIT;
        U2STAvalue  = UART_INT_TX & UART_INT_RX_CHAR &UART_SYNC_BREAK_DISABLED &
                      UART_TX_ENABLE & UART_ADR_DETECT_DIS &
                      UART_IrDA_POL_INV_ZERO; // If not, whole output inverted.
        U2BRGvalue  = 21; //9; // = (4e6 / (4 * 1e5)) - 1 so the baud rate = 100000
        //this value matches SkinProc

        // =3 for 2.5M Baud
        //U2BRGvalue  = 43; // =43 for 230500Baud (Fcy / ({16|4} * baudrate)) - 1
        //U2BRGvalue  = 86; // =86 for 115200 Baud
        //U2BRGvalue  = 1041; // =1041 for 9600 Baud


        OpenUART2(U2MODEvalue, U2STAvalue, U2BRGvalue);



        ConfigIntUART2(UART_TX_INT_EN & UART_TX_INT_PR4 & UART_RX_INT_EN & UART_RX_INT_PR4);
        EnableIntU2TX;
        EnableIntU2RX;
    }

    tx_idx = TACTILE_TX_IDLE;
    rx_idx = TACTILE_RX_IDLE;

    rx_count = 0;
    TACTILE_ROWS = 0xFF;
    TACTILE_COLS = 0xFF;

    checkFrameSize();
    release = 0;
}

void checkFrameSize() {
    max_buffer_length = SMALL_BUFFER;
    buffer_length = 0;
    unsigned char temp1[max_buffer_length];
    buffer = temp1;

    unsigned char length = 1;
    unsigned char test[1];
    test[0] = TACTILE_MODE_G;
    delay_ms(500); //waiting
    delay_ms(500); //for
    delay_ms(500); //skinproc
    delay_ms(500); //to startup
    sendTactileCommand(length, test);
    delay_ms(500); //wait for
    delay_ms(500); //skinproc

    Nop();
    Nop();
    if (buffer[0] == test[0]) {
        TACTILE_ROWS = buffer[1];
        TACTILE_COLS = buffer[2];
        max_buffer_length = TACTILE_ROWS*TACTILE_COLS*2+2;
        //unsigned char temp2[max_buffer_length];
        //unsigned char temp2[max_buffer_length+10];
        //buffer = temp2;
        buffer_length = 0;

    }
    else {
        max_buffer_length = LARGE_BUFFER;
        //unsigned char temp2[max_buffer_length];
        //unsigned char temp2[max_buffer_length+10];
        //buffer = temp2;
        buffer_length = 0;
    }
    /*unsigned char rowcol[2];
    rowcol[0] = TACTILE_ROWS;
    rowcol[1] = TACTILE_COLS;
    string_length = 2;
    rx_idx = TACTILE_MODE_G;
    handleSkinData(string_length, rowcol);
    rx_idx = TACTILE_RX_IDLE;*/

}

void checkTactileBuffer(){
    if(buffer_length >= LARGE_BUFFER) {
        handleSkinData(buffer_length, buffer);
        buffer_length = 0;
        rx_idx = TACTILE_RX_IDLE;
    }
}


void handleSkinRequest(unsigned char length, unsigned char *frame) {
    unsigned char cmd = frame[0];
    //unsigned char tempframe[TACTILE_ROWS * TACTILE_COLS * 2 + 1];
    unsigned char tempframe[max_buffer_length];
    static unsigned int expected_length;
    switch (cmd) {
        case TACTILE_MODE_G:
            buffer_length = 3;
            expected_length = 3;
            rx_idx = TACTILE_MODE_G;
            unsigned char rowcol[3];
            rowcol[0] = rx_idx;
            rowcol[1] = TACTILE_ROWS;
            rowcol[2] = TACTILE_COLS;
            buffer = rowcol;
            break;
        case TACTILE_MODE_A:
            rx_idx = TACTILE_MODE_A;
            buffer_length = 0;
            expected_length = 5;
            buffer = tempframe;
            sendTactileCommand(length,frame);
            break;
        case TACTILE_MODE_B: //haven't checked yet
            rx_idx = TACTILE_MODE_B;
            sendTactileCommand(length,frame);
            break;
        case TACTILE_MODE_T:
            rx_idx = TACTILE_MODE_T;
            buffer_length = 0;
            expected_length = max_buffer_length;
            
            buffer = tempframe;
            sendTactileCommand(length,frame);
            break;
        default:
            rx_idx = cmd;
            sendTactileCommand(length,frame);
            break;
    }
    //wait for skinproc to answer
    while (buffer_length < expected_length) {
        Nop();
    }
    handleSkinData(buffer_length, buffer);
    rx_idx = TACTILE_RX_IDLE;
}

//General blocking UART send function, appends basic checksum
unsigned char sendTactileCommand(unsigned char length, unsigned char *frame) {
    static int i;
    static unsigned char val;

    //while(BusyUART2());
    //WriteUART2(length);
    //while(BusyUART2());
    //WriteUART2(~length);

    //checksum = 0xFF;
    //send payload data
    tx_idx = frame[0];
    for (i = 0; i < length; i++) {
        //checksum += frame[i];
        val = frame[i];
        if (TACTILEUART) {
            while(BusyUART2());
            WriteUART2(val);
        }
    }
    //Send Checksum Data
    //while(BusyUART2());
    //WriteUART2(checksum);
    return 1;
}

void skinDataReceived(unsigned char rx_byte){
    
                    //test code
                /*unsigned char i, status, string_length;
                unsigned char rowcol[2];
                rowcol[0] = 9;
                rowcol[1] = 6;
                string_length=2;
                status = 3;
                radioSendData(RADIO_DEST_ADDR, status, CMD_TACTILE,
                        string_length, rowcol, 0);
                return;
                */
    

    if(rx_idx == TACTILE_RX_IDLE && rx_byte == tx_idx) {
        rx_idx = rx_byte;
        tx_idx = TACTILE_TX_IDLE;
        rx_count = 0;
        return;
    }

    switch(rx_idx){
        case TACTILE_MODE_A:
            if (rx_count == 0) {
                RCLM[0] = rx_byte;
            } else if (rx_count == 1) {
                RCLM[1] = rx_byte;
            } else if (rx_count == 2) {
                RCLM[2] = rx_byte;
            } else if (rx_count == 3) {
                RCLM[3] = rx_byte;
                string_length = 4;
                handleSkinData(string_length, RCLM);
                rx_idx = TACTILE_RX_IDLE;
            }
            break;
        case TACTILE_MODE_B:
            if (TACTILE_ROWS == 0xFF || TACTILE_COLS == 0xFF){
                rx_idx = TACTILE_RX_IDLE;
                break;
            }
            /*LFRAME[rx_count] = rx_byte;
            if (rx_count > 105) {
                Nop();
                Nop();
            }
            if (rx_count >= TACTILE_ROWS*TACTILE_COLS*2-1) {
                string_length = TACTILE_ROWS*TACTILE_COLS*2+1;
                //DisableIntT1;
                handleSkinData(string_length, LFRAME);
                
                rx_idx = TACTILE_RX_IDLE;
                //EnableIntT1;
            }
            */
            if (rx_count % 4 == 0) {
                RCLM[0] = rx_byte;
            } else if (rx_count % 4 == 1) {
                RCLM[1] = rx_byte;
            } else if (rx_count % 4 == 2) {
                //LFRAME[RCLM[0]][RCLM[1]] = rx_byte;
            } else if (rx_count % 4 == 3) {
                //MFRAME[RCLM[0]][RCLM[1]] = rx_byte;
                //string_length = 4;
                //handleSkinData(string_length, RCLM);
                LED_1 = ~LED_1;
                if (RCLM[0] == TACTILE_ROWS - 1 && RCLM[1] == TACTILE_COLS - 1)
                {
                    string_length = TACTILE_ROWS*TACTILE_COLS*2;
                    //LFRAME[string_length-1] = 'A';
                    //DisableIntT1;
                    //handleSkinData(string_length, LFRAME);
                    //EnableIntT1;
                    //Nop();
                    //Nop();
                    //handleSkinData(string_length, MFRAME);
                    
                    rx_idx = TACTILE_RX_IDLE;
                }
            }

            break;
        case TACTILE_MODE_C:
            if (rx_count == 0) {
                RCLM[0] = rx_byte;
            } else if (rx_count == 1) {
                RCLM[1] = rx_byte;
                totalSamples = (unsigned int)(RCLM[0]) + ((unsigned int)(RCLM[1]) << 8);
                max_count = totalSamples * 2 + 3;
                LOWER8 = 1;
                string_length = 4;
            } else if (rx_count == 2) {
                RCLM[0] = rx_byte;
            } else if (rx_count == 3) {
                RCLM[1] = rx_byte;
            } else {
                if (LOWER8) {
                    RCLM[2] = rx_byte;
                    LOWER8 = 0;
                } else {
                    RCLM[3] = rx_byte;
                    handleSkinData(string_length, RCLM);
                    LOWER8 = 1;
                    if (rx_count == max_count) {
                        rx_idx = TACTILE_RX_IDLE;
                    }
                }
            }
            break;
        case TACTILE_MODE_D:
            break;
        case TACTILE_MODE_E:
            break;
        case TACTILE_MODE_F:
            break;
        case TACTILE_MODE_G:
            if (rx_count == 0) {
                TACTILE_ROWS = rx_byte;
            } else if (rx_count == 1) {
                TACTILE_COLS = rx_byte;
                unsigned char tempLFRAME[TACTILE_ROWS][TACTILE_COLS];
                unsigned char tempMFRAME[TACTILE_ROWS][TACTILE_COLS];
                //LFRAME = tempLFRAME;
                //MFRAME = tempMFRAME;
                int i;
                int j;
                for (i = 0; i < TACTILE_ROWS; ++i){
                    for (j = 0; j < TACTILE_COLS; ++i){
                        //LFRAME[i][j] = 0xFF;
                        //MFRAME[i][j] = 0xFF;
                    }
                }
                unsigned char rowcol[2];
                rowcol[0] = TACTILE_ROWS;
                rowcol[1] = TACTILE_COLS;
                string_length = 2;
                handleSkinData(string_length, rowcol);
                rx_idx = TACTILE_RX_IDLE;
            }
            break;
        default:
            rx_idx = TACTILE_RX_IDLE;
            break;
    }


    rx_count = rx_count + 1;

}

void handleSkinData(unsigned char length, unsigned char *data){
    //Cannot handle any length over 114
    if (length > 114) {
        length = 114;
    }
    radioSendData(RADIO_DEST_ADDR, 0, CMD_TACTILE, length, data, 0);
    //data = data + length/2 - 1;
    //data[0] = rx_idx;
    //radioSendData(RADIO_DEST_ADDR, 0, CMD_TACTILE, length/2 +1, data, 0);
}

//read data from the UART, and call the proper function based on the Xbee code
void __attribute__((__interrupt__, no_auto_psv)) _U2RXInterrupt(void) {
    unsigned char rx_byte;

    CRITICAL_SECTION_START
    LED_1 = ~LED_1;
    while(U2STAbits.URXDA) {
        rx_byte = U2RXREG;
        //skinDataReceived(rx_byte);
        buffer[buffer_length] = rx_byte;
        Nop();
        ++buffer_length;

    }
    /*while(U2STAbits.URXDA) {
        rx_byte = U2RXREG;

        if(rx_idx == UART_RX_IDLE && rx_byte < UART_MAX_SIZE) {
            rx_checksum = rx_byte;
            rx_idx = UART_RX_CHECK_SIZE;
        } else if(rx_idx == UART_RX_CHECK_SIZE) {
            if((rx_checksum ^ rx_byte) == 0xFF && rx_checksum < UART_MAX_SIZE) {
                rx_packet = ppoolRequestFullPacket(rx_checksum - (PAYLOAD_HEADER_LENGTH+3));
                rx_payload = rx_packet->payload;
                rx_checksum += rx_byte;
                rx_idx = 0;

            } else {
                rx_checksum = rx_byte;
            }
        } else if (rx_idx == rx_payload->data_length + PAYLOAD_HEADER_LENGTH) {
            if(rx_checksum == rx_byte && rx_callback != NULL) {
                (rx_callback)(rx_packet);
            } else {
                ppoolReturnFullPacket(rx_packet);
            }
            rx_idx = UART_RX_IDLE;
        } else {
            rx_checksum += rx_byte;
            rx_payload->pld_data[rx_idx++] = rx_byte;
        }
    }
    
    */

    if(U2STAbits.OERR) {
        U2STAbits.OERR = 0;
    }

    _U2RXIF = 0;
    //LED_1 = 0;
    CRITICAL_SECTION_END
}


void __attribute__((interrupt, no_auto_psv)) _U2TXInterrupt(void) {
    //unsigned char tx_byte;
    CRITICAL_SECTION_START
    LED_3 = 1;
    /*if(tx_idx != UART_TX_IDLE) {
        if(tx_idx == UART_TX_SEND_SIZE) {
            tx_idx = 0;
            tx_byte = ~tx_checksum; // send size check
        } else if(tx_idx == tx_payload->data_length + PAYLOAD_HEADER_LENGTH) {
            ppoolReturnFullPacket(tx_packet);
            tx_packet = NULL;
            tx_idx = UART_TX_IDLE;
            tx_byte = tx_checksum;
        } else {
            tx_byte = tx_payload->pld_data[tx_idx++];
        }
        tx_checksum += tx_byte;
        WriteUART2(tx_byte);
    }
    */
    _U2TXIF = 0;
    LED_3 = 0;
    CRITICAL_SECTION_END
}
