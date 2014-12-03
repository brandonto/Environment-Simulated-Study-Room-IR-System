#include <lirc/lirc_client.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define PORT "80"
#define DELAY 1000
#define MAX_HTTP_REQUEST_LEN 1024
#define MAX_HTTP_RESPONSE_LEN 4096

/*
 * Team Nile
 *
 * DESCRIPTION: This program demos the infrared capabilities of the raspberry pi
 *              using the LIRC library. (http://www.lirc.org/)
 *
 * ARGUMENTS: An optional file path can be given as an argument, if not present,
 *            the default path ~/etc/lirc/lircd.conf is used
 * 
 * LAST MODIFIED: December 3, 2014 @ 21:38 (UTC)
 *
 */

//Function prototypes for key handlers
int handle_key_power(char *http_request, char *http_request_format);
int handle_key_stop(char *http_request, char *http_request_format);
int handle_key_pause(char *http_request, char *http_request_format);
int handle_key_play(char *http_request, char *http_request_format);
int handle_key_rewind(char *http_request, char *http_request_format);
int handle_key_forward(char *http_request, char *http_request_format);

//Function prototypes for network code
int send_http_request(int sockfd, char *http_request);
int receive_http_response(int sockfd, char *http_response);

//Function prototypes for misc code
char *concat(char *string1, char *string2);

//Main function
int main(int argc, char* argv[])
{
    //LIRC Variables
    struct lirc_config *config;

    //HTTP Variables
    char *host = "team-nile-test.webege.com";
    char *http_request_format = "POST /php/httptest.php HTTP/1.1\n"
                                "Host: team-nile-test.webege.com\n"
                                "Content-Type: application/x-www-form-urlencoded\n"
                                "Content-Length: 10\n\n"
                                "function=%s\n";
    char http_request[MAX_HTTP_REQUEST_LEN];
    char http_response[MAX_HTTP_RESPONSE_LEN];

    //Socket Variables
    int sockfd, error;
    struct addrinfo hints, *addrinfo_list, *addrinfo;

    //Misc Variables
    int valid_key = 0;

    //Outputs usage and exits if given more than one file path in arguments
    if (argc>2)
    {
        fprintf(stderr, "Usage: %s <config file>\n", argv[0]);
        return -1;
    }

    //Initializes LIRC and exits if unsuccessful, called at start of execution
    if (lirc_init("lirc",1)==-1)
    {
        fprintf(stderr, "lirc_init: failed to initialize lirc\n");
        return -1;
    }

    //Clears hints structure and set appropriately for call to getaddrinfo()
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    //Gets the linked list of possible addrinfo structures
    if (error = getaddrinfo(host, PORT, &hints, &addrinfo_list) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    //Iterate through linked list of possible addrinfo structures and attempts to create socket to connect to
    for (addrinfo = addrinfo_list; addrinfo != NULL; addrinfo = addrinfo->ai_next)
    {
        //Creates a socket
        if ((sockfd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol)) == -1)
        {
            continue;
        }

        //Connect to socket
        if (connect(sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen) == -1)
        {
            close(sockfd);
            continue;
        }

        //Break out of loop if calls to socket() and connect() are both successful
        break;
    }

    //If none of the addrinfo structures in the linked list works, then exit program
    if (addrinfo == NULL)
    {
        fprintf(stderr, "failed to connect to %s\n", host);
        return -1;
    }

    fprintf(stdout, "connection to %s was successful\n", host);

    close(sockfd);

    //Deallocate memory from linked list of addrinfo structures
    //freeaddrinfo(addrinfo_list);

    //Read in configuration file from either argument or default path
    if (lirc_readconfig(argc==2 ? argv[1]:NULL, &config, NULL) == 0)
    {
        char *code; //Used to store the key code of the remote

        //Blocks the program until an IR signal is recieved from key press
        while (lirc_nextcode(&code)==0)
        {
            //Handle logic key press, if any
            if (code==NULL) continue;
            {
                //Clears request and response buffer
                memset(&http_request, 0, sizeof http_request);
                memset(&http_response, 0, sizeof http_response);

                if (strstr(code,"KEY_POWER"))
                {
                    if ((valid_key = handle_key_power(http_request, http_request_format)) == -1) { return -1; }
                }
                else if (strstr(code,"KEY_STOP"))
                {
                    if ((valid_key = handle_key_stop(http_request, http_request_format)) == -1) { return -1; }
                }
                else if (strstr(code,"KEY_PAUSE"))
                {
                    if ((valid_key = handle_key_pause(http_request, http_request_format)) == -1) { return -1; }
                }
                else if (strstr(code,"KEY_PLAY"))
                {
                    if ((valid_key = handle_key_play(http_request, http_request_format)) == -1) { return -1; }
                }
                else if (strstr(code,"KEY_REWIND"))
                {
                    if ((valid_key = handle_key_rewind(http_request, http_request_format)) == -1) { return -1; }
                }
                else if (strstr(code,"KEY_FORWARD"))
                {
                    if ((valid_key = handle_key_forward(http_request, http_request_format)) == -1) { return -1; }
                }

                if (valid_key)
                {
                    //Creates socket
                    if ((sockfd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol)) == -1)
                    {
                        return -1;
                    }

                    //Connect to socket
                    if (connect(sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen) == -1)
                    {
                        close(sockfd);
                        return -1;
                    }

                    if (send_http_request(sockfd, http_request) == -1)
                    {
                        fprintf(stderr, "An error has occur while sending the HTTP request\n");
                    }
                    else
                    {
                        fprintf(stdout, "HTTP request has been sent successfully.\n--------------------------------------------------\n%s\n\n", http_request);
                    }

                    if (receive_http_response(sockfd, http_response) == -1)
                    {
                        fprintf(stderr, "An error has occur while receiving the HTTP response\n");
                    }
                    else
                    {
                        fprintf(stdout, "HTTP response has been received successfully.\n--------------------------------------------------\n%s\n\n", http_response);
                    }

                    close(sockfd);
                }
            }
            free(code); //Free allocated memory for code after key press is handled
        }
        lirc_freeconfig(config); //Free allocated memory for configuration structure
    }

    lirc_deinit(); //Deinitializes LIRC, called at end of program execution
    return 0;
} //end of main()


//Key press handlers
//Constructs the http_request string to be sent
int handle_key_power(char *http_request, char *http_request_format)
{
    printf("POWER BUTTON PRESSED!\n");
    sprintf(http_request, http_request_format, "1");
    return 1;
}

int handle_key_stop(char *http_request, char *http_request_format)
{
    printf("STOP BUTTON PRESSED!\n");
    sprintf(http_request, http_request_format, "2");
    return 1;
}

int handle_key_pause(char *http_request, char *http_request_format)
{
    printf("PAUSE BUTTON PRESSED!\n");
    sprintf(http_request, http_request_format, "3");
    return 1;
}

int handle_key_play(char *http_request, char *http_request_format)
{
    printf("PLAY BUTTON PRESSED!\n");
    sprintf(http_request, http_request_format, "4");
    return 1;
}

int handle_key_rewind(char *http_request, char *http_request_format)
{
    printf("REWIND BUTTON PRESSED!\n");
    sprintf(http_request, http_request_format, "5");
    return 1;
}

int handle_key_forward(char *http_request, char *http_request_format)
{
    printf("FORWARD BUTTON PRESSED!\n");
    sprintf(http_request, http_request_format, "6");
    return 1;
}

int send_http_request(int sockfd, char *http_request)
{
    int http_request_length = strlen(http_request);
    int bytes_sent = 0;
    int total_bytes_sent = 0;
    do
    {
        if ((bytes_sent = send(sockfd, http_request+total_bytes_sent, http_request_length-total_bytes_sent, 0)) == -1)
        {
            break;
        }

        total_bytes_sent = total_bytes_sent + bytes_sent;
    } while (total_bytes_sent < http_request_length);

    return bytes_sent==-1 ? -1:0;
}

int receive_http_response(int sockfd, char *http_response)
{
    int http_response_length = MAX_HTTP_RESPONSE_LEN;
    int bytes_received = 0;
    int total_bytes_received = 0;
    do
    {
        if ((bytes_received = recv(sockfd, http_response+total_bytes_received, http_response_length-total_bytes_received, 0)) <= 0)
        {
            break;
        }

        total_bytes_received = total_bytes_received + bytes_received;
    } while (total_bytes_received < http_response_length);

    return bytes_received==-1 ? -1:0;
}

char *concat(char *string1, char *string2)
{
    char *result = malloc(strlen(string1)+strlen(string2)+1);
    if (result == NULL)
    {
        exit(1);
    }
    strcpy(result, string1);
    strcat(result, string2);
    return result;
}
