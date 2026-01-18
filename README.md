Barômetro Digital AVR (ATmega328P + BMP390 + OLED SSD1306 I2C)

Projeto de um barômetro digital simples e preciso, feito em C “bruto” (AVR-GCC / Microchip Studio), usando um ATmega328P com clock interno e comunicação I2C (TWI) para ler o sensor BMP390 (Bosch) e mostrar os valores em um display OLED 0.96” 128×64 (SSD1306).

O objetivo é ter um instrumento offline (sem Wi-Fi/Bluetooth), com leitura estável e boa apresentação para uso cotidiano (casa/oficina).

Funcionalidades

Leitura de pressão atmosférica via BMP390

Leitura de temperatura via BMP390

Exibição no OLED 128×64 I2C (SSD1306)

Driver I2C (TWI) próprio, sem bibliotecas Arduino

Conversão:

Pressão em Pa (interno) e exibida em hPa

Temperatura em °C

Código modular em arquivos .c/.h

Hardware
Componentes

ATmega328P

BMP390 (I2C, endereço 0x76 ou 0x77)

OLED 0.96” 128×64 I2C (SSD1306, endereço 0x3C ou 0x3D)

Resistores de pull-up para I2C (se seu módulo não tiver)

Alimentação (importante)

BMP390 geralmente trabalha em 3.3V.

OLED I2C geralmente aceita 3.3V ou 5V (depende do módulo).

Se o ATmega estiver em 5V, use level shifting ou garanta compatibilidade do barramento I2C.

Ligações (I2C)
ATmega328P (TWI padrão)

SDA: PC4 (A4)

SCL: PC5 (A5)

Conectar em paralelo:

SDA → SDA do BMP390 e do OLED

SCL → SCL do BMP390 e do OLED

GND comum entre todos

Endereços I2C

BMP390: 0x76 ou 0x77 (depende do pino SDO do módulo)

OLED SSD1306: 0x3C (mais comum) ou 0x3D

Se não aparecer nada, teste os endereços alternativos no main.c.

Estrutura do projeto

main.c — loop principal (leitura do sensor e atualização do OLED)

twi_master.c/.h — driver I2C (TWI) do ATmega328P

bmp390.c/.h — driver do BMP390 (leitura + compensação)

ssd1306.c/.h — driver do OLED SSD1306 (comandos, clear, print)

Compilação
Microchip Studio / AVR-GCC

Configure F_CPU conforme seu clock (ex.: 1 MHz)

Certifique-se de adicionar todos os .c no projeto (Source Files)

Exemplo:

#define F_CPU 1000000UL

Uso

Ligue o circuito com SDA/SCL e GND comum.

Ajuste os endereços I2C no main.c:

OLED: 0x3C ou 0x3D

BMP390: 0x76 ou 0x77

Grave o firmware no ATmega328P.

O OLED mostrará:

Pressão (hPa)

Temperatura (°C)

Observações

Se o build falhar com erros de link, confira:

se bmp390.c, ssd1306.c e twi_master.c estão incluídos no projeto

Se o display ficar em branco:

teste 0x3C e 0x3D

verifique pull-ups no I2C

Se o BMP390 não responder:

teste 0x76 e 0x77

confirme alimentação 3.3V
