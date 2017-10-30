#include	"h/main.h"


//************************************************************************
// 函数功能: 延时
// 输入: 无
// 输出: 
// 备注:
//************************************************************************
void Delay(unsigned int count)
{
  unsigned char i,j;
  for(;count>0;count--)
    for(i=100;i>0;i--)
	    for(j=100;j>0;j--);
}




/*************************************************************
*			  ADC中断函数
*
*	功能:初始化PWM功能
*
*	参数：
*			ADC采集数据完毕标识
*	输出：无
*
**************************************************************/
void adc_isr() interrupt 6	 using 1
{
	unsigned int tm=0;

	/*清ADC中断标识 EOC/ADCIF*/
	ADCCON &= ~ADCIF;

	//ADC转换完，采集数据, 计算平均值
	tm=ADCVH;
//	tm=tm<<4;
//	tm+=(ADCVL>>4);


	/*两次数据采集值相等为有效数据*/
	if(adc.adcval[adc.index]!=tm)
	{
		adc.adcval[adc.index]=tm;
		return;
	}

	/*采集有效数据，过滤FILTER_NUM次后，更新缓冲区数据*/
	if(++adc.cnt < FILTER_NUM)
	{
		return;
	}
	adc.cnt=0;

	//索引大于缓冲最大值，复位到0
	if(adc.index >= ADC_BUF_NUM -1)
	{
		adc.index = 0;
	}
	else
		adc.index++;
					
//	ADCCON |= ADCS;						//重新启动ADC 转换
}

/*************************************************************
*			  time1中断函数
*
*	功能:
*
*	参数：
*
*	输出：无
*
**************************************************************/
//void time1_isr() interrupt 3 using 3
//{
//}

/*************************************************************
*			  time2中断函数
*
*	功能: 用作任务调度器计时器
*
*	参数：
*
*	输出：无
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
*			  int24中断函数
*
*	功能:
*
*	参数：
*
*	输出：无
*
**************************************************************/
void int24_isr() interrupt 10
{
	INT2R =0;								//INT2F4上升沿中断关闭
	IE1	&=~(1<<3);							//关闭EINT2中断
	int24_flag_bit=ON;
//	P27	= OFF;
	TR0 = OFF;								//关闭time0定时器计时
}


/*************************************************************
*			  int24_init初始化函数
*
*	功能:初始化int24_init功能
*
*	说明:
* 
*	参数：
*
*	输出：无
*
**************************************************************/
//void int24_init()
//{
//	switch(machine.water_flag)
//	{
//	 case 0:
//			P2CON	|=	P2C0;							//P2.0推挽输出		
//			P20=OFF;
//			P27	= OFF;
//			machine.water_flag =1;
//			break;
//
//	 case 1:
//			P20=OFF;
//			INT2R =(1<<4);								//INT2F4上升沿中断开启
//			IE1	|=(1<<3);								//打开EINT2中断
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
*			  UART中断函数
*
*	功能:
*
*	参数：
*
*	输出：无
*
**************************************************************/
#ifdef _UART_
void uart_isr() interrupt 4
{	
	if(TI)
	{	//清串口中断标识
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
	//随系统频率更新作为波特率定时器1 的定时时间 HRCR : 初始值 HRCR=134

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

	//串口输出ADC转移最大值，最小值，平均值
	if(uart_b.output_flag & (1<<0))
	{
		//计算ADC缓冲区中的最大值，和最小值
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
	
	//从串口输出当前ADC.AVE值
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
		//从串口输出当前ADC值
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
		//从串口发回接收到的命令字节
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

	//从串口输出当前ADC.AVE统计出现次数值
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


	//按照串口的命令更新频率寄存器的值
	if(uart_b.stat == UART_REC_FINISHED)
	{
		switch(uart_b.rb[1])
		{
			case 1:	//调整频率寄存器值
					machine.hrcr = uart_b.rb[2];
			
					OPINX =0x83;
					OPREG = machine.hrcr;

					//随系统频率更新作为波特率定时器1 的定时时间
					uart_baudrate_adjust();

					uart_b.stat = COMPLETED;	 
					break;

			case 2:	//调整ADC上下限值
					adc.adc_level_up = uart_b.rb[2];
					adc.adc_level_down = uart_b.rb[3];
					uart_b.stat = COMPLETED;
					break;

			case 3://初始化adc_ave缓冲值										
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

			case 4:	//调整频率寄存器上下限值
					machine.hrcr_max = uart_b.rb[2];
					machine.hrcr_min = uart_b.rb[3];
					uart_b.stat = COMPLETED;
					break;

			case 5://初始化hrcr_ave缓冲值											
					for(i=0,j=machine.hrcr_min;(j <= machine.hrcr_max) && (i < ADC_BUF_BYTE0);i++,j++)
					{
						adc.buf[i][0]=j;
						adc.buf[i][1]=0;
					}
					uart_b.stat = COMPLETED;
					break;

			case 6:	//停止调频 	uart_b.rb[2] = DISABLE_FREQ_TRACE = 9
					machine.stat = uart_b.rb[2];
					uart_b.stat = COMPLETED;
					break;

			case 7:	//初始化平均值的最大值，最小值
					adc.max = adc.ave;
					adc.min = adc.ave;
					uart_b.stat = COMPLETED;
					break;

			case 8:	//串口输出控制
					uart_b.stat = COMPLETED;											
					uart_b.output_flag = uart_b.rb[2];
					break;

			case 9:	//串口输出控制
					uart_b.stat = COMPLETED;											
					uart_b.output_flag |= (1<<uart_b.rb[2]);
					break;

			case 10://串口输出控制											
					uart_b.output_flag &= ~(1<<uart_b.rb[2]);

					uart_b.stat = COMPLETED;
					break;

			case 11://串口输出控制											
					uart_b.begin = uart_b.rb[2];
					uart_b.end = uart_b.rb[3];
					
					uart_b.stat = COMPLETED;
					break;

			case 12://pwm1输出控制											
					//开启pwm1
					if(uart_b.rb[2] == 1)
						pwm1_start();
					//停止pwm1
					else if(uart_b.rb[2] == 0)
						pwm1_stop();
					
					uart_b.stat = COMPLETED;
					break;

			case 13://水位调整控制											
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
*			  uart_on_time1_init()初始化函数(用作串口波特率发生器)
*
*	功能:初始化uart_on_time1_init()功能
*
*	说明:定时器时钟为Fsys  Fsys=12M  time1用作uart波特率发生器 fn2:定时器时钟频率 BaudRate = 2/16 * fn2/(256 - TH2) * 2
* 		 TH2=256-fn2/(16*Baudrate)   Baudrate=19200
*	参数：
*
*	输出：无
*
**************************************************************/
void uart_on_time1_init()
{
   PCON |=0X80; 				//方式1
   T2CON &= ~(RCLK | TCLK);  	//使用定时器1作UART时钟
   
   TMOD &= ~CT1;
   TMOD |= M11;					//定时器1  8位自动重载
   TMOD &= ~M01;

   TMCON |= T1FD; 				//定时器1 = Fsys

	//随系统频率更新作为波特率定时器1 的定时时间
   uart_baudrate_adjust();

//   TL1=217;		
//   TH1=217;					//Fsys=24M UART 波特率38400；
   TR1=0;
//   ET1=1;
   TR1=1;
}


/*************************************************************
*			  uart初始化函数
*
*	功能:初始化uart功能
*
*	说明:定时器时钟为Fsys  Fsys=12M  time1 || time2用作uart波特率发生器 baudrate=19200
* 		 
*	参数：
*
*	输出：无
*
**************************************************************/
void uart_init()
{
	P1CON |= (1<<3);									//P1.3设置为输出
	P1PH  &= ~(1<<3);
	P1CON &= ~(1<<2);									//P1.2设置为输入
	P1PH  |= (1<<2);
	P13   = 1;
	P12   = 1;
	
	uart_b.index_rb = 0;
	uart_b.index_sb = 0;
	uart_b.output_flag = 0x42;
//	uart_b.output_flag = (1<<10) |(1<<1);
	uart_b.begin = 0;
	uart_b.end = 7;

   SCON |= SM01 | REN;									//串行通信模式 模式1 10位全双工异步通信,允许接收
   uart_on_time1_init();								//串口波特率发生器使用定时器1
   EUART=1;
}
#endif

/*************************************************************
*			  uart_on_time2_init()初始化函数
*
*	功能:初始化uart_on_time2_init()功能
*
*	说明:定时器时钟为Fsys  Fsys=24M  time2用作uart波特率发生器 fn2:定时器时钟频率 BaudRate = 2/16 * fn2/(65536 - [RCAP2H,RCAP2L]) * 2
* 		 Baudrate=38400
*	参数：
*
*	输出：无
*
**************************************************************/
//void uart_on_time2_init()
//{
//    SCON=0X50;         //方式1，允许接收数据
//    PCON |=0X80;       //方式1
//	T2CON=0x35;	       //定时器2产生波特率
//	T2MOD=0x03;        //16位自动重载定时器模式
//	TMCON=0X04; 
//	RCAP2H=(65536-20)/256;			
//	RCAP2L=(65536-20)%256;
//	TR2=0;
////	ET2=1;
//   	TR2=1;
//}




/*************************************************************
*			  time2初始化函数(用做任务调度器节拍)
*
*	功能:初始化time2功能
*
*	说明:定时器时钟为Fsys/12  Fsys=12M  溢出时间 1/24M/12*10000 = 10ms
* 		 用作任务调度器时钟节拍
*	参数：
*
*	输出：无
*
**************************************************************/
void time2_init()
{
	T2CON &= ~CT2;										  //time2 设定为定时功能
	T2CON &= ~CPRL2;									  //16位带重载功能的定时器

	T2MOD &= ~T2OE;
	T2MOD &= ~DCEN;

	TMCON &= ~T2FD;										  //T2 频率=Fsys/12

	RCAP2L = (65536-10000)%256;
	RCAP2H = (65536-10000)/256;

	TR2 = 0;
	ET2 = 1;
	TR2 = 1;
}


/*************************************************************
*			  time1初始化函数
*
*	功能:初始化time1功能
*
*	说明:定时器时钟为Fsys/12  Fsys=24M  溢出时间 1/24M/12*200 = 100us
* 
*	参数：
*
*	输出：无
*
**************************************************************/
void time0_init()
{
	
	TMOD &= ~CT0;										  //time0 设定为定时功能
	TMOD |= TIME0_16BIT;								  //time0 设定为16位定时器

	TMCON &= ~T0FD;										  //T0 时钟频率=Fsys/12

	TH0 =0;
	TL0 =0;

//	ET0 = ON;
//	TR0 = ON;
}

/*************************************************************
*			  ADC初始化函数
*
*	功能:初始化ADC功能
*
*	参数：
*		  vref		:	参考电压选择(vdd:参考电压为vdd; v2p4:参考电压为v2p4)
*		  fosc		:	ADC时钟频率(取值范围:0--Fosc; 2--Fosc/2; 4--Fosc/4; 6--Fosc/6;)
*
*	输出：无
*
**************************************************************/
void adc_init(void)
{
	unsigned char i;

	P2CON &= ~P2C1;			 //P2.1为ADC输入
	P2PH  &= ~P2H1;			 //P2.1上拉电阻关闭

	ADCCON =0X81;  			//选择AIN1作为输入通道，2M采样频率,打开ADC电源

	ADCCFG0=0X02;  			//选择AIN1为ADC输入；
	ADCCFG1=0X00;
	
	OPINX =0xc2;
//	OPREG = 0;				//ADC参考电压为VDD	
	OPREG |= VREFS;			//ADC参考电压为内部2.4V	

	/*清ADC中断标识 EOC/ADCIF*/
	ADCCON &= ~ADCIF;
	/*开启ADC中断*/
	EADC = 1;

	adc.index=0;
	adc.cnt=0;
	adc.ave=0;

	//工作电流由如下ADC值限定在680mA左右
//	adc.adc_level_up = 20;
//	adc.adc_level_down = 18;
	//工作电流由如下ADC值限定在630mA左右
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
	/*清除ADC中断标识(ADCIF) 启动ADC转换(ADCS)*/
	ADCCON &= ~ADCIF;
	ADCCON |= ADCS;
}

/*************************************************************
*			  PWM初始化函数
*
*	功能:初始化PWM功能
*
*	参数：Fsys=12M
*	输出：无
*
**************************************************************/
void pwm1_init(void)
{
   	/*设置pwm1时钟频率为Fosc PWMCKS[2:0] 000:Fosc;*/
	PWMCON 	&= ~PWMCKS;	

	//PWM1输出脚选择 P2.6
	PWMCFG	|= PWMOS1;

   	/*设置pwm1高电位时钟数(宽度) PWMDTY0[7:0] 宽度=PWMDTY0xClk.pwm*/
	PWMDTY1 = 3;
 
   	/*设置pwm1周期为5 PWMPRD[7:0] 周期=(PWMPRD + 1) x Clkpwm */
	PWMPRD	= 6;											
}


void pwm1_start(void)
{
	/*pwm1开启*/
	PWMCON	|= ENPWM | ENPWM1;							//开启pwm工作时钟 pwm1输出到IO
	machine.pwm1_stat=PWM1_RUN;
}

void pwm1_stop(void)
{
	/*pwm1关闭输出*/
	PWMCON	&= ~(ENPWM | ENPWM1);						//停止pwm工作时钟 pwm1输出到IO
	machine.pwm1_stat=PWM1_STOP;
}



/*************************************************************
*			  硬件初始化函数
*
*	功能:
*			1.调节时钟频率为高频振荡频率的2分频
*			2.设置主频至12M + n x 23.4kHz
*			3.设置P2.6 P2.0为推挽输出
*	参数：
*		  	OPINX: 主频寄存器指针
*		  	OPREG: 主频值寄存器
*			HRCR : 初始值 HRCR=134
*
*	输出：
*			HRCR+=5	:	1.731M	MOS管微烫	630mA	  雾化量***
*
**************************************************************/
void hardware_init(void)
{
	P2CON	|=	P2C6;							//P2.6推挽输出
	P2CON	|=	P2C7;							//P2.7推挽输出

	EA	= ON;
}

unsigned char adc_val_handler(void *p)
{
	unsigned int bf=0,tm=0;
	unsigned char i;

	p=p;

	if((ADCCON & ADCIF)) return 1;				//没有转换完成退出

	//计算ADC平均值
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

										
	/*重新启动ADC转换(ADCS)*/
	ADCCON |= ADCS;						//重新启动ADC 转换
	
	return 0;						
}


/*************************************************************
*			  频率追踪
*
*	参数：
*		  	OPINX: 主频寄存器指针
*		  	OPREG: 主频值寄存器
*			HRCR : 初始值 HRCR=134
*			0x83 : 时钟频率调节器地址
*	输出：
*
*	说明：
*			adc.ave与adc.adc_level_down 或adc.adc_level_up比较 调整时钟频率寄存器的值，
*			进而调节时钟频率，间接调节PWM输出频率的目的。
*
**************************************************************/
unsigned char freq_trace(void *p)
{
//	unsigned char i;

	p=p;

	//采集ADC值不在范围，pwm1停止工作时,停止频率跟踪
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

		//调节时钟频率 
		machine.hrcr--;
		OPINX =0x83;
		OPREG = machine.hrcr;

		#ifdef _UART_
		//随系统频率更新作为波特率定时器1 的定时时间
		uart_baudrate_adjust();
		#endif
	}
	else if(adc.ave >=  adc.adc_level_up)
	{
		if(++machine.freq_trace_cnt_threshold[0] <=machine.freq_trace_cnt_threshold[1]) return 1;
		machine.freq_trace_cnt_threshold[0]=0;

		if(adc.adcval[adc.index] < adc.ave) return 1;

		//调节时钟频率 
		machine.hrcr++;
		OPINX =0x83;
		OPREG = machine.hrcr;

		#ifdef _UART_
		//随系统频率更新作为波特率定时器1 的定时时间
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
*			  水位侦测
*
*	参数：
*
*	输出：
*
*	说明：
*
*			
*
**************************************************************/
unsigned char no_water_detect(void *p)
{
	p = p;

	//无水干烧检测，采集的当前数据大于ADC_NO_WATER_THRESHOLD 无水阀值，停止PWM1输出。
	if(adc.adcval[adc.index] >= ADC_NO_WATER_THRESHOLD)	
	{
		pwm1_stop();
		machine.water_stat = NO_WATER;
	}

	return 0;
}

void water_detect_init()
{
	P2CON	|=	P2C0;							//P2.0推挽输出		
	P20=OFF;
//	P27	= OFF;
}

void water_detect_start()
{
	P2CON	&=	~P2C0;							//P2.0输入	
	INT2R =(1<<4);								//INT2F4上升沿中断开启
	IE1	|=(1<<3);								//打开EINT2中断
	int24_flag_bit=OFF;
//	P27	= ON;

	TH0 =0;
	TL0 =0;
	TR0 = ON;									//开启time0定时器计时
}

void water_detect()
{
	unsigned int water_level_time=0;						//水位检测时钟计数

	water_level_time = TH0;
	water_level_time = (water_level_time<<8);
	water_level_time += TL0;								

	if(water_level_time < WATER_FULL_UP && water_level_time > WATER_FULL_DOWN)
	{
		pwm1_start();
		task[4].period =10;						//pwm1启动后缩短低水位探测反应时间
		machine.water_stat = FULL_WATER;
	}
	else if(water_level_time < WATER_LOW_UP && water_level_time > WATER_LOW_DOWN)
	{
		pwm1_stop();
		task[4].period =100;					//pwm1停止后延长低水位探测恢复时间
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

	//低水位检测 LOW_WATER_DETECT
	switch(machine.water_cnt)
	{
	 case 0:
			water_detect_init();
//			P2CON	|=	P2C0;							//P2.0推挽输出		
//			P20=OFF;
//			P27	= OFF;
			machine.water_cnt =1;
			break;

	 case 1:
			water_detect_start();
//			P20=OFF;
//			P2CON	&=	~P2C0;							//P2.0输入	
//			INT2R =(1<<4);								//INT2F4上升沿中断开启
//			IE1	|=(1<<3);								//打开EINT2中断
//			int24_flag_bit=OFF;
//			P27	= ON;
//
//			TH0 =0;
//			TL0 =0;
//			TR0 = ON;									//开启time0定时器计时
			machine.water_cnt =2;
			break;

	 case 2:		
			//正常获取传感器数据
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

	//调节时钟频率 Fsys=Fcyc/2
	OPINX =0xc1;
	OPREG |= SCLKS0;
	OPREG &= ~SCLKS1;

	//调高时钟频率 12000 + 2 x 23.4 kHz =12046800 kHz
	OPINX =0x83;
	machine.hrcr_origin=OPREG;
	OPREG += 14;
	machine.hrcr=OPREG;

	machine.hrcr_max = 160;
	machine.hrcr_min = 141;
	//初始化调整hrcr 的阀值

	machine.freq_trace_cnt_threshold[1]=3;

	//低水位检测 LOW_WATER_DETECT
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
// 函数功能: 主函数
// 输入: 无
// 输出: 
// 备注: 
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

	task_add(0,uart_send,100,100,READY);

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