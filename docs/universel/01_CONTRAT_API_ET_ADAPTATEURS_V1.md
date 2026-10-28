# XONE — CONTRAT API MIDI NEUTRE ET ADAPTATEURS V1
Date : 28 octobre 2026. Ne crée aucune nouvelle adresse.

## Adresse stable et sortie indépendante des plugins
Le couple (famille, adresse virtuelle) identifie un paramètre XONE, pas un plugin. Emettre l'API virtuelle dès que l'action métier est valide, même sans MIDI Learn ni machine musicale attachée. Ne pas prétendre que le récepteur a effectivement appliqué la commande.
Pour adresse a dans 0..1023 : canal MIDI = 7 + floor(a/128), numéro = a modulo 128. Canaux 7..14 exclusivement pour MUSIC_OUT ; canal 15 pour LED K2 ; MIDI16 non attribué.
CC nominaux 0..930, target572 invalide. Notes d'état 0..543, SOFT 544..607, GROUP/LONG 608..679, TOP pulses 680..701. CUE 931..938 et LOAD 939..946 en CC ; DJ relatif 947..948. Le reste reste réservé selon API S7 V1.2.
LOOP : apiCC 569/570/571 = gains effectifs r1/r2/r3 ; facteur m de target573 interne, non destination audio indépendante. Recalcul selon état publié unique.
TOUCH/FLASH/TOP : préserver fronts NoteOn/Off, pas de disparition de pulse par déduplication ; dette OFF sur destination capturée. Pas de replay d'actions transitoires historiques au reopen.
PATCH/MEMORY/PRESET/RED/BASE/RESET émettent les CC et notes résultants dans les mêmes adresses nominales, sans opcode artificiel « PRESET ».

## Chemin A : AUM
AUM possède la matrice MIDI, MIDI Learn/MIDI Control et fan-out vers effets, bypass, volume, instruments AUv3 compatibles. C'est une opération de liaison ultérieure, pas une condition d'existence de la sortie virtuelle.

## Chemin B : machine MIDI
Un profil versionné convertit l'identité virtuelle vers les capacités effectivement vérifiées (canal/port, plage, CC, PC, SysEx, sens, defaults, retour). Un SH-01A ne partage pas automatiquement le profil JU-06A. Ne jamais envoyer de commandes Roland supposées.
Une même commande peut en principe cibler MIDI Learn AUM et adaptateur matériel configuré, sans dupliquer le coeur.

## Tests et statuts
K2_SIM_V3 -> XONE AU -> moniteur POST, sans instrument. Vérifier type, canal, numéro, valeur, ordre, sample offset, quantification, STOP/OFF, pickup, SAVE/RECALL et LED.
Distinguer DESIRED, TX attempted/sent, observation réelle et succès du service externe. Un port MIDI sans destinataire ne prouve pas une action réelle et ne constitue pas un échec du moteur universel.
RESET_DEFAULT inconnu => refus de cette demande seulement ; BASE local provisoire 0/OFF/MIX => décision explicitement autorisée sur son scope. Aucun reset au boot.

## Non-régression
GOLDEN CCOUT est une annexe historique non normative pour les offsets supplantés. Ne jamais remplacer S7 V1.2 par le remapping du banc technique L02 (CH1 CC4->74, CC8->71). Conserver sorties LED, ownership, exclusions S8 et tests SOURCE/CI.

Référence API : https://drive.google.com/file/d/18GJYIWt8MC0-pWFpl6TR0WYfcWe4fSbh/view
Référence native : https://drive.google.com/file/d/1FtvRVTBNo11sF_gxyydzLIxrKMybO06D/view
