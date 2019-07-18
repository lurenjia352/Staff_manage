#include "staff.h"

int main(int argc, const char *argv[])
{
	int clientfd;
	struct sockaddr_in serveraddr;
	socklen_t len = sizeof(serveraddr);

	//提示参数填写
	if(argc != 3){
		printf("User:%s <IP> <port>\n",argv[0]);
		return -1;
	}

	//创建套接字
	if((clientfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		printf("fail to create client socket\n");
		return -1;
	}
	
	//填充地址
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi(argv[2]));
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);

	//连接服务器
	if(connect(clientfd,(const struct sockaddr *)&serveraddr,len) < 0){
		printf("fail to connect\n");
		return -1;
	}

	client_login(clientfd);

	close(clientfd);

	return 0;
}

int client_login(int clientfd)
{
	int n;
	MSG msg;
	memset(&msg.info,0,sizeof(staff_info));

	while(1){
		printf("*************************************************************\n");
		printf("********  1：管理员模式    2：普通用户模式    3：退出********\n");
		printf("*************************************************************\n");
		printf("请输入您的选择（数字）>>");
		scanf("%d",&n);
		getchar();

		switch(n){
		case 1:
			msg.msgtype = MANAGER_LOGIN;
			msg.usertype = MANAGER;
			break;
		case 2:
			msg.msgtype = USER_LOGIN;
			msg.usertype = USER;
			break;
		case 3:
			msg.msgtype = QUIT;
			if(send(clientfd,&msg,sizeof(MSG),0) < 0){
				printf("fail to send message\n");
				return -1;
			}
			close(clientfd);
			return 0;
		default:
			printf("您的输入有误，请重新输入\n");
			continue;
		}
		memset(msg.name, 0, LENGTH);
		printf("请输入用户名：");
		scanf("%s",msg.name);
		getchar();

		memset(msg.passwd, 0, LENGTH);
		printf("请输入密码：");
		scanf("%s",msg.passwd);
		getchar();

		send(clientfd,&msg,sizeof(MSG),0);
		recv(clientfd,&msg,sizeof(MSG),0);
		
		if(strncmp(msg.recvmsg,"OK",2) == 0){
			if(msg.usertype == MANAGER){
				printf("亲爱的管理员，欢迎登录用户管理系统\n");
				manger_manu(clientfd,&msg);
			}else if(msg.usertype == USER){
				printf("亲爱的用户，欢迎登录用户管理系统\n");
				user_manu(clientfd,&msg);
			}
		}else{
			printf("登录失败:%s\n",msg.recvmsg);
			return -1;
		}
	}
	return 0;
}

int manger_manu(int clientfd,MSG * msg)
{
	int n;

	while(1)
	{
		printf("*************************************************************\n");
		printf("* 1：查询  2：修改 3：添加用户  4：删除用户  5：查询历史记录*\n");
		printf("* 6：退出													*\n");
		printf("*************************************************************\n");
		printf("请输入您的选择（数字）>>");
		scanf("%d",&n);
		getchar();

		switch(n)
		{
		case 1:
			manger_query(clientfd,msg);
			break;
		case 2:
			manger_modify(clientfd,msg);
			break;
		case 3:
			manger_adduser(clientfd,msg);
			break;
		case 4:
			manger_deluser(clientfd,msg);
			break;
		case 5:
			manger_history(clientfd,msg);
			break;
		case 6:
			//msg->msgtype = QUIT;
			//send(clientfd, msg, sizeof(MSG), 0);
			//close(clientfd);
			return 0;
		default:
			printf("您输入有误，请重新输入\n");
		}
	}
	return 0;
}

int manger_query(int clientfd,MSG *msg)
{
	int n;
	msg->msgtype = MANAGER_QUERY;
	while(1){
		memset(&msg->info,0,sizeof(staff_info));
		memset(msg->recvmsg,0,LENGTH);
		printf("*************************************************************\n");
		printf("******** 1：按人名查找  	2：查找所有 	3：退出	 ********\n");
		printf("*************************************************************\n");
		printf("请输入您的选择（数字）>>");
		scanf("%d",&n);
		getchar();

		
		switch(n){
			case 1:
				msg->flags = 1;
				printf("请输入您要查找的用户名：");
				scanf("%s",msg->info.name);
				getchar();

				send(clientfd, msg, sizeof(MSG), 0);
				recv(clientfd, msg, sizeof(MSG), 0);
				printf("工号 \t用户类型  姓名  \t密码  \t年龄\t电话\t\t地址   \t职位   \t入职年月    \t等级\t 工资\n");
				printf("%s\n",msg->recvmsg);
				break;
			case 2:
				msg->flags = 2;
				send(clientfd, msg, sizeof(MSG), 0);
				printf("工号 \t用户类型  姓名  \t密码  \t年龄\t电话\t\t地址   \t职位   \t入职年月    \t等级\t 工资\n");
				while(1){
					recv(clientfd,msg,sizeof(MSG),0);
					if(strncmp(msg->recvmsg,"over",4) == 0){
						break;
					}
					printf("%s\n",msg->recvmsg);
				}
				break;
			case 3:
				msg->msgtype = QUIT;
				send(clientfd, msg, sizeof(MSG), 0);
				//close(clientfd);
				return 0;
		}
	}
	return 0;
}

int manger_modify(int clientfd,MSG *msg)
{
	int n;
	msg->msgtype = MANGER_MODIFY;
	memset(&msg->info,0,sizeof(staff_info));
	send(clientfd,msg,sizeof(MSG),0);

	printf("请输入您要修改的工号：");
	scanf("%d", &msg->info.num);
	getchar();
	
	printf("*******************请输入要修改的选项********************\n");
	printf("******	1：姓名	  2：年龄	3：家庭住址   4：电话  ******\n");
	printf("******	5：职位	   6：工资  7：入职年月   8：评级  ******\n");
	printf("******	9：密码	 10：退出				  *******\n");
	printf("*************************************************************\n");
	printf("请输入您的选择（数字）>>");
	scanf("%d",&n);
	getchar();

	switch(n){
		case 1:
			printf("请输入用户名：");
			msg->recvmsg[0] = 'N';
			scanf("%s",msg->info.name);
			getchar();
			break;
		case 2:
			printf("请输入年龄：");
			msg->recvmsg[0] = 'A';
			scanf("%d",&msg->info.age);
			getchar();
			break;
		case 3:
			printf("请输入家庭住址：");
			msg->recvmsg[0] = 'D';
			scanf("%s",msg->info.address);
			getchar();
			break;
		case 4:
			printf("请输入电话：");
			msg->recvmsg[0] = 'P';
			scanf("%s",msg->info.tel_number);
			getchar();
			break;
		case 5:
			printf("请输入职位：");
			msg->recvmsg[0] = 'W';
			scanf("%s",msg->info.position);
			getchar();
			break;
		case 6:
			printf("请输入工资：");
			msg->recvmsg[0] = 'S';
			scanf("%d",&msg->info.salary);
			getchar();
			break;
		case 7:
			printf("请输入入职日期(格式：0000.00.00.)：");
			msg->recvmsg[0] = 'T';
			scanf("%s",msg->info.date);
			getchar();
			break;
		case 8:
			printf("请输入评级(1~5,5为最高)：");
			msg->recvmsg[0] = 'L';
			scanf("%d",&msg->info.level);
			getchar();
			break;
		case 9:
			printf("请输入新密码：(6位数字)");
			msg->recvmsg[0] = 'M';
			scanf("%s",msg->info.passwd);
			getchar();
			break;
		case 10:
			return 0;
	}
	
	send(clientfd,msg,sizeof(MSG),0);
	recv(clientfd,msg,sizeof(MSG),0);
	printf("%s\n",msg->recvmsg);
	//printf("end\n");
	return 0;
}

int manger_adduser(int clientfd,MSG *msg)
{
	char confirm;
	msg->msgtype  = MANAGER_ADDUSER;
	memset(&msg->info,0,sizeof(staff_info));
	
	while(1){
		printf("请输入工号：");
		scanf("%d",&msg->info.num);
		getchar();
		printf("您输入的工号是：%d",msg->info.num);
		printf("工号信息一旦录入无法更改，请确认您所输入的是否正确！(Y/N)");
		scanf("%c",&confirm);
		getchar();
		if(confirm == 'N' || confirm == 'n'){
			printf("请重新添加用户：");
			break;
		}

		printf("请输入用户名：");
		scanf("%s",msg->info.name);
		getchar();

		printf("请输入用户密码：");
		scanf("%6s",msg->info.passwd);
		getchar();

		printf("请输入年龄：");
		scanf("%d",&msg->info.age);
		getchar();

		printf("请输入电话：");
		scanf("%s",msg->info.tel_number);
		getchar();

		printf("请输入家庭住址：");
		scanf("%s",msg->info.address);
		getchar();

		printf("请输入职位：");
		scanf("%s",msg->info.position);
		getchar();

		printf("请收入入职日期(格式：0000.00.00)：");
		scanf("%s",msg->info.date);
		getchar();

		printf("请输入评级(1~5,5为最高，新员工为 1)：");
		scanf("%d",&msg->info.level);
		getchar();

		printf("请输入工资：");
		scanf("%d",&msg->info.salary);
		getchar();

		printf("是否为管理员：(Y/N)");
		scanf("%c",&confirm);
		getchar();
		if(confirm == 'Y' || confirm == 'y')
			msg->info.type = MANAGER;
		else if(confirm == 'N' || confirm == 'n')
			msg->info.type = USER;
		printf("msg->info.usertype:%d\n",msg->info.type);

		send(clientfd,msg,sizeof(MSG),0);
		recv(clientfd,msg,sizeof(MSG),0);

		if(strncmp(msg->recvmsg,"OK",2) == 0){
			printf("添加成功\n");
		}else{
			printf("%s",msg->recvmsg);
		}

		printf("是否继续添加员工:(Y/N)");
		scanf("%c",&confirm);
		getchar();
		if(confirm == 'N' || confirm == 'n')
			break;
	}
	return 0;
}

int manger_deluser(int clientfd,MSG *msg)
{
	msg->msgtype = MANAGER_DELUSER;

	printf("请输入要删除的用户工号："); 
	scanf("%d",&msg->info.num);
	getchar();

	printf("请输入要删除的用户名："); 
	scanf("%s",msg->info.name);
	getchar();

	send(clientfd, msg, sizeof(MSG), 0);
	recv(clientfd, msg, sizeof(MSG), 0);

	if(strncmp(msg->recvmsg,"OK",2)==0)
		printf("删除成功！\n");
	else
		printf("%s",msg->recvmsg);

	printf("删除工号为：%d 的用户.\n",msg->info.num);
	return 0;
}

int manger_history(int clientfd,MSG *msg)
{
	msg->msgtype = MANAGER_HISTORY;
	memset(msg->recvmsg,0,LENGTH);
	send(clientfd,msg,sizeof(MSG),0);
	while(1){
		printf("time\t\tname\toption\n");
		recv(clientfd,msg,sizeof(MSG),0);
		if(strncmp(msg->recvmsg,"over",4) == 0)
			break;
		printf("%s\n",msg->recvmsg);
	}
	printf("----查询历史记录结束----\n");
	return 0;
}

int user_manu(int clientfd,MSG * msg)
{
	int n;
	while(1)
	{
		printf("*************************************************************\n");
		printf("*************  1：查询  	2：修改		3：退出	 *************\n");
		printf("*************************************************************\n");
		printf("请输入您的选择（数字）>>");
		scanf("%d",&n);
		getchar();

		switch(n)
		{
		case 1:
			user_query(clientfd,msg);
			break;
		case 2:
			user_modify(clientfd,msg);
			break;
		case 3:
			//msg->msgtype = QUIT;
			//send(clientfd, msg, sizeof(MSG), 0);
			//close(clientfd);
			return 0;
		default:
			printf("您输入有误，请重新输入\n");
			break;
		}
	}
	return 0;
}

int user_query(int clientfd,MSG *msg)
{
	msg->msgtype = USER_QUERY;

	send(clientfd,msg,sizeof(MSG),0);
	recv(clientfd,msg,sizeof(MSG),0);
	printf("工号 \t用户类型\t 姓名  \t密码  \t年龄\t电话   \t地址   \t职位 \t入职年月  \t等级\t 工资\n");
	printf("%s\n",msg->recvmsg);
	return 0;
}

int user_modify(int clientfd,MSG *msg)
{
	int n = 0;
	msg->msgtype = USER_MODIFY;

	printf("请输入您要修改的工号：");
	scanf("%d", &msg->info.num);
	getchar();

	printf("***********请输入要修改的选项(其他信息亲请联系管理员)*********\n");
	printf("***********	1：家庭住址   2：电话  3：密码  4：退出***********\n");
	printf("**************************************************************\n");
	printf("请输入您的选择（数字）>>");
	scanf("%d",&n);
	getchar();

	switch(n)
	{
	case 1:
		printf("请输入家庭住址：");
		msg->recvmsg[0] = 'D';
		scanf("%s",msg->info.address);
		getchar();
		break;
	case 2:
		printf("请输入电话：");
		msg->recvmsg[0] = 'P';
		scanf("%s",msg->info.tel_number);
		getchar();
		break;
	case 3:
		printf("请输入新密码：(6位数字)");
		msg->recvmsg[0] = 'M';
		scanf("%6s",msg->info.passwd);
		getchar();
		break;
	case 4:
		return 0;
	}

	send(clientfd, msg, sizeof(MSG), 0);
	recv(clientfd, msg, sizeof(MSG), 0);
	printf("%s",msg->recvmsg);

	printf("修改结束.\n");
	return 0;	
}



	
