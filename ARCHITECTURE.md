# Architecture de jsh

## Introduction
Ce document décrit l'architecture du projet `jsh`, un interpréteur de commandes (aka shell) interactif reprenant quelques fonctionnalités classiques des shells usuels, en particulier la gestion des tâches lancées depuis le shell. Outre la possibilité d'exécuter toutes les commandes externes, `jsh` propose quelques commandes internes, permet la redirection des flots standard ainsi que les combinaisons par tube, et adapter le prompt à la situation.

## Structure du Projet
Le projet est organisé en dossiers et fichiers de manière à séparer les en-têtes des fichiers source. Voici la structure de haut niveau:

- `head/` : Contient les fichiers d'en-tête du projet.
- `src/` : Contient les fichiers source C qui implémentent les fonctionnalités du shell.

Le projet est structuré en plusieurs fichiers C, chacun ayant une responsabilité spécifique :

- `builtin.c` : Implémente les commandes internes du shell.
- `command.c` : Gère l'interprétation et l'exécution des commandes.
- `execute.c` : Responsable de l'exécution des commandes et de la gestion des processus.
- `job.c` : Gère les jobs et les processus en arrière-plan ou suspendus.
- `main.c` : Point d'entrée du shell, où la boucle principale est exécutée.
- `parser.c` : Analyse les commandes entrées par l'utilisateur.
- `prompt.c` : Gère l'affichage et la mise à jour de l'invite de commande.
- `redirections.c` : Gère la redirection des entrées/sorties des commandes.

## Structure des Données

### Job
Un job est une structure de données qui représente une commande ou un pipeline de commandes. Elle contient les champs suivants :

- `age` : Un entier représentant le numéro du job.
- `pid` : Un identifiant de processus (de type `pid_t`) représentant l'ID du processus.
- `state` : Une variable de type `job_state` représentant l'état du job.
- `command` : Une chaîne de caractères représentant la ligne de commande.
- `next` : Un pointeur vers le prochain job dans la liste (de type `struct job`).
  
### Command
Une commande est une structure de données qui représente une commande simple. Elle contient les champs suivants :

- `name` : Une chaîne de caractères représentant le nom de la commande.
- `arguments` : Un pointeur vers une structure `Argument` qui contient les arguments de la commande.
- `redirection` : Un pointeur vers une structure `Redirection` qui contient les informations de redirection de la commande.
- `substitutions` : Un pointeur vers un tableau de pointeurs vers des structures `Command`. Ces structures représentent les substitutions de commandes (c'est-à-dire les commandes qui sont exécutées et dont le résultat est utilisé comme argument d'une autre commande).
- `nb_substitutions` : Un entier représentant le nombre de substitutions de commandes.
- `size_substitutions` : Un entier représentant la taille du tableau de substitutions.
- `background` : Un entier qui indique si la commande doit être exécutée en arrière-plan (1) ou en premier plan (0).
- `pipe` : Un pointeur vers un entier qui représente le descripteur de fichier du pipe utilisé pour la redirection de la sortie de cette commande vers l'entrée de la commande suivante.
- `next` : Un pointeur vers la prochaine structure `Command` dans la liste des commandes pipées.

### Argument
Une structure `Argument` représente un argument d'une commande. Elle contient les champs suivants :

- `value` : Une chaîne de caractères représentant la valeur de l'argument.
- `next` : Un pointeur vers la prochaine structure `Argument` dans la liste des arguments.

### Redirection
Une structure `Redirection` représente une redirection de la sortie standard ou de l'entrée standard d'une commande. Elle contient les champs suivants :

- `type` : Une variable de type `RedirectionType` représentant le type de redirection. `RedirectionType` est un énumérateur qui peut prendre des valeurs comme `REDIRECTION_INPUT`, `REDIRECTION_OUTPUT`, etc., représentant les différents types de redirections possibles.
- `value` : Une chaîne de caractères représentant la valeur de la redirection. Cela pourrait être le nom du fichier dans lequel rediriger la sortie, par exemple.
- `next` : Un pointeur vers la prochaine structure `Redirection` dans la liste des redirections.

## Fonctionnement du Shell

### Boucle Principale
Le shell est exécuté dans une boucle infinie. À chaque itération, le shell affiche l'invite de commande, lit la commande entrée par l'utilisateur, l'analyse, l'exécute et affiche le résultat de l'exécution. La boucle principale est implémentée dans la fonction `main()` du fichier `main.c`.

### Analyse de la Commande
L'analyse de la commande est effectuée par la fonction `parse_command()` du fichier `parser.c`. Cette fonction prend une chaîne de caractères représentant la commande entrée par l'utilisateur et renvoie une structure `Command` qui représente la commande analysée. La fonction `parse_command()` utilise la fonction `parse_command_internal()` pour analyser les commandes internes et la fonction `parse_command_external()` pour analyser les commandes externes.

### Exécution de la Commande
L'exécution de la commande est effectuée par la fonction `execute_command()` du fichier `execute.c`. Cette fonction prend une structure `Command` représentant la commande à exécuter et l'exécute. La fonction `execute_command()` utilise la fonction `execute_command_internal()` pour exécuter les commandes internes et la fonction `execute_command_external()` pour exécuter les commandes externes.