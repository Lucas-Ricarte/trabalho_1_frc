#ifndef DNS_CLIENT_H
#define DNS_CLIENT_H

#include <stddef.h>

#include "dns_types.h"

resultado_t consultar_mx(const char *dominio, const char *ip_servidor,
                         char *servidor_email, size_t capacidade);

#endif
