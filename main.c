#include	"h/main.h"


//************************************************************************
// ��������: ��ʱ
// ����: ��
// ���: 
// ��ע:
//************************************************************************
void Delay(unsigned int count)
{
  unsigned char i,j;
  for(;count>0;count--)
    for(i=100;i>0;i--)
	    for(j=100;j>0;j--);
}




/*************************************************************
*			  ADC�жϺ���
*
*	����:��ʼ��PWM����
*
*	������
*			ADC�ɼ�������ϱ�ʶ
*	�������
*
**************************************************************/
void adc_isr() interrupt 6	 using 1
{
	unsigned int tm=0;

	/*��ADC�жϱ�ʶ EOC/ADCIF*/
	ADCCON &= ~ADCIF;

	//ADCת���꣬�ɼ�����, ����ƽ��ֵ
	tm=ADCVH;
//	tm=tm<<4;
//	tm+=(ADCVL>>4);


	/*�������ݲɼ�ֵ���Ϊ��Ч����*/
	if(adc.adcval[adc.index]!=tm)
	{
		adc.adcval[adc.index]=tm;
		return;
	}

	/*�ɼ���Ч���ݣ�����FILTER_NUM�κ󣬸��»���������*/
	if(++adc.cnt < FILTER_NUM)
	{
		return;
	}
	adc.cnt=0;

	//�������ڻ������ֵ����λ��0
	if(adc.index >= ADC_BUF_NUM -1)
	{
		adc.index = 0;
	}
	else
		adc.index++;
					
//	ADCCON |= ADCS;						//��������ADC ת��
}

/*************************************************************
*			  time1�жϺ���
*
*	����:
*
*	������
*
*	�������
*
**************************************************************/
//void time1_isr() interrupt 3 using 3
//{
//}

/*************************************************************
*			  time2�жϺ���
*
*	����: ���������������ʱ��
*
*	������
*
*	�������
*
**************************************************************/
void time2_isr() interrupt 5  using 2
{
	unsigned char i;

	TF2 = 0;	
	for(i=0;i<MAX_TASK;i++)
	{
		if(task[i].fn == (void *)0 || task[i].stat != READY) continue;
		if(task[i].delay == 0) 
		{	
			if(task[i].run < 255) task[i].run++;
			if(task[i].period > 0)
			{	task[i].delay =  task[i].period;}
		}
		else
		{task[i].delay--;}		 

	}
}


/*************************************************************
*			  int24�жϺ���
*
*	����:
*
*	������
*
*	�������
*
**************************************************************/
void int24_isr() interrupt 10
{
	INT2R =0;								//INT2F4�������жϹر�
	IE1	&=~(1<<3);							//�ر�EINT2�ж�
	int24_flag_bit=ON;
//	P27	= OFF;
	TR0 = OFF;								//�ر�time0��ʱ����ʱ
}


/*************************************************************
*			  int24_init��ʼ������
*
*	����:��ʼ��int24_init����
*
*	˵��:
* 
*	������
*
*	�������
*
**************************************************************/
//void int24_init()
//{
//	switch(machine.water_flag)
//	{
//	 case 0:
//			P2CON	|=	P2C0;							//P2.0�������		
//			P20=OFF;
//			P27	= OFF;
//			machine.water_flag =1;
//			break;
//
//	 case 1:
//			P20=OFF;
//			INT2R =(1<<4);								//INT2F4�������жϿ���
//			IE1	|=(1<<3);								//��EINT2�ж�
//			P27	= ON;
//			machine.water_flag =2;
//			break;
//
//	 case 2:		
//			machine.water_flag =0;
//			break;
//	}
//}



/*************************************************************
*			  UART�жϺ���
*
*	����:
*
*	������
*
*	�������
*
**************************************************************/
#ifdef _UART_
void uart_isr() interrupt 4
{	
	if(TI)
	{	//�崮���жϱ�ʶ
		TI=0;
		if(uart_b.index_sb >= (MAX_UART_SEND_BYTE -1)) return;
		SBUF = uart_b.sb[++uart_b.index_sb];
	}													
	else if(RI)
	{
		RI=0;
		
		if(SBUF == '#' && uart_b.rb[0] !='#')
			uart_b.index_rb = 0;

		uart_b.rb[uart_b.index_rb++]=SBUF;

		if(uart_b.index_rb >= MAX_UART_REC_BYTE)
		{
			uart_b.index_rb = 0;
			uart_b.stat = UART_REC_FINISHED;
		}
	}		
}

void uart_baudrate_adjust()
{
	//��ϵͳƵ�ʸ�����Ϊ�����ʶ�ʱ��1 �Ķ�ʱʱ�� HRCR : ��ʼֵ HRCR=134

	if(machine.hrcr >= (machine.hrcr_origin + 10) )
	{											
		TL1 = 216;
		TH1 = 216;
	}
	else if(machine.hrcr < (machine.hrcr_origin + 10) )
	{											
		TL1 = 217;
		TH1 = 217;
	}
}


unsigned char uart_send(void *p)
{
	unsigned char i,j,str[20];

	p=p;

	memset(uart_b.sb,'\0',sizeof(uart_b.sb));

	//�������ADCת�����ֵ����Сֵ��ƽ��ֵ
	if(uart_b.output_flag & (1<<0))
	{
		//����ADC�������е����ֵ������Сֵ
		for(i=0;i<ADC_BUF_NUM;i++)
		{
			if(*adc.pmax < adc.adcval[i])
				adc.pmax = &(adc.adcval[i]);
	
			if(*adc.pmin > adc.adcval[i])
				adc.pmin = &(adc.adcval[i]);
		}

		sprintf(str,"adc_max_min:");
		strcat(uart_b.sb,str);
		sprintf(str,"[%d,",(unsigned int)(*adc.pmax));
		strcat(uart_b.sb,str);
		sprintf(str,"%d]",(unsigned int)(*adc.pmin));
		strcat(uart_b.sb,str);
	}
	
	//�Ӵ��������ǰADC.AVEֵ
	if(uart_b.output_flag & (1<<1))
	{
		if(adc.max < adc.ave)	adc.max = adc.ave;
		if(adc.min > adc.ave)	adc.min = adc.ave;
		sprintf(str," ave:");
		strcat(uart_b.sb,str);
		sprintf(str,"[%d,",(unsigned int)(adc.max));
		strcat(uart_b.sb,str);
		sprintf(str,"%d,",(unsigned int)(adc.min));
		strcat(uart_b.sb,str);
		sprintf(str,"%d]",(unsigned int)(adc.ave));
		strcat(uart_b.sb,str);

	}

	if(uart_b.output_flag & (1<<2))
	{
		//�Ӵ��������ǰADCֵ
		sprintf(str," [cur:%d]",(unsigned int)(adc.adcval[adc.index]));
		strcat(uart_b.sb,str);
	}

	if(uart_b.output_flag & (1<<3))
	{
		sprintf(str," [stat:%d]",(unsigned int)(machine.stat));
		strcat(uart_b.sb,str);
	}

	if(uart_b.output_flag & (1<<4))
	{
		//�Ӵ��ڷ��ؽ��յ��������ֽ�
		sprintf(str," rb:");
		strcat(uart_b.sb,str);
		for(i=0;i<4;i++)
		{
			sprintf(str,"[%d]",(unsigned int)(uart_b.rb[i]));
			strcat(uart_b.sb,str);
		}
	}

	if(uart_b.output_flag & (1<<5))
	{
		sprintf(str," adcval:");
		strcat(uart_b.sb,str);
		for(i=0;i<ADC_BUF_NUM;i++)
		{
		 	if(i == adc.index)
			{sprintf(str,"[%d],",(unsigned int)(adc.adcval[i]));}
			else			
			{sprintf(str,"%d,",(unsigned int)(adc.adcval[i]));}
			strcat(uart_b.sb,str);
		}
	}

	if(uart_b.output_flag & (1<<6))
	{
		sprintf(str," hrcr:");
		strcat(uart_b.sb,str);
		sprintf(str,"[%d,",(unsigned int)(machine.hrcr_max));
		strcat(uart_b.sb,str);
		sprintf(str,"%d,",(unsigned int)(machine.hrcr_min));
		strcat(uart_b.sb,str);
		sprintf(str,"%d]",(unsigned int)(machine.hrcr));
		strcat(uart_b.sb,str);
	}

	if(uart_b.output_flag & (1<<7))
	{
		sprintf(str," [u:%d,",(unsigned int)(adc.adc_level_up));
		strcat(uart_b.sb,str);
		sprintf(str,"d:%d]",(unsigned int)(adc.adc_level_down));
		strcat(uart_b.sb,str);
	}

//	if(uart_b.output_flag & (1<<8))
//	{
//		sprintf(str," hrcr_ave:");
//		strcat(uart_b.sb,str);
//		for(i=uart_b.begin;i<uart_b.end;i++)
//		{			
//			sprintf(str,"%d:",(unsigned int)(adc.buf[i][0]));
//			strcat(uart_b.sb,str);
//			sprintf(str,"%d,",(unsigned int)(adc.buf[i][1]));
//			strcat(uart_b.sb,str);
//		}
//	}

//	if(uart_b.output_flag & (1<<9))
//	{
//
//	}

	//�Ӵ��������ǰADC.AVEͳ�Ƴ��ִ���ֵ
	if(uart_b.output_flag & (1<<10))
	{
		for(i=uart_b.begin;i<=uart_b.end;i++)
		{			
			sprintf(str,"[%d:",(unsigned int)(adc.buf[i][0]));
			strcat(uart_b.sb,str);
			sprintf(str,"%d],",(unsigned int)(adc.buf[i][1]));
			strcat(uart_b.sb,str);
		}
	}

	if(uart_b.output_flag & (1<<11))
	{
		sprintf(str," [water_stat:%d]",(unsigned int)(machine.water_stat));
		strcat(uart_b.sb,str);
	}


	//���մ��ڵ��������Ƶ�ʼĴ�����ֵ
	if(uart_b.stat == UART_REC_FINISHED)
	{
		switch(uart_b.rb[1])
		{
			case 1:	//����Ƶ�ʼĴ���ֵ
					machine.hrcr = uart_b.rb[2];
			
					OPINX =0x83;
					OPREG = machine.hrcr;

					//��ϵͳƵ�ʸ�����Ϊ�����ʶ�ʱ��1 �Ķ�ʱʱ��
					uart_baudrate_adjust();

					uart_b.stat = COMPLETED;	 
					break;

			case 2:	//����ADC������ֵ
					adc.adc_level_up = uart_b.rb[2];
					adc.adc_level_down = uart_b.rb[3];
					uart_b.stat = COMPLETED;
					break;

			case 3://��ʼ��adc_ave����ֵ										
					for(i=0,j=adc.min;i < ADC_BUF_BYTE0;i++)
					{
						if(j > adc.max) 
						{
							adc.buf[i][0]=0;
							adc.buf[i][1]=0;
							continue;							
						}

						adc.buf[i][0]=j++;
						adc.buf[i][1]=0;
					}

					uart_b.stat = COMPLETED;
					break;

			case 4:	//����Ƶ�ʼĴ���������ֵ
					machine.hrcr_max = uart_b.rb[2];
					machine.hrcr_min = uart_b.rb[3];
					uart_b.stat = COMPLETED;
					break;

			case 5://��ʼ��hrcr_ave����ֵ											
					for(i=0,j=machine.hrcr_min;(j <= machine.hrcr_max) && (i < ADC_BUF_BYTE0);i++,j++)
					{
						adc.buf[i][0]=j;
						adc.buf[i][1]=0;
					}
					uart_b.stat = COMPLETED;
					break;

			case 6:	//ֹͣ��Ƶ 	uart_b.rb[2] = DISABLE_FREQ_TRACE = 9
					machine.stat = uart_b.rb[2];
					uart_b.stat = COMPLETED;
					break;

			case 7:	//��ʼ��ƽ��ֵ�����ֵ����Сֵ
					adc.max = adc.ave;
					adc.min = adc.ave;
					uart_b.stat = COMPLETED;
					break;

			case 8:	//�����������
					uart_b.stat = COMPLETED;											
					uart_b.output_flag = uart_b.rb[2];
					break;

			case 9:	//�����������
					uart_b.stat = COMPLETED;											
					uart_b.output_flag |= (1<<uart_b.rb[2]);
					break;

			case 10://�����������											
					uart_b.output_flag &= ~(1<<uart_b.rb[2]);

					uart_b.stat = COMPLETED;
					break;

			case 11://�����������											
					uart_b.begin = uart_b.rb[2];
					uart_b.end = uart_b.rb[3];
					
					uart_b.stat = COMPLETED;
					break;

			case 12://pwm1�������											
					//����pwm1
					if(uart_b.rb[2] == 1)
						pwm1_start();
					//ֹͣpwm1
					else if(uart_b.rb[2] == 0)
						pwm1_stop();
					
					uart_b.stat = COMPLETED;
					break;

			case 13://ˮλ��������											
					machine.water_stat = uart_b.rb[2];
					
					uart_b.stat = COMPLETED;
					break;

		}
	
	}

	i = strlen(uart_b.sb);
	if(i >= MAX_UART_SEND_BYTE)
		 i = MAX_UART_SEND_BYTE -1;
	uart_b.sb[i] = '\n';

	TI=0;
	uart_b.index_sb=0;
	SBUF = 	uart_b.sb[uart_b.index_sb];

//	for(i=0;i<MAX_UART;i++)
//	{			
//		TI=0;
//		SBUF = 	uart_b.sb[i];
//		while(!TI);
//	}

//   EUART=1;
	return 0;
}

/*************************************************************
*			  uart_on_time1_init()��ʼ������(�������ڲ����ʷ�����)
*
*	����:��ʼ��uart_on_time1_init()����
*
*	˵��:��ʱ��ʱ��ΪFsys  Fsys=12M  time1����uart�����ʷ����� fn2:��ʱ��ʱ��Ƶ�� BaudRate = 2/16 * fn2/(256 - TH2) * 2
* 		 TH2=256-fn2/(16*Baudrate)   Baudrate=19200
*	������
*
*	�������
*
**************************************************************/
void uart_on_time1_init()
{
   PCON |=0X80; 				//��ʽ1
   T2CON &= ~(RCLK | TCLK);  	//ʹ�ö�ʱ��1��UARTʱ��
   
   TMOD &= ~CT1;
   TMOD |= M11;					//��ʱ��1  8λ�Զ�����
   TMOD &= ~M01;

   TMCON |= T1FD; 				//��ʱ��1 = Fsys

	//��ϵͳƵ�ʸ�����Ϊ�����ʶ�ʱ��1 �Ķ�ʱʱ��
   uart_baudrate_adjust();

//   TL1=217;		
//   TH1=217;					//Fsys=24M UART ������38400��
   TR1=0;
//   ET1=1;
   TR1=1;
}


/*************************************************************
*			  uart��ʼ������
*
*	����:��ʼ��uart����
*
*	˵��:��ʱ��ʱ��ΪFsys  Fsys=12M  time1 || time2����uart�����ʷ����� baudrate=19200
* 		 
*	������
*
*	�������
*
**************************************************************/
void uart_init()
{
	P1CON |= (1<<3);									//P1.3����Ϊ���
	P1PH  &= ~(1<<3);
	P1CON &= ~(1<<2);									//P1.2����Ϊ����
	P1PH  |= (1<<2);
	P13   = 1;
	P12   = 1;
	
	uart_b.index_rb = 0;
	uart_b.index_sb = 0;
	uart_b.output_flag = 0x42;
//	uart_b.output_flag = (1<<10) |(1<<1);
	uart_b.begin = 0;
	uart_b.end = 7;

   SCON |= SM01 | REN;									//����ͨ��ģʽ ģʽ1 10λȫ˫���첽ͨ��,�������
   uart_on_time1_init();								//���ڲ����ʷ�����ʹ�ö�ʱ��1
   EUART=1;
}
#endif

/*************************************************************
*			  uart_on_time2_init()��ʼ������
*
*	����:��ʼ��uart_on_time2_init()����
*
*	˵��:��ʱ��ʱ��ΪFsys  Fsys=24M  time2����uart�����ʷ����� fn2:��ʱ��ʱ��Ƶ�� BaudRate = 2/16 * fn2/(65536 - [RCAP2H,RCAP2L]) * 2
* 		 Baudrate=38400
*	������
*
*	�������
*
**************************************************************/
//void uart_on_time2_init()
//{
//    SCON=0X50;         //��ʽ1�������������
//    PCON |=0X80;       //��ʽ1
//	T2CON=0x35;	       //��ʱ��2����������
//	T2MOD=0x03;        //16λ�Զ����ض�ʱ��ģʽ
//	TMCON=0X04; 
//	RCAP2H=(65536-20)/256;			
//	RCAP2L=(65536-20)%256;
//	TR2=0;
////	ET2=1;
//   	TR2=1;
//}




/*************************************************************
*			  time2��ʼ������(�����������������)
*
*	����:��ʼ��time2����
*
*	˵��:��ʱ��ʱ��ΪFsys/12  Fsys=12M  ���ʱ�� 1/24M/12*10000 = 10ms
* 		 �������������ʱ�ӽ���
*	������
*
*	�������
*
**************************************************************/
void time2_init()
{
	T2CON &= ~CT2;										  //time2 �趨Ϊ��ʱ����
	T2CON &= ~CPRL2;									  //16λ�����ع��ܵĶ�ʱ��

	T2MOD &= ~T2OE;
	T2MOD &= ~DCEN;

	TMCON &= ~T2FD;										  //T2 Ƶ��=Fsys/12

	RCAP2L = (65536-10000)%256;
	RCAP2H = (65536-10000)/256;

	TR2 = 0;
	ET2 = 1;
	TR2 = 1;
}


/*************************************************************
*			  time1��ʼ������
*
*	����:��ʼ��time1����
*
*	˵��:��ʱ��ʱ��ΪFsys/12  Fsys=24M  ���ʱ�� 1/24M/12*200 = 100us
* 
*	������
*
*	�������
*
**************************************************************/
void time0_init()
{
	
	TMOD &= ~CT0;										  //time0 �趨Ϊ��ʱ����
	TMOD |= TIME0_16BIT;								  //time0 �趨Ϊ16λ��ʱ��

	TMCON &= ~T0FD;										  //T0 ʱ��Ƶ��=Fsys/12

	TH0 =0;
	TL0 =0;

//	ET0 = ON;
//	TR0 = ON;
}

/*************************************************************
*			  ADC��ʼ������
*
*	����:��ʼ��ADC����
*
*	������
*		  vref		:	�ο���ѹѡ��(vdd:�ο���ѹΪvdd; v2p4:�ο���ѹΪv2p4)
*		  fosc		:	ADCʱ��Ƶ��(ȡֵ��Χ:0--Fosc; 2--Fosc/2; 4--Fosc/4; 6--Fosc/6;)
*
*	�������
*
**************************************************************/
void adc_init(void)
{
	unsigned char i;

	P2CON &= ~P2C1;			 //P2.1ΪADC����
	P2PH  &= ~P2H1;			 //P2.1��������ر�

	ADCCON =0X81;  			//ѡ��AIN1��Ϊ����ͨ����2M����Ƶ��,��ADC��Դ

	ADCCFG0=0X02;  			//ѡ��AIN1ΪADC���룻
	ADCCFG1=0X00;
	
	OPINX =0xc2;
//	OPREG = 0;				//ADC�ο���ѹΪVDD	
	OPREG |= VREFS;			//ADC�ο���ѹΪ�ڲ�2.4V	

	/*��ADC�жϱ�ʶ EOC/ADCIF*/
	ADCCON &= ~ADCIF;
	/*����ADC�ж�*/
	EADC = 1;

	adc.index=0;
	adc.cnt=0;
	adc.ave=0;

	//��������������ADCֵ�޶���680mA����
//	adc.adc_level_up = 20;
//	adc.adc_level_down = 18;
	//��������������ADCֵ�޶���630mA����
	adc.adc_level_up = 19;
	adc.adc_level_down = 17;
	adc.pmax = &adc.adcval[0];
	adc.pmin = &adc.adcval[ADC_BUF_NUM - 1];
	for(i=0;i<ADC_BUF_NUM;i++)
	{
		adc.adcval[i]=i;
	}

//	for(i=0,j=machine.hrcr_min;j <= machine.hrcr_max || i < ADC_BUF_BYTE0;i++,j++)
//	{
//		adc.buf[i][0]=j;
//		adc.buf[i][1]=0;
//	}
			
}

void adc_start(void)
{
	/*���ADC�жϱ�ʶ(ADCIF) ����ADCת��(ADCS)*/
	ADCCON &= ~ADCIF;
	ADCCON |= ADCS;
}

/*************************************************************
*			  PWM��ʼ������
*
*	����:��ʼ��PWM����
*
*	������Fsys=12M
*	�������
*
**************************************************************/
void pwm1_init(void)
{
   	/*����pwm1ʱ��Ƶ��ΪFosc PWMCKS[2:0] 000:Fosc;*/
	PWMCON 	&= ~PWMCKS;	

	//PWM1�����ѡ�� P2.6
	PWMCFG	|= PWMOS1;

   	/*����pwm1�ߵ�λʱ����(���) PWMDTY0[7:0] ���=PWMDTY0xClk.pwm*/
	PWMDTY1 = 3;
 
   	/*����pwm1����Ϊ5 PWMPRD[7:0] ����=(PWMPRD + 1) x Clkpwm */
	PWMPRD	= 6;											
}


void pwm1_start(void)
{
	/*pwm1����*/
	PWMCON	|= ENPWM | ENPWM1;							//����pwm����ʱ�� pwm1�����IO
	machine.pwm1_stat=PWM1_RUN;
}

void pwm1_stop(void)
{
	/*pwm1�ر����*/
	PWMCON	&= ~(ENPWM | ENPWM1);						//ֹͣpwm����ʱ�� pwm1�����IO
	machine.pwm1_stat=PWM1_STOP;
}



/*************************************************************
*			  Ӳ����ʼ������
*
*	����:
*			1.����ʱ��Ƶ��Ϊ��Ƶ��Ƶ�ʵ�2��Ƶ
*			2.������Ƶ��12M + n x 23.4kHz
*			3.����P2.6 P2.0Ϊ�������
*	������
*		  	OPINX: ��Ƶ�Ĵ���ָ��
*		  	OPREG: ��Ƶֵ�Ĵ���
*			HRCR : ��ʼֵ HRCR=134
*
*	�����
*			HRCR+=5	:	1.731M	MOS��΢��	630mA	  ����***
*
**************************************************************/
void hardware_init(void)
{
	P2CON	|=	P2C6;							//P2.6�������
	P2CON	|=	P2C7;							//P2.7�������

	EA	= ON;
}

unsigned char adc_val_handler(void *p)
{
	unsigned int bf=0,tm=0;
	unsigned char i;

	p=p;

	if((ADCCON & ADCIF)) return 1;				//û��ת������˳�

	//����ADCƽ��ֵ
	for(i=0;i<ADC_BUF_NUM;i++)
	{
		if(i == adc.index) continue;
		bf += adc.adcval[i];
	}	
	adc.ave = bf/(ADC_BUF_NUM-1);

#ifdef _DEBUG
for(i=0;i<ADC_BUF_BYTE0;i++)
{
	if(adc.buf[i][0] == 0) break;
	if(adc.buf[i][0] == adc.ave)
		adc.buf[i][1]++;
}
#endif	

										
	/*��������ADCת��(ADCS)*/
	ADCCON |= ADCS;						//��������ADC ת��
	
	return 0;						
}


/*************************************************************
*			  Ƶ��׷��
*
*	������
*		  	OPINX: ��Ƶ�Ĵ���ָ��
*		  	OPREG: ��Ƶֵ�Ĵ���
*			HRCR : ��ʼֵ HRCR=134
*			0x83 : ʱ��Ƶ�ʵ�������ַ
*	�����
*
*	˵����
*			adc.ave��adc.adc_level_down ��adc.adc_level_up�Ƚ� ����ʱ��Ƶ�ʼĴ�����ֵ��
*			��������ʱ��Ƶ�ʣ���ӵ���PWM���Ƶ�ʵ�Ŀ�ġ�
*
**************************************************************/
unsigned char freq_trace(void *p)
{
//	unsigned char i;

	p=p;

	//�ɼ�ADCֵ���ڷ�Χ��pwm1ֹͣ����ʱ,ֹͣƵ�ʸ���
	if((adc.ave <= FREQ_TRACE_DOWN) || (adc.ave >= FREQ_TRACE_UP) || (machine.pwm1_stat == PWM1_STOP))
	{
		machine.stat = DISABLE_FREQ_TRACE;		
		return 4;
	}

	machine.stat = ENABLE_FREQ_TRACE;

	if(adc.ave <= adc.adc_level_down)
	{
		if(++machine.freq_trace_cnt_threshold[0] <= machine.freq_trace_cnt_threshold[1]) return 1;
		machine.freq_trace_cnt_threshold[0]=0;

		if(adc.adcval[adc.index] > adc.ave) return 2;

		//����ʱ��Ƶ�� 
		machine.hrcr--;
		OPINX =0x83;
		OPREG = machine.hrcr;

		#ifdef _UART_
		//��ϵͳƵ�ʸ�����Ϊ�����ʶ�ʱ��1 �Ķ�ʱʱ��
		uart_baudrate_adjust();
		#endif
	}
	else if(adc.ave >=  adc.adc_level_up)
	{
		if(++machine.freq_trace_cnt_threshold[0] <=machine.freq_trace_cnt_threshold[1]) return 1;
		machine.freq_trace_cnt_threshold[0]=0;

		if(adc.adcval[adc.index] < adc.ave) return 1;

		//����ʱ��Ƶ�� 
		machine.hrcr++;
		OPINX =0x83;
		OPREG = machine.hrcr;

		#ifdef _UART_
		//��ϵͳƵ�ʸ�����Ϊ�����ʶ�ʱ��1 �Ķ�ʱʱ��
		uart_baudrate_adjust();
		#endif
	}
	else
	{
		machine.freq_trace_cnt_threshold[0]=0;
		task[2].period = 200;
		machine.freq_trace_cnt_threshold[1]=5;
	}

	return 0;
}

/*************************************************************
*			  ˮλ���
*
*	������
*
*	�����
*
*	˵����
*
*			
*
**************************************************************/
unsigned char no_water_detect(void *p)
{
	p = p;

	//��ˮ���ռ�⣬�ɼ��ĵ�ǰ���ݴ���ADC_NO_WATER_THRESHOLD ��ˮ��ֵ��ֹͣPWM1�����
	if(adc.adcval[adc.index] >= ADC_NO_WATER_THRESHOLD)	
	{
		pwm1_stop();
		machine.water_stat = NO_WATER;
	}

	return 0;
}

void water_detect_init()
{
	P2CON	|=	P2C0;							//P2.0�������		
	P20=OFF;
//	P27	= OFF;
}

void water_detect_start()
{
	P2CON	&=	~P2C0;							//P2.0����	
	INT2R =(1<<4);								//INT2F4�������жϿ���
	IE1	|=(1<<3);								//��EINT2�ж�
	int24_flag_bit=OFF;
//	P27	= ON;

	TH0 =0;
	TL0 =0;
	TR0 = ON;									//����time0��ʱ����ʱ
}

void water_detect()
{
	unsigned int water_level_time=0;						//ˮλ���ʱ�Ӽ���

	water_level_time = TH0;
	water_level_time = (water_level_time<<8);
	water_level_time += TL0;								

	if(water_level_time < WATER_FULL_UP && water_level_time > WATER_FULL_DOWN)
	{
		pwm1_start();
		task[4].period =10;						//pwm1���������̵�ˮλ̽�ⷴӦʱ��
		machine.water_stat = FULL_WATER;
	}
	else if(water_level_time < WATER_LOW_UP && water_level_time > WATER_LOW_DOWN)
	{
		pwm1_stop();
		task[4].period =100;					//pwm1ֹͣ���ӳ���ˮλ̽��ָ�ʱ��
		machine.water_stat = LOW_WATER;
	}																		
	else
	{
		pwm1_stop();
		task[4].period =100;
		machine.water_stat = ERROR;
	}
}

unsigned char low_water_detect(void *p)
{
	p = p;

	//��ˮλ��� LOW_WATER_DETECT
	switch(machine.water_cnt)
	{
	 case 0:
			water_detect_init();
//			P2CON	|=	P2C0;							//P2.0�������		
//			P20=OFF;
//			P27	= OFF;
			machine.water_cnt =1;
			break;

	 case 1:
			water_detect_start();
//			P20=OFF;
//			P2CON	&=	~P2C0;							//P2.0����	
//			INT2R =(1<<4);								//INT2F4�������жϿ���
//			IE1	|=(1<<3);								//��EINT2�ж�
//			int24_flag_bit=OFF;
//			P27	= ON;
//
//			TH0 =0;
//			TL0 =0;
//			TR0 = ON;									//����time0��ʱ����ʱ
			machine.water_cnt =2;
			break;

	 case 2:		
			//������ȡ����������
			if(int24_flag_bit == ON)
			{
				water_detect();
//				water_level_time = TH0;
//				water_level_time = (water_level_time<<8);
//				water_level_time += TL0;								
//
//				if(water_level_time < WATER_FULL_UP && water_level_time > WATER_FULL_DOWN && machine.water_stat == LOW_WATER)
//				{
//					pwm1_start();
//					machine.water_stat = FULL_WATER;
//				}
//				else if(water_level_time < WATER_LOW_UP && water_level_time > WATER_LOW_DOWN && machine.water_stat == FULL_WATER)
//				{
//					pwm1_stop();
//					machine.water_stat = LOW_WATER;
//				}																		
			}
			else
			{
				pwm1_stop();
				machine.water_stat = ERROR;
			}
						
			machine.water_cnt =0;
			break;
		
	}
	return 0;
}

void machine_init()
{
	memset(&machine,0,sizeof(struct MACHINE));

	//����ʱ��Ƶ�� Fsys=Fcyc/2
	OPINX =0xc1;
	OPREG |= SCLKS0;
	OPREG &= ~SCLKS1;

	//����ʱ��Ƶ�� 12000 + 2 x 23.4 kHz =12046800 kHz
	OPINX =0x83;
	machine.hrcr_origin=OPREG;
	OPREG += 14;
	machine.hrcr=OPREG;

	machine.hrcr_max = 160;
	machine.hrcr_min = 141;
	//��ʼ������hrcr �ķ�ֵ

	machine.freq_trace_cnt_threshold[1]=3;

	//��ˮλ��� LOW_WATER_DETECT
//	if(LOW_WATER_DETECT == ON)
//	{
//		machine.water_stat = FULL_WATER;
//		pwm1_start();
//	}
//	else
//	{
//		machine.water_stat = LOW_WATER;
//		pwm1_stop();
//	}


//	machine.stat = ENABLE_FREQ_TRACE;	
}

void schedule_init()
{
	memset(task,0,sizeof(task));

//	unsigned char i;
//
//	for(i=0;i<MAX_TASK;i++)
//	{
//		task[i].fn=(void *)0;
//		task[i].delay=0;
//		task[i].period=0;
//		task[i].delay=0;
//		task[i].stat=0;
//		task[i].run=0;
//	}
}

unsigned char task_add(unsigned char tid,fn f,unsigned int d,unsigned int p,enum STATU s)
{
	if(	task[tid].fn != (void *)0 || tid >MAX_TASK)
		return 1;	

	task[tid].fn = f;
	task[tid].delay = d;
	task[tid].period = p;
	task[tid].stat = s;
	task[tid].run = 0;

	return 0;
}

//void task_delete(struct TASK * pt)
//{
//	pt->fn=(void *)0;
//	pt->delay=0;
//	pt->period=0;
//	pt->delay=0;
//	pt->stat=0;
//	pt->run=0;
//}


void schedule()
{
	unsigned char i;

	for(i=0;i<MAX_TASK;i++)
	{		
		if(task[i].fn == (void *)0 || task[i].stat != READY) continue;
		if(task[i].run > 0)
		{
		 	task[i].run--;

			(*task[i].fn)((void *)0);

		}
	}
}


//************************************************************************
// ��������: ������
// ����: ��
// ���: 
// ��ע: 
//************************************************************************			
void main()
{						   		
	hardware_init();

	machine_init();

	schedule_init();

	pwm1_init();

	time0_init();
	time2_init();

	#ifdef _UART_
	uart_init();
	#endif

//	task_add(0,uart_send,100,100,READY);
//      this is a comment

	task_add(1,adc_val_handler,3,3,READY);

	task_add(2,freq_trace,200,200,READY);

//	pwm1_start();
//	pwm1_stop();

	task_add(3,no_water_detect,1,1,READY);

	task_add(4,low_water_detect,10,10,READY);
	
	adc_init();
 	adc_start();			
	
	water_detect_init();
	water_detect_start();
	Delay(100);
	while(!int24_flag_bit);
	water_detect();

	while(1)
	{		
		schedule();						
    }
}
