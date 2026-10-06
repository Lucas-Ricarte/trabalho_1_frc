/* Interface da comunicacao com servidor DNS por UDP. */

#ifndef DNS_CLIENT_H
#define DNS_CLIENT_H

#include <stddef.h>

#include "dns_types.h"

/* Consulta um registro MX e escreve o exchange no buffer de saida. */
resultado_t dns_query(const char *domain, const char *server_ip,
                      char *exchange, size_t exchange_size);

#endif /* DNS_CLIENT_H */