#include "mbed.h"
#include "TTP229.h"
#include "MbedJSONValue.h"
#include "DHT.h"
#include <string>

using namespace std;

#define IAP_LOCATION 0x1FFF1FF1
typedef void (*IAP)(unsigned long [], unsigned long[] );
IAP iap_entry = (IAP) IAP_LOCATION;

MbedJSONValue boardData;

Ticker tempTicker;
Ticker energyTicker;
Ticker sensorDataTicker;

//MODULES COMMUNICATION
    TTP229 touchpad(P0_0, P0_1); // For Connecting Capacitive Touchpad using I2C
    Serial pc(P0_2, P0_3); // (USBTX, USBRX) Opens up serial communication through the USB port via the computer
    Serial xbeeSerial(P0_15, P0_16);

// LED LIGHTS ON BOARD
    DigitalOut heartbeatLED(P1_18); // LED1
    DigitalOut xbeeLED(P1_20); // LED2
    DigitalOut sensorLED(P1_21); // LED3
    DigitalOut led4(P1_23); // LED4

// DIGITAL SWITCHES
    DigitalOut DSw1(P0_9);
    DigitalOut DSw2(P0_8);
    DigitalOut DSw3(P0_7);
    DigitalOut DSw4(P0_6);
    DigitalOut DSw5(P0_18);
    DigitalOut DSw6(P0_17);
    DigitalOut DSw7(P0_23);

// ANALOG SWITCHES

    PwmOut ASw1(P2_5);  // Connect Potentiometer (Trimpot 10K with Knob)
    PwmOut ASw2(P2_4);  // Connect Potentiometer (Trimpot 10K with Knob)
    PwmOut ASw3(P2_3);  // Connect Potentiometer (Trimpot 10K with Knob)

    DHT temHumSensor(P1_30, DHT11);
    AnalogIn energySensor(P1_31);

// RGB LED PINS THAT CAN BE USED

    DigitalOut DSw1Led(P0_24);
    DigitalOut DSw2Led(P0_25);
    DigitalOut DSw3Led(P0_26);
    DigitalOut DSw4Led(P2_2);
    DigitalOut DSw5Led(P2_1);
    DigitalOut DSw6Led(P2_0);
    DigitalOut DSw7Led(P0_11); //p27 (Serial RX)
    DigitalOut ASw1Led(P0_10); //p28 (Serial TX)
    DigitalOut ASw2Led(P0_5);
    DigitalOut ASw3Led(P0_4);

void broadcastChange(std::string command){
    command = command + "\n";
    printf("\nBroadcast = %s\r\n" ,  command.c_str());
    xbeeSerial.puts(command.c_str());
    xbeeLED = 1;
    wait(0.5);
    xbeeLED = 0;
}

void refreshMyStatus(){
  boardData["DSw1"] = 0;
  boardData["DSw2"] = 0;
  boardData["DSw3"] = 0;
  boardData["DSw4"] = 0;
  boardData["DSw5"] = 0;
  boardData["DSw6"] = 0;
  boardData["DSw7"] = 0;
  boardData["ASw1_dval"] = 0;
  boardData["ASw1_aval"] = 0;
  boardData["ASw2_dval"] = 0;
  boardData["ASw2_aval"] = 0;
  boardData["ASw3_dval"] = 0;
  boardData["ASw3_aval"] = 0;

  boardData["temp"] = 0;
  boardData["hum"] = 0;
  boardData["energy"] = 0.00;

  MbedJSONValue command;
  command["id"] = boardData["id"];
  command["type"] = "status";
  std::string str = command.serialize();
  broadcastChange(str);
  //  printf("Ask Gateway to refresh board status for all switches\n\n");
}

void readTempHumidityData(){

    float temperature;
    float temperature_f;
    float humidity;
    float dewpoint;

    temHumSensor.readData();

    temperature = temHumSensor.ReadTemperature(CELCIUS);
    temperature_f = temHumSensor.ReadTemperature(FARENHEIT);
    humidity = temHumSensor.ReadHumidity();
    dewpoint = temHumSensor.CalcdewPointFast(temperature, humidity);

    printf("\r\n");
    printf("Temperature: %f C / %f F\r\n", temperature, temperature_f);
    printf("Humidity: %f%%\r\n", humidity);
    printf("Dewpoint: %f C / %f F\r\n", dewpoint, (dewpoint * 1.8f) + 32);

    boardData["temp"] = temperature;
    boardData["hum"] = humidity;

    sensorLED = 1;

}

void readEnergyConsumption(){
    boardData["energy"] = boardData["energy"].get<double>() + 0.31;
}

void sendSensorData(){
    MbedJSONValue command;
    command["id"] = boardData["id"];
    command["energy"] = boardData["energy"];
    command["temp"] = boardData["temp"];
    command["hum"] = boardData["hum"];
    std::string str = command.serialize();
    broadcastChange(str);
    // boardData["energy"] = 0.00;
    // boardData["temp"] = 0.00;
    // boardData["hum"] = 0.00;
}

void readNSaveSensorsData(){
    tempTicker.attach(&readTempHumidityData, 3.0);
    energyTicker.attach(&readEnergyConsumption, 1.0);
    sensorDataTicker.attach(&sendSensorData, 5.0);
}

void switchTouched(){
//    printf("%16s\r\n",to_string(touchpad).c_str());
    int8_t key = touchpad.onkey();
    int dv;
    int av;
    // printf("%d\r\n", key);
    //int sw=touchpad.getsingle();
    //if(sw!=0) myleds=sw%16;
    MbedJSONValue command;
    command["id"] = boardData["id"];
    command["type"] = "sb";
    switch(key){
        case 1:
            DSw1 = !DSw1;
            command["index"] = key;
            command["dv"] = DSw1;
            break;
        case 2:
            DSw2 = !DSw2;
            command["index"] = key;
            command["dv"] = DSw2;
            break;
        case 3:
            DSw3 = !DSw3;
            command["index"] = key;
            command["dv"] = DSw3;
            break;
        case 4:
            DSw4 = !DSw4;
            command["index"] = key;
            command["dv"] = DSw4;
            break;
        case 5:
            DSw5 = !DSw5;
            command["index"] = key;
            command["dv"] = DSw5;
            break;
        case 6:
            DSw6 = !DSw6;
            command["index"] = key;
            command["dv"] = DSw6;
            break;
        case 7:
            DSw7 = !DSw7;
            command["index"] = key;
            command["dv"] = DSw7;
            break;
        case 8:
            command["index"] = 8;
            av = boardData["ASw1_aval"].get<int>();
            if(av < 10){
              av = av + 1;
              command["av"] = av;
              boardData["ASw1_aval"] = av;
              ASw1.write(av);
            }else{
              key = 0; // DO NOTHING
            }
            break;
        case 9:
            command["index"] = 8;
            dv = boardData["ASw1_dval"].get<int>();
            if(dv > 0){
              dv = 0;
            }else{
              dv = 1;
            }
            ASw1 = dv;
            command["dv"] = dv;
            boardData["ASw1_dval"] = command["dv"];
            break;
        case 10:
            command["index"] = 8;
            av = boardData["ASw1_aval"].get<int>();
            if(av > 0){
              av = av - 1;
              command["av"] = av;
              boardData["ASw1_aval"] = av;
              ASw1.write(av);
            }else{
              key = 0; // DO NOTHING
            }
            break;
        case 11:
            command["index"] = 9;
            av = boardData["ASw2_aval"].get<int>();
            if(av < 10){
              av = av + 1;
              command["av"] = av;
              boardData["ASw2_aval"] = av;
              ASw2.write(av);
            }else{
              key = 0; // DO NOTHING
            }
            break;
        case 12:
            command["index"] = 9;
            dv = boardData["ASw2_dval"].get<int>();
            if(dv > 0){
              dv = 0;
            }else{
              dv = 1;
            }
            ASw2.write(dv);
            command["dv"] = dv;
            boardData["ASw2_dval"] = command["dv"];
            break;
        case 13:
            command["index"] = 9;
            av = boardData["ASw2_aval"].get<int>();
            if(av > 0){
              av = av - 1;
              command["av"] = av;
              boardData["ASw2_aval"] = av;
              ASw2.write(av);
            }else{
              key = 0; // DO NOTHING
            }
            break;
        case 14:
            command["index"] = 10;
            av = boardData["ASw3_aval"].get<int>();
            if(av < 10){
              av = av + 1;
              command["av"] = av;
              boardData["ASw3_aval"] = av;
              ASw3.write(av);
            }else{
              key = 0; // DO NOTHING
            }
            break;
        case 15:
            command["index"] = 10;
            dv = boardData["ASw3_dval"].get<int>();
            if(dv > 0){
              dv = 0;
            }else{
              dv = 1;
            }
            ASw3 = dv;
            command["dv"] = dv;
            boardData["ASw3_dval"] = command["dv"];
            break;
        case 16:
            command["index"] = 10;
            av = boardData["ASw3_aval"].get<int>();
            if(av > 0){
              av = av - 1;
              command["av"] = av;
              boardData["ASw3_aval"] = av;
              ASw3.write(av);
            }else{
              key = 0; // DO NOTHING
            }
            break;
    }

    if(key > 0){
        std::string str = command.serialize();
        broadcastChange(str);
    }
}

void setDeviceId(){
    unsigned long comm[5] = {0,0,0,0,0};
    unsigned long result[5] = {0,0,0,0,0};
    comm[0] = 58;  // read device serial number
    printf("\r\nSerial number:\r\n");
    iap_entry(comm, result);
    boardData["id"] = "";
    char tmpbuf[256];
    if (result[0] == 0) {
        printf("\nSerial Number: ");
        sprintf(tmpbuf,"%08X%08X",result[0], result[1]);
        printf(tmpbuf);
        sprintf(tmpbuf,"%08X%08X", result[2], result[3]);
        printf(tmpbuf);
        boardData["id"] = tmpbuf;
        /*
        std::string temp = "";
        for(int i = 1; i < 5; i++) {
           // printf( "0x%x\r\n", result[i] );
            unsigned char *s=(unsigned char *)&result[i];
            char buffer [10];
            sprintf (buffer, "%lu" , result[i] );
            temp = temp + buffer;
        }
        command["id"] = temp;
        */
    } else {
        printf("Status error!\r\n");
    }
}

void handleDataReceived(char data[128]){
    pc.puts(data);
}

// main() runs in its own thread in the OS
int main() {
    setDeviceId();
    refreshMyStatus();
    readNSaveSensorsData();
    touchpad.attach(&switchTouched);

    while (true) {
        sensorLED = 0;
        heartbeatLED = 1;
        wait(0.5);
        heartbeatLED = 0;
        wait(0.5);

        if (pc.readable()) {//Checking for serial comminication
            xbeeSerial.putc(pc.getc()); //XBee write whatever the PC is sending
        }

        char value[128];
        int index=0;
        char ch;

        do
        {
           if (xbeeSerial.readable()){      // if there is an character to read from the device
              ch = xbeeSerial.getc();   // read it
              if (index<128)               // just to avoid buffer overflow
                 value[index++]=ch;  // put it into the value array and increment the index
          }
        } while (ch!='\n');    // loop until the '\n' character

        value[index]='\x0';  // add un 0 to end the c string
        handleDataReceived(value);

    }
}