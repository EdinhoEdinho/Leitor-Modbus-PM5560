#include <modbus/modbus.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_HOST_LEN 64
#define CURRENT_TAGS 6
#define VOLTAGE_TAGS 8

typedef struct {
    modbus_t *ctx;
    char host[MAX_HOST_LEN];
    int port;
    int unit_id;
} FloatModbusClient;

typedef struct {
    const char *name;
    int address;
    float value;
} TagFloat;

static float regs_to_float(uint16_t hi, uint16_t lo) {
    union {
        uint32_t u32;
        float f32;
    } conv;

    conv.u32 = ((uint32_t)hi << 16) | (uint32_t)lo;
    return conv.f32;
}

static int read_float_at(FloatModbusClient *client, int address, float *out) {
    uint16_t regs[2];
    int rc = modbus_read_registers(client->ctx, address, 2, regs);
    if (rc == -1) {
        return -1;
    }

    *out = regs_to_float(regs[0], regs[1]);
    return 0;
}

static int float_modbus_client_init(FloatModbusClient *client, const char *host, int port, int unit_id) {
    memset(client, 0, sizeof(*client));
    strncpy(client->host, host, MAX_HOST_LEN - 1);
    client->host[MAX_HOST_LEN - 1] = '\0';
    client->port = port;
    client->unit_id = unit_id;

    client->ctx = modbus_new_tcp(client->host, port);
    if (client->ctx == NULL) {
        return -1;
    }

    modbus_set_slave(client->ctx, unit_id);
    return 0;
}

static void float_modbus_client_destroy(FloatModbusClient *client) {
    if (client->ctx != NULL) {
        modbus_close(client->ctx);
        modbus_free(client->ctx);
        client->ctx = NULL;
    }
}

static int client_connect(FloatModbusClient *client) {
    return modbus_connect(client->ctx);
}

static void client_disconnect(FloatModbusClient *client) {
    if (client->ctx != NULL) {
        modbus_close(client->ctx);
    }
}

static void print_now(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char buffer[64];

    if (tm_info != NULL && strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info) > 0) {
        printf("%s", buffer);
    } else {
        printf("timestamp indisponivel");
    }
}

static void read_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    fflush(stdout);

    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    buffer[strcspn(buffer, "\r\n")] = '\0';
}

static int read_int(const char *prompt) {
    char buffer[64];
    char *endptr = NULL;
    long value;

    while (1) {
        read_line(prompt, buffer, sizeof(buffer));
        value = strtol(buffer, &endptr, 10);
        if (endptr != buffer && *endptr == '\0') {
            return (int)value;
        }
        printf("Entrada invalida. Tente novamente.\n");
    }
}

static int read_int_default(const char *prompt, int default_value) {
    char buffer[64];
    char *endptr = NULL;
    long value;

    while (1) {
        printf("%s", prompt);
        fflush(stdout);

        if (fgets(buffer, (int)sizeof(buffer), stdin) == NULL) {
            return default_value;
        }

        buffer[strcspn(buffer, "\r\n")] = '\0';
        if (buffer[0] == '\0') {
            return default_value;
        }

        value = strtol(buffer, &endptr, 10);
        if (endptr != buffer && *endptr == '\0') {
            return (int)value;
        }

        printf("Entrada invalida. Tente novamente.\n");
    }
}

static void fill_tag_address_list(const char *title, const char *labels[], int count, TagFloat tags[]) {
    printf("\n%s\n", title);
    for (int i = 0; i < count; i++) {
        char prompt[96];
        snprintf(prompt, sizeof(prompt), "Endereco da tag %s: ", labels[i]);
        tags[i].name = labels[i];
        tags[i].address = read_int(prompt);
        tags[i].value = 0.0f;
    }
}

static int leitura_registros(FloatModbusClient *client, TagFloat currents[], TagFloat voltages[]) {
    printf("\n############################################\n");
    printf("Leitura iniciada em: ");
    print_now();
    printf("\n");
    printf("IP do medidor: %s\n", client->host);
    printf("No do medidor: %d\n", client->unit_id);
    printf("Modelo: PM5560 - Schneider Electric\n");
    printf("############################################\n\n");

    printf("Lendo correntes (A)...\n");
    for (int i = 0; i < CURRENT_TAGS; i++) {
        if (read_float_at(client, currents[i].address, &currents[i].value) != 0) {
            fprintf(stderr, "ERRO na leitura da tag %s (endereco %d): %s\n",
                    currents[i].name,
                    currents[i].address,
                    modbus_strerror(errno));
            return -1;
        }
    }

    printf("Correntes lidas com sucesso:\n");
    for (int i = 0; i < CURRENT_TAGS; i++) {
        printf("  %-5s = %.3f A\n", currents[i].name, currents[i].value);
    }

    printf("\nLendo tensoes (V)...\n");
    for (int i = 0; i < VOLTAGE_TAGS; i++) {
        if (read_float_at(client, voltages[i].address, &voltages[i].value) != 0) {
            fprintf(stderr, "ERRO na leitura da tag %s (endereco %d): %s\n",
                    voltages[i].name,
                    voltages[i].address,
                    modbus_strerror(errno));
            return -1;
        }
    }

    printf("Tensoes lidas com sucesso:\n");
    for (int i = 0; i < VOLTAGE_TAGS; i++) {
        printf("  %-5s = %.1f V\n", voltages[i].name, voltages[i].value);
    }

    printf("\nLeitura concluida com sucesso.\n");
    printf("############################################\n\n");
    return 0;
}

int main(void) {
    FloatModbusClient client;
    TagFloat currents[CURRENT_TAGS];
    TagFloat voltages[VOLTAGE_TAGS];
    const char *current_labels[CURRENT_TAGS] = { "IA", "IB", "IC", "IN", "IG", "Iavg" };
    const char *voltage_labels[VOLTAGE_TAGS] = { "Vab", "Vbc", "Vca", "VllAvg", "Van", "Vbn", "Vcn", "VlnAvg" };
    char host_input[MAX_HOST_LEN];
    int port;
    int unit_id;

    read_line("Digite o IP do medidor: ", host_input, sizeof(host_input));
    port = read_int_default("Digite a porta Modbus [502]: ", 502);
    unit_id = read_int_default("Digite o unit_id do medidor [6]: ", 6);

    fill_tag_address_list("Informe os enderecos das correntes:", current_labels, CURRENT_TAGS, currents);
    fill_tag_address_list("Informe os enderecos das tensoes:", voltage_labels, VOLTAGE_TAGS, voltages);

    if (float_modbus_client_init(&client, host_input, port, unit_id) != 0) {
        fprintf(stderr, "Falha ao criar contexto Modbus.\n");
        return EXIT_FAILURE;
    }

    if (client_connect(&client) != 0) {
        fprintf(stderr, "Falha ao conectar: %s\n", modbus_strerror(errno));
        float_modbus_client_destroy(&client);
        return EXIT_FAILURE;
    }

    if (leitura_registros(&client, currents, voltages) != 0) {
        client_disconnect(&client);
        float_modbus_client_destroy(&client);
        return EXIT_FAILURE;
    }

    client_disconnect(&client);
    float_modbus_client_destroy(&client);
    return EXIT_SUCCESS;
}
