/*
 * 72-asiakas-palvelin (asiakas)
 *
 * Tämä ohjelma toimii asiakasohjelmana UDP-pohjaisessa asiakas-palvelin-ohjelmassa.
 * Asiakas lähettää palvelimelle viestin ja odottaa palvelimelta vastauksena seitsemän satunnaista lottonumeroa, jotka palvelin arpoo.
 * Asiakas tulostaa lopuksi palvelimelta saadut lottonumerot ja lopettaa.
 *
 * Ohjelman kääntäminen ja ajaminen:
 *  1. make
 *  2. ./palvelin
 *  3. ./asiakas
 *  4. make clean (lopuksi käännettyjen ohjelmien poistamiseen)
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 6000         // Portti, jota palvelin kuuntelee
#define BUFFER_SIZE 1024  // Maksimipituus viestille

int main() {
    int udp_socket;                            // Socketin tiedot
    struct sockaddr_in server_addr;            // Palvelimen osoiterakenne
    char buffer[BUFFER_SIZE];                  // Puskuri palvelimelta tuleville tiedoille
    char *message = "Anna loton voittorivi!";  // Lähetettävä viesti

    // Luo UDP-socket: AF_INET (IPv4), SOCK_DGRAM tyyppi (tuki datagram paketeille), IPPROTO_UDP = UDP protokolla
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {  // Palauttaa -1, jos virhe
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Nollaa palvelimen osoiterakenne bzero-funktiolla muistista
    bzero(&server_addr, sizeof(server_addr));

    // Määritä palvelimen osoiterakenne
    server_addr.sin_family = AF_INET;                      // IPv4
    server_addr.sin_port = htons(PORT);                    // Portti (muutetaan lyhyt integer verkon tavujärjestykseen (16bit) htons-funktiolla)
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // muuta IP-osoite (localhost) verkkojärjestykseen inet_addr-funktiolla

    // Lähetä viesti palvelimelle sendto-funktiolla (viesti, viestin pituus, 0 (ei lippuja), palvelimen osoiterakenne, osoiterakenteen koko)
    // UDP:ssä ei tarvita erillistä yhteyden muodostamista, joten ei tarvitse kutsua connect-funktiota
    if (sendto(udp_socket, message, strlen(message), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to send message");
        close(udp_socket);  // Sulje socket
        exit(EXIT_FAILURE);
    }

    printf("Lähetettiin viesti palvelimelle: %s\n", message);

    // Vastaanota palvelimen vastaus recvfrom-funktiolla
    // Parametreina socket, puskuri, puskurin maksimipituus, 0 (ei lippuja), NULL (ei tallenneta lähettävää osoitetta), NULL (ei tallenneta osoitteen kokoa)
    int len = recvfrom(udp_socket, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)NULL, NULL);
    if (len < 0) {
        perror("Failed to receive response");
        close(udp_socket);  // Sulje socket
        exit(EXIT_FAILURE);
    }
    buffer[len] = '\0';

    printf("  -> Saatiin vastaus palvelimelta: %s\n", buffer);

    // Sulje socket
    close(udp_socket);
    return EXIT_SUCCESS;
}
