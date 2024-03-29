#include "NORFlash.h"

// Global pin names/variables
static pin_size_t write_protect = 21;
static pin_size_t reset = 22;
static pin_size_t cs1 = 17; //CS1 - 17 | CS2 - 5 | CS3 - 21
static pin_size_t cs2 = 5;
static pin_size_t cs3 = 21;
static pin_size_t sck = 18;
static pin_size_t sdo = 19;
static pin_size_t sdi = 16;

String last_mode; //Current Operating Mode to display
static char command = 0; //Command issued by the user
static NORFlash* mem;
static int loop_cnt = 0;
static bool verbose = false;

//Global functions
void clear_screen(){
  //Clear the serial output screen
  Serial.write(27); //ESC command
  Serial.print("[2J"); //clear screen command
  Serial.write(27);
  Serial.print("[H"); //cursor to home command
}

void red_text(){
  Serial.write(27); //ESC command
  Serial.print("[0;31m"); //clear screen command
}

void green_text(){
  Serial.write(27); //ESC command
  Serial.print("[0;32m"); //clear screen command
}

void default_text(){
  Serial.write(27); //ESC command
  Serial.print("[0;39m"); //clear screen command
}

void print_screen(){
  clear_screen();
  if (mem->part_number != "Unknown"){
    digitalWrite(LED_BUILTIN, HIGH);
  }
  Serial.println("-------------------------------- NOR Flash Tester -----------------------------");
  Serial.println("");
  Serial.print("Part Number  : ");
  Serial.print(mem->device_type);
  Serial.print(" ");
  Serial.println(mem->part_number);
  Serial.print("Manufacturer : ");
  Serial.print(mem->mfg_id);
  Serial.print(" ");
  Serial.println(mem->mfg);
  Serial.print("Density (MB) : ");
  Serial.println(mem->density);
  Serial.print("Target DUT   : DUT");
  Serial.println(mem->dut);
  //Serial.print("TEMP         : ");
  //Serial.println(mem->tmp, HEX);
  Serial.println("");
  Serial.println("----------------------------------- Commands ----------------------------------");
  Serial.println("");
  Serial.println("(1)-Block Erase | (2)-Read | (3)-Verbose Read | (4)-Write | (5)-SWReset | (6)-HWReset | (7)-CS1 | (8)-CS2 | (9)-CS3");
  Serial.println("");
  Serial.println("------------------------------------ Status -----------------------------------");
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
      green_text();
      Serial.println("0");
      default_text();
    }
    else{
      red_text();
      Serial.println(mem->error_bytes);
      default_text();
    }
  }
  if (mem->mode == "Read" || mem->mode == "Write"){
    Serial.print("Current Sector (4KB)  : ");
    if (mem->current_sector == 0){
      Serial.println("0");
    }
    else{
      Serial.println(mem->current_sector);
    }
    //Number of complete 
    double complete = ((mem->bytes_covered/(mem->block_size))*100);
    Serial.print("Bytes covered         : ");
    if (mem->bytes_covered == 0){
      Serial.print("0/");
      Serial.println((int) mem->block_size);
    }
    else{
      Serial.print(mem->bytes_covered);
      Serial.print("/");
      Serial.println((int) mem->block_size);
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
  if (mem->mode == "Read" && verbose){
    Serial.println("Block Data (expect all 0xAA):");
    if(mem->error_bytes != 0){
      int sector_count = 0;
      for (int i = 0; i < mem->block_size; i++){
        if(i % mem->sector_size == 0){
          sector_count++;
        }
        if (mem->block_verbose[i] != 0xAA){
          Serial.print("Error bytes found: Byte #");
          Serial.print(i);
          Serial.print(" in Sector #");
          Serial.print(sector_count);
          Serial.print(": ");
          red_text();
          Serial.println(mem->block_verbose[i], HEX);
          default_text();
        }
      }
    }
    else{
      green_text();
      Serial.println("All bytes correct!");
      default_text();
    }
    Serial.println("");
  }
  Serial.println("------------------------------ SpaceX (c) 2023 -----------------------------");
  Serial.print("\r");
}

void setup() {

  //On-board LED
  pinMode(LED_BUILTIN, OUTPUT);

  // Begin the Serial Port for Debugging
  Serial.begin(9600);

  delay(1000);  //Give everything a moment

  //Init NOR Flash
  mem = new NORFlash(sck, sdi, sdo, reset, cs1, cs2, cs3, write_protect);

  delay(1000);  //Give everything a moment
  print_screen();
}

void loop() {
  //Wait for a command
  if (Serial.available() > 0){
    //Read byte the user sent
    command = Serial.read();

    //Interpret the command and change the operating mode of the device
    switch(command){
      case '1':
        mem->block_erase();
        break;
      case '2':
        verbose = false;
        mem->read();
        break;
      case '3':
        verbose = true;
        mem->read();
        break;
      case '4':
        mem->write();
        break;
      case '5':
        mem->software_reset();
        break;
      case '6':
        mem->hardware_reset();
        break;
      case '7':
        mem->set_chip_select(cs1, 1);
        break;
      case '8':
        mem->set_chip_select(cs2, 2);
        break;
      case '9':
        mem->set_chip_select(cs3, 3);
        break;
      default:
        __asm__("nop");
    }

    //Remain in this mode until the memory WIP bit is back to 0, then enter standby mode
    
  }
  mem->read_id();
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
