#ifndef _COIN_API_H
#define _COIN_API_H


#define STATUS_COIN_N_A					0 //δ֪
#define STATUS_COIN_ENABLE				1 //����
#define STATUS_COIN_FAULT				2 //����
#define STATUS_COIN_QURBI				3 //ȱ��
#define STATUS_COIN_DISABLE				4 //�������
#define STATUS_COIN_COM					5 //ͨ�Ź���
#define CTL_COIN_INIT				1//�·�Ӳ������ʼ��
#define CTL_COIN_ENABLE				2//�·�Ӳ����ʹ��
#define CTL_COIN_DISABLE			3//�·�Ӳ��������
#define CTL_COIN_PAYOUT				4//�·�Ӳ��������

#define STATUS_COIN_ERR_COM				0x0001 //ͨ�Ź���
#define STATUS_COIN_ERR_SENSOR			0x0002 //����������
#define STATUS_COIN_ERR_TUBEJAM			0x0004 //���ҿڿ���
#define STATUS_COIN_ERR_ROM				0x0008 //rom����
#define STATUS_COIN_ERR_ROUTING			0x0010 //����ͨ������
#define STATUS_COIN_ERR_JAM				0x0020 //Ͷ�ҿ���
#define STATUS_COIN_ERR_REMOVETUBE		0x0040 //Ӳ�Ҷ��ƿ�
#define STATUS_COIN_ERR_DISABLE			0x0100 //�������
#define STATUS_COIN_ERR_UNKNOW			0x8000 //Ӳ���������ֹ���


#define SET_COIN_ERR(state,errno)  	do{state |= errno; }while(0)
#define CLR_COIN_ERR(state)  		(state = 0)
 



//Ӳ������ز���
typedef struct _st_dev_coin_{
	
	unsigned char  pcDisabled;//�ϴ��·���Ӳ���� 0-ʹ��  1-����
	unsigned short curErrNo;
	unsigned char  quebi;//ȱ�ҹ���
	unsigned char  comerr;//ͨ�Ź���
	unsigned char  state;//
	unsigned short errNo;				//������		
	unsigned char  level;				//Ӳ�����ȼ�
	unsigned char  scale;			 	//��������
	unsigned short code;			 	//���Ҵ���
	unsigned short decimal;		 		//10^С��λ��
	unsigned short routing;		 		//Bit is set to indicate a coin type can be routed to the tube
	unsigned short chNum[8];	     	//����ͨ���������
	unsigned char  payoutNum[8];	    //һ�������������ͨ���ұ�����
	unsigned char  identity[36];		//Identification
	unsigned int   channel[8];	  		//����ͨ����ֵ


}ST_DEV_COIN;


unsigned char coinPoll(void);
unsigned int coinRecv(void);
void coinGetNums(void);
void coinDisable(void);
void coinEnable(void);
unsigned char coinInit(void);
unsigned char coinChange(unsigned int payMoney,unsigned int *changeMoney);
unsigned char coinTaskPoll(void);


#endif
