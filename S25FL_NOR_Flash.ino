#include "NORFlash.h"

// Global pin names/variables
static pin_size_t write_protect = 21;
static pin_size_t reset = 20;
static pin_size_t cs = 17;
static pin_size_t sck = 18;
static pin_size_t sdo = 19;
static pin_size_t sdi = 16;

String last_mode; //Current Operating Mode to display
static char command = 0; //Command issued by the user
static NORFlash* mem;
static int loop_cnt = 0;

//Global functions
void clear_screen(){
  //Clear the serial output screen
  Serial.write(27); //ESC command
  Serial.print("[2J"); //clear screen command
  Serial.write(27);
  Serial.print("[H"); //cursor to home command
}

void print_screen(){
  clear_screen();
  if (mem->part_number != "Unknown"){
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else{
    Serial.println("Error: Incorrect unable to identify chip. Please Power Cycle/Check SPI Interface.");
    for(;;){}
  }
  Serial.println("---------------------- NOR Flash Tester --------------------");
  Serial.println("");
  Serial.print("Part Number  : ");
  Serial.println(mem->part_number);
  Serial.print("Manufacturer : ");
  Serial.println(mem->mfg);
  Serial.print("Density (MB) : ");
  Serial.println(mem->density);
  Serial.println("");
  Serial.println("------------------------- Commands -------------------------");
  Serial.println("");
  Serial.println("(1)-Erase | (2)-Read | (3)-Write | (4)-SWReset | (5)-HWReset");
  Serial.println("");
  Serial.println("--------------------------- Status -------------------------");
  Serial.println("");
  Serial.print("Mode                  : ");
  Serial.print(mem->mode);
  Serial.println("");
  Serial.print("Write In Progress     : ");
  if (mem->wip){
    Serial.print("True ");
    switch(loop_cnt){
      case 0:
        Serial.println("-");
        loop_cnt++;
        break;
      case 1:
        Serial.println("\\");
        loop_cnt++;
        break;
      case 2:
        Serial.println("|");
        loop_cnt++;
        break;
      case 3:
        Serial.println("/");
        loop_cnt = 0;
        break;
      default:
        loop_cnt = 0;
    }
  }
  else{
    Serial.println("False");
  }
  Serial.print("Write Enable Latch    : ");
  if (mem->wel){
    Serial.println("True");
  }
  else{
    Serial.println("False");
  }
  Serial.print("Program Error         : ");
  if (mem->p_err){
    Serial.println("True");
  }
  else{
    Serial.println("False");
  }
  Serial.print("Erase Error           : ");
  if (mem->e_err){
    Serial.println("True");
  }
  else{
    Serial.println("False");
  }
  if (mem->mode == "Read"){
    Serial.print("Incorrect Bytes found : ");
    if (mem->error_bytes == 0){
      Serial.println("0");
    }
    else{
      Serial.println(mem->error_bytes);
    }
  }
  if (mem->mode == "Read" || mem->mode == "Write"){
    //Number of complete 
    double complete = ((mem->bytes_covered/(pow(mem->density, 3)))*100);
    Serial.print("Bytes covered         : ");
    if (mem->bytes_covered == 0){
      Serial.print("0/");
      Serial.println((int) pow(mem->density, 3));
    }
    else{
      Serial.print(mem->bytes_covered);
      Serial.print("/");
      Serial.println((int) pow(mem->density, 3));
    }
    Serial.println("");
    Serial.print(" ");
    Serial.print(complete);
    Serial.print("% [");
    for (int i = 0; i < 100; i++){
      if (i % 2 == 0){
        if (i <= complete){
          Serial.print("#");
        }
        else{
          Serial.print(".");
        }
      }
    }
    Serial.println("]");
  }
  Serial.println("");
  Serial.println("--------------------- SpaceX (c) 2023 ----------------------");
  Serial.print("\r");
}

void setup() {

  // Begin the Serial Port for Debugging
  Serial.begin(9600);

  delay(2000);  //Give everything a moment

  //Init NOR Flash
  mem = new NORFlash(sck, sdi, sdo, reset, cs, write_protect);

  delay(1000);  //Give everything a moment

  //On-board LED
  pinMode(LED_BUILTIN, OUTPUT);
  delay(3000);  //Give everything another moment

  //print_screen();
}

void loop() {
  //Wait for a command
  if (Serial.available() > 0){
    //Read byte the user sent
    command = Serial.read();

    //Interpret the command and change the operating mode of the device
    switch(command){
      case '1':
        mem->erase();
        break;
      case '2':
        mem->read();
        break;
      case '3':
        mem->write();
        break;
      case '4':
        mem->software_reset();
        break;
      case '5':
        mem->hardware_reset();
        break;
      default:
        __asm__("nop");
    }

    //Remain in this mode until the memory WIP bit is back to 0, then enter standby mode
    
  }
  mem->read_status();

}

void setup1(){
  delay(3000);
  Serial.println("Core 1 Online");
}

void loop1(){
  //Update the screen
  print_screen();
  delay(100);
}
