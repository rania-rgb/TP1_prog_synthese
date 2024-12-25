#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#define PROMPT "enseash %% "

// Fonction pour utiliser write
void write_string(const char *str) {
    write(1, str, strlen(str));
}

int main() {

    char input[128];//tableau où on stock la commande   
    char *args[16];    
    pid_t pid;        
    int status; //Statut pour attendre la fin du child process      
    struct timespec start_time, end_time; 
    write_string("Bienvenue dans le shell de l'ENSEA\n");
    write_string("Pour quitter, tapez 'exit'\n");

    while (1) { 
        write_string(PROMPT); //Le parent s'occupe à afficher le prompt à chaque fois dans le shell
  
        ssize_t command_read = read(0, input, sizeof(input) - 1);
        if (command_read <= 0) { //Erreur de lecture de commande
            write_string("Error encountered\n");
            break;
        }

        input[command_read - 1] = '\0'; //Ajouté pour supprimer le \n à la fin de la commande

        if (strcmp(input, "exit") == 0) { //Cas où on tape exit
            write_string("Bye bye...\n");
            break;
        }

        //J'ai remarqué que quand je fais une commande qui a plusieurs arguments, j'ai rien. Donc j'ai ajouté cette fonction qui stock la commande 
        //avec args, cette décomposition est aussi gérée par the parent process
        int i = 0; 
        args[i] = strtok(input, " "); //On utilise strtok pour diviser les chaines en sous-chaines pour les token
        while (args[i] != NULL && i < 15) {
            i++;
            args[i] = strtok(NULL, " ");
        }
        //is_background s'occupe de l'arrière-plan: 1 pour arrière-plan et 0 pour plan principal. Si il y'a "&" le terminal se lance en arrière-plan
        //et donne immédiatement la main à l'utilisateur de rentrer des nouvelles commandes. 
        //Exemple : sleep 2 , je vais attendre 2 secondes jusqu'à ce que ça s'exécute
        //si je fais sleep 2 & , je vais pas du tout attendre et j'aurais la main directement
        // Vérifie si la commande se termine par '&'
        int is_background = 0;
        if (i > 0 && strcmp(args[i - 1], "&") == 0) {
            is_background = 1;
            args[i - 1] = NULL; // Supprime '&' des arguments
        }

        // Fork a child process
        pid = fork();
        if (pid == -1) { //Echec du fork
            write_string("Fork failed\n");
            continue; 
        }

        if (pid == 0) { // Child process
         // Partie du processus enfant :
            // L'enfant utilise execvp pour chercher et exécuter la commande
            // `execvp` cherche le programme spécifié par args[0] dans les répertoires du PATH,
            // puis remplace l'image actuelle du processus par celle du programme trouvé.
            // args contient le nom de la commande et ses arguments.
            execvp(args[0], args); 
            // Si execvp échoue (commande non trouvée), on affiche un message et l'enfant se termine
            write_string("Command not found\n");
            exit(-1);
        } else { // Parent process
           if (is_background) {
               // Processus en arrière-plan : affiche le PID et ne pas attendre
               char bg_message[64];
               snprintf(bg_message, sizeof(bg_message), "[%d] Running in background\n", pid);
               write_string(bg_message);
           } else {
               
              
               // Une fois que l'enfant a terminé, le parent reprend la boucle
               // pour afficher le prompt et attendre une nouvelle commande.

               //Signaux:Un signal est une méthode utilisée par le système d'exploitation pour envoyer un message à un processus. Ce message
               // informe le processus qu'un événement particulier s'est produit ou lui demande de faire quelque chose (comme se terminer ou redémarrer).

               //WIFEXITED(status)== true, si the child porcess s'est terminé normalement
               //WIFSIGNALED(status)==true, si the child process s'est terminé à cause d'un signal
               //WTERMSIG(status) renvoie alors le numéro du signal qui a causé la fin du child process
               // 1. Capture du Temps de Début Avant l'Exécution de la Commande
                if (clock_gettime(CLOCK_MONOTONIC, &start_time) == -1) {
                    write_string("clock_gettime failed\n");
                    // Continuer même si la mesure échoue
                }
                // Le parent attend la fin de l'exécution de l'enfant grâce à waitpid

                waitpid(pid, &status, 0);
                // Mesure du temps après l'exécution de la commande
                if (clock_gettime(CLOCK_MONOTONIC, &end_time) == -1) {
                    write_string("clock_gettime failed\n");
                    // Continuer même si la mesure échoue
                }
                // Calcul du temps écoulé en secondes et nanosecondes
                double elapsed_sec = end_time.tv_sec - start_time.tv_sec;
                double elapsed_nsec = end_time.tv_nsec - start_time.tv_nsec;
                double total_elapsed = elapsed_sec + elapsed_nsec / 1e9;
               if (WIFEXITED(status)) {
                   // Si l'enfant s'est terminé normalement, récupérer le code de sortie
                   printf("code exit : %d , temps d'exécution : %.6f secondes\n", WEXITSTATUS(status), total_elapsed);
               } else if (WIFSIGNALED(status)) {
                   // Si l'enfant a été terminé par un signal, récupérer le numéro du signal
                   printf("signal exit: %d,, temps d'exécution : %.6f secondes\n", WTERMSIG(status), total_elapsed);
               }
           }
        }
    }

    return 0;
}
