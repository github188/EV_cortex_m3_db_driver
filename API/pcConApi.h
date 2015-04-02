
#ifndef _PCCONAPI_H_
#define _PCCONAPI_H_

//add by yoc
#define LED_DB    		(1ul << 26)//P3.26ת�Ӱ� �û�LED��	
#define SET_ULED_ON()	do{FIO3DIR |= LED_DB; FIO3CLR |= LED_DB;}while(0)
#define SET_ULED_OFF()	do{FIO3DIR |= LED_DB; FIO3SET |= LED_DB;}while(0)

#define PCD_HEAD  0xE5
#define DPC_HEAD  0xE6

#define print_err(...)



//ֽӲ����������״̬�ṹ��
typedef struct _bcdb_st_{

	unsigned char status;//����״̬0���� 1æ
	unsigned char billType;//ֽ��������2 MDB
	unsigned char coinType;//Ӳ��������2 MDB
	unsigned char billStatus;//ֽ����״̬0 ������ 1���� 2���� 3���� 
	unsigned char coinStatus;//Ӳ����״̬0������ 1���� 2���� 3���� 
	unsigned int  billAmount;//ֽ������ǰ�ձҽ��
	unsigned int  coinAmount;//Ӳ������ǰ�ձҽ��
	unsigned int  billChange;//ֽ�����ϴ�������
	unsigned int  coinChange;//Ӳ�����ϴ�������
	unsigned char payout;//�˱Ұ�ť��־

}BCDB_ST;






void pcCon(void);
void pcdCoinEscrow(void);




#endif
