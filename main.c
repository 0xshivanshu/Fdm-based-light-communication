#include <lpc17xx.h>

#define RS 27 // P0.27
#define EN 28 // P0.28
#define DT 23 // P0.23 to P0.26 data lines

// Define the two LED pins
#define LED1_PORT LPC_GPIO0 
#define LED1_PIN (1 << 4) // P0.4 
#define LED2_PORT LPC_GPIO0 
#define LED2_PIN (1 << 5) // P0.5 

#define MATCH_VAL_100HZ 125000 
#define MATCH_VAL_300HZ 41667 

unsigned long int t1 = 0, t2 = 0, i;
unsigned char f1 = 0, f2 = 0;
unsigned char msg[] = {"key="};
unsigned char col, row, flag, key;
unsigned long var; 
unsigned int r; 
unsigned int bit_index; 
unsigned long int initcom[] = {0x30, 0x30, 0x30, 0x20, 0x28, 0x0c, 0x06, 0x01, 0x80}; 

volatile int g_led1_enabled = 0; // The "switch" for LED 1, 0=OFF, 1=ON (blinking) 

void lcdw(void); 
void portw(void); 
void delayl(unsigned int); 
void scan(); 

void delay_for_bit_period(void) {
    volatile unsigned int delay_counter; 
    for(delay_counter = 0; delay_counter < 6000000; delay_counter++); 
}

void Timer0_Init(void) {
    LPC_SC->PCONP |= (1 << 1); 
    LPC_SC->PCLKSEL0 &= ~(0x3 << 2); 
    LPC_SC->PCLKSEL0 |= (0x1 << 2); 
    LPC_TIM0->TCR = 0x02; 
    LPC_TIM0->MCR = (1 << 0) | (1 << 3); // Interrupts on MR0 and MR1 
    LPC_TIM0->MR0 = MATCH_VAL_100HZ; 
    LPC_TIM0->MR1 = MATCH_VAL_300HZ; 
    NVIC_EnableIRQ(TIMER0_IRQn); 
    LPC_TIM0->TCR = 0x01; 
}

void TIMERO_IRQHandler(void) {
    if (LPC_TIM0->IR & (1 << 0)) { // MR0 (100Hz) 
        LPC_TIM0->IR = (1 << 0); 
        if (g_led1_enabled == 1) { 
            if (LED1_PORT->FIOPIN & LED1_PIN) 
                LED1_PORT->FIOCLR = LED1_PIN; 
            else
                LED1_PORT->FIOSET = LED1_PIN; 
        } else {
            LED1_PORT->FIOCLR = LED1_PIN; 
        }
        LPC_TIM0->MR0 += MATCH_VAL_100HZ; 
    }
    if (LPC_TIM0->IR & (1 << 1)) { // MR1 (300Hz) 
        if (LED2_PORT->FIOPIN & LED2_PIN) 
            LED2_PORT->FIOCLR = LED2_PIN; 
        else
            LED2_PORT->FIOSET = LED2_PIN; 
        LPC_TIM0->MR1 += MATCH_VAL_300HZ; 
        LPC_TIM0->IR = (1 << 1); 
    }
}

int main(void) {
    SystemInit(); 
    SystemCoreClockUpdate(); 
    LPC_PINCON->PINSEL0 = 0; 
    LPC_GPIO0->FIODIR = 1<<RS | 1<<EN | 0xF<<DT | LED1_PIN | LED2_PIN; 
    
    // Keypad configuration 
    LPC_PINCON->PINSEL3 &= 0xFFC03FFF;
    LPC_PINCON->PINSEL4 &= 0xF00FFFFF;
    LPC_GPIO2->FIODIR = 0x00003C00; 
    LPC_GPIO1->FIODIR &= 0xF87FFFFF;

    f1 = 0; // Command mode 
    for (i = 0; i < 9; i++) {
        t1 = initcom[i]; 
        lcdw(); 
    }

    f1 = 1; // Data mode 
    i = 0;
    while (msg[i] != '\0') { 
        t1 = msg[i]; 
        lcdw(); 
        i++; 
    }

    Timer0_Init(); 

    while(1) {
        for (row = 0; row < 4; row++) { 
            LPC_GPIO2->FIOPIN = 1 << (row + 10); 
            flag = 0; 
            scan(); 
            if (flag == 1) { 
                key = 4 * row + col; 
                if (key < 10) t1 = key + 0x30; 
                else t1 = key - 10 + 0x41; 
                lcdw(); 

                for (bit_index = 0; bit_index < 4; bit_index++) { 
                    if ((key >> bit_index) & 1) g_led1_enabled = 1; 
                    else g_led1_enabled = 0; 
                    delay_for_bit_period(); 
                }
                g_led1_enabled = 0; 
                do {
                    LPC_GPIO2->FIOPIN = (1 << (row + 10)); 
                    flag = 0; 
                    scan(); 
                } while (flag == 1); 
                delayl(40000); 
            }
        }
    }
}

void lcdw(void) {
    f2 = (f1 == 1) ? 0 : ((t1 == 0x30) || (t1 == 0x20)) ? 1 : 0; 
    unsigned long temp = t1;
    t1 = (temp & 0xf0) << (DT - 4); 
    portw(); 
    if (!f2) {
        t1 = (temp & 0x0f) << DT; 
        portw(); 
    }
}

void portw(void) {
    LPC_GPIO0->FIOCLR = (1 << RS) | (1 << EN) | (0x0F << DT); 
    LPC_GPIO0->FIOSET = t1; 
    if (f1 == 0) LPC_GPIO0->FIOCLR = (1 << RS); 
    else LPC_GPIO0->FIOSET = (1 << RS); 
    LPC_GPIO0->FIOSET = (1 << EN); 
    delayl(25); 
    LPC_GPIO0->FIOCLR = (1 << EN); 
    delayl(30000); 
}

void delayl(unsigned int r1) {
    while(r1--); 
}

void scan() {
    var = LPC_GPIO1->FIOPIN & (0xf << 23); 
    if(var) {
        flag = 0x1; 
        var = var >> 23; 
        col = (int)log2(var); 
    }
}
