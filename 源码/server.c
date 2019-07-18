#include "staff.h"

sqlite3 * db;

void history_add(MSG *msg,char *buf);
void get_system_time(char* timedata);

int main(int argc, const char *argv[])
{
	char *errmsg;
	ssize_t recvRet;
	MSG msg;

	int serverfd,clientfd;
	int ret;
	int reuse = 1;
	struct sockaddr_in serveraddr,clientaddr;
	socklen_t len = sizeof(clientaddr);
	
	//select相关变量
	fd_set rdfs,temfs;	//定义select表
	int i,nfds,readycount;

	//提示参数填写
	if(argc != 3){
		printf("User:%s <IP> <port>\n",argv[0]);
		return -1;
	}
	
	//打开数据库，创建表
	if(sqlite3_open(STAFF_DATABASE,&db) != SQLITE_OK){
		printf("%s\n",sqlite3_errmsg(db));
		return -1;
	}
	if(sqlite3_exec(db,"create table staff(num int,type int, name text, passwd text, age int, telNumber text, address text, position text, date text, level int, salary int);",NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s\n",errmsg);
	}
	if(sqlite3_exec(db,"create table history(time text,name text,action text);",NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s\n",errmsg);	
	}

	//创建套接字
	if((serverfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		printf("fail to socket\n");
		return -1;
	}

	//填充地址
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi(argv[2]));
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
	
	//设置地址重用，克服链接断开需要等待的问题
	if(setsockopt(serverfd,SOL_SOCKET,SO_REUSEADDR,(void *)&reuse,sizeof(reuse)) < 0){
		printf("setsockopt reuseAddr fail\n");
		close(serverfd);
		return 0;
	}

	//绑定套接字
	if((ret = bind(serverfd,(struct sockaddr * )&serveraddr,sizeof(serveraddr))) < 0){
		printf("fail to bind\n");
		return -1;
	}

	//监听套接字
	if((ret = listen(serverfd,10)) < 0){
		printf("fail to listen\n");
		return -1;
	}
	
	//清空表
	FD_ZERO(&rdfs);
	FD_ZERO(&temfs);
	
	//添加监听的事件
	FD_SET(serverfd,&rdfs);
	nfds = serverfd + 1;

	while(1){
		temfs = rdfs;

		//等待准备好的事件
		readycount = select(nfds,&temfs,NULL,NULL,NULL);
		if(readycount < 0){
			printf("fail to select\n");
			return -1;
		}else if(readycount == 0){
			printf("time out \n");
			continue;
		}

		//遍历监听的文件描述符
		for(i = 0; i < nfds; i++ ){
			if(FD_ISSET(i,&temfs)){	//判断是否是读表中的事件
				if(i == serverfd){

					//接受连接请求
					clientfd = accept(serverfd,(struct sockaddr *)&clientaddr,&len);
					if(clientfd < 0){
						printf("fail to accept\n");
						return -1;
					}
					printf("new client: ip %s,%d\n",inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

					//新连接添加监听
					FD_SET(clientfd,&rdfs);
					if(clientfd > nfds -1){
						nfds = clientfd + 1;	//更新最大描述符
					}
				}else{
					recvRet = recv(i,&msg,sizeof(msg),0);
					
					if(recvRet < 0){
						printf("fail to receive client data\n");
						continue;
					}else if(recvRet == 0){
						printf("client exit\n");
						close(i);
						FD_CLR(i,&rdfs);
					}else{
						process_client_request(i,&msg);
					}
				}
			}
		}
	}

	close(serverfd);

	return 0;
}

int process_client_request(int clientfd,MSG *msg)
{
	//printf("process_client_request entry or not\n");
	switch(msg->msgtype){
		case MANAGER_LOGIN:
		case USER_LOGIN:
			process_login(clientfd,msg);
			break;
		case MANAGER_QUERY:
			process_manager_query(clientfd,msg);
			break;
		case MANGER_MODIFY:
			process_manager_modify(clientfd,msg);
			break;
		case MANAGER_ADDUSER:
			process_manager_adduser(clientfd,msg);
			break;
		case MANAGER_DELUSER:
			process_manager_deluser(clientfd,msg);
			break;
		case MANAGER_HISTORY:
			process_manager_history(clientfd,msg);
			break;
		case USER_QUERY:
			process_user_query(clientfd,msg);
			break;
		case USER_MODIFY:
			process_user_modify(clientfd,msg);
			break;
		case QUIT:
			process_client_quit(clientfd,msg);
			break;
		default:
			break;
	}
	return 0;
}

int process_client_quit(int clientfd,MSG* msg)
{
	//printf("%s quit\n",msg->name);
	return 0;
}

int process_login(int clientfd,MSG *msg)
{
	char sq[LENGTH];
	char *errmsg;
	char **result;
	int nrow,ncolumn;

	msg->info.type = msg->usertype;
	strcpy(msg->info.name,msg->name);
	strcpy(msg->info.passwd,msg->passwd);
	
	sprintf(sq,"select * from staff where type=%d and name='%s' and passwd='%s';",msg->info.type,msg->info.name,msg->info.passwd);
	if(sqlite3_get_table(db,sq,&result,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
		printf("%s\n",errmsg);		
	}else{
		if(nrow == 0){
			strcpy(msg->recvmsg,"name or passwd failed.\n");
			send(clientfd,msg,sizeof(MSG),0);
		}else{
			strcpy(msg->recvmsg,"OK");
			send(clientfd,msg,sizeof(MSG),0);
		}
	}
	
	return 0;
}

int process_manager_query(int clientfd,MSG *msg)
{
	char sq[LENGTH];
	char *errmsg;
	char **result;
	int nrow,ncolumn;
	int i;

	if(msg->flags == 1){
		sprintf(sq,"select * from staff where name='%s';",msg->info.name);
	}else{
		sprintf(sq,"select * from staff;");
	}

	if(sqlite3_get_table(db,sq,&result,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
		printf("%s",errmsg);
	}else{
		int index = ncolumn;
		for(i = 0; i < nrow; i++){
		//	sprintf(msg->recvmsg,"%s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s",result[i][0],\
		//			);
			sprintf(msg->recvmsg,"%s,    %s,      %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s;",result[index+ncolumn-11],result[index+ncolumn-10],\
					result[index+ncolumn-9],result[index+ncolumn-8],result[index+ncolumn-7],result[index+ncolumn-6],result[index+ncolumn-5],\
					result[index+ncolumn-4],result[index+ncolumn-3],result[index+ncolumn-2],result[index+ncolumn-1]);
			send(clientfd,msg,sizeof(MSG),0);
			index += ncolumn;
		}
		
		if(msg->flags != 1){
			strcpy(msg->recvmsg,"over");
			send(clientfd,msg,sizeof(MSG),0);
		}
		sqlite3_free_table(result);

	}
	return 0;
}

int process_manager_modify(int clientfd,MSG* msg)
{
	int nrow,ncolumn;		
	char *errmsg, **resultp;
	char sql[LENGTH] = {0};	
	char historybuf[LENGTH] = {0};

	switch(msg->recvmsg[0]){
	case 'N':
		sprintf(sql,"update staff set name='%s' where num=%d;",msg->info.name, msg->info.num);
		sprintf(historybuf,"%s修改工号为%d的用户名为%s",msg->name,msg->info.num,msg->info.name);
		break;
	case 'A':
		sprintf(sql,"update staff set age=%d where num=%d;",msg->info.age, msg->info.num);
		sprintf(historybuf,"%s修改工号为%d的年龄为%d",msg->name,msg->info.num,msg->info.age);
		break;
	case 'P':
		sprintf(sql,"update staff set telNumber='%s' where num=%d;",msg->info.tel_number,msg->info.num);
		sprintf(historybuf,"%s修改工号为%d的电话为%s",msg->name,msg->info.num,msg->info.tel_number);
		break;
	case 'D':
		sprintf(sql,"update staff set address='%s' where num=%d;",msg->info.address, msg->info.num);
		sprintf(historybuf,"%s修改工号为%d的家庭住址为%s",msg->name,msg->info.num,msg->info.address);
		break;
	case 'W':
		sprintf(sql,"update staff set position='%s' where num=%d;",msg->info.position, msg->info.num);
		sprintf(historybuf,"%s修改工号为%d的职位为%s",msg->name,msg->info.num,msg->info.position);
		break;
	case 'T':
		sprintf(sql,"update staff set date='%s' where num=%d;",msg->info.date, msg->info.num);
		sprintf(historybuf,"%s修改工号为%d的入职日期为%s",msg->name,msg->info.num,msg->info.date);
		break;
	case 'L':
		sprintf(sql,"update staff set level=%d where num=%d;",msg->info.level, msg->info.num);
		sprintf(historybuf,"%s修改工号为%d的评级为%d",msg->name,msg->info.num,msg->info.level);
		break;
	case 'S':
		sprintf(sql,"update staff set salary=%d where num=%d;",msg->info.salary, msg->info.num);
		sprintf(historybuf,"%s修改工号为%d的工资为%d",msg->name,msg->info.num,msg->info.salary);
		break;
	case 'M':
		sprintf(sql,"update staff set passwd='%s' where num=%d;",msg->info.passwd, msg->info.num);
		sprintf(historybuf,"%s修改工号为%d的密码为%s",msg->name,msg->info.num,msg->info.passwd);
		break;
	}

	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s.\n",errmsg);
		sprintf(msg->recvmsg,"数据库修改失败！%s", errmsg);
	}else{
		printf("the database is updated successfully.\n");
		sprintf(msg->recvmsg, "数据库修改成功!");
		history_add(msg,historybuf);
	}

	send(clientfd,msg,sizeof(MSG),0);

	return 0;
}

int process_manager_adduser(int clientfd,MSG *msg)
{
	char sq[LENGTH];
	char buf[LENGTH];
	char *errmsg;

	sprintf(sq,"insert into staff values(%d,%d,'%s','%s',%d,'%s','%s','%s','%s',%d,%d);",\
			msg->info.num,msg->info.type,msg->info.name,msg->info.passwd,\
			msg->info.age,msg->info.tel_number,msg->info.address,msg->info.position,\
					msg->info.date,msg->info.level,msg->info.salary);

	if(sqlite3_exec(db,sq,NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s\n",errmsg);
		strcpy(msg->recvmsg,"failed");
		send(clientfd,msg,sizeof(MSG),0);
		return -1;
	}else{
		strcpy(msg->recvmsg,"OK");
		send(clientfd,msg,sizeof(msg),0);
	}

	sprintf(buf,"管理员%s添加了%s用户",msg->name,msg->info.name);
	history_add(msg,buf);
	return 0;
}

int process_manager_deluser(int clientfd,MSG *msg)
{
	char sq[LENGTH];
	char buf[LENGTH];
	char *errmsg;

	sprintf(sq,"delete from staff where num=%d and name='%s';",msg->info.num,msg->info.name);

	if(sqlite3_exec(db,sq,NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s\n",errmsg);
		strcpy(msg->recvmsg,"failed");
		send(clientfd,msg,sizeof(MSG),0);
		return -1;
	}else{
		strcpy(msg->recvmsg,"OK");
		send(clientfd,msg,sizeof(msg),0);
	}

	sprintf(buf,"管理员%s删除了%s用户",msg->name,msg->info.name);
	history_add(msg,buf);

	return 0;
}

void history_add(MSG *msg,char *buf)
{
	int nrow,ncolumn;
	char *errmsg, **resultp;
	char sqlhistory[LENGTH] = {0};
	char timedata[LENGTH] = {0};

	get_system_time(timedata);

	sprintf(sqlhistory,"insert into history values ('%s','%s','%s');",timedata,msg->name,buf);
	if(sqlite3_exec(db,sqlhistory,NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s\n",errmsg);
		printf("insert into history failed.\n");
	}else{
		printf("insert into history success.\n");
	}
}

void get_system_time(char* timedata)
{
	time_t t;
	struct tm *tp;

	time(&t);
	tp = localtime(&t);
	sprintf(timedata,"%d-%d-%d %d:%d:%d",tp->tm_year+1900,tp->tm_mon+1,\
			tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec);
	return ;
}

int process_manager_history(int clientfd,MSG* msg)
{	
	char sq[LENGTH];
	char *errmsg;
	char **result;
	int nrow,ncolumn;
	int i;
	
	printf("receive or not\n");
	sprintf(sq,"select * from history;");

	if(sqlite3_get_table(db,sq,&result,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
		printf("%s",errmsg);
	}else{
		int index = ncolumn;
		for(i = 0; i < nrow; i++){
			sprintf(msg->recvmsg,"%s,    %s,    %s;",result[index+ncolumn-3],result[index+ncolumn-2],result[index+ncolumn-1]);
			send(clientfd,msg,sizeof(MSG),0);
			index += ncolumn;
		}
		
		strcpy(msg->recvmsg,"over");
		send(clientfd,msg,sizeof(MSG),0);

		sqlite3_free_table(result);

	}

	return 0;	
}

int process_user_query(int clientfd, MSG* msg)
{
	char sq[LENGTH];
	char *errmsg;
	char **result;
	int nrow,ncolumn;
	int i;

	sprintf(sq,"select * from staff where name='%s';",msg->name);

	if(sqlite3_get_table(db,sq,&result,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
		printf("%s",errmsg);
	}else{
		int index = ncolumn;
		for(i = 0; i < nrow; i++){
		//	sprintf(msg->recvmsg,"%s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s",result[i][0],\
		//			);
			sprintf(msg->recvmsg,"%s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s;",result[index+ncolumn-11],result[index+ncolumn-10],\
					result[index+ncolumn-9],result[index+ncolumn-8],result[index+ncolumn-7],result[index+ncolumn-6],result[index+ncolumn-5],\
					result[index+ncolumn-4],result[index+ncolumn-3],result[index+ncolumn-2],result[index+ncolumn-1]);
			send(clientfd,msg,sizeof(MSG),0);
			index += ncolumn;
		}
		
		sqlite3_free_table(result);

	}
	return 0;
}

int process_user_modify(int clientfd,MSG *msg)
{
	int nrow,ncolumn;
	char *errmsg, **resultp;
	char sql[LENGTH] = {0};	
	char historybuf[LENGTH] = {0};

	switch (msg->recvmsg[0])
	{	
	case 'P':
		sprintf(sql,"update staff set telNumber='%s' where num=%d;",msg->info.tel_number,msg->info.num);
		sprintf(historybuf,"%s修改工号为%d的电话为%s",msg->name,msg->info.num,msg->info.tel_number);
		break;
	case 'D':
		sprintf(sql,"update staff set address='%s' where num=%d;",msg->info.address, msg->info.num);
		sprintf(historybuf,"%s修改工号为%d的家庭住址为%s",msg->name,msg->info.num,msg->info.address);
		break;
	case 'M':
		sprintf(sql,"update staff set passwd='%s' where num=%d;",msg->info.passwd, msg->info.num);
		sprintf(historybuf,"%s修改工号为%d的密码为%s",msg->name,msg->info.num,msg->info.passwd);
		break;

	}

	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s.\n",errmsg);
		sprintf(msg->recvmsg,"数据库修改失败！%s\n", errmsg);
	}else{
		printf("the database is updated successfully.\n");
		sprintf(msg->recvmsg, "数据库修改成功!\n");
		history_add(msg,historybuf);
	}

	send(clientfd,msg,sizeof(MSG),0);

	return 0;
}
