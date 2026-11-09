# Dernier LIVE dans le document AUv3 — V3, 26/10/2026

Base musicale 03be998 : CI complète PASS https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40366951514.
Contrat RED publié sur 15a7a243 : CI portable en échec attendu avant existence du codec/échange session, iOS non lancé. La CI du raccordement est à confirmer séparément.

## Raccordement et portée
L'extension implémente fullStateForDocument via une clef NSData XONE_LAST_LIVE_V1. Elle conserve les clefs du superclass. fullState (preset hôte générique) n'est pas détourné pour confondre le dernier LIVE avec les PATCH/BASE XONE.
Source API Apple : https://developer.apple.com/documentation/audiotoolbox/auaudiounit/fullstatefordocument
L'image contient uniquement les 2048 cellules LIVE existantes, le schéma, la configuration déclarée de la session et sa révision. Aucun état observé, port, format audio ou banque PATCH/BASE S8 n'est créé. Le codec conserve l'image LIVE existante sans l'assimiler à une capture PATCH/PRESET/RED. La séparation fine des configurations S8, la NAV complète et les autres persistances restent à intégrer selon leurs contrats.

Une image invalide, une version inconnue, un mauvais nombre de cellules, un CRC incorrect ou des valeurs hors domaine sont refusés avant mutation. UNKNOWN reste -2. Une AUv3 neuve sans référence/session connue ne transforme pas UNKNOWN en zéro. Le chargement n'émet pas de MIDI.

## Scénarios qui justifient l'échange entre threads
AUM peut lire la session alors que le thread audio modifie le LIVE : une simple copie concurrente produirait une image déchirée et une course mémoire C++. Deux mailboxes SPSC à trois slots transfèrent uniquement des images possédées : UI -> rendu pour la demande, rendu -> UI pour le snapshot. Le lecteur conserve un slot que le producteur ne peut pas écrire ; les changements de propriété sont atomiques. Pas de verrou, allocation, attente ou sérialisation dans le rendu.
Les callbacks de document UI sont sérialisés entre eux avec @synchronized hors rendu. Le getter ne lit jamais directement le moteur audio. Une génération de demande évite qu'un ancien snapshot écrase une restauration demandée mais pas encore appliquée.
La restauration s'applique au début d'un buffer, seulement après les vrais UP de touches tenues et sans dette NoteOff. Elle réinitialise pickup et cache de sorties pour ne pas réutiliser un état envoyé antérieur. Elle ne forge aucun UP ni nouvelle observation. Le snapshot est publié en fin de buffer lorsque sa révision/génération change.

## Épreuves
Tests/test_host_session.cpp : round-trip codec, CRC même pour une corruption qui reste dans le domaine, longueur, version/config/valeurs ; set/get avant premier rendu ; application au buffer ; contrôle CC après restauration ; sauvegarde du LIVE modifié ; restauration pendant une tenue différée jusqu'au vrai UP ; UNKNOWN backend conservé ; 30000 publications concurrentes sans image déchirée avec validation des 2048 cellules.
Ce sont des tests du harness C++ et une compilation du wrapper iOS, pas une preuve save/reopen AUM. AUM_FULL_DEFERRED, K2_PHYSICAL_PENDING et SH01A_PHYSICAL_PENDING restent ouverts. L13 reste partiel, aucun lot clôturé.
Le travail local est arrêté par le quota du service de revue automatique ; cette tranche est éditée par le connecteur GitHub autorisé et vérifiée par CI distante. Le checkout local doit être synchronisé une fois l'accès local revenu, sans écraser d'éventuelles modifications utilisateur.

CI complète de la tranche d30bb1d : PASS https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40397065753 (GCC, Clang, Apple Clang, sanitizers, AUv3 iOS unsigned). La recette AUM reste différée.
