#include <Wire.h>

#define MPU_ADDRESS 0x68 // MPU6050 I2C address
const float Acc_Const = 0.000061035;
const float Gyr_Const = 0.007629394;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // 21 22 with node
  Wire.setClock(400000);
  
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // Set internal clock to 8MHz
  Wire.endTransmission(true);
  
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(0x1C); // ACCEL_CONFIG register
  Wire.write(0x00); // set to +/- 2g
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(0x1B); // GYRO_CONFIG register
  Wire.write(0x00); // set to +/- 250 degrees per second
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(0x19); // SMPLRT_DIV
  Wire.write(7); // set Sample Rate to 1kHz
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(0x1A); // CONFIG register
  Wire.write(0x00); // Disable Filters
  Wire.endTransmission(true);
}

int16_t accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z;
uint16_t Pos_Index = 0;

float Acc_X_Vec[128];
float Acc_Y_Vec[128];
float Acc_Z_Vec[128];
float Gyr_X_Vec[128];
float Gyr_Y_Vec[128];
float Gyr_Z_Vec[128];

void loop() {

  while (Pos_Index<128){

    //Read accelerometer data
    Wire.beginTransmission(MPU_ADDRESS);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDRESS, 6, true);
    
    accel_x = (Wire.read() << 8) | Wire.read();
    accel_y = (Wire.read() << 8) | Wire.read();
    accel_z = (Wire.read() << 8) | Wire.read();

    Acc_X_Vec[Pos_Index] = (accel_x * Acc_Const)*0.5;
    Acc_Y_Vec[Pos_Index] = (accel_y * Acc_Const)*0.5;
    Acc_Z_Vec[Pos_Index] = (accel_z * Acc_Const)*0.5;

    // Read gyroscope data
    Wire.beginTransmission(MPU_ADDRESS);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDRESS, 6, true);

    gyro_x = (Wire.read() << 8) | Wire.read();
    gyro_y = (Wire.read() << 8) | Wire.read();
    gyro_z = (Wire.read() << 8) | Wire.read();

    Gyr_X_Vec[Pos_Index] = (gyro_x * Gyr_Const)*0.004;
    Gyr_Y_Vec[Pos_Index] = (gyro_y * Gyr_Const)*0.004;
    Gyr_Z_Vec[Pos_Index] = (gyro_z * Gyr_Const)*0.004;

    Pos_Index++;
    delay(1);

  }
  
  for (int i=0; i<128; i++){
    Serial.printf("%.5f",Acc_X_Vec[i]);
    Serial.print(",");
  }

  for (int i=0; i<128; i++){
    Serial.printf("%.5f",Acc_Y_Vec[i]);
    Serial.print(",");
  }

  for (int i=0; i<128; i++){
    Serial.printf("%.5f",Acc_Z_Vec[i]);
    Serial.print(",");
  }

  for (int i=0; i<128; i++){
    Serial.printf("%.5f",Gyr_X_Vec[i]);
    Serial.print(",");
  }

  for (int i=0; i<128; i++){
    Serial.printf("%.5f",Gyr_Y_Vec[i]);
    Serial.print(",");
  }

  for (int i=0; i<128; i++){
    Serial.printf("%.5f",Gyr_Z_Vec[i]);
    if (i!=127){
      Serial.print(",");
    }
  }

  Serial.print("\n");

  Pos_Index = 0;
  
}