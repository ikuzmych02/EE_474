#define NOTE_1 16000000 / (2 * 293) - 1
#define NOTE_2 16000000 / (2 * 329) - 1
#define NOTE_3 16000000 / (2 * 261) - 1
#define NOTE_4 16000000 / (2 * 130) - 1
#define NOTE_5 16000000 / (2 * 196) - 1
#define NOTE_rest 0
#define N_TASKS 4
#define READY 0
#define RUNNING 1
#define SLEEPING 2

#define PENDING 0
#define DONE 1

#define DELAY_TIMEOUT_VALUE 16000000 / (2 * 500) - 1

void (*taskPointers[N_TASKS]) (void);
int state[N_TASKS];
int sleepingTime[N_TASKS]; // to keep track of how long each function needs to sleep, will constantly decrement
volatile int sFlag = PENDING;

byte seven_seg_digits[10] = { 0b11111100, // 0
                              0b01100000, // 1
                              0b11011010, // 2
                              0b11110010, // 3
                              0b01100110, // 4
                              0b10110110, // 5 
                              0b10111110, // 6
                              0b11100000, // 7
                              0b11111110, // 8
                              0b11100110 }; // 9
/**
 * Pin 30: A
 * Pin 31: B
 * Pin 32: C
 * Pin 33: D
 * Pin 34: E
 * Pin 35: F
 * Pin 36: G
 * Pin 37: DP
 * Pin 23: DS4
 * Pin 24: DS3
 * Pin 25: DS2
 * Pin 26: DS1
 */

/* array defining all the frequencies of the melody  */
long melody[] = { NOTE_1, NOTE_rest, NOTE_2, NOTE_rest, NOTE_3, NOTE_rest, NOTE_4, NOTE_rest, NOTE_5 };
int curr = 0;
int task_index;
// int sleep;
unsigned long timer;
unsigned long counter = 0;

void setup() {
  taskPointers[0] = task1;
  taskPointers[1] = task2;
  taskPointers[2] = schedule_sync;
  taskPointers[3] = NULL;
  
  // DDRA = 0b00011110;
  // DDRC = 0xFF;
  DDRL |= 1 << DDL2;
  
  noInterrupts();
  TCCR4A = B01010100; // bottom two bits 0 (WGM31 & WGM30)
  TCCR4B = B00001001; // 4th bit set to 1 (WGMn4 & WGMn3) and set bottom bit for clock select

  /* for the delay timer */
  TCCR3A = B01010100; // bottom two bits 0 (WGMn1 & WGMn0)
  TCCR3B = B00001001; // 4th bit set to 1 (WGMn4 & WGMn3) and set bottom bit for clock select (prescaler of 1), clear timer on compare
  TIMSK3 = B00000010; // enable compare match interrupt for TIMER3
  OCR3A = DELAY_TIMEOUT_VALUE;
  
  DDRH |= 1 << DDH3; // pin 6
  interrupts();
  
}

ISR(TIMER3_COMPA_vect) {
    sFlag = DONE;
}

int x = 0;
void task1(void) {
 if (state[task_index] != SLEEPING) {
    state[task_index] = RUNNING; 
    if (x < 125) {
      PORTL |= 1 << PORTL2;
    } else {
      PORTL &= ~(1 << PORTL2);    
    }
    state[task_index] = READY;
 } else {
    state[task_index] = READY;
 }
  if (x == 500) {
    x = 0;
  }
  x++;
  return;
}

/**
 * Playing a song for Task 2...
 */
void task2(void) {
 if (state[task_index] != SLEEPING) {
    state[task_index] = RUNNING;   
    OCR4A = melody[curr];
    if (timer % 500 == 0 && !(melody[curr] == NOTE_rest)) {
      curr++;
    }
    else if ((melody[curr] == NOTE_rest) && (timer % 1000 == 0)) {
      curr++;
    }
    if ( curr == 9 ) {
      sleep_474 (1250);
      curr = 0;
      OCR4A = 0;
    } else {
      state[task_index] = READY;
    }
  }
  else {
    OCR4A = 0;
    curr = 0;
  }
}

void start_function(void (*functionPtr) () ) {
  functionPtr(); // runs the function that was passed down
  return;
}
 
void sleep_474 (int t) {
  state[task_index] = SLEEPING; // change the state of the current task to sleep
  sleepingTime[task_index] = t;
  return;
}


void schedule_sync(void) {
  while (sFlag == PENDING) {
    
  }
  for (int k = 0; k < task_index; k++) {
    if (sleepingTime[k] > 0) {
      sleepingTime[k]--;
    }
    if (sleepingTime[k] <= 0) {
      state[k] = READY;
    }
  }
  sFlag = PENDING;
  return;
}



void scheduler(void) {
  if ((taskPointers[task_index] == NULL) && (task_index != 0)) {
    task_index = 0;
  }
  if (task_index > 3) { // just in case above one fails
    task_index = 0;
  }
  start_function(taskPointers[task_index]);
  task_index++;
  return;
}

void loop() {
  if (sFlag == DONE) {
    timer++;
    scheduler();
    sFlag = PENDING; // reset the interrupt flag
  }
  
  // delay(1)
}
