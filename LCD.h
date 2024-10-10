#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>
#include <util/delay.h>

#define DATA_BUS	PORTD
#define CTL_BUS		PORTD
#define DATA_DDR	DDRD
#define CTL_DDR		DDRD
#define LCD_D4			4
#define LCD_D5			5
#define LCD_D6			6
#define LCD_D7			7
#define LCD_EN			3			
#define	LCD_RS			2

#define LCD_CMD_CLEAR_DISPLAY	          0x01
#define LCD_CMD_CURSOR_HOME		          0x02

// Display control
#define LCD_CMD_DISPLAY_OFF                0x08
#define LCD_CMD_DISPLAY_NO_CURSOR          0x0c
#define LCD_CMD_DISPLAY_CURSOR_NO_BLINK    0x0E
#define LCD_CMD_DISPLAY_CURSOR_BLINK       0x0F

// Function set
#define LCD_CMD_4BIT_2ROW_5X7              0x28
#define LCD_CMD_8BIT_2ROW_5X7              0x38

void lcd_send_command (uint8_t command)
{
	DATA_BUS=(command&0b11110000); 
	CTL_BUS &=~(1<<LCD_RS);
	CTL_BUS |=(1<<LCD_EN);
	_delay_ms(1);
	CTL_BUS &=~(1<<LCD_EN);
	_delay_ms(1);
	DATA_BUS=((command&0b00001111)<<4);
	CTL_BUS |=(1<<LCD_EN);
	_delay_ms(1);
	CTL_BUS &=~(1<<LCD_EN);
	_delay_ms(1);
}

void lcd_init(void)
{	
	
	DATA_DDR = (1<<LCD_D7) | (1<<LCD_D6) | (1<<LCD_D5)| (1<<LCD_D4);
	CTL_DDR |= (1<<LCD_EN)|(1<<LCD_RS);

	
	DATA_BUS = (0<<LCD_D7)|(0<<LCD_D6)|(1<<LCD_D5)|(0<<LCD_D4);
	CTL_BUS|= (1<<LCD_EN)|(0<<LCD_RS);
	_delay_ms(1);
	CTL_BUS &=~(1<<LCD_EN);
	_delay_ms(1);
	
	lcd_send_command(LCD_CMD_4BIT_2ROW_5X7);
	_delay_ms(1);
	lcd_send_command(LCD_CMD_DISPLAY_CURSOR_BLINK);
	_delay_ms(1);
	lcd_send_command(0x80);
    _delay_ms(10);
	
}


void lcd_write_character(char character)
{
	
	DATA_BUS=(DATA_BUS & 0x0F) | (character & 0b11110000);
	CTL_BUS|=(1<<LCD_RS);
	CTL_BUS |=(1<<LCD_EN);
	_delay_ms(2);
	CTL_BUS &=~(1<<LCD_EN);
	_delay_ms(2);
	DATA_BUS= (DATA_BUS & 0x0F) | ((character & 0b00001111)<<4);
	CTL_BUS |=(1<<LCD_EN);
	_delay_ms(2);
	CTL_BUS &=~(1<<LCD_EN);
	_delay_ms(2);
	
}

void lcd_write_str(char* str)
{
	int i=0;
	while(str[i]!='\0')
	{
		lcd_write_character(str[i]);
		i++;
	}
}

void lcd_clear()
{
	lcd_send_command(LCD_CMD_CLEAR_DISPLAY);
	_delay_ms(5);
}
void lcd_goto_xy (uint8_t line,uint8_t pos)				//line = 0 or 1
{
	lcd_send_command((0x80|(line<<6))+pos);
	_delay_us (50);
}

//Added code to create custom character
void lcd_send_nibble(uint8_t nibble) {
    PORTD = (PORTD & 0x0F) | (nibble << 4);
    PORTD |= (1 << LCD_EN);
    _delay_us(1);
    PORTD &= ~(1 << LCD_EN);
    _delay_us(100);
}
void lcd_send_byte(uint8_t byte, uint8_t mode) {
    if (mode) {
        PORTD |= (1 << LCD_RS);
    } else {
        PORTD &= ~(1 << LCD_RS);
    }
    _delay_us(1);
    lcd_send_nibble(byte >> 4);
    lcd_send_nibble(byte & 0x0F);
}
void lcd_command(uint8_t cmd) {
    lcd_send_byte(cmd, 0);
}
void lcd_data(uint8_t data) {
    lcd_send_byte(data, 1);
}
void lcd_write_custom_char(uint8_t loc, uint8_t *char_map) {
    lcd_command(0x40 + (loc * 8));
    for (uint8_t i = 0; i < 8; i++) {
        lcd_data(char_map[i]);
    }
}

#endif /* LCD_H_ */