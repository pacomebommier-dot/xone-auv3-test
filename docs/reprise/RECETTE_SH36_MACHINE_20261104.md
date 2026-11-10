# Recette AUM / iPhone / K2 / SH — préparation du 4 novembre2026

Statut : protocole préparé, **aucun essai physique exécuté par cette reprise**. Depuis SOC-06, poursuivre la recette avec la **PR #24** `prep/ipa-soc01-05-recette-20261109`, et uniquement le SHA réellement signé/installé. Les mentions de PR17 ci-dessous étaient historiques.

## Préparation

Conserver une copie de la sessionAUM. Installer le build iOS du SHA après CI Apple, signature par le propriétaire de l'appareil. K2 physique OU K2_SIM, source exclusive, canal15. Dans XONE choisir «Configurer SH-01A · essais», source1(INST1) à4(YELLOW2) et canal dédié du Roland (1–6 ou16, 1 par exemple ; 7–14 réservés S7 et15 K2).

Activer CCOUTPUT sur le SH selon son manuel. Route SH OUT→XONE IN et route XONE OUT→SH IN, filtre sur le canalSH. Ne pas faire revenir automatiquement XONE OUT vers son entrée ; pas de MIDI Thru générique. Conserver les canauxS7 7..14 vers moniteur/destinationsAUM et LED15 vers K2. Le câbleAU0 est une frontière configurée, pas une authentification de l'appareil.

Cette première recette n'affecte au SH que FILTER1/2/3 de la MACHINE impaire associée : VCF FREQ CC74, RES71, MOD25 (référenceV27). Les autres commandes universelles restent virtuelles ; les paramètres non affectés sont observés et sauvegardés silencieusement dans les36du miroir. Pas de macro A/D ni d'énumération discrète supposée.

## Campagne SH36

Correspondance du prochain patch. Programme 1 = BASE (PC0), programmes 2–4 = PATCH XONE 1–3 (PC1–3). Vérifier la version installée. Les builds antérieurs utilisent encore l’ancienne correspondance PC0–2.

| Test | Action | Critère / preuve à conserver | État |
|---|---|---|---|
| T01 | Rappeler le programme 2, observer PC1 puis CC | SH_MIRROR_36 dans diagnostic, liste exacte36numéros/valeurs sur une seule epoch | NON_TESTE matériel |
| T02 | Rappeler les programmes 3 et 4, puis 1, 5 et 64 | Seuls miroirs2/3 changent ; PC0/4/63 actualisent LIVE hors des miroirs | NON_TESTE matériel |
| T03 | Interrompre rafale, STOP, changementPC, mauvaise route | SH_CAPTURE_INCOMPLETE avec PC, compteur, numéros manquants et supplémentaires reçus sur la route SH pendant la collecte. Ancien miroir conservé | NON_TESTE matériel |
| T04 | Doublons, ordre, PCécho ; conserver relevé brut | Pas de faux36/36 ; le PCrépété peut annuler une collecte, jamais les additionner | NON_TESTE matériel |
| T05 | TournerVCF depuisK2 puisSH | OctetsTX74 corrects, LIVE/LED/pickup cohérents ; miroir non modifié hors rappel | NON_TESTE matériel |
| T06 | Éditer parK2, WRITE physiqueSH, quitter/rappeler | Valeur reçue après rappel conservée parWRITE ; nouveau miroir, aucun SAVE XONE | NON_TESTE matériel |
| T07 | TOP1..3 et CUE/LOAD depuisXONE en marche puis arrêt | PC seul 1..3 au vrai BAR, immédiat si arrêt prouvé ; aucune rafaleCC36 émise | NON_TESTE matériel |
| T08 | LONG sur slot de PATCH SH, puis autre profil non-SH ; comparer TOP et CUE/LOAD | Sur SH : **aucun WRITE, PC de rappel, miroir modifié ou faux succès SAVE** ; sur profil non-SH : LONG = SAVE/remplacement. | NON_TESTE matériel |
| T09 | Save/reopenAUM, réallocation/reconnexion | MiroirsVALID conservés, capturevolatile perdue ; aucunPC/CC36/WRITE rejoué | NON_TESTE matériel |
| BASE | WRITE physique sur le programme 1, CUE4 long puis court | Long enregistre les FX locaux. Court envoie PC0 et restaure les FX. Les miroirs PATCH 1–3 sont conservés. | NON_TESTE matériel |
| BASE SH entrant | Rappeler le programme 1 depuis le Roland, puis tester un rappel CUE4 avec retour PC0 | Le rappel physique applique les FX BASE et les CC reçus actualisent le LIVE. Le retour d’un CUE4 ne réapplique pas les FX et ne réémet aucun PC. Les trois miroirs restent inchangés. | NON_TESTE matériel |
| T10 | Même programme puis aller-retour manuel | Établir si la resélection émet36 ; sinon quitter/rappeler manuellement, jamais automatiquement | NON_TESTE matériel |
| T11 · SOC-06 | Faire TOP/CUE/LOAD court sur PATCH SH renseigné, puis LONG ≥200ms sur le même slot, avec PRE/POST MIDI ; CUE4 long/court | COURT : PC1..3 selon sélection ; LONG : **aucun PC, aucun WRITE et aucun SAVE sonore**. CUE4 LONG capture seulement référence BASE + FX XONE ; CUE4 court rappelle PC0 selon le contrat. Le vrai WRITE s'exécute uniquement sur le SH. | NON_TESTE matériel |

Table provisoire36 :3,5,12–31,71–75,78–83,85–87. Hypothèse autorisée par l'utilisateur, non certificationRoland du contenu exact d'une rafale. Capturer les numéros réellement reçus avant toute correction de table ; ne pas accepter36événements arbitraires. Les valeurs0/127 sont des octets bruts, pas une calibration des sélecteurs.

**Attention SOC-06 :** les accords historiques BOT2+TOP (SAMPLE) et BOT3+TOP (MACHINE) sont supprimés. **TOP seul court** rappelle un PATCH renseigné (premier SAVE s'il est vide et non-SH) ; **TOP seul long** remplace un PATCH non-SH. Le SH ne doit **jamais** recevoir un WRITE depuis XONE et les nouveaux gestes ne doivent pas déclencher de PC de rappel lors d'un LONG.

## V0 / temps réel et fin de vie

Tester257/1025/4097 événements dans le harnessApple avant campagneAUM, UP/STOP en queue. Surappareil : interfaceouverte/fermée, sessionlégère puis chargée, navigation+CC+LED, PATCH/PRESET/RED, STOP, retraitAU après vraisUP/OFF, réallocation, échec/reprise de route et save/reopen. Noter iPhone/iPad, iOS, AUM, firmwareSH, interfaceMIDI, Hz, frames, CPUrepos/pics, xruns, tracesPRE/POST etSHA. Le dernier retraitAU sans nouveau rendu ne garantit pas l'émission desOFF : STOP et vérifierOFF avant suppression.

BASE matérielSH est encore sans référence : refus UNKNOWN_CONFIG attendu, pas de PCBASE ni zéro Roland inventé. Les modesSPOT S8 restent horsPATCH/BASE/PRESET/RED selon décision du19/10. Les tests MP01..MP06 natifs sont intégrés ; MP07 Apple et AUM/K2 restent à exécuter.


## Campagne MACHINE paires — MP07 matériel

Pour chacune des paires 1/2, 3/4, 5/6, 7/8 : entrer par SHORT sur la page paire ; comparer les 12 rotatifs et les 12 SPOT aux quatre sous-pages FX natives de la source associée. LONG sur la paire active : vue empruntée ; LONG suivant : native. La bascule seule ne doit émettre aucun changement musical. Comparer aussi LED, RESET secondaire, pickup dans les deux sens et SPOT maintenu pendant une bascule.

Vérifier que POTs/faders restent MACHINE, que SAVE/RECALL PATCH laisse tous les FX alias inchangés, et que MEMORY/PRESET/RED retrouvent le même état FX depuis les deux accès. STOP doit fermer chaque NoteOff dû, y compris après changement de page/vue ; réouverture sans émission spontanée. Conserver traces MIDI avant/après, vidéo LED et SHA. Toute cette campagne reste NON_TESTE matériel.
