/*
 * This file contains the logic for RCP. It is based on the reference implementation.
 * Read up on RCP here: https://wiki.liquidrocket.org/en/electronics/rcp
 *
 * The implementation follows the spec very closely, and does not do anything else. The only things
 * to mention are use of the testing framework and prompts.
 *
 * Prompts are set by passing to RCP a function pointer to the function responsible for handling
 * when the prompt is received, and the prompt data type. When the prompt is received, the prompt
 * handler is executed. The prompt acceptor is passed the pointer to a 4 byte array that will contain
 * the prompt data. It is up to the prompt acceptor to know what data type is present in the memory region.
 *
 * In this implementation, the testing framework is used for executing the emergency stop sequence. The
 * sequence can be defined by changing the ESTOP_PROC variable.
 */
#include <string.h>

#include "RCP_Target/RCP_Target.h"

#include "RCP_Target/procedures.h"

#ifndef __GNUG__
#error "This code uses GCC weak symbols, therefore a GCC compiler must be used"
#endif

namespace RCP {
    RCP_Channel channel;
    Test::Procedure* ESTOP_PROC = nullptr;

    static LRI::RingBuf<uint8_t, RCP_SERIAL_BUFFER_SIZE> inbuffer;

    static uint8_t testNum;
    static RCP_TestRunningState testState;
    static bool dataStreaming;
    static bool ready = false;
    static uint8_t heartbeatTime;
    static uint32_t lastHeartbeatReceived;
    static bool writeUpdatesPaused;

    static bool firstTestRun;
    static bool initDone = false;
    static uint32_t timeOffset = 0;

    static PromptData promptdata;
    static RCP_PromptDataType lastType;
    static PromptAcceptor pacceptor;

    inline void insertTimestamp(uint8_t* start) {
        uint32_t time = millis();
        start[0] = time >> 24;
        start[1] = time >> 16;
        start[2] = time >> 8;
        start[3] = time;
    }

    void init() {
        testNum = 0;
        testState = RCP_TEST_STOPPED;
        dataStreaming = false;
        firstTestRun = false;
        initDone = true;
        heartbeatTime = 0;
        lastHeartbeatReceived = 0;
        timeOffset = 0;
        inbuffer.clear();
        writeUpdatesPaused = false;
    }

    static void sendSimpleActuatorState(uint8_t id, RCP_SimpleActuatorState state) {
        uint8_t pkt[8];
        pkt[0] = channel | 0x06;
        pkt[1] = RCP_DEVCLASS_SIMPLE_ACTUATOR;
        insertTimestamp(pkt + 2);
        pkt[6] = id;
        pkt[7] = state ? RCP_SIMPLE_ACTUATOR_ON : RCP_SIMPLE_ACTUATOR_OFF;
        write(pkt, 8);
    }

    // The majority of RCP related functions
    void yield() {
        if(!initDone) return;

        // Read SERIAL_BYTES_PER_LOOP bytes into the buffer
        for(int i = 0; i < SERIAL_BYTES_PER_LOOP && readAvail(); i++) {
            uint8_t val = read();
            inbuffer.push(val);
        }

        if(heartbeatTime != 0 && millis() - lastHeartbeatReceived > heartbeatTime) ESTOP();

        // Calculate the packet length from the header available in the buffer
        if(inbuffer.isEmpty()) return;
        uint8_t head = 0;
        inbuffer.peek(head, 0);
        uint8_t pktlen = head & (~RCP_CHANNEL_MASK);

        // RCP::RCPWriteSerialString((String(head) + " " + String(inbuffer.size()) + "\n").c_str());
        // If the packet length is zero, this indicates an ESTOP condition. Do that immediately.
        if(pktlen == 0) ESTOP();

        // If the buffer contains all the bytes for a packet, read it in to the
        // array bytes
        if(inbuffer.size() >= pktlen + 2) {
            // RCPDebug("pkt");
            uint8_t bytes[65];
            for(int i = 0; i < pktlen + 2; i++) {
                inbuffer.pop(bytes[i]);
            }

            // If the channel does not match, exit early
            if((bytes[0] & RCP_CHANNEL_MASK) != channel) return;

            // Switch on the device class
            switch(auto devclass = static_cast<RCP_DeviceClass>(bytes[1])) {
                // Handle test state packet
            case RCP_DEVCLASS_TEST_STATE: {
                switch(bytes[2] & 0xF0) {
                case 0x00:
                    if(testState != RCP_TEST_STOPPED) break;
                    testNum = bytes[2] & 0x0F;
                    testState = RCP_TEST_RUNNING;
                    firstTestRun = true;
                    break;

                case 0x10: {
                    switch(bytes[2] & 0x0F) {
                    case 0x00:
                        if(testState == RCP_TEST_RUNNING || testState == RCP_TEST_PAUSED) {
                            Test::getTests()[testNum]->end(true);
                            testState = RCP_TEST_STOPPED;
                            resetPrompt();
                        }
                        break;

                    case 0x01: {
                        if(testState == RCP_TEST_RUNNING) testState = RCP_TEST_PAUSED;
                        else if(testState == RCP_TEST_PAUSED) testState = RCP_TEST_RUNNING;
                        break;

                    default:
                        break;
                    }

                    case 0x02:
                        systemReset();

                    case 0x03:
                        timeOffset = millis();
                        break;
                    }

                    break;
                }

                case 0x20:
                    dataStreaming = (bytes[2] & 0x0F) != 0;
                    break;

                case 0xF0:
                    if((bytes[2] & 0x0F) == 0x0F) lastHeartbeatReceived = millis();
                    else heartbeatTime = bytes[2] & 0x0F;
                    break;

                default:
                    break;
                }

                sendTestState();

                break;
            }

            case RCP_DEVCLASS_PROMPT: {
                if(!pacceptor) break;
                if(lastType == RCP_PromptDataType_GONOGO) promptdata.boolData = bytes[2];
                else memcpy(&promptdata.floatData, bytes + 2, 4);

                pacceptor(promptdata);
                pacceptor = nullptr;
                break;
            }

            case RCP_DEVCLASS_SIMPLE_ACTUATOR: {
                if(pktlen == 1) sendSimpleActuatorState(bytes[2], readSimpleActuator(bytes[2]));
                else writeSimpleActuator(bytes[2], static_cast<RCP_SimpleActuatorState>(bytes[3]));
                break;
            }

            case RCP_DEVCLASS_STEPPER: {
                if(pktlen == 1) sendTwoFloat(RCP_DEVCLASS_STEPPER, bytes[2], readStepper(bytes[2]));
                else {
                    auto ctlmode = static_cast<RCP_StepperControlMode>(bytes[3]);
                    float ctlval;
                    memcpy(&ctlval, bytes + 4, 4);
                    writeStepper(bytes[2], ctlmode, ctlval);
                }

                break;
            }

            case RCP_DEVCLASS_ANGLED_ACTUATOR: {
                if(pktlen == 1) sendOneFloat(RCP_DEVCLASS_ANGLED_ACTUATOR, bytes[2], readAngledActuator(bytes[2]));
                else {
                    float val = 0;
                    memcpy(&val, bytes + 3, 4);
                    writeAngledActuator(bytes[2], val);
                }

                break;
            }

            case RCP_DEVCLASS_CUSTOM:
                handleCustomData(bytes + 2, pktlen);
                break;


            case RCP_DEVCLASS_BOOL_SENSOR: {
                bool resval = readBoolSensor(bytes[2]);
                uint8_t data[8];
                data[0] = channel | 0x06;
                data[1] = RCP_DEVCLASS_BOOL_SENSOR;
                insertTimestamp(data + 2);
                data[6] = bytes[2];
                data[7] = resval ? 0x80 : 0x00;
                write(data, 8);
                break;
            }

            case RCP_DEVCLASS_AM_PRESSURE:
            case RCP_DEVCLASS_AM_TEMPERATURE:
            case RCP_DEVCLASS_PRESSURE_TRANSDUCER:
            case RCP_DEVCLASS_RELATIVE_HYGROMETER:
            case RCP_DEVCLASS_LOAD_CELL: {
                if(pktlen == 1) {
                    sendOneFloat(devclass, bytes[2], readSensor(devclass, bytes[2]).vals[0]);
                }

                else {
                    uint8_t channel = bytes[3];
                    float tareval;
                    memcpy(&tareval, bytes + 4, 4);
                    writeSensorTare(devclass, bytes[2], channel, tareval);
                }

                break;
            }

            case RCP_DEVCLASS_POWERMON: {
                if(pktlen == 1) {
                    sendTwoFloat(devclass, bytes[2], readSensor(devclass, bytes[2]).vals);
                }

                else {
                    uint8_t channel = bytes[3];
                    float tareval;
                    memcpy(&tareval, bytes + 4, 4);
                    writeSensorTare(devclass, bytes[2], channel, tareval);
                }

                break;
            }

            case RCP_DEVCLASS_ACCELEROMETER:
            case RCP_DEVCLASS_GYROSCOPE:
            case RCP_DEVCLASS_MAGNETOMETER: {
                if(pktlen == 1) {
                    sendThreeFloat(devclass, bytes[2], readSensor(devclass, bytes[2]).vals);
                }

                else {
                    uint8_t channel = bytes[3];
                    float tareval;
                    memcpy(&tareval, bytes + 4, 4);
                    writeSensorTare(devclass, bytes[2], channel, tareval);
                }

                break;
            }

            case RCP_DEVCLASS_GPS: {
                if(pktlen == 1) {
                    sendFourFloat(devclass, bytes[2], readSensor(devclass, bytes[2]).vals);
                }

                else {
                    uint8_t channel = bytes[3];
                    float tareval;
                    memcpy(&tareval, bytes + 4, 4);
                    writeSensorTare(devclass, bytes[2], channel, tareval);
                }

                break;
            }

            default:
                break;
            }
        }
    }

    void runTest() {
        if(testState != RCP_TEST_RUNNING) return;
        Test::Procedure* test = Test::getTests()[testNum];

        if(firstTestRun) {
            test->initialize();
            firstTestRun = false;
        }

        test->execute();
        if(test->isFinished()) {
            test->end(false);
            testState = RCP_TEST_STOPPED;
            firstTestRun = true;
            sendTestState();
        }
    }

    // The weak attribute is needed so user defined versions of systemReset will override this one
    [[gnu::weak, noreturn]] void systemReset() {
        while(true) {}
    }

    void pauseWriteUpdates() { writeUpdatesPaused = true; }

    void unpauseWriteUpdates() { writeUpdatesPaused = false; }

    void sendTestState() {
        uint8_t data[7] = {0};
        data[0] = channel | 0x05;
        data[1] = 0x00;
        insertTimestamp(data + 2);
        data[6] = testState | heartbeatTime | (dataStreaming ? 0x80 : 0x00) | (ready ? 0x10 : 0x00);
        write(data, 7);
    }

    void startProcedure(uint8_t id) {
        testNum = id;
        testState = RCP_TEST_RUNNING;
        firstTestRun = true;
    }

    [[noreturn]] void ESTOP() {
        if(testState == RCP_TEST_RUNNING || testState == RCP_TEST_PAUSED) Test::getTests()[testNum]->end(true);
        testState = RCP_TEST_ESTOP;
        sendTestState();
        if(ESTOP_PROC) {
            ESTOP_PROC->initialize();
            while(!ESTOP_PROC->isFinished()) ESTOP_PROC->execute();
            ESTOP_PROC->end(false);
        }

        while(true) {}
    }

    void RCPWriteSerialString(const char* str) {
        uint8_t len = strlen(str);
        if(len > 63) return;
        uint8_t data[65] = {0};
        data[0] = channel | len;
        data[1] = RCP_DEVCLASS_CUSTOM;
        memcpy(data + 2, str, len);
        write(data, len + 2);
    }

    void setReady(bool newready) {
        if(!initDone || newready == ready) return;
        ready = newready;
        sendTestState();
    }

    void setPrompt(const char* str, RCP_PromptDataType gng, PromptAcceptor acceptor) {
        size_t len = strlen(str);
        if(len > 62) return;
        pacceptor = acceptor;
        lastType = gng;
        uint8_t pkt[65];
        pkt[0] = channel | (len + 1);
        pkt[1] = RCP_DEVCLASS_PROMPT;
        pkt[2] = gng;
        memcpy(pkt + 3, str, len);
        write(pkt, len + 3);
    }

    void resetPrompt() {
        pacceptor = nullptr;
        uint8_t pkt[3] = {0};
        pkt[0] = channel | 1;
        pkt[1] = RCP_DEVCLASS_PROMPT;
        pkt[2] = RCP_PromptDataType_RESET;
        write(pkt, 3);
    }

    bool getDataStreaming() { return dataStreaming; }

    uint8_t getTestNum() { return testNum; }

    uint32_t millis() { return systime() - timeOffset; }

    uint8_t getHeartbeatTime() { return heartbeatTime; }

    RCP_TestRunningState getTestState() { return testState; }

    void sendOneFloat(const RCP_DeviceClass devclass, const uint8_t id, float value) {
        uint8_t data[11] = {0};
        data[0] = channel | 9;
        data[1] = devclass;
        insertTimestamp(data + 2);
        data[6] = id;
        memcpy(data + 7, &value, 4);
        write(data, 11);
    }

    void sendTwoFloat(const RCP_DeviceClass devclass, const uint8_t id, const float value[2]) {
        uint8_t data[15] = {0};
        data[0] = channel | 13;
        data[1] = devclass;
        insertTimestamp(data + 2);
        data[6] = id;
        memcpy(data + 7, value, 8);
        write(data, 15);
    }

    void sendThreeFloat(const RCP_DeviceClass devclass, const uint8_t id, const float value[3]) {
        uint8_t data[19] = {0};
        data[0] = channel | 17;
        data[1] = devclass;
        insertTimestamp(data + 2);
        data[6] = id;
        memcpy(data + 7, value, 12);
        write(data, 19);
    }

    void sendFourFloat(const RCP_DeviceClass devclass, const uint8_t id, const float value[4]) {
        uint8_t data[23] = {0};
        data[0] = channel | 21;
        data[1] = devclass;
        insertTimestamp(data + 2);
        data[6] = id;
        memcpy(data + 7, value, 16);
        write(data, 23);
    }

    [[gnu::weak]] void write([[maybe_unused]] const void* data, [[maybe_unused]] uint8_t length) {}
    [[gnu::weak]] uint8_t readAvail() { return 0; }
    [[gnu::weak]] uint8_t read() { return 0; }
    [[gnu::weak]] uint32_t systime() { return 0; }

    RCP_SimpleActuatorState writeSimpleActuator(uint8_t id, RCP_SimpleActuatorState state) {
        RCP_SimpleActuatorState newstate = simpleActuatorWrite_CLBK(id, state);
        if(!writeUpdatesPaused) sendSimpleActuatorState(id, newstate);
        return newstate;
    }

    Floats2 writeStepper(uint8_t id, RCP_StepperControlMode controlMode, float controlVal) {
        Floats2 newstate = stepperWrite_CLBK(id, controlMode, controlVal);
        if(!writeUpdatesPaused) sendTwoFloat(RCP_DEVCLASS_STEPPER, id, newstate);
        return newstate;
    }

    float writeAngledActuator(uint8_t id, float controlVal) {
        float newstate = angledActuatorWrite_CLBK(id, controlVal);
        if(!writeUpdatesPaused) sendOneFloat(RCP_DEVCLASS_ANGLED_ACTUATOR, id, newstate);
        return newstate;
    }

    [[gnu::weak]] RCP_SimpleActuatorState readSimpleActuator([[maybe_unused]] uint8_t id) {
        return RCP_SIMPLE_ACTUATOR_OFF;
    }

    [[gnu::weak]] RCP_SimpleActuatorState simpleActuatorWrite_CLBK([[maybe_unused]] uint8_t id,
                                                                   [[maybe_unused]] RCP_SimpleActuatorState state) {
        return RCP_SIMPLE_ACTUATOR_OFF;
    }

    [[gnu::weak]] Floats2 readStepper([[maybe_unused]] uint8_t id) { return {}; }

    [[gnu::weak]] Floats2 stepperWrite_CLBK([[maybe_unused]] uint8_t id,
                                            [[maybe_unused]] RCP_StepperControlMode controlMode,
                                            [[maybe_unused]] float controlVal) {
        return {};
    }

    [[gnu::weak]] float readAngledActuator([[maybe_unused]] uint8_t id) { return 0; }

    [[gnu::weak]] float angledActuatorWrite_CLBK([[maybe_unused]] uint8_t id, [[maybe_unused]] float controlVal) {
        return 0;
    }

    [[gnu::weak]] Floats4 readSensor([[maybe_unused]] RCP_DeviceClass devclass, [[maybe_unused]] uint8_t id) {
        return {};
    }

    [[gnu::weak]] bool readBoolSensor([[maybe_unused]] uint8_t id) { return false; }

    [[gnu::weak]] void writeSensorTare([[maybe_unused]] RCP_DeviceClass devclass, [[maybe_unused]] uint8_t id,
                                       [[maybe_unused]] uint8_t dataChannel, [[maybe_unused]] float tareVal) {}

    [[gnu::weak]] void handleCustomData([[maybe_unused]] const void* data, [[maybe_unused]] uint8_t length) {}


} // namespace RCP

namespace Test {
    [[gnu::weak]] Tests& getTests() {
        static Tests tests = {
            new Procedure(), new Procedure(), new Procedure(), new Procedure(), new Procedure(),
            new Procedure(), new Procedure(), new Procedure(), new Procedure(), new Procedure(),
            new Procedure(), new Procedure(), new Procedure(), new Procedure(), new Procedure(),
        };

        return tests;
    }
} // namespace Test
