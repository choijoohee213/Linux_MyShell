# Linux_MyShell
System Programming - My own Shell Program made using various system calls in Linux/Unix


***


<strong>MyShell 의 기능</strong>
1) stdin command parsing 및 fork/exec 를 이용한 command 실행.
2) background process 제공 ( & ). 즉, command 뒤에 &를 붙이면, background 로 실행되어야 한다. 예는 prompt> cmd 1 & prompt> cmd1 | cmd2 
3) signal 처리. terminal 에서 발생하는 SIGINT, SIGQUIT 에 대한 처리
기능 
4) pipe 지원 ( | ). 예는 prompt> cmd1 | cmd2 | cmd3

프로그램을 구동하면 현재 working directory 를 prompt 로 설정하고 사용자의 입력을 기다린다. 사용자의 입력에 따라 각 명령을 처리하고 다시 현재 working directory를 prompt 로 설정하여 사용자 입력을 대기한다. 다음의 명령 중 logout 명령을 받을 때까지 이를 계속 반복한다.

1. command 실행 시작과 동시에 바로 prompt 로 return 되어 다른 command 를 입력받아 실행할 수 있도록 해야 한다.
2. myshell 자체는 해당 signal 를 받아도 종료되어서는 안 되며 두 signal 를 받으면 prompt 를 출력하고 즉 입력 대기상태로 가야 한다.
- signal 를 받을 당시 실행되고 있는 foreground processes 들은 종료
- signal 를 받을 당시 실행되고 있는 background processes 들은 종료 안 됨
3.  pipe 는 최소 2 개 지원해야 함. 즉, com1 | com2 | com3


***



<strong>MyShell features</strong>
1) stdin command parsing and command execution using fork/exec.
2) Provide background process (& ). In other words, if you add & after command, it must be executed in background. Examples are prompt> cmd 1 & prompt> cmd1 | cmd2
3) signal processing. Handling SIGINT and SIGQUIT occurring in terminal
function
4) Pipe support (| ). For example, prompt> cmd1 | cmd2 | cmd3

When you run the program, it sets the current working directory to prompt and waits for user input. Each command is processed according to the user's input, and the current working directory is set to prompt again to wait for user input. This is repeated until the logout command is received among the following commands.

1. As soon as the command execution starts, it returns to the prompt immediately so that other commands can be input and executed.
2. Myshell itself should not be terminated even if it receives the corresponding signal, and when it receives both signals, it displays a prompt and goes to the input waiting state.
-Foreground processes running at the time signal is received are terminated
-Background processes running at the time signal is received are not terminated
3. At least 2 pipes must be supported. That is, com1 | com2 | com3
