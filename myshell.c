#include<unistd.h>
#include<stdio.h>
#include<sys/wait.h>
#include<string.h>
#include<signal.h>
#include<stdlib.h>

#define EOL 1  //줄의 끝
#define ARG 2  //정상적 인수
#define AMPERSAND 3  //백그라운드
#define SEMICOLON 4  
#define PIPE 5  //파이프라인

#define MAXARG 512  //명령인수의 최대수
#define MAXBUF 512  //입력줄의 최대길이
#define FOREGROUND 0  
#define BACKGROUND 1


//프로그램 버퍼 및 작업용 포인터들
static char inpbuf[MAXBUF], tokbuf[2 * MAXBUF],
*ptr = inpbuf, *tok = tokbuf;

static struct sigaction act;  //시그널 처리
static char special[] = { ' ','\t','\n','\0' };  //특별히 쓰이는 문자

int fatal(char* s);
void join_command(char* lfpipe[], char* rgpipe[]);
void separate(char* command[], char* lfpipe[], char* rgpipe[]);
char *prompt = "Prompt> ";  //프롬프트 


//프롬프트를 프린트하고 한 줄을 읽는다
int userin(char *p)
{
	int c, count;

	//뒤의 루틴들을 위한 초기화
	ptr = inpbuf;
	tok = tokbuf;

	printf("%s", p);  //프롬프트를 표시

	count = 0;

	while (1)  //사용자로부터 명령어 읽어들이기
	{
		if ((c = getchar()) == EOF)
			return (EOF);

		if (count < MAXBUF)
			inpbuf[count++] = c;

		//정상적인 경우
		if (c == '\n' && count < MAXBUF)
		{
			inpbuf[count] = '\0';
			return count;
		}

		//줄이 너무 길면 재시작
		if (c == '\n')
		{
			printf("smallsh : input line too long \n");
			count = 0;
			printf("%s", p);
		}
	}
}


//한 문자가 특별히 쓰이는 문자인지 아닌지 조사
int inarg(char c)
{
	char *wrk;

	for (wrk = special; *wrk; wrk++)
	{
		if (c == *wrk)  //특수문자일 경우 0을 리턴
			return(0);
	}

	return(1);
}


//토큰을 가져와서 tokbuf에 넣음
int gettok(char ** outptr)
{
	int type;

	*outptr = tok;  //outptr 문자열을 tok로 지정

	//토큰을 포함하고 있는 버퍼로부터 여백 제거
	while (*ptr == ' ' || *ptr == '\t')
		ptr++;

	*tok++ = *ptr;  //토큰 포인터를 버퍼내의 첫 토큰으로 지정

	//버퍼내의 토큰에 따라 유형 변수 지정
	switch (*ptr++) {
	case '\n':
		type = EOL;
		break;
	case '&':
		type = AMPERSAND;
		break;
	case ';':
		type = SEMICOLON;
		break;
	case '|':
		type = PIPE;
		break;
	default:
		type = ARG;

		//유효한 보통문자들을 계속 읽음
		while (inarg(*ptr))
			*tok++ = *ptr++;
	}

	*tok++ = '\0';
	return type;
}



//시그널 처리(인터럽트키와 종료키에 대한)
void sigex(int sig_default)
{
	//디폴트 행동을 복원
	if (sig_default == 1)
	{
		act.sa_handler = SIG_DFL;  //디폴트 지정
		sigfillset(&(act.sa_mask));

		sigaction(SIGINT, &act, NULL);
		sigaction(SIGQUIT, &act, NULL);
	}

	//SIGINT,SIGQUIT 시그널 무시
	else if (sig_default == 0)
	{
		act.sa_handler = SIG_IGN;
		sigfillset(&(act.sa_mask));

		sigaction(SIGINT, &act, NULL);
		sigaction(SIGQUIT, &act, NULL);
	}
}


//입력줄 처리
int procline(void)
{
	char *arg[MAXARG + 1];  //runcommand를 위한 포인터 배열
	int toktype;  //명령내의 토큰의 유형
	int narg;  //지금까지의 인수 수
	int type;  //FOREGROUND 또는 BACKGROUND
	int ispipe = 0; //파이프 여부

	narg = 0;

	//영원히 루프를 돌음
	for (;;)
	{
		//토큰 유형에 따라 행동을 취함
		switch (toktype = gettok(&arg[narg])) {  //gettok으로 명령줄로부터 개별 토큰 추출
		case ARG:  //유효한 보통문자
			if (narg<MAXARG)
				narg++;
			break;
		case PIPE: //파이프
			if (narg<MAXARG) narg++;
			ispipe = 1;
			break;
		case EOL:
		case SEMICOLON:
		case AMPERSAND:
			if (toktype == AMPERSAND)  //백그라운드 설정
				type = BACKGROUND;
			else
				type = FOREGROUND;

			if (narg != 0)
			{
				arg[narg] = NULL;
				runcommand(arg, type, ispipe);
			}
			if (toktype == EOL)
				return;
			narg = 0;
			break;
		}
	}
}



//명령 수행 위한 모든 프로세스 시작
int runcommand(char** cline, int where, int ispipe)
{
	pid_t pid;
	int status;
	char shell_exit[7] = "logout"; //로그아웃(myshell 종료) 명령
	char cd[3] = "cd";  //cd(디렉토리 변경) 명령

	//프롬프트에서 로그아웃 명령을 받으면 myshell종료
	if (strcmp(*cline, shell_exit) == 0)
	{
		exit(0);
	}

	//cd명령이오면 디렉토리 변경
	if (strcmp(*cline, cd) == 0)
	{
		if (chdir(*(cline+1)) == -1)
			perror("directory change failed");
		return (0);
	}

	switch (pid = fork()) {
	case -1:
		perror("smallsh");
		return(-1);
	case 0:   //자식
		if (where == BACKGROUND)
			sigex(0);  //background라면 시그널 무시해야함
		else
			sigex(1);  //foreground라면 시그널의 디폴트 행동

		if (ispipe) //파이프 여부에 따라 파이프 지원
		{
			char* com1[10];
			char* com2[10];

			separate(cline, com1, com2);  //파이프를 기준으로 왼쪽,오른쪽 명령어 분리
			join_command(com1, com2);  //분리된 두 명령어를 파이프로 결합 실행

			exit(1);
		}

		//파이프가 없다면 명령어 바로 실행
		execvp(*cline, cline);
		perror(*cline);
		exit(1);
	}

	/*부모 코드
	만일 백그라운드 프로세스이면 프로세스가 실행되었음을 알리며 
	프로세스 식별자를 프린트하고 퇴장*/
	if (where == BACKGROUND)
	{
		printf("[Start working in the background, process id : %d]\n", pid);
		return (0);
	}

	//프로세스 pid가 퇴장할때까지 기다림
	if (waitpid(pid, &status, 0) == -1)
		return(-1);
	else
		return(status);

}



//파이프 처리할 명령 2개를 분리
void separate(char* command[], char* lfpipe[], char* rgpipe[])
{
	int m = 0, n = 0;

	for (; command[m] != NULL; m++)
	{
		if (*command[m] == '|')  //문자가 파이프라면
		{
			for (; command[m + 1] != NULL; m++, n++)  //파이프 뒤의 문자열을 추출
			{
				rgpipe[n] = command[m + 1];  //파이프 뒤의 문자열을 rgpipe에 순서대로 저장
			}
			rgpipe[n] = NULL;
			return;

		}

		//문자열 내용의 끝까지 파이프가 없었다면 lfpipe에 순서대로 저장
		lfpipe[m] = command[m];
	}
}



//두 명령을 파이프로 결합
void join_command(char* lfpipe[], char* rgpipe[])
{
	int p[2];
	char* com1[10] = { NULL, };
	char* com2[10] = { NULL, };

	if (pipe(p) == -1) //파이프를 개방한다
		fatal("pipe call in join");

	switch (fork())
	{
	case -1:
		fatal("2nd fork call in join");  //오류
	case 0:  //손자(짜식의 자식)
		//쓰는 프로세스
		dup2(p[1], 1);  //표준 출력이 파이프로 가게 함 

		close(p[0]); //파일 기술자를 절약
		close(p[1]); 

		execvp(lfpipe[0], lfpipe); //첫번째 명령어(파이프의 왼쪽) 실행

		fatal("1st execvp call in join");  //execvp가 복귀하면 오류가 발생한 것
	default:   //자식(손자의 부모)
		//읽는 프로세스
		dup2(p[0], 0);  //표준 입력이 파이프로부터 오게 함

		close(p[0]);  //파일 기술자를 절약
		close(p[1]);

		separate(rgpipe, com1, com2);  //파이프가 2개이상인지 확인,추출 위해 또 호출
		if (com1[0] == NULL || com2[0] == NULL)  //파이프가 1개라면 com2[0]는 0일것이므로 조건충족
		{
			execvp(rgpipe[0], rgpipe);  //두번째 명령어(파이프의 오른쪽) 실행
			fatal("2nd execvp call in join");
		}
		else  //파이프가 2개이상이라면 파이프 결합 또 호출
			join_command(com1, com2);
	}
}


//오류 처리 함수
int fatal(char* s)
{
	perror(s);
	exit(1);
}


int main()
{
	sigex(0);  //myshell 자체가 인터럽트에 의해 종료되지않도록 함

	while (userin(prompt) != EOF)
		procline();
}

