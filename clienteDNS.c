/*
 * Cliente DNS - Trabalho 01 (Fundamentos de Redes de Computadores)
 * ------------------------------------------------------------------
 * Realiza consultas DNS do tipo MX (mail exchanger) montando o payload
 * UDP manualmente, conforme RFC 1034/1035, sem uso de bibliotecas de
 * resolução de nomes (getaddrinfo, gethostbyname, resolv.h, etc).
 *
 * Uso:
 *   ./dns_client <nome_dominio> <ip_servidor_dns>
 *
 * Exemplo:
 *   ./dns_client unb.br 8.8.8.8
 */

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

/* ------------------------------------------------------------------ */
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <nome_dominio> <ip_servidor_dns>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *dominio = argv[1];
    const char *ip_servidor = argv[2];

    srand((unsigned int) time(NULL) ^ (unsigned int) getpid());

    /* Monta o pacote de consulta */
    unsigned char query[MAX_PACKET];
    uint16_t query_id;
    int query_len = build_query(dominio, query, &query_id);

    /* Cria o socket UDP */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

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

    for (tentativa = 1; tentativa <= MAX_TENTATIVAS; tentativa++) {
        ssize_t sent = sendto(sock, query, query_len, 0,
                               (struct sockaddr *) &server_addr, sizeof(server_addr));
        if (sent < 0) {
            perror("sendto");
            close(sock);
            return EXIT_FAILURE;
        }

        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        recv_len = recvfrom(sock, response, sizeof(response), 0,
                             (struct sockaddr *) &from_addr, &from_len);

        if (recv_len < 0) {
            /* timeout (EAGAIN/EWOULDBLOCK) -> tenta novamente */
            continue;
        }

        /* Confere se a resposta corresponde ao Transaction ID enviado */
        dns_header_t *resp_hdr = (dns_header_t *) response;
        if (ntohs(resp_hdr->id) != query_id) {
            /* pacote de outra transacao; ignora e tenta de novo */
            recv_len = -1;
            continue;
        }

        break; /* resposta valida recebida */
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