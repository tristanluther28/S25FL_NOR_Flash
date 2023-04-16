#include <SPI.h>
#include <bitset>

// Read Device ID
#define RDID 0x9F // Read ID
#define RSFDP 0x5A // Read JEDEC Serial Flash Parameters 
#define RDQID 0xAF // Read Quad ID
#define RUID 0x4B // Read Unique ID

// Register Access
#define RDSR1 0x05 // Read Status Register 1
// RDSR1 Bits
#define WIP 0 //Write in Progress
#define WEL 1 //Write Enable Latch
//End RDSR1 Bits
#define RDSR2 0x07 // Read Status Register 2
// RDSR2 Bits
#define E_ERR 6 //Erase Error Occurred (1 == Error)
#define P_ERR 5 //Programming Error Occurred (1 == Error)
#define ES 1 //Erase Suspend
#define PS 0 //Program Suspend
//End RDSR2 Bits
#define RDCR1 0x35 // Read Configuration Register 1
#define RDCR2 0x15 // Read Configuration Register 2
#define RDCR3 0x33 // Read Configuration Register 3
#define RDAR 0x65 // Read Any Register
#define WRR 0x01 // Write Register, All Status and Configuration
#define WRDI 0x04 // Write Disable
#define WREN 0x06 // Write Enable for Non-volatile data change
#define WRENV 0x50 // Write Enable for Volatile Status and Configuration Registers
#define WRAR 0x71 // Write Any Register
#define CLSR 0x30 // Clear Status Register
#define BEN4 0xB7 // Enter 4 Byte Address Mode
#define BEX4 0xE9 // Exit 4 Byte Address Mode
#define SBL 0x77 // Set Burst Length
#define QPIEN 0x38 // Enter QPI
#define QPIEX 0xF5 // Exit QPI
#define DLPRD 0x41 // Data Learning Pattern Read
#define PDLRNV 0x43 // Program NV Data Learning Register
#define WDLRV 0x4A // Write Volatile Data Learning Register

// Read Flash Array
#define READ 0x03 // Read
#define READ4 0x13 // Read...again?
#define FAST_READ 0x0B // Fast Read
#define FAST_READ4 0x0C // Fast Read...also again
#define DOR 0x3B // Dual Output Read
#define DOR4 0x3C // Dual Output Read
#define QOR 0x6B // Quad Output Read
#define QOR4 0x6C // Quad Output Read
#define DIOR 0xBB // Dual I/O Read
#define DIOR4 0xBC // Dual I/O Read
#define QIOR 0xEB // Quad I/O Read - CR1V[1] = 1 or CR2V[3] = 1
#define QIOR4 0xEC // Quad I/O Read - CR1V[1] = 1 or CR2V[3] = 1
#define DDRQIOR 0xED // DDR Quad I/O Read - CR1V[1] = 1 or CR2V[3] = 1
#define DDRRQIOR4 0x0EE // DDR Quad I/O Read - CR1V[1] = 1 or CR2V[3] = 1

//Program Flash Array
#define PP 0x02 // Page Program
#define PP4 0x12 //Page Program
#define QPP 0x32 //Quad Page Program
#define QPP4 0x34 //Quad Page Program

//Erase Flash Array
#define SE 0x20 //Sector Erase
#define SE4 0x21 //Sector Erase
#define HBE 0x52 //Half Block Erase
#define HBE4 0x53 //Half Block Erase
#define BE 0xD8 //Block Erase
#define BE4 0xDC //Block Erase
#define CE 0x60 //Chip Erase
#define CE_ALT 0xC7 //Chip Erase (Alternate Instruction)

//Erase/Program Suspend/Resume
#define EPS 0x75 //Erase/Program Suspend
#define EPR 0x7A //Erase/Program Resume

//Security Region Array
#define SECRE 0x44 //Security Region Erase
#define SECRP 0x42 //Security Region Program
#define SECRR 0x48 //Security Region Read

//Array Protection
#define IBLRD 0x3D //IBL Read
#define IBLRD4 0xE0 //IBL Read
#define IBL 0x36 //IBL Lock
#define IBL4 0xE1 //IBL Lock
#define IBUL 0x39 //IBL Unlock
#define IBUL4 0xE2 //IBL Unlock
#define GBL 0x7E //Global IBL Lock0
#define GBUL 0x98 //Global IBL Unlock
#define SPRP 0xFB //Set Pointer Region Protection
#define SPRP4 0xE3 //Set Pointer Region Protection

//Individual and Region Protection
#define IRPRD 0x2B //IRP Register Read
#define IRPP 0x2F //IRP Register Program
#define PRRD 0xA7 //Protection Register Read
#define PRL 0xA6 //Protection Register Lock (NVLOCK Bit Write)
#define PASSRD 0xE7 //Password Read
#define PASSP 0xE8 //Password Program
#define PASSU 0xEA //Password Unlock

//Reset
#define RSTEN 0x66 //Software Reset Enable
#define RST 0x99 //Software Reset
#define MBR 0xFF //Mode Bit Reset

//Deep Power Down
#define DPD 0xB9 //Deep Power Down
#define RES 0xAB //Release from Deep Power Down/Device ID


// Global pin names/variables
static pin_size_t write_protect = 21;
static pin_size_t reset = 20;
static pin_size_t cs = 17;
static pin_size_t sck = 18;
static pin_size_t sdo = 19;
static pin_size_t sdi = 16;
static int cycle_count = 0;
static int xchange_error = 0;

//Global functions

void software_reset(){
  //TODO: Allow serial input from device to trigger this function
  digitalWrite(cs, LOW);
  SPI.transfer(RSTEN);
  SPI.transfer(RST);
  digitalWrite(cs, HIGH);
}

void hardware_reset(){
  //TODO: Allow serial input from device to trigger this function
  digitalWrite(reset, LOW);
  delay(10);
  digitalWrite(reset, HIGH);
}

void clear_screen(){
  //Clear the serial output screen
  Serial.write(27); //ESC command
  Serial.print("[2J"); //clear screen command
  Serial.write(27);
  Serial.print("[H"); //cursor to home command
}

//Read the status register to see executing commands, hold until commands have finished executing succesfully
void read_status_until_done() {
  /*
    Design for Reliability Note:
      From page 73 of the datasheet:
        'The host system can determine when a write, program, erase, suspend or other embedded operation is complete
        by monitoring the Write in Progress (WIP) bit in the Status Register.'

      Get the WIP bit from the read status register 1. Read from status register 2 to get the state of the program error (P_ERR) and erase error (E_ERR).
      Note that the WIP bit will remain 1 when the P_ERR or E_ERR bits are 1 and the device will remain busy. This can be cleared with the Software Reset, 
      Clear Staus Register or Hardware Reset pin.
  */
  uint8_t wip = 0;
  int i = 0;
  do{
    //Monitor the E_ERR Bit and WIP Bit in the Read Status Registers 
    digitalWrite(cs, LOW);
    SPI.transfer(RDSR1);
    uint8_t status1 = SPI.transfer(0x00); //Dummy Cycles to get info from SR1NV
    digitalWrite(cs, HIGH);
    wip = (status1 >> WIP) & 0x1;
    digitalWrite(cs, LOW);
    SPI.transfer(RDSR2);
    uint8_t status2 = SPI.transfer(0x00);
    digitalWrite(cs, HIGH);
    //Show the Erase Error Bit and the Work in Progress Bit from the status register
    //Serial.print("Status 1: ");
    //Serial.println(status1, BIN);
    Serial.print("WIP: ");
    Serial.println((status1 >> WIP) & 0x1, BIN);
    Serial.print("WEL: ");
    Serial.println((status1 >> WEL) & 0x1, BIN);
    //Serial.print("Status 2: ");
    //Serial.println(status2, BIN);
    Serial.print("E_ERR: ");
    Serial.println((status2 >> E_ERR) & 0x1, BIN); 
    Serial.print("P_ERR: ");
    Serial.println((status2 >> P_ERR) & 0x1, BIN);
    for(int j = 0; j < i; j++){
      Serial.print(".");
    }
    Serial.println("");
    i++;
    delay(500); //Wait 500ms then check again 
  }
  while(wip == 1);
}

void setup() {
  // Begin the Serial Port for Debugging
  Serial.begin(9600);

  //Begin the SPI Interface for comms with the NOR Flash
  SPI.begin();
  //Change SPI setting per datasheet, page 17
  //Change the SPI Settings to allow Clock Frequency of 10MHz (absolute max per datasheet is 133MHz)
  //Most Significant Bit First
  //Data Mode 0 (Clock Polarity = 0 and Clock Phase Angle = 0)
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  SPI.setCS(cs);  
  SPI.setSCK(sck);
  SPI.setRX(sdi);
  SPI.setTX(sdo);
  pinMode(cs, OUTPUT);
  //Disable write protect
  pinMode(write_protect, OUTPUT);
  digitalWrite(write_protect, HIGH);
  //Hold RESET low (Pulse high for reset)
  pinMode(reset, OUTPUT);
  digitalWrite(reset, HIGH);
  //On-board LED
  pinMode(LED_BUILTIN, OUTPUT);
  delay(3000);  //Give everything a moment

  //Test the interface by reading the Chip ID - report over serial
  /*
    The data format for the S25FL is as follows:

    | * * * instruction * * * | * * * 3-byte or 4-byte address or modifier * * * | * * * data * * * |
    
    Each section is an 8-bit word.

  */
  digitalWrite(cs, LOW);
  SPI.transfer(RDID); // Read Idenfification
  uint8_t manufacturer_id = SPI.transfer(0x00); // The Manufacturer ID 
  uint16_t device_id = (SPI.transfer(0x00) << 8) | SPI.transfer(0x00); // The device ID MSB, shifted 8 bits then or'ed with the LSB
  digitalWrite(cs, HIGH);
  delay(10);
  // Get the device unique ID
  digitalWrite(cs, LOW);
  SPI.transfer(RUID);
  //Four Dummy Bytes before Serial Number Readout
  for(int i = 0; i <= 4; i++){
    SPI.transfer(0x00);
  }  
  uint64_t device_uid = ((uint64_t)SPI.transfer(0x00) << 24) | ((uint64_t)SPI.transfer(0x00) << 16) | (SPI.transfer(0x00) << 8) | SPI.transfer(0x00);
  digitalWrite(cs, HIGH);
  Serial.print("Manufacturer ID: ");
  Serial.println(manufacturer_id, HEX);
  Serial.print("Device ID: ");
  Serial.println(device_id, HEX);
  Serial.print("Unique Device ID: ");
  Serial.println(device_uid, HEX);
  if (manufacturer_id == 0x01){
    digitalWrite(LED_BUILTIN, HIGH);
    if (device_id == 0x6018){
      Serial.println("S25FL - 128Mb");
    }
    else if (device_id == 0x6019){
      Serial.println("S25FL - 256Mb");
    }
    else{
      Serial.println("Unknown");
    }
  }
  else{
    Serial.println("Error: Incorrect Manufacturer ID? Please Power Cycle.");
    for(;;){}
  }
}

void loop() {
  //Loop through the different operational modes - READ, WRITE, ERASE, STANDBY
  Serial.print("\n-------- Cycle #");
  Serial.print(cycle_count);
  Serial.println(" --------");
  Serial.print("\nRead/Write Errors Seen: ");
  Serial.println(xchange_error);

  //STAGE 1: ERASE
  //Enable writes then erase The entire chip
  Serial.println("ERASE Command Check");
  digitalWrite(cs, LOW);
  SPI.transfer(WREN); //Write Enable (must be enabled before every command)
  digitalWrite(cs, HIGH);
  digitalWrite(cs, LOW); 
  SPI.transfer(CE);
  digitalWrite(cs, HIGH);
  read_status_until_done();
  Serial.println("Command Complete");
  Serial.println("------");

  /*
    Notes on Program Flash Array Section 8.5
      Page Programming allows up to a page size of 256 bytes in one operation

      Before the PP (Page Program) command can be accepted by the device a WREN (Write Enable) command must be issued.
      After recieing the enable the WEL (Write Enable Latch) in the status regsiter is set and operations are enabled.

      Page Program Structure:

      | * * * WREN * * * | * * * PP * * * | * * * adress byte 1 * * * | * * * adress byte 2 * * * | * * * adress byte 3 * * * | * * * data... * * * |

      This assumes one is using the 3-byte addressing mode, 4-byte addressing is also possible and has a different command structure. 
  */

  //STAGE 2: WRITE 
  //Here is two bytes to put into memory
  uint16_t remember_me = 0x4165;
  uint8_t address = 0x02; //Place to store the byte
  Serial.println("WRITE Command Check");
  digitalWrite(cs, LOW);  
  SPI.transfer(WREN); //Write Enable (must be enabled before every command)
  digitalWrite(cs, HIGH);
  digitalWrite(cs, LOW); 
  SPI.transfer(PP);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer(address);
  SPI.transfer16(remember_me);
  digitalWrite(cs, HIGH);
  read_status_until_done();
  Serial.println("Command Complete");
  Serial.println("------");
  
  //STAGE 3: READ 
  //Read back that memory 
  Serial.println("READ Command Check");
  digitalWrite(cs, LOW);
  SPI.transfer(READ);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer(address);
  uint16_t read_data = SPI.transfer16(0x0000);
  digitalWrite(cs, HIGH);
  read_status_until_done();
  Serial.print("Found Data: ");
  Serial.print(read_data, HEX);
  Serial.print(" @ ");
  Serial.println(address, HEX);
  if (read_data != remember_me){
    Serial.println("ERROR!!! Written and read bytes do not match");
    xchange_error++;
  }
  Serial.println("Command Complete");
  Serial.println("------");

  //STAGE 4: STANDBY
  Serial.println("STANDBY Check...");
  delay(5000);
  clear_screen();
  cycle_count++;

}
