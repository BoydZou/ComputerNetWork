#pragma once
#include <iostream>
//#include<string>
//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>       
#include <ws2tcpip.h>
#include<fstream>
#include<ctime>
using namespace std;
#define SOURCE_FILE "RESULT.txt"
#pragma comment(lib, "Ws2_32.lib")


double dur;
int iTTL;
bool flag = false;
bool fail = false;
//IP��ͷ
typedef struct IP_HEADER
{
	unsigned char Head_Len : 4;              //4λͷ������
	unsigned char Version : 4;               //4λ�汾��
	unsigned char Tos;                       //8λ��������
	unsigned short Total_Len;                //16λ�ܳ���
	unsigned short Identifier;               //16λ��ʶ��
	unsigned short Frag_and_Flag;            //3λ��־��13λƬƫ��
	unsigned char TTL;                       //8λ����ʱ��
	unsigned char Protocol;                  //8λ�ϲ�Э���
	unsigned short Check_Sum;                //16λУ���
	unsigned long Sour_IPAddress;            //32λԴIP��ַ
	unsigned long Dest_IPAddress;            //32λĿ��IP��ַ
} IP_HEADER;

//ICMP��ͷ
typedef struct ICMP_HEADER
{
	BYTE type;            //8λ�����ֶ�
	BYTE code;            //8λ�����ֶ�
	USHORT Check_Sum;     //16λУ���
	USHORT id;            //16λ��ʶ��
	USHORT seq;           //16λ���к�
} ICMP_HEADER;

//���Ľ���ṹ
typedef struct DECODE_RESULT
{
	USHORT usSeqNo;        //���к�
	DWORD dwRoundTripTime; //����ʱ��
	in_addr dwIPaddr;      //���ر��ĵ�IP��ַ
}DECODE_RESULT;

//��������У��ͺ���
USHORT checksum(USHORT* pBuf, int iSize)
{
	unsigned long cksum = 0;
	while (iSize > 1)
	{
		cksum += *pBuf++;
		iSize -= sizeof(USHORT);
	}
	if (iSize)//��� iSize Ϊ������Ϊ�������ֽ�
	{
		cksum += *(UCHAR*)pBuf; //����ĩβ����һ���ֽڣ�ʹ֮��ż�����ֽ�
	}
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (USHORT)(~cksum);
}

//�����ݰ����н���
BOOL DecodeIcmpResponse(char* pBuf, int iPacketSize, DECODE_RESULT& DecodeResult,
	BYTE ICMP_ECHO_REPLY, BYTE  ICMP_TIMEOUT, fstream &file)
{
	//������ݱ���С�ĺϷ���
	IP_HEADER* pIpHdr = (IP_HEADER*)pBuf;
	int iIpHdrLen = pIpHdr->Head_Len * 4;    //ip��ͷ�ĳ�������4�ֽ�Ϊ��λ��

	//�����ݰ���С С�� IP��ͷ + ICMP��ͷ�������ݱ���С���Ϸ�
	if (iPacketSize < (int)(iIpHdrLen + sizeof(ICMP_HEADER)))
		return FALSE;

	//����ICMP����������ȡID�ֶκ����к��ֶ�
	ICMP_HEADER* pIcmpHdr = (ICMP_HEADER*)(pBuf + iIpHdrLen);//ICMP��ͷ = ���յ��Ļ������� + IP��ͷ
	USHORT usID, usSquNo;

	if (pIcmpHdr->type == ICMP_ECHO_REPLY)    //ICMP����Ӧ����
	{
		usID = pIcmpHdr->id;        //����ID
		usSquNo = pIcmpHdr->seq;    //�������к�
	}
	else if (pIcmpHdr->type == ICMP_TIMEOUT)//ICMP��ʱ�����
	{
		char* pInnerIpHdr = pBuf + iIpHdrLen + sizeof(ICMP_HEADER); //�غ��е�IPͷ
		int iInnerIPHdrLen = ((IP_HEADER*)pInnerIpHdr)->Head_Len * 4; //�غ��е�IPͷ��
		ICMP_HEADER* pInnerIcmpHdr = (ICMP_HEADER*)(pInnerIpHdr + iInnerIPHdrLen);//�غ��е�ICMPͷ

		usID = pInnerIcmpHdr->id;        //����ID
		usSquNo = pInnerIcmpHdr->seq;    //���к�
	}
	else
	{
		return false;
	}

	//���ID�����к���ȷ���յ��ڴ����ݱ�
	if (usID != (USHORT)GetCurrentProcessId() || usSquNo != DecodeResult.usSeqNo)
	{
		return false;
	}
	//��¼IP��ַ����������ʱ��
	DecodeResult.dwIPaddr.s_addr = pIpHdr->Sour_IPAddress;
	DecodeResult.dwRoundTripTime = GetTickCount() - DecodeResult.dwRoundTripTime;

	//������ȷ�յ���ICMP���ݱ�
	if (pIcmpHdr->type == ICMP_ECHO_REPLY || pIcmpHdr->type == ICMP_TIMEOUT)
	{
		//�������ʱ����Ϣ
		if (DecodeResult.dwRoundTripTime)
			file << "      " << DecodeResult.dwRoundTripTime << "ms";
		else
			file << "      " << "<1ms";
	}
	return true;
}
void Start_TraceRoute(char* IpAddress)
{
	clock_t start, end;
	fail = false;
	fstream file;
	file.open(SOURCE_FILE, ios::out);
	//��ʼ��Windows sockets���绷��
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)   //��ʼ��ʧ��
	{
		fail = true;
		file << "Initial Error!";
		return ;
	}
	

	//cout << "������һ��IP��ַ��������";
	//cin >> IpAddress;
	
	//�õ�Ҫ��ѯ��IP��ַ
	unsigned long Dest_IPAddress = inet_addr(IpAddress);
	

	//ת�����ɹ�ʱ����������
	if (Dest_IPAddress == INADDR_NONE)
	{
		hostent* pHostent = gethostbyname(IpAddress);
		if (pHostent)
		{
			Dest_IPAddress = (*(in_addr*)pHostent->h_addr).s_addr;
		}
		else
		{
			fail = true;
			file << "�����IP��ַ��������Ч!" << endl;
			file.close();
			WSACleanup();
			return;
		}
	}

	start = clock();
	file << "׷�ٵ� " << IpAddress << "��·����Ϣ����(�����һ��ֱ��׷�ٽ���)��" << endl;

	//���Ŀ�Ķ�socket�׽��ֵ�ַ
	sockaddr_in destSockAddr;
	ZeroMemory(&destSockAddr, sizeof(sockaddr_in));
	destSockAddr.sin_family = AF_INET;
	destSockAddr.sin_addr.s_addr = Dest_IPAddress;

	//����ԭʼ�׽���
	SOCKET sockRaw = WSASocketW(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, WSA_FLAG_OVERLAPPED);

	//��ʱʱ��
	int iTimeout = 1000;

	//���ý��ճ�ʱʱ��
	setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char*)&iTimeout, sizeof(iTimeout));

	//���÷��ͳ�ʱʱ��
	setsockopt(sockRaw, SOL_SOCKET, SO_SNDTIMEO, (char*)&iTimeout, sizeof(iTimeout));

	//����ICMP����������Ϣ������TTL������˳���ͱ���
	//ICMP�����ֶ�
	const BYTE ICMP_ECHO_REQUEST = 8;    //�������
	const BYTE ICMP_ECHO_REPLY = 0;    //����Ӧ��
	const BYTE ICMP_TIMEOUT = 11;   //���䳬ʱ

	//������������
	const int DEF_ICMP_DATA_SIZE = 32;    //ICMP����Ĭ�������ֶγ���
	const int MAX_ICMP_PACKET_SIZE = 1024;  //ICMP������󳤶ȣ�������ͷ��
	const DWORD DEF_ICMP_TIMEOUT = 1000;  //����Ӧ��ʱʱ��
	const int DEF_MAX_HOP = 30;    //�����վ��

	//���ICMP������ÿ�η���ʱ������ֶ�
	char IcmpSendBuf[sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE];//���ͻ�����
	memset(IcmpSendBuf, 0, sizeof(IcmpSendBuf));               //��ʼ�����ͻ�����
	char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];                      //���ջ�����
	memset(IcmpRecvBuf, 0, sizeof(IcmpRecvBuf));               //��ʼ�����ջ�����

	ICMP_HEADER* pIcmpHeader = (ICMP_HEADER*)IcmpSendBuf;
	pIcmpHeader->type = ICMP_ECHO_REQUEST; //����Ϊ�������
	pIcmpHeader->code = 0;                //�����ֶ�Ϊ0
	pIcmpHeader->id = (USHORT)GetCurrentProcessId();    //ID�ֶ�Ϊ��ǰ���̺�
	memset(IcmpSendBuf + sizeof(ICMP_HEADER), 'E', DEF_ICMP_DATA_SIZE);//�����ֶ�

	USHORT usSeqNo = 0;            //ICMP�������к�

	iTTL = 1;            //TTL��ʼֵΪ1
	flag = false;
	

	BOOL bReachDestHost = FALSE;        //ѭ���˳���־
	int iMaxHot = DEF_MAX_HOP;  //ѭ����������
	DECODE_RESULT DecodeResult;    //���ݸ����Ľ��뺯���Ľṹ������
	
	while (!bReachDestHost && iMaxHot--)
	{
		//����IP��ͷ��TTL�ֶ�
		setsockopt(sockRaw, IPPROTO_IP, IP_TTL, (char*)&iTTL, sizeof(iTTL));
		file << iTTL;    //�����ǰ���

		//���ICMP������ÿ�η��ͱ仯���ֶ�
		((ICMP_HEADER*)IcmpSendBuf)->Check_Sum = 0;                   //У�������Ϊ0
		((ICMP_HEADER*)IcmpSendBuf)->seq = htons(usSeqNo++);    //������к�
		((ICMP_HEADER*)IcmpSendBuf)->Check_Sum =
			checksum((USHORT*)IcmpSendBuf, sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE); //����У���

		//��¼���кź͵�ǰʱ��
		DecodeResult.usSeqNo = ((ICMP_HEADER*)IcmpSendBuf)->seq;    //��ǰ���
		DecodeResult.dwRoundTripTime = GetTickCount();                          //��ǰʱ��

		//����TCP����������Ϣ
		sendto(sockRaw, IcmpSendBuf, sizeof(IcmpSendBuf), 0, (sockaddr*)&destSockAddr, sizeof(destSockAddr));

		//����ICMP����Ĳ����н�������
		sockaddr_in from;           //�Զ�socket��ַ
		int iFromLen = sizeof(from);//��ַ�ṹ��С
		int iReadDataLen;           //�������ݳ���
		while (1)
		{
			//��������
			iReadDataLen = recvfrom(sockRaw, IcmpRecvBuf, MAX_ICMP_PACKET_SIZE, 0, (sockaddr*)&from, &iFromLen);
			if (iReadDataLen != SOCKET_ERROR)//�����ݵ���
			{
				//�����ݰ����н���
				if (DecodeIcmpResponse(IcmpRecvBuf, iReadDataLen, DecodeResult, ICMP_ECHO_REPLY, ICMP_TIMEOUT,file))
				{
					//����Ŀ�ĵأ��˳�ѭ��
					if (DecodeResult.dwIPaddr.s_addr == destSockAddr.sin_addr.s_addr)
					{
						bReachDestHost = true;
						flag = true;
						end = clock();
						dur = (double)(end - start);
					}
					//���IP��ַ
					file << '\t' << inet_ntoa(DecodeResult.dwIPaddr) << endl;
					
					break;
				}
			}
			else if (WSAGetLastError() == WSAETIMEDOUT)    //���ճ�ʱ�����*��
			{
				file << "         *" << '\t' << "����ʱ" << endl;
				break;
			}
			else
			{
				break;
			}
		}
		iTTL++;    //����TTLֵ
	}
	/*if (iTTL > 30)
	{
		file << "NO\0";
	}*/
	file.close();
	WSACleanup();
	//system("pause");
	
	return ;
}