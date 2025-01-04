/*
 * 72-asiakas-palvelin (palvelin)
 *
 * Tämä on UDP-pohjainen asiakas-palvelin-ohjelma, jossa tässä toteutettu palvelin vastaanottaa
 * asiakkaalta paketin ja vastaa siihen satunnaisesti arvotuilla lottonumeroilla.
 * Asiakas lähettää palvelimelle viestin ja palvelin lähettää takaisin seitsemän arvottua lottonumeroa.
 * Palvelin kuuntelee saapuvia UDP-paketteja portissa 6000.
 *
 * Ohjelman kääntäminen ja ajaminen:
 *  1. make
 *  2. ./palvelin
 *  3. ./asiakas
 *  4. make clean (lopuksi käännettyjen ohjelmien poistamiseen)
 */

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PORT 6000         // Portti, jota palvelin kuuntelee
#define BUFFER_SIZE 1024  // Maksimipituus viestille

// Arpoo 7 uniikkia lottonumeroa
void draw_lottery_numbers(int *numbers) {
    int i, num;
    bool unique;

    // Arvo 7 uniikkia lottonumeroa
    for (i = 0; i < 7; i++) {
        do {
            // Arvo satunnainen numero väliltä 1-40
            num = rand() % 40 + 1;

            // Tarkista, ettei numero ole jo arvottu
            unique = true;
            for (int j = 0; j < i; j++) {
                if (numbers[j] == num) {
                    unique = false;  // Numero löytyy taulukosta, ei uniikki
                    break;
                }
            }
        } while (!unique);  // Jatketaan, kunnes saadaan uniikki numero

        // Lisää uniikki numero taulukkoon
        numbers[i] = num;
    }
}

// Muuttaa lottonumerot merkkijonoksi ja lisää ne annettuun merkkijonoon
void lottery_numbers_to_string(int *numbers, char *buffer) {
    for (int i = 0; i < 7; i++) {
        char num[5];
        // Lisää pilkku numeron perään, paitsi viimeiseen numeroon
        if (i == 6) {
            sprintf(num, "%d", numbers[i]);
        } else {
            sprintf(num, "%d, ", numbers[i]);
        }
        strcat(buffer, num);  // Lisää numerot annettuun merkkijonoon
    }
}

int main() {
    int udp_socket;                               // Socketin tiedot
    struct sockaddr_in server_addr, client_addr;  // Palvelimen ja asiakkaan osoiterakenteet
    socklen_t client_len = sizeof(client_addr);   // Asiakkaan osoiterakenteen koko
    char buffer[BUFFER_SIZE];                     // Puskuri palvelimelle tuleville viesteille
    int numbers[7];                               // Lottonumerotaulukko

    // Nollaa palvelimen osoiterakenne bzero-funktiolla muistista
    bzero(&server_addr, sizeof(server_addr));

    // Luo UDP-socket: AF_INET (IPv4), SOCK_DGRAM tyyppi (tuki datagram paketeille), IPPROTO_UDP = UDP protokolla
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {  // Palauttaa -1, jos virhe
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Määritä palvelimen osoiterakenne
    server_addr.sin_family = AF_INET;                 // IPv4
    server_addr.sin_port = htons(PORT);               // Portti (muutetaan lyhyt integer verkon tavujärjestykseen (16bit) htons-funktiolla)
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // Hyväksy yhteydet kaikista osoitteista (INADDR_ANY) muutetaan integerit verkon tavujärjestykseen (32bit) htonl-funktiolla

    // Liitä palvelimen osoiterakenne sockettiin, parametreina socket, osoiterakenne ja osoiterakenteen koko
    if (bind(udp_socket, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        close(udp_socket);  // Sulje socket
        exit(EXIT_FAILURE);
    }

    printf("Palvelin on käynnissä ja kuuntelee portissa %d...\n", PORT);

    // Kuuntele silmukassa palvelimelle saapuvia viestejä ja vastaa niihin
    while (true) {
        // Vastaanota viesti asiakkaalta recvfrom-funktiolla, parametreina socket, puskuri, puskurin koko, liput 0 (ei lippuja), asiakkaan osoiterakenne ja osoiterakenteen koko
        int len = recvfrom(udp_socket, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_addr, &client_len);
        if (len < 0) {
            perror("Failed to receive message");
            continue;  // Siirrytään odottamaan seuraavaa viestiä
        }

        buffer[len] = '\0';
        printf("Vastaanotettiin viesti asiakkaalta: %s\n", buffer);

        // Arvo lottonumerot
        draw_lottery_numbers(numbers);

        // Muodosta vastaus lottonumeroista asiakkaalle
        char response[BUFFER_SIZE] = "Lottonumeronne ovat ";
        lottery_numbers_to_string(numbers, response);

        // Lähetä vastaus asiakkaalle sendto-funktiolla, parametreina socket, vastaus, vastauksen koko, liput 0 (ei lippuja), asiakkaan osoiterakenne ja osoiterakenteen koko
        if (sendto(udp_socket, response, strlen(response), 0, (struct sockaddr *)&client_addr, client_len) < 0) {
            perror("Failed to send response");
        } else {
            printf("  -> Lähetettiin vastaus asiakkaalle: %s\n", response);
        }
    }

    // Sulje socket
    close(udp_socket);
    return EXIT_SUCCESS;
}
