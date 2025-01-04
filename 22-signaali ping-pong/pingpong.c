/*
 * 22-signaali ping-pong
 *
 * Tämä ohjelma luo kopioita itsestään. Kopiot lähettävät SIGUSR1-signaalin pääohjelmalle,
 * joka vastaanottaa ne ja lähettää SIGUSR2-signaalin kaikille kopioille.
 * Kopio lopettaa vastaanotettuaan SIGUSR2-signaalin.
 * Pääohjelma odottaa kaikkien kopioiden päättyvän ja lopettaa itse, kun kaikki kopiot ovat päättyneet.
 *
 * Ohjelman kääntäminen ja ajaminen:
 *  1. make
 *  2. ./pingpong --kopioita X (jossa X on kopioiden määrä)
 *  3. make clean (lopuksi käännetyn ohjelman poistamiseen)
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int copy_count;            // Kopioiden määrä
int received_signals = 0;  // Vastaanotettujen SIGUSR1-signaalien määrä pääohjelmassa
int *copy_pids;            // Kopioiden prosessi-id:t

// Käsittelijä pääohjelman vastaanottamille SIGUSR1-signaalille
void sigusr1_handler(int signum) {
    received_signals++;
    printf("Pääohjelma: Vastaanotettu SIGUSR1 (%d/%d).\n", received_signals, copy_count);

    // Jos kaikki SIGUSR1-signaalit on vastaanotettu, lähetetään SIGUSR2-signaalit kopioille
    if (received_signals == copy_count) {
        printf("Pääohjelma: Kaikki SIGUSR1-signaalit vastaanotettu. Lähetetään SIGUSR2-signaalit kopioille.\n");

        sleep(1);  // Pieni viive seuraamisen vuoksi

        for (int i = 0; i < copy_count; i++) {
            kill(copy_pids[i], SIGUSR2);  // Lähetä SIGUSR2-signaalit kopioille kill() systeemikutsulla, joka ottaa parametreina prosessin id:n ja signaalin
            usleep(5000);                 // Lyhyt viive ennen seuraavan signaalin lähetystä
        }
    }
}

// Käsittelijä kopioiden vastaanottamille SIGUSR2-signaalille
void sigusr2_handler(int signum) {
    printf("Kopio PID %d: vastaanotti SIGUSR2, lopetetaan.\n", getpid());  // Tulosta prosessin-id kutsumalla getpid() funktiota
    exit(EXIT_SUCCESS);                                                    // Lopeta kopio
}

// Kopioprosessi
void copy_process() {
    printf("Kopio PID %d: Lähetetään SIGUSR1.\n", getpid());
    kill(getppid(), SIGUSR1);          // Lähetä SIGUSR1-signaali pääohjelmalle. getppid() palauttaa prosessin vanhemman (pääohjelman) id:n
    signal(SIGUSR2, sigusr2_handler);  // Rekisteröi käsittelijä SIGUSR2-signaalille signal() funktiolla, joka ottaa parametreina signaalin ja käsittelijän
    pause();                           // Nuku ja odota SIGUSR2-signaalia
}

int main(int argc, char *argv[]) {
    // Puskurointi pois tulostuksesta päällekkäisyyksien välttämiseksi, parametreina stdout, bufferi NULL, moodi _IONBF (ei puskurointia), koko 0
    setvbuf(stdout, NULL, _IONBF, 0);

    // Tarkistetaan parametrit
    if (argc != 3 || strcmp(argv[1], "--kopioita") != 0) {
        printf("Käyttö: %s --kopioita X\n", argv[0]);  // Tulostetaan ohjeet, jos parametrit ovat väärät
        return EXIT_FAILURE;                           // Lopetetaan ohjelma jos parametrit ovat väärät
    }

    copy_count = atoi(argv[2]);                    // Valittujen kopioiden määrä talteen integeriksi
    copy_pids = malloc(copy_count * sizeof(int));  // Muistin varaus kopiotaulukolle kopiomäärän mukaan

    // Tarkista muistin varaus
    if (copy_pids == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);  // Lopeta ohjelma, jos muistin varaus epäonnistuu
    }

    // Tämän structin käytön apuna on hyödynnetty tekoälyä (ChatGPT)
    struct sigaction signal_action = {0};        // Alustus struct signaalin käsittelylle sigaction structilla
    signal_action.sa_handler = sigusr1_handler;  // Pääohjelman SIGUSR1-signaalin käsittelijä structiin
    signal_action.sa_flags = SA_RESTART;         // SA_RESTART lippu, jotta keskeytyneet kutsut aloitetaan alusta

    // Rekisteröi käsittelijä SIGUSR1-signaalille sigaction() systeemikutsulla, joka ottaa parametreina signaalin, sigaction-structin ja NULL (ei vanhaa käsittelijää)
    sigaction(SIGUSR1, &signal_action, NULL);

    printf("Pääohjelma: Luodaan %d kopiota.\n", copy_count);
    sleep(1);  // Pieni viive seuraamisen vuoksi

    // Kopioiden luonti
    for (int i = 0; i < copy_count; i++) {
        int pid = fork();  // Luo kopio fork() funktiolla, joka "monistaa" prosessin ja palauttaa uuden prosessin id:n

        // Jos palautettu id on 0, ollaan kopiossa ja kutsutaan copy_process() funktiota ja aloitetaan kopion toiminta
        if (pid == 0) {
            copy_process();
        }
        // Pääohjelmassa, jos palautettu id on positiivinen eli uuden kopion id
        else if (pid > 0) {
            copy_pids[i] = pid;  // Tallenna kopion PID taulukkoon
            printf("Pääohjelma: Luotu kopio PID %d.\n", pid);
            usleep(5000);  // Pieni viive ennen seuraavan kopion luontia
        }
        // Jos fork() palauttaa -1, on tapahtunut virhe
        else {
            perror("fork");
            free(copy_pids);     // vapautetaan muisti
            exit(EXIT_FAILURE);  // lopetetaan ohjelma
        }
    }

    printf("Pääohjelma: Odotetaan kopioita.\n");
    sleep(1);  // Pieni viive seuraamisen vuoksi

    // Odotetaan kopioita
    while (copy_count > 0) {
        // Etsitään päättyneitä kopioita (arvo -1 = mikä tahansa kopio), ei tallenneta tilanne informaatiota (NULL) ja palataan heti, jos ei löydy (WNOHANG)
        pid_t pid = waitpid(-1, NULL, WNOHANG);  // waitpid() palauttaa kopion id:n, jos kopio on päättynyt, muuten 0

        if (pid > 0) {
            printf("Pääohjelma: Kopio PID %d on päättynyt.\n", pid);
            copy_count--;  // Vähennetään odotettavia kopioita
        }
        // Jos ei päättyneitä kopioita, odotetaan hetki
        else if (pid == 0) {
            usleep(10000);
        }
        // Jos virhe ei ole signaalikeskeytys, poistutaan
        else if (errno != EINTR) {
            perror("waitpid");
            break;
        }
    }
    sleep(1);  // Pieni viive seuraamisen vuoksi

    printf("Kaikki kopiot ovat päättyneet.\n");

    free(copy_pids);  // Vapauta muisti lopuksi
    return EXIT_SUCCESS;
}
