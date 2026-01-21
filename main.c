#include <lpc17xx.h>

#define RS 27 // P0.27
#define EN 28 // P0.28
#define DT 23 // P0.23 to P0.26 data lines

// Define the two LED pins
#define LED1_PORT LPC_GPIO0 [cite: 167]
#define LED1_PIN (1 << 4) // P0.4 [cite: 168]
#define LED2_PORT LPC_GPIO0 [cite: 169]
#define LED2_PIN (1 << 5) // P0.5 [cite: 170]

#define MATCH_VAL_100HZ 125000 [cite: 171]
#define MATCH_VAL_300HZ 41667 [cite: 172]

unsigned long int t1 = 0, t2 = 0, i; [cite: 173]
unsigned char f1 = 0, f2 = 0; [cite: 174]
unsigned char msg[] = {"key="}; [cite: 175]
unsigned char col, row, flag, key; [cite: 176]
unsigned long var; [cite: 177, 187]
unsigned int r; [cite: 178, 188]
unsigned int bit_index; [cite: 180]
unsigned long int initcom[] = {0x30, 0x30, 0x30, 0x20, 0x28, 0x0c, 0x06, 0x01, 0x80}; [cite: 184]

volatile int g_led1_enabled = 0; // The "switch" for LED 1, 0=OFF, 1=ON (blinking) [cite: 189]

void lcdw(void); [cite: 181]
void portw(void); [cite: 182]
void delayl(unsigned int); [cite: 183]
void scan(); [cite: 179, 194]

void delay_for_bit_period(void) {
    volatile unsigned int delay_counter; [cite: 192]
    for(delay_counter = 0; delay_counter < 6000000; delay_counter++); [cite: 193]
}

void Timer0_Init(void) {
    LPC_SC->PCONP |= (1 << 1); [cite: 196]
    LPC_SC->PCLKSEL0 &= ~(0x3 << 2); [cite: 197]
    LPC_SC->PCLKSEL0 |= (0x1 << 2); [cite: 198]
    LPC_TIM0->TCR = 0x02; [cite: 199]
    LPC_TIM0->MCR = (1 << 0) | (1 << 3); // Interrupts on MR0 and MR1 [cite: 201]
    LPC_TIM0->MR0 = MATCH_VAL_100HZ; [cite: 203]
    LPC_TIM0->MR1 = MATCH_VAL_300HZ; [cite: 204]
    NVIC_EnableIRQ(TIMER0_IRQn); [cite: 205]
    LPC_TIM0->TCR = 0x01; [cite: 206]
}

void TIMERO_IRQHandler(void) {
    if (LPC_TIM0->IR & (1 << 0)) { // MR0 (100Hz) [cite: 211]
        LPC_TIM0->IR = (1 << 0); [cite: 212]
        if (g_led1_enabled == 1) { [cite: 213, 215]
            if (LED1_PORT->FIOPIN & LED1_PIN) [cite: 217]
                LED1_PORT->FIOCLR = LED1_PIN; [cite: 218]
            else
                LED1_PORT->FIOSET = LED1_PIN; [cite: 220]
        } else {
            LED1_PORT->FIOCLR = LED1_PIN; [cite: 223]
        }
        LPC_TIM0->MR0 += MATCH_VAL_100HZ; [cite: 226]
    }
    if (LPC_TIM0->IR & (1 << 1)) { // MR1 (300Hz) [cite: 228]
        if (LED2_PORT->FIOPIN & LED2_PIN) [cite: 230]
            LED2_PORT->FIOCLR = LED2_PIN; [cite: 231]
        else
            LED2_PORT->FIOSET = LED2_PIN; [cite: 233]
        LPC_TIM0->MR1 += MATCH_VAL_300HZ; [cite: 236]
        LPC_TIM0->IR = (1 << 1); [cite: 237]
    }
}

int main(void) {
    SystemInit(); [cite: 239]
    SystemCoreClockUpdate(); [cite: 240]
    LPC_PINCON->PINSEL0 = 0; [cite: 241]
    LPC_GPIO0->FIODIR = 1<<RS | 1<<EN | 0xF<<DT | LED1_PIN | LED2_PIN; [cite: 242]
    
    // Keypad configuration [cite: 243, 244, 245, 249, 250]
    LPC_PINCON->PINSEL3 &= 0xFFC03FFF;
    LPC_PINCON->PINSEL4 &= 0xF00FFFFF;
    LPC_GPIO2->FIODIR = 0x00003C00; 
    LPC_GPIO1->FIODIR &= 0xF87FFFFF;

    f1 = 0; // Command mode [cite: 251]
    for (i = 0; i < 9; i++) {
        t1 = initcom[i]; [cite: 254]
        lcdw(); [cite: 255]
    }

    f1 = 1; // Data mode [cite: 257]
    i = 0;
    while (msg[i] != '\0') { [cite: 259]
        t1 = msg[i]; [cite: 262]
        lcdw(); [cite: 263]
        i++; [cite: 260]
    }

    Timer0_Init(); [cite: 270]

    while(1) {
        for (row = 0; row < 4; row++) { [cite: 273]
            LPC_GPIO2->FIOPIN = 1 << (row + 10); [cite: 275]
            flag = 0; [cite: 276]
            scan(); [cite: 277]
            if (flag == 1) { [cite: 278]
                key = 4 * row + col; [cite: 285]
                if (key < 10) t1 = key + 0x30; [cite: 287]
                else t1 = key - 10 + 0x41; [cite: 289]
                lcdw(); [cite: 290]

                for (bit_index = 0; bit_index < 4; bit_index++) { [cite: 292]
                    if ((key >> bit_index) & 1) g_led1_enabled = 1; [cite: 294, 297]
                    else g_led1_enabled = 0; [cite: 301]
                    delay_for_bit_period(); [cite: 302]
                }
                g_led1_enabled = 0; [cite: 304]
                do {
                    LPC_GPIO2->FIOPIN = (1 << (row + 10)); [cite: 307]
                    flag = 0; [cite: 308]
                    scan(); [cite: 309]
                } while (flag == 1); [cite: 310]
                delayl(40000); [cite: 311]
            }
        }
    }
}

void lcdw(void) {
    f2 = (f1 == 1) ? 0 : ((t1 == 0x30) || (t1 == 0x20)) ? 1 : 0; [cite: 316]
    unsigned long temp = t1;
    t1 = (temp & 0xf0) << (DT - 4); [cite: 317]
    portw(); [cite: 318]
    if (!f2) {
        t1 = (temp & 0x0f) << DT; [cite: 321]
        portw(); [cite: 322]
    }
}

void portw(void) {
    LPC_GPIO0->FIOCLR = (1 << RS) | (1 << EN) | (0x0F << DT); [cite: 325]
    LPC_GPIO0->FIOSET = t1; [cite: 326]
    if (f1 == 0) LPC_GPIO0->FIOCLR = (1 << RS); [cite: 328]
    else LPC_GPIO0->FIOSET = (1 << RS); [cite: 330]
    LPC_GPIO0->FIOSET = (1 << EN); [cite: 332]
    delayl(25); [cite: 333]
    LPC_GPIO0->FIOCLR = (1 << EN); [cite: 334]
    delayl(30000); [cite: 335]
}

void delayl(unsigned int r1) {
    while(r1--); [cite: 337]
}

void scan() {
    var = LPC_GPIO1->FIOPIN & (0xf << 23); [cite: 341]
    if(var) {
        flag = 0x1; [cite: 343]
        var = var >> 23; [cite: 344]
        col = (int)log2(var); [cite: 345]
    }
}
