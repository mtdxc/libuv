﻿#pragma once
#ifndef __TINYWEB_H__
#define __TINYWEB_H__

#ifdef _MSC_VER
//去掉 warning C4996: '***': The POSIX name for this item is deprecated.Instead, use the ISO C++ conformant name...
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "uv.h"
/*
#pragma comment(lib, "libuv.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Userenv.lib")
*/
#  if defined(WIN32) && !defined(snprintf)
#  define snprintf _snprintf
#  endif

#else //__GNUGC__

#include <uv.h>

#endif

#include "tools.h"

#if TinyWeb_Function_Description //TinyWeb功能说明

auth lzpong 2016/11/24
功能基于 libuv 跨平台库

0.支持设置文档编码,默认 utf-8
1.支持使用HTTP: GET/POST方式访问
2.支持Socket, WebSocket 连接
3.支持返回404错误页面
4.支持指定根目录（默认程序所在目录）
5.支持任意格式文件访问(带/不带扩展名, 文件下载)
	a.支持静态网页访问：html/htm
	b.支持其他静态文件：js, css, png, jpeg/jpg, gif, ico, txt, xml, json, log, wam, wav, mp3, mp4, apk 等
	c.支持其他文件格式, 默认文件类型为："application/octet-stream"
	d.支持不带扩展名文件访问
	e.支持 Range 请求参数下载大文件(Range: bytes=sizeFrom-[sizeTo],支持负反向计算)
6.支持默认index页面(index.html/index.htm)，可以自定义设置
7.支持目录列表
8.不允许访问根目录上级文件或文件夹
9.支持回调
	a.接收到HTTP请求后先回调（此功能便于程序返回自定义功能）,回调失败或返回0时执行普通http响应
	b.WebSocket 数据回调
	c.socket 数据回调
10.支持x64,支持超过2G大文件
11.支持cookie/setcookie
12.支持添加自定义头部信息
13.支持POST较大的数据(支持分包发送的http Post内容)
==============stable,future
14.支持分包发送的http头部(http get)


#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tw_peerAddr {
	uchar  flag;//标志字节 ([0~7]: [0]是否需要保持连接 [1]是否WebSocket 高位4bit(flag>>4)与WebSocket协议每帧的opcode相同)
	ushort port;//客户端端口
	ushort fport;//本机端口
	size_t sk;//socket
	char   ip[17];//客户端IP
	char   fip[17];//本机IP
}tw_peerAddr;

typedef struct tw_reqHeads {
	uchar method;//0:Socket 1:GET 2:POST
	char host[260]; //IP:port, domain
	char path[512]; //路径
	char query[1500];//参数
	char* data; //数据
	char cookie[1025];//cookie
	size_t contentLen;//Content Lenth
	size_t len; //接收的数据长度
	long long Range_frm, Range_to;
}tw_reqHeads;

//服务配置
typedef struct tw_config {
//private data:
	uv_tcp_t _server;

//public data:
	uchar dirlist:1;      //是否允许列出目录
	const char* doc_dir;  //Web根目录，绝对路径，末尾带斜杠'\'(uninx为'/')； 默认程序文件所在目录
	const char* doc_index;//默认主页文件名，逗号分隔； 默认"index.html,index.htm"
	const char* ip;       //服务的IP地址 is only ipV4, can be NULL or "" or "*", which means "0.0.0.0"
	ushort port;          //服务监听端口
	const char* charset;  //文档编码(默认utf-8)

	void* udata;    //用户数据,如对象指针

	//客户端接入
	char (*on_connect)(void* udata, uv_stream_t* client, tw_peerAddr* pa);

	//返回非0表示已经处理处理请求
	//返回0表示没有适合的处理请求，将自动查找文件/文件夹，若未找到则发送404响应
	//此功能便于程序返回自定义功能
	//heads成员不需要free
	//pa->flag:标志字节 ([0~7]: [0]是否需要保持连接 [1]是否WebSocket [2]是否WebSocket文本帧
	char (*on_request)(void* udata, uv_stream_t* client, tw_peerAddr* pa, tw_reqHeads* heads);

	//Socket 或 WebSocket 数据, 可以通过buf->flag 或 pa->flag判断
	//buf成员不需要free
	//pa->flag:标志字节 ([0~7]: [0]是否需要保持连接 [1]是否WebSocket [2]是否WebSocket文本帧
	char (*on_data)(void* udata, uv_stream_t* client, tw_peerAddr* pa, membuf_t* buf);

	//Socket 检测到错误(此时链接可能已经断开)
	//错误消息格式："%d:%s,%s"
	//pa->flag:标志字节 ([0~7]: [0]是否需要保持连接 [1]是否WebSocket [2]是否WebSocket文本帧
	char (*on_error)(void* udata, uv_stream_t* client, tw_peerAddr* pa,int errcode, char* errstr);

	//Socket 已关闭(此时链接已经断开)
	//flag:标志字节 ([0~7]: [0]是否需要保持连接<非长连接为http> [1]是否WebSocket
	char (*on_close)(void* udata, uv_stream_t* client, tw_peerAddr* pa);
} tw_config;


//start web server, start with the config
//loop: if is NULL , it will be uv_default_loop()
//conf: the server config
//返回值不为0表示错误代码,用uv_err_name(n),和uv_strerror(n)查看原因字符串
int tinyweb_start(uv_loop_t* loop, tw_config* conf);

//stop TinyWeb
//loop: if is NULL , it will be &uv_default_loop()
void tinyweb_stop(uv_loop_t* loop);

//=================================================

//制造头部 SetCookie 字段和值
//set_cookie: 缓存区(至少 42+strlen(domain)=strlen(path) )
//ckLen: set_cookie的长度
//expires: 多少秒后过期
//domain: Domain, 域名或IP地址
//path: Path, 可以是 heads->path
void tw_make_setcookie(char* cookie, int ckLen, const char* key, const char* val, int expires, const char* domain, const char* path);

//制造头部 delete cookie
void tw_make_delcookie(char* del_cookie, int ckLen, char* key);

//返回格式华的HTTP响应内容 (需要free返回数据)
//status：http状态,如:"200 OK"
//ext_heads：额外的头部字符串，如："head1: this-is-head1\r\nSetCookie: TINY_SSID=Tiny1531896250879; Expires=...\r\n"
//content_type：文件类型，如："text/html" ；可以调用tw_get_content_type()得到
//content：使用utf-8编码格式的数据，特别是html文件类型的响应
//content_length：0或-1自动计算 content 长度(c_str, end with NULL)
//respone_size：if not NULL,可以获取发送的数据长度 the size of respone will be writen to request
//returns malloc()ed c_str, need free() by caller
char* tw_format_http_respone(uv_stream_t* client, const char* status, const char* ext_heads, const char* content_type, const char* content, size_t content_length, size_t* respone_size);

//根据扩展名返回文件类型 content_type
//可以传入路径/文件名/扩展名
const char* tw_get_content_type(const char* fileExt);

//发送数据到客户端; 如果是短连接,则发送完后会关闭连接
//data：待发送数据
//len： 数据长度, -1 将自动计算数据长度
//need_copy_data：是否需要复制数据; 如果是 WebSocket 就是 opCode
//need_free_data：是否需要free数据; 如果need_copy_data非零则忽略此参数
void tw_send_data(uv_stream_t* client, const void* data, size_t len, char need_copy_data, char need_free_data);

//发送'200 OK' 响应; 不会释放(free)传入的数据(u8data)
//content_type：Content Type 文档类型
//u8data：utf-8编码的数据
//content_length：数据长度，为0或-1时自动计算(strlen)(c_str, end with NULL)
//respone_size：获取响应最终发送的数据长度，为0表示放不需要取此长度
void tw_send_200_OK(uv_stream_t* client, const char* ext_heads, const char* content_type, const void* u8data, size_t content_length, size_t* respone_size);

//http协议发送文件,异步
//file_path: 文件路径
void tw_http_send_file(uv_stream_t* client, tw_reqHeads* heads, const char* ext_heads, const char* content_type, const char* file_path);

//发送301响应,路径永久重定位
void tw_301_Moved(uv_stream_t* client, tw_reqHeads* heads, const char* ext_heads);
//发送302响应,路径临时重定位
void tw_302_Moved(uv_stream_t* client, tw_reqHeads* heads, const char* ext_heads);

//关闭客户端连接
void tw_close_client(uv_stream_t* client);

#ifdef __cplusplus
} // extern "C"
#endif
#endif //__TINYWEB_H__