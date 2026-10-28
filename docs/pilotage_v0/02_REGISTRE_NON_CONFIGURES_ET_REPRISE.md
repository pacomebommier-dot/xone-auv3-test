# XONE AUv3 — REGISTRE DES NON-CONFIGURÉS ET PROTOCOLE DE REPRISE
Version 1.0 — 28 octobre 2026 — document vivant, à lire à chaque reprise

## 1. Registre initial
| Sujet | État à retenir pour V0 | Comportement sûr attendu | Déblocage futur |
|---|---|---|---|
| Synthés / plugins / presets de référence | A_CONFIGURER | Aucun preset ni image instrument inventés ; session inconnue explicitement conservée | Choix et calibration par instrument |
| Destinations MIDI et profils incomplets | A_CONFIGURER | Refuser localement les commandes sans route validée ; ne pas bloquer les autres | Table de routage/profil confirmée |
| Sources et plugins audio AUM | A_CONFIGURER | Aucun routage audio fictif dans XONE ; pas de statut audio observé inventé | Configuration AUM par l'utilisateur |
| Defaults FILTER / BASE non qualifiés | A_CONFIGURER | UNKNOWN et refus avant mutation ; ne pas convertir en zéro | Références sûres et binding de profil |
| Banques PATCH non fournies / sauvegarde réelle | A_CONFIGURER / tâche 2 | INVALID tant que banque absente ; rappeler uniquement les enregistrements valides et configurés | Gestes SAVE et alimentation des banques |
| PRESET / RED / MEMORY exhaustifs | DIFFERE / tâches 2–3 | Préserver contrats et exclusions S8 ; ne pas simuler les rappels | Intégration sur profils et banques réels |
| DJ LOOP audio externe | DIFFERE | Distinguer demande service, remise, et observation externe ; ne pas fabriquer de lecture | Choix et raccordement de l'exécutant |
| Browser LOAD externe | DIFFERE | Demande à usage unique possible ; aucun chargement confirmé sans exécuteur | API de sélection/chargement effectivement disponible |
| LED secondaires et persistance complète | DIFFERE / tâches 4–5 | Maintenir le feedback déjà opérationnel et les états déjà sauvegardés | Intégration progressive par domaine |

> Ce registre décrit des **frontières**, pas des preuves de capacité. Mettre à jour seulement à partir des tests, du code effectivement présent et de décisions explicites. Il ne remplace pas les contrats musicaux détaillés.

## 2. Procédure obligatoire à chaque reprise Work / Codex
1. **Relire, dans cet ordre :** `00_CONTRAT_PILOTAGE_V0.md`, `01_CONTRAT_LIVRAISON_RECETTE_V0.md`, puis ce registre. C'est une lecture courte, pas un audit de tout le corpus.
2. Vérifier uniquement la branche active, HEAD local/distant, `git status`, les modifications non commitées et la dernière CI sur le SHA visé. Sauvegarder les modifications non publiées avant une opération Git destructive.
3. Identifier **le prochain obstacle BLOQUANT V0** au sens du contrat, ou un raccordement essentiel déjà existant à terminer. Éviter les explorations transversales non justifiées.
4. Réutiliser les contrats GOLDEN/S7/S8 pour **la fonction concernée seulement** ; ne pas les modifier. Tester le vrai chemin AU et les cas d'échec de cette fonction.
5. Commit fonctionnel limité → tests pertinents → CI complète du SHA de candidate. Ne pas déclarer PASS si la CI est simplement lancée ou incomplète.
6. Mettre à jour le registre en indiquant `preuve`, `reste`, `prochain obstacle`, **sans rouvrir tout le plan**. Enchaîner automatiquement si aucun arbitrage utilisateur n'est indispensable.

## 3. Format de bilan de fin de tranche (maximum 8 lignes)
`Branche/HEAD : ...`
`Objectif V0 traité : ...`
`Ce qui a été raccordé (réutilisé, non réécrit) : ...`
`Preuve locale : ...`
`CI sur SHA : ...`
`Restant BLOQUANT V0 : ...`
`A_CONFIGURER ou DIFFERE : ...`
`Prochaine action exacte : ...`

## 4. Conditions d'arrêt / d'escalade
S'arrêter pour décision humaine **seulement** si un choix musical nouveau doit être inventé, si deux contrats gelés se contredisent, si une opération risque de détruire du travail non publié, ou si aucune solution sûre n'existe pour un véritable BLOQUANT V0. Dans les autres cas, coder/tester/commiter et continuer.

Ne pas chercher à « finir 1/6 » au détriment de la V0. Conserver les compteurs des six tâches distincts du statut de livraison V0.

## 5. Production V0 — 28 octobre 2026
- Point de départ vérifié : local09b30ca, distant5e00032f ; CI09b30ca PASS40583029529. Aucun merge main.
- MACHINE et preuves non publiés conservés dans XONE_AUV3_REPRISE_20261027 ; copie de sécurité SAUVEGARDES/V0_AVANT_PRODUCTION_20261028. Candidate isolée dans XONE_AUV3_V0_CANDIDATE depuis5e00032f ; MACHINE DIFFERE, non intégré.
- BLOQUANT V0 traité : reprise STOP réémettait un timestamp ancien. Test reproduit FAIL off.sample==30, puis PASS après rattachement au début du buffer courant.
- RACCORDABLE V0 : fixture explicite du harnais AU pour tester CC/SPOT, sans default de production ; interface et recette corrigées. Désallocation annule les attentes transitoires et préserve les OFF dus pour réallocation. Le diagnostic TOP_RESET_DEFERRED dispose de son libellé.
- Preuve locale : contrôles sources/simulateur/profils PASS ; test STOP ciblé PASS. Vrai harnais AU/Apple/iOS : vérifier la CI du SHA COMMIT.txt avant de qualifier la candidate.
- Reste : CI complète et artefact iOS de ce SHA ; signature et tous essais AUM NON_TESTE_AUM. Retrait définitif : STOP/OFF observés avant destruction, aucun callback hors rendu supposé.
- Paquet de recette : docs/reprise/RECETTE_AUM_V02.md et Mozaic/K2_SIM_V3.txt joints à l’artefact avec les sources exactes pour signature Xcode. Avancement des six tâches indépendant, inchangé.

- CI1e49ecc /40667162494 : GCC, Clang, Apple Clang et ASan/UBSan PASS ; harnais AU refusé à la compilation car une lambda C++ ne peut capturer une variable ObjC __block. Correction limitée au test : tableau de timestamps à durée de vie du harnais, pointeur capturé comme la fixture de grille existante. Aucun test retiré ; relancer toute la CI sur le SHA corrigé. Pas d’IPA sur ce premier essai.

- CIe9a6417 /40668083541 : GCC/Clang PASS ; runner macos-15-intel non attribué après plus de7min. Production basculée vers le pool standard macos-15 Apple Silicon ; mêmes suites/ASan/UBSan/harnais/build/intégrité, harnais placé en premier pour retour ciblé rapide. Aucun contrôle désactivé.

- CIfb3334f /40669262587 : GCC/Clang PASS ; harnais AU compilé puis assertion de réallocation FAIL. Cause observée : le superclass AU efface MIDIOutputEventBlock à la désallocation (déjà visible dans la trace historique). Le harnais doit simuler la reconnexion par l’hôte. Scénario renforcé : premier rendu sans sortie conserve la dette ; réallocation avec callback reconnecté émet l’OFF au timestamp courant, puis aucun doublon. Code moteur inchangé dans cette correction de test ; nouvelle CI complète requise.

## 6. Bilan de livraison candidate — 28 octobre2026
Branche/HEAD binaire : feature/native-e3-bar-beat-reprise /2cedd1eefe74db4fef4ea44764164391d3118911.
Objectif V0 traité : candidate iOS AUv3 construite et paquet de recette publié ; voir03_LIVRAISON_CANDIDATE_AUM.md.
Raccordement réutilisé : restauration LIVE pour fixture explicite ; STOP existant, timestamps courants et dette conservée à réallocation ; aucun moteur nouveau.
Preuve locale : test STOP FAIL reproduit puis PASS ; intégrité/simulateur/profils PASS ; sauvegarde vérifiée438 fichiers + patch identique.
CI sur SHA : PASS40671623461 — GCC/Clang, Apple Clang, ASan/UBSan, vraie AU macOS, build iOS et intégrité ; artefact12808951685 téléchargé et vérifié.
Restant BLOQUANT V0 logiciel : aucun échec constaté dans ces contrôles ; signature/install et AUM restent non réalisés, STOP/OFF avant retrait définitif requis.
A_CONFIGURER / DIFFERE : tableau du rapport ; MACHINE préservé hors candidate et six tâches inchangées.
Prochaine action exacte : signer app+extension depuis ce SHA, installer, créer la session minimale AUM et consigner les sept essais réels. NON_TESTE_AUM.
