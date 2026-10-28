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
