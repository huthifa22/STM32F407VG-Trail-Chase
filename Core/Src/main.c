#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <time.h>
#include "stdio.h"

#define RED_LED1 GPIO_PIN_9
#define ORANGE_LED1 GPIO_PIN_1
#define YELLOW_LED1 GPIO_PIN_2
#define GREEN_LED1 GPIO_PIN_3
#define GREEN_LED2 GPIO_PIN_4
#define GREEN_LED3 GPIO_PIN_5
#define YELLOW_LED2 GPIO_PIN_6
#define ORANGE_LED2 GPIO_PIN_7
#define RED_LED2 GPIO_PIN_8
#define JOYSTICK_BUTTON GPIO_PIN_0

void LED_Init(void);
void Button_Init(void);
void toggle_led(GPIO_PinState state, uint16_t led_pin);
void reset_game(void);
void hit(void);
void EXTI0_IRQHandler(void);

uint16_t leds[] = {RED_LED1, ORANGE_LED1, YELLOW_LED1,
                  GREEN_LED1, GREEN_LED2, GREEN_LED3,
                  YELLOW_LED2, ORANGE_LED2, RED_LED2};

int num_leds = sizeof(leds) / sizeof(leds[0]);
int current_led = 0;
int direction = 1;
int score = 0;
int delay = 460;
uint32_t led_change_time = 0;

//Values always read because of ISR changes
volatile int button_pressed = 0;
volatile int reset_in_progress = 0;

int main(void) {
    HAL_Init();
    LED_Init();
    Button_Init();
    current_led = 0;

    HAL_Delay(100);

    while (1) {
    	if (reset_in_progress) continue;

        toggle_led(GPIO_PIN_SET, leds[current_led]);
        HAL_Delay(delay);
        toggle_led(GPIO_PIN_RESET, leds[current_led]);

        led_change_time = HAL_GetTick();
        current_led += direction; //Increment by direction (+1 or -1)

        if (button_pressed && !reset_in_progress) {
            button_pressed = 0;
            uint32_t button_press_time = HAL_GetTick();

            if ((leds[current_led] == RED_LED1 || leds[current_led] == RED_LED2)) {
                if (button_press_time - led_change_time <= 160) { //When LED turned on Minus when pressed
                    hit();
                } else {
                    reset_game();
                }
            } else {
                reset_game();
            }
        }
        //Change Direction
        if (current_led == num_leds - 1 || current_led == 0) {
            direction = -direction;

            if ((direction == 1 && current_led == num_leds - 1) || (direction == -1 && current_led == 0)) {
                if (!button_pressed && !reset_in_progress) {
                    reset_game();
                }
            }
        }
    }
}

void LED_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = RED_LED1 | ORANGE_LED1 | YELLOW_LED1 |
                          GREEN_LED1 | GREEN_LED2 | GREEN_LED3 |
                          YELLOW_LED2 | ORANGE_LED2 | RED_LED2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Button_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = JOYSTICK_BUTTON;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    //Interrupt priority and sub priority for JoyStick press
    HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void toggle_led(GPIO_PinState state, uint16_t led_pin) {
    HAL_GPIO_WritePin(GPIOA, led_pin, state);
}

void hit(void) {
	//Blink 3 green LEDS in middle 3 times
    for (int i = 0; i < 3; i++) {
        toggle_led(GPIO_PIN_SET, GREEN_LED1);
        toggle_led(GPIO_PIN_SET, GREEN_LED2);
        toggle_led(GPIO_PIN_SET, GREEN_LED3);
        HAL_Delay(80);
        toggle_led(GPIO_PIN_RESET, GREEN_LED1);
        toggle_led(GPIO_PIN_RESET, GREEN_LED2);
        toggle_led(GPIO_PIN_RESET, GREEN_LED3);
        HAL_Delay(80);
    }
    //Update Score and increase speed until 40
    score++;
    delay = delay > 40 ? delay - 20 : delay;
    printf("Score: %d\n", score);
}

void reset_game(void) {
    if (reset_in_progress) {
        return;
    }
    reset_in_progress = 1;

    //Blink 3 Times to show restart
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < num_leds; j++) {
            toggle_led(GPIO_PIN_SET, leds[j]);
        }
        HAL_Delay(100);
        for (int j = 0; j < num_leds; j++) {
            toggle_led(GPIO_PIN_RESET, leds[j]);
        }
        HAL_Delay(100);
    }
    //Reset vars
    score = 0;
    printf("Score: %d\n", score);
    delay = 460;
    current_led = 0;
    direction = 1;

    // Wait till button release
    while (HAL_GPIO_ReadPin(GPIOA, JOYSTICK_BUTTON) == GPIO_PIN_SET);

    reset_in_progress = 0;
}

void EXTI0_IRQHandler(void) {
	// Check if interrupt triggered by JoyStick
    if (__HAL_GPIO_EXTI_GET_IT(JOYSTICK_BUTTON) != RESET) {

    	//Clear flag
        __HAL_GPIO_EXTI_CLEAR_IT(JOYSTICK_BUTTON);

        //If not reset state indicate its been pressed
        if (!reset_in_progress) {
            button_pressed = 1;
        }
    }
}
