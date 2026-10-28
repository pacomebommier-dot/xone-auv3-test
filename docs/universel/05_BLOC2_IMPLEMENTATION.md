# Bloc 2 — implémentation native du 28 octobre 2026

## CUE/LOAD et banques locales
CUE INST4/8 vise INST1/2 (owners PATCH 8/9), LOAD YELLOW4/8 vise YELLOW1/2 (owners 10/11). Chaque source possède trois PATCH de 60 valeurs : deux blocs MACHINE de 30 paramètres, sans leurs modes S8. Position 4 appelle BASE ; aucun PATCH4. Le DOWN capture source et position avant la mutation NAV. Le BAR authentifié applique via S3 et émet les CC/notes nominaux. CC931..946 conservent leur confirmation de callback indépendante : cette émission ne prouve ni rappel local accepté ni chargement externe.

`saveSourcePatch` capture uniquement des valeurs LIVE connues. Cette API native réelle de sauvegarde/rappel des paires reste distincte des SAVE physiques SAMPLE/MACHINE existants. Aucun geste SAVE de paire absent du contrat n'est inventé : son raccordement physique reste une frontière explicite. Les tests vérifient les banques et mutations réelles, pas un simple adaptateur MIDI.

## Composition BASE/PATCH
Chaque rappel de paire compose 97 champs : 60 valeurs/états MACHINE et 37 cibles SOCLE autorisées (24 CC, 12 états SPOT, filtre source). PATCH restaure son payload et remet seulement son SOCLE à zéro. BASE utilise les seuls zéros provisoires explicitement autorisés. Les modes S8, MIX1328..1331, isolations, INIT partagés et autres owners restent intacts. UNKNOWN dans une capture refuse le SAVE ; aucune banque neuve fictive.

Le contexte logique natif initial vaut 1, indépendamment de tout profil instrumental. Toutes les valeurs musicales restent UNKNOWN au démarrage, sans sortie MIDI. Ce contexte permet BASE sur un plugin neuf sans fixture ; aucun backend n'est configuré ou prétendu observé. Les pickup CC dont les cibles changent sont réarmés. Les PATCH sauvegardés sont conservés après sélection ou BASE.

## MEMORY et arbitrage
28 pages / 96 slots physiques : six CC et trois états SPOT. Le premier engagement inactif capture le LIVE connu (cycle GOLDEN), puis un appui engagé rappelle au BEAT. Contenu S6 et engagement musical sont distincts. Une demande valide remplace seulement celle de sa page ; un refus conserve la demande précédente. Les copies pending sont épinglées au geste, indépendantes de la page visible.

Le HostActionScheduler existant est déplacé du wrapper vers HostEngine, sans second scheduler ni horloge. PATCH/BASE et MEMORY partagent son ordre d'admission. Au BAR, PATCH/BASE annule les MEMORY ultérieures de ses trois pages et désengage leurs douze slots ; une MEMORY antérieure peut s'appliquer puis être remplacée par BASE au même événement. Banques, autres sources et pages DRUMS/FILL restent intactes. Aucun désengagement MACHINE simple n'est inventé par égalité numérique.

STOP annule les attentes et conserve les banques. Une restauration de session retire les intentions et engagements périmés. La file MIDI nominale est bornée ; échec callback = reprise au render suivant, pas dans le même tampon. Les NoteOff dus restent sous l'autorité S4. Le preset externe BASE reste non configuré ; aucun succès de synthé/AUM n'est inféré.

## Vérification et limites
62/62 suites C++ locales Clang Windows PASS, dix tests C historiques PASS et trois vérifications d'intégrité PASS. Tests ciblés : 12 slots PATCH, quatre BASE, PATCH2→PATCH1→BASE, deux owners au même BAR, changement NAV après DOWN, UNKNOWN/EMPTY, pickup, 96 routes MEMORY, pages cachées, remplacement/refus, deux ordres BAR/BEAT, désengagement sans effacement, STOP et 24 NoteOff sans duplication.

Le banc AU macOS réel ajoute CUE4 physique→BASE au BAR, sortie nominale, callback refusé et reprise au render suivant. Son résultat dépend de la CI Apple, distincte du résultat local Windows. La configuration CI Apple/macOS-15-intel est inchangée. Aucune validation AUM/iPhone n'est annoncée.

Persistance des banques et feedback LED complet restent au bloc 4. PRESET/RED et alternatives S8 restent au bloc 3. Une nouvelle commande à transport explicitement arrêté s'applique directement, sans BAR/BEAT synthétique ni attente de START ; STOP retire les anciennes attentes. En cas de transport inconnu, aucune échéance n'est inventée.
