# RESET/FILTER et coût CPU — correction du 30 octobre 2026

Base : aa6195c, candidate de la campagne existante. Travail isolé sur fix/reset-filter-cpu ; aucun merge main et aucun remplacement de session iPad.

## Changements réalisés

- TOP4 exécute désormais RESET FILTER GLOBAL/TOTAL sur l'unique cible LIVE1476 / CC virtuel708 lorsque son default est explicitement connu. Sous PRESET/SAMPLE/MACHINE/RESET, sa destination reste capturée ; STOP ou changement de NAV invalide l'ancien geste.
- SPOT de type RESET agit sur le paramètre continu associé, sans modifier ON/OFF ou mode SPOT. Dans MUTE, BASE conservent MUTE/SOLO et leurs BAR ; SPOT vise le contrôle continu du contexte dessous.
- BASE RESET court : scope12 CC SAMPLE/MACHINE ; six CC des pages FX admissibles ; six ou douze CC des pages BUS/TOTAL adressées. Le banc physique neuf vérifie SAMPLE. Les pages CUE/LOAD ne sont pas converties en pages FX.
- BASE RESET SAMPLE long : groupe1..4 ou5..8, 48 CC, au vrai UP ; pas de chute vers le socle. Origine NAV capturée et anciens UP annulés par STOP.
- Un scope numérique est validé entièrement avant mutation ; un default UNKNOWN, une cible non continue, une configuration incompatible ou une file insuffisante refuse le scope entier. Banques PATCH/MEMORY et états ON/OFF ne sont pas réécrits.
- Les CC résultants rejoignent la file nominale existante, avec reprise après échec au prochain rendu et réarmement du pickup. Aucun opcode RESET, scheduler supplémentaire ou service matériel fictif.
- Les defaults portent configuration et identités alternatives attendues. Une valeur configurée pour l'ancien instrument ne peut pas être appliquée à une autre alternative.
- La table de defaults est un champ d'état AU indépendant XONE_RESET_DEFAULTS_V1, schema1,951 octets, CRC32 et valeurs typées. Ancien document sans table = defaults UNKNOWN. La configuration seule n'émet rien. Lors d'un restore, la table attend le même token/frontière réelle que l'image musicale, y compris avec une touche tenue.
- Le moteur ne reconstruit plus le document complet à chaque avance de rendu, grille vide ou entrée MIDI non admise. Une mutation LIVE, NAV, banque ou état appliqué continue à publier. Les transitions longues MUTE/NAV restent capturées.
- La borne de l'index MEMORY est rendue explicite dans applyDocument pour GCC16, sans modifier BankS6 ou les sources gelées.

## Limites conservées

Aucun default réel n'est choisi par ces tests. Les nombres63/41/etc. des bancs sont des stimuli explicitement configurés ; ils ne constituent pas des valeurs de plugins approuvées.

Les pulses TOP1..3 S7 déjà raccordés restent des actions MIDI virtuelles ; ils ne sont pas remplacés par des CC arbitraires. Leur interprétation appartient aux bindings des effets.

Ce changement ne clôt pas tous les RESET : BASE4 GLOBAL/LOOP (reset complet de tranche avec retour INIT) reste à raccorder ; les autres groupes longs ne sont pas déduits par analogie. Les balances LOOP569..571, l'adresse invalide572 et le multiplicateur573 sont exclus des defaults RESET.

La fourniture des valeurs s'effectue via l'API native/état AU ; aucun éditeur utilisateur de defaults n'est ajouté ici. Les profils, plugins, noms de destinations et defaults réels restent à configurer. Ni ACK instrumental ni réception matérielle ne sont inférés du callback.

## Vérification

Tests ajoutés avant correction :
- test_host_idle_publication : assertion en échec avant suppression des copies ;4096 buffers sans publication supplémentaire après correction ; grilles sans pending, entrées non admises, CC et visibilité MUTE longue.
- test_host_reset : TOP4 avec default connu en échec avant raccordement ; puis refus UNKNOWN/config, scope entier, codec corrompu, retry, PRESET/STOP, RESET SPOT et BASE SAMPLE court/long. Ajouts : identité d'alternative et table retardée jusqu'au vrai UP du restore.
- test_auv3_document.mm : vraie classe AU, table d'état sans fixture, aucun TX de configuration, TOP4 physique et callback échoué/rendu suivant, métadonnées corrompues. Exécution Apple/macOS dans la CI ; pas sur Windows.

Régressions locales :71 suites C++ optimisées strictes au cours du développement, dix tests C historiques et trois contrôles d'intégrité PASS. Les derniers scénarios ciblés et la CI du commit final doivent être confirmés dans le checkpoint de clôture. Les sources/14 suites gelées restent intactes.

Les preuves locales sont dans evidence/reset-cpu. Le relevé de comparaison reproductible est généré par tools/benchmark_idle.py ; il ne fixe aucun seuil arbitraire de PASS temporel.

## Réflexion CPU et suite de mesure

Le nombre de plugins AUM ne multiplie pas le nombre de moteurs internes XONE. Il augmente surtout la charge audio et le coût de distribution dans l'hôte. XONE ne traite pas l'audio et ne décide pas ici du bypass des instruments. Un éventuel bypass devra respecter les effets partagés, fins de son et capacités réelles de l'hôte.

Le premier coût démontré était évitable : reconstruire/copier toutes les banques au repos. Il est supprimé. La projection LED reste animée selon les phases contractuelles ; elle n'est pas gelée pour gagner artificiellement du CPU.

À48kHz,128frames représentent2,67ms pour un buffer. Une moyenne Windows faible ne garantit ni cette échéance sur iPad, ni la capacité de toute la session AUM. Il faut mesurer les pointes, les xruns et les rappels importants, interface fermée puis ouverte, moniteurs/diagnostic distingués de XONE.

La comparaison native utilise4096 buffers128frames,256 buffers de chauffe, cinq passes alternées, même compilateur/O1/assertions et même workload pour aa6195c et le code corrigé. Elle inclut l'entretien des LED au repos. Elle ne mesure ni UIKit ni le wrapper/audio/AUM ou des plugins.

Restent à mesurer sur appareil : session légère puis représentative chargée, navigation/CC simultanés, PRESET/RED/BASE, STOP/reopen, rafales LED et reconnexion. Conserver modèle iPad, iOS/AUM, fréquence/buffer, CPU repos/pics, xruns et traces POST. Les destinations et sources audio peuvent être choisies plus tard ; elles ne bloquent pas ces corrections internes.

Le plafond de256 événements AU/buffer et les grosses rafales nominales restent des points de charge à tester dans le vrai wrapper ; aucune récupération nouvelle d'événements perdus n'est prétendue ici.

La première comparaison O2 stricte refuse la référence aa6195c avec GCC16 sur son avertissement array-bounds déjà identifié. La mesure comparative O1 garde les warnings fatals des deux côtés ; elle ne remplace pas les tests O2 du code corrigé. Échec initial conservé dans evidence/reset-cpu/benchmark/reference.compile.log.

## Mesure native de repos — cinq passes O1 strictes

Windows11 / GCC16.2, même code de banc,256 buffers de chauffe puis4096 buffers128frames à48kHz. Médiane des moyennes : référence10,7637µs/buffer ; correction0,370361µs/buffer, ratio29,06. Taille du document95276octets. Le moteur augmente de340760 à350744octets pour les tables/canaux de defaults ; pas d'allocation de ces tables dans le rendu.

Les cinq passes et leurs p99/max sont conservées dans evidence/reset-cpu/BENCHMARK_IDLE_O1.json. La première mesure conservée dans benchmark-O1/PRE_LONG_RESULTATS.json contenait un pic isolé de3407µs ; la dernière mesure a un maximum corrigé de11µs. Le banc Windows ne certifie pas une échéance audio, même si le gain médian est net. Aucun gain de30x sur la session AUM totale n'est annoncé.

Le test LONG a reproduit un refus indu : le garde de rappel PATCH interdisait aussi RESET dans un contexte momentané stable. RESET dispose maintenant d’un commit LIVE dédié dans CoreV3, sans toucher au retour NAV LONG ; TOP4 pendant LONG SAMPLE et SPOT pendant LONG RESET sont vérifiés. Les protections PATCH restent inchangées.
