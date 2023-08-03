#define TAMANHO_AMOSTRAS 32

uint16_t buffer1[TAMANHO_AMOSTRAS] = { 0 };
uint16_t buffer2[TAMANHO_AMOSTRAS] = { 0 };

volatile unsigned long int conta = 0;
volatile bool dadosProntos = false;
volatile uint16_t *ponteiroDados = buffer2;
volatile uint16_t *ponteiroAmostras = buffer1;


ISR(ADC_vect) {
    ponteiroAmostras[conta++] = le_adc();
    if(conta >= TAMANHO_AMOSTRAS)
    {
        conta = 0;
        volatile uint16_t *tmp = ponteiroAmostras;
        ponteiroAmostras = ponteiroDados;
        ponteiroDados = tmp;
        dadosProntos = true;
    }
}

void configura_adc() {
    // Seleciona canal ADC0, ajustado à esquerda, referência de tensão é AVcc.
    ADMUX = 0b01000000;
    DIDR0 |= _BV(ADC0D); // Desabilita entrada digital para o canal ADC0.

    // Habilita o ADC. Habilita operação automática do ADC.
    // Habilita interrupção do ADC e configura o divisor de clock para 128.
    // Isso resulta em um clock base de 125kHz para o ADC. São necessários 13 ciclos para cada leitura
    // o que resulta em 125kS/s / 13 = 9615 amostras por segundo.
    ADCSRA = (_BV(ADEN) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0));

    // Seleciona operação em modo livre (aquisição contínua)
    ADCSRB &= 0b11111000;
}

uint16_t le_adc()
{
    // ADCL deve ser lido antes de ADCH
    uint16_t valor = ADCL & 0xff;
    valor |= (ADCH & 0x00ff) << 8;
    return valor;
}

void setup() {
    Serial.begin(256000);

    configura_adc();
    // Inicia ADC
    ADCSRA |= _BV(ADSC);
}

void loop() {
    if(dadosProntos)
    {
        uint16_t *dados = const_cast<uint16_t*>(ponteiroDados);
        // Cada amostra tem dois bytes, portanto são 2 * TAMANHO_AMOSTRAS bytes.
        Serial.write(reinterpret_cast<char*>(dados), 2 * TAMANHO_AMOSTRAS);

        uint16_t sincroniza = 0xffff;
        Serial.write(reinterpret_cast<char*>(&sincroniza), 2);

        dadosProntos = false;
    }
}
