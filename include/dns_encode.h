#ifndef DNS_ENCODE_H
#define DNS_ENCODE_H

#include "dns_types.h"

int montar_consulta(const char *dominio, uint16_t id, unsigned char *consulta);

#endif
