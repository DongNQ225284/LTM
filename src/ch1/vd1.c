#include <stdio.h>
#include <stdint.h>

int main() {
    uint8_t packet_bytes[] = {
        0x45, 0x00, 0x00, 0x40,
        0x7c, 0xda, 0x40, 0x00,
        0x80, 0x06, 0xfa, 0xd8,
        0xc0, 0xa8, 0x0f, 0x0b,
        0xbc, 0xac, 0xf6, 0xa4
    };

    uint8_t version = packet_bytes[0] >> 4;
    uint8_t ihl = packet_bytes[0] & 0x0F;

    uint16_t total_length =
        (packet_bytes[2] << 8) | packet_bytes[3];

    uint8_t *src_ip = &packet_bytes[12];

    uint8_t *dst_ip = &packet_bytes[16];

    printf("Version: %u\n", version);
    printf("IHL: %u (%u bytes)\n", ihl, ihl * 4);
    printf("Total Length: %u bytes\n", total_length);

    printf("Source IP: %u.%u.%u.%u\n",
           src_ip[0], src_ip[1],
           src_ip[2], src_ip[3]);

    printf("Destination IP: %u.%u.%u.%u\n",
           dst_ip[0], dst_ip[1],
           dst_ip[2], dst_ip[3]);

    return 0;
}