#ifndef __STAFF_H__
#define __STAFF_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>
#include <time.h>

#define MANAGER 1	//管理员模式
#define USER 2		//用户模式
#define QUIT 3		//退出

#define MANAGER_LOGIN 4 
#define MANGER_MODIFY 5
#define MANAGER_ADDUSER 6
#define MANAGER_DELUSER 7
#define MANAGER_QUERY 8
#define MANAGER_HISTORY 9

#define USER_LOGIN 10
#define USER_MODIFY 11
#define USER_QUERY 12

#define LENGTH 128	//字符串长度
#define STAFF_DATABASE "./staff_manage.db"

//员工信息
typedef struct {
	int type;				//用户类型
	char name[LENGTH];		//员工姓名
	char passwd[LENGTH];	//用户密码
	int num;				//工号
	int age;				//年龄
	char tel_number[LENGTH];//电话
	char address[LENGTH];	//地址
	char position[LENGTH];	//职位
	char date[LENGTH];		//何时入职
	int level;				//等级
	int salary;				//工资
}staff_info;

//传递的信息
typedef struct{
	int msgtype;			//消息类型
	int usertype;			//用户类型
	char name[LENGTH];
	char passwd[LENGTH];
	char recvmsg[LENGTH];
	int flags; 			//查询标志位
	staff_info info;	//员工信息
}MSG;

#endif	//__STAFF_H__
