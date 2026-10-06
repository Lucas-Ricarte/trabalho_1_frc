/* Cliente DNS (consultas MX via UDP) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "dns_types.h"
#include "dns_encode.h"
#include "dns_parse.h"

/* Espera até TIMEOUT_SEC por uma resposta com o ID esperado, ignorando as demais. */
static int aguardar_resposta(int sock, uint16_t query_id,
                             unsigned char *response, size_t size) {
    struct timeval agora, limite;
    gettimeofday(&limite, NULL);
    limite.tv_sec += TIMEOUT_SEC;

    for (;;) {
        gettimeofday(&agora, NULL);
        struct timeval restante;
        restante.tv_sec = limite.tv_sec - agora.tv_sec;
        restante.tv_usec = limite.tv_usec - agora.tv_usec;
        if (restante.tv_usec < 0) {
            restante.tv_sec--;
            restante.tv_usec += 1000000;
        }
        if (restante.tv_sec < 0 || (restante.tv_sec == 0 && restante.tv_usec == 0)) {
            return -1;
        }

        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &restante, sizeof(restante));

        ssize_t len = recvfrom(sock, response, size, 0, NULL, NULL);
        if (len < 0) {
            return -1;
        }

        if (len >= (ssize_t) sizeof(dns_header_t)) {
            const dns_header_t *hdr = (const dns_header_t *) response;
            if (ntohs(hdr->id) == query_id) {
                return (int) len;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <nome_dominio> <ip_servidor_dns>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *dominio = argv[1];
    const char *ip_servidor = argv[2];

    srand((unsigned int) time(NULL) ^ (unsigned int) getpid());

    unsigned char query[MAX_PACKET];
    uint16_t query_id;
    int query_len = build_query(dominio, query, &query_id);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DNS_PORT);
    if (inet_pton(AF_INET, ip_servidor, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Erro: IP de servidor invalido '%s'\n", ip_servidor);
        close(sock);
        return EXIT_FAILURE;
    }

    unsigned char response[MAX_PACKET];
    int recv_len = -1;
    int tentativa;

    for (tentativa = 1; tentativa <= MAX_TENTATIVAS && recv_len < 0; tentativa++) {
        ssize_t sent = sendto(sock, query, query_len, 0,
                               (struct sockaddr *) &server_addr, sizeof(server_addr));
        if (sent < 0) {
            perror("sendto");
            close(sock);
            return EXIT_FAILURE;
        }

        recv_len = aguardar_resposta(sock, query_id, response, sizeof(response));
    }

    close(sock);

    if (recv_len < 0) {
        printf("Nao foi possível coletar entrada MX para %s\n", dominio);
        return EXIT_FAILURE;
    }

    char exchange[MAX_NAME_LEN];
    resultado_t res = parse_response(response, recv_len, exchange, sizeof(exchange));

    switch (res) {
        case RES_OK:
            printf("%s <> %s\n", dominio, exchange);
            break;
        case RES_NXDOMAIN:
            printf("Dominio %s nao encontrado\n", dominio);
            break;
        case RES_SEM_MX:
            printf("Dominio %s nao possui entrada MX\n", dominio);
            break;
        case RES_ERRO_SERVIDOR:
        default:
            printf("Nao foi possível coletar entrada MX para %s\n", dominio);
            break;
    }

    return EXIT_SUCCESS;
}   