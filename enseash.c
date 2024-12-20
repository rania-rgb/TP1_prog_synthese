#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

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
           // Le parent attend la fin de l'exécution de l'enfant grâce à waitpid
            waitpid(pid, &status, 0);
             // Une fois que l'enfant a terminé, le parent reprend la boucle
            // pour afficher le prompt et attendre une nouvelle commande.

            //Signaux:Un signal est une méthode utilisée par le système d'exploitation pour envoyer un message à un processus. Ce message
            // informe le processus qu'un événement particulier s'est produit ou lui demande de faire quelque chose (comme se terminer ou redémarrer).

            //WIFEXITED(status)== true, si the child porcess s'est terminé normalement
            //WIFSIGNALED(status)==true, si the child process s'est terminé à cause d'un signal
            //WTERMSIG(status) renvoie alors le numéro du signal qui a causé la fin du child process
            // Vérification du statut du processus enfant
            if (WIFEXITED(status)) {
                // Si l'enfant s'est terminé normalement, récupérer le code de sortie
                int exit_code = WEXITSTATUS(status);
                char prompt[128]; //prompt est une variable locale temporaire, dédiée uniquement à la gestion du texte du prompt.
                //prompt: son rôle est de formater et afficher le message de statut
                snprintf(prompt, sizeof(prompt), "enseash [exit:%d] %% ", exit_code);
                write_string(prompt);
            } else if (WIFSIGNALED(status)) {
                // Si l'enfant a été terminé par un signal, récupérer le numéro du signal
                int signal = WTERMSIG(status);
                char prompt[128];
                snprintf(prompt, sizeof(prompt), "enseash [sign:%d] %% ", signal);
                write_string(prompt);
            }
        }
    }

    return 0;
}
