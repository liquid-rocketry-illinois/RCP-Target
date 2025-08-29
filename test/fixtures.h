#ifndef FIXTURES_H
#define FIXTURES_H

#include "RCP_Target/RCP_Target.h"
#include "gtest/gtest.h"

class RCPRawTest;
extern RCPRawTest* context;

class RCPRawTest : public testing::Test {
protected:
    RCPRawTest() { context = this; }

    ~RCPRawTest() override { context = nullptr; }

public:
    LRI::RingBuf<uint8_t, 65> outbuf;
    LRI::RingBuf<uint8_t, 65> inbuf;
    uint32_t systime = 0;
};

class RCPTest : public RCPRawTest {
protected:
    RCPTest() {
        RCP::init();
        RCP::setReady(true);
        for(int i = 0; i < 7; i++) {
            uint8_t val;
            outbuf.pop(val);
        }
    }

    ~RCPTest() override { RCP::setReady(false); }
};

class RCPEstopTest : public RCPTest {
protected:
    ::Test::OneShot estop{[] { std::exit(8675309); }};

    RCPEstopTest() { RCP::ESTOP_PROC = &estop; }

    ~RCPEstopTest() override { RCP::ESTOP_PROC = nullptr; }
};

class RCPPromptTest : public RCPTest {
protected:
    ~RCPPromptTest() override = default;

public:
    RCP::PromptData data;
};

class RCPSimpleActuators : public RCPTest {
protected:
    ~RCPSimpleActuators() override = default;

public:
    bool actuators[256];
};

class RCPSteppers : public RCPTest {
protected:
    ~RCPSteppers() override = default;

public:
    float steppers[256][2];
};

class RCPAngledActuator : public RCPTest {
protected:
    ~RCPAngledActuator() override = default;

public:
    float actuators[256];
};

class RCPSensors : public RCPTest {
protected:
    ~RCPSensors() override = default;

public:
    float sensorvals[4];
};

class RCPRawData : public RCPTest {
protected:
    ~RCPRawData() override = default;

public:
    char data[64];
    uint8_t length;
};

#define IN context->inbuf
#define OUT context->outbuf
#define SYSTIME context->systime
#define ACTS dynamic_cast<RCPSimpleActuators*>(context)->actuators
#define STEPS dynamic_cast<RCPSteppers*>(context)->steppers
#define ANGACT dynamic_cast<RCPAngledActuator*>(context)->actuators
#define SENSE dynamic_cast<RCPSensors*>(context)->sensorvals
#define RAWDATA dynamic_cast<RCPRawData*>(context)->data
#define RAWLEN dynamic_cast<RCPRawData*>(context)->length
#define PROMPT dynamic_cast<RCPPromptTest*>(context)->data
#define HELLOHEX 0x48, 0x45, 0x4C, 0x4C, 0x4F

#define PI 3.1415925f
#define HPI 0xda0f4940

#define PI2 6.283185f
#define HPI2 0xda0fc940

#define PI3 9.4247775f
#define HPI3 0xe3cb1641

#define PI4 12.56637f
#define HPI4 0xda0f4941

#define HFLOATARR(value) (uint8_t) (value >> 24), (uint8_t) (value >> 16), (uint8_t) (value >> 8), (uint8_t) value

#define PUSH(...)                                                                                                      \
    do {                                                                                                               \
        uint8_t vals[] = {__VA_ARGS__};                                                                                \
        for(size_t i = 0; i < sizeof(vals); i++) IN.push(vals[i]);                                                     \
    }                                                                                                                  \
    while(0)

#define CHECK_OUTBUF(...)                                                                                              \
    do {                                                                                                               \
        uint8_t vals[] = {__VA_ARGS__};                                                                                \
        EXPECT_GE(OUT.size(), sizeof(vals)) << "Incorrect size of outbuffer";                                          \
        for(int i = 0; i < sizeof(vals); i++) {                                                                        \
            uint8_t val;                                                                                               \
            OUT.pop(val);                                                                                              \
            EXPECT_EQ(val, vals[i]) << "Incorrect value at index " << i;                                               \
        }                                                                                                              \
    }                                                                                                                  \
    while(0)

#define CHECK_ONEFLOAT(devclass, id, hfloat1)                                                                          \
    CHECK_OUTBUF(0x09, devclass, 0x00, 0x00, 0x00, 0x00, id, HFLOATARR(hfloat1))

#define CHECK_TWOFLOAT(devclass, id, hfloat1, hfloat2)                                                                 \
    CHECK_OUTBUF(0x0D, devclass, 0x00, 0x00, 0x00, 0x00, id, HFLOATARR(hfloat1), HFLOATARR(hfloat2))

#define CHECK_THREEFLOAT(devclass, id, hfloat1, hfloat2, hfloat3)                                                      \
    CHECK_OUTBUF(0x11, devclass, 0x00, 0x00, 0x00, 0x00, id, HFLOATARR(hfloat1), HFLOATARR(hfloat2), HFLOATARR(hfloat3))

#define CHECK_FOURFLOAT(devclass, id, hfloat1, hfloat2, hfloat3, hfloat4)                                              \
    CHECK_OUTBUF(0x15, devclass, 0x00, 0x00, 0x00, 0x00, id, HFLOATARR(hfloat1), HFLOATARR(hfloat2),                   \
                 HFLOATARR(hfloat3), HFLOATARR(hfloat4))

#endif // FIXTURES_H
