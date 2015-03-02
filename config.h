#ifndef ARSC_CONFIG_H
#define ARSC_CONFIG_H

#define CONFIG_LEN 1024

struct arsc_config;

void config_to_string(const struct arsc_config *config, char buf[CONFIG_LEN]);

#endif
