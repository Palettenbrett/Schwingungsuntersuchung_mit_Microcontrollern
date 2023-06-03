// Tensorflow imports
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

//Model Import
#include "Model_Container/ESP32_Prediction_Model.h"

#include <Wire.h>
#define MPU_ADDRESS 0x68 // MPU6050 I2C address

namespace {
  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  TfLiteTensor* output = nullptr;

  // Create Memory arena for calculations
  constexpr int kTensorArenaSize = 12 * 1024;
  uint8_t tensor_arena[kTensorArenaSize];

}

void setup(){

  // Set up the MPU-6050 --------------------------------------------
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

  // Set up Tensorflow ----------------------------------------------
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter; 

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model = tflite::GetModel(ESP32_Prediction_Model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  // This pulls in all the operation implementations we need.
  // NOLINTNEXTLINE(runtime-global-variables)
   static tflite::MicroMutableOpResolver<2> micro_op_resolver(error_reporter);
  if (micro_op_resolver.AddTanh() != kTfLiteOk) {
    return;
  }
  if (micro_op_resolver.AddFullyConnected() != kTfLiteOk) {
    return;
  }

  // Build an interpreter to run the model with.
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }

  // Get input and output pointers
  input = interpreter->input(0);
  output = interpreter->output(0);

}

const float Acc_Const = 0.000061035;
const float Gyr_Const = 0.007629394;

int16_t accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z;

float Sum = 0.0;
float MAE = 0.0;
uint16_t Pos_Index = 0;

float Acc_X_Vec[128];
float Acc_Y_Vec[128];
float Acc_Z_Vec[128];
float Gyr_X_Vec[128];
float Gyr_Y_Vec[128];
float Gyr_Z_Vec[128];

void loop(){

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
    input->data.int8[i] = Acc_X_Vec[i] / input->params.scale + input->params.zero_point;
  }

  for (int i=128; i<128*2; i++){
    input->data.int8[i] = Acc_Y_Vec[i-(128)] / input->params.scale + input->params.zero_point;
  }

  for (int i=128*2; i<128*3; i++){
    input->data.int8[i] = Acc_Z_Vec[i-(128*2)] / input->params.scale + input->params.zero_point;
  }

  for (int i=128*3; i<128*4; i++){
    input->data.int8[i] = Gyr_X_Vec[i-(128*3)] / input->params.scale + input->params.zero_point;
  }

  for (int i=128*4; i<128*5; i++){
    input->data.int8[i] = Gyr_Y_Vec[i-(128*4)] / input->params.scale + input->params.zero_point;
  }

  for (int i=128*5; i<128*6; i++){
    input->data.int8[i] = Gyr_Z_Vec[i-(128*5)] / input->params.scale + input->params.zero_point;
  }

  TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk) {
          Serial.print("Invoke failed!");
          while (1);
          return;
        }

  for (int i=0; i<128; i++){
    Sum += abs(Acc_X_Vec[i] - float((output->data.int8[i] - output->params.zero_point) * output->params.scale));
    Sum += abs(Acc_Y_Vec[i] - float((output->data.int8[i+128] - output->params.zero_point) * output->params.scale));
    Sum += abs(Acc_Z_Vec[i] - float((output->data.int8[i+128*2] - output->params.zero_point) * output->params.scale));
    Sum += abs(Gyr_X_Vec[i] - float((output->data.int8[i+128*3] - output->params.zero_point) * output->params.scale));
    Sum += abs(Gyr_Y_Vec[i] - float((output->data.int8[i+128*4] - output->params.zero_point) * output->params.scale));
    Sum += abs(Gyr_Z_Vec[i] - float((output->data.int8[i+128*5] - output->params.zero_point) * output->params.scale));
  }

  MAE = Sum/(128*6);

  Serial.printf("%.5f", MAE);
  Serial.print("\n");

  Pos_Index = 0;
  Sum = 0.0;
  MAE = 0.0;

}