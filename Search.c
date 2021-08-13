// cd "c:\Users\Jayanta\Desktop\" && gcc search.c -lWs2_32 -o search && "c:\Users\Jayanta\Desktop\"search

#ifndef _WIN32
#define STYLISH_TEXT
#define CLEAR_SCREEN
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#define BUFFERSIZE 1024
#define ASCII_ESC 27
#define NON_PRINTABLE 15

////////////////////////////////////////////////////////////////////////////////////
// defines styles //

#ifdef STYLISH_TEXT
#define PROGRAM_HELP "★彡[ᴇɴᴛᴇʀ ᴋᴇʏᴡᴏʀᴅꜱ ᴛᴏ ꜱᴇᴀʀᴄʜ ᴏʀ ᴇꜱᴄ ᴛᴏ ᴇxɪᴛ]彡★"
#define SEARCH_PROMT "🔍 "
#define ABSTRACT_PREFIX " ➥ "
#define RESULT_PREFIX " ➢ "
#define NO_RESULT_FOUND "✗ “%s” - 𝙉𝙤 𝙧𝙚𝙨𝙪𝙡𝙩𝙨 𝙛𝙤𝙪𝙣𝙙‼"
#define PROGRAM_ERROR "⚠ "
#else
#define PROGRAM_HELP "[ENTER KEYWORDS TO SEARCH OR ESC TO EXIT]"
#define SEARCH_PROMT "Search: "
#define ABSTRACT_PREFIX "\t"
#define RESULT_PREFIX " > "
#define NO_RESULT_FOUND "* \"%s\" - No result found!!"
#define PROGRAM_ERROR "!! "
#endif

#define STYLE_UNDERLINE     "[4m"
#define STYLE_NO_UNDERLINE  "[24m"
#define STYLE_BOLD          "[1m"
#define STYLE_NO_BOLD       "[22m"

typedef enum { COLOR_BLACK=30, COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE } Color;

const char ESCAPE_CHARS[5][2][7] = {
    {"&lt;", "<"},
    {"&gt;", ">"},
    {"&amp;", "&"},
    {"&quot;", "\""},
    {"&apos;", "\'"}
};

////////////////////////////////////////////////////////////////////////////////////
// output style functions //

static inline void set_output_color(Color color) {printf("\033[0;%dm", color);}

static inline void set_output_bgcolor(Color color) {printf("\033[0;%dm", color+10);}

static inline void reset_output_colors() {printf("\033[0m");}

static inline void set_output_style(char* style) {printf("\033%s", style);}

static inline void reset_output_style() {set_output_style(STYLE_NO_BOLD); set_output_style(STYLE_NO_UNDERLINE);}

// clears screen
static inline void clear_screen()
{
#ifdef CLEAR_SCREEN
    printf( "%c[2J%c[H", ASCII_ESC, ASCII_ESC );
#endif
}

////////////////////////////////////////////////////////////////////////////////////
// string utility functions //

// converts UPPERCASE to LOWERCASE
static inline char to_lower_char(char c) {return (c >= 'A' && c <= 'Z') ? c + 32 : c;}

// returns pointer where needle found + size of needle
char *strstr_end(char *haystack, const char *needle)
{
    return (haystack = strstr(haystack, needle)) ? haystack + strlen(needle) : NULL;
}

// returns substring pointer insensitive to CASE
char *strcasestr(const char *haystack, const char *needle)
{
    /* Edge case: The empty string is a substring of everything. */
    if (!needle[0]) return (char *)haystack;

    /* Loop over all possible start positions. */
    for (size_t i = 0; haystack[i]; i++)
    {
        int matches = 1;
        /* See if the string matches here. */
        for (size_t j = 0; needle[j]; j++)
        {
            /* If we're out of room in the haystack, give up. */
            if (!haystack[i + j])
                return NULL;

            /* If there's a character mismatch, the needle doesn't fit here. */
            if (to_lower_char((unsigned char)needle[j]) !=
                to_lower_char((unsigned char)haystack[i + j]))
            {
                matches = 0;
                break;
            }
        }
        if (matches)
            return (char *)(haystack + i);
    }
    return NULL;
}

// same as strstr_end but CASE insensitive
char *strcasestr_end(char *haystack, const char *needle)
{
    return (haystack = strcasestr(haystack, needle)) ? haystack + strlen(needle) : NULL;
}

////////////////////////////////////////////////////////////////////////////////////
// error functions //

// terminates program on error
void program_error(const char *msg, const char *detail)
{
    clear_screen();
    set_output_color(COLOR_RED);
    set_output_style(STYLE_BOLD);
    printf("\n");
    printf(PROGRAM_ERROR);
    printf(" %s, ", msg);

    set_output_color(COLOR_YELLOW);
    set_output_style(STYLE_NO_BOLD);

    printf("%s\n\n", detail);
    exit(1);
}

////////////////////////////////////////////////////////////////////////////////////
// memory allocation function //

void alloc_memory(char** ptr, size_t size)
{
    *ptr = (char*) (*ptr ? realloc(*ptr, size) : malloc(size));
    if (!*ptr)   program_error("alloc_memory()", "Can't allocate Memory");
}

////////////////////////////////////////////////////////////////////////////////////
// socket functions //

// connects to the server socket
int socket_connect(const char *host, const char *service)
{
    int addrReturn, sock;
    struct addrinfo *servAddr;

    if ((addrReturn = getaddrinfo(host, service, NULL, &servAddr)) != 0)
        program_error("getaddrinfo() failed", gai_strerror(addrReturn));

    for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next)
    {
        if ((sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) < 0)
            continue; // Socket creation failed; try next address

        if (connect(sock, addr->ai_addr, addr->ai_addrlen) == 0)
            break; // Socket connection succeeded; break and return socket

        close(sock); // Socket connection failed; try next address
    }
    freeaddrinfo(servAddr); // Free addrinfo allocated in getaddrinfo()

    if (sock < 0) program_error("socket_connect() failed", "unable to connect");

    return sock;
}

////////////////////////////////////////////////////////////////////////////////////
// http functions //

// reads from socket one char at a time untill new line
int http_read_line(int socket, char *buffer, ssize_t size)
{
    int i, r;
    for (i = 0; i < size - 1; i++)
    {
        r = recv(socket, (buffer + i), 1, 0);
        // r=-1 when recv() fails and 0 when connection closes prematurely
        if (r < 1)  return r-1; // 0 is success so -1 makes -1 and -2 error values

        if (buffer[i] == '\n')
        {
            buffer[--i] = '\0';
            return i;
        }
    }
    // overflow
    buffer[i] = '\0';
    return size;
}

// writes to the socket
static inline int http_write(int socket, const char *string)
{
    return send(socket, string, strlen(string), 0);
}

// writes to the socket and appends new line
static inline int http_write_line(int socket)
{
    return http_write(socket, "\r\n");
}

// wrtites key and value pair to socket
int http_write_header(int socket, const char *key, const char *value)
{
    int written = 0;
    written += http_write(socket, key);
    written += http_write(socket, ": ");
    written += http_write(socket, value);
    written += http_write_line(socket);

    return written;
}

// reads from socket
ssize_t http_read_from_stream(int socket, char *buffer, ssize_t size)
{
    int bytes_read;
    ssize_t total_bytes_read = 0;
    while (total_bytes_read < size)
    {
        bytes_read = recv(socket, buffer, size - total_bytes_read, 0);

        if (bytes_read < 0)       program_error("http_read_from_stream()", "recv() failed");
        else if (bytes_read == 0) program_error("http_read_from_stream()", "connection closed prematurely");

        buffer += bytes_read;
        total_bytes_read += bytes_read;
    }
    return total_bytes_read;
}


int http_download_xml(char** dataPtr, int socket, const char *server, const char *path, const char *keyword)
{
    // HTTP Request
    http_write(socket, "GET ");
    http_write(socket, path);
    http_write(socket, keyword);
    http_write(socket, " HTTP/1.1");
    http_write_line(socket);
    http_write_header(socket, "Host", server);
    http_write_header(socket, "Connection", "Keep-Alive");
    // http_write_header(socket, "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
    // http_write_header(socket, "User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36 Edge/12.246");
    http_write_line(socket);

    static char buffer[BUFFERSIZE];
    int content_length = 0;

    // status line
    int status = http_read_line(socket, buffer, BUFFERSIZE);
    if(status < 1)  return status;

    // http_read_line() returns 0 when only \r\n
    // then follows body
    while (http_read_line(socket, buffer, BUFFERSIZE) > 1)
    {
        char *length = strcasestr_end(buffer, "Content-length: ");
        if (length) content_length = atoi(length);
    }

    char *data = NULL;

    if (content_length > 0)
    {
        // printf("\nDownloading %d bytes", content_length);
        alloc_memory(&data, content_length);
        http_read_from_stream(socket, data, content_length);
    }
    else
    { // chunked
        // size\r\n data\r\n terminate 0\r\n \r\n
        for(content_length = 0;;)
        {
            http_read_line(socket, buffer, BUFFERSIZE);
            size_t chunk_size = (size_t) strtol(buffer, NULL, 16); // size in hex

            if (chunk_size < 1) {
                http_read_line(socket, buffer, BUFFERSIZE); // contains an empty line
                break;
            }

            // printf("\nDownloading chunk %d bytes", chunk_size);

            alloc_memory(&data, content_length + chunk_size);

            http_read_from_stream(socket, (data + content_length), chunk_size);
            http_read_line(socket, buffer, BUFFERSIZE); // contains an empty line
            content_length += chunk_size;
        }
    }

    if(data) data[content_length - 1] = '\0'; // override \r\n

    *dataPtr = data;

    return content_length;
}

////////////////////////////////////////////////////////////////////////////////////
// xml crawling functions //

// replaces escape sequence eg: &amp; with quivalent char
void pop_escape_characters(char* string, char* end) {
    for(; string != end; ++string) {
        if(*string == '&') {
            for(int i=0; i<5; ++i) {
                int size = strlen(ESCAPE_CHARS[i][0]);
                if(strncmp(string, ESCAPE_CHARS[i][0], size) == 0) {
                    memset(string, NON_PRINTABLE, size);
                    *string = ESCAPE_CHARS[i][1][0];
                    string += size;
                    break;
                }
            }
        }
    }
}

// finds given token and prints its value with an optional prefix
int print_tag(char **data, char *tag, char *prefix)
{
    char *start = *data;
    char *end = NULL;

    while(1)
    {
        if (!(start = strstr_end(start, tag)))  return -1;
        if(start[0] == '>') {start++; break;}
    }

    if (!(end = strstr(start, "<")))    return -1;
    
    if(start != end)
    {
        pop_escape_characters(start, end);
        if (prefix) printf("%s", prefix);
        printf("%.*s\n", end - start, start);
    }

    *data = end + strlen(tag) + 3;
    return end - start;
}

int print_result(char* text)
{
    // marks end of search. after this there's related results
    char *text_end = strstr(text, "<RelatedTopicsSection");
    if(text_end) *text_end = '\0';

    printf("\n");
    set_output_color(COLOR_GREEN);
    set_output_style(STYLE_BOLD);

    if (!text || print_tag(&text, "Heading", SEARCH_PROMT) < 1)
        return 1;

    printf("\n");
    set_output_color(COLOR_CYAN);
    set_output_style(STYLE_NO_BOLD);

    if (print_tag(&text, "AbstractText", ABSTRACT_PREFIX) > 0)
        printf("\n");

    // skip to the result part
    set_output_color(COLOR_YELLOW);
    for(text = strstr_end(text, "<RelatedTopics>"); print_tag(&text, "Text", RESULT_PREFIX) > 0; printf("\n"));

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
// main function //

int main(int argc, char *argv[])
{
// required for windows
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa))
        program_error("WSAStartup() failed", "unable to init");
    
    // for utf8 chars in windows terminal
    SetConsoleOutputCP(65001);
#endif

    const char *server = "api.duckduckgo.com";
    const char *service = "http";
    const char *path = "/?format=xml&no_html=1&no_redirect=1&q=";

    char keyword[256];

    set_output_color(COLOR_GREEN);
    set_output_style(STYLE_BOLD);

    printf("\nConnecting...\n");

    // socket exception is handled within the function
    int socket = socket_connect(server, service);

    // print program help
    clear_screen();
    set_output_color(COLOR_CYAN);
    printf(PROGRAM_HELP); printf("\n\n");

    for(;;)
    {
        // promt user for input
        set_output_color(COLOR_GREEN);
        set_output_style(STYLE_BOLD);
        printf(SEARCH_PROMT);
        scanf(" %255[^\n]", keyword);

        // exit if user enters ESC or ~ button
        if(keyword[0] == ASCII_ESC || keyword[0] == '~')   break;

        char *data = NULL;

        // download xml data
        while (http_download_xml(&data, socket, server, path, keyword) < 0 || !data)
        {// connection closed try to reconnect
            printf("\nReconnecting...\n");
            close(socket);
            socket = socket_connect(server, service);
        }

        clear_screen();

        if (print_result(data))
        {// print no result found
            set_output_color(COLOR_RED);
            set_output_style(STYLE_BOLD);
            printf(NO_RESULT_FOUND, keyword); printf("\n\n");
        }

        if(data) free(data);
    }

    close(socket);

#ifdef _WIN32
    WSACleanup();
#endif

    exit(0);
}