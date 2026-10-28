# XONE AUv3 — CONTRAT MAÎTRE UNIVERSEL V1
Date : 28 octobre 2026. Décision utilisateur ; nouveau cadrage de pilotage, sans révision rétroactive des contrats musicaux.

## Mission
Livrer XONE AUv3 universel complet : toutes les commandes, pages, états, banques, snapshots, sorties MIDI virtuelles et feedback testables sans synthé, effet ni destination définitive. La candidate 2cedd1e est un jalon d'installation, PAS une clôture fonctionnelle.

## Règle maîtresse
XONE commande, mémorise, restitue et émet. AUM distribue. Les instruments interprètent.
Aucune fonction interne ni adresse virtuelle normative n'est à différer faute de choix d'instrument, preset ou tranche. Une sortie virtuelle MIDI n'est pas une preuve de réception, preset chargé ni effet audio.

## Architecture sans ambiguïté
ENTRÉE : K2 physique OU une instance Mozaic K2_SIM_V3 exclusive ; uniquement notes/CC physiques et timings. Mozaic ne prend aucune décision musicale.
MOTEUR : moteur natif unique, gestes/S1-S2-S3, LIVE, PICKUP, CC/SPOT, PAGE MUTE, SOLO, LOOP, TOP/BOT/DJ, PATCH/MEMORY/CUE/LOAD/BASE/PRESET/RED, propriétaires S8, MIDI, LED, STOP, persistance et resync.
SORTIE : API virtuelle S7 V1.2 MIDI7..14, MIDI15 exclusivement LED. AUM MIDI Learn/MIDI Control/fan-out pour effets et instruments AUv3. Profil adaptateur versionné pour matériel MIDI (par exemple SH-01A), sans changement du coeur XONE. Aucun transfert ni mixage audio dans XONE.

## Connu / inconnu : deux couches séparées
- Adresse et commande virtuelle prévues : émission MIDI contractuelle même sans binding instrument, observable au moniteur.
- BASE local provisoire : valeurs 0/OFF et modes SPOT MIX/code1 décidés le 15/10, dans le scope local défini. Action explicite, jamais reset automatique à OnLoad.
- RESET_DEFAULT individuel UNKNOWN ou adaptation matérielle non configurée : refuser seulement l'opération qui en dépend, avant mutation ; ne pas fabriquer de CC, PC, SysEx, ACK ou succès de chargement.
- Aucun UNKNOWN ne devient 0 par commodité. L'absence de plugin externe ne bloque pas la logique interne admissible.

## Intégralité métier obligatoire
Gestes / slots / sorties CC et notes ; SAVE et RECALL PATCH1..3, MEMORY au BEAT ; CUE/LOAD1..3 = rappel PATCH de l'owner au BAR ; 4e touche CUE/LOAD = BASE local de la même source, sans créer PATCH4. TOP4 reste FILTER GLOBAL.
PRESET global / RED temporaire, ownership/alternatives S8, FX, FADER, FILTER, MUTE/SOLO, MIDI LED et captures de destination. OFF dus/STOP, conflits et transport ; persistance compatible et restauration sûre.
PATCH/PRESET/RED n'émettent pas de « commande magique » : ils restituent les CC/notes ordinaires de leurs résultats.
Respecter les exclusions S8 de modes SPOT MACHINE dans les snapshots, sans les généraliser à SAMPLE. Les modes SPOT MACHINE sont attachés au profil S8. Ne pas importer de données historiques ou fixture automatiquement dans une banque neuve.

## Hiérarchie et garde-fous
GOLDEN et tableurs CCOUT/BINS constituent la provenance. Ne pas reprendre leurs indices supplantés : la spécification API S7 V1.2 et les arbitrages S8 spécialisés définissent les sorties actuelles. Les contrats musicaux existants font autorité sur leur périmètre ; ne pas les réécrire.
Le précédent 00_CONTRAT_PILOTAGE_V0 reste applicable pour intégrité, non-régression, STOP et refus, MAIS sa classification « absence de binding = fonction interne différée » est remplacée par ce document.
Pas de nouveau moteur, double scheduler, AUDIO XONE, merge main, modification archives/empreintes, masquage de tests. Aucun PASS AUM/physique non exécuté.

## Definition of Done
En présence de K2_SIM ou K2, sans destinataire musical : toutes les commandes logiquement admissibles modifient LIVE au bon timing ; MIDI CC/NOTE et LED attendus vérifiables ; banques SAVE/RECALL et persistance fonctionnent ; STOP/NoteOff sûrs. Les actions qui exigent le succès d'un instrument externe restent explicites « non raccordé » jusqu'à preuve réelle.

## Sources
S7 API : https://drive.google.com/file/d/18GJYIWt8MC0-pWFpl6TR0WYfcWe4fSbh/view
Correctif architecture 26/10 : https://drive.google.com/file/d/1FtvRVTBNo11sF_gxyydzLIxrKMybO06D/view
Contrat BASE/PATCH/CUE/LOAD : https://docs.google.com/document/d/1AjlLzvS_LwQPIUuNNxfmaqwTr6ZEmeDLTD08ba-v_xI/edit
RESET V1.5 : https://drive.google.com/file/d/1426LT1eD_mWgzQIJgiB9gikC3nrJmJlZ/view
CCOUT provenance : https://docs.google.com/spreadsheets/d/1KT_cgU_cV4bjNn8idsGFm4XpOZlbEW0T2IiSU_Ldp5s/edit
