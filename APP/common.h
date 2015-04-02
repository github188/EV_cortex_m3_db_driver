/****************************************Copyright (c)*************************************************
**                      Fujian Junpeng Communicaiton Technology Co.,Ltd.
**                               http://www.easivend.com.cn
**--------------File Info------------------------------------------------------------------------------
** File name:           common.h
** Last modified Date:  2013-01-06
** Last Version:        No
** Descriptions:        ϵͳ�������弰ͨ�ú�������                     
**------------------------------------------------------------------------------------------------------
** Created by:          sunway
** Created date:        2013-01-06
** Version:             V0.1
** Descriptions:        The original version        
********************************************************************************************************/
#ifndef __COMMON_H 
#define __COMMON_H


#include "..\API\billApi.h"
#include "..\API\coinApi.h"

#define print_log(...)	




//==============================��ʱ�����������ṹ��========================================================

typedef struct _timer_st_{

	volatile unsigned char secTimer;//�����뼶 ��ʱ

//==============================10���뼶��ʱ������=====================================================================
	volatile unsigned short printTimer;
	volatile unsigned short pcm_handle_timeout;
	volatile unsigned short checkDeviceTimeout;
	volatile unsigned short HpHandleTimer;
	volatile unsigned int bill_comunication_timeout;//MDBͨѶ��ʱʱ��
	volatile unsigned short led_paoma_timer;
	volatile unsigned char  user_led_green;

//==============================�뼶��ʱ������=====================================================================
	volatile unsigned short sec_usr_op_timer;//�û�������ʱ	
	volatile unsigned short sec_changer_timer;
	volatile unsigned short sec_hopper_state_timer;//hopper  ��ʱ��λ
	volatile unsigned short sec_pccon_timer;//����PC���ʱ
	volatile unsigned char	PayoutTimer;//���ҵȴ���ʱ��


}TIMER_ST;
extern TIMER_ST Timer;


/************************************************************************************
*ϵͳ�����ṹ��
***************************************************************************************/
typedef struct _system_para_{

	//Ӳ��������: ����λ��ʾӲ�������� 0-�� 1-hopper 2-MDB
	//			  ����λ��ʾӲ�ҽ����� 0��  1-����   2-MDB  3-����
	unsigned char coinType;

	//ֽ��������: ����λ��ʾֽ�������� 0-�� 1-Ԥ�� 2-MDB
	//			  ����λ��ʾֽ�ҽ����� 0��  1-Ԥ�� 2-MDB  
	unsigned char billType; 
	unsigned char traceFlag;//���Կ���
			


}SYSTEM_PARA;
extern SYSTEM_PARA sysPara;



/************************************************************************************
*�豸 �ṹ��
***************************************************************************************/
typedef struct _st_dev_{
	ST_DEV_BILL bill;//ֽ�����ṹ��
	ST_DEV_COIN coin;//Ӳ�����ṹ��
	uint32_t hpBaseChange;//hopper �ṹ��
	uint16_t HpValue[3];

}ST_DEV;

extern ST_DEV stDev;


/************************************************************************************
*�ܽ������ݽṹ��
***************************************************************************************/
typedef struct _st_total_trade_sum_
{	
	unsigned int BillMoneySum;//����ֽ���ܽ��	
	unsigned int CoinMoneySum;//����Ӳ���ܽ��	
	unsigned int DispenceSum;//�����ܽ��	
	unsigned int Hopper1DispSum;//00��ַHopper����������	
	unsigned int Hopper2DispSum;//01��ַHopper����������	
	unsigned int Hopper3DispSum;//10��ַHopper����������
	unsigned int tradePageNo;//����ҳ�� 0 ��ʾû�н��׼�¼ 
	unsigned int tradeNums;//�ܽ�������
	unsigned int iouAmount;//Ƿ�ѽ��
	unsigned int tradeAmount;//�����ܽ��


}ST_TOTAL_TRADE_SUM;

extern ST_TOTAL_TRADE_SUM stTotalTrade;


/************************************************************************************
*����ṹ��
***************************************************************************************/
//ֽ��������ͨѶ�ṹ��
typedef struct _task_msg_bill_{

	 unsigned char type;
	 unsigned char status;
	 unsigned int  amount;
	 unsigned int  change;

}TASK_MSG_BILL;

//Ӳ��������ͨѶ�ṹ��
typedef struct _task_msg_coin_{

	 unsigned char type;
	 unsigned char status;
	 unsigned int  amount;
	 unsigned int  change;


}TASK_MSG_COIN;


//hopper ����ͨѶ�ṹ��
typedef struct _task_msg_hopper_{

   	 unsigned char value8;
	 unsigned short value16;
	 unsigned int  value32;

}TASK_MSG_HOPPER;
//�������������豸����֮��ͨ�ŵĽṹ��
typedef struct
{
    unsigned char   type;				//��������
	unsigned char   subType;		   //�Ӽ���������
	TASK_MSG_BILL 	bill;
	TASK_MSG_COIN 	coin;
	TASK_MSG_HOPPER hopper;

	
} MAIN_DEV_TASK_MSG;




//����ͨ������ĶԷ�������
extern OS_EVENT *g_msg_main_to_dev;
extern OS_EVENT *g_msg_dev_to_main;
extern MAIN_DEV_TASK_MSG task_msg_main_to_dev;
extern MAIN_DEV_TASK_MSG task_msg_dev_to_main;


//ֽ��������
extern OS_EVENT *g_billIN;
typedef struct _bill_recv_msg_{
	unsigned char channel;
	unsigned int  value;
	
}BILL_RECV_MSG;
#define G_BILL_IN_SIZE    20  
extern BILL_RECV_MSG bill_recv_msg[G_BILL_IN_SIZE];


//Ӳ����ͨ����Ϣ����
#define G_COIN_IN_SIZE    100
extern OS_EVENT *g_CoinIn;
extern unsigned char  CoinIn[G_COIN_IN_SIZE];


//��ʼ�������ź���
extern OS_EVENT *g_InitSem;


/*---------------------------------------------------------
�����ͨ���豸������ܵĲ���ָ���
-----------------------------------------------------------*/
#define TASK_DISABLE           	1 //�������ͽ��� ����

#define TASK_ENABLE            	3 //��������ʹ�� ����
 	 
#define TASK_HOPPER         	5 //��������hopper ����

#define TASK_INIT		    	7//�������ͳ�ʼ������

#define TASK_CHANGER			9 //��������

#define TASK_OVER				0xAA
#define TASK_NOT_ACK			0x0A
//�������
#define OBJ_BILL					0x0001
#define OBJ_COIN					0x0002
#define OBJ_HOPPER					0x0004

#define OBJ_ALL						0xFFFF



//================================ϵͳ������======================================================

#define SYS_ERR_NO_NORMAL  			(unsigned short)(0)  //����
#define SYS_ERR_NO_HOPPER  			(unsigned short)(0x01 << 0)  //Hopper����
#define SYS_ERR_NO_BILL    			(unsigned short)(0x01 << 1)  //ֽ��������
#define SYS_ERR_NO_HUODAO_PRICE   	(unsigned short)(0x01 << 2)  //�����Լ�ȫ����������Ϊ��
#define SYS_ERR_NO_HUODAO_EMPTY   	(unsigned short)(0x01 << 3)  //����ϵͳ�����еĴ�����ȫΪ0
#define SYS_ERR_NO_HUODAO_FAULT  	(unsigned short)(0x01 << 4)  //�����Լ�ȫ������������
#define SYS_ERR_NO_COIN				(unsigned short)(0x01 << 5)  //Ӳ��������
#define SYS_ERR_NO_CHANGER_FAULT 	(unsigned short)(0x01 << 6)  //��hopper����������

//================================hopper������======================================================

#define HP_ERR_NO_NORMAL  		 ((unsigned short)(0))//����
#define HP_ERR_NO_HOPPER1_FAULT  (unsigned short)(0x01 << 0)//hopper1����
#define HP_ERR_NO_HOPPER1_EMPTY  (unsigned short)(0x01 << 1)//hopper1ȱ��
#define HP_ERR_NO_HOPPER2_FAULT  (unsigned short)(0x01 << 2)//hopper2����
#define HP_ERR_NO_HOPPER2_EMPTY  (unsigned short)(0x01 << 3)//hopper2ȱ��
#define HP_ERR_NO_HOPPER3_FAULT  (unsigned short)(0x01 << 4)//hopper3����
#define HP_ERR_NO_HOPPER3_EMPTY  (unsigned short)(0x01 << 5)//hopper3ȱ��
#define HP_ERR_NO_HOPPER4_FAULT  (unsigned short)(0x01 << 6)//hopper4����
#define HP_ERR_NO_HOPPER4_EMPTY  (unsigned short)(0x01 << 7)//hopper4ȱ��



//hopper������
extern unsigned int hopperErrNo;
//�ۻ���������
extern uint16_t HardWareErr;


//======================================================================================
unsigned char XorCheck(unsigned char *pstr,unsigned short len);
unsigned short CrcCheck(unsigned char *msg, unsigned short len);
void msleep(unsigned int msec);
unsigned char mbox_post_main_to_dev(unsigned char type);
unsigned char mbox_post_dev_to_main(unsigned char type);
void CreateCommonMBox(void);



#endif
/**************************************End Of File*******************************************************/
