#define   CLK_MAIN       84000000UL

// Name - Port - Pin(DUE board)
// TCLK1 = PA4 = AD5
// TIOA1 = PA2 = AD7
// TIOB1 = PA3 = AD6

volatile uint32_t freq = 0;
volatile uint32_t flag = 0;
volatile uint32_t freq_adj = 0;

int in_Byte = 0; 
int choice = 0;
int period = 0;
int counter = 0;
unsigned long SMP_RATE = 100;
unsigned long TMR_CNTR = CLK_MAIN/(2*SMP_RATE);

int debug_osm = 0; // debug over serial monitor
String instring="";
void setup()
{
  Serial.begin (115200);
  Serial.println("Please input the time period: ");   
  pinMode(53,OUTPUT);
  Atmr_setup();
  Btmr_setup();

  pio_TIOA0();  // Gate OUT, Dig. 2 conect Dig-2 to AN-7.
  pio_TCLK1();  // Counter IN, AN - 5 (max 3.3V -> keep safe !!!)
  pio_TIOA1();  // Gate IN, AN - 7.
  pinMode( 7, OUTPUT); // OUT, Digital 7 -> connect to AN-5 for Testing
  analogWriteResolution(12);
}

void loop()
{
   debug_osm = 1; 
  if ( Serial.available() > 0 ) {       //tells how may characters are available in the receive buffer 
       in_Byte = Serial.read(); 
       if(isDigit(in_Byte)){
            instring+=(char)in_Byte;
       }
       if (in_Byte=='\n'){
            choice = instring.toInt();
            Serial.print(choice);    
            if(choice!=0){
               
            }
            Serial.print("The time period you set is ");  
            Serial.print("Hz");
            SMP_RATE = (unsigned long)(choice);
            TMR_CNTR = (unsigned long)(CLK_MAIN / (2 *SMP_RATE));
            Atmr_setup();
            Serial.print('\n');
            Serial.print("SMP_RATE:                  ");
            Serial.print(SMP_RATE);
            Serial.print('\n');
            Serial.print("TMR_CNTR:                  ");
            Serial.print(TMR_CNTR);
            instring="";
       }
    }
    
  if ( flag ) {
    freq_adj = (4095*freq)/(4000000/SMP_RATE);
    analogWrite(DAC0, freq_adj);
    if(debug_osm){
      Serial.print("\n\tfreq: ");
      Serial.print(freq,  DEC);
      flag = 0;
    }
  }
}


void TC1_Handler(void)            //check the IRQ
{
  if ((TC_GetStatus(TC0, 1) & TC_SR_LDRBS) == TC_SR_LDRBS) {    //get the status of counter0 && with float probe
    uint32_t dummy = TC0->TC_CHANNEL[1].TC_RA;
    freq  = TC0->TC_CHANNEL[1].TC_RB;
    flag  = 1;
   // Serial.print("interrupt!\n");
  }
}

void Atmr_setup() // Gate 1 sec., conect Dig-2 to AN-7.
{
  pmc_enable_periph_clk (TC_INTERFACE_ID + 0 * 3 + 0);
  TcChannel  * t = &(TC0->TC_CHANNEL)[0];
  t->TC_CCR = TC_CCR_CLKDIS;                    //chanel control register
  t->TC_IDR = 0xFFFFFFFF;                       //interupt disable, 1->disable all interupts
  t->TC_SR;                                     //status register -> read only
  t->TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 |     
              TC_CMR_WAVE |
              TC_CMR_WAVSEL_UP_RC |
              TC_CMR_EEVT_XC0 |
              TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_CLEAR |
              TC_CMR_BCPB_CLEAR | TC_CMR_BCPC_CLEAR;      //select clock 1, set value to all 32 bits in TC_CMR (Control mode register) 
  t->TC_RC = TMR_CNTR;
  t->TC_RA = TMR_CNTR / 2;      //maybe RC
  t->TC_CMR = (t->TC_CMR & 0xFFF0FFFF) | TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_SET;
  t->TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
}

void Btmr_setup()  // Counter     //TC1 TO TC0
{
  pmc_enable_periph_clk (TC_INTERFACE_ID + 0 * 3 + 1);

  uint32_t reg_val = TC_BMR_TC1XC1S_TCLK1; // Input Capture Pin, AN-5.
  TC0->TC_BMR |= reg_val;

  TcChannel * t = &(TC0->TC_CHANNEL)[1];
  t->TC_CCR = TC_CCR_CLKDIS;
  t->TC_IDR = 0xFFFFFFFF;
  t->TC_SR;
  t->TC_CMR = TC_CMR_TCCLKS_XC1
              | TC_CMR_LDRA_RISING
              | TC_CMR_LDRB_FALLING
              | TC_CMR_ABETRG
              | TC_CMR_ETRGEDG_FALLING;

  t->TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;

  t->TC_IER =  TC_IER_LDRBS;                      //interupt enable register
  t->TC_IDR = ~TC_IER_LDRBS;                      

  NVIC_DisableIRQ(TC1_IRQn);
  NVIC_ClearPendingIRQ(TC1_IRQn);
  NVIC_SetPriority(TC1_IRQn, 0);
  NVIC_EnableIRQ(TC1_IRQn);
}

void pio_TIOA0()
{
  PIOB->PIO_PDR   = PIO_PB25B_TIOA0;            //PIO disable register
  PIOB->PIO_IDR   = PIO_PB25B_TIOA0;            //Interupt diable register
  PIOB->PIO_ABSR |= PIO_PB25B_TIOA0;            //AB select register 
}

void pio_TCLK1()
{
  PIOA->PIO_PDR = PIO_PA4A_TCLK1;
  PIOA->PIO_IDR = PIO_PA4A_TCLK1;
}

void pio_TIOA1()
{
  PIOA->PIO_PDR = PIO_PA2A_TIOA1;
  PIOA->PIO_IDR = PIO_PA2A_TIOA1;
}
