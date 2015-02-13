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
static unsigned char buffer[LARGE_BUFFER]; //buffer for bytes received from skinproc
static unsigned int max_buffer_length; //maximum length of buffer pointer
static unsigned int buffer_length; //current length of buffer pointer
static unsigned int expected_length; //length of current buffer
static unsigned char rxflag;

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
    clearRXFlag();
    checkFrameSize();
    //max_buffer_length = LARGE_BUFFER;
}

//Query skinproc for size of frame
void checkFrameSize() {
    max_buffer_length = LARGE_BUFFER;
    buffer_length = 0;
    unsigned char length = 1;
    unsigned char test[1];
    rx_idx = TACTILE_MODE_G;
    test[0] = rx_idx;
    expected_length = 0;
    delay_ms(500); //waiting
    delay_ms(500); //for
    delay_ms(500); //skinproc
    delay_ms(500); //to startup
    sendTactileCommand(length, test);
    Nop();
    Nop();
    Nop();
    //blocking wait for skinproc to answer
    while (!checkRXFlag()) {
        Nop();
    }
    if (buffer[0] == rx_idx) {
        TACTILE_ROWS = buffer[2];
        TACTILE_COLS = buffer[3];
        max_buffer_length = TACTILE_ROWS*TACTILE_COLS*2+2;
        buffer_length = 0;

    }
    else {
        char test = buffer[0];
        TACTILE_ROWS = buffer[0];
        TACTILE_COLS = buffer[1];
        max_buffer_length = LARGE_BUFFER;
        buffer_length = 0;
    }

    Nop();
    Nop();
    rx_idx = TACTILE_RX_IDLE;
    clearRXFlag();
    sendCTS();
}


//Callback function when imageproc receives tactile command from radio
void handleSkinRequest(unsigned char length, unsigned char *frame) {
    unsigned char cmd = frame[0];
    //unsigned char tempframe[TACTILE_ROWS * TACTILE_COLS * 2 + 1];
    //static unsigned char tempframe[100];
    //buffer = tempframe;
    buffer_length = 0;
    expected_length = 0;
    switch (cmd) {
        case TACTILE_MODE_G: //query number of rows and columns
            rx_idx = TACTILE_MODE_G;
            //TACTILE_ROWS = 0x00;
            //TACTILE_COLS = 0x00;
            buffer_length = 4;
            //expected_length = 3;
            buffer[0] = rx_idx;
            buffer[1] = 0x02;
            buffer[2] = TACTILE_ROWS;
            buffer[3] = TACTILE_COLS;
            setRXFlag();
            //buffer = rowcol;
            //buffer = tempframe;
            //sendTactileCommand(length,frame);
            break;
        case TACTILE_MODE_A: //sample individual pixel
            rx_idx = TACTILE_MODE_A;
            sendTactileCommand(length,frame);
            break;
        case TACTILE_MODE_B: //sample frame
            rx_idx = TACTILE_MODE_B;
            sendTactileCommand(length,frame);
            break;
        case TACTILE_MODE_E: //start scan
            rx_idx = TACTILE_MODE_E;
            sendTactileCommand(length,frame);
            break;
        case TACTILE_MODE_F: //stop scan
            rx_idx = TACTILE_RX_IDLE;
            sendTactileCommand(length,frame);
            break;
        case TACTILE_MODE_T:
            rx_idx = TACTILE_MODE_T;
            buffer_length = 0;
            expected_length = max_buffer_length;
            
            //buffer = tempframe;
            sendTactileCommand(length,frame);
            break;
        default:
            rx_idx = cmd;
            sendTactileCommand(length,frame);
            break;
    }
    //blocking wait for skinproc to answer
    /*while (buffer_length < expected_length) {
        Nop();
    }
    handleSkinData(buffer_length, buffer);
    rx_idx = TACTILE_RX_IDLE;*/
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
void handleSkinData(unsigned int length, unsigned char *data){
    //Cannot handle any length over 114
    if (length > 114) {
        length = 114;
    }
    radioSendData(RADIO_DST_ADDR, 0, CMD_TACTILE, length, data, 0);
    //data = data + length/2 - 1;
    //data[0] = rx_idx;
    //radioSendData(RADIO_DST_ADDR, 0, CMD_TACTILE, length/2 +1, data, 0);
}

void checkTactileBuffer(){
    if (checkRXFlag()) {
        handleSkinData(buffer_length, buffer);
        expected_length = 0;
        buffer_length = 0;
        rx_idx = TACTILE_RX_IDLE;
        clearRXFlag();
        sendCTS();
    }
}

void setRXFlag(){
    rxflag = 0x01;
}

void clearRXFlag(){
    rxflag = 0x00;
}

unsigned char checkRXFlag(){
    return rxflag;
}

void sendCTS(){
    unsigned char frame[1];
    frame[0] = CTS;
    sendTactileCommand(1,frame);
}

//read data from the UART, and fill each byte into the buffer
void __attribute__((__interrupt__, no_auto_psv)) _U2RXInterrupt(void) {
    unsigned char rx_byte;

    CRITICAL_SECTION_START
    LED_1 = ~LED_1;
    while(U2STAbits.URXDA) {
        rx_byte = U2RXREG;
        if (0){//buffer_length == 0 && rx_byte != rx_idx) {
            Nop();  //first byte received isn't rx_idx
        } else {
            buffer[buffer_length] = rx_byte;
            if (buffer_length == 1) {
                Nop();
                Nop();
                expected_length = ((unsigned int) rx_byte) + 2;
            }
            ++buffer_length;
        }


    }
    if (expected_length != 0 && buffer_length >= expected_length) { //captured a full packet
        setRXFlag();
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
