#include "minisumo.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ee.h"
#include "motor.h"
#include "rc5.h"
#include "sensors.h"
#include "startStopModule.h"
#include "status.h"
#include "stm32h523xx.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_adc.h"
#include "stm32h5xx_hal_uart.h"
#include "wsLed.h"

#define MOTOR_MOVE_DUTY_PEACE 30
#define MOTOR_MOVE_DUTY_WAR 60

// TODO: Move to NVS
#define LINE_LEFT_LIMIT 800
#define LINE_RIGHT_LIMIT 800

uint32_t motorDuty = MOTOR_MOVE_DUTY_PEACE;

uint8_t                   uartBuf[200];
extern UART_HandleTypeDef huart5;

static StatusInfo         info;
static ADC_HandleTypeDef* adc;
static Rc5                rc5;
Motor                     motorLeft, motorRight;
static uint8_t            isFighting = 0;

typedef enum {
    ROBOT_MOVE_STOP = 0,
    ROBOT_MOVE_FWD,
    ROBOT_MOVE_RIGHT,
    ROBOT_MOVE_LEFT,
    ROBOT_MOVE_BACK,
} RobotDirection;

struct {
    MotorDirection left;
    MotorDirection right;
    uint32_t*      duty;
} RobotDirectionMotorMap[] = {[ROBOT_MOVE_STOP]  = {MOTOR_DIRECTION_FWD, MOTOR_DIRECTION_FWD, 0U},
                              [ROBOT_MOVE_FWD]   = {MOTOR_DIRECTION_FWD, MOTOR_DIRECTION_FWD, &motorDuty},
                              [ROBOT_MOVE_RIGHT] = {MOTOR_DIRECTION_FWD, MOTOR_DIRECTION_BCK, &motorDuty},
                              [ROBOT_MOVE_LEFT]  = {MOTOR_DIRECTION_BCK, MOTOR_DIRECTION_FWD, &motorDuty},
                              [ROBOT_MOVE_BACK]  = {MOTOR_DIRECTION_BCK, MOTOR_DIRECTION_BCK, &motorDuty}};

typedef enum {
    DETECTED_LINE_NONE = 0,
    DETECTED_LINE_LEFT,
    DETECTED_LINE_RIGHT,
    DETECTED_LINE_BOTH,
} DetectedLine;

typedef enum {
    DETECTED_TARGET_NONE = 0,
    DETECTED_TARGET_CENTER,
    DETECTED_TARGET_LEFT,
    DETECTED_TARGET_RIGHT,
} DetectedTarget;

typedef enum { MOVE_STOP = 0, MOVE_FWD, MOVE_BCK, MOVE_LEFT, MOVE_RIGHT } MoveDirection;

typedef struct _PatternMove {
    uint32_t             duration;  // [ms]
    uint32_t             speed;     // [duty(0-100)]
    RobotDirection       direction;
    uint32_t             startMs;  // [ms]
    struct _PatternMove* next;
} PatternMove;

typedef enum {
    PATTERN_SEEK = 0,
    PATTERN_SEEK_1,
    PATTERN_LINE_LEFT,
    PATTERN_LINE_LEFT_1,
    PATTERN_LINE_RIGHT,
    PATTERN_LINE_RIGHT_1,
} PatternName;

PatternMove seekMoves[] = {
    // Seeking for enemy
    [PATTERN_SEEK]         = {.direction = ROBOT_MOVE_RIGHT,
                              .duration  = 2000,
                              .speed     = MOTOR_MOVE_DUTY_PEACE,
                              .startMs   = 0,
                              .next      = &seekMoves[PATTERN_SEEK_1]},
    [PATTERN_SEEK_1]       = {.direction = ROBOT_MOVE_FWD,
                              .duration  = 400,
                              .speed     = MOTOR_MOVE_DUTY_PEACE,
                              .startMs   = 0,
                              .next      = &seekMoves[PATTERN_SEEK]},
    [PATTERN_LINE_LEFT]    = {.direction = ROBOT_MOVE_BACK,
                              .duration  = 100,
                              .speed     = MOTOR_MOVE_DUTY_PEACE,
                              .startMs   = 0,
                              .next      = &seekMoves[PATTERN_LINE_LEFT_1]},
    [PATTERN_LINE_LEFT_1]  = {.direction = ROBOT_MOVE_LEFT,
                              .duration  = 200,
                              .speed     = MOTOR_MOVE_DUTY_PEACE,
                              .startMs   = 0,
                              .next      = &seekMoves[PATTERN_SEEK]},
    [PATTERN_LINE_RIGHT]   = {.direction = ROBOT_MOVE_BACK,
                              .duration  = 100,
                              .speed     = MOTOR_MOVE_DUTY_PEACE,
                              .startMs   = 0,
                              .next      = &seekMoves[PATTERN_LINE_RIGHT_1]},
    [PATTERN_LINE_RIGHT_1] = {.direction = ROBOT_MOVE_RIGHT,
                              .duration  = 200,
                              .speed     = MOTOR_MOVE_DUTY_PEACE,
                              .startMs   = 0,
                              .next      = &seekMoves[PATTERN_SEEK]},
};

PatternMove* currentPattern;

struct FightInfo {
    DetectedTarget target;
    uint32_t       lastDetectionMs;
} fightInfo;

struct {
    uint32_t dohyoId;
} eepromData;

typedef void (*StateFunc)(void);

DetectedLine   detectedLine   = DETECTED_LINE_NONE;
DetectedTarget detectedTarget = DETECTED_TARGET_NONE;

volatile uint16_t sharp[3];
volatile uint16_t line[2];

static void minisumo_move(RobotDirection dir)
{
    uint32_t duty = RobotDirectionMotorMap[dir].duty ? *RobotDirectionMotorMap[dir].duty : 0;
    snprintf(uartBuf, 150, "(%lu)MOVE FORCE:%d\r\n", HAL_GetTick(), dir);
    HAL_UART_Transmit(&huart5, uartBuf, strlen(uartBuf), 1000);

    motor_setTarget(&motorLeft, RobotDirectionMotorMap[dir].left, duty);
    motor_setTarget(&motorRight, RobotDirectionMotorMap[dir].right, duty);
    motor_setDuty(&motorLeft, RobotDirectionMotorMap[dir].left, duty);
    motor_setDuty(&motorRight, RobotDirectionMotorMap[dir].right, duty);
}

static void minisumo_moveTarget(RobotDirection dir)
{
    snprintf(uartBuf, 150, "(%lu)MOVE TARGET:%d\r\n", HAL_GetTick(), dir);
    HAL_UART_Transmit(&huart5, uartBuf, strlen(uartBuf), 1000);
    motor_setTarget(&motorLeft, RobotDirectionMotorMap[dir].left, motorDuty);
    motor_setTarget(&motorRight, RobotDirectionMotorMap[dir].right, motorDuty);
}

static void minisumo_patternLoop()
{
    uint32_t currentMs = HAL_GetTick();
    if ((currentMs - currentPattern->startMs) > currentPattern->duration) {
        currentPattern = currentPattern->next;

        currentPattern->startMs = currentMs;
        minisumo_moveTarget(currentPattern->direction);
        snprintf(uartBuf, 150, "Pattern Finished\r\n");
        HAL_UART_Transmit(&huart5, uartBuf, strlen(uartBuf), 1000);
    }
}

static void minisumo_patternStart(PatternName p)
{
    uint32_t currentMs      = HAL_GetTick();
    currentPattern          = &seekMoves[p];
    currentPattern->startMs = currentMs;
    minisumo_moveTarget(currentPattern->direction);
}

static uint8_t minisumo_handleLineDetected(DetectedLine line)
{
    if ((detectedLine != DETECTED_LINE_NONE)) {
        switch (detectedLine) {
            case DETECTED_LINE_NONE:
                break;
            case DETECTED_LINE_RIGHT:
                minisumo_patternStart(PATTERN_LINE_LEFT);
                break;
            case DETECTED_LINE_LEFT:
            case DETECTED_LINE_BOTH:
                minisumo_patternStart(PATTERN_LINE_RIGHT);
                break;
        }
    }

    // True if line is detected, false otherwise.
    return (line != DETECTED_LINE_NONE);
}

static uint8_t minisumo_handleTargetDetected(DetectedTarget target)
{
    uint32_t currentMs = HAL_GetTick();
    motorDuty          = MOTOR_MOVE_DUTY_PEACE;

    if (fightInfo.target != target) {
        switch (detectedTarget) {
            case DETECTED_TARGET_NONE:
                // Check Timer?
                break;
            case DETECTED_TARGET_CENTER:
                motorDuty = MOTOR_MOVE_DUTY_WAR;
                minisumo_moveTarget(ROBOT_MOVE_FWD);
                //minisumo_move(ROBOT_MOVE_FWD);
                fightInfo.lastDetectionMs = currentMs;
                break;
            case DETECTED_TARGET_LEFT:
                fightInfo.lastDetectionMs = currentMs;
                minisumo_move(ROBOT_MOVE_LEFT);
                break;
            case DETECTED_TARGET_RIGHT:
                fightInfo.lastDetectionMs = currentMs;
                minisumo_move(ROBOT_MOVE_RIGHT);
                break;
        }
    }
    fightInfo.target = target;
    return (target != DETECTED_TARGET_NONE);
}

static DetectedTarget minisumo_targetDetected(uint16_t sharp[3])
{
    snprintf(uartBuf, 150, "$%d;%d;%d;\r\n", sharp[0], sharp[1], sharp[2]);
    HAL_UART_Transmit(&huart5, uartBuf, strlen(uartBuf), 1000);
    if (sharp[1] > 1500) {
        return DETECTED_TARGET_CENTER;
    }
    else if (sharp[0] > 1500) {
        return DETECTED_TARGET_LEFT;
    }
    else if (sharp[2] > 1500) {
        return DETECTED_TARGET_RIGHT;
    }
    return DETECTED_TARGET_NONE;
}

static DetectedLine minisumo_lineDetected(uint16_t line[2])
{
    DetectedLine ret = DETECTED_LINE_NONE;

    if (line[0] < LINE_LEFT_LIMIT) {
        ret = DETECTED_LINE_LEFT;
    }
    if (line[1] < LINE_RIGHT_LIMIT) {
        ret = (ret == DETECTED_LINE_LEFT) ? DETECTED_LINE_BOTH : DETECTED_LINE_RIGHT;
    }

    return ret;
}

static void state_start(void);
static void state_seek(void);

static void state_set_fight(void);
static void state_stop(void);

StateFunc currentStateFunc = state_start;

static void state_set_stop(void)
{
    motor_setDuty(&motorLeft, MOTOR_DIRECTION_FWD, 0);
    motor_setDuty(&motorRight, MOTOR_DIRECTION_FWD, 0);
    currentStateFunc = state_stop;
    state_stop();
}

static void state_set_seek(void)
{
    snprintf(uartBuf, 150, "SEEK START\r\n");
    HAL_UART_Transmit(&huart5, uartBuf, strlen(uartBuf), 1000);
    status_rawLeds((Rgb){20, 0, 0}, (Rgb){0, 0, 0});
    minisumo_patternStart(PATTERN_SEEK);
    currentStateFunc = state_seek;
}

/**
* Starting state. Wait for CONFIG or FIGHT commands.
*/
static void state_start(void)
{
    Rc5Packet    pkt;
    Rc5Ret       ret       = rc5_getPkt(&rc5, &pkt);
    StartStopRet startStop = STARTSTOP_OK;

    if (ret == RC5_OK) {
        switch (pkt.address) {
            case ADDR_STARTSTOP:
            case ADDR_PROGRAMMING:
                startStop = startstop_run(&pkt);
                break;
            case ADDR_CUSTOM_PROG:
                break;
            default:
                status_setLed(LED_B, (Rgb){0, 20, 0});
                break;
        }

        switch (startStop) {
            case STARTSTOP_RUN:
                state_set_seek();
                return;
            case STARTSTOP_PROGRAM_OK:
                status_setLed(LED_A, (Rgb){0, 0, 0});
                break;
            case STARTSTOP_OK:
            case STARTSTOP_ADDRERR:
            case STARTSTOP_DOHYOERR:
            case STARTSTOP_STOP:
            default:
                // Nothing special in these cases.
                break;
        }
    }

    // Cal sensor_isDataReady to cleanup ADC buffers.
    sensors_isDataReady();

    HAL_Delay(10);
}

static void state_seek(void)
{
    Rc5Packet    pkt;
    Rc5Ret       ret       = rc5_getPkt(&rc5, &pkt);
    StartStopRet startStop = STARTSTOP_OK;

    if (ret == RC5_OK) {
        switch (pkt.address) {
            case ADDR_STARTSTOP:
            case ADDR_PROGRAMMING:
                startStop = startstop_run(&pkt);
                break;
            case ADDR_CUSTOM_PROG:
                break;
            default:
                status_setLed(LED_B, (Rgb){0, 20, 0});
                break;
        }

        if (startStop == STARTSTOP_STOP) {
            state_set_stop();
            return;
        }
    }

    if (sensors_isDataReady() == 1) {
        sharp[0]       = sensors_get(SENSOR_SHARP_LEFT);
        sharp[1]       = sensors_get(SENSOR_SHARP_CENTER);
        sharp[2]       = sensors_get(SENSOR_SHARP_RIGHT);
        line[0]        = sensors_get(SENSOR_LINE_LEFT);
        line[1]        = sensors_get(SENSOR_LINE_RIGHT);
        detectedLine   = minisumo_lineDetected(line);
        detectedTarget = minisumo_targetDetected(sharp);
    }

    if (minisumo_handleTargetDetected(detectedTarget)) {
        state_set_fight();
        return;
    }
    if (minisumo_handleLineDetected(detectedLine) == 0) {}

    minisumo_patternLoop();

    motor_update(&motorLeft);
    motor_update(&motorRight);
}

/**
* Fight State. Look for the enemy and attack.
*/

static void state_fight(void)
{
    Rc5Packet    pkt;
    Rc5Ret       ret       = rc5_getPkt(&rc5, &pkt);
    StartStopRet startStop = STARTSTOP_OK;

    if (ret == RC5_OK) {
        switch (pkt.address) {
            case ADDR_STARTSTOP:
            case ADDR_PROGRAMMING:
                startStop = startstop_run(&pkt);
                break;
            case ADDR_CUSTOM_PROG:
                break;
            default:
                status_setLed(LED_B, (Rgb){0, 20, 0});
                break;
        }

        if (startStop == STARTSTOP_STOP) {
            state_set_stop();
            return;
        }
    }

    if (sensors_isDataReady() == 1) {
        sharp[0]       = sensors_get(SENSOR_SHARP_LEFT);
        sharp[1]       = sensors_get(SENSOR_SHARP_CENTER);
        sharp[2]       = sensors_get(SENSOR_SHARP_RIGHT);
        line[0]        = sensors_get(SENSOR_LINE_LEFT);
        line[1]        = sensors_get(SENSOR_LINE_RIGHT);
        detectedLine   = minisumo_lineDetected(line);
        detectedTarget = minisumo_targetDetected(sharp);
    }

    if (detectedLine == DETECTED_LINE_NONE) {
        minisumo_handleTargetDetected(detectedTarget);
        /*snprintf(uartBuf, 150, "Fight detected: %d\r\n", detectedTarget);
        HAL_UART_Transmit(&huart5, uartBuf, strlen(uartBuf), 1000);
        */
    }
    else {
        motorDuty = MOTOR_MOVE_DUTY_PEACE;
        minisumo_move(ROBOT_MOVE_BACK);
        HAL_Delay(100);
        fightInfo.target = DETECTED_TARGET_NONE;
        detectedLine     = DETECTED_LINE_NONE;
        minisumo_moveTarget(ROBOT_MOVE_STOP);
        snprintf(uartBuf, 150, "FGIHT Reversing\r\n");
        HAL_UART_Transmit(&huart5, uartBuf, strlen(uartBuf), 1000);
    }

    motor_update(&motorLeft);
    motor_update(&motorRight);
}
static void state_set_fight(void)
{
    snprintf(uartBuf, 150, "FIGHT START\r\n");
    HAL_UART_Transmit(&huart5, uartBuf, strlen(uartBuf), 1000);
    status_rawLeds((Rgb){20, 0, 0}, (Rgb){0, 0, 20});
    currentStateFunc = state_fight;
    currentStateFunc();
}

/**
* Config State, For custom config commands.
* - Used for calibration
*/
static void state_config(void) {}

static void state_stop(void)
{
    motor_setDuty(&motorLeft, MOTOR_DIRECTION_FWD, 0);
    motor_setDuty(&motorRight, MOTOR_DIRECTION_FWD, 0);
    status_rawLeds((Rgb){20, 0, 0}, (Rgb){0, 0, 0});
    HAL_Delay(200);
    status_rawLeds((Rgb){0, 0, 0}, (Rgb){0, 0, 0});
    HAL_Delay(200);

    Rc5Packet    pkt;
    Rc5Ret       ret       = rc5_getPkt(&rc5, &pkt);
    StartStopRet startStop = STARTSTOP_OK;

    if (ret == RC5_OK) {
        switch (pkt.address) {
            case ADDR_STARTSTOP:
            case ADDR_PROGRAMMING:
                startStop = startstop_run(&pkt);
                break;
            case ADDR_CUSTOM_PROG:
                break;
            default:
                status_setLed(LED_B, (Rgb){0, 20, 0});
                break;
        }

        if (startStop == STARTSTOP_RUN) {
            NVIC_SystemReset();
            return;
        }
    }
}

void minisumo_setup(MinisumoConfig* config)
{

    ee_init(&eepromData, sizeof(eepromData));
    //ee_format();
    ee_read();

    snprintf(uartBuf, 150, "Dohyo ID: 0x%02X\r\n", eepromData.dohyoId);
    HAL_UART_Transmit(&huart5, uartBuf, strlen(uartBuf), 1000);

    startstop_init();

    rc5.init.pin  = GPIO_PIN_1;
    rc5.init.port = GPIOB;
    rc5.init.irq  = EXTI1_IRQn;
    rc5.init.tim  = config->rc5Timer;
    rc5_init(&rc5);

    // config_init();
    status_init(&config->statusConfig);

    motorLeft.init            = config->motorLeft;
    motorLeft.currentSpeed    = 0;
    motorLeft.targetSpeed     = 0;
    motorLeft.targetDirection = MOTOR_DIRECTION_FWD;
    motorLeft.maxRate         = 1;
    motor_init(&motorLeft);
    motorRight.init            = config->motorRight;
    motorRight.currentSpeed    = 0;
    motorRight.targetSpeed     = 0;
    motorRight.targetDirection = MOTOR_DIRECTION_FWD;
    motorRight.maxRate         = 1;
    motor_init(&motorRight);

    adc = config->adc;

    // rc5_init();
    // sensors_init();

    info.led1 = 0;
    info.led2 = 0;
}

void minisumo_loop_new()
{
    currentStateFunc();
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == rc5.init.pin) {
        rc5_handleRising(&rc5);
    }
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == rc5.init.pin) {
        rc5_handleFalling(&rc5);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (htim == rc5.init.tim) {
        rc5_handleTimEvent(&rc5);
    }
}
