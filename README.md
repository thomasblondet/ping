# Ping

Implémentation minimale de `ping`.

`ping` est une commande qui permet de tester la connexion à un hôte.

Elle utilise le protocole ICMP.

En envoyant une requête ICMP (*Echo Request*), le programme attend une réponse de l'hôte (*Echo Reply*) ou un message d'erreur.

ICMP (*Internet Control Message Protocol*) est un protocole utilisé notamment pour transmettre des messages de contrôle, de diagnostic et d'erreur sur un réseau.

## Usage

Une *raw socket* est nécessaire pour envoyer directement des paquets ICMP. L'utilisation d'une *raw socket* nécessite des privilèges élevés.

```bash
make
sudo ./ping <hostname ou adresse IP>
```

## Features

* Envoi de requêtes ICMP
* Réception et analyse des réponses
* Calcul du temps de réponse (*Round Trip Time*)
* Affichage des paquets autres que les *Echo Reply* avec l'option verbose (`-v`)
* Choix du TTL dans le header IP avec l'option `-m`
* Choix du nombre de requêtes envoyées avec l'option count (`-c`)

## Fonctionnement

* Résolution de l'hostname ou de l'adresse passée en paramètre
* Création de la *raw socket*
* Construction du paquet ICMP
* Envoi du paquet
* Attente et parsing de la réponse
* Calcul du temps de réponse
* Affichage des statistiques lors de l'interruption du programme (`^C`) ou lorsque le nombre de requêtes défini avec l'option `-c` est atteint

## Demo

![Ping demo](assets/ping_demo.gif)