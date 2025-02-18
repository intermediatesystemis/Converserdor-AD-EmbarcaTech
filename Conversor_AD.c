#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"

#define BUTTON_JOY 22
#define BUTTON_A 5
#define PIN_JOY_VRY 26
#define PIN_JOY_VRX 27
#define LED_PIN_BLUE 12
#define LED_PIN_RED 13
#define LED_PIN_GREEN 11
#define MAX_RESOLUTION 4095
#define OFFSET 300
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C


static volatile uint32_t last_time_joy = 0;
static volatile uint32_t last_time_a = 0;
static volatile bool pwm_state = true;
static ssd1306_t ssd; 

void config_pwm(int pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_clkdiv(slice, 8.0);
    pwm_set_wrap(slice, MAX_RESOLUTION);
    pwm_set_gpio_level(pin, 0);
    pwm_set_enabled(slice, true);
}

void controlar_led(int read_joy, int pin, int center) {
    if (read_joy > center - OFFSET && read_joy < center + OFFSET) {
        pwm_set_gpio_level(pin, 0);
    } else if (read_joy >= center + OFFSET) {
        pwm_set_gpio_level(pin, read_joy);
    } else if (read_joy <= center - OFFSET) {
        pwm_set_gpio_level(pin, MAX_RESOLUTION - read_joy);
    }
}

void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time()); 

    if (gpio == BUTTON_JOY && current_time - last_time_joy > 200000) {
        gpio_put(LED_PIN_GREEN, !gpio_get(LED_PIN_GREEN));

        if (gpio_get(LED_PIN_GREEN)) {
            ssd1306_fill(&ssd, false);
            ssd1306_send_data(&ssd);            
            ssd1306_rect(&ssd, 0, 0, 128, 64, true, false);    
            ssd1306_send_data(&ssd);       
        } else {
            ssd1306_fill(&ssd, false);
            ssd1306_send_data(&ssd);            
            ssd1306_rect(&ssd, 4, 9, 110, 55, true, false);    
            ssd1306_send_data(&ssd);
        }

        last_time_joy = current_time;
    } else if (gpio == BUTTON_A && current_time - last_time_a > 200000) {
        uint slice_blue = pwm_gpio_to_slice_num(LED_PIN_BLUE);
        uint slice_red = pwm_gpio_to_slice_num(LED_PIN_RED);
        pwm_state = !pwm_state;
        pwm_set_enabled(slice_blue, pwm_state);
        pwm_set_enabled(slice_red, pwm_state);         

        last_time_a = current_time;
    }
}

int main()
{
    stdio_init_all();    
    
    adc_init();
    adc_gpio_init(PIN_JOY_VRY);   
    adc_select_input(0); // Canal 0 

    adc_gpio_init(PIN_JOY_VRX);   
    adc_select_input(1); // Canal 1

    config_pwm(LED_PIN_BLUE);
    config_pwm(LED_PIN_RED);

    gpio_init(BUTTON_JOY);
    gpio_set_dir(BUTTON_JOY, GPIO_IN);
    gpio_pull_up(BUTTON_JOY);
    gpio_set_irq_enabled_with_callback(BUTTON_JOY, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);

    i2c_init(I2C_PORT, 400 * 1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line  
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

   


    while (true) {
        adc_select_input(0); //ler o canal 0 
        uint16_t read_joy_vry = adc_read();
        controlar_led(read_joy_vry, LED_PIN_BLUE, 2030);
       
        adc_select_input(1); //ler o canal 1
        uint16_t read_joy_vrx = adc_read();
        controlar_led(read_joy_vrx, LED_PIN_RED, 1997); 
                    
        ssd1306_fill(&ssd, false);  
        //controla o movimendo do quadrado no display
        ssd1306_rect(&ssd, 56 - ((abs(read_joy_vry - 512) / 64) % 57), (abs(read_joy_vrx - 256) / 32) % 120, 8, 8, true, true);
        sleep_ms(10);
        ssd1306_send_data(&ssd);
    }
}
