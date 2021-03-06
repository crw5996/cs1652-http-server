#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

int read_bytes(int socket, char* buffer, int buflen);
void detect_end_headers(int);

/**
  * Usage: 
  *	http_client k host port file
  **/
int main(int argc, char* argv[]) {

	char* get_str = "GET %s HTTP/1.0\n\n";

	int server_port = -1;
	char* server_name = NULL; 	// host
	char* server_path = NULL; 	// file
	char* read_buffer = NULL;
	int socket_fd;			// socket file descriptor

	struct sockaddr_in sa;
	struct hostent* site = NULL;

	if (argc != 5) {
		fprintf(stderr, "Usage Error: http_client k host port file");
		return -1;
	}

	if (toupper(*argv[1]) == 'K') {

	} else if (toupper(*argv[1]) == 'U') {

	} else {
		exit(-1);
	}

	server_name = argv[2];
	server_port = atoi(argv[3]);
	server_path = argv[4];

	/**
	  * First Arg: 
	  *	AF_INET: Address Family 
	  **/
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		fprintf(stderr, "If this hits something is horribly wrong\n");
		return -1;
	}

	// Do DNS lookup
	site = gethostbyname(server_name);

	if (site == NULL) {
		fprintf(stderr, "Unable to Find Server...Invalid\n");
		close(socket_fd);
		exit(-1);
	}
	
	memset(&sa, 0, sizeof(sa));
	sa.sin_port = htons(server_port);	// extends server_port to 16 bits
	memcpy(&sa.sin_addr, site -> h_addr_list[0], site -> h_length);
	sa.sin_family = AF_INET;
	
	/**
	  * Connect to Socket
	  **/
	if (connect(socket_fd, (struct sockaddr*) &sa, sizeof(sa)) < 0) {
		fprintf(stderr, "Error Connecting To Socket");
		close(socket_fd);
		return -1;
	}
	
	// this is slightly bigger than necessary due to %s character which are subbed out. Idk if it matters
	char* get_request = (char*) malloc((strlen(get_str) + strlen(server_path)) * sizeof(char));	

	if(sprintf(get_request, get_str, server_path) < 0) {
		fprintf(stderr, "Error Formatting File Path");
		close(socket_fd);
		free(get_request);
		return -1;
	}
	
	/**
	  * Send data to socket
	  **/
	if (send(socket_fd, get_request, strlen(get_request), 0) < 0) {
		fprintf(stderr, "Error Sending To Socket");
		close(socket_fd);
		free(get_request);
		return -1;
	}
	
	read_buffer = (char*) malloc(sizeof(char) * 100);

	int read_error = read_bytes(socket_fd, read_buffer, sizeof(char) * 100);
	
	close(socket_fd);

	free(read_buffer);
	free(get_request);
	return read_error;
}

int read_bytes(int socket, char* buffer, int buff_len) {
	FILE* file = NULL;
	int x = 1;
	char* state_flag;		// If this == NULL we got anything other than a 200 response
	char header_string[14]; 	// this is going to hold http header
	char temp_header[15];

	file = fdopen(socket, "r");
	
	if(file == NULL) {
		fprintf(stderr, "Error Opening Socket For Reading");
		return -1;
	}
	
	read(socket, temp_header, 15);

	state_flag = (char*) malloc(sizeof(char) * 10);	 // Size of "200 OK" + null term
	state_flag = strstr(temp_header, "200 OK");
	
	detect_end_headers(socket);
	

	/**
	  * to parse out http headers 
	  * split against "\r\n\r\n"
	  * after this the message should start
	  **/
	while(x != 0) {
		
		x = read(socket, buffer, buff_len);
		
		if(x < 0 || state_flag == NULL) {
			fprintf(stderr, "%s", buffer);
		} else {
			printf("%s", buffer);
		}

		memset(buffer, 0, buff_len);

	}
	
	fclose(file);

	if (x < 0 || state_flag == NULL) {
		free(state_flag);
		return -1;
	}
	return 0;
}

void detect_end_headers(int socket) {
	
	int flag = 0;
	char current = ' ';

	while(read(socket, & current, 1) > 0) {

		if (current == '\r' && flag == 0) {
			flag |= 1;
		} else if(current == '\n' && flag == 1) {
			flag |= 2;
		} else if(current == '\r' && flag == 3) {
			flag |= 4;
		} else if (current == '\n' && flag == 7) {
			return;
		} else {
			flag &= 0;
		}

	}

	/**
	  * Since HTTP Standard says headers have to end with \r\n\r\n
	  * it's unlikely this will hit. But if it does, we screwed up
	  **/
	fprintf(stderr, "Invalid HTTP Protocol");
	exit(-1);
}
