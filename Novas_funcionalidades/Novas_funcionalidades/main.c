/*
 * Sprint10.c
 *
 * Created: 2/10/2021 09:02:18
 * Author : Gabriel Marques Sarmento de Queiroga_118210417
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#define F_CPU 16000000UL
#include "nokia5110.h"

//Protótipos
void temporizador(uint8_t time, uint8_t *t, uint8_t sel);
void semaforo( uint8_t *i,uint8_t t_red,uint8_t t_green, uint8_t t_yellow,uint8_t *tr, uint8_t *tg, uint8_t *ty);
void teste_botao(uint8_t *num);
void display(uint8_t t_red,uint8_t t_green, uint8_t t_yellow, uint8_t botao, uint16_t taxa);
void cursor(uint8_t botao);
void ajuste_automatico();
void Leitura_sensores_ADC( uint8_t *ligar);


//Variáveis globais
uint8_t t_red = 1, t_green = 1,t_yellow = 1, botao = 0,ligar = 0, samu = 0,acidente= 0;
uint8_t flag_[9] = {0},aux = 0, M_A = 0,aux2 = 0;
uint32_t tempo_ms = 0;//Incrementador de timer
uint16_t taxa=0,cont_carro= 0;

//Interrupções
ISR(PCINT2_vect){	//Botao selecional qual cor mudar o tempo
	if(!(PIND & 0b00000001))//Retorna 1 se PD0 for 1 
	{
		//Presença de ambulância na rua- acionar abertura do sinal
		samu = 1;
	
	}else{
		if(!(PIND & 0b00000010))//Retorna 1 se PD1 for 1
		{
			//Possível acidente na rua- para movimentção de veículos
			acidente = 1;
			
		}else{
			if(botao == 0){//Ativa e desativa modo automatico
				teste_botao(0);
				//Verifica se os botões de + ou - são apertados para mudar o modo
				if(M_A){
					ajuste_automatico();// Altera os tempos automaticamente
				}
				display( t_red,t_green,t_yellow,botao, taxa);
		
			}else{
				M_A = 0;
				if(botao == 1){//Mudar tempo do sinal verde
					teste_botao(&t_green);//Teste para aumentar ou diminuir o tempo
			
				}else{if(botao ==2)// Mudar tempo do sinal vermelho
						teste_botao(&t_red);
					else//Mudar tempo do amarelo
						teste_botao(&t_yellow);
			
				}
				display( t_red,t_green,t_yellow,botao, taxa);
				}
			}
			}
}
ISR(INT1_vect){// Muda o cursor
	if(botao ==3){
		botao = 0;
	}else{
		botao++;
	}
	cursor(botao);
}
ISR(INT0_vect){//Interrupção de contagem de carros
	static uint16_t cont_carro =0;
	cont_carro++;
	if(aux2){
		taxa = cont_carro/5*60;//Média de carros em 1 min
		aux2=0;
		cont_carro = 0;
	}
	if(M_A)//Verifica de o modo automático está ligado para ajustar os tempos
		ajuste_automatico();
	if(samu == 0 && acidente ==0)
		display(t_red,t_green,  t_yellow,  botao,  taxa);
}
ISR(TIMER2_COMPA_vect){//Incrementa a casa 1 ms
	tempo_ms ++;
	//PORTD ^=0b00000010;
	//Flag para calcular media de carros em 5s
	if(!(tempo_ms % 5000)){
		aux2 = 1;
	}
	if(!(tempo_ms % 250)){// A cada 500 ms é feita a aquisição da quantidade de luminosidade e de chuva
		
		Leitura_sensores_ADC(&ligar);//Plota os valores lo LCD e verifica se precisa acende a luz
		
		if(ligar == 1)
			OCR0A = 255;//Acende a Iluminação com 100%
		else if(ligar == 2)
				OCR0A = 77;//Iluminação com 30%
			else
				OCR0A = 0;//Apaga a iluminação
	}
}

int main(void)
{
    //Conficurações para interrução por timers
    TCCR2A = 0b00000010; // TC2 em operação CTC
    TCCR2B = 0b00000100; // TC2 com prescaler = 64. Overflow a cada 1ms = 64*(249+1)/16MHz
    OCR2A = 249;//Austa o comparador para TC2 contar até 249
    TIMSK2 = 0b00000010; // habilita a interrupção na igualdade de comparação com OCR2A
    
	DDRC = 0b11011110; //Habilita o pino PC5 como entrada
    DDRD = 0b01000000; // Habilita os pinos PD0..5,7 como entradas - Saída da PD6 para Iluminação  PD0 - Sensor de ambulâncias
    DDRB = 0b00001111; //Habilita os pinos PB0...4 como saidas
    PORTD = 0b10111011; //Habilita os resistores de pull-up da entrada  PD0,1,3..5,7 - Saida em 0
    
	//Registradores para interrupções dos botões +, -, s, A,E, e para entrada de pulso
    EICRA = 0b00001010;//Interrupção sensivel a borda de descida de PD3 e a borda de descida no PD2
    EIMSK = 0b00000011;//Habilitação da interrupção INT1 e INT0
    PCICR = 0b00000100; // Interrupção externa da PORTD
    PCMSK2 = 0b00110011;// Habilita individualmente o PD0,4,5 como interrupção
	
	//CONFIG. do ADC
	ADMUX = 0b01000101; //VCC como ref, canal 5
	ADCSRA = 0b11100111; //Habilita o ADC- modo de conversão contínua - prescaler de 128
	ADCSRB = 0x00; //modo de conversão contínua
	DIDR0 = 0b00100001;// Desabilita o PC5 como entrada digital
	
	//fast PWM, TOP = 0xFF, OC0A habilitado OC0A = PD6
	TCCR0A = 0b10000011; //PWM não invertido nos pinos  OC0A
	TCCR0B = 0b00000100; //liga TC0, prescaler = 256, fpwm = f0sc/(256*prescaler) = 16MHz/(256*256) = 244.1Hz
	OCR0A = 77; //controle do ciclo ativo do PWM OC0A - duty = 77/256 = 30%
	
	
    sei();//Ativas as instruções
    
    nokia_lcd_init();//Inicializa o display
    uint8_t i = 0,tr =0,tg =0,ty =0;
	
    while (1)
    {
			semaforo(&i, t_red, t_green, t_yellow,&tr,&tg,&ty);	//Acende o semáforo		
    }
}

//Função de tempo q muda o estado de t quando percorrido o tempo 'time' selecionado 
//e o 'sel' habilita o tempo do led amarelo (multiplicando o tempo por 4)
void temporizador(uint8_t time, uint8_t *t, uint8_t sel) {
	static uint32_t t_anterior  = 0;
	
	switch(time){
		case 1:
			if(((tempo_ms - t_anterior) >= 250)&&sel){
				aux+=1;
				t_anterior= tempo_ms;
				if(aux == 4){
					*t = 1;
					aux =0;
				}
					
			}else{
				if(((tempo_ms - t_anterior) >= 250)&& (sel==0)){
					*t = 1;
					t_anterior = tempo_ms;
					}
			}
			break;
		case 2:
			if(((tempo_ms - t_anterior) >= 500) && sel){
				aux+=1;
				t_anterior = tempo_ms;
				if(aux == 4){
					*t = 1;
					aux =0;
				}
			}
			else{
				if(((tempo_ms - t_anterior) >= 500)&& (sel==0)){
					*t = 1;
					t_anterior = tempo_ms;
				}
			}
			break;
		case 3:
		if(((tempo_ms - t_anterior) >= 750) && sel){
			aux+=1;
			t_anterior= tempo_ms;
			if(aux == 4){
				*t = 1;
				aux =0;
			}
		}
		else{
			if(((tempo_ms - t_anterior) >= 750)&& (sel==0)){
			*t = 1;
			t_anterior= tempo_ms;
			}
		}
		break;
		case 4:
			if(((tempo_ms - t_anterior) >= 1000) && sel){
				aux+=1;
				t_anterior= tempo_ms;
				if(aux == 4){
					*t = 1;
					aux =0;
				}
			}
			else{
				if(((tempo_ms - t_anterior) >= 1000)&& (sel==0)){
					*t = 1;
					t_anterior= tempo_ms;
					}
			}
			break;
		case 5:
			if(((tempo_ms - t_anterior) >= 1250) && sel ){
				aux+=1;
				t_anterior= tempo_ms;
				if(aux == 4){
					*t = 1;
					aux =0;
				}
			}
			else{
				if(((tempo_ms - t_anterior) >= 1250)&& (sel==0)){
				*t = 1;
				t_anterior= tempo_ms;
				}
			}
			break;
		case 6:
			if(((tempo_ms - t_anterior) >= 1500) && sel){
				aux+=1;
				t_anterior= tempo_ms;
				if(aux == 4){
					*t = 1;
					aux =0;
				}
			}
			else{
				if(((tempo_ms - t_anterior) >= 1500)&& (sel==0)){
					*t = 1;
					t_anterior= tempo_ms;
				}
			}
			break;
		case 7:
			if(((tempo_ms - t_anterior) >= 1750) && sel){
				aux+=1;
				t_anterior= tempo_ms;
				if(aux == 4){
					*t = 1;
					aux =0;
				}
			}
			else{
				if(((tempo_ms - t_anterior) >= 1750)&& (sel==0)){
					*t = 1;
					t_anterior= tempo_ms;
				}
			}
			break;
		case 8:
			if(((tempo_ms - t_anterior) >= 2000)&& sel){
				aux+=1;
				t_anterior= tempo_ms;
				if(aux == 4){
					*t = 1;
					aux =0;
				}
			}
			else{
				if(((tempo_ms - t_anterior) >= 2000)&& (sel==0)){
					*t = 1;
					t_anterior= tempo_ms;
				}
			}
			break;
		case 9:
			if(((tempo_ms - t_anterior) >= 2250) && sel){
				aux+=1;
				t_anterior= tempo_ms;
				if(aux == 4){
					*t = 1;
					aux =0;
				}
			}
			else{
				if(((tempo_ms - t_anterior) >= 2500)&& (sel==0)){
					*t = 1;
					t_anterior= tempo_ms;
				}
			}
			break;
			default:
				*t =1;
	}
}

//Função para teste do botão + -, e Mudar para modo Automático
void teste_botao(uint8_t *num){
	
	if(!(PIND & 0b00010000)) //Retorna 1 se PD4 for 1 e 0 se PD4 for 0
	{	
		if(!botao){
			if(!M_A)
				M_A = !M_A;
		}else
			if(*num < 9 )
			{
				*num+=1;
			}
		
	}else{
		if(!(PIND & 0b00100000))//Retorna 1 se PD5 for 1 e 0 se PD5 for 0
		{
			if(!botao){
				if(M_A)
					M_A = !M_A;
			}else
				if(*num > 1)
				{
					*num-=1;
				}
		}
	}
}

//Muda o estado dos leds do semaforo
void semaforo( uint8_t *i,uint8_t t_red,uint8_t t_green, uint8_t t_yellow,uint8_t *tr, uint8_t *tg, uint8_t *ty){
	
	uint8_t port[9] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	if(samu == 0 && acidente == 0){
		if(*i <=3 ){
			PORTB = port[*i];
			temporizador(t_red, tr,0);
				if(*tr == 1){
					*i+=1;
					*tr = 0;
					}
		}else 
			if(*i >3 && *i<=7){
				PORTB = port[*i];
				temporizador(t_green, tg,0);
					if(*tg == 1){
						*i+=1;
						*tg =0;}
			}else{
					PORTB = port[*i];
					temporizador(t_yellow, ty,1);
					if(*ty ==1){
						*i = 0;
						*ty = 0;
					}
			
			}
		}else{
			if(samu == 1){
				PORTB = port[4];
				temporizador(9, tg,1);
				if(*tg == 1){
					*tg =0;
					samu = 0;
					nokia_lcd_clear();
					nokia_lcd_render();
				}
			}else{
				PORTB = port[0];
				temporizador(9, tr,1);
				if(*tr == 1){
					*tr =0;
					acidente = 0;
					nokia_lcd_clear();
					nokia_lcd_render();
				}
			}
		}
		
}

//Atualiza o tempo no display LCD	
void display(uint8_t t_red,uint8_t t_green, uint8_t t_yellow, uint8_t botao, uint16_t taxa){
	
	unsigned char t_green_string[2];
	unsigned char t_red_string[2];
	unsigned char t_yellow_string[2];
	unsigned char taxa_string[4];
	
	sprintf(t_green_string,"%u",t_green);
	sprintf(t_red_string,"%u",t_red);
	sprintf(t_yellow_string,"%u",t_yellow);
	
	sprintf(taxa_string,"%u",taxa);
	if(samu == 0 && acidente == 0){
	//nokia_lcd_clear();//Limpa o display
	nokia_lcd_set_cursor(0,0); //Muda o cursos para a posição 0,5 ou seja, pula uma linha
	nokia_lcd_write_string("Modo",1); //Escreve um texto do tamanho 1
	nokia_lcd_set_cursor(30, 0);
	if(M_A)
		nokia_lcd_write_string("A",1);
	else
		nokia_lcd_write_string("M",1);
		
	nokia_lcd_set_cursor(0, 10);
	nokia_lcd_write_string("T.Vd",1);
	nokia_lcd_set_cursor(30, 10);
	nokia_lcd_write_string(t_green_string,1);
	nokia_lcd_set_cursor(0, 20);
	nokia_lcd_write_string("T.Vm",1);
	nokia_lcd_set_cursor(30, 20);
	nokia_lcd_write_string(t_red_string,1);//Escreve o tempo do led vermelho com escala 1
	nokia_lcd_set_cursor(0, 30);
	nokia_lcd_write_string("T.Am",1);
	nokia_lcd_set_cursor(30, 30);
	nokia_lcd_write_string(t_yellow_string,1);//Escreve 1 do tamanho 1
	nokia_lcd_set_cursor(40,botao*10);
	nokia_lcd_write_string("<",1);
	for(uint8_t i = 1; i< 45;i++){
		nokia_lcd_set_pixel(45,i,1);
	}
	nokia_lcd_set_cursor(50,25);
	nokia_lcd_write_string(taxa_string,2);
	nokia_lcd_set_cursor(50,40);
	nokia_lcd_write_string("c/min",1);
	nokia_lcd_render(); //Atualiza a tela do display com o conteúdo do buffer
	}
	else{
		if(samu == 1){
			nokia_lcd_clear();
			nokia_lcd_set_cursor(0,0); //Muda o cursos para a posição 0,0 ou seja, pula uma linha
			nokia_lcd_write_string(" Emergencia",1);
			nokia_lcd_set_cursor(0, 12);
			nokia_lcd_write_string(" Identificada",1);
			nokia_lcd_render();
		}else{
			nokia_lcd_clear();
			nokia_lcd_set_cursor(0,0); //Muda o cursos para a posição 0,0 ou seja, pula uma linha
			nokia_lcd_write_string(" Acidente",1);
			nokia_lcd_set_cursor(0, 12);
			nokia_lcd_write_string(" Identificado",1);
			nokia_lcd_set_cursor(0, 25);
			nokia_lcd_write_string("Samu a caminho",1);
			nokia_lcd_set_cursor(0, 35);
			nokia_lcd_write_string("Notificar STTP",1);
			nokia_lcd_render();
		}
	}
	
}

//Altera o cursor de lugar
void cursor(uint8_t botao){
	uint8_t i;
	for(i = 0;i<=3; i++){//Apaga todos os campos do cursor
		nokia_lcd_set_cursor(40,i*10);
		nokia_lcd_write_string(" ",1);
	}
	nokia_lcd_set_cursor(40,botao*10);
	nokia_lcd_write_string("<",1);
	nokia_lcd_render();
}

//Atualiza os tempos do semáforo com base no gráfico apresentado
void ajuste_automatico(){
	t_yellow = 1;
	if(taxa < 60){
		t_green = 1;
		t_red = 9;
		
	}else if(taxa <120){
			t_green = 2;
			t_red = 8;
			
	}else if (taxa <180){
			t_green = 3;
			t_red = 7;
			
	}else if(taxa <240){
		t_green = 4;
		t_red = 6;
				
	}else if(taxa <300){
		t_green = 5;
		t_red = 5;
		
	}else if(taxa <360){
		t_green = 6;
		t_red = 4;
		
	}else if(taxa <420){
		t_green = 7;
		t_red = 3;
		
	}else if(taxa <480){
		t_green = 8;
		t_red = 2;
		
	}else {
		t_green = 9;
		t_red = 1;
	}
	
}

void Leitura_sensores_ADC( uint8_t *ligar){//Ler as entradas analogicas e decide se liga a iluminação
	
	static uint8_t canal =0;
	static uint8_t Chuva_leitura= 0;
	unsigned char Lux_dgt[4] = {0};
	unsigned char leitura_Chuva_string[3];
	 static uint16_t Lux_leitura= 0;

			switch(canal){
				case 0:
					Lux_leitura = 1023000/ADC - 1000;
					ADMUX = 0b01000000;//Mudança para canal PC0_Chuva
					break;
				case 1:
					Chuva_leitura = 50*ADC/1023 +5;
					ADMUX = 0b01000101;//Mudança para canal PC5_Iluminação natural
					break;
			}
		if(canal == 0)
			canal++;
		else
			canal = 0;
		
		sprintf(Lux_dgt, "%u",Lux_leitura);
		sprintf(leitura_Chuva_string,"%u",  Chuva_leitura);
		if(samu == 0 && acidente == 0){
			nokia_lcd_set_cursor(0,40);
			nokia_lcd_write_string("mm/h",1);//milímetros por hora
			nokia_lcd_set_cursor(30,40);
			nokia_lcd_write_string("  ",1);//Para limpar o espaço do numero da chuva
			nokia_lcd_set_cursor(30,40);
			nokia_lcd_write_string("  ",1);
			nokia_lcd_set_cursor(30,40);
			nokia_lcd_write_string(leitura_Chuva_string,1);
			nokia_lcd_set_cursor(50,0);
			nokia_lcd_write_string("   ",2);
			nokia_lcd_set_cursor(50,0);
			nokia_lcd_write_string(Lux_dgt,2);
			nokia_lcd_set_cursor(50,15);
			nokia_lcd_write_string("lux",1);
			nokia_lcd_render();
			display(t_red,t_green,  t_yellow,  botao,  taxa);
		}

		//Condição para ativar a Iluminação
		//Se a chuva estiver com uma taxa de mais de 10mm/h a iluminação irá acender independente da quantidade de luminosidade no ambiente
		if(Chuva_leitura > 10)
			*ligar = 1; //Ligada 100%
		else{
			if(Lux_leitura > 300){
					*ligar = 0;//Apagado
			}
			else
				if ( (taxa != 0) || !(PIND & 0b00000001))
					*ligar = 1;	//Ligada 100%
				else
					*ligar = 2;//Meia luz- Irá ficar com meia luz se estiver com < 300 lux e não estiver passando carros nem pessoas
		}

}

