# Recette AUM / iPhone / K2 / SH — préparation du 4 novembre2026

Statut : protocole préparé, **aucun essai physique exécuté par cette reprise**. Branche reprise/global-20261104, PR17 draft. Ne qualifier que le SHA réellement installé.

## Préparation

Conserver une copie de la sessionAUM. Installer le build iOS du SHA après CI Apple, signature par le propriétaire de l'appareil. K2 physique OU K2_SIM, source exclusive, canal15. Dans XONE choisir «Configurer SH-01A · essais», source1(INST1) à4(YELLOW2) et canal du Roland (1 par exemple, jamais15).

Activer CCOUTPUT sur le SH selon son manuel. Route SH OUT→XONE IN et route XONE OUT→SH IN, filtre sur le canalSH. Ne pas faire revenir automatiquement XONE OUT vers son entrée ; pas de MIDI Thru générique. Conserver les canauxS7 7..14 vers moniteur/destinationsAUM et LED15 vers K2. Le câbleAU0 est une frontière configurée, pas une authentification de l'appareil.

Cette première recette n'affecte au SH que FILTER1/2/3 de la MACHINE impaire associée : VCF FREQ CC74, RES71, MOD25 (référenceV27). Les autres commandes universelles restent virtuelles ; les paramètres non affectés sont observés et sauvegardés silencieusement dans les36du miroir. Pas de macro A/D ni d'énumération discrète supposée.

## Campagne SH36

| Test | Action | Critère / preuve à conserver | État |
|---|---|---|---|
| T01 | Rappeler programmeSH1, observer PC0 puisCC | SH_MIRROR_36 dans diagnostic, liste exacte36numéros/valeurs sur une seule epoch | NON_TESTE matériel |
| T02 | RappelerSH2/3 puis4et64 | Seuls miroirs2/3 changent ; PC3/63 actualisentLIVE sans miroir | NON_TESTE matériel |
| T03 | Interrompre rafale, STOP, changementPC, mauvaise route | SH_CAPTURE_INCOMPLETE ; ancien miroir conservé, pas de fauxCURRENT | NON_TESTE matériel |
| T04 | Doublons, ordre, PCécho ; conserver relevé brut | Pas de faux36/36 ; le PCrépété peut annuler une collecte, jamais les additionner | NON_TESTE matériel |
| T05 | TournerVCF depuisK2 puisSH | OctetsTX74 corrects, LIVE/LED/pickup cohérents ; miroir non modifié hors rappel | NON_TESTE matériel |
| T06 | Éditer parK2, WRITE physiqueSH, quitter/rappeler | Valeur reçue après rappel conservée parWRITE ; nouveau miroir, aucun SAVE XONE | NON_TESTE matériel |
| T07 | TOP1..3 et CUE/LOAD depuisXONE en marche puis arrêt | PCseul0..2 au vraiBAR, immédiat si arrêt prouvé ; aucune rafaleCC36 émise | NON_TESTE matériel |
| T08 | GesteSAVE surSH puis autre profil | Aucun WRITE/miroir/succèsSAVE SH ; autre instrument conserve SAVE/RECALL | NON_TESTE matériel |
| T09 | Save/reopenAUM, réallocation/reconnexion | MiroirsVALID conservés, capturevolatile perdue ; aucunPC/CC36/WRITE rejoué | NON_TESTE matériel |
| T10 | Même programme puis aller-retour manuel | Établir si la resélection émet36 ; sinon quitter/rappeler manuellement, jamais automatiquement | NON_TESTE matériel |

Table provisoire36 :3,5,12–31,71–75,78–83,85–87. Hypothèse autorisée par l'utilisateur, non certificationRoland du contenu exact d'une rafale. Capturer les numéros réellement reçus avant toute correction de table ; ne pas accepter36événements arbitraires. Les valeurs0/127 sont des octets bruts, pas une calibration des sélecteurs.

## V0 / temps réel et fin de vie

Tester257/1025/4097 événements dans le harnessApple avant campagneAUM, UP/STOP en queue. Surappareil : interfaceouverte/fermée, sessionlégère puis chargée, navigation+CC+LED, PATCH/PRESET/RED, STOP, retraitAU après vraisUP/OFF, réallocation, échec/reprise de route et save/reopen. Noter iPhone/iPad, iOS, AUM, firmwareSH, interfaceMIDI, Hz, frames, CPUrepos/pics, xruns, tracesPRE/POST etSHA. Le dernier retraitAU sans nouveau rendu ne garantit pas l'émission desOFF : STOP et vérifierOFF avant suppression.

BASE matérielSH est encore sans référence : refus UNKNOWN_CONFIG attendu, pas de PCBASE ni zéro Roland inventé. Les modesSPOT S8 restent horsPATCH/BASE/PRESET/RED selon décision du19/10. Les testsMACHINE MP01..MP07 seront consignés après intégration, sans les annoncer exécutés ici.
