#include "fixtures.h"
#include "gtest/gtest.h"

#include "RCP_Target/procedures.h"

RCPRawTest* context;

namespace RCP {
    void write(const void* rdata, uint8_t length) {
        const auto* data = static_cast<const uint8_t*>(rdata);
        int i = 0;
        for(; i < length && !OUT.isFull(); i++) {
            OUT.push(data[i]);
        }
    }

    uint8_t readAvail() { return IN.size(); }

    uint8_t read() {
        uint8_t pop = 0;
        IN.pop(pop);
        return pop;
    }

    uint32_t systime() { return SYSTIME; }

    bool readSimpleActuator(uint8_t id) { return ACTS[id] ? RCP_SIMPLE_ACTUATOR_ON : RCP_SIMPLE_ACTUATOR_OFF; }

    bool writeSimpleActuator(uint8_t id, RCP_SimpleActuatorState state) {
        if(state == RCP_SIMPLE_ACTUATOR_TOGGLE) ACTS[id] = !ACTS[id];
        else ACTS[id] = state;
        return ACTS[id];
    }

    Floats2 readStepper(uint8_t id) { return {STEPS[id][0], STEPS[id][1]}; }

    Floats2 writeStepper(uint8_t id, RCP_StepperControlMode controlMode, float controlVal) {
        switch(controlMode) {
        case RCP_STEPPER_SPEED_CONTROL:
            STEPS[id][1] = controlVal;
            break;

        case RCP_STEPPER_ABSOLUTE_POS_CONTROL:
            STEPS[id][0] = controlVal;
            break;

        case RCP_STEPPER_RELATIVE_POS_CONTROL:
            STEPS[id][0] += controlVal;
        }

        return readStepper(id);
    }

    float readAngledActuator(uint8_t id) { return ANGACT[id]; }

    float writeAngledActuator(uint8_t id, float controlVal) {
        ANGACT[id] = controlVal;
        return controlVal;
    }

    Floats4 readSensor(RCP_DeviceClass devclass, uint8_t id) { return {SENSE[0], SENSE[1], SENSE[2], SENSE[3]}; }

    bool readBoolSensor(uint8_t id) { return SENSEB; }

    void writeSensorTare(RCP_DeviceClass devclass, uint8_t id, uint8_t dataChannel, float tareVal) {
        SENSE[dataChannel] += tareVal;
    }

    void handleCustomData(const void* data, uint8_t length) {
        memcpy(RAWDATA, data, length);
        RAWLEN = length;
    }

    void systemReset() { std::exit(8675309); }
} // namespace RCP

void pacceptor(const RCP::PromptData& data) { PROMPT = data; }

TEST(RCPConstants, Constants) {
    // Channels
    EXPECT_EQ(RCP_CH_ZERO, 0x00);
    EXPECT_EQ(RCP_CH_ONE, 0x40);
    EXPECT_EQ(RCP_CH_TWO, 0x80);
    EXPECT_EQ(RCP_CH_THREE, 0xC0);

    // Device classes
    EXPECT_EQ(RCP_DEVCLASS_TEST_STATE, 0x00);
    EXPECT_EQ(RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x01);
    EXPECT_EQ(RCP_DEVCLASS_STEPPER, 0x02);
    EXPECT_EQ(RCP_DEVCLASS_PROMPT, 0x03);
    EXPECT_EQ(RCP_DEVCLASS_ANGLED_ACTUATOR, 0x04);
    EXPECT_EQ(RCP_DEVCLASS_CUSTOM, 0x80);
    EXPECT_EQ(RCP_DEVCLASS_AM_PRESSURE, 0x90);
    EXPECT_EQ(RCP_DEVCLASS_AM_TEMPERATURE, 0x91);
    EXPECT_EQ(RCP_DEVCLASS_PRESSURE_TRANSDUCER, 0x92);
    EXPECT_EQ(RCP_DEVCLASS_RELATIVE_HYGROMETER, 0x93);
    EXPECT_EQ(RCP_DEVCLASS_LOAD_CELL, 0x94);
    EXPECT_EQ(RCP_DEVCLASS_BOOL_SENSOR, 0x95);
    EXPECT_EQ(RCP_DEVCLASS_POWERMON, 0xA0);
    EXPECT_EQ(RCP_DEVCLASS_ACCELEROMETER, 0xB0);
    EXPECT_EQ(RCP_DEVCLASS_GYROSCOPE, 0xB1);
    EXPECT_EQ(RCP_DEVCLASS_MAGNETOMETER, 0xB2);
    EXPECT_EQ(RCP_DEVCLASS_GPS, 0xC0);

    // Test state values
    EXPECT_EQ(RCP_TEST_START, 0x00);
    EXPECT_EQ(RCP_TEST_STOP, 0x10);
    EXPECT_EQ(RCP_TEST_PAUSE, 0x11);
    EXPECT_EQ(RCP_DEVICE_RESET, 0x12);
    EXPECT_EQ(RCP_DEVICE_RESET_TIME, 0x13);
    EXPECT_EQ(RCP_DATA_STREAM_STOP, 0x20);
    EXPECT_EQ(RCP_DATA_STREAM_START, 0x21);
    EXPECT_EQ(RCP_TEST_QUERY, 0x30);
    EXPECT_EQ(RCP_HEARTBEATS_CONTROL, 0xF0);

    EXPECT_EQ(RCP_DATA_STREAM_MASK, 0x80);
    EXPECT_EQ(RCP_TEST_STATE_MASK, 0x60);
    EXPECT_EQ(RCP_DEVICE_INITED_MASK, 0x10);
    EXPECT_EQ(RCP_HEARTBEAT_TIME_MASK, 0x0F);

    EXPECT_EQ(RCP_TEST_RUNNING, 0x00);
    EXPECT_EQ(RCP_TEST_STOPPED, 0x20);
    EXPECT_EQ(RCP_TEST_PAUSED, 0x40);
    EXPECT_EQ(RCP_TEST_ESTOP, 0x60);

    // Simple actuator values
    EXPECT_EQ(RCP_SIMPLE_ACTUATOR_OFF, 0x00);
    EXPECT_EQ(RCP_SIMPLE_ACTUATOR_ON, 0x80);
    EXPECT_EQ(RCP_SIMPLE_ACTUATOR_TOGGLE, 0xC0);

    // Stepper control mode values
    EXPECT_EQ(RCP_STEPPER_ABSOLUTE_POS_CONTROL, 0x40);
    EXPECT_EQ(RCP_STEPPER_RELATIVE_POS_CONTROL, 0x80);
    EXPECT_EQ(RCP_STEPPER_SPEED_CONTROL, 0xC0);

    // Prompt values
    EXPECT_EQ(RCP_PromptDataType_GONOGO, 0x00);
    EXPECT_EQ(RCP_PromptDataType_Float, 0x01);
    EXPECT_EQ(RCP_PromptDataType_RESET, 0xFF);

    EXPECT_EQ(RCP_GONOGO_NOGO, 0x00);
    EXPECT_EQ(RCP_GONOGO_GO, 0x01);
}

TEST_F(RCPRawTest, NonInit) {
    PUSH(0x00, RCP_DEVCLASS_TEST_STATE, 0x30);
    RCP::yield();
    EXPECT_EQ(OUT.size(), 0);
}

TEST_F(RCPRawTest, InitMessage) {
    RCP::init();
    RCP::setReady(true);
    CHECK_OUTBUF(0x05, RCP_DEVCLASS_TEST_STATE, 0x00, 0x00, 0x00, 0x00, 0x30);
    OUT.clear();
    RCP::setReady(false);
    CHECK_OUTBUF(0x05, RCP_DEVCLASS_TEST_STATE, 0x00, 0x00, 0x00, 0x00, 0x20);
}

TEST_F(RCPTest, WrongChannel) {
    PUSH(0x81, RCP_DEVCLASS_TEST_STATE, 0x30);
    RCP::yield();
    EXPECT_EQ(OUT.size(), 0);
}

TEST_F(RCPTest, EmptyBuffer) {
    RCP::yield();
    EXPECT_EQ(OUT.size(), 0);
}

TEST_F(RCPEstopTest, ESTOP) {
    PUSH(0x00);
    EXPECT_EXIT(RCP::yield(), testing::ExitedWithCode(8675309), "");
}

TEST_F(RCPTest, Systime) {
    SYSTIME = 0x12345678;
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0x30);
    RCP::yield();
    CHECK_OUTBUF(0x05, RCP_DEVCLASS_TEST_STATE, 0x12, 0x34, 0x56, 0x78, 0x30);
}

TEST_F(RCPTest, Millis) {
    SYSTIME = 0;
    EXPECT_EQ(RCP::millis(), 0);
    SYSTIME = 0x12345678;
    EXPECT_EQ(RCP::millis(), 0x12345678);
    SYSTIME = 0x87654321;
    EXPECT_EQ(RCP::millis(), 0x87654321);
}

TEST_F(RCPTest, MillisOffset) {
    SYSTIME = 0x000000FF;
    EXPECT_EQ(RCP::millis(), 0x000000FF);
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0x13);
    RCP::yield();
    SYSTIME = 0x0000FFFF;
    EXPECT_EQ(RCP::millis(), 0x0000FF00);
}

TEST_F(RCPTest, StringWrite) {
    RCP::RCPWriteSerialString("HELLO");
    CHECK_OUTBUF(0x05, RCP_DEVCLASS_CUSTOM, HELLOHEX);
}

TEST_F(RCPTest, OneFloat) {
    RCP::sendOneFloat(RCP_DEVCLASS_TEST_STATE, 10, PI);
    CHECK_OUTBUF(0x09, RCP_DEVCLASS_TEST_STATE, 0x00, 0x00, 0x00, 0x00, 0x0A, HFLOATARR(HPI));
}

TEST_F(RCPTest, TwoFloat) {
    RCP::sendTwoFloat(RCP_DEVCLASS_TEST_STATE, 10, {PI, PI2});
    CHECK_OUTBUF(0x0D, RCP_DEVCLASS_TEST_STATE, 0x00, 0x00, 0x00, 0x00, 0x0A, HFLOATARR(HPI), HFLOATARR(HPI2));
}

TEST_F(RCPTest, ThreeFloat) {
    RCP::sendThreeFloat(RCP_DEVCLASS_TEST_STATE, 10, {PI, PI2, PI3});
    CHECK_OUTBUF(0x11, RCP_DEVCLASS_TEST_STATE, 0x00, 0x00, 0x00, 0x00, 0x0A, HFLOATARR(HPI), HFLOATARR(HPI2),
                 HFLOATARR(HPI3));
}

TEST_F(RCPTest, FourFloat) {
    RCP::sendFourFloat(RCP_DEVCLASS_TEST_STATE, 10, {PI, PI2, PI3, PI4});
    CHECK_OUTBUF(0x15, RCP_DEVCLASS_TEST_STATE, 0x00, 0x00, 0x00, 0x00, 0x0A, HFLOATARR(HPI), HFLOATARR(HPI2),
                 HFLOATARR(HPI3), HFLOATARR(HPI4));
}

TEST_F(RCPTest, TestStart) {
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0x01);
    RCP::yield();
    CHECK_OUTBUF(0x05, RCP_DEVCLASS_TEST_STATE, 0x00, 0x00, 0x00, 0x00, 0x10);
    EXPECT_EQ(RCP::getTestNum(), 1);

    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0x02);
    RCP::yield();
    CHECK_OUTBUF(0x05, RCP_DEVCLASS_TEST_STATE, 0x00, 0x00, 0x00, 0x00, 0x10);
    EXPECT_EQ(RCP::getTestNum(), 1);
}

TEST_F(RCPTest, TestStop) {
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0x01);
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0x10);
    RCP::yield();
    RCP::yield();
    EXPECT_EQ(RCP::getTestNum(), 1);
    EXPECT_EQ(RCP::getTestState(), RCP_TEST_STOPPED);
}

TEST_F(RCPTest, TestPause) {
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0x01);
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0x11);
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0x11);
    RCP::yield();
    RCP::yield();
    CHECK_OUTBUF(0x05, RCP_DEVCLASS_TEST_STATE, 0x00, 0x00, 0x00, 0x00, 0x10);
    CHECK_OUTBUF(0x05, RCP_DEVCLASS_TEST_STATE, 0x00, 0x00, 0x00, 0x00, 0x50);
    EXPECT_EQ(RCP::getTestState(), RCP_TEST_PAUSED);
    RCP::yield();
    EXPECT_EQ(RCP::getTestState(), RCP_TEST_RUNNING);
    CHECK_OUTBUF(0x05, RCP_DEVCLASS_TEST_STATE, 0x00, 0x00, 0x00, 0x00, 0x10);
}

TEST_F(RCPTest, SysReset) {
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0x12);

    EXPECT_EXIT(RCP::yield(), ::testing::ExitedWithCode(8675309), "");
}

TEST_F(RCPTest, DataStreaming) {
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0x21);
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0x20);
    RCP::yield();
    EXPECT_TRUE(RCP::getDataStreaming());
    CHECK_OUTBUF(0x05, RCP_DEVCLASS_TEST_STATE, 0x00, 0x00, 0x00, 0x00, 0xB0);

    RCP::yield();
    EXPECT_FALSE(RCP::getDataStreaming());
    CHECK_OUTBUF(0x05, RCP_DEVCLASS_TEST_STATE, 0x00, 0x00, 0x00, 0x00, 0x30);
}

TEST_F(RCPEstopTest, HeartbeatKill) {
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0xF1);
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0xFF);
    PUSH(0x01, RCP_DEVCLASS_TEST_STATE, 0xFF);
    RCP::yield();
    RCP::yield();
    SYSTIME = 0xFFFFFFFF;
    EXPECT_EXIT(RCP::yield(), testing::ExitedWithCode(8675309), "");
}

TEST_F(RCPPromptTest, PromptString) {
    RCP::setPrompt("HELLO", RCP_PromptDataType_GONOGO, &pacceptor);
    CHECK_OUTBUF(0x06, RCP_DEVCLASS_PROMPT, 0x00, HELLOHEX);
}

TEST_F(RCPPromptTest, BoolPromptGO) {
    RCP::setPrompt("", RCP_PromptDataType_GONOGO, &pacceptor);
    CHECK_OUTBUF(0x01, RCP_DEVCLASS_PROMPT, 0x00);

    PUSH(0x01, RCP_DEVCLASS_PROMPT, 0x01);
    RCP::yield();
    EXPECT_TRUE(PROMPT.boolData);
}

TEST_F(RCPPromptTest, BoolPromptNOGO) {
    RCP::setPrompt("", RCP_PromptDataType_GONOGO, &pacceptor);
    CHECK_OUTBUF(0x01, RCP_DEVCLASS_PROMPT, 0x00);

    PUSH(0x01, RCP_DEVCLASS_PROMPT, 0x00);
    RCP::yield();
    EXPECT_FALSE(PROMPT.boolData);
}

TEST_F(RCPPromptTest, FloatPrompt) {
    RCP::setPrompt("", RCP_PromptDataType_Float, &pacceptor);
    CHECK_OUTBUF(0x01, RCP_DEVCLASS_PROMPT, 0x01);

    PUSH(0x04, RCP_DEVCLASS_PROMPT, HFLOATARR(HPI));
    RCP::yield();
    EXPECT_EQ(PROMPT.floatData, PI);
}

TEST_F(RCPPromptTest, PromptReset) {
    RCP::resetPrompt();
    CHECK_OUTBUF(0x01, RCP_DEVCLASS_PROMPT, 0xFF);
}

TEST_F(RCPSimpleActuators, ActuatorRead) {
    ACTS[0] = true;
    ACTS[1] = false;
    ACTS[2] = false;
    ACTS[3] = true;

    PUSH(0x01, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x00);
    PUSH(0x01, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x01);
    PUSH(0x01, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x02);
    PUSH(0x01, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x03);

    RCP::yield();
    RCP::yield();
    RCP::yield();
    RCP::yield();

    CHECK_OUTBUF(0x06, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x00, 0x00, 0x00, 0x00, 0x00, RCP_SIMPLE_ACTUATOR_ON);
    CHECK_OUTBUF(0x06, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x00, 0x00, 0x00, 0x00, 0x01, RCP_SIMPLE_ACTUATOR_OFF);
    CHECK_OUTBUF(0x06, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x00, 0x00, 0x00, 0x00, 0x02, RCP_SIMPLE_ACTUATOR_OFF);
    CHECK_OUTBUF(0x06, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x00, 0x00, 0x00, 0x00, 0x03, RCP_SIMPLE_ACTUATOR_ON);
}

TEST_F(RCPSimpleActuators, ActuatorWrite) {
    PUSH(0x02, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x00, RCP_SIMPLE_ACTUATOR_ON);
    RCP::yield();
    EXPECT_TRUE(ACTS[0]);
    CHECK_OUTBUF(0x06, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x00, 0x00, 0x00, 0x00, 0x00, RCP_SIMPLE_ACTUATOR_ON);

    PUSH(0x02, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x00, RCP_SIMPLE_ACTUATOR_OFF);
    RCP::yield();
    EXPECT_FALSE(ACTS[0]);
    CHECK_OUTBUF(0x06, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x00, 0x00, 0x00, 0x00, 0x00, RCP_SIMPLE_ACTUATOR_OFF);

    PUSH(0x02, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x00, RCP_SIMPLE_ACTUATOR_TOGGLE);
    PUSH(0x02, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x00, RCP_SIMPLE_ACTUATOR_TOGGLE);

    RCP::yield();
    EXPECT_TRUE(ACTS[0]);
    CHECK_OUTBUF(0x06, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x00, 0x00, 0x00, 0x00, 0x00, RCP_SIMPLE_ACTUATOR_ON);

    RCP::yield();
    EXPECT_FALSE(ACTS[0]);
    CHECK_OUTBUF(0x06, RCP_DEVCLASS_SIMPLE_ACTUATOR, 0x00, 0x00, 0x00, 0x00, 0x00, RCP_SIMPLE_ACTUATOR_OFF);
}

TEST_F(RCPSteppers, StepperRead) {
    STEPS[0][0] = PI;
    STEPS[0][1] = PI2;
    STEPS[1][0] = PI3;
    STEPS[1][1] = PI4;

    PUSH(0x01, RCP_DEVCLASS_STEPPER, 0x00);
    PUSH(0x01, RCP_DEVCLASS_STEPPER, 0x01);

    RCP::yield();
    CHECK_TWOFLOAT(RCP_DEVCLASS_STEPPER, 0, HPI, HPI2);

    RCP::yield();
    CHECK_TWOFLOAT(RCP_DEVCLASS_STEPPER, 1, HPI3, HPI4);
}

TEST_F(RCPSteppers, StepperWritePos) {
    PUSH(0x06, RCP_DEVCLASS_STEPPER, 0, RCP_STEPPER_ABSOLUTE_POS_CONTROL, HFLOATARR(HPI));
    RCP::yield();
    EXPECT_EQ(PI, STEPS[0][0]);
    CHECK_TWOFLOAT(RCP_DEVCLASS_STEPPER, 0, HPI, 0);

    PUSH(0x06, RCP_DEVCLASS_STEPPER, 0, RCP_STEPPER_RELATIVE_POS_CONTROL, HFLOATARR(HPI));
    RCP::yield();
    EXPECT_EQ(PI2, STEPS[0][0]);
    CHECK_TWOFLOAT(RCP_DEVCLASS_STEPPER, 0, HPI2, 0);

    PUSH(0x06, RCP_DEVCLASS_STEPPER, 0, RCP_STEPPER_ABSOLUTE_POS_CONTROL, 0x00, 0x00, 0x00, 0x00);
    RCP::yield();
    EXPECT_EQ(0, STEPS[0][0]);
    CHECK_TWOFLOAT(RCP_DEVCLASS_STEPPER, 0, 0, 0);
}

TEST_F(RCPSteppers, StepperWriteSpeed) {
    PUSH(0x06, RCP_DEVCLASS_STEPPER, 0, RCP_STEPPER_SPEED_CONTROL, HFLOATARR(HPI));
    RCP::yield();
    EXPECT_EQ(PI, STEPS[0][1]);
    CHECK_TWOFLOAT(RCP_DEVCLASS_STEPPER, 0, 0, HPI);
}

TEST_F(RCPAngledActuator, AngledActuatorsRead) {
    ANGACT[0] = PI;
    ANGACT[1] = PI2;
    ANGACT[2] = PI3;
    ANGACT[3] = PI4;

    PUSH(0x01, RCP_DEVCLASS_ANGLED_ACTUATOR, 0x00);
    PUSH(0x01, RCP_DEVCLASS_ANGLED_ACTUATOR, 0x01);
    PUSH(0x01, RCP_DEVCLASS_ANGLED_ACTUATOR, 0x02);
    PUSH(0x01, RCP_DEVCLASS_ANGLED_ACTUATOR, 0x03);

    RCP::yield();
    RCP::yield();
    RCP::yield();
    RCP::yield();

    CHECK_ONEFLOAT(RCP_DEVCLASS_ANGLED_ACTUATOR, 0, HPI);
    CHECK_ONEFLOAT(RCP_DEVCLASS_ANGLED_ACTUATOR, 1, HPI2);
    CHECK_ONEFLOAT(RCP_DEVCLASS_ANGLED_ACTUATOR, 2, HPI3);
    CHECK_ONEFLOAT(RCP_DEVCLASS_ANGLED_ACTUATOR, 3, HPI4);
}

TEST_F(RCPAngledActuator, AngledActuatorsWrite) {
    PUSH(0x05, RCP_DEVCLASS_ANGLED_ACTUATOR, 0x00, HFLOATARR(HPI));
    RCP::yield();
    EXPECT_EQ(PI, ANGACT[0]);
    CHECK_ONEFLOAT(RCP_DEVCLASS_ANGLED_ACTUATOR, 0, HPI);
}

TEST_F(RCPSensors, SensorRead1) {
    SENSE[0] = PI;
    PUSH(0x01, RCP_DEVCLASS_PRESSURE_TRANSDUCER, 0x00);
    RCP::yield();
    CHECK_ONEFLOAT(RCP_DEVCLASS_PRESSURE_TRANSDUCER, 0, HPI);
}

TEST_F(RCPSensors, SensorRead2) {
    SENSE[0] = PI;
    SENSE[1] = PI2;
    PUSH(0x01, RCP_DEVCLASS_POWERMON, 0x00);
    RCP::yield();
    CHECK_TWOFLOAT(RCP_DEVCLASS_POWERMON, 0, HPI, HPI2);
}

TEST_F(RCPSensors, SensorRead3) {
    SENSE[0] = PI;
    SENSE[1] = PI2;
    SENSE[2] = PI3;
    PUSH(0x01, RCP_DEVCLASS_ACCELEROMETER, 0x00);
    RCP::yield();
    CHECK_THREEFLOAT(RCP_DEVCLASS_ACCELEROMETER, 0, HPI, HPI2, HPI3);
}

TEST_F(RCPSensors, SensorRead4) {
    SENSE[0] = PI;
    SENSE[1] = PI2;
    SENSE[2] = PI3;
    SENSE[3] = PI4;
    PUSH(0x01, RCP_DEVCLASS_GPS, 0x00);
    RCP::yield();
    CHECK_FOURFLOAT(RCP_DEVCLASS_GPS, 0, HPI, HPI2, HPI3, HPI4);
}

TEST_F(RCPSensors, SensorTare1) {
    SENSE[0] = 0;

    PUSH(0x06, RCP_DEVCLASS_PRESSURE_TRANSDUCER, 0x00, 0x00, HFLOATARR(HPI));

    RCP::yield();

    EXPECT_EQ(OUT.size(), 0);
    EXPECT_EQ(SENSE[0], PI);
}

TEST_F(RCPSensors, SensorTare2) {
    SENSE[0] = 0;
    SENSE[1] = 0;

    PUSH(0x06, RCP_DEVCLASS_GPS, 0x00, 0x00, HFLOATARR(HPI));
    PUSH(0x06, RCP_DEVCLASS_GPS, 0x00, 0x01, HFLOATARR(HPI2));

    RCP::yield();
    RCP::yield();

    EXPECT_EQ(OUT.size(), 0);
    EXPECT_EQ(SENSE[0], PI);
    EXPECT_EQ(SENSE[1], PI2);
}

TEST_F(RCPSensors, SensorTare3) {
    SENSE[0] = 0;
    SENSE[1] = 0;
    SENSE[2] = 0;

    PUSH(0x06, RCP_DEVCLASS_GPS, 0x00, 0x00, HFLOATARR(HPI));
    PUSH(0x06, RCP_DEVCLASS_GPS, 0x00, 0x01, HFLOATARR(HPI2));
    PUSH(0x06, RCP_DEVCLASS_GPS, 0x00, 0x02, HFLOATARR(HPI3));

    RCP::yield();
    RCP::yield();
    RCP::yield();

    EXPECT_EQ(OUT.size(), 0);
    EXPECT_EQ(SENSE[0], PI);
    EXPECT_EQ(SENSE[1], PI2);
    EXPECT_EQ(SENSE[2], PI3);
}

TEST_F(RCPSensors, SensorTare4) {
    SENSE[0] = 0;
    SENSE[1] = 0;
    SENSE[2] = 0;
    SENSE[3] = 0;

    PUSH(0x06, RCP_DEVCLASS_GPS, 0x00, 0x00, HFLOATARR(HPI));
    PUSH(0x06, RCP_DEVCLASS_GPS, 0x00, 0x01, HFLOATARR(HPI2));
    PUSH(0x06, RCP_DEVCLASS_GPS, 0x00, 0x02, HFLOATARR(HPI3));
    PUSH(0x06, RCP_DEVCLASS_GPS, 0x00, 0x03, HFLOATARR(HPI4));

    RCP::yield();
    RCP::yield();
    RCP::yield();
    RCP::yield();

    EXPECT_EQ(OUT.size(), 0);
    EXPECT_EQ(SENSE[0], PI);
    EXPECT_EQ(SENSE[1], PI2);
    EXPECT_EQ(SENSE[2], PI3);
    EXPECT_EQ(SENSE[3], PI4);
}

TEST_F(RCPRawData, RawSend) {
    PUSH(0x05, RCP_DEVCLASS_CUSTOM, HELLOHEX);
    RCP::yield();
    EXPECT_EQ(RAWLEN, 5);
    std::string rawstr = std::string(RAWDATA, RAWLEN);
    EXPECT_STREQ(rawstr.c_str(), "HELLO");
}

TEST_F(RCPBoolSensor, BoolSensor) {
    SENSEB = false;
    PUSH(0x01, RCP_DEVCLASS_BOOL_SENSOR, 0x0A);
    RCP::yield();
    CHECK_OUTBUF(0x06, RCP_DEVCLASS_BOOL_SENSOR, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00);

    SENSEB = true;
    PUSH(0x01, RCP_DEVCLASS_BOOL_SENSOR, 0x0A);
    RCP::yield();
    CHECK_OUTBUF(0x06, RCP_DEVCLASS_BOOL_SENSOR, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x80);
}
