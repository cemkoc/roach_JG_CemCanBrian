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
	#define U2BAUD    		9  // For 1 Mbps (HIGH SPEED)
	#define U1BRGH  		1
#endif

static unsigned char tx_idx; //tx mode (to radio)
static unsigned char rx_idx; //rx mode (from radio)
static unsigned char TACTILE_ROWS; //number of rows in tactile grid
static unsigned char TACTILE_COLS; //number of columns in tactile grid
static int rx_count; //count received characters
static unsigned char* buffer; //buffer for bytes received from skinproc
static unsigned int max_buffer_length; //maximum length of buffer pointer
static unsigned int buffer_length; //current length of buffer pointer

//Initialize UART module and query skinproc tactile grid size
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
}

//Query skinproc for size of frame
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
    if (buffer[0] == TACTILE_MODE_G) {
        TACTILE_ROWS = buffer[1];
        TACTILE_COLS = buffer[2];
        max_buffer_length = TACTILE_ROWS*TACTILE_COLS*2+2;
        buffer_length = 0;

    }
    else {
        max_buffer_length = LARGE_BUFFER;
        buffer_length = 0;
    }

}


//Callback function when imageproc receives tactile command from radio
void handleSkinRequest(unsigned char length, unsigned char *frame) {
    unsigned char cmd = frame[0];
    //unsigned char tempframe[TACTILE_ROWS * TACTILE_COLS * 2 + 1];
    unsigned char tempframe[max_buffer_length];
    static unsigned int expected_length;
    switch (cmd) {
        case TACTILE_MODE_G: //query number of rows and columns
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
            buffer_length = 0;
            expected_length = max_buffer_length-1;
            buffer = tempframe;
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
    //blocking wait for skinproc to answer
    while (buffer_length < expected_length) {
        Nop();
    }
    handleSkinData(buffer_length, buffer);
    rx_idx = TACTILE_RX_IDLE;
}

//Send command over UART to skinproc
unsigned char sendTactileCommand(unsigned char length, unsigned char *frame) {
    static int i;
    static unsigned char val;

    tx_idx = frame[0];
    for (i = 0; i < length; i++) {
        val = frame[i];
        if (TACTILEUART) {
            while(BusyUART2());
            WriteUART2(val);
        }
    }
    return 1;
}

//transmit skin data over radio, cap data length if over threshhold
void handleSkinData(unsigned char length, unsigned char *data){
    //Cannot handle any length over 114
    if (length > 114) {
        length = 114;
    }
    radioSendData(RADIO_DST_ADDR, 0, CMD_TACTILE, length, data, 0);
    //data = data + length/2 - 1;
    //data[0] = rx_idx;
    //radioSendData(RADIO_DST_ADDR, 0, CMD_TACTILE, length/2 +1, data, 0);
}

//read data from the UART, and fill each byte into the buffer
void __attribute__((__interrupt__, no_auto_psv)) _U2RXInterrupt(void) {
    unsigned char rx_byte;

    CRITICAL_SECTION_START
    LED_1 = ~LED_1;
    while(U2STAbits.URXDA) {
        rx_byte = U2RXREG;
        buffer[buffer_length] = rx_byte;
        Nop();
        ++buffer_length;

    }

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
    
    _U2TXIF = 0;
    LED_3 = 0;
    CRITICAL_SECTION_END
}
