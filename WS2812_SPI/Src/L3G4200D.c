

#include "L3G4200D.h"
#include "stdbool.h"

#define   uchar unsigned char
#define   uint unsigned int	

#define   u8 unsigned char
#define   u16 unsigned int	
	


#define	L3G4200_Addr   0xD2	 //定义器件在IIC总线中的从地址,根据ALT  ADDRESS地址引脚不同修改
//**********L3G4200D内部寄存器地址*********
#define WHO_AM_I 0x0F
#define CTRL_REG1 0x20
#define CTRL_REG2 0x21
#define CTRL_REG3 0x22
#define CTRL_REG4 0x23
#define CTRL_REG5 0x24
#define REFERENCE 0x25
#define OUT_TEMP 0x26
#define STATUS_REG 0x27
#define OUT_X_L 0x28
#define OUT_X_H 0x29
#define OUT_Y_L 0x2A
#define OUT_Y_H 0x2B
#define OUT_Z_L 0x2C
#define OUT_Z_H 0x2D
#define FIFO_CTRL_REG 0x2E
#define FIFO_SRC_REG 0x2F
#define INT1_CFG 0x30
#define INT1_SRC 0x31
#define INT1_TSH_XH 0x32
#define INT1_TSH_XL 0x33
#define INT1_TSH_YH 0x34
#define INT1_TSH_YL 0x35
#define INT1_TSH_ZH 0x36
#define INT1_TSH_ZL 0x37
#define INT1_DURATION 0x38

unsigned char TX_DATA[4];  
unsigned char BUF[8];                         //接收数据缓存区
char  test=0; 
short T_X,T_Y,T_Z;

//************************************

#define Pin_SDA		GPIO_PIN_7
#define GPIO_SDA	GPIOB

#define Pin_SCL		GPIO_PIN_13
#define GPIO_SCL	GPIOC

/*模拟IIC端口输出输入定义*/
#define SCL_H         GPIOC->BSRR = GPIO_PIN_13
#define SCL_L         GPIOC->BRR  = GPIO_PIN_13 
   
#define SDA_H         GPIOB->BSRR = GPIO_PIN_7
#define SDA_L         GPIOB->BRR  = GPIO_PIN_7

#define SCL_read      GPIOC->IDR  & GPIO_PIN_13
#define SDA_read      GPIOB->IDR  & GPIO_PIN_7


void I2C_delay(void)
{
		
   u8 i=30; //这里可以优化速度	，经测试最低到5还能写入
   while(i) 
   { 
     i--; 
   }  
}

static GPIO_InitTypeDef  GPIO_InitStruct;
void I2C_SDA_Dir (unsigned char dir)
{
	if ( dir == 0 ) 
	{
		GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull  = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

		GPIO_InitStruct.Pin = Pin_SDA;
		HAL_GPIO_Init(GPIO_SDA, &GPIO_InitStruct);
	}
	else 
	{
		GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

		GPIO_InitStruct.Pin = Pin_SDA;
		HAL_GPIO_Init(GPIO_SDA, &GPIO_InitStruct);
	}      
}

/*******************************************************************************
* Function Name  : I2C_Start
* Description    : Master Start Simulation IIC Communication
* Input          : None
* Output         : None
* Return         : Wheather	 Start
****************************************************************************** */
bool I2C_Start(void)
{
	I2C_SDA_Dir(0);	
	
	SDA_H;
	SCL_H;
	I2C_delay();
	I2C_SDA_Dir(1);
	if(!SDA_read)return false;	//SDA线为低电平则总线忙,退出
	SDA_L;
	I2C_delay();
	I2C_SDA_Dir(1);
	if(SDA_read) return false;	//SDA线为高电平则总线出错,退出
	SDA_L;
	I2C_delay();
	return true;
}
/*******************************************************************************
* Function Name  : I2C_Stop
* Description    : Master Stop Simulation IIC Communication
* Input          : None
* Output         : None
* Return         : None
****************************************************************************** */
void I2C_Stop(void)
{
	I2C_SDA_Dir(0);	
	
	SCL_L;
	I2C_delay();
	SDA_L;
	I2C_delay();
	SCL_H;
	I2C_delay();
	SDA_H;
	I2C_delay();
} 
/*******************************************************************************
* Function Name  : I2C_Ack
* Description    : Master Send Acknowledge Single
* Input          : None
* Output         : None
* Return         : None
****************************************************************************** */
void I2C_Ack(void)
{	
	I2C_SDA_Dir(0);	
	
	SCL_L;
	I2C_delay();
	SDA_L;
	I2C_delay();
	SCL_H;
	I2C_delay();
	SCL_L;
	I2C_delay();
}   
/*******************************************************************************
* Function Name  : I2C_NoAck
* Description    : Master Send No Acknowledge Single
* Input          : None
* Output         : None
* Return         : None
****************************************************************************** */
void I2C_NoAck(void)
{	
	I2C_SDA_Dir(0);	
	
	SCL_L;
	I2C_delay();
	SDA_H;
	I2C_delay();
	SCL_H;
	I2C_delay();
	SCL_L;
	I2C_delay();
} 
/*******************************************************************************
* Function Name  : I2C_WaitAck
* Description    : Master Reserive Slave Acknowledge Single
* Input          : None
* Output         : None
* Return         : Wheather	 Reserive Slave Acknowledge Single
****************************************************************************** */
bool I2C_WaitAck(void) 	 //返回为:=1有ACK,=0无ACK
{
	I2C_SDA_Dir(0);	
	
	SCL_L;
	I2C_delay();
	SDA_H;			
	I2C_delay();
	SCL_H;
	I2C_delay();
	I2C_SDA_Dir(1);
	if(SDA_read)
	{
      SCL_L;
	  I2C_delay();
      return false;
	}
	SCL_L;
	I2C_delay();
	return true;
}
/*******************************************************************************
* Function Name  : I2C_SendByte
* Description    : Master Send a Byte to Slave
* Input          : Will Send Date
* Output         : None
* Return         : None
****************************************************************************** */
void I2C_SendByte(u8 SendByte) //数据从高位到低位//
{
    u8 i=8;
	I2C_SDA_Dir(0);	
    while(i--)
    {
        SCL_L;
        I2C_delay();
      if(SendByte&0x80)
        SDA_H;  
      else 
        SDA_L;   
        SendByte<<=1;
        I2C_delay();
		SCL_H;
        I2C_delay();
    }
    SCL_L;
}  
/*******************************************************************************
* Function Name  : I2C_RadeByte
* Description    : Master Reserive a Byte From Slave
* Input          : None
* Output         : None
* Return         : Date From Slave 
****************************************************************************** */
unsigned char I2C_RadeByte(void)  //数据从高位到低位//
{ 
    u8 i=8;
    u8 ReceiveByte=0;

	I2C_SDA_Dir(0);	
    SDA_H;				
    while(i--)
    {
      ReceiveByte<<=1;      
      SCL_L;
      I2C_delay();
	  SCL_H;
      I2C_delay();
	I2C_SDA_Dir(1);		
      if(SDA_read)
      {
        ReceiveByte|=0x01;
      }
    }
    SCL_L;
    return ReceiveByte;
} 
//ZRX          
//单字节写入*******************************************
bool Single_Write(unsigned char SlaveAddress,unsigned char REG_Address,unsigned char REG_data)		     //void
{
  	if(!I2C_Start())return false;
    I2C_SendByte(SlaveAddress);   //发送设备地址+写信号//I2C_SendByte(((REG_Address & 0x0700) >>7) | SlaveAddress & 0xFFFE);//设置高起始地址+器件地址 
    if(!I2C_WaitAck()){I2C_Stop(); return false;}
    I2C_SendByte(REG_Address );   //设置低起始地址      
    I2C_WaitAck();	
    I2C_SendByte(REG_data);
    I2C_WaitAck();   
    I2C_Stop(); 
    HAL_Delay(5);
    return true;
}

//单字节读取*****************************************
unsigned char Single_Read(unsigned char SlaveAddress,unsigned char REG_Address)
{   unsigned char REG_data;     	
	if(!I2C_Start())return false;
    I2C_SendByte(SlaveAddress); //I2C_SendByte(((REG_Address & 0x0700) >>7) | REG_Address & 0xFFFE);//设置高起始地址+器件地址 
    if(!I2C_WaitAck()){I2C_Stop();test=1; return false;}
    I2C_SendByte((u8) REG_Address);   //设置低起始地址      
    I2C_WaitAck();
    I2C_Start();
    I2C_SendByte(SlaveAddress+1);
    I2C_WaitAck();

	REG_data= I2C_RadeByte();
    I2C_NoAck();
    I2C_Stop();
    //return TRUE;
	return REG_data;

}						      


 //************初始化L3G4200D*********************************
void Init_L3G4200D(void)
{
	Single_Write(L3G4200_Addr,CTRL_REG1, 0x0f);
	Single_Write(L3G4200_Addr,CTRL_REG2, 0x00);
	Single_Write(L3G4200_Addr,CTRL_REG3, 0x08);
	Single_Write(L3G4200_Addr,CTRL_REG4, 0x30);	//+-2000dps
	Single_Write(L3G4200_Addr,CTRL_REG5, 0x00);
}	

//******读取L3G4200D数据****************************************
void READ_L3G4200D(void)
{
   BUF[0]=Single_Read(L3G4200_Addr,OUT_X_L);
   BUF[1]=Single_Read(L3G4200_Addr,OUT_X_H);
   T_X=	(BUF[1]<<8)|BUF[0];

   BUF[2]=Single_Read(L3G4200_Addr,OUT_Y_L);
   BUF[3]=Single_Read(L3G4200_Addr,OUT_Y_H);
   T_Y=	(BUF[3]<<8)|BUF[2];

   BUF[4]=Single_Read(L3G4200_Addr,OUT_Z_L);
   BUF[5]=Single_Read(L3G4200_Addr,OUT_Z_H);
   T_Z=	(BUF[5]<<8)|BUF[4];

}

