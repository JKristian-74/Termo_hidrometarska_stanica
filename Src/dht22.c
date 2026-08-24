/* ============================================================
 *  dht22.c — DHT22 driver za STM32 HAL
 *
 *  Tvoj clock:
 *  TIM3 clock = 90 MHz
 *  Prescaler = 44
 *
 *  90 MHz / (44 + 1) = 2 MHz
 *  1 tick = 0.5 us
 *  Zato vrijedi: 1 us = 2 ticka
 * ============================================================ */

#include "dht22.h"

static TIM_HandleTypeDef *dht_tim;
static GPIO_TypeDef      *dht_port;
static uint16_t           dht_pin;

#define DHT_TICKS_PER_US 2

/* ============================================================
 *  delay_us — kašnjenje u mikrosekundama
 *  Kod tebe 1 us = 2 ticka jer TIM3 radi na 2 MHz
 * ============================================================ */
static void delay_us(uint16_t us)
{
    uint32_t target = us * DHT_TICKS_PER_US;

    __HAL_TIM_SET_COUNTER(dht_tim, 0);

    while (__HAL_TIM_GET_COUNTER(dht_tim) < target)
    {
    }
}

/* ============================================================
 *  set_output — GPIO pin kao open-drain izlaz
 * ============================================================ */
static void set_output(void)
{
    GPIO_InitTypeDef cfg = {0};

    cfg.Pin   = dht_pin;
    cfg.Mode  = GPIO_MODE_OUTPUT_OD;
    cfg.Pull  = GPIO_PULLUP;
    cfg.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(dht_port, &cfg);
}

/* ============================================================
 *  set_input — GPIO pin kao ulaz s pull-up
 * ============================================================ */
static void set_input(void)
{
    GPIO_InitTypeDef cfg = {0};

    cfg.Pin  = dht_pin;
    cfg.Mode = GPIO_MODE_INPUT;
    cfg.Pull = GPIO_PULLUP;

    HAL_GPIO_Init(dht_port, &cfg);
}

/* ============================================================
 *  wait_for — čeka određeno stanje pina s timeoutom
 *  timeout_us se pretvara u tickove
 * ============================================================ */
static uint8_t wait_for(GPIO_PinState state, uint16_t timeout_us)
{
    uint32_t timeout_ticks = timeout_us * DHT_TICKS_PER_US;

    __HAL_TIM_SET_COUNTER(dht_tim, 0);

    while (HAL_GPIO_ReadPin(dht_port, dht_pin) != state)
    {
        if (__HAL_TIM_GET_COUNTER(dht_tim) >= timeout_ticks)
        {
            return 0;
        }
    }

    return 1;
}

/* ============================================================
 *  DHT22_Init
 * ============================================================ */
void DHT22_Init(TIM_HandleTypeDef *timer, GPIO_TypeDef *gpio, uint16_t gpio_pin)
{
    dht_tim  = timer;
    dht_port = gpio;
    dht_pin  = gpio_pin;

    HAL_TIM_Base_Start(dht_tim);

    set_output();
    HAL_GPIO_WritePin(dht_port, dht_pin, GPIO_PIN_SET);

    HAL_Delay(2000);
}

/* ============================================================
 *  DHT22_Read
 * ============================================================ */
HAL_StatusTypeDef DHT22_Read(float *temperature, float *humidity)
{
    uint8_t data[5] = {0, 0, 0, 0, 0};

    /* 1. START signal */
    set_output();

    HAL_GPIO_WritePin(dht_port, dht_pin, GPIO_PIN_RESET);
    HAL_Delay(2);

    HAL_GPIO_WritePin(dht_port, dht_pin, GPIO_PIN_SET);
    delay_us(30);

    /* 2. Čekanje odgovora senzora */
    set_input();

    if (!wait_for(GPIO_PIN_RESET, 200)) return HAL_TIMEOUT;
    if (!wait_for(GPIO_PIN_SET,   200)) return HAL_TIMEOUT;
    if (!wait_for(GPIO_PIN_RESET, 200)) return HAL_TIMEOUT;

    /* 3. Čitanje 40 bitova */
    for (int i = 0; i < 40; i++)
    {
        uint32_t high_time_ticks = 0;

        /* Čekaj HIGH dio bita */
        if (!wait_for(GPIO_PIN_SET, 200)) return HAL_TIMEOUT;

        /* Mjeri koliko traje HIGH */
        __HAL_TIM_SET_COUNTER(dht_tim, 0);

        while (HAL_GPIO_ReadPin(dht_port, dht_pin) == GPIO_PIN_SET)
        {
            high_time_ticks = __HAL_TIM_GET_COUNTER(dht_tim);

            if (high_time_ticks > (200 * DHT_TICKS_PER_US))
            {
                return HAL_TIMEOUT;
            }
        }

        /*
         * DHT22:
         * bit 0 = HIGH oko 26 us
         * bit 1 = HIGH oko 70 us
         *
         * Kod tebe:
         * 45 us = 90 tickova
         */
        if (high_time_ticks > (45 * DHT_TICKS_PER_US))
        {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    /* 4. Checksum */
    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4])
    {
        return HAL_ERROR;
    }

    /* 5. Vlaga */
    uint16_t raw_humidity = (data[0] << 8) | data[1];
    *humidity = raw_humidity / 10.0f;

    /* 6. Temperatura */
    uint16_t raw_temperature = ((data[2] & 0x7F) << 8) | data[3];
    *temperature = raw_temperature / 10.0f;

    if (data[2] & 0x80)
    {
        *temperature *= -1.0f;
    }

    return HAL_OK;
}
