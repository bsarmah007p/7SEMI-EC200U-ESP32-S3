// ESP32  Serial1 test - for loopback test connect pins 16 and 17

//XD1 is defined as pin 12, used as the receive pin for UART1.
//TXD1 is defined as pin 13, used as the transmit pin for UART1.
#define RXD1 12
#define TXD1 13
void setup() {
  
  // initialize both serial ports:
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
  Serial.println();
  Serial.println("\n\nESP32 serial1 test Rx pin 16 Tx pin 17");
  Serial.write("for loopback test connect pin 16 to pin 17\n");
}

void loop() {
  // read from port 1, send to port 0:
  if (Serial1.available()) { // Checks if there is any data available to read on URAT1.
    int inByte = Serial1.read(); // Reads the byte from UART1
    Serial.write(inByte); // Sends the byte to UART0 (your computer).
  }

  // read from port 0, send to port 1:
  if (Serial.available()) { // Checks if there is any data available to read from the USB serial port
    int inByte = Serial.read(); // Reads the byte from URAT0 (USB Connection)
    //Serial.write(inByte);
    Serial1.write(inByte);
  }
}