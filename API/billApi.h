#ifndef _BILL_API_H
#define _BILL_API_H

#define STATUS_BILL_N_A				0 //δ֪
#define STATUS_BILL_ENABLE			1 //����
#define STATUS_BILL_FAULT			2 //����
#define STATUS_BILL_QURBI			3 //ȱ��
#define STATUS_BILL_DISABLE			4 //����
#define STATUS_BILL_COM				5 //ͨ�Ź���		

#define CTL_BILL_INIT				1//�·�ֽ������ʼ��
#define CTL_BILL_ENABLE				2//�·�ֽ����ʹ��
#define CTL_BILL_DISABLE			3//�·�ֽ��������	

#define STATUS_BILL_ERR_COM				0x0001 //ͨ�Ź���
#define STATUS_BILL_ERR_SENSOR			0x0002 //����������
#define STATUS_BILL_ERR_TUBEJAM			0x0004 //���ҿڿ���
#define STATUS_BILL_ERR_ROM				0x0008 //rom����
#define STATUS_BILL_ERR_ROUTING			0x0010 //����ͨ������
#define STATUS_BILL_ERR_JAM				0x0020 //Ͷ�ҿ���
#define STATUS_BILL_ERR_REMOVECASH		0x0040 //�Ƴ�ֽ�ҳ���
#define STATUS_BILL_ERR_DISABLE			0x0080 //����
#define STATUS_BILL_ERR_MOTO			0x0100 //������
#define STATUS_BILL_ERR_CASH			0x0200 //ֽ�ҳ������
#define STATUS_BILL_ERR_UNKNOW			0x8000 //�������ֹ���


#define SET_BILL_ERR(state,errno)  (state |= errno)
#define CLR_BILL_ERR(state)  (state = 0)



typedef struct	_st_dev_bill_{
	unsigned char  pcDisabled;//�ϴ��·�ֽ��������  0-enable -disable
	unsigned short  curErrNo;//��ǰʵʱ������
	unsigned char  comerr;//ͨ�Ź���
	unsigned short errNo; //������
	unsigned char  state;//0 δ֪   1���� 2 ���� 3ȱ�� 4����
	unsigned char  level; //ֽ�����ȼ�
	unsigned char  escrowFun;		 //�ݴ湦��
	unsigned short code;		  //���Ҵ���
	unsigned short scale;			  //��������
	unsigned short decimal;		  //10^С��λ��
	unsigned short stkCapacity;	  //��������
	unsigned short security;	//��ȫ�ȼ�
	unsigned int   channel[8];	 //ͨ����ֵ
	unsigned char  IDENTITYBuf[36];  //Identification


}ST_DEV_BILL;


unsigned char billInit(void);
void billDisable(void);
void billEnable(void);
unsigned int billRecv(void);
void billReject(void);
unsigned char billPoll(void);
void billStatusUpdate(void);
unsigned char billTaskPoll(void);

#endif
