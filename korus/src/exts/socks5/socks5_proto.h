#pragma once

#include <stdint.h>

//	---ע��: ������֧��ipv6���Լ���֧�ֳ�"����Ҫ��֤��������֤"�����֤��ʽ---

#define SOCKS5_V	0x05

enum SOCKS_METHOD_TYPE
{
	SMT_NONE = 0,
	SMT_NOAUTH = 1,
	SMT_GSSAPI = 2,		// �ݲ�֧��
	SMT_USERPSW = 3,
};

//
//	�ͻ���	<--->	���������	<--->	Ŀ�������
//
//	1,	method_req:	u8ver + u8nmethod * u8method										�ͻ�����֧�ֵ���֤���ϣ��ô��������ѡһ��
//						u8ver = 0x05
//						u8method:
//							0x05		����Ҫ��֤
//							0x01		GSSAPI
//							0x02		�û��� / ����
//							0x03--0x7F	��IANA����
//							0x80--0xFE	Ϊ˽�˷�����������
//							0xFF		û�п��Խ��ܵķ���
//
//	2,	method_ack:	u8ver + u8method													�����������Ӧ����ѡ��һ������˵����֧��
//							

//	3,	auth_req:	u8ver + u8ulen * u8name + u8plen * u8psw							�ͻ����ύ�˺����룬�ô����������֤
//						u8ver = 0x01
//
//	4,	auth_ack:	u8ver + u8status													�����������Ӧ�Ƿ���֤�ɹ�
//						u8status:
//							0x00		�ɹ�
//							����		ʧ��		
//

//	5,	tunnel_req:	u8ver + u8cmd + u8rsv + u8atyp + vdst_addr + u16dst_port			�ͻ��������������ͨ����ͨ��
//						u8ver = 0x05
//						u8cmd:
//							0x01		CONNECTģʽ:
//								vdst_addr	Ŀ������ַ
//								u16dst_port	Ŀ�����˿�
//							0x02		BINDģʽ:
//								vdst_addr	Ŀ������ַ
//								u16dst_port	Ŀ�����˿�
//							0x03		UDP ASSOCIATEģʽ:
//								vdst_addr	��0
//								u16dst_port	Ҫ��ͻ����뷢��/����UDP���ı��ض˿�
//						u8rsv = 0x00
//						u8atyp:
//							0x01		IPV4,	vdst_addr = u32ip
//							0x03		����,	vdst_addr = u8len * u8domain
//							0x04		IPV6,	vdst_addr = u128ip
// 
//	6,	tunnel_ack:	u8ver + u8rep + u8rsv + u8atyp + vdst_addr + u16dst_port			������񷵻ش�ͨ������ر����BINDģʽ����һ������������Ӧ
//						rep:
//							0x00		�ɹ�
//							0x01		��ͨ��SOCKS����������ʧ��
//							0x02		���еĹ������������
//							0x03		���粻�ɴ�
//							0x04		�������ɴ�
//							0x05		���ӱ���
//							0x06		TTL��ʱ
//							0x07		��֧�ֵ�����
//							0x08		��֧�ֵĵ�ַ����
//							0x09�C0xFF	δ����
//
//						CONNECTģʽ:
//							vdst_addr	Ŀ������ַ
//							u16dst_port	Ŀ�����˿�
//						BINDģʽ��һ�λ�Ӧ:
//							vdst_addr	����������Ŀ���������ļ�����ַ
//							u16dst_port	����������Ŀ���������ļ����˿�
//						BINDģʽ�ڶ��λ�Ӧ:
//							vdst_addr	Ŀ��������ӵ���������������ӵ�ַ
//							u16dst_port	Ŀ��������ӵ���������������Ӷ˿�
//						UDP ASSOCIATEģʽ�£���ʱ��
//							vdst_addr	���������udp������ַ
//							u16dst_port	���������udp�����˿�

//	7,	udp_associate_req:	u16rsv + u8flag + u8atyp + vdst_addr + u16dst_port + vdata	�ͻ���������������udp���ݰ�������ת��/��⵽Ŀ�������
//						u16rsv = 0x0000
//						u8flag:	��ǰudp���ֶκţ��޷ֶ���0
//						vdst_addr:	Ŀ���������ַ
//						u16dst_port:Ŀ��������˿�
//
//	8,	udp_associate_ack:	u16rsv + u8flag + u8atyp + vdst_addr + u16dst_port + vdata	��������װ/ת������Ŀ����������ݰ����ͻ���
//						u16rsv = 0x0000
//						u8flag:	��ǰudp���ֶκţ��޷ֶ���0		
//						vdst_addr:	Ŀ���������ַ
//						u16dst_port:Ŀ��������˿�


