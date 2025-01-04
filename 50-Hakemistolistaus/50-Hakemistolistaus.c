/*
 * 50-Hakemistolistaus
 *
 * Tämä ohjelma listaa annetun hakemistopolun tiedostot ja alihakemistot sekä tulostaa niistä:
 *    - tiedostojen perustiedot (tyyppi, koko, omistaja, oikeudet, aikaleimat)
 *    - laajennetut attribuutit, jos niitä on
 *
 * Ohjelman kääntäminen ja ajaminen:
 *  1. make
 *  2. ./50-Hakemistolistaus <hakemistopolku>
 *  3. make clean (lopuksi käännetyn ohjelman poistamiseen)
 */

#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <langinfo.h>
#include <locale.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <time.h>
#include <unistd.h>

// Tulostaa tiedoston tyypin ja siihen liittyvät oikeudet merkkijonona (esim. "d rwx rwx rwx")
void print_permissions(mode_t mode) {
    char permissions[14];  // Merkkijono tiedoston oikeuksille

    // Tiedoston tyyppi
    if (S_ISDIR(mode))
        permissions[0] = 'd';  // Hakemisto
    else if (S_ISLNK(mode))
        permissions[0] = 'l';  // Symbolinen linkki
    else if (S_ISREG(mode))
        permissions[0] = '-';  // Tavallinen tiedosto
    else if (S_ISCHR(mode))
        permissions[0] = 'c';  // Merkkilaitetiedosto
    else if (S_ISBLK(mode))
        permissions[0] = 'b';  // Lohkolaitetiedosto
    else if (S_ISFIFO(mode))
        permissions[0] = 'p';  // FIFO/putki
    else if (S_ISSOCK(mode))
        permissions[0] = 's';  // Socket
    else
        permissions[0] = '?';

    permissions[1] = ' ';

    // Omistajan oikeudet
    permissions[2] = (mode & S_IRUSR) ? 'r' : '-';  // Lukuoikeus
    permissions[3] = (mode & S_IWUSR) ? 'w' : '-';  // Kirjoitusoikeus
    permissions[4] = (mode & S_IXUSR) ? 'x' : '-';  // Suoritusoikeus (tavalliset tiedostot) ja hakuoikeus (hakemistot)

    permissions[5] = ' ';

    // Ryhmän oikeudet
    permissions[6] = (mode & S_IRGRP) ? 'r' : '-';  // Lukuoikeus
    permissions[7] = (mode & S_IWGRP) ? 'w' : '-';  // Kirjoitusoikeus
    permissions[8] = (mode & S_IXGRP) ? 'x' : '-';  // Suoritusoikeus (tavalliset tiedostot) ja hakuoikeus (hakemistot)

    permissions[9] = ' ';

    // Muiden oikeudet
    permissions[10] = (mode & S_IROTH) ? 'r' : '-';  // Lukuoikeus
    permissions[11] = (mode & S_IWOTH) ? 'w' : '-';  // Kirjoitusoikeus
    permissions[12] = (mode & S_IXOTH) ? 'x' : '-';  // Suoritusoikeus (tavalliset tiedostot) ja hakuoikeus (hakemistot)

    permissions[13] = '\0';  // Merkkijonon lopetusmerkki

    // Tulosta lopullinen merkkijono tiedoston tyypistä ja oikeuksista
    printf("  - Tyyppi ja oikeudet: %s\n", permissions);
}

// Tulostaa tiedoston yleiset tilatiedot: koko, laite-id, oikeudet, omistajan ja ryhmän nimet tai id:t,
// linkkien määrä, sekä viimeisimmät käyttö- ja muokkausajat.
void print_stat_info(struct stat *file_stat) {  // stat-rakenne tiedoston tilatietojen tallentamiseen
    struct passwd *pwd;                         // Osoitin käyttäjän tietoja sisältävään passwd-rakenteeseen
    struct group *grp;                          // Osoitin ryhmän tietoja sisältävään group-rakenteeseen
    struct tm *tm;                              // Osoitin tm-rakenteeseen, joka sisältää ajan tietoja
    char date[256];                             // Merkkijono ajan muotoilua varten

    // Tulosta tiedoston koko tavuina
    printf("  - Koko: %lld tavua\n", (long long)file_stat->st_size);

    // Tulosta tiedoston laite-id
    printf("  - Laite id: %ld\n", (long)file_stat->st_dev);

    // Tulosta tiedoston tyyppi ja oikeudet
    print_permissions(file_stat->st_mode);

    // Tulosta 'hard' linkkien määrä tiedostoon
    printf("  - Linkit: %ld\n", file_stat->st_nlink);

    // Hae tiedoston omistajan käyttäjänimi getpwuid-funktiolla tai vaihtoehtoisesti uid-numero, jos nimeä ei löydy
    if ((pwd = getpwuid(file_stat->st_uid)) != NULL) {
        printf("  - Omistajan nimi: %-8.8s\n", pwd->pw_name);
    } else {
        printf("  - Omistajan uid: %-8d\n", file_stat->st_uid);
    }

    // Hae tiedoston ryhmän nimi getgrgid-funktiolla tai vaihtoehtoisesti gid-numero, jos nimeä ei löydy
    if ((grp = getgrgid(file_stat->st_gid)) != NULL) {
        printf("  - Ryhmän nimi: %-8.8s\n", grp->gr_name);
    } else {
        printf("  - Ryhmän id: %-8d\n", file_stat->st_gid);
    }

    // Tulosta tiedoston viimeisin käyttöaika muotoiltuna localtime-funktiolla
    tm = localtime(&file_stat->st_atime);
    strftime(date, sizeof(date), nl_langinfo(D_T_FMT), tm);  // nl_langinfo palauttaa lokaalia tietoa parametrilla D_T_FMT, joka on aika ja päiväys formaatti
    printf("  - Viimeksi käytetty: %s\n", date);

    // Tulosta tiedoston viimeisin muokkausaika muotoiltuna
    tm = localtime(&file_stat->st_mtime);
    strftime(date, sizeof(date), nl_langinfo(D_T_FMT), tm);
    printf("  - Viimeksi muokattu: %s\n", date);
}

// Hakee ja tulostaa tiedoston mahdolliset laajennetut attribuutit, mikäli ne on käytettävissä
void print_extended_attributes(const char *path) {
    char *buf;         // Osoitin muistiin laajennettujen attribuuttien nimilistalle
    char *key;         // Osoitin laajennetun attribuutin avaimeen
    char *value;       // Osoitin laajennetun attribuutin arvoon
    ssize_t buflen;    // Laajennettujen attribuuttien nimilistan koko
    ssize_t keylen;    // Laajennetun attribuutin avaimen koko
    ssize_t valuelen;  // Laajennetun attribuutin arvon koko

    printf("  - Laajennetut attribuutit:\n");

    // Hae laajennettujen attribuuttien listan koko listxattr-funktiolla, parametreina tiedostopolku, listan osoitin (NULL) ja koko
    buflen = listxattr(path, NULL, 0);
    if (buflen == -1) {  // listxattr palauttaa -1, jos virhe

        // Tarkista, tukeeko tiedostojärjestelmä laajennettuja attribuutteja
        if (errno == ENOTSUP) {  // virhe ENOTSUP = ei tuettu
            printf("No extended attributes supported\n");
        } else {
            perror("Error getting extended attributes list");
        }
        return;
    }

    // Tiedostolla ei ole laajennettuja attribuutteja
    else if (buflen == 0) {
        printf("      • ei laajennettuja attribuutteja\n");
        return;
    } else {
        // Varaa muisti laajennettujen attribuuttien nimilistalle
        buf = malloc(buflen);
        if (buf == NULL) {
            perror("Error allocating memory for extended attributes list");
            return;
        }

        // Hae laajennettujen attribuuttien nimet, parametreina tiedostopolku, listan osoitin ja koko
        buflen = listxattr(path, buf, buflen);
        if (buflen == -1) {
            perror("Error getting extended attributes list");
            free(buf);
            return;
        }

        // Käsittele jokainen attribuutti erikseen
        key = buf;  // Ensimmäinen avain

        while (buflen > 0) {
            printf("      • %s: ", key);

            // Hae laajennetun attribuutin arvon koko getxattr-funktiolla, parametreina tiedostopolku, avain (nimi), arvo (NULL) ja koko (0)
            valuelen = getxattr(path, key, NULL, 0);
            if (valuelen == -1) {
                perror("Error getting extended attribute value size");
            } else if (valuelen > 0) {
                // Varaa muisti attribuutin arvolle
                value = malloc(valuelen + 1);
                if (value == NULL) {
                    perror("Error allocating memory for extended attribute value");
                    free(buf);
                    return;
                }

                // Hae laajennetun attribuutin arvo, parametreina tiedostopolku, avain (nimi), arvo ja koko
                valuelen = getxattr(path, key, value, valuelen);
                if (valuelen == -1) {
                    perror("Error getting extended attribute value");
                } else {
                    value[valuelen] = '\0';  // Nollamerkki merkkijonon loppuun
                    printf("%s", value);
                }

                // Vapauta muistissa oleva arvo
                free(value);
            } else if (valuelen == 0) {
                printf("<ei arvoa>");
            }

            printf("\n");

            // Siirrytään seuraavaan laajennettuun attribuuttiin
            keylen = strlen(key) + 1;
            buflen -= keylen;
            key += keylen;
        }

        // Vapauta lopuksi varattu muisti attribuuttien nimilistalle
        free(buf);
    }
}

// Avaa hakemiston ja listaa sen sisältämät tiedostot ja niiden tiedot yksi kerrallaan
void list_directory(const char *path) {
    DIR *dir;              // Hakemisto
    struct dirent *entry;  // Tiedosto/hakemistosisältö

    // Yritä avata annettu hakemisto opendir-funktiolla
    dir = opendir(path);
    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }

    // Käy läpi hakemiston sisältö tiedosto kerrallaan
    while ((entry = readdir(dir)) != NULL) {  // readdir palauttaa NULL, kun hakemiston sisältö on käyty läpi
        char filepath[1024];                  // Tiedostopolku
        struct stat file_stat;                // Tiedoston tilatiedot

        // Ohita '.' ja '..' hakemistot
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Muodosta tiedostopolku hakemiston ja tiedoston nimen perusteella
        snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);

        // Hae tiedoston tilatiedot stat-funktiolla ja tarkista mahdollinen virhe
        if (stat(filepath, &file_stat) == -1) {
            perror("Error getting file status");
            continue;
        }

        // Tulosta tiedoston nimi
        printf("Tiedosto: %s\n", entry->d_name);

        // Tulosta tiedoston tilatiedot
        print_stat_info(&file_stat);

        // Tulosta tiedoston mahdolliset laajennetut attribuutit
        print_extended_attributes(filepath);

        printf("\n");
    }

    // Sulje lopuksi hakemisto
    closedir(dir);
}

int main(int argc, char *argv[]) {
    // Tarkista, että ohjelma saa oikean määrän parametreja
    if (argc != 2) {
        fprintf(stderr, "Väärä määrä annettuja parametreja\n");
        fprintf(stderr, "Käyttö: %s <hakemistopolku>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Kutsu list_directory funktiota annetulla hakemistopolulla
    list_directory(argv[1]);

    return EXIT_SUCCESS;
}
