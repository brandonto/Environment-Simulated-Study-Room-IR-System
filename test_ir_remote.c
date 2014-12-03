#include <lirc/lirc_client.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define PORT "80"
#define DELAY 1000

/*
 * Team Nile
 *
 * DESCRIPTION: This program demos the infrared capabilities of the raspberry pi
 *              using the LIRC library. (http://www.lirc.org/)
 *
 * ARGUMENTS: An optional file path can be given as an argument, if not present,
 *            the default path ~/etc/lirc/lircd.conf is used
 * 
 * LAST MODIFIED: December 2, 2014 @ 20:34 (UTC)
 *
 */

//Function prototypes for key handlers
char *handle_key_power(char *http_request);
char *handle_key_stop(char *http_request);
char *handle_key_pause(char *http_request);
char *handle_key_play(char *http_request);
char *handle_key_rewind(char *http_request);
char *handle_key_forward(char *http_request);

//Function prototypes for network code
int send_http_request(int sockfd, char *http_request);

//Function prototypes for misc code
char *concat(char *string1, char *string2);

//Main function
int main(int argc, char* argv[])
{
    //LIRC Variables
    struct lirc_config *config;

    //HTTP POST to send to web server
    char *host = "team-nile-test.webege.com";
    char *http_request = "POST /php/httptest.php HTTP/1.0\nHost: team-nile-test.webege.com\nContent-Type: application/x-www-form-urlencoded\nContent-Length: ";
    char *final_http_request = NULL;

    //Socket Variables
    int sockfd, numbytes;
    struct addrinfo hints, *addrinfo_list, *addrinfo;
    int error;

    //Timer Variables
    int msec = 0;
    clock_t start = clock(), diff;

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

    //Initializes hints and set appropriately
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    //
    if (error = getaddrinfo(host, PORT, &hints, &addrinfo_list) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    for (addrinfo = addrinfo_list; addrinfo != NULL; addrinfo = addrinfo->ai_next)
    {
        if ((sockfd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol)) == -1)
        {
            continue;
        }

        if (connect(sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen) == -1)
        {
            close(sockfd);
            continue;
        }

        break;
    }

    if (addrinfo == NULL)
    {
        fprintf(stderr, "failed to connect to %s\n", host);
        return -1;
    }

    fprintf(stdout, "connection to %s was successful\n", host);

    freeaddrinfo(addrinfo_list);

    //Read in configuration file from either argument or default path
    if (lirc_readconfig(argc==2 ? argv[1]:NULL, &config, NULL) == 0)
    {
        char *code; //Used to store the key code of the remote

        //Blocks the program until an IR signal is recieved from key press
        while (lirc_nextcode(&code)==0)
        {
            //Handle any logic corresponding to the key pressed, if any
            if (code==NULL) continue;
            {
                diff = clock() - start;
                msec = diff*1000/CLOCKS_PER_SEC;
                printf("%d\n", msec);
                if (msec > DELAY)
                {
                    if (strstr(code,"KEY_POWER"))
                    {
                        if ((final_http_request = handle_key_power(http_request)) == NULL) { return -1; }
                    }
                    else if (strstr(code,"KEY_STOP"))
                    {
                        if ((final_http_request = handle_key_stop(http_request)) == NULL) { return -1; }
                    }
                    else if (strstr(code,"KEY_PAUSE"))
                    {
                        if ((final_http_request = handle_key_pause(http_request)) == NULL) { return -1; }
                    }
                    else if (strstr(code,"KEY_PLAY"))
                    {
                        if ((final_http_request = handle_key_play(http_request)) == NULL) { return -1; }
                    }
                    else if (strstr(code,"KEY_REWIND"))
                    {
                        if ((final_http_request = handle_key_rewind(http_request)) == NULL) { return -1; }

                    }
                    else if (strstr(code,"KEY_FORWARD"))
                    {
                        if ((final_http_request = handle_key_forward(http_request)) == NULL) { return -1; }
                    }

                    if (final_http_request != NULL)
                    {
                        if (send_http_request(sockfd, final_http_request) == -1)
                        {
                            fprintf(stderr, "An error has occur when sending the HTTP request\n");
                        }
                        else
                        {
                            fprintf(stdout, "HTTP request has been sent successfully.\n%s", final_http_request);
                        }
                        free(final_http_request);
                        final_http_request = NULL;
                        //start = clock();
                    }
                }
            }
            free(code); //Free allocated memory for code after key press is handled
        }
        lirc_freeconfig(config); //Free allocated memory for configuration structure
    }

    lirc_deinit(); //Deinitializes LIRC, called at end of program execution
    return 0;
} //end of main()


//Handlers for key presses
//These will be replaced with real functions later on
char *handle_key_power(char *http_request)
{
    printf("POWER BUTTON PRESSED!\n");
    char *http_request_new = NULL;
    http_request_new = concat(http_request, "10\n\nfunction=1\n");
    return http_request_new;
}

char *handle_key_stop(char *http_request)
{
    printf("STOP BUTTON PRESSED!\n");
    char *http_request_new = NULL;
    http_request_new = concat(http_request, "10\n\nfunction=2\n");
    exit(EXIT_SUCCESS);
    return http_request_new;
}

char *handle_key_pause(char *http_request)
{
    printf("PAUSE BUTTON PRESSED!\n");
    char *http_request_new = NULL;
    http_request_new = concat(http_request, "10\n\nfunction=3\n");
    return http_request_new;
}

char *handle_key_play(char *http_request)
{
    printf("PLAY BUTTON PRESSED!\n");
    char *http_request_new = NULL;
    http_request_new = concat(http_request, "10\n\nfunction=4\n");
    return http_request_new;
}

char *handle_key_rewind(char *http_request)
{
    printf("REWIND BUTTON PRESSED!\n");
    char *http_request_new = NULL;
    http_request_new = concat(http_request, "10\n\nfunction=5\n");
    return http_request_new;
}

char *handle_key_forward(char *http_request)
{
    printf("FORWARD BUTTON PRESSED!\n");
    char *http_request_new = NULL;
    http_request_new = concat(http_request, "10\n\nfunction=6\n");
    return http_request_new;
}

int send_http_request(int sockfd, char *http_request)
{
    int bytes_sent = 0;
    int total_bytes_sent = 0;
    int http_request_length = strlen(http_request);
    int bytes_left = http_request_length;
    while (total_bytes_sent < http_request_length)
    {
        if ((bytes_sent = send(sockfd, http_request+total_bytes_sent, bytes_left, 0)) == -1)
        {
            break;
        }

        total_bytes_sent = total_bytes_sent + bytes_sent;
        bytes_left = bytes_left - bytes_sent;
    }

    return bytes_sent==-1 ? -1:0;
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
