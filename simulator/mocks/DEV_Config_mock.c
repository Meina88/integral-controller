/*
 * Mock de DEV_Config — reemplaza drivers/io/DEV_Config.c.
 * Todas las funciones son no-ops en el simulador.
 */
#include <stdint.h>
#include <stdio.h>

void    DEV_Digital_Write(uint16_t pin, uint8_t val) { (void)pin; (void)val; }
uint8_t DEV_Digital_Read(uint16_t pin)               { (void)pin; return 0; }
void    DEV_GPIO_Mode(uint16_t pin, uint16_t mode)   { (void)pin; (void)mode; }
void    DEV_KEY_Config(uint16_t pin)                 { (void)pin; }
void    DEV_GPIO_INT(int32_t pin, void (*isr)(void *)) { (void)pin; (void)isr; }
uint16_t DEC_ADC_Read(void)                          { return 0; }
void    DEV_SPI_WriteByte(uint8_t val)               { (void)val; }
void    DEV_SPI_Write_nByte(uint8_t *d, uint32_t l)  { (void)d; (void)l; }
void    DEV_Delay_ms(uint32_t ms)                    { (void)ms; }

int DEV_I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t val)
    { (void)addr;(void)reg;(void)val; return 0; }
int DEV_I2C_Write_nByte(uint8_t addr, uint8_t *d, uint32_t l)
    { (void)addr;(void)d;(void)l; return 0; }
int DEV_I2C_Read_Byte(uint8_t addr, uint8_t reg, uint8_t *data)
    { (void)addr;(void)reg; if(data)*data=0; return 0; }
int DEV_I2C_Read_nByte(uint8_t addr, uint8_t reg, uint8_t *d, uint32_t l)
    { (void)addr;(void)reg;(void)d;(void)l; return 0; }
void DEV_I2C_SCAN(void) {}
void DEV_SET_PWM(uint8_t val) { (void)val; }

uint8_t DEV_Module_Init(void)
{
    printf("[SIM] DEV_Module_Init OK (mock)\n");
    return 0;
}
