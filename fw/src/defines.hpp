#define CS_MCP3202 5
#define PIN_SDA 21
#define PIN_SCL 22
#define MOT_1A 16
#define MOT_1B 4
#define MOT_2A 2
#define MOT_2B 15 
#define ADC_CHANNEL_BAT 1 
#define ADC_CHANNEL_HEAT 0
#define VIB_SW 12
#define ENC_A 34 
#define ENC_B 35
#define ENC_SW 32
#define MOV_SW 14
#define HEAT_PWM 13
#define PIEZO_A 33
#define PIEZO_B 25
#define SHUTDOWN 17

const double A = 3.9083E-3;
const double B = -5.775E-7;
const double R0 = 1000.0;
const double RES = 4096.0;
const double I = 0.000475;
const double Uref = 3.3;
const double defined_a = (-R0*A);
const double defined_b = R0*R0*A*A;
const double defined_c = 4*R0*B;
const double defined_d = 2*R0*B;
const double defined_e = RES*I;