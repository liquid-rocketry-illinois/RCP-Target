#ifndef RCP_HOST_H
#define RCP_HOST_H

#include <cstdint>

#include "LRIRingBuf.h"
#include "procedures.h"
#include "VERSION.h"

// This do-while(0) thing is horrible but if the linux kernel can do it so can I
#define RCPDebug(str)                                                                                                  \
do {                                                                                                                   \
static_assert(sizeof str < 64, "RCP Debug string too long!");                                                          \
RCP::RCPWriteSerialString(str "\n");                                                                                   \
} while(0)

typedef enum {
    RCP_CH_ZERO = 0x00,
    RCP_CH_ONE = 0x40,
    RCP_CH_TWO = 0x80,
    RCP_CH_THREE = 0xC0,
    RCP_CHANNEL_MASK = 0xC0,
} RCP_Channel;

typedef enum {
    RCP_DEVCLASS_TEST_STATE = 0x00,
    RCP_DEVCLASS_SIMPLE_ACTUATOR = 0x01,
    RCP_DEVCLASS_STEPPER = 0x02,
    RCP_DEVCLASS_PROMPT = 0x03,
    RCP_DEVCLASS_ANGLED_ACTUATOR = 0x04,
    RCP_DEVCLASS_CUSTOM = 0x80,

    RCP_DEVCLASS_AM_PRESSURE = 0x90,
    RCP_DEVCLASS_AM_TEMPERATURE = 0x91,
    RCP_DEVCLASS_PRESSURE_TRANSDUCER = 0x92,
    RCP_DEVCLASS_RELATIVE_HYGROMETER = 0x93,
    RCP_DEVCLASS_LOAD_CELL = 0x94,
    RCP_DEVCLASS_BOOL_SENSOR = 0x95,

    RCP_DEVCLASS_POWERMON = 0xA0,

    RCP_DEVCLASS_ACCELEROMETER = 0xB0,
    RCP_DEVCLASS_GYROSCOPE = 0xB1,
    RCP_DEVCLASS_MAGNETOMETER = 0xB2,

    RCP_DEVCLASS_GPS = 0xC0,
} RCP_DeviceClass;

typedef enum {
    RCP_TEST_START = 0x00,
    RCP_TEST_STOP = 0x10,
    RCP_TEST_PAUSE = 0x11,
    RCP_DEVICE_RESET = 0x12,
    RCP_DEVICE_RESET_TIME = 0x13,
    RCP_DATA_STREAM_STOP = 0x20,
    RCP_DATA_STREAM_START = 0x21,
    RCP_TEST_QUERY = 0x30,
    RCP_HEARTBEATS_CONTROL = 0xF0
} RCP_TestStateControlMode;

typedef enum {
    RCP_TEST_RUNNING = 0x00,
    RCP_DEVICE_INITED_MASK = 0x10,
    RCP_TEST_STOPPED = 0x20,
    RCP_TEST_PAUSED = 0x40,
    RCP_TEST_ESTOP = 0x60,
    RCP_TEST_STATE_MASK = 0x60,
    RCP_DATA_STREAM_MASK = 0x80,
    RCP_HEARTBEAT_TIME_MASK = 0x0F,
} RCP_TestRunningState;

typedef enum {
    RCP_SIMPLE_ACTUATOR_OFF = 0x00,
    RCP_SIMPLE_ACTUATOR_ON = 0x80,
    RCP_SIMPLE_ACTUATOR_TOGGLE = 0xC0,
} RCP_SimpleActuatorState;

typedef enum {
    RCP_STEPPER_ABSOLUTE_POS_CONTROL = 0x40,
    RCP_STEPPER_RELATIVE_POS_CONTROL = 0x80,
    RCP_STEPPER_SPEED_CONTROL = 0xC0,
} RCP_StepperControlMode;

typedef enum {
    RCP_PromptDataType_GONOGO = 0x00,
    RCP_PromptDataType_Float = 0x01,
    RCP_PromptDataType_RESET = 0xFF,
} RCP_PromptDataType;

typedef enum {
    RCP_GONOGO_NOGO = 0x00,
    RCP_GONOGO_GO = 0x01,
} RCP_GONOGO;

namespace RCP {
    union PromptData {
        bool boolData;
        float floatData;
    };

    static_assert(sizeof(PromptData) == 4, "PromptData size does not equal 4!");

    using PromptAcceptor = void (*)(const PromptData& promptData);

    template<size_t NUM_FLOATS>
    struct Floats {
        float vals[NUM_FLOATS];
    };

    using Floats2 = Floats<2>;
    using Floats3 = Floats<3>;
    using Floats4 = Floats<4>;

    constexpr int SERIAL_BYTES_PER_LOOP = 20;
    constexpr int RCP_SERIAL_BUFFER_SIZE = 128;

    extern RCP_Channel channel;
    extern Test::Procedure* ESTOP_PROC;

    // extern LRI::RingBuf<uint8_t, RCP_SERIAL_BUFFER_SIZE> inbuffer;


    // extern bool dataStreaming;
    // extern uint8_t testNum;
    // extern RCP_TestRunningState testState;
    // extern bool ready;
    // extern uint8_t promptValue[4];
    // extern uint32_t timeOffset;

    void init();
    void yield();
    void runTest();
    [[noreturn]] void systemReset();

    void sendTestState();
    [[noreturn]] void ESTOP();
    void RCPWriteSerialString(const char* str);

    void setReady(bool newready);
    void setPrompt(const char* str, RCP_PromptDataType gng, PromptAcceptor acceptor);
    void resetPrompt();

    bool getDataStreaming();
    uint8_t getTestNum();
    uint32_t millis();

    void sendOneFloat(RCP_DeviceClass devclass, uint8_t id, float value);

    void sendTwoFloat(RCP_DeviceClass devclass, uint8_t id, const float value[2]);
    inline void sendTwoFloat(RCP_DeviceClass devclass, uint8_t id, Floats2 floats) {
        sendTwoFloat(devclass, id, floats.vals);
    }

    void sendThreeFloat(RCP_DeviceClass devclass, uint8_t id, const float value[3]);
    inline void sendThreeFloat(RCP_DeviceClass devclass, uint8_t id, Floats3 floats) {
        sendThreeFloat(devclass, id, floats.vals);
    }

    void sendFourFloat(RCP_DeviceClass devclass, uint8_t id, const float value[4]);
    inline void sendFourFloat(RCP_DeviceClass devclass, uint8_t id, Floats4 floats) {
        sendFourFloat(devclass, id, floats.vals);
    }

    void write(const void* data, uint8_t length);
    uint8_t readAvail();
    uint8_t read();
    uint32_t systime();

    RCP_SimpleActuatorState readSimpleActuator(uint8_t id);
    RCP_SimpleActuatorState writeSimpleActuator(uint8_t id, RCP_SimpleActuatorState state);

    Floats2 readStepper(uint8_t id);
    Floats2 writeStepper(uint8_t id, RCP_StepperControlMode controlMode, float controlVal);

    float readAngledActuator(uint8_t id);
    float writeAngledActuator(uint8_t id, float controlVal);

    Floats4 readSensor(RCP_DeviceClass devclass, uint8_t id);
    void writeSensorTare(RCP_DeviceClass devclass, uint8_t id, uint8_t dataChannel, float tareVal);

    void handleCustomData(const void* data, uint8_t length);
} // namespace RCP


#endif // RCP_HOST_H
