// Conditional compilation of driver
#ifdef USE_MQ2

// Define driver ID
#define XSNS_81                     81

#define ANALOG_PIN                  A0
#define RL_VALUE                    5     //define the load resistance on the board, in kilo ohms
#define RO_CLEAN_AIR_FACTOR         9.83f
#define CALIBRAION_SAMPLE_TIMES     5
#define CALIBRATION_SAMPLE_INTERVAL 50
#define READ_SAMPLE_INTERVAL        50
#define READ_SAMPLE_TIMES           5

#define GAS_CO 0
#define GAS_LPG 1
#define GAS_SMOKE 2

bool Mq2Detected = 0;

float LPGCurve[3]  =  {2.3,0.21,-0.47};
float COCurve[3]  =  {2.3,0.72,-0.34};
float SmokeCurve[3] = {2.3,0.53,-0.44};
float Ro = 10.0;

const char mq2Name[] PROGMEM = "MQ2";

float Mq2ResistanceCalculation(uint32_t raw)
{
    return (((float)RL_VALUE * (1023 - raw) / raw));
}

float Mq2Calibration()
{
    float val = 0;

    for (int i = 0; i < CALIBRAION_SAMPLE_TIMES; i++) {            //take multiple samples
        val += Mq2ResistanceCalculation(analogRead(ANALOG_PIN));
        delay(CALIBRATION_SAMPLE_INTERVAL);
    }
    val = val / CALIBRAION_SAMPLE_TIMES;                   //calculate the average value

    val = val / RO_CLEAN_AIR_FACTOR;                        //divided by RO_CLEAN_AIR_FACTOR yields the Ro 
                                                            //according to the chart in the datasheet 
    return val;
}

int Mq2GetPercentage(float &rs_ro_ratio, float *pcurve)
{
  return (pow(10,(((log(rs_ro_ratio)-pcurve[1])/pcurve[2]) + pcurve[0])));
}

float Mq2GetGasPercentage(float rs_ro_ratio, uint8_t gas_id)
{
    if (gas_id == GAS_LPG) {
        return Mq2GetPercentage(rs_ro_ratio, LPGCurve);
    } else if ( gas_id == GAS_CO ) {
        return Mq2GetPercentage(rs_ro_ratio, COCurve);
    } else if ( gas_id == GAS_SMOKE ) {
        return Mq2GetPercentage(rs_ro_ratio, SmokeCurve);
    }
    return 0;
}

float Mq2ReadRaw()
{
    uint32_t i;
    float rs = 0;
    uint32_t raw = analogRead(ANALOG_PIN);

    for (i = 0; i < READ_SAMPLE_TIMES; i++) {
        rs += Mq2ResistanceCalculation(raw);
        delay(READ_SAMPLE_INTERVAL);
    }

    rs = rs / READ_SAMPLE_TIMES;
    return rs;
}

bool Mq2Read(float &co, float &lpg, float &smoke)
{
    co = Mq2GetGasPercentage((Mq2ReadRaw() / Ro), GAS_CO);
    lpg = Mq2GetGasPercentage((Mq2ReadRaw() / Ro), GAS_LPG);
    smoke = Mq2GetGasPercentage((Mq2ReadRaw() / Ro), GAS_SMOKE);

    return (!isnan(co) && !isnan(lpg) && !isnan(smoke));
}

void Mq2Detect(void)
{
    if(Mq2Detected){return;}
    else if(analogRead(ANALOG_PIN) >= 1){
        Ro = Mq2Calibration();
        Mq2Detected = 1;
    }
}

void Mq2Show(bool json)
{
    float co;
    float lpg;
    float smoke;
    // char name[4] = "MQ2";
    if(Mq2Read(co, lpg, smoke)){
        CoLpgSmokeShow(json, ((0 == TasmotaGlobal.tele_period)), mq2Name, co, lpg, smoke);
    }
}

bool Xsns81(uint8_t callback_id) {

  // Set return value to `false`
  bool result = false;

  // Check which callback ID is called by Tasmota
  switch (callback_id) {
    case FUNC_INIT:
        Mq2Detect();
        break;
    case FUNC_JSON_APPEND:
        Mq2Show(1);
        break;
#ifdef USE_WEBSERVER
    case FUNC_WEB_SENSOR:
        Mq2Show(0);
        break;
#endif // USE_WEBSERVER
  }

  // Return boolean result
  return result;
}

#endif // USE_MQ2